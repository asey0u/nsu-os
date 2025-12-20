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
  pthread_mutex_init(&node->mutex, NULL);
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
  pthread_mutex_init(&st->mutex, NULL);
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
    pthread_mutex_destroy(&tmp->mutex);
    free(tmp);
  }

  pthread_mutex_destroy(&st->mutex);
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
  pthread_mutex_lock(&curr->mutex);

  while (curr->next) {
    Node *next = curr->next;
    pthread_mutex_lock(&next->mutex);
    pthread_mutex_unlock(&curr->mutex);
    curr = next;
  }

  curr->next = new;
  pthread_mutex_unlock(&curr->mutex);
}
