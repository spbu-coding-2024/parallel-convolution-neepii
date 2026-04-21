#ifndef MATRIX_MATRIX_H
#define MATRIX_MATRIX_H

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

#define SHARPENER_COEF 1
#define SHARPENER_SIZE 3
#define SHARPENER_LAYER \
  { 0, -1, 0, \
   -1,  5, -1,                                 \
    0, -1, 0}

#define PREWITT_FIRST_COEF 1
#define PREWITT_FIRST_SIZE 3
#define PREWITT_FIRST_LAYER \
  { 1,  1,  1, \
    0,  0,  0,                                 \
   -1, -1, -1}

#define PREWITT_SECOND_COEF 1
#define PREWITT_SECOND_SIZE 3
#define PREWITT_SECOND_LAYER \
  { 1, 0, -1, \
    1, 0, -1,                                 \
    1, 0, -1}

// clang-format on

#endif
