#ifndef TASK_TASK_H
#define TASK_TASK_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  READER,
  COMPUTE,
  WRITE,
} task_type_t;

typedef void (*task_func_t)(void *);

struct task_s {
  uint32_t id;
  task_type_t type;
  task_func_t func;
  void *args;
};

struct task_queue_s {
  struct task_s *queue;
  size_t size;
  size_t tail;
  size_t head;
  size_t count;

  pthread_mutex_t mutex;
  pthread_cond_t is_full;
  pthread_cond_t is_empty;
};

struct task_queue_s *task_queue_init(size_t size);

void task_queue_free(struct task_queue_s *queue);

struct task_s *task_init(task_type_t type, task_func_t func, void *args);

void task_free(struct task_s *task);

int task_queue_push(struct task_queue_s *queue, struct task_s *task);

struct task_s *task_queue_peek(struct task_queue_s *queue);

int task_queue_is_empty(struct task_queue_s *queue);

int task_queue_is_full(struct task_queue_s *queue);
size_t task_queue_count(struct task_queue_s *queue);

struct task_s *task_queue_pop(struct task_queue_s *queue);

struct task_s *task_queue_try_pop(struct task_queue_s *queue);

#endif
