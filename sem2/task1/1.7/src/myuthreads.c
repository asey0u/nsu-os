#define _GNU_SOURCE
#include "myuthreads.h"

#include <ucontext.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdio.h>

#define WORKER_COUNT 4

typedef enum {
  UTHREAD_READY,
  UTHREAD_RUNNING,
  UTHREAD_FINISHED
} uthread_state_t;

struct mythread {
  uint64_t id;

  ucontext_t ctx;
  void *stack;

  atomic_int state;
  void *retval;

  atomic_int detached;
  atomic_int joined;

  void *(*start_routine)(void *);
  void *arg;

  pthread_mutex_t lock;
  pthread_cond_t cv;

  struct mythread *next;
};

static atomic_uint_fast64_t next_id = ATOMIC_VAR_INIT(1);

static mythread_t rq_head = NULL;
static mythread_t rq_tail = NULL;

static pthread_mutex_t rq_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t rq_cv = PTHREAD_COND_INITIALIZER;

static __thread mythread_t current = NULL;
static __thread ucontext_t scheduler_ctx;

static pthread_once_t runtime_once = PTHREAD_ONCE_INIT;

static void enqueue(mythread_t t) {
  pthread_mutex_lock(&rq_lock);

  t->next = NULL;
  if (!rq_tail) {
    rq_head = rq_tail = t;
  } else {
    rq_tail->next = t;
    rq_tail = t;
  }

  pthread_cond_signal(&rq_cv);
  pthread_mutex_unlock(&rq_lock);
}

static mythread_t dequeue(void) {
  pthread_mutex_lock(&rq_lock);

  while (!rq_head)
    pthread_cond_wait(&rq_cv, &rq_lock);

  mythread_t t = rq_head;
  rq_head = t->next;
  if (!rq_head)
    rq_tail = NULL;

  pthread_mutex_unlock(&rq_lock);
  return t;
}

static void uthread_entry(void) {
  mythread_t t = current;

  void *ret = t->start_routine(t->arg);
  mythread_exit(ret);
}

static void *worker_loop(void *arg) {
  (void)arg;

  getcontext(&scheduler_ctx);

  while (1) {
    mythread_t t = dequeue();

    current = t;
    atomic_store(&t->state, UTHREAD_RUNNING);

    swapcontext(&scheduler_ctx, &t->ctx);

    pthread_mutex_lock(&t->lock);
    if (atomic_load(&t->state) == UTHREAD_FINISHED) {
      if (atomic_load(&t->joined)) {
        pthread_cond_signal(&t->cv);
      }
      if (atomic_load(&t->detached)) {
        pthread_mutex_unlock(&t->lock);
        free(t->stack);
        free(t);
        continue;
      }
    } else {
      atomic_store(&t->state, UTHREAD_READY);
      enqueue(t);
    }
    pthread_mutex_unlock(&t->lock);
  }
  return NULL;
}

static void runtime_init(void) {
  pthread_t workers[WORKER_COUNT];

  for (int i = 0; i < WORKER_COUNT; i++) {
    pthread_create(&workers[i], NULL, worker_loop, NULL);
    pthread_detach(workers[i]);
  }
}

int mythread_create(mythread_t *thread,
                    void *(*start_routine)(void *),
                    void *arg) {
  if (!thread || !start_routine)
    return MY_USER_THREAD_FAILURE;

  pthread_once(&runtime_once, runtime_init);

  mythread_t t = calloc(1, sizeof(*t));
  if (!t)
    return MY_USER_THREAD_FAILURE;

  t->id = atomic_fetch_add(&next_id, 1);
  t->stack = malloc(MYUTHREAD_STACK_SIZE);
  if (!t->stack) {
    free(t);
    return MY_USER_THREAD_FAILURE;
  }

  atomic_init(&t->state, UTHREAD_READY);
  atomic_init(&t->detached, 0);
  atomic_init(&t->joined, 0);

  pthread_mutex_init(&t->lock, NULL);
  pthread_cond_init(&t->cv, NULL);

  t->start_routine = start_routine;
  t->arg = arg;

  getcontext(&t->ctx);
  t->ctx.uc_stack.ss_sp = t->stack;
  t->ctx.uc_stack.ss_size = MYUTHREAD_STACK_SIZE;
  t->ctx.uc_link = &scheduler_ctx;
  makecontext(&t->ctx, uthread_entry, 0);

  enqueue(t);
  *thread = t;
  return MY_USER_THREAD_SUCCESS;
}

void mythread_exit(void *retval) {
  mythread_t t = current;

  t->retval = retval;
  atomic_store(&t->state, UTHREAD_FINISHED);

  swapcontext(&t->ctx, &scheduler_ctx);
}

void mythread_yield(void) {
  mythread_t t = current;
  swapcontext(&t->ctx, &scheduler_ctx);
}

int mythread_join(mythread_t thread, void **retval) {
  if (!thread || atomic_load(&thread->detached))
    return MY_USER_THREAD_FAILURE;

  int expected_joined = 0;
  if (!atomic_compare_exchange_strong(&thread->joined, &expected_joined, 1))
    return MY_USER_THREAD_FAILURE;

  pthread_mutex_lock(&thread->lock);
  while (atomic_load(&thread->state) != UTHREAD_FINISHED)
    pthread_cond_wait(&thread->cv, &thread->lock);

  if (retval)
    *retval = thread->retval;

  pthread_mutex_unlock(&thread->lock);

  free(thread->stack);
  free(thread);
  return MY_USER_THREAD_SUCCESS;
}

int mythread_detach(mythread_t thread) {
  atomic_store(&thread->detached, 1);
  return MY_USER_THREAD_SUCCESS;
}
