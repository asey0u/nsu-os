#include "storage.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>

atomic_long asc_counter = 0;
atomic_long eq_counter = 0;
atomic_long desc_counter = 0;
atomic_long swap_counter = 0;

atomic_long asc_iter = 0;
atomic_long desc_iter = 0;
atomic_long eq_iter = 0;

typedef enum ComparatorType {
  EQUAL,
  LESS,
  GREATER
} ComparatorType;

typedef struct Data {
  Storage *storage;
  ComparatorType type;
} Data;

static void gen_random_string(char *buf, size_t maxlen) {
  size_t len = rand() % (maxlen - 1) + 1;
  for (size_t i = 0; i < len; i++) {
    buf[i] = 'a' + rand() % 26;
  }
  buf[len] = '\0';
}

void *counter(void *arg) {
  Data *data = (Data*)arg;
  Storage *st = data->storage;
  ComparatorType type = data->type;

  while (1) {
    pthread_rwlock_rdlock(&st->rwlock);
    Node *curr = st->first;
    if (!curr) { pthread_rwlock_unlock(&st->rwlock); return NULL; }
    pthread_rwlock_rdlock(&curr->rwlock);
    pthread_rwlock_unlock(&st->rwlock);

    Node *next = curr->next;
    while (next) {
      pthread_rwlock_rdlock(&next->rwlock);

      int diff = strlen(curr->value) - strlen(next->value);
      switch (type) {
        case LESS:    if (diff < 0) atomic_fetch_add(&asc_counter, 1); atomic_fetch_add(&asc_iter, 1); break;
        case GREATER: if (diff > 0) atomic_fetch_add(&desc_counter, 1); atomic_fetch_add(&desc_iter, 1); break;
        case EQUAL:   if (diff == 0) atomic_fetch_add(&eq_counter, 1); atomic_fetch_add(&eq_iter, 1); break;
      }

      pthread_rwlock_unlock(&curr->rwlock);
      curr = next;
      next = curr->next;
    }
    pthread_rwlock_unlock(&curr->rwlock);
  }

  return NULL;
}

static void swap_nodes(Node **ptr_to_new_b, Node *a, Node *b) {
  atomic_fetch_add(&swap_counter, 1);
  *ptr_to_new_b = b;
  a->next = b->next;
  b->next = a;
}

void *swapper(void *arg) {
  Storage *st = (Storage*)arg;

  while (1) {
    pthread_rwlock_rdlock(&st->rwlock);
    pthread_rwlock_wrlock(&st->first->rwlock);
    Node* prev = st->first;
    pthread_rwlock_unlock(&st->rwlock);
    Node* curr = NULL;
    Node* next = NULL;
      
    while (prev->next != NULL) {
      pthread_rwlock_wrlock(&prev->next->rwlock);
      curr = prev->next;
      if (curr->next != NULL) {
        pthread_rwlock_wrlock(&curr->next->rwlock);
        next = curr->next;
        if (rand() % 3 == 0) { 
          prev->next = next;
          curr->next = next->next;
          next->next = curr;
          atomic_fetch_add(&swap_counter, 1);
        }
        pthread_rwlock_unlock(&next->rwlock);
      }
      pthread_rwlock_unlock(&prev->rwlock);
      prev = curr;
    }

    pthread_rwlock_unlock(&prev->rwlock);
  }

  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2) { fprintf(stderr, "usage: %s <length>\n", argv[0]); return EXIT_FAILURE; }
  int length = atoi(argv[1]);
  if (length <= 1) { fprintf(stderr, "length must be > 1\n"); return EXIT_FAILURE; }

  srand(time(NULL));

  Storage *st = storage_create();
  if (!st) return EXIT_FAILURE;

  for (int i = 0; i < length; i++) {
    char buf[100];
    gen_random_string(buf, sizeof(buf));
    storage_push_back(st, buf);
  }

  pthread_t t_asc, t_desc, t_eq, t_sw1, t_sw2, t_sw3;
  Data data_asc = {st, LESS};
  Data data_desc = {st, GREATER};
  Data data_eq = {st, EQUAL};

  pthread_create(&t_asc, NULL, counter, &data_asc);
  pthread_create(&t_desc, NULL, counter, &data_desc);
  pthread_create(&t_eq, NULL, counter, &data_eq);
  pthread_create(&t_sw1, NULL, swapper, st);
  pthread_create(&t_sw2, NULL, swapper, st);
  pthread_create(&t_sw3, NULL, swapper, st);

  while (1) {
    sleep(1);
    printf("asc_it=%ld asc_cnt=%ld | desc_it=%ld desc_cnt=%ld | eq_it=%ld eq_cnt=%ld | swaps=%ld\n",
      atomic_load(&asc_iter), atomic_load(&asc_counter),
      atomic_load(&desc_iter), atomic_load(&desc_counter),
      atomic_load(&eq_iter), atomic_load(&eq_counter),
      atomic_load(&swap_counter));
  }

  return 0;
}
