#pragma once
#define _GNU_SOURCE

#include <pthread.h>

typedef struct Node {
    char value[100];
    struct Node *next;

    pthread_spinlock_t spin;
} Node;

typedef struct Storage {
    Node *first;
    pthread_spinlock_t spin;
} Storage;

Storage *storage_create();
void storage_free(Storage *st);
void storage_push_back(Storage *st, const char *value);
int storage_swap(Storage *st, Node *prev, Node *a, Node *b);
Node *create_node(const char *value);
