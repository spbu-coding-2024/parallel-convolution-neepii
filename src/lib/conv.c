#include "conv.h"
#include "cbmp.h"
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

#define CHANNELS_COUNT 3

struct ker_info_s {
  char *name;
  KernelMatrix *(*init_func)(void);
};

struct pixel_s {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

static inline int64_t multiply_by_rational_and_ceil(long num, long numer,
                                                    long denom) {
  long product = num * numer;
  long remainder = product % denom;
  long quotient = product / denom;

  return quotient + (remainder > 0 ? 1 : 0);
}

static inline uint8_t to_byte(int64_t value) {
  if (value < 0) {
    return 0;
  }
  if (value > UCHAR_MAX) {
    return UCHAR_MAX;
  }
  return value;
}

static inline int32_t apply_mirror_padding(int32_t cord, size_t max_value) {
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

static void convolute_pixel_sequentialy(BMP *image, int32_t x_cord,
                                        int32_t y_cord,
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

void conv_apply_kernel_sequentialy(BMP *image, const KernelMatrix *kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  for (size_t j = 0; j < height; ++j) {
    for (size_t i = 0; i < width; ++i) {
      convolute_pixel_sequentialy(image, i, j, kernel, new_pixels);
    }
  }

  for (size_t y_cord = 0; y_cord < height; ++y_cord) {
    for (size_t x_cord = 0; x_cord < width; ++x_cord) {
      const struct pixel_s cell = new_pixels[(y_cord * width) + x_cord];
      set_pixel_rgb(image, x_cord, y_cord, cell.r, cell.g, cell.b);
    }
  }

  free(new_pixels);
}

void conv_apply_kernel_parallelly(BMP *image, const KernelMatrix *kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  for (size_t j = 0; j < height; ++j) {
#pragma omp parallel for
    for (size_t i = 0; i < width; ++i) {
      convolute_pixel_sequentialy(image, i, j, kernel, new_pixels);
    }
  }

  for (size_t y_cord = 0; y_cord < height; ++y_cord) {
#pragma omp parallel for
    for (size_t x_cord = 0; x_cord < width; ++x_cord) {
      const struct pixel_s cell = new_pixels[(y_cord * width) + x_cord];
      set_pixel_rgb(image, x_cord, y_cord, cell.r, cell.g, cell.b);
    }
  }

  free(new_pixels);
}

// TODO: make a generic function for these init function

static inline KernelMatrix *ker_init(const size_t ker_size, const int32_t coef,
                                     const int32_t *layer_mtx) {
  const int32_t layer_size = ker_size * ker_size;
  KernelMatrix *ker = malloc(sizeof(KernelMatrix));
  ker->denominator_coef = coef;
  ker->size = ker_size;

  for (int i = 0; i < LAYER_COUNT; ++i) {
    ker->mtx[i] = malloc(sizeof(int32_t) * layer_size);
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

static const struct ker_info_s info_arr[] = {
    {.name = "ident", .init_func = ker_identity},
    {.name = "blur3", .init_func = ker_3x3_gauss_blur},
    {.name = "blur5", .init_func = ker_5x5_gauss_blur},
    {.name = "ridge", .init_func = ker_ridge},
    {.name = "sharp", .init_func = ker_sharpener},
};

KernelMatrix *choose_kernel_matrix(const char *name) {
  if (!name) {
    return ker_identity();
  }
  const size_t arr_size = sizeof(info_arr) / sizeof(struct ker_info_s);
  for (size_t i = 0; i < arr_size; ++i) {
    if (strcmp(name, info_arr[i].name) == 0) {
      return info_arr[i].init_func();
    }
  }
  fputs("No such filter\n", stderr);
  return NULL;
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

  struct pixel_s *temp_pixels = malloc(height * width * sizeof(struct pixel_s));
  struct pixel_s *new_pixels = malloc(height * width * sizeof(struct pixel_s));

  if (args.parallelize) {
    for (size_t j = 0; j < height; ++j) {
#pragma omp parallel for
      for (size_t i = 0; i < width; ++i) {
        convolute_pixel_sequentialy(image, i, j, kernel_first, temp_pixels);
      }
    }

    for (size_t j = 0; j < height; ++j) {
#pragma omp parallel for
      for (size_t i = 0; i < width; ++i) {
        convolute_pixel_sequentialy(image, i, j, kernel_second, new_pixels);
      }
    }
  } else {
    for (size_t j = 0; j < height; ++j) {
      for (size_t i = 0; i < width; ++i) {
        convolute_pixel_sequentialy(image, i, j, kernel_first, temp_pixels);
      }
    }

    for (size_t j = 0; j < height; ++j) {
      for (size_t i = 0; i < width; ++i) {
        convolute_pixel_sequentialy(image, i, j, kernel_second, new_pixels);
      }
    }
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
  KernelMatrix *kernel_mtx = choose_kernel_matrix(args.filter_name);
  if (kernel_mtx == NULL) {
    free_main_args(&args);
    return true;
  }

  if (args.parallelize) {
    conv_apply_kernel_parallelly(image, kernel_mtx);
  } else {
    conv_apply_kernel_sequentialy(image, kernel_mtx);
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

  if (strcmp(args.filter_name, "prewitt") == 0) {
    return apply_prewitt_filter(image, args);
  }
  return apply_basic_filter(image, args);

#ifdef BENCHMARK
  clock_gettime(CLOCK_MONOTONIC, &end);
  double time_taken =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
  printf("%f\n", time_taken);
#endif
}
