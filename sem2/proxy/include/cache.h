#pragma once
#include "config.h"

struct cache_entry {
  char url[1024];

  char *data;
  size_t size;
  size_t capacity;

  int loading;
  int finished;
  int refcount;

  pthread_mutex_t mutex;
  pthread_cond_t cond;

  cache_entry_t *prev;
  cache_entry_t *next;
};

void cache_init(size_t max_size);

cache_entry_t *cache_get(const char *url);

cache_entry_t *cache_create(const char *url);

void cache_release(cache_entry_t *e);
