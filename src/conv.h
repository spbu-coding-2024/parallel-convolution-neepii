#pragma once
#include "cbmp.h"
#include <stdint.h>

typedef struct {
  int32_t mtx[3][3][3];
  int32_t denominator_coef;
} KernelMatrix;

void conv_apply_kernel_sequentialy(BMP *image, KernelMatrix kernel);

KernelMatrix ker_identity();
KernelMatrix ker_gauss_blur();
KernelMatrix ker_ridge();
