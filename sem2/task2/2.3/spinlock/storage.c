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
  pthread_spin_init(&node->spin, 0);
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
  pthread_spin_init(&st->spin, 0);
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
    pthread_spin_destroy(&tmp->spin);
    free(tmp);
  }

  pthread_spin_destroy(&st->spin);
  free(st);
}

void storage_push_back(Storage *st, const char *value) {
  if (!st) return;

  Node *new = create_node(value);
  if (!new) return;

  if (!st->first) {
    st->first = new;
    return;
  }

  Node *curr = st->first;
  pthread_spin_lock(&curr->spin);

  while (curr->next) {
    Node *next = curr->next;
    pthread_spin_lock(&next->spin);
    pthread_spin_unlock(&curr->spin);
    curr = next;
  }

  curr->next = new;
  pthread_spin_unlock(&curr->spin);
}
