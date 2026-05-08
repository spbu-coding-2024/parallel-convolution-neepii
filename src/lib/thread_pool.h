#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H
#include "common.h"
#include "task.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*work_func_t)(void *);

struct work_s {
  void *args;
  work_func_t func;
  atomic_int type;
  atomic_int state;
};

struct tpool_s {
  pthread_t next_tid;

  pthread_t monitor_tid;

  size_t thread_count;
  size_t work_count;
  bool stop;

  struct work_s *workers;

  pthread_mutex_t mutex;
  pthread_cond_t is_finished;
  pthread_cond_t is_in_progress;
  pthread_cond_t work_available;

  struct task_queue_s *queue[NUM_OF_TASK_TYPES];
};
int64_t t_process_count(void);
struct tpool_s *tpool_init(size_t thread_count);
void *monitor_loop(void *tpool_ptr);
void tpool_destroy(struct tpool_s *tpool);

#endif
