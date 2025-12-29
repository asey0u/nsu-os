#include "cache.h"
#include <stdlib.h>
#include <string.h>

static cache_entry_t *head = NULL;
static cache_entry_t *tail = NULL;
static size_t cache_size = 0;
static size_t cache_limit = 0;

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

void cache_init(size_t max_size) {
  cache_limit = max_size;
}

static void lru_move_front(cache_entry_t *entry) {
  if (entry == head) return;

  if (entry->prev) entry->prev->next = entry->next;
  if (entry->next) entry->next->prev = entry->prev;
  if (entry == tail) tail = entry->prev;

  entry->prev = NULL;
  entry->next = head;
  if (head) head->prev = entry;
  head = entry;
  if (!tail) tail = entry;
}

static void lru_delete_last() {
  while (cache_size > cache_limit && tail && tail->refcount == 0) {
    cache_entry_t *entry = tail;
    tail = entry->prev;
    if (tail) tail->next = NULL;
    else head = NULL;
    
    cache_size -= entry->capacity;
    free(entry->data);
    pthread_mutex_destroy(&entry->mutex);
    pthread_cond_destroy(&entry->cond);
    free(entry);
  }
}

cache_entry_t *cache_get(const char *url) {
  pthread_mutex_lock(&cache_mutex);
  cache_entry_t *entry = head;
  while (entry) {
    if (strcmp(entry->url, url) == 0) { // нашли наш кэш по ссылке
      entry->refcount++;
      lru_move_front(entry); // передвинули в начало списка т.к. будем использовать т.е. он last recently used
      pthread_mutex_unlock(&cache_mutex);
      return entry;
    }
    entry = entry->next;
  }
  pthread_mutex_unlock(&cache_mutex);
  return NULL;
}

cache_entry_t *cache_create(const char *url) {
  pthread_mutex_lock(&cache_mutex);

  cache_entry_t *entry = calloc(1, sizeof(*entry));
  strcpy(entry->url, url);
  entry->capacity = 8192;
  entry->data = malloc(entry->capacity);

  pthread_mutex_init(&entry->mutex, NULL);
  pthread_cond_init(&entry->cond, NULL);
  entry->loading = 1;
  entry->refcount = 1;

  entry->next = head;
  if (head) head->prev = entry;
  head = entry;
  if (!tail) tail = entry;

  cache_size += entry->capacity;
  lru_delete_last();

  pthread_mutex_unlock(&cache_mutex);
  return entry;
}

void cache_release(cache_entry_t *entry) {
  pthread_mutex_lock(&cache_mutex);
  entry->refcount--;
  pthread_mutex_unlock(&cache_mutex);
}
