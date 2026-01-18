#include "list.h"
#include <stdlib.h>

void list_init(list_t *list) {
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
}

int list_is_empty(const list_t *list) {
  return list->head == NULL;
}

void list_push_front(list_t *list, void *data) {
  list_node_t *node = malloc(sizeof(list_node_t));
  node->data = data;
  node->next = list->head;
  node->prev = NULL;
  
  if (list->head != NULL) {
    list->head->prev = node;
  } else {
    list->tail = node;
  }
  
  list->head = node;
  list->size++;
}

void list_push_back(list_t *list, void *data) {
  list_node_t *node = malloc(sizeof(list_node_t));
  node->data = data;
  node->prev = list->tail;
  node->next = NULL;
  
  if (list->tail != NULL) {
    list->tail->next = node;
  } else {
    list->head = node;
  }
  
  list->tail = node;
  list->size++;
}

void list_remove(list_t *list, list_node_t *node) {
  if (node->prev != NULL) {
    node->prev->next = node->next;
  } else {
    list->head = node->next;
  }
  
  if (node->next != NULL) {
    node->next->prev = node->prev;
  } else {
    list->tail = node->prev;
  }
  
  list->size--;
  free(node);
}

void list_move_to_front(list_t *list, list_node_t *node) {
  if (list->head == node) {
    return;
  }
  
  void *data = node->data;
  list_remove(list, node);
  list_push_front(list, data);
}

list_node_t *list_front(const list_t *list) {
  return list->head;
}

list_node_t *list_back(const list_t *list) {
  return list->tail;
}

list_node_t *list_pop_back(list_t *list) {
  if (list->tail == NULL) {
    return NULL;
  }
  
  list_node_t *node = list->tail;
  
  if (node->prev != NULL) {
    node->prev->next = NULL;
    list->tail = node->prev;
  } else {
    list->head = NULL;
    list->tail = NULL;
  }
  
  list->size--;
  return node;
}

size_t list_get_size(const list_t *list) {
  return list->size;
}
