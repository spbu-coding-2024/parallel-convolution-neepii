#include "conv_pipeline.h"
#include "cli.h"
#include "conv.h"
#include "thread_pool.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct task_args {
  struct tpool_s *pool;
  BMP *input_image;
  BMP *output_image;
  struct pixel_s *pixels;
  KernelMatrix *kernel;
  size_t cord;
};

static void task_writer_columns(void *args) {
  const struct task_args *task_args = (struct task_args *)args;

  BMP *output_image = task_args->output_image;
  const struct pixel_s *pixels = task_args->pixels;

  const size_t xcord = task_args->cord;

  const size_t height = get_height(output_image);
  const size_t width = get_width(output_image);

  for (size_t ycord = 0; ycord < height; ++ycord) {
    const struct pixel_s cell = pixels[(ycord * width) + xcord];
    set_pixel_rgb(output_image, xcord, ycord, cell.r, cell.g, cell.b);
  }
}
static void task_computer_columns(void *args) {
  const struct task_args *task_args = (struct task_args *)args;

  BMP *image = task_args->input_image;
  struct pixel_s *output_pixels = task_args->pixels;
  struct tpool_s *pool = task_args->pool;

  const KernelMatrix *kernel = task_args->kernel;
  const size_t xcord = task_args->cord;
  const size_t height = get_height(image);

  for (size_t ycord = 0; ycord < height; ++ycord) {
    convolute_pixel_sequentialy(image, xcord, ycord, kernel, output_pixels);
  }
  task_queue_add_task(pool, WRITER, task_writer_columns, args);
}

/* static void task_reader_columns(void *args) { */
/*   struct task_args *task_args = (struct task_args *)args; */

/*   BMP *input_image = task_args->input_image; */
/*   struct tpool_s *pool = task_args->pool; */

/*   const size_t height = get_height(input_image); */

/*   for (size_t ycord = 0; ycord < height; ++ycord) { */
/*     uint8_t red; */
/*     uint8_t green; */
/*     uint8_t blue; */
/*     get_pixel_rgb(image, xcord, ycord, &red, &green, &blue); */
/*     input_pixels[(ycord * width) + xcord].r = red; */
/*     input_pixels[(ycord * width) + xcord].g = green; */
/*     input_pixels[(ycord * width) + xcord].b = blue; */
/*   } */
/*   task_queue_add_task(pool, COMPUTER, task_computer_columns, args); */
/* } */

int32_t apply_filter_pipeline_columns(BMP *image, KernelMatrix *kernel,
                                      struct tpool_s *pool) {
  uint8_t exit_code = true;
  BMP *output_image = b_deep_copy(image);
  if (!output_image) {
    return false;
  }

  const size_t height = get_height(image);
  const size_t width = get_width(image);
  struct pixel_s *pixels = malloc(height * width * sizeof(struct pixel_s));
  if (pixels == NULL) {
    exit_code = false;
    goto pixels_free;
  }
  struct task_args *task_args = malloc(width * sizeof(struct task_args));
  if (task_args == NULL) {
    exit_code = false;
    goto task_free;
  }

  for (size_t i = 0; i < width; i++) {
    task_args[i].pool = pool;
    task_args[i].kernel = kernel;
    task_args[i].input_image = image;
    task_args[i].output_image = output_image;
    task_args[i].pixels = pixels;
    task_args[i].cord = i;
  }

  for (size_t xcord = 0; xcord < width; xcord++) {
    task_queue_add_task(pool, COMPUTER, task_computer_columns,
                        &task_args[xcord]);
  }
  tpool_wait(pool);
  *image = *output_image;

  free(output_image);
task_free:
  free(task_args);
pixels_free:
  free(pixels);
  return exit_code;
}
int32_t apply_filter_pipeline(BMP *image, struct main_args args) {
  size_t thread_count = thread_process_count();
  struct tpool_s *pool = tpool_init(thread_count);
  if (!pool) {
    fputs("Cannot create thread pool\n", stderr);
    return false;
  }
  KernelMatrix *kernel = choose_kernel_matrix(args.filter_option);
  if (kernel == NULL) {
    tpool_destroy(pool);
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

  free_kernel_matrix(kernel);
  return true;
}
