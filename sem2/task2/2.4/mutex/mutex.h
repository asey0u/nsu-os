#pragma once

#include <stdatomic.h>
#include <sys/types.h>

typedef enum mutex_state {
  MUTEX_FREE,
  MUTEX_LOCKED,
  MUTEX_WAIT,
  MUTEX_DESTROYED = -1
} mutex_state;

typedef struct my_mutex {
  atomic_int state;
  pid_t owner;
} my_mutex_t;

int my_mutex_init(my_mutex_t *mutex);
int my_mutex_lock(my_mutex_t *mutex);
int my_mutex_unlock(my_mutex_t *mutex);
int my_mutex_destroy(my_mutex_t *mutex);
