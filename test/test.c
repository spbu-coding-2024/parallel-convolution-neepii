#include "cbmp/cbmp.h"
#include "conv.h"
#include "conv_pipeline.h"
#include "thread_pool.h"

// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>
// clang-format on

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TEST_IMAGE_1_PATH "test/images/emacs.bmp"
#define TEST_IMAGE_2_PATH "test/images/coolgame.bmp"

static int32_t compare_images(BMP *fst, BMP *snd) {
  const size_t fst_height = get_height(fst);
  const size_t fst_width = get_width(fst);
  const size_t snd_height = get_height(snd);
  const size_t snd_width = get_width(snd);

  if (fst_height != snd_height || fst_width != snd_width) {
    return 1;
  }

  for (size_t x = 0; x < fst_width; ++x) {
    for (size_t y = 0; y < fst_height; ++y) {
      const size_t idx = (y * fst_width) + x;
      const int32_t r_diff = fst->pixels[idx].red - snd->pixels[idx].red;
      const int32_t g_diff = fst->pixels[idx].green - snd->pixels[idx].green;
      const int32_t b_diff = fst->pixels[idx].blue - snd->pixels[idx].blue;
      if (r_diff != 0) {
        fprintf(stderr, "(%zu, %zu)\n", x, y);
        return r_diff;
      }
      if (g_diff != 0) {
        fprintf(stderr, "(%zu, %zu)\n", x, y);
        return g_diff;
      }
      if (b_diff != 0) {
        fprintf(stderr, "(%zu, %zu)\n", x, y);
        return b_diff;
      }
    }
  }

  return 0;
}

static void test_template_openmp(char *image_path, const int8_t filter) {
  BMP *image_par_col = bopen(image_path);
  BMP *image_par_row = bopen(image_path);
  BMP *image_par_pix = bopen(image_path);
  BMP *image_par_blk = bopen(image_path);
  BMP *image_seq = bopen(image_path);
  KernelMatrix *ker = choose_kernel_matrix(filter);

  conv_apply_kernel_parallelly_columns(image_par_col, ker);
  conv_apply_kernel_parallelly_rows(image_par_row, ker);
  conv_apply_kernel_parallelly_pixel_by_pixel(image_par_pix, ker);
  conv_apply_kernel_parallelly_block(image_par_blk, ker);

  conv_apply_kernel_sequentialy(image_seq, ker);

  assert_int_equal(compare_images(image_seq, image_par_col), 0);
  assert_int_equal(compare_images(image_par_col, image_par_row), 0);
  assert_int_equal(compare_images(image_par_row, image_par_pix), 0);
  assert_int_equal(compare_images(image_par_pix, image_par_blk), 0);

  bclose(image_seq);
  bclose(image_par_col);
  bclose(image_par_row);
  bclose(image_par_pix);
  bclose(image_par_blk);
  free_kernel_matrix(ker);
}

static void test_template_pipeline(char *image_path, const int8_t filter) {
  BMP *image_par = bopen(image_path);
  BMP *image_seq = bopen(image_path);
  KernelMatrix *ker = choose_kernel_matrix(filter);
  size_t thread_count = thread_process_count();
  struct tpool_s *pool = tpool_init(thread_count);

  conv_apply_kernel_sequentialy(image_seq, ker);
  apply_filter_pipeline_columns(image_par, ker, pool);

  assert_int_equal(compare_images(image_par, image_seq), 0);

  bclose(image_seq);
  bclose(image_par);
  tpool_destroy(pool);
  free_kernel_matrix(ker);
}

static void test_ridge1_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_1_PATH, RIDGE);
}

static void test_ridge2_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_2_PATH, RIDGE);
}

static void test_3x3_gauss1_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_1_PATH, BLUR3);
}

static void test_3x3_gauss2_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_2_PATH, BLUR3);
}

static void test_5x5_gauss1_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_1_PATH, BLUR5);
}

static void test_5x5_gauss2_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_2_PATH, BLUR5);
}

static void test_sharp1_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_1_PATH, SHARP);
}

static void test_sharp2_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_2_PATH, SHARP);
}

static void test_identity1_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_1_PATH, IDENT);
}

static void test_identity2_openmp(void **state) {
  (void)state;
  test_template_openmp(TEST_IMAGE_2_PATH, IDENT);
}

static void test_ridge1_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_1_PATH, RIDGE);
}

static void test_ridge2_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_2_PATH, RIDGE);
}

static void test_3x3_gauss1_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_1_PATH, BLUR3);
}

static void test_3x3_gauss2_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_2_PATH, BLUR3);
}

static void test_5x5_gauss1_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_1_PATH, BLUR5);
}

static void test_5x5_gauss2_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_2_PATH, BLUR5);
}

static void test_sharp1_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_1_PATH, SHARP);
}

static void test_sharp2_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_2_PATH, SHARP);
}

static void test_identity1_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_1_PATH, IDENT);
}

static void test_identity2_pipeline(void **state) {
  (void)state;
  test_template_pipeline(TEST_IMAGE_2_PATH, IDENT);
}

int main(void) {

  const struct CMUnitTest tests[] = {
      // clang-format off
    cmocka_unit_test(test_3x3_gauss1_openmp),
    cmocka_unit_test(test_3x3_gauss2_openmp),
    cmocka_unit_test(test_5x5_gauss1_openmp),
    cmocka_unit_test(test_5x5_gauss2_openmp),
    cmocka_unit_test(test_identity1_openmp),
    cmocka_unit_test(test_identity2_openmp),
    cmocka_unit_test(test_ridge1_openmp),
    cmocka_unit_test(test_ridge2_openmp),
    cmocka_unit_test(test_sharp1_openmp),
    cmocka_unit_test(test_sharp2_openmp),
    cmocka_unit_test(test_3x3_gauss1_pipeline),
    cmocka_unit_test(test_3x3_gauss2_pipeline),
    cmocka_unit_test(test_5x5_gauss1_pipeline),
    cmocka_unit_test(test_5x5_gauss2_pipeline),
    cmocka_unit_test(test_identity1_pipeline),
    cmocka_unit_test(test_identity2_pipeline),
    cmocka_unit_test(test_ridge1_pipeline),
    cmocka_unit_test(test_ridge2_pipeline),
    cmocka_unit_test(test_sharp1_pipeline),
    cmocka_unit_test(test_sharp2_pipeline),
      // clang-format on
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
