#ifndef CONV_CONV_H
#define CONV_CONV_H

#include "cbmp/cbmp.h"
#include "cli.h"
#include <stddef.h>
#include <stdint.h>

#define LAYER_COUNT 3

#ifdef __GCC_DESTRUCTIVE_SIZE
#define CACHE_LINE_SIZE __GCC_DESTRUCTIVE_SIZE / 8
#else
#define CACHE_LINE_SIZE 8
#endif

struct pixel_s {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

typedef struct {
  int32_t *mtx[LAYER_COUNT];
  int32_t denominator_coef;
  size_t size;
} KernelMatrix;

typedef KernelMatrix *(*KernelInit)(void);

uint8_t to_byte(int64_t value);

int64_t multiply_by_rational_and_ceil(long num, long numer, long denom);
int32_t apply_mirror_padding(int32_t cord, size_t max_value);

void convolute_pixel_sequentialy(BMP *image, int32_t x_cord, int32_t y_cord,
                                 const KernelMatrix *kernel,
                                 struct pixel_s *pixel_arr);
void conv_apply_kernel_sequentialy(BMP *image, const KernelMatrix *kernel);
void conv_apply_kernel_parallelly_rows(BMP *image, const KernelMatrix *kernel);
void conv_apply_kernel_parallelly_columns(BMP *image,
                                          const KernelMatrix *kernel);
void conv_apply_kernel_parallelly_pixel_by_pixel(BMP *image,
                                                 const KernelMatrix *kernel);
void conv_apply_kernel_parallelly_block(BMP *image, const KernelMatrix *kernel);

KernelMatrix *choose_kernel_matrix(const int8_t filter);

void free_kernel_matrix(KernelMatrix *ker);

int32_t apply_filter(BMP *image, struct main_args args);

#endif
