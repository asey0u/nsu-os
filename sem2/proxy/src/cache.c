#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

int cache_init(cache_t *cache, size_t max_entries) {
  if (!cache) {
    return -1;
  }
  
  list_init(&cache->lru_list);
  cache->entry_count = 0;
  cache->max_entries = max_entries;
  
  if (pthread_mutex_init(&cache->mutex, NULL) != 0) {
    perror("pthread_mutex_init");
    return -1;
  }
  
  return 0;
}

void cache_destroy(cache_t *cache) {
  if (!cache) {
    return;
  }
  
  if (pthread_mutex_lock(&cache->mutex) != 0) {
    perror("pthread_mutex_lock");
    return;
  }
  
  while (!list_is_empty(&cache->lru_list)) {
    list_node_t *node = list_pop_back(&cache->lru_list);
    if (!node) {
      break;
    }
    
    cache_entry_t *entry = (cache_entry_t *)node->data;
    
    if (entry->data) {
      free(entry->data);
    }
    pthread_mutex_destroy(&entry->mutex);
    pthread_cond_destroy(&entry->cond);
    free(entry);
    free(node);
  }
  
  pthread_mutex_unlock(&cache->mutex);
  pthread_mutex_destroy(&cache->mutex);
}

static void lru_delete_last(cache_t *cache) {
  while (cache->entry_count > cache->max_entries && !list_is_empty(&cache->lru_list)) {
    list_node_t *node = list_back(&cache->lru_list);
    if (!node) {
      break;
    }
    
    cache_entry_t *entry = (cache_entry_t *)node->data;
    
    if (entry->refcount == 0) {
      if (entry->data) {
        free(entry->data);
      }
      pthread_mutex_destroy(&entry->mutex);
      pthread_cond_destroy(&entry->cond);
      free(entry);
      
      list_remove(&cache->lru_list, node);
      cache->entry_count--;
    } else {
      break;
    }
  }
}

cache_entry_t *cache_get(cache_t *cache, const char *key) {
  if (!cache || !key) {
    return NULL;
  }
  
  if (pthread_mutex_lock(&cache->mutex) != 0) {
    perror("pthread_mutex_lock");
    return NULL;
  }
  
  list_node_t *node = list_front(&cache->lru_list);
  while (node != NULL) {
    cache_entry_t *entry = (cache_entry_t *)node->data;
    
    if (entry && strcmp(entry->key, key) == 0) {
      entry->refcount++;
      list_move_to_front(&cache->lru_list, node);
      pthread_mutex_unlock(&cache->mutex);
      return entry;
    }
    
    node = node->next;
  }
  
  pthread_mutex_unlock(&cache->mutex);
  return NULL;
}

cache_entry_t *cache_create(cache_t *cache, const char *key) {
  if (!cache || !key) {
    return NULL;
  }
  
  if (pthread_mutex_lock(&cache->mutex) != 0) {
    perror("pthread_mutex_lock");
    return NULL;
  }
  
  cache_entry_t *entry = calloc(1, sizeof(cache_entry_t));
  if (!entry) {
    perror("calloc");
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  strncpy(entry->key, key, MAX_CACHE_KEY_LEN - 1);
  entry->key[MAX_CACHE_KEY_LEN - 1] = '\0';
  
  entry->capacity = CACHE_ENTRY_CAPACITY;
  entry->data = malloc(CACHE_ENTRY_CAPACITY);
  if (!entry->data) {
    perror("malloc");
    free(entry);
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  entry->size = 0;
  entry->state = LOADING;
  entry->refcount = 1;
  
  if (pthread_mutex_init(&entry->mutex, NULL) != 0) {
    perror("pthread_mutex_init");
    free(entry->data);
    free(entry);
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  if (pthread_cond_init(&entry->cond, NULL) != 0) {
    perror("pthread_cond_init");
    pthread_mutex_destroy(&entry->mutex);
    free(entry->data);
    free(entry);
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  list_push_front(&cache->lru_list, entry);
  cache->entry_count++;
  
  lru_delete_last(cache);
  
  pthread_mutex_unlock(&cache->mutex);
  return entry;
}

cache_entry_t *cache_get_or_create(cache_t *cache, const char *key, int *need_load) {
  if (!cache || !key || !need_load) {
    return NULL;
  }
  
  *need_load = 0;
  
  pthread_mutex_lock(&cache->mutex);
  
  list_node_t *node = list_front(&cache->lru_list);
  while (node != NULL) {
    cache_entry_t *entry = (cache_entry_t *)node->data;
    
    if (entry && strcmp(entry->key, key) == 0) {
      entry->refcount++;
      list_move_to_front(&cache->lru_list, node);
      pthread_mutex_unlock(&cache->mutex);
      return entry;
    }
    
    node = node->next;
  }
  
  cache_entry_t *entry = calloc(1, sizeof(cache_entry_t));
  if (!entry) {
    perror("calloc");
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  strncpy(entry->key, key, MAX_CACHE_KEY_LEN - 1);
  entry->key[MAX_CACHE_KEY_LEN - 1] = '\0';
  
  entry->capacity = CACHE_ENTRY_CAPACITY;
  entry->data = malloc(entry->capacity);
  if (!entry->data) {
    perror("malloc");
    free(entry);
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  entry->size = 0;
  entry->state = LOADING;
  entry->refcount = 1;
  
  if (pthread_mutex_init(&entry->mutex, NULL) != 0) {
    perror("pthread_mutex_init");
    free(entry->data);
    free(entry);
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  if (pthread_cond_init(&entry->cond, NULL) != 0) {
    perror("pthread_cond_init");
    pthread_mutex_destroy(&entry->mutex);
    free(entry->data);
    free(entry);
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
  }
  
  list_push_front(&cache->lru_list, entry);
  cache->entry_count++;
  
  lru_delete_last(cache);
  
  *need_load = 1;
  
  pthread_mutex_unlock(&cache->mutex);
  
  return entry;
}


void cache_release(cache_t *cache, cache_entry_t *entry) {
  if (!cache || !entry) {
    return;
  }
  
  if (pthread_mutex_lock(&cache->mutex) != 0) {
    perror("pthread_mutex_lock");
    return;
  }
  
  if (entry->refcount > 0) {
    entry->refcount--;
  }
  
  pthread_mutex_unlock(&cache->mutex);
}
