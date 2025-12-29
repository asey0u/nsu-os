#pragma once
#define _POSIX_C_SOURCE 200809L  // для addrinfo!!!

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "http_parser.h"

#define BUF_SIZE 4096
#define MAX_URL_LEN 1024
#define CACHE_MAX_SIZE 10 * 1024 * 1024

typedef struct cache_entry cache_entry_t;
