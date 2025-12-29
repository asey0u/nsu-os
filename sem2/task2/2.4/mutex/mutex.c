#include "mutex.h"

#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h>
#include <stdlib.h>

static int futex_wait(void *addr, int expected) {
  return syscall(SYS_futex, addr, FUTEX_WAIT, expected, NULL, NULL, 0);
}

static int futex_wake(void *addr, int thread_count) {
  return syscall(SYS_futex, addr, FUTEX_WAKE, thread_count, NULL, NULL, 0);
}

int my_mutex_init(my_mutex_t *mutex) {
  if (mutex == NULL) return FAILURE;

  atomic_init(&mutex->state, MUTEX_FREE);
  atomic_init(&mutex->owner, 0);
  return SUCCESS;
}

int my_mutex_lock(my_mutex_t *mutex) {
  if (mutex == NULL) return FAILURE;

  pid_t self = syscall(SYS_gettid);

  while (1) {
    if (atomic_compare_exchange_strong(&mutex->state, &(int){MUTEX_FREE}, MUTEX_LOCKED)) {
      atomic_store(&mutex->owner, self);
      return SUCCESS;
    }
    
    atomic_compare_exchange_strong(&mutex->state, &(int){MUTEX_LOCKED}, MUTEX_WAIT);

    futex_wait(&mutex->state, MUTEX_WAIT);

    if (atomic_load(&mutex->state) == MUTEX_DESTROYED) {
      return FAILURE;
    }
  }
}

int my_mutex_unlock(my_mutex_t *mutex) {
  if (mutex == NULL) return FAILURE;

  pid_t self = syscall(SYS_gettid);

  pid_t owner = atomic_load(&mutex->owner);

  if (owner != self) {
    return FAILURE;
  }

  atomic_store(&mutex->owner, 0);

  int prev = atomic_exchange(&mutex->state, MUTEX_FREE);
  if (prev == MUTEX_WAIT) {
    futex_wake(&mutex->state, 1);
  }

  return SUCCESS;
}

int my_mutex_destroy(my_mutex_t *mutex) {
  if (mutex == NULL) return FAILURE;

  mutex_state current_state = mutex->state;
  pid_t current_owner = mutex->owner;

  if (current_state != MUTEX_FREE || current_owner != 0) {
    return FAILURE;
  }

  atomic_store(&mutex->state, MUTEX_DESTROYED);
  mutex->owner = 0;
  
  return SUCCESS;
}