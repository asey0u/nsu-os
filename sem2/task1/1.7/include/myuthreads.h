#ifndef MYUTHREADS_H
#define MYUTHREADS_H

#include <stdint.h>

#define MYUTHREAD_MAX_THREADS 1024
#define GUARD_PAGE_OFFSET 4096
#define MYUTHREAD_STACK_SIZE (2 * 1024 * 1024)
#define TIME_INTERVAL_US 10000

typedef enum {
    MY_USER_THREAD_SUCCESS = 0,
    MY_USER_THREAD_FAILURE = -1
} mythread_error_t;

typedef struct mythread *mythread_t;

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg);
void mythread_exit(void *retval);
mythread_t mythread_self(void);
int mythread_equal(mythread_t t1, mythread_t t2);
int mythread_join(mythread_t thread, void **retval);
int mythread_detach(mythread_t thread);
void mythread_yield();

#endif