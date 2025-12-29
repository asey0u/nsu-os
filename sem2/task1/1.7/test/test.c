#define _GNU_SOURCE
#include "myuthreads.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>

static atomic_int counter = 0;

void *simple_thread(void *arg) {
  int v = (int)(intptr_t)arg;
  return (void *)(intptr_t)(v * 2);
}

void *yield_thread(void *arg) {
  int id = (int)(intptr_t)arg;

  for (int i = 0; i < 5; i++) {
    printf("uthread %d: iteration %d\n", id, i);
    mythread_yield();
  }

  return NULL;
}

void *counter_thread(void *arg) {
  int loops = (int)(intptr_t)arg;

  for (int i = 0; i < loops; i++) {
    atomic_fetch_add(&counter, 1);
    mythread_yield();
  }

  return NULL;
}

int main(void) {
  mythread_t t1, t2, t3, t4;
  void *ret;

  printf("TEST 1: create + join\n");
  mythread_create(&t1, simple_thread, (void *)10);
  mythread_join(t1, &ret);
  printf("  result = %ld (expected 20)\n", (long)ret);

  printf("\nTEST 2: yield scheduling\n");
  mythread_create(&t1, yield_thread, (void *)1);
  mythread_create(&t2, yield_thread, (void *)2);
  mythread_join(t1, NULL);
  mythread_join(t2, NULL);

  printf("\nTEST 3: detach\n");
  mythread_create(&t1, simple_thread, (void *)5);
  mythread_detach(t1);
  sleep(1);
  printf("  detached thread finished\n");

  printf("\nTEST 4: parallel counter\n");
  atomic_store(&counter, 0);
  mythread_create(&t1, counter_thread, (void *)1000);
  mythread_create(&t2, counter_thread, (void *)1000);
  mythread_create(&t3, counter_thread, (void *)1000);
  mythread_create(&t4, counter_thread, (void *)1000);

  mythread_join(t1, NULL);
  mythread_join(t2, NULL);
  mythread_join(t3, NULL);
  mythread_join(t4, NULL);

  printf("  counter = %d (expected 4000)\n", atomic_load(&counter));

  printf("\nALL TESTS DONE\n");
  return 0;
}
