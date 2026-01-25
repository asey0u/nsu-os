#include "cache_manager.h"
#include "thread_funcs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int cache_manager_init(cache_manager_t *manager, cache_t *cache) {
  if (!manager || !cache) {
    return -1;
  }
  
  manager->cache = cache;
  
  if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
    perror("pthread_mutex_init");
    return -1;
  }
  
  return 0;
}

void cache_manager_destroy(cache_manager_t *manager) {
  if (!manager) {
    return;
  }
  
  pthread_mutex_destroy(&manager->mutex);
}

cache_entry_t *cache_manager_get(cache_manager_t *manager, const char *url) {
  if (!manager || !url) {
    return NULL;
  }
  
  int need_load = 0;
  cache_entry_t *entry = cache_get_or_create(manager->cache, url, &need_load);
  
  if (!entry) {
    fprintf(stderr, "[ERROR] Failed to get or create cache entry\n");
    return NULL;
  }
  
  if (need_load) {
    printf("[CACHE MISS] %s - starting load\n", url);
    
    loader_thread_arg_t *loader_arg = malloc(sizeof(loader_thread_arg_t));
    if (!loader_arg) {
      fprintf(stderr, "[ERROR] malloc failed\n");
      cache_release(manager->cache, entry);
      return NULL;
    }
    
    loader_arg->entry = entry;
    loader_arg->cache = manager->cache;
    
    pthread_t loader;
    if (pthread_create(&loader, NULL, loader_thread, loader_arg) != 0) {
      fprintf(stderr, "[ERROR] pthread_create failed\n");
      free(loader_arg);
      cache_release(manager->cache, entry);
      return NULL;
    }
    pthread_detach(loader);
  } else {
    printf("[CACHE HIT] %s\n", url);
  }
  
  pthread_mutex_lock(&entry->mutex);
  while (entry->state != FINISHED) {
    pthread_cond_wait(&entry->cond, &entry->mutex);
  }
  pthread_mutex_unlock(&entry->mutex);
  
  printf("[REQUEST] Data ready: %zu bytes\n", entry->size);
  
  return entry;
}
