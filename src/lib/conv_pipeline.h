#ifndef CONVPIPELINE_CONVPIPELINE_H
#define CONVPIPELINE_CONVPIPELINE_H
#include "cli.h"
#include "conv.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "thread_pool.h"

#include <stdint.h>

int32_t apply_filter_pipeline(BMP *image, struct main_args args);
int32_t apply_filter_pipeline_columns(BMP *image, KernelMatrix *kernel,
                                      struct tpool_s *pool);
#endif
