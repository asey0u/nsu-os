#pragma once

#include "config.h"
#include "list.h"
#include <pthread.h>

typedef enum cache_state {
  LOADING,
  FINISHED
} cache_state;

typedef struct cache_entry {
  char key[MAX_CACHE_KEY_LEN]; // теперь еще не привязываем ключ по которому ищем data именно к url, просто некий key
  
  char *data;
  size_t size;
  size_t capacity;
  
  cache_state state;
  int refcount;
  
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} cache_entry_t;

typedef struct cache {
  list_t lru_list;
  size_t entry_count;
  size_t max_entries;
  pthread_mutex_t mutex;
} cache_t;

int cache_init(cache_t *cache, size_t max_entries);
void cache_destroy(cache_t *cache);
cache_entry_t *cache_get(cache_t *cache, const char *key);
cache_entry_t *cache_create(cache_t *cache, const char *key);
cache_entry_t *cache_get_or_create(cache_t *cache, const char *key, int *need_load);
void cache_release(cache_t *cache, cache_entry_t *entry);