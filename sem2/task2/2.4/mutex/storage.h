#ifndef STORAGE_H
#define STORAGE_H
#define _GNU_SOURCE

#include "mutex.h"

typedef struct Node {
    char value[100];
    struct Node *next;

    my_mutex_t mutex;
} Node;

typedef struct Storage {
    Node *first;
    my_mutex_t mutex;
} Storage;

Storage *storage_create();
void storage_free(Storage *st);
void storage_push_back(Storage *st, const char *value);
Node *create_node(const char *value);

#endif
