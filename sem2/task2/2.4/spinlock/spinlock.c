#include "spinlock.h"

#include <sys/syscall.h>
#include <unistd.h>

int my_spinlock_init(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return 0;

  atomic_init(&spinlock->state, SPINLOCK_FREE);

  return 1;
}

int my_spinlock_lock(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return 0;
  
  while (1) {
    if (atomic_compare_exchange_strong(&spinlock->state, &(int){SPINLOCK_FREE}, SPINLOCK_LOCKED)) {
      return 1;
    }
  }

  return 1;
}

int my_spinlock_unlock(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return 0;

  atomic_store(&spinlock->state, SPINLOCK_FREE);
  
  return 1;
}

int my_spinlock_destroy(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return 0;
  
  atomic_store(&spinlock->state, SPINLOCK_DESTROY);

  return 0;
}
