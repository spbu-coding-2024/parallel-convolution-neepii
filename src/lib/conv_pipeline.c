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
  size_t x;
  size_t y;
  size_t height;
  size_t width;
};

static int32_t prepare_image(struct reader_args *task_args, BMP **input_image,
                             BMP **result_image, struct pixel_s **pixels) {
  const size_t num = task_args->num;

  *input_image = bopen(task_args->args->arr_input[num]);
  *result_image = b_deep_copy(*input_image);
  task_args->results[num] = *result_image;

  const size_t width = get_width(*input_image);
  const size_t height = get_height(*input_image);
  *pixels = malloc(height * width * sizeof(struct pixel_s));
  if (!pixels) {
    flockfile(stderr);
    fputs("Error in thread: ", stderr);
    fputs("Can allocate memory", stderr);
    fputc('\n', stderr);
    funlockfile(stderr);
    free(task_args);
    return false;
  }
  return true;
}

static struct computer_args *init_cargs(struct reader_args *task_args,
                                        BMP *input_image, BMP *result_image,
                                        struct pixel_s *pixels) {
  struct computer_args *cargs = malloc(sizeof(struct computer_args));
  if (!cargs) {
    flockfile(stderr);
    fputs("Error in thread: ", stderr);
    fputs("Can allocate memory", stderr);
    fputc('\n', stderr);
    funlockfile(stderr);
    return NULL;
  }
  cargs->pool = task_args->pool;
  cargs->input_image = input_image;
  cargs->result = result_image;
  cargs->pixels = pixels;
  cargs->kernel = task_args->kernel;
  return cargs;
}

static void task_writer_columns(void *args) {
  struct computer_args *targs = (struct computer_args *)args;
  const size_t height = get_height(targs->result);
  const size_t width = get_width(targs->result);

  for (size_t ycord = 0; ycord < height; ++ycord) {
    const struct pixel_s cell = targs->pixels[(ycord * width) + targs->x];
    set_pixel_rgb(targs->result, targs->x, ycord, cell.r, cell.g, cell.b);
  }
  free(targs);
}

static void task_computer_columns(void *args) {
  const struct computer_args *targs = (struct computer_args *)args;
  const size_t height = get_height(targs->input_image);

  for (size_t ycord = 0; ycord < height; ++ycord) {
    convolute_pixel_sequentialy(targs->input_image, targs->x, ycord,
                                targs->kernel, targs->pixels);
  }
  task_queue_add_task(targs->pool, WRITER, task_writer_columns, args);
}

static void task_reader_columns(void *args) {
  struct reader_args *targs = (struct reader_args *)args;
  BMP *input_image;
  BMP *result_image;
  struct pixel_s *pixels = {NULL};
  if (!prepare_image(targs, &input_image, &result_image, &pixels)) {
    return;
  };

  const size_t width = get_width(input_image);
  for (size_t i = 0; i < width; ++i) {
    struct computer_args *cargs =
        init_cargs(targs, input_image, result_image, pixels);
    cargs->x = i;
    task_queue_add_task(targs->pool, COMPUTER, task_computer_columns, cargs);
  }
}

static void task_writer_rows(void *args) {
  struct computer_args *targs = (struct computer_args *)args;
  const size_t width = get_width(targs->result);

  for (size_t xcord = 0; xcord < width; ++xcord) {
    const struct pixel_s cell = targs->pixels[(targs->y * width) + xcord];
    set_pixel_rgb(targs->result, xcord, targs->y, cell.r, cell.g, cell.b);
  }
  free(targs);
}

static void task_computer_rows(void *args) {
  const struct computer_args *targs = (struct computer_args *)args;
  const size_t width = get_width(targs->input_image);

  for (size_t xcord = 0; xcord < width; ++xcord) {
    convolute_pixel_sequentialy(targs->input_image, xcord, targs->y,
                                targs->kernel, targs->pixels);
  }
  task_queue_add_task(targs->pool, WRITER, task_writer_rows, args);
}

static void task_reader_rows(void *args) {
  struct reader_args *targs = (struct reader_args *)args;
  BMP *input_image;
  BMP *result_image;
  struct pixel_s *pixels = {NULL};
  if (!prepare_image(targs, &input_image, &result_image, &pixels)) {
    return;
  };

  const size_t height = get_height(input_image);
  for (size_t i = 0; i < height; ++i) {
    struct computer_args *cargs =
        init_cargs(targs, input_image, result_image, pixels);
    cargs->y = i;
    task_queue_add_task(targs->pool, COMPUTER, task_computer_rows, cargs);
  }
}

static void task_writer_pixel_by_pixel(void *args) {
  struct computer_args *targs = (struct computer_args *)args;
  const size_t width = get_width(targs->result);

  const struct pixel_s cell = targs->pixels[(targs->y * width) + targs->x];
  set_pixel_rgb(targs->result, targs->x, targs->y, cell.r, cell.g, cell.b);

  free(targs);
}

static void task_computer_pixel_by_pixel(void *args) {
  const struct computer_args *targs = (struct computer_args *)args;
  convolute_pixel_sequentialy(targs->input_image, targs->x, targs->y,
                              targs->kernel, targs->pixels);
  task_queue_add_task(targs->pool, WRITER, task_writer_pixel_by_pixel, args);
}

static void task_reader_pixel_by_pixel(void *args) {
  struct reader_args *targs = (struct reader_args *)args;
  BMP *input_image;
  BMP *result_image;
  struct pixel_s *pixels = {NULL};
  if (!prepare_image(targs, &input_image, &result_image, &pixels)) {
    return;
  };

  const size_t height = get_height(input_image);
  const size_t width = get_width(input_image);
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      struct computer_args *cargs =
          init_cargs(targs, input_image, result_image, pixels);
      cargs->y = i;
      cargs->x = j;
      task_queue_add_task(targs->pool, COMPUTER, task_computer_pixel_by_pixel,
                          cargs);
    }
  }
}

static void task_writer_block(void *args) {
  struct computer_args *targs = (struct computer_args *)args;
  const size_t width = get_width(targs->result);

  for (size_t xcord = targs->x; xcord < targs->x + targs->width; ++xcord) {
    for (size_t ycord = targs->y; ycord < targs->y + targs->height; ++ycord) {
      const struct pixel_s cell = targs->pixels[(ycord * width) + xcord];
      set_pixel_rgb(targs->result, xcord, ycord, cell.r, cell.g, cell.b);
    }
  }
  free(targs);
}

static void task_computer_block(void *args) {
  const struct computer_args *targs = (struct computer_args *)args;
  for (size_t xcord = targs->x; xcord < targs->x + targs->width; ++xcord) {
    for (size_t ycord = targs->y; ycord < targs->y + targs->height; ++ycord) {
      convolute_pixel_sequentialy(targs->input_image, xcord, ycord,
                                  targs->kernel, targs->pixels);
    }
  }
  task_queue_add_task(targs->pool, WRITER, task_writer_block, args);
}

static void task_reader_block(void *args) {
  struct reader_args *targs = (struct reader_args *)args;
  BMP *input_image;
  BMP *result_image;
  struct pixel_s *pixels = {NULL};
  if (!prepare_image(targs, &input_image, &result_image, &pixels)) {
    return;
  };

  const size_t height = get_height(input_image);
  const size_t width = get_width(input_image);
  const size_t num_blocks_x = (width + TILE_SIZE - 1) / TILE_SIZE;
  const size_t num_blocks_y = (height + TILE_SIZE - 1) / TILE_SIZE;
  const size_t total_blocks = num_blocks_x * num_blocks_y;
  for (size_t b = 0; b < total_blocks; ++b) {
    struct computer_args *cargs =
        init_cargs(targs, input_image, result_image, pixels);
    if (!cargs) {
      return;
    }
    const size_t by_idx = b / num_blocks_x;
    const size_t bx_idx = b % num_blocks_x;
    cargs->x = bx_idx * TILE_SIZE;
    cargs->y = by_idx * TILE_SIZE;
    cargs->width =
        (cargs->x + TILE_SIZE > width) ? (width - cargs->x) : TILE_SIZE;
    cargs->height =
        (cargs->y + TILE_SIZE > height) ? (height - cargs->y) : TILE_SIZE;
    task_queue_add_task(targs->pool, COMPUTER, task_computer_block, cargs);
  }
}

BMP **start_pipeline(struct main_args *args, KernelMatrix *kernel,
                     struct tpool_s *pool, task_func_t work_func) {
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
    task_queue_add_task(pool, READER, work_func, &task_args[i]);
  }
  tpool_wait(pool);
  free(task_args);
  return results;
}

BMP **apply_filter_pipeline_columns(struct main_args *args,
                                    KernelMatrix *kernel,
                                    struct tpool_s *pool) {
  return start_pipeline(args, kernel, pool, task_reader_columns);
}

BMP **apply_filter_pipeline_rows(struct main_args *args, KernelMatrix *kernel,
                                 struct tpool_s *pool) {
  return start_pipeline(args, kernel, pool, task_reader_rows);
}

BMP **apply_filter_pipeline_pixel_by_pixel(struct main_args *args,
                                           KernelMatrix *kernel,
                                           struct tpool_s *pool) {
  return start_pipeline(args, kernel, pool, task_reader_pixel_by_pixel);
}

BMP **apply_filter_pipeline_block(struct main_args *args, KernelMatrix *kernel,
                                  struct tpool_s *pool) {
  return start_pipeline(args, kernel, pool, task_reader_block);
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
    results = apply_filter_pipeline_rows(args, kernel, pool);
    break;
  case PIXEL_BY_PIXEL:
    results = apply_filter_pipeline_pixel_by_pixel(args, kernel, pool);
    break;
  case BLOCK:
    results = apply_filter_pipeline_block(args, kernel, pool);
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
