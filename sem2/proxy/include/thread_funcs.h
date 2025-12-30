#pragma once

#include "config.h"
#include "cache.h"

void *client_thread(void *arg);

void *loader_thread(void *arg);

int on_url(http_parser *parser, const char *at, size_t len);
