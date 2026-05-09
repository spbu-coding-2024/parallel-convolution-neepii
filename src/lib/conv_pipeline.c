#include "conv_pipeline.h"
#include "cli.h"
#include "conv.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "thread_pool.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct task_args {
  struct tpool_s *pool;
  BMP *image;
  KernelMatrix *kernel;
  struct pixel_s *input_pixels;
  struct pixel_s *output_pixels;
  size_t cord;
};

static void task_writer_columns(void *args) {
  const struct task_args *task_args = (struct task_args *)args;

  BMP *image = task_args->image;
  struct pixel_s *pixels = task_args->output_pixels;

  const size_t xcord = task_args->cord;

  const size_t height = get_height(image);
  const size_t width = get_width(image);

  for (size_t ycord = 0; ycord < height; ++ycord) {
    const struct pixel_s cell = pixels[(ycord * width) + xcord];
    set_pixel_rgb(image, xcord, ycord, cell.r, cell.g, cell.b);
  }
}
static void task_computer_columns(void *args) {
  const struct task_args *task_args = (struct task_args *)args;

  BMP *image = task_args->image;
  struct pixel_s *input_pixels = task_args->input_pixels;
  struct pixel_s *output_pixels = task_args->output_pixels;
  struct tpool_s *pool = task_args->pool;

  const KernelMatrix *kernel = task_args->kernel;
  const size_t xcord = task_args->cord;
  const size_t height = get_height(image);
  const size_t width = get_width(image);
  const size_t ker_size = kernel->size;

  for (size_t ycord = 0; ycord < height; ++ycord) {
    int64_t red = 0;
    int64_t green = 0;
    int64_t blue = 0;
    for (size_t offset_xcord = 0; offset_xcord < ker_size; ++offset_xcord) {
      for (size_t offset_ycord = 0; offset_ycord < ker_size; ++offset_ycord) {
        const int32_t temp_xcord =
            apply_mirror_padding(xcord + offset_xcord - (ker_size / 2), width);
        const int32_t temp_ycord =
            apply_mirror_padding(ycord + offset_ycord - (ker_size / 2), height);
        const struct pixel_s temp_cell =
            input_pixels[(temp_ycord * width) + temp_xcord];
        red += kernel->mtx[0][(offset_ycord * ker_size) + offset_xcord] *
               temp_cell.r;
        green += kernel->mtx[1][(offset_ycord * ker_size) + offset_xcord] *
                 temp_cell.g;
        blue += kernel->mtx[2][(offset_ycord * ker_size) + offset_xcord] *
                temp_cell.b;
      }
    }

    red = multiply_by_rational_and_ceil(red, 1, kernel->denominator_coef);
    green = multiply_by_rational_and_ceil(green, 1, kernel->denominator_coef);
    blue = multiply_by_rational_and_ceil(blue, 1, kernel->denominator_coef);

    output_pixels[(ycord * width) + xcord].r = to_byte(red);
    output_pixels[(ycord * width) + xcord].g = to_byte(green);
    output_pixels[(ycord * width) + xcord].b = to_byte(blue);
  }
  task_queue_add_task(pool, WRITER, task_writer_columns, args);
}

static void task_reader_columns(void *args) {
  struct task_args *task_args = (struct task_args *)args;

  BMP *image = task_args->image;
  struct pixel_s *input_pixels = task_args->input_pixels;
  struct tpool_s *pool = task_args->pool;

  const size_t xcord = task_args->cord;
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  for (size_t ycord = 0; ycord < height; ++ycord) {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    get_pixel_rgb(image, xcord, ycord, &red, &green, &blue);
    input_pixels[(ycord * width) + xcord].r = red;
    input_pixels[(ycord * width) + xcord].g = green;
    input_pixels[(ycord * width) + xcord].b = blue;
  }
  if (xcord == 0) {
    return;
  }
  if (xcord == width - 1) {
    task_queue_add_task(pool, COMPUTER, task_computer_columns, args);
    return;
  }
  struct task_args *base_args = (struct task_args *)args;
  task_queue_add_task(pool, COMPUTER, task_computer_columns,
                      (void *)&base_args[-1]);
}

int32_t apply_filter_pipeline_columns(BMP *image, KernelMatrix *kernel,
                                      struct tpool_s *pool) {
  uint8_t exit_code = true;

  const size_t height = get_height(image);
  const size_t width = get_width(image);
  const size_t ker_size = kernel->size;
  if (ker_size != 3) {
    fputs("kernel size != 3", stderr);
    exit_code = false;
    goto kernel_free;
  }

  struct pixel_s *input_pixels =
      malloc(height * width * sizeof(struct pixel_s));
  if (input_pixels == NULL) {
    exit_code = false;
    goto input_free;
  }
  struct pixel_s *output_pixels =
      malloc(height * width * sizeof(struct pixel_s));
  if (output_pixels == NULL) {
    exit_code = false;
    goto output_free;
  }
  struct task_args *task_args = malloc(width * sizeof(struct task_args));
  if (task_args == NULL) {
    exit_code = false;
    goto task_free;
  }

  for (size_t i = 0; i < width; i++) {
    task_args[i].pool = pool;
    task_args[i].image = image;
    task_args[i].kernel = kernel;
    task_args[i].input_pixels = input_pixels;
    task_args[i].output_pixels = output_pixels;
    task_args[i].cord = i;
  }

  for (size_t xcord = 0; xcord < width; xcord++) {
    task_queue_add_task(pool, READER, task_reader_columns, &task_args[xcord]);
  }

  tpool_wait(pool);
task_free:
  free(task_args);
output_free:
  free(output_pixels);
input_free:
  free(input_pixels);
kernel_free:
  free_kernel_matrix(kernel);
  return exit_code;
}
int32_t apply_filter_pipeline(BMP *image, struct main_args args) {
  size_t thread_count = thread_process_count();
  if (thread_count < 3) {
    fputs("Thread count must be atleast 3\n", stderr);
    return false;
  }
  struct tpool_s *pool = tpool_init(thread_count);
  if (!pool) {
    fputs("Cannot create thread pool\n", stderr);
    return false;
  }
  KernelMatrix *kernel = choose_kernel_matrix(args.filter_option);
  if (kernel == NULL) {
    fputs("No such kernel", stderr);
    return false;
  }

  switch (args.mode_option) {
  case SEQUENTIALLY:
  case COLUMNS:
    apply_filter_pipeline_columns(image, kernel, pool);
    break;
  case ROWS:
    fputs("UNIMPL\n", stderr);
    break;
  case PIXEL_BY_PIXEL:
    fputs("UNIMPL\n", stderr);
    break;
  case BLOCK:
    fputs("UNIMPL\n", stderr);
    break;
  }
  tpool_destroy(pool);
  return true;
}
