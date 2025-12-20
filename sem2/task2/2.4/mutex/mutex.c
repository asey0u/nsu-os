#include "mutex.h"

#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h>

static int futex_wait(void *addr, int expected) {
  return syscall(SYS_futex, addr, FUTEX_WAIT, expected, NULL, NULL, 0);
}

static int futex_wake(void *addr, int thread_count) {
  return syscall(SYS_futex, addr, FUTEX_WAKE, thread_count, NULL, NULL, 0);
}

int my_mutex_init(my_mutex_t *mutex) {
  if (mutex == NULL) return 0;

  atomic_init(&mutex->state, MUTEX_FREE);
  mutex->owner = 0;
  return 1;
}

int my_mutex_lock(my_mutex_t *mutex) {
  if (mutex == NULL) return 0;

  pid_t self = syscall(SYS_gettid);

  if (atomic_compare_exchange_strong(&mutex->state, &(int){MUTEX_FREE}, MUTEX_LOCKED)) {
    mutex->owner = self;
    return 1;
  }

  while (1) {
    atomic_compare_exchange_strong(&mutex->state, &(int){MUTEX_LOCKED}, MUTEX_WAIT);

    futex_wait(&mutex->state, MUTEX_WAIT);

    if (atomic_compare_exchange_strong(&mutex->state, &(int){MUTEX_FREE}, MUTEX_LOCKED)) {
      mutex->owner = self;
      return 1;
    }
  }
}

int my_mutex_unlock(my_mutex_t *mutex) {
  if (mutex == NULL) return 0;

  pid_t self = syscall(SYS_gettid);

  if (mutex->owner != self) {
    return 0;
  }

  mutex->owner = 0;

  int prev = atomic_exchange(&mutex->state, MUTEX_FREE);
  if (prev == MUTEX_WAIT) {
    futex_wake(&mutex->state, 1);
  }

  return 1;
}

int my_mutex_destroy(my_mutex_t *mutex) {
  if (mutex == NULL) return 0;

  mutex_state current_state = mutex->state;
  pid_t current_owner = mutex->owner;

  if (current_state != MUTEX_FREE || current_owner != 0) {
    return 0;
  }

  atomic_store(&mutex->state, MUTEX_DESTROYED);
  mutex->owner = 0;
  
  return 1;
}