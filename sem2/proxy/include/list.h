#pragma once

#include <stddef.h>

typedef struct list_node {
  struct list_node *prev;
  struct list_node *next;
  void *data;
} list_node_t;

typedef struct list {
  list_node_t *head;
  list_node_t *tail;
  size_t size;
} list_t;

void list_init(list_t *list);
int list_is_empty(const list_t *list);
void list_push_front(list_t *list, void *data);
void list_push_back(list_t *list, void *data);
void list_remove(list_t *list, list_node_t *node);
void list_move_to_front(list_t *list, list_node_t *node);
list_node_t *list_front(const list_t *list);
list_node_t *list_back(const list_t *list);
list_node_t *list_pop_back(list_t *list);
size_t list_get_size(const list_t *list);
