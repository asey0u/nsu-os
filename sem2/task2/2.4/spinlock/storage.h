#ifndef STORAGE_H
#define STORAGE_H
#define _GNU_SOURCE

#include "spinlock.h"

typedef struct Node {
    char value[100];
    struct Node *next;

    my_spinlock_t spin;
} Node;

typedef struct Storage {
    Node *first;
    my_spinlock_t spin;
} Storage;

Storage *storage_create();
void storage_free(Storage *st);
void storage_push_back(Storage *st, const char *value);
Node *create_node(const char *value);

#endif
