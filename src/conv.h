#pragma once
#include "cbmp.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  int32_t *mtx[3];
  int32_t denominator_coef;
  size_t size;
} KernelMatrix;

void conv_apply_kernel_sequentialy(BMP *image, KernelMatrix kernel);
void conv_apply_kernel_parallelly(BMP *image, KernelMatrix kernel);

KernelMatrix ker_identity();
KernelMatrix ker_3x3_gauss_blur();
KernelMatrix ker_5x5_gauss_blur();
KernelMatrix ker_ridge();

void free_kernel_matrix(KernelMatrix ker);
