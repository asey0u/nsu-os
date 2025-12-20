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
  my_spinlock_init(&node->spin);
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
  my_spinlock_init(&st->spin);
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
    my_spinlock_destroy(&tmp->spin);
    free(tmp);
  }

  my_spinlock_destroy(&st->spin);
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
  my_spinlock_lock(&curr->spin);

  while (curr->next) {
    Node *next = curr->next;
    my_spinlock_lock(&next->spin);
    my_spinlock_unlock(&curr->spin);
    curr = next;
  }

  curr->next = new;
  my_spinlock_unlock(&curr->spin);
}
