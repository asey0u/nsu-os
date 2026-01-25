#pragma once
#define _POSIX_C_SOURCE 200809L  // для addrinfo!!!

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "http_parser.h"

#define HTTP_REQUEST_SIZE 2048
#define HOST_SIZE 256
#define PATH_SIZE 1024
#define BUF_SIZE 4096
#define MAX_URL_LEN 1024

#define MAX_CACHE_KEY_LEN 1024
#define CACHE_ENTRY_CAPACITY 8192
#define CACHE_MAX_ENTRIES 20
#define MAX_CONNECTIONS_REQUESTS 128
