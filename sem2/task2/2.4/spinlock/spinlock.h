#pragma once

#include <stdatomic.h>
#include <sys/types.h>

typedef enum spinlock_state {
  SPINLOCK_FREE,
  SPINLOCK_LOCKED,
  SPINLOCK_WAIT,
  SPINLOCK_DESTROY
} spinlock_state;

enum {
  SUCCESS = 0,
  FAILURE = 1
};

typedef struct my_spinlock {
  atomic_int state;
} my_spinlock_t;

int my_spinlock_init(my_spinlock_t *spinlock);
int my_spinlock_lock(my_spinlock_t *spinlock);
int my_spinlock_unlock(my_spinlock_t *spinlock);
int my_spinlock_destroy(my_spinlock_t *spinlock);