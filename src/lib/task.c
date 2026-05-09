#include "task.h"
#include "thread_pool.h"
#include <pthread.h>
#include <stdlib.h>

#define UNDEF_TASK_ID -1

struct task_queue_s *task_queue_init(size_t size) {
  struct task_queue_s *queue;

  queue = malloc(sizeof(struct task_queue_s));
  if (queue == NULL) {
    return NULL;
  }
  queue->size = size;
  queue->head = 0;
  queue->tail = 0;
  queue->count = 0;
  queue->queue = malloc(size * sizeof(struct task_s));
  if (queue->queue == NULL) {
    free(queue);
    return NULL;
  }

  pthread_mutex_init(&queue->mutex, NULL);
  pthread_cond_init(&queue->is_empty, NULL);
  pthread_cond_init(&queue->is_full, NULL);

  return queue;
}

void task_queue_free(struct task_queue_s *queue) {
  if (queue == NULL) {
    return;
  }

  pthread_mutex_destroy(&queue->mutex);
  pthread_cond_destroy(&queue->is_empty);
  pthread_cond_destroy(&queue->is_full);

  free(queue->queue);
  free(queue);
}

struct task_s *task_init(task_type_t type, task_func_t func, void *args) {
  if (args == NULL || func == NULL) {
    return NULL;
  }
  struct task_s *task;

  task = malloc(sizeof(struct task_s));
  if (task == NULL) {
    return NULL;
  }
  task->type = type;
  task->func = func;
  task->args = args;
  task->id = UNDEF_TASK_ID;

  return task;
}

void task_free(struct task_s *task) { free(task); }

int task_queue_push(struct task_queue_s *queue, struct task_s *task) {
  if (queue == NULL || task == NULL) {
    return -1;
  }

  pthread_mutex_lock(&queue->mutex);

  while (queue->count >= queue->size) {
    pthread_cond_wait(&queue->is_full, &queue->mutex);
  }

  queue->queue[queue->tail] = *task;
  queue->tail = (queue->tail + 1) % queue->size;
  queue->count++;

  pthread_cond_signal(&queue->is_empty);

  pthread_mutex_unlock(&queue->mutex);

  return 0;
}

struct task_s *task_queue_peek(struct task_queue_s *queue) {
  if (queue == NULL || queue->count == 0) {
    return NULL;
  }

  return &queue->queue[queue->head];
}

int task_queue_is_empty(struct task_queue_s *queue) {
  if (queue == NULL)
    return -1;

  pthread_mutex_lock(&queue->mutex);
  int empty = (queue->count == 0);
  pthread_mutex_unlock(&queue->mutex);

  return empty;
}

int task_queue_is_full(struct task_queue_s *queue) {
  if (queue == NULL)
    return -1;

  pthread_mutex_lock(&queue->mutex);
  int full = (queue->count >= queue->size);
  pthread_mutex_unlock(&queue->mutex);

  return full;
}

size_t task_queue_count(struct task_queue_s *queue) {
  if (queue == NULL) {
    return 0;
  }
  pthread_mutex_lock(&queue->mutex);
  int count = queue->count;
  pthread_mutex_unlock(&queue->mutex);

  return count;
}

struct task_s *task_queue_pop(struct task_queue_s *queue) {
  if (queue == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&queue->mutex);

  while (queue->count == 0) {
    pthread_cond_wait(&queue->is_empty, &queue->mutex);
  }

  struct task_s *task = malloc(sizeof(struct task_s));
  if (task == NULL) {
    pthread_mutex_unlock(&queue->mutex);
    return NULL;
  }

  *task = queue->queue[queue->head];
  queue->head = (queue->head + 1) % queue->size;
  queue->count--;

  pthread_cond_signal(&queue->is_full);

  pthread_mutex_unlock(&queue->mutex);

  return task;
}

struct task_s *task_queue_try_pop(struct task_queue_s *queue) {
  if (queue == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&queue->mutex);

  if (queue->count == 0) {
    pthread_mutex_unlock(&queue->mutex);
    return NULL;
  }

  struct task_s *task = malloc(sizeof(struct task_s));
  if (task == NULL) {
    pthread_mutex_unlock(&queue->mutex);
    return NULL;
  }

  *task = queue->queue[queue->head];
  queue->head = (queue->head + 1) % queue->size;
  queue->count--;

  pthread_cond_signal(&queue->is_full);

  pthread_mutex_unlock(&queue->mutex);

  return task;
}
