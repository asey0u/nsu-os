#include "spinlock.h"

#include <sys/syscall.h>
#include <unistd.h>

int my_spinlock_init(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return FAILURE;

  atomic_init(&spinlock->state, SPINLOCK_FREE);

  return SUCCESS;
}

int my_spinlock_lock(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return FAILURE;
  
  while (1) {
    if (atomic_compare_exchange_strong(&spinlock->state, &(int){SPINLOCK_FREE}, SPINLOCK_LOCKED)) {
      return SUCCESS;
    }
  }
}

int my_spinlock_unlock(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return FAILURE;

  atomic_store(&spinlock->state, SPINLOCK_FREE);
  
  return SUCCESS;
}

int my_spinlock_destroy(my_spinlock_t *spinlock) {
  if (spinlock == NULL) return FAILURE;
  
  atomic_store(&spinlock->state, SPINLOCK_DESTROY);

  return SUCCESS;
}
