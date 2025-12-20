#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

Node *create_node(const char *value) {
  Node *node = malloc(sizeof(Node));
  if (node == NULL) {
    perror("malloc error");
    return NULL;
  }

  strcpy(node->value, value);
  pthread_rwlock_init(&node->rwlock, NULL);
  node->next = NULL;

  return node;
}

Storage *storage_create() {
  Storage *st = malloc(sizeof(Storage));

  if (st == NULL) {
    perror("malloc error");
    return NULL;
  }

  st->first = NULL;
  pthread_rwlock_init(&st->rwlock, NULL);
  return st;
}

void storage_free(Storage *st) {
  if (st == NULL) {
    return;
  }

  Node *curr = st->first;
  while (curr) {
    Node *tmp = curr;
    curr = curr->next;
    pthread_rwlock_destroy(&tmp->rwlock);
    free(tmp);
  }

  pthread_rwlock_destroy(&st->rwlock);
  free(st);
}

void storage_push_back(Storage *st, const char *value) {
  if (!st) return;

  Node *new = create_node(value);
  if (!new) return;

  pthread_rwlock_wrlock(&st->rwlock);

  if (!st->first) {
    st->first = new;
    pthread_rwlock_unlock(&st->rwlock);
    return;
  }

  Node *curr = st->first;

  while (curr->next) {
    Node *next = curr->next;
    curr = next;
  }

  curr->next = new;
  pthread_rwlock_unlock(&st->rwlock);
}
