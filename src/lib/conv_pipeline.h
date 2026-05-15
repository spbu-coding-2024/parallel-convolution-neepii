#ifndef CONVPIPELINE_CONVPIPELINE_H
#define CONVPIPELINE_CONVPIPELINE_H
#include "cli.h"
#include "conv.h"
#include "thread_pool.h"

#include <stdint.h>

int32_t apply_filter_pipeline(struct main_args *args);
BMP **apply_filter_pipeline_columns(struct main_args *args,
                                    KernelMatrix *kernel, struct tpool_s *pool);
BMP **apply_filter_pipeline_rows(struct main_args *args, KernelMatrix *kernel,
                                 struct tpool_s *pool);
BMP **apply_filter_pipeline_pixel_by_pixel(struct main_args *args,
                                           KernelMatrix *kernel,
                                           struct tpool_s *pool);
BMP **apply_filter_pipeline_block(struct main_args *args, KernelMatrix *kernel,
                                  struct tpool_s *pool);
#endif
