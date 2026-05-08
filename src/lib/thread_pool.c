#include "thread_pool.h"
#include "common.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

#define SLEEP_TIME_MONITOR_LOOP 1

int64_t t_process_count(void) { return sysconf(_SC_NPROCESSORS_ONLN); }

static void *thread_work_loop(void *tpool_ptr) {
  struct tpool_s *pool = (struct tpool_s *)tpool_ptr;

  pthread_t tid = pool->next_tid;

  while (!pool->stop) {
    struct task_s *task;
    int32_t type = atomic_load(&pool->workers[tid].type);

    switch (type) {
    case READER:
      task = task_queue_try_pop(pool->queue[READER]);
      break;
    case COMPUTER:
      task = task_queue_try_pop(pool->queue[COMPUTER]);
      break;
    case WRITER:
      task = task_queue_try_pop(pool->queue[WRITER]);
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
    task->func(task->args);
    atomic_store(&pool->workers[tid].state, IDLE);
    free(task);
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
    sleep(SLEEP_TIME_MONITOR_LOOP);

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
  tpool_destroy(pool);
  return NULL;
}

/**
 * @param thread_count Number of threads in threadpool excluding monitor thread
 */
struct tpool_s *tpool_init(size_t thread_count) {
  pthread_t thread;
  struct tpool_s *pool = malloc(sizeof(struct tpool_s));
  if (pool == NULL) {
    return NULL;
  }

  pool->thread_count = thread_count;
  pool->work_count = thread_count;
  pool->stop = false;

  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->is_finished, NULL);
  pthread_cond_init(&pool->is_in_progress, NULL);
  pthread_cond_init(&pool->work_available, NULL);

  for (int i = 0; i < 3; ++i) {
    pool->queue[i] = task_queue_init(thread_count);
  }

  pool->workers = malloc(thread_count * sizeof(struct work_s));
  if (pool->workers == NULL) {
    free(pool);
    return NULL;
  }

  for (size_t i = 0; i < thread_count; i++) {
    pool->next_tid = i;
    pthread_create(&thread, NULL, &thread_work_loop, (void *)pool);
    atomic_store(&pool->workers[i].type,
                 i < 2 ? 0
                       : (i < thread_count - 2
                              ? 1
                              : 2)); // TODO: replace ugly ternary operators
    pthread_detach(thread);
  }
  pthread_create(&pool->monitor_tid, NULL, &monitor_loop, (void *)pool);
  return pool;
}

static void tpool_wait(struct tpool_s *tpool) {
  pthread_mutex_lock(&tpool->mutex);

  while (tpool->thread_count > 0)
    pthread_cond_wait(&tpool->is_finished, &tpool->mutex);

  pthread_mutex_unlock(&tpool->mutex);
}

void tpool_destroy(struct tpool_s *tpool) {
  pthread_mutex_lock(&tpool->mutex);
  tpool->stop = 1;

  free(tpool->workers);
  tpool->work_count = 0;

  pthread_cond_broadcast(&tpool->is_in_progress);
  pthread_mutex_unlock(&tpool->mutex);
  tpool_wait(tpool);

  pthread_mutex_destroy(&tpool->mutex);
  pthread_cond_destroy(&tpool->is_in_progress);
  pthread_cond_destroy(&tpool->is_finished);

  free(tpool);
}
