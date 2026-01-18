#pragma once

#include "cache.h"
#include <pthread.h>

/*
 1. URL не в кеше, не грузится -> запустить загрузку, заблокироваться, вернуть данные
 2. URL не в кеше, уже грузится -> заблокироваться, дождаться загрузки, вернуть данные
 3. URL в кеше -> вернуть данные сразу
 */
typedef struct cache_manager {
  cache_t *cache;
  pthread_mutex_t mutex;
} cache_manager_t;

int cache_manager_init(cache_manager_t *manager, cache_t *cache);
void cache_manager_destroy(cache_manager_t *manager);
cache_entry_t *cache_manager_get(cache_manager_t *manager, const char *url);
