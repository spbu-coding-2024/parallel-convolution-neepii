#include <stdlib.h>
#include <thread_pool.h>
#include <unistd.h>

int64_t t_process_count(void) { return sysconf(_SC_NPROCESSORS_ONLN); }

int32_t tpool_add_work(struct tpool_s *tpool, work_func_t func, void *args) {
  struct work_s *work = malloc(sizeof(struct work_s));
  work->args = args;
  work->func = func;

  pthread_mutex_lock(&tpool->mutex);
  struct work_s *last = tpool->last;
  if (last) {
    last->next = work;
  } else {
    tpool->first = work;
  }
  tpool->last = work;
  tpool->work_count++;
  pthread_cond_signal(&tpool->is_in_progress);
  pthread_mutex_unlock(&tpool->mutex);

  return 0;
}

struct work_s *tpool_pop_work(struct tpool_s *tpool) {
  if (tpool->work_count == 0) {
    return NULL;
  }
  struct work_s *head = tpool->first;
  if (head->next) {
    tpool->first = head->next;
  } else {
    tpool->first = NULL;
    tpool->last = NULL;
  }
  tpool->work_count--;
  return head;
}

static void *thread_work_loop(void *tpool_ptr) {
  struct tpool_s *pool = (struct tpool_s *)tpool_ptr;
  for (;;) {
    pthread_mutex_lock(&pool->mutex);
    while (pool->work_count == 0 && !pool->stop) {
      pthread_cond_wait(&pool->is_in_progress, &pool->mutex);
    }
    if (pool->stop) {
      pool->thread_count--;
      pthread_cond_signal(&pool->is_finished);
      pthread_mutex_unlock(&pool->mutex);
      break;
    }
    struct work_s *work = tpool_pop_work(pool);

    pthread_mutex_unlock(&pool->mutex);
    work->func(work->args);
    free(work);
  }
  return NULL;
}

struct tpool_s *tpool_init(size_t thread_count) {
  struct tpool_s *pool = malloc(sizeof(struct tpool_s));
  pthread_t thread;
  for (size_t i = 0; i < thread_count; i++) {
    pthread_create(&thread, NULL, &thread_work_loop, (void *)pool);
    pthread_detach(thread);
  }
  return pool;
}

void t_pool_wait(struct tpool_s *tpool) {
  pthread_mutex_lock(&tpool->mutex);
  while (tpool->thread_count > 0)
    pthread_cond_wait(&tpool->is_finished, &tpool->mutex);
  pthread_mutex_unlock(&tpool->mutex);
}

void t_pool_destroy(struct tpool_s *tp) {
  pthread_mutex_lock(&tp->mutex);
  tp->stop = 1;

  struct work_s *cur = tp->first;
  struct work_s *prev = NULL;

  while (cur) {
    prev = cur;
    cur = cur->next;
    if (prev)
      free(prev);
  }
  tp->work_count = 0;

  pthread_cond_broadcast(&tp->is_in_progress);
  pthread_mutex_unlock(&tp->mutex);
  t_pool_wait(tp);
}
