#include "conv_pipeline.h"
#include "cli.h"
#include "conv.h"
#include "thread_pool.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct reader_args {
  struct tpool_s *pool;
  struct main_args *args;
  BMP **results;
  KernelMatrix *kernel;
  size_t num;
};
struct computer_args {
  struct tpool_s *pool;
  BMP *result;
  BMP *input_image;
  struct pixel_s *pixels;
  KernelMatrix *kernel;
  size_t cord;
};

static void task_writer_columns(void *args) {
  struct computer_args *task_args = (struct computer_args *)args;

  BMP *output_image = task_args->result;
  const struct pixel_s *pixels = task_args->pixels;

  const size_t xcord = task_args->cord;

  const size_t height = get_height(output_image);
  const size_t width = get_width(output_image);

  for (size_t ycord = 0; ycord < height; ++ycord) {
    const struct pixel_s cell = pixels[(ycord * width) + xcord];
    set_pixel_rgb(output_image, xcord, ycord, cell.r, cell.g, cell.b);
  }
  free(task_args);
}
static void task_computer_columns(void *args) {
  const struct computer_args *task_args = (struct computer_args *)args;

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

static void task_reader_columns(void *args) {
  struct reader_args *task_args = (struct reader_args *)args;
  const size_t num = task_args->num;

  BMP *input_image = bopen(task_args->args->arr_input[num]);
  BMP *result_image = b_deep_copy(input_image);
  task_args->results[num] = result_image;

  const size_t width = get_width(input_image);
  const size_t height = get_height(input_image);
  struct pixel_s *pixels = malloc(height * width * sizeof(struct pixel_s));
  if (!pixels) {
    flockfile(stderr);
    fputs("Error in thread: ", stderr);
    fputs("Can allocate memory", stderr);
    fputc('\n', stderr);
    funlockfile(stderr);
    free(task_args);
    return;
  }

  for (size_t i = 0; i < width; ++i) {
    struct computer_args *cargs = malloc(sizeof(struct computer_args));
    cargs->pool = task_args->pool;
    cargs->input_image = input_image;
    cargs->result = result_image;
    cargs->pixels = pixels;
    cargs->kernel = task_args->kernel;
    cargs->cord = i;
    /* struct tpool_s *pool; */
    /* BMP *input_image; */
    /* BMP *output_image; */
    /* struct pixel_s *pixels; */
    /* KernelMatrix *kernel; */
    /* size_t cord */;
    task_queue_add_task(task_args->pool, COMPUTER, task_computer_columns,
                        cargs);
  }
}

BMP **apply_filter_pipeline_columns(struct main_args *args,
                                    KernelMatrix *kernel,
                                    struct tpool_s *pool) {
  struct reader_args *task_args =
      malloc(args->num_of_inputs * sizeof(struct reader_args));
  if (task_args == NULL) {
    return NULL;
  }
  BMP **results = malloc(sizeof(args->num_of_inputs * sizeof(BMP *)));
  if (!results) {
    free(task_args);
    return NULL;
  }

  for (size_t i = 0; i < args->num_of_inputs; i++) {
    task_args[i].pool = pool;
    task_args[i].kernel = kernel;
    task_args[i].results = results;
    task_args[i].args = args;
    task_args[i].num = i;
  }

  for (size_t i = 0; i < args->num_of_inputs; i++) {
    task_queue_add_task(pool, READER, task_reader_columns, &task_args[i]);
  }
  tpool_wait(pool);
  free(task_args);
  return results;
}
int32_t apply_filter_pipeline(struct main_args *args) {
  size_t thread_count = thread_process_count();
  struct tpool_s *pool = tpool_init(thread_count);
  if (!pool) {
    fputs("Cannot create thread pool\n", stderr);
    return false;
  }
  KernelMatrix *kernel = choose_kernel_matrix(args->filter_option);
  if (kernel == NULL) {
    tpool_destroy(pool);
    fputs("No such kernel", stderr);
    return false;
  }
  BMP **results = NULL;
  switch (args->mode_option) {
  case SEQUENTIALLY:
  case COLUMNS:
    results = apply_filter_pipeline_columns(args, kernel, pool);
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

  if (results) {
    for (size_t i = 0; i < args->num_of_inputs; i++) {
      const size_t output_len = strlen(args->arr_input[i]);
      char buffer[output_len + 2];
      from_input_to_output(args->arr_input[i], output_len, buffer);
      bwrite(results[i], buffer);
      bclose(results[i]);
    }
    free(results);
  }

  tpool_destroy(pool);
  free_kernel_matrix(kernel);
  return true;
}
