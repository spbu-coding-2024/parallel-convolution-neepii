#include "conv.h"
#include "cbmp.h"
#include <stdbool.h>
#include <stddef.h>

#define KERNEL_OFFSET_SIZE 9
#define CHANNELS_COUNT 3

#define IDENTITY_LAYER {{0, 0, 0}, {0, 1, 0}, {0, 0, 0}}
#define GAUSS_BLUR_LAYER {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}}
#define RIDGE_LAYER {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}}

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
  for (int32_t offset_x = 0; offset_x < 3; ++offset_x) {
    for (int32_t offset_y = 0; offset_y < 3; ++offset_y) {
      const int32_t temp_x = x + offset_x - 1;
      const int32_t temp_y = y + offset_y - 1;
      if (!in_bounds_of_image(temp_x, temp_y, height, width)) {
        continue;
      }
      uint8_t temp_red, temp_green, temp_blue;
      get_pixel_rgb(image, temp_x, temp_y, &temp_red, &temp_green, &temp_blue);

      red += kernel.mtx[0][offset_y][offset_x] * temp_red;
      green += kernel.mtx[1][offset_y][offset_x] * temp_green;
      blue += kernel.mtx[2][offset_y][offset_x] * temp_blue;
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
  const KernelMatrix ker = {
      .mtx = {IDENTITY_LAYER, IDENTITY_LAYER, IDENTITY_LAYER},
      .denominator_coef = 1,
  };
  return ker;
}

KernelMatrix ker_gauss_blur() {
  const KernelMatrix ker = {
      .mtx = {GAUSS_BLUR_LAYER, GAUSS_BLUR_LAYER, GAUSS_BLUR_LAYER},
      .denominator_coef = 16,
  };
  return ker;
}

KernelMatrix ker_ridge() {
  const KernelMatrix ker = {
      .mtx = {RIDGE_LAYER, RIDGE_LAYER, RIDGE_LAYER},
      .denominator_coef = 1,
  };
  return ker;
}
