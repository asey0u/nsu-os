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
  my_mutex_init(&node->mutex);
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
  my_mutex_init(&st->mutex);
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
    my_mutex_destroy(&tmp->mutex);
    free(tmp);
  }

  my_mutex_destroy(&st->mutex);
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
  my_mutex_lock(&curr->mutex);

  while (curr->next) {
    Node *next = curr->next;
    my_mutex_lock(&next->mutex);
    my_mutex_unlock(&curr->mutex);
    curr = next;
  }

  curr->next = new;
  my_mutex_unlock(&curr->mutex);
}
