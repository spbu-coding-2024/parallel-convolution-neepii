#include "conv.h"
#include "cbmp.h"
#include <limits.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define KERNEL_OFFSET_SIZE 9
#define CHANNELS_COUNT 3

/* clang-format off */
#define GAUSSIAN_5x5_BLUR_COEF 256
#define GAUSSIAN_5x5_BLUR_SIZE 5
#define GAUSSIAN_5x5_BLUR_LAYER \
  {1,  4,  6,  4, 1, \
   4, 16, 24, 16, 4, \
   6, 24, 36, 24, 6, \
   4, 16, 24, 16, 4, \
   1,  4,  6,  4, 1}

#define RIDGE_COEF 1
#define RIDGE_SIZE 3
#define RIDGE_LAYER \
  {-1, -1, -1, \
   -1,  8, -1, \
   -1, -1, -1}

#define GAUSSIAN_3x3_BLUR_COEF 16
#define GAUSSIAN_3x3_BLUR_SIZE 3
#define GAUSSIAN_3x3_BLUR_LAYER \
  {1, 2, 1, \
   2, 4, 2, \
   1, 2, 1}

#define IDENTITY_COEF 1
#define IDENTITY_SIZE 3
#define IDENTITY_LAYER \
  {0, 0, 0, \
   0, 1, 0, \
   0, 0, 0}

// clang-format on

static bool in_bounds_of_image(int32_t x_cord, int32_t y_cord, int32_t height,
                               int32_t width) {
  return (x_cord >= 0 && x_cord < width && y_cord >= 0 && y_cord < height);
}

static int64_t multiply_by_rational_and_ceil(long num, long numer, long denom) {
  long product = num * numer;
  long remainder = product % denom;
  long quotient = product / denom;

  return quotient + (remainder > 0 ? 1 : 0);
}

static uint8_t to_byte(int64_t value) {
  if (value < 0) {
    return 0;
  }
  if (value > UCHAR_MAX) {
    return UCHAR_MAX;
  }
  return value;
}

static void convolute_pixel_sequentialy(BMP *image, int32_t x_cord,
                                        int32_t y_cord, KernelMatrix kernel) {
  int64_t red = 0;
  int64_t green = 0;
  int64_t blue = 0;
  const size_t height = get_height(image);
  const size_t width = get_width(image);
  const size_t ker_size = kernel.size;

  for (size_t offset_x_cord = 0; offset_x_cord < ker_size; ++offset_x_cord) {
    for (size_t offset_y_cord = 0; offset_y_cord < ker_size; ++offset_y_cord) {
      const int32_t temp_x_cord = x_cord + offset_x_cord - (ker_size / 2);
      const int32_t temp_y_cord = y_cord + offset_y_cord - (ker_size / 2);
      if (!in_bounds_of_image(temp_x_cord, temp_y_cord, height, width)) {
        continue;
      }
      uint8_t temp_red;
      uint8_t temp_green;
      uint8_t temp_blue;
      get_pixel_rgb(image, temp_x_cord, temp_y_cord, &temp_red, &temp_green,
                    &temp_blue);

      red +=
          kernel.mtx[0][(offset_y_cord * ker_size) + offset_x_cord] * temp_red;
      green += kernel.mtx[1][(offset_y_cord * ker_size) + offset_x_cord] *
               temp_green;
      blue +=
          kernel.mtx[2][(offset_y_cord * ker_size) + offset_x_cord] * temp_blue;
    }
  }

  red = multiply_by_rational_and_ceil(red, 1, kernel.denominator_coef);
  green = multiply_by_rational_and_ceil(green, 1, kernel.denominator_coef);
  blue = multiply_by_rational_and_ceil(blue, 1, kernel.denominator_coef);

  set_pixel_rgb(image, x_cord, y_cord, to_byte(red), to_byte(green),
                to_byte(blue));
}

void conv_apply_kernel_sequentialy(BMP *image, KernelMatrix kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  for (size_t j = 0; j < height; ++j) {
    for (size_t i = 0; i < width; ++i) {
      convolute_pixel_sequentialy(image, i, j, kernel);
    }
  }
}

void conv_apply_kernel_parallelly(BMP *image, KernelMatrix kernel) {
  const size_t height = get_height(image);
  const size_t width = get_width(image);

  for (size_t j = 0; j < height; ++j) {
#pragma omp parallel for
    for (size_t i = 0; i < width; ++i) {
      convolute_pixel_sequentialy(image, i, j, kernel);
    }
  }
}

KernelMatrix ker_identity() {
  static const int32_t identity_mtx[3 * 3] = IDENTITY_LAYER;
  KernelMatrix ker = {
      .mtx = {NULL, NULL, NULL},
      .denominator_coef = IDENTITY_COEF,
      .size = IDENTITY_SIZE,
  };

  for (int i = 0; i < 3; ++i) {
    ker.mtx[i] = malloc(sizeof(int32_t) * 3 * 3);
    memcpy(ker.mtx[i], identity_mtx, sizeof(identity_mtx));
  }

  return ker;
}

KernelMatrix ker_3x3_gauss_blur() {
  static const int32_t gauss_mtx[3 * 3] = GAUSSIAN_3x3_BLUR_LAYER;
  KernelMatrix ker = {
      .mtx = {NULL, NULL, NULL},
      .denominator_coef = GAUSSIAN_3x3_BLUR_COEF,
      .size = GAUSSIAN_3x3_BLUR_SIZE,
  };

  for (int i = 0; i < 3; ++i) {
    ker.mtx[i] = malloc(sizeof(int32_t) * 3 * 3);
    memcpy(ker.mtx[i], gauss_mtx, sizeof(gauss_mtx));
  }

  return ker;
}

KernelMatrix ker_ridge() {
  static const int32_t ridge[3 * 3] = RIDGE_LAYER;

  KernelMatrix ker = {
      .mtx = {NULL, NULL, NULL},
      .denominator_coef = RIDGE_COEF,
      .size = RIDGE_SIZE,
  };

  for (int i = 0; i < 3; ++i) {
    ker.mtx[i] = malloc(sizeof(int32_t) * 3 * 3);
    memcpy(ker.mtx[i], ridge, sizeof(ridge));
  }

  return ker;
}

KernelMatrix ker_5x5_gauss_blur() {
  static const int32_t ridge[5 * 5] = GAUSSIAN_5x5_BLUR_LAYER;

  KernelMatrix ker = {
      .mtx = {NULL, NULL, NULL},
      .denominator_coef = GAUSSIAN_5x5_BLUR_COEF,
      .size = GAUSSIAN_5x5_BLUR_SIZE,
  };

  for (int i = 0; i < 3; ++i) {
    ker.mtx[i] = malloc(sizeof(int32_t) * ker.size * ker.size);
    memcpy(ker.mtx[i], ridge, sizeof(ridge));
  }

  return ker;
}

void free_kernel_matrix(KernelMatrix ker) {
  for (size_t i = 0; i < ker.size; ++i) {
    if (ker.mtx[i]) {
      free(ker.mtx[i]);
    }
  }
}
