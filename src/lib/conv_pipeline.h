#ifndef CONVPIPELINE_CONVPIPELINE_H
#define CONVPIPELINE_CONVPIPELINE_H
#include "cli.h"
#include "conv.h"
#include "thread_pool.h"
#include <stdint.h>

int32_t apply_filter_pipeline(struct main_args *args);
BMP **start_pipeline_and_wait(struct main_args *args, KernelMatrix *kernel,
                              struct tpool_s *pool);
#endif
