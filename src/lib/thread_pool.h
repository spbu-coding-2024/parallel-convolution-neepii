#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <task.h>

typedef void (*work_func_t)(void *);

struct work_s {
  void *args;
  work_func_t func;
  struct work_s *next;
};

struct tpool_s {
  size_t thread_count;
  size_t work_count;
  bool stop;

  struct work_s *first;
  struct work_s *last;

  pthread_mutex_t mutex;
  pthread_cond_t is_finished;
  pthread_cond_t is_in_progress;
};

#endif
