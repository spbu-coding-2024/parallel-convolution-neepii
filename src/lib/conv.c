#include "conv.h"
#include "cbmp/cbmp.h"
#include "cli.h"
#include "matrix.h"
#include <limits.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BENCHMARK
#include <time.h>
#endif

int64_t multiply_by_rational_and_ceil(long num, long numer, long denom) {
  const long product = num * numer;
  const long remainder = product % denom;
  const long quotient = product / denom;

  return quotient + (remainder > 0 ? 1 : 0);
}

uint8_t to_byte(int64_t value) {
  if (value < 0) {
    return 0;
  }
  if (value > UCHAR_MAX) {
    return UCHAR_MAX;
  }
  return value;
}

int32_t apply_mirror_padding(int32_t cord, size_t max_value) {
  if (max_value == 1) {
    return 0;
  }
  const size_t period = 2 * (max_value - 1);
  size_t mod = cord % period;
  if (mod <= 0) {
    mod += period;
  }
  if (mod <= max_value - 1) {
    return mod;
  }
  return period - mod;
}

void convolute_pixel_sequentialy(BMP *image, int32_t x_cord, int32_t y_cord,
                                 const KernelMatrix *kernel,
                                 struct pixel_s *pixel_arr) {
  int64_t red = 0;
  int64_t green = 0;
  int64_t blue = 0;
  const size_t height = get_height(image);
  const size_t width = get_width(image);
  const size_t ker_size = kernel->size;

  for (size_t offset_x_cord = 0; offset_x_cord < ker_size; ++offset_x_cord) {
    for (size_t offset_y_cord = 0; offset_y_cord < ker_size; ++offset_y_cord) {
      const int32_t temp_x_cord =
          apply_mirror_padding(x_cord + offset_x_cord - (ker_size / 2), width);
      const int32_t temp_y_cord =
          apply_mirror_padding(y_cord + offset_y_cord - (ker_size / 2), height);

      uint8_t temp_red;
      uint8_t temp_green;
      uint8_t temp_blue;
      get_pixel_rgb(image, temp_x_cord, temp_y_cord, &temp_red, &temp_green,
                    &temp_blue);

      red +=
          kernel->mtx[0][(offset_y_cord * ker_size) + offset_x_cord] * temp_red;
      green += kernel->mtx[1][(offset_y_cord * ker_size) + offset_x_cord] *
               temp_green;
      blue += kernel->mtx[2][(offset_y_cord * ker_size) + offset_x_cord] *
              temp_blue;
    }
  }

  red = multiply_by_rational_and_ceil(red, 1, kernel->denominator_coef);
  green = multiply_by_rational_and_ceil(green, 1, kernel->denominator_coef);
  blue = multiply_by_rational_and_ceil(blue, 1, kernel->denominator_coef);

  pixel_arr[(y_cord * width) + x_cord].r = to_byte(red);
  pixel_arr[(y_cord * width) + x_cord].g = to_byte(green);
  pixel_arr[(y_cord * width) + x_cord].b = to_byte(blue);
}

static void apply_kernel_to_matrix_sequentialy(BMP *image,
                                               const KernelMatrix *kernel,
                                               struct pixel_s *new_pixels) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);
  for (size_t j = 0; j < height; ++j) {
    for (size_t i = 0; i < width; ++i) {
      convolute_pixel_sequentialy(image, i, j, kernel, new_pixels);
    }
  }
}

void conv_apply_kernel_sequentialy(BMP *image, const KernelMatrix *kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  apply_kernel_to_matrix_sequentialy(image, kernel, new_pixels);

  for (size_t y_cord = 0; y_cord < height; ++y_cord) {
    for (size_t x_cord = 0; x_cord < width; ++x_cord) {
      const struct pixel_s cell = new_pixels[(y_cord * width) + x_cord];
      set_pixel_rgb(image, x_cord, y_cord, cell.r, cell.g, cell.b);
    }
  }

  free(new_pixels);
}

static void apply_kernel_to_matrix_rows(BMP *image, const KernelMatrix *kernel,
                                        struct pixel_s *new_pixels) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);
  for (size_t j = 0; j < height; ++j) {
#pragma omp parallel for default(none)                                         \
    shared(width, height, image, kernel, new_pixels, j)
    for (size_t i = 0; i < width; ++i) {
      convolute_pixel_sequentialy(image, i, j, kernel, new_pixels);
    }
  }
}

void conv_apply_kernel_parallelly_rows(BMP *image, const KernelMatrix *kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  apply_kernel_to_matrix_rows(image, kernel, new_pixels);

  for (size_t y_cord = 0; y_cord < height; ++y_cord) {
#pragma omp parallel for default(none)                                         \
    shared(width, height, image, kernel, new_pixels, y_cord)
    for (size_t x_cord = 0; x_cord < width; ++x_cord) {
      const struct pixel_s cell = new_pixels[(y_cord * width) + x_cord];
      set_pixel_rgb(image, x_cord, y_cord, cell.r, cell.g, cell.b);
    }
  }

  free(new_pixels);
}

static void apply_kernel_to_matrix_columns(BMP *image,
                                           const KernelMatrix *kernel,
                                           struct pixel_s *new_pixels) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);
#pragma omp parallel for default(none)                                         \
    shared(width, height, image, kernel, new_pixels)
  for (size_t j = 0; j < height; ++j) {
    for (size_t i = 0; i < width; ++i) {
      convolute_pixel_sequentialy(image, i, j, kernel, new_pixels);
    }
  }
}

void conv_apply_kernel_parallelly_columns(BMP *image,
                                          const KernelMatrix *kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  apply_kernel_to_matrix_columns(image, kernel, new_pixels);

#pragma omp parallel for default(none)                                         \
    shared(width, height, image, kernel, new_pixels)
  for (size_t y_cord = 0; y_cord < height; ++y_cord) {
    for (size_t x_cord = 0; x_cord < width; ++x_cord) {
      const struct pixel_s cell = new_pixels[(y_cord * width) + x_cord];
      set_pixel_rgb(image, x_cord, y_cord, cell.r, cell.g, cell.b);
    }
  }

  free(new_pixels);
}

static void apply_kernel_to_matrix_pixel_by_pixel(BMP *image,
                                                  const KernelMatrix *kernel,
                                                  struct pixel_s *new_pixels) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

#pragma omp parallel for default(none)                                         \
    shared(width, height, image, kernel, new_pixels)
  for (size_t idx = 0; idx < height * width; ++idx) {
    const size_t y = idx / width;
    const size_t x = idx % width;
    convolute_pixel_sequentialy(image, x, y, kernel, new_pixels);
  }
}

void conv_apply_kernel_parallelly_pixel_by_pixel(BMP *image,
                                                 const KernelMatrix *kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  apply_kernel_to_matrix_pixel_by_pixel(image, kernel, new_pixels);

#pragma omp parallel for default(none)                                         \
    shared(width, height, image, kernel, new_pixels)
  for (size_t idx = 0; idx < height * width; ++idx) {
    const size_t y_cord = idx / width;
    const size_t x_cord = idx % width;
    const struct pixel_s cell = new_pixels[(y_cord * width) + x_cord];
    set_pixel_rgb(image, x_cord, y_cord, cell.r, cell.g, cell.b);
  }

  free(new_pixels);
}

static void process_image_block(BMP *image, struct pixel_s *new_pixels,
                                const KernelMatrix *kernel, const size_t x0,
                                const size_t y0, const size_t bw,
                                const size_t bh) {
  for (size_t y = y0; y < y0 + bh; ++y) {
    for (size_t x = x0; x < x0 + bw; ++x) {
      convolute_pixel_sequentialy(image, x, y, kernel, new_pixels);
    }
  }
}

static void set_image_block(BMP *image, struct pixel_s *new_pixels,
                            const size_t x0, const size_t y0, const size_t bw,
                            const size_t bh) {
  const size_t width = get_width(image);
  for (size_t y_cord = y0; y_cord < y0 + bh; ++y_cord) {
    for (size_t x_cord = x0; x_cord < x0 + bw; ++x_cord) {
      const struct pixel_s cell = new_pixels[(y_cord * width) + x_cord];
      set_pixel_rgb(image, x_cord, y_cord, cell.r, cell.g, cell.b);
    }
  }
}

static void apply_kernel_to_matrix_block(BMP *image, const KernelMatrix *kernel,
                                         struct pixel_s *new_pixels) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);
  const size_t num_blocks_x = (width + TILE_SIZE - 1) / TILE_SIZE;
  const size_t num_blocks_y = (height + TILE_SIZE - 1) / TILE_SIZE;
  const size_t total_blocks = num_blocks_x * num_blocks_y;

#pragma omp parallel for default(none)                                         \
    shared(image, new_pixels, width, height, kernel, num_blocks_x,             \
               total_blocks, num_blocks_y) schedule(dynamic)
  for (size_t b = 0; b < total_blocks; ++b) {
    const size_t by_idx = b / num_blocks_x;
    const size_t bx_idx = b % num_blocks_x;
    const size_t x0 = bx_idx * TILE_SIZE;
    const size_t y0 = by_idx * TILE_SIZE;
    const size_t bw = (x0 + TILE_SIZE > width) ? (width - x0) : TILE_SIZE;
    const size_t bh = (y0 + TILE_SIZE > height) ? (height - y0) : TILE_SIZE;

    process_image_block(image, new_pixels, kernel, x0, y0, bw, bh);
  }
}

void conv_apply_kernel_parallelly_block(BMP *image,
                                        const KernelMatrix *kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  apply_kernel_to_matrix_block(image, kernel, new_pixels);

  const size_t num_blocks_x = (width + TILE_SIZE - 1) / TILE_SIZE;
  const size_t num_blocks_y = (height + TILE_SIZE - 1) / TILE_SIZE;
  const size_t total_blocks = num_blocks_x * num_blocks_y;

#pragma omp parallel for default(none)                                         \
    shared(image, new_pixels, width, height, kernel, num_blocks_x,             \
               total_blocks, num_blocks_y) schedule(dynamic)
  for (size_t b = 0; b < total_blocks; ++b) {
    const size_t by_idx = b / num_blocks_x;
    const size_t bx_idx = b % num_blocks_x;
    const size_t x0 = bx_idx * TILE_SIZE;
    const size_t y0 = by_idx * TILE_SIZE;
    const size_t bw = (x0 + TILE_SIZE > width) ? (width - x0) : TILE_SIZE;
    const size_t bh = (y0 + TILE_SIZE > height) ? (height - y0) : TILE_SIZE;

    set_image_block(image, new_pixels, x0, y0, bw, bh);
  }

  free(new_pixels);
}

static inline KernelMatrix *ker_init(const size_t ker_size, const int32_t coef,
                                     const int32_t *layer_mtx) {
  const int32_t layer_size = ker_size * ker_size;
  KernelMatrix *ker = malloc(sizeof(KernelMatrix));
  if (!ker) {
    return NULL;
  }
  ker->denominator_coef = coef;
  ker->size = ker_size;

  for (int i = 0; i < LAYER_COUNT; ++i) {
    ker->mtx[i] = malloc(sizeof(int32_t) * layer_size);
    if (!ker->mtx[i]) {
      for (int j = 0; j < i; ++j) {
        free(ker->mtx[j]);
      }
      free(ker);
      return NULL;
    }
    memcpy(ker->mtx[i], layer_mtx, sizeof(int32_t) * layer_size);
  }

  return ker;
}

static KernelMatrix *ker_identity(void) {
  const int32_t layer[IDENTITY_SIZE * IDENTITY_SIZE] = IDENTITY_LAYER;
  return ker_init(IDENTITY_SIZE, IDENTITY_COEF, layer);
}

static KernelMatrix *ker_3x3_gauss_blur(void) {
  const int32_t layer[GAUSSIAN_3x3_BLUR_SIZE * GAUSSIAN_3x3_BLUR_SIZE] =
      GAUSSIAN_3x3_BLUR_LAYER;
  return ker_init(GAUSSIAN_3x3_BLUR_SIZE, GAUSSIAN_3x3_BLUR_COEF, layer);
}

static KernelMatrix *ker_ridge(void) {
  const int32_t layer[RIDGE_SIZE * RIDGE_SIZE] = RIDGE_LAYER;
  return ker_init(RIDGE_SIZE, RIDGE_COEF, layer);
}

static KernelMatrix *ker_5x5_gauss_blur(void) {
  const int32_t layer[GAUSSIAN_5x5_BLUR_SIZE * GAUSSIAN_5x5_BLUR_SIZE] =
      GAUSSIAN_5x5_BLUR_LAYER;
  return ker_init(GAUSSIAN_5x5_BLUR_SIZE, GAUSSIAN_5x5_BLUR_COEF, layer);
}

static KernelMatrix *ker_sharpener(void) {
  const int32_t layer[SHARPENER_SIZE * SHARPENER_SIZE] = SHARPENER_LAYER;
  return ker_init(SHARPENER_SIZE, SHARPENER_COEF, layer);
}

static KernelMatrix *ker_first_prewitt(void) {
  const int32_t layer[PREWITT_FIRST_SIZE * PREWITT_FIRST_SIZE] =
      PREWITT_FIRST_LAYER;
  return ker_init(PREWITT_FIRST_SIZE, PREWITT_FIRST_COEF, layer);
}

static KernelMatrix *ker_second_prewitt(void) {
  const int32_t layer[PREWITT_SECOND_SIZE * PREWITT_SECOND_SIZE] =
      PREWITT_SECOND_LAYER;
  return ker_init(PREWITT_SECOND_SIZE, PREWITT_SECOND_COEF, layer);
}

static const KernelInit filter_arr[] = {
    [IDENT] = ker_identity,       [BLUR3] = ker_3x3_gauss_blur,
    [BLUR5] = ker_5x5_gauss_blur, [RIDGE] = ker_ridge,
    [SHARP] = ker_sharpener,
};

KernelMatrix *choose_kernel_matrix(const int8_t filter) {
  return filter_arr[filter]();
}

void free_kernel_matrix(KernelMatrix *ker) {
  for (size_t i = 0; i < LAYER_COUNT; ++i) {
    free(ker->mtx[i]);
  }
  free(ker);
}

static int32_t apply_prewitt_filter(BMP *image, struct main_args args) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  KernelMatrix *kernel_first = ker_first_prewitt();
  KernelMatrix *kernel_second = ker_second_prewitt();

  if (!kernel_first || !kernel_second) {
    if (kernel_first) {
      free_kernel_matrix(kernel_first);
    }
    if (kernel_second) {
      free_kernel_matrix(kernel_second);
    }
    return false;
  }

  struct pixel_s *temp_pixels = malloc(height * width * sizeof(struct pixel_s));
  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  if (!temp_pixels || !new_pixels) {
    free(temp_pixels);
    free(new_pixels);
    free_kernel_matrix(kernel_first);
    free_kernel_matrix(kernel_second);
    return false;
  }

  switch (args.mode_option) {
  case SEQUENTIALLY:
    apply_kernel_to_matrix_sequentialy(image, kernel_first, temp_pixels);
    apply_kernel_to_matrix_sequentialy(image, kernel_second, new_pixels);
    break;
  case COLUMNS:
    apply_kernel_to_matrix_columns(image, kernel_first, temp_pixels);
    apply_kernel_to_matrix_columns(image, kernel_second, new_pixels);
    break;
  case ROWS:
    apply_kernel_to_matrix_rows(image, kernel_first, temp_pixels);
    apply_kernel_to_matrix_rows(image, kernel_second, new_pixels);
    break;
  case PIXEL_BY_PIXEL:
    apply_kernel_to_matrix_pixel_by_pixel(image, kernel_first, temp_pixels);
    apply_kernel_to_matrix_pixel_by_pixel(image, kernel_second, new_pixels);
    break;
  case BLOCK:
    apply_kernel_to_matrix_block(image, kernel_first, temp_pixels);
    apply_kernel_to_matrix_block(image, kernel_second, new_pixels);
    break;
  }

  for (size_t y_cord = 0; y_cord < height; ++y_cord) {
    for (size_t x_cord = 0; x_cord < width; ++x_cord) {
      const size_t idx = (y_cord * width) + x_cord;
      const struct pixel_s gx = temp_pixels[idx];
      const struct pixel_s gy = new_pixels[idx];

      int64_t r = (int64_t)gx.r * gx.r + (int64_t)gy.r * gy.r;
      int64_t g = (int64_t)gx.g * gx.g + (int64_t)gy.g * gy.g;
      int64_t b = (int64_t)gx.b * gx.b + (int64_t)gy.b * gy.b;

      r = (r > 0) ? (int64_t)((int64_t)r * 1000 / 256 / 256) : 0;
      g = (g > 0) ? (int64_t)((int64_t)g * 1000 / 256 / 256) : 0;
      b = (b > 0) ? (int64_t)((int64_t)b * 1000 / 256 / 256) : 0;

      set_pixel_rgb(image, x_cord, y_cord, to_byte(r), to_byte(g), to_byte(b));
    }
  }

  free(temp_pixels);
  free(new_pixels);
  free_kernel_matrix(kernel_first);
  free_kernel_matrix(kernel_second);

  return true;
}

static int32_t apply_basic_filter(BMP *image, struct main_args args) {
  KernelMatrix *kernel_mtx = choose_kernel_matrix(args.filter_option);
  if (kernel_mtx == NULL) {
    return false;
  }

  switch (args.mode_option) {
  case SEQUENTIALLY:
    conv_apply_kernel_sequentialy(image, kernel_mtx);
    break;
  case COLUMNS:
    conv_apply_kernel_parallelly_columns(image, kernel_mtx);
    break;
  case ROWS:
    conv_apply_kernel_parallelly_rows(image, kernel_mtx);
    break;
  case PIXEL_BY_PIXEL:
    conv_apply_kernel_parallelly_pixel_by_pixel(image, kernel_mtx);
    break;
  case BLOCK:
    conv_apply_kernel_parallelly_block(image, kernel_mtx);
    break;
  }

  free_kernel_matrix(kernel_mtx);
  return true;
}

int32_t apply_filter(BMP *image, struct main_args args) {

#ifdef BENCHMARK
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);
#endif

  int32_t result;
  if (args.filter_option == PREWITT) {
    result = apply_prewitt_filter(image, args);
  } else {
    result = apply_basic_filter(image, args);
  }

#ifdef BENCHMARK
  clock_gettime(CLOCK_MONOTONIC, &end);
  double time_taken =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
  printf("%f\n", time_taken);
#endif

  return result;
}
