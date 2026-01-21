#define _GNU_SOURCE
#include "myuthreads.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>

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

void *busy_thread(void *arg) {
  int id = (int)(intptr_t)arg;
  volatile long x = 0;
  for (long i = 0; i < 500000000; i++) {
    x += i;
  }
  printf("uthread %d finished\n", id);
  return NULL;
}

double timespec_diff(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(void) {
  mythread_t t1, t2, t3, t4;
  void *ret;

  printf("TEST 1: create + join\n");
  mythread_create(&t1, simple_thread, (void *)10);
  mythread_join(t1, &ret);
  printf("\tresult = %ld (expected 20)\n", (long)ret);

  printf("\nTEST 2: yield scheduling\n");
  mythread_create(&t1, yield_thread, (void *)1);
  mythread_create(&t2, yield_thread, (void *)2);
  mythread_join(t1, NULL);
  mythread_join(t2, NULL);

  printf("\nTEST 3: detach\n");
  mythread_create(&t1, simple_thread, (void *)5);
  mythread_detach(t1);
  sleep(1);
  printf("\tdetached thread finished\n");

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

  printf("\tcounter = %d (expected 4000)\n", atomic_load(&counter));

  printf("\nTEST 5: sequential execution\n");
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < 4; i++)
    busy_thread((void *)(intptr_t)i);
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("\tSequential elapsed time: %.6f seconds\n", timespec_diff(start, end));

  printf("\nTEST 6: parallel execution\n");
  mythread_t tids[4];

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < 4; i++)
    mythread_create(&tids[i], busy_thread, (void *)(intptr_t)i);

  for (int i = 0; i < 4; i++)
    mythread_join(tids[i], NULL);
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("\tParallel elapsed time: %.6f seconds\n", timespec_diff(start, end));

  printf("\nALL TESTS DONE\n");
  return 0;
}
