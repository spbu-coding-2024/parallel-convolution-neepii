#ifndef CONV_CONV_H
#define CONV_CONV_H

#include "cbmp.h"
#include <stddef.h>
#include <stdint.h>

#define LAYER_COUNT 3

#if defined(__GCC_DESTRUCTIVE_SIZE)
#define CACHE_LINE_SIZE __GCC_DESTRUCTIVE_SIZE / 8
#else
#define CACHE_LINE_SIZE 8
#endif

typedef struct {
  int32_t *mtx[LAYER_COUNT];
  int32_t denominator_coef;
  size_t size;
} KernelMatrix;

void conv_apply_kernel_sequentialy(BMP *image, KernelMatrix *kernel);
void conv_apply_kernel_parallelly(BMP *image, KernelMatrix *kernel);

KernelMatrix *choose_kernel_matrix(const char *name);

void free_kernel_matrix(KernelMatrix *ker);

#endif
