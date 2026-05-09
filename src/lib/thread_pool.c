#include "thread_pool.h"
#include "common.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SLEEP_TIME_MONITOR_LOOP 1

int64_t thread_process_count(void) { return sysconf(_SC_NPROCESSORS_ONLN); }

struct work_loop_args_s {
  struct tpool_s *tpool;
  pthread_t tid;
};

static void *thread_work_loop(void *args) {
  struct work_loop_args_s *work_loop_args = (struct work_loop_args_s *)args;
  struct tpool_s *pool = work_loop_args->tpool;
  pthread_t tid = work_loop_args->tid;
  free(args);

  while (!pool->stop) {
    struct task_s *task;
    int32_t type = atomic_load(&pool->workers[tid].type);

    switch (type) {
    case READER:
      task = task_queue_pop(pool->queue[READER]);
      break;
    case COMPUTER:
      task = task_queue_pop(pool->queue[COMPUTER]);
      break;
    case WRITER:
      task = task_queue_pop(pool->queue[WRITER]);
      break;
    default:
      return NULL;
    }

    if (!task) {
      pthread_mutex_lock(&pool->mutex);
      pthread_cond_wait(&pool->work_available, &pool->mutex);
      pthread_mutex_unlock(&pool->mutex);
      continue;
    }
    atomic_store(&pool->workers[tid].state, BUSY);
    atomic_fetch_add(&pool->active_work_count, 1);
    task->func(task->args);
    atomic_store(&pool->workers[tid].state, IDLE);
    atomic_fetch_add(&pool->active_work_count, -1);
    free(task);

    pthread_mutex_lock(&pool->mutex);
    pthread_cond_signal(&pool->is_finished);
    pthread_mutex_unlock(&pool->mutex);
  }
  return NULL;
}

void move_thread_to_role(struct tpool_s *pool, uint8_t from_type,
                         uint8_t to_type) {
  for (size_t i = 0; i < pool->thread_count; ++i) {
    int32_t type = atomic_load(&pool->workers[i].type);
    int32_t state = atomic_load(&pool->workers[i].state);
    if (type == from_type && state == IDLE) {
      atomic_store(&pool->workers[i].type, to_type);
      break;
    }
  }
}

void *monitor_loop(void *tpool_ptr) {
  struct tpool_s *pool = tpool_ptr;

  while (!pool->stop) {
    /* sleep(SLEEP_TIME_MONITOR_LOOP); */

    size_t read_q = pool->queue[READER]->count;
    size_t compute_q = pool->queue[COMPUTER]->count;
    size_t write_q = pool->queue[WRITER]->count;

    int32_t readers = 0;
    int32_t computers = 0;
    /* int32_t writers = 0; */
    for (size_t i = 0; i < pool->thread_count; ++i) {
      switch (atomic_load(&pool->workers[i].type)) {
      case READER:
        readers++;
        break;
      case COMPUTER:
        computers++;
        break;
      /* case WRITER: */
      /*   writers++; */
      /*   break; */
      default:
        break;
      }
    }

    // Rebalancing decisions
    if (compute_q > 10 && readers > 1) {
      move_thread_to_role(pool, READER, COMPUTER);
    }

    if (write_q > 10 && computers > 1) {
      move_thread_to_role(pool, COMPUTER, WRITER);
    }

    if (read_q < 5 && readers > 1 && compute_q > 10) {
      move_thread_to_role(pool, READER, COMPUTER);
    }

    pthread_cond_broadcast(&pool->work_available);
  }
  return NULL;
}

struct tpool_s *tpool_init(size_t thread_count) {
  pthread_t thread;
  struct tpool_s *pool = malloc(sizeof(struct tpool_s));
  if (pool == NULL) {
    return NULL;
  }

  pool->thread_count = thread_count;
  pool->work_count = thread_count;
  pool->active_work_count = 0;
  pool->stop = false;

  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->is_finished, NULL);
  pthread_cond_init(&pool->is_in_progress, NULL);
  pthread_cond_init(&pool->work_available, NULL);

  for (int i = 0; i < NUM_OF_TASK_TYPES; ++i) {
    pool->queue[i] = task_queue_init(32);
    if (pool->queue[i] == NULL) {
      for (int j = i - 1; j >= 0; --j) {
        task_queue_free(pool->queue[i]);
      }
      free(pool);
      return NULL;
    }
  }

  pool->workers = malloc(thread_count * sizeof(struct work_s));
  if (pool->workers == NULL) {
    for (int i = 0; i < NUM_OF_TASK_TYPES; ++i) {
      task_queue_free(pool->queue[i]);
    }
    free(pool);
    return NULL;
  }

  for (size_t i = 0; i < thread_count; i++) {
    struct work_loop_args_s *args = malloc(sizeof(struct work_loop_args_s));
    args->tpool = pool;
    args->tid = i;
    uint8_t type = i < 2 ? 0 : (i < thread_count - 2 ? 1 : 2);
    atomic_store(&pool->workers[i].state, IDLE);
    atomic_store(&pool->workers[i].type, type);
    pthread_create(&thread, NULL, &thread_work_loop, (void *)args);
  }
  pthread_create(&pool->monitor_tid, NULL, &monitor_loop, (void *)pool);
  return pool;
}

void tpool_wait(struct tpool_s *tpool) {
  pthread_mutex_lock(&tpool->mutex);

  while (tpool->active_work_count > 0 || tpool->queue[READER]->count ||
         tpool->queue[COMPUTER]->count || tpool->queue[WRITER]->count) {
    pthread_cond_wait(&tpool->is_finished, &tpool->mutex);
  }

  pthread_mutex_unlock(&tpool->mutex);
}
void tpool_destroy(struct tpool_s *tpool) {
  pthread_mutex_lock(&tpool->mutex);
  tpool->stop = true;
  tpool->work_count = 0;
  tpool->active_work_count = 0;
  pthread_cond_broadcast(&tpool->is_in_progress);
  pthread_mutex_unlock(&tpool->mutex);
  tpool_wait(tpool);

  pthread_mutex_destroy(&tpool->mutex);
  pthread_cond_destroy(&tpool->is_in_progress);
  pthread_cond_destroy(&tpool->is_finished);
  free(tpool->workers);
  free(tpool);
}

void task_queue_add_task(struct tpool_s *pool, task_type_t type,
                         task_func_t func, void *args) {
  struct task_s *task = task_init(type, func, args);
  task_queue_push(pool->queue[type], task);
  pthread_cond_signal(&pool->work_available);
}
