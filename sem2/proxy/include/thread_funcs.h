#pragma once

#include "cache.h"
#include "config.h"
#include "cache_manager.h"

typedef struct client_thread_arg {
  int client_fd;
  cache_manager_t *manager;
} client_thread_arg_t;

typedef struct loader_thread_arg {
  cache_entry_t *entry;
  cache_t *cache;
} loader_thread_arg_t;

void *client_thread(void *arg);
void *loader_thread(void *arg);
