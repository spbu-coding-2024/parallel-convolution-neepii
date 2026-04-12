#include "conv.h"
#include "cbmp.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define KERNEL_OFFSET_SIZE 9
#define CHANNELS_COUNT 3

/* clang-format off */
#define GAUSSIAN_5x5_BLUR_LAYER \
  {1,  4,  6,  4, 1, \
   4, 16, 24, 16, 4, \
   6, 24, 36, 24, 6, \
   4, 16, 24, 16, 4, \
   1,  4,  6,  4, 1}

// clang-format on

static bool in_bounds_of_image(size_t x, int32_t y, int32_t height,
                               int32_t width) {
  return (x >= 0 && x < width && y >= 0 && y < height);
}

static int64_t multiply_by_rational_and_ceil(long num, long numer, long denom) {
  long product = num * numer;
  long remainder = product % denom;
  long quotient = product / denom;

  return quotient + (remainder > 0 ? 1 : 0);
}

static void convolute_pixel_sequentialy(BMP *image, int32_t x, int32_t y,
                                        KernelMatrix kernel) {
  int64_t red = 0;
  int64_t green = 0;
  int64_t blue = 0;
  const int32_t height = get_height(image);
  const int32_t width = get_width(image);
  const int32_t ker_size = kernel.size;
  for (int32_t offset_x = 0; offset_x < ker_size; ++offset_x) {
    for (int32_t offset_y = 0; offset_y < ker_size; ++offset_y) {
      const int32_t temp_x = x + offset_x - (ker_size / 2);
      const int32_t temp_y = y + offset_y - (ker_size / 2);
      if (!in_bounds_of_image(temp_x, temp_y, height, width)) {
        continue;
      }
      uint8_t temp_red, temp_green, temp_blue;
      get_pixel_rgb(image, temp_x, temp_y, &temp_red, &temp_green, &temp_blue);

      red += kernel.mtx[0][offset_y * ker_size + offset_x] * temp_red;
      green += kernel.mtx[1][offset_y * ker_size + offset_x] * temp_green;
      blue += kernel.mtx[2][offset_y * ker_size + offset_x] * temp_blue;
    }
  }

  red = multiply_by_rational_and_ceil(red, 1, kernel.denominator_coef);
  green = multiply_by_rational_and_ceil(green, 1, kernel.denominator_coef);
  blue = multiply_by_rational_and_ceil(blue, 1, kernel.denominator_coef);

  red = (red < 0) ? 0 : (red > 255) ? 255 : red;
  green = (green < 0) ? 0 : (green > 255) ? 255 : green;
  blue = (blue < 0) ? 0 : (blue > 255) ? 255 : blue;
  set_pixel_rgb(image, x, y, red, green, blue);
}

void conv_apply_kernel_sequentialy(BMP *image, KernelMatrix kernel) {
  const int32_t height = get_height(image);
  const int32_t width = get_width(image);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      convolute_pixel_sequentialy(image, x, y, kernel);
    }
  }
}

KernelMatrix ker_identity() {
  static const int32_t identity_mtx[3 * 3] = {0, 0, 0, 0, 1, 0, 0, 0, 0};
  KernelMatrix ker = {
      .mtx = {NULL, NULL, NULL},
      .denominator_coef = 1,
      .size = 3,
  };

  for (int i = 0; i < 3; ++i) {
    ker.mtx[i] = malloc(sizeof(int32_t) * 3 * 3);
    memcpy(ker.mtx[i], identity_mtx, sizeof(identity_mtx));
  }

  return ker;
}

KernelMatrix ker_3x3_gauss_blur() {
  static const int32_t gauss_mtx[3 * 3] = {1, 2, 1, 2, 4, 2, 1, 2, 1};
  KernelMatrix ker = {
      .mtx = {NULL, NULL, NULL},
      .denominator_coef = 16,
      .size = 3,
  };

  for (int i = 0; i < 3; ++i) {
    ker.mtx[i] = malloc(sizeof(int32_t) * 3 * 3);
    memcpy(ker.mtx[i], gauss_mtx, sizeof(gauss_mtx));
  }

  return ker;
}

KernelMatrix ker_ridge() {
  static const int32_t ridge[3 * 3] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};

  KernelMatrix ker = {
      .mtx = {NULL, NULL, NULL},
      .denominator_coef = 1,
      .size = 3,
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
      .denominator_coef = 256,
      .size = 5,
  };

  for (int i = 0; i < 3; ++i) {
    ker.mtx[i] = malloc(sizeof(int32_t) * ker.size * ker.size);
    memcpy(ker.mtx[i], ridge, sizeof(ridge));
  }

  return ker;
}

void free_kernel_matrix(KernelMatrix ker) {
  for (int i = 0; i < 3; ++i) {
    if (ker.mtx[i]) {
      free(ker.mtx[i]);
    }
  }
}
