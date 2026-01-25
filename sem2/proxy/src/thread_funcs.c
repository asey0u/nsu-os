#include "thread_funcs.h"
#include "cache_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdint.h>
#include <errno.h>

static ssize_t send_all(int fd, const char *buf, size_t len) {
  size_t total_sent = 0;
  
  while (total_sent < len) {
    ssize_t n = send(fd, buf + total_sent, len - total_sent, 0);
    
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    
    if (n == 0) {
      break;
    }
    
    total_sent += n;
  }
  
  return total_sent;
}

static void proxy_direct_request(int client_fd, char *req, ssize_t req_len) {
  char host[HOST_SIZE] = {0};
  char path[PATH_SIZE] = {0};

  if (sscanf(req, "%*s http://%255[^/]%1023s", host, path) < 1) {
    fprintf(stderr, "[ERROR] Failed to parse request\n");
    return;
  }

  printf("[DIRECT] %s%s\n", host, path);

  struct addrinfo hints = {0}, *res;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, "80", &hints, &res) != 0) {
    fprintf(stderr, "[ERROR] getaddrinfo failed for %s: %s\n", host, strerror(errno));
    return;
  }

  int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (server_fd < 0) {
    fprintf(stderr, "[ERROR] socket failed: %s\n", strerror(errno));
    freeaddrinfo(res);
    return;
  }

  if (connect(server_fd, res->ai_addr, res->ai_addrlen) != 0) {
    fprintf(stderr, "[ERROR] connect failed to %s: %s\n", host, strerror(errno));
    close(server_fd);
    freeaddrinfo(res);
    return;
  }
  freeaddrinfo(res);

  if (send_all(server_fd, req, req_len) < 0) {
    fprintf(stderr, "[ERROR] send_all failed: %s\n", strerror(errno));
    close(server_fd);
    return;
  }

  char buf[BUF_SIZE];
  ssize_t n;
  size_t total = 0;
  
  while ((n = recv(server_fd, buf, sizeof(buf), 0)) > 0) {
    total += n;
    if (send_all(client_fd, buf, n) < 0) {
      fprintf(stderr, "[ERROR] send_all to client failed: %s\n", strerror(errno));
      break;
    }
  }

  if (n < 0) {
    fprintf(stderr, "[ERROR] recv from server failed: %s\n", strerror(errno));
  } else {
    printf("[DIRECT] Completed: %zu bytes\n", total);
  }

  close(server_fd);
}

/*
  эта функция вызывается каждый раз когда http_parser находит фрагмент или url целиком в запросе
  в at лежит текущая часть(ну или целая ссылка) и strncat мы ее припиливаем к уже записанной в
  parser->data частям
*/
static int on_url_handler(http_parser *parser, const char *at, size_t len) {
  char *url = (char *) parser->data;

  if (strlen(url) + len < MAX_URL_LEN) {
    strncat(url, at, len);
  }

  return 0;
}

void *client_thread(void *arg) {
  client_thread_arg_t *ctx = (client_thread_arg_t *)arg;
  int client_fd = ctx->client_fd;
  cache_manager_t *manager = ctx->manager;
  free(ctx);

  char buf[BUF_SIZE];
  char url[MAX_URL_LEN] = {0};

  http_parser parser;
  http_parser_settings settings = {0};
  settings.on_url = on_url_handler;

  http_parser_init(&parser, HTTP_REQUEST);
  parser.data = url;

  ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
  if (n <= 0) {
    if (n < 0) {
      fprintf(stderr, "[ERROR] recv from client failed: %s\n", strerror(errno));
    }
    close(client_fd);
    return NULL;
  }

  http_parser_execute(&parser, &settings, buf, n);

  if (url[0] == '\0') {
    fprintf(stderr, "[ERROR] Empty URL in request\n");
    close(client_fd);
    return NULL;
  }

  enum http_method method = parser.method;

  if (method != HTTP_GET) {
    proxy_direct_request(client_fd, buf, n);
    close(client_fd);
    return NULL;
  }

  cache_entry_t *entry = cache_manager_get(manager, url); // теперь тут вся логика работы с кешем
  
  if (!entry) {
    fprintf(stderr, "[ERROR] Failed to get resource\n");
    close(client_fd);
    return NULL;
  }

  size_t sent = 0;

  pthread_mutex_lock(&entry->mutex);
  
  if (entry->size > 0) {
    pthread_mutex_unlock(&entry->mutex);
    
    if (send_all(client_fd, entry->data, entry->size) < 0) {
      fprintf(stderr, "[ERROR] send_all to client failed: %s\n", strerror(errno));
    } else {
      sent = entry->size;
    }
    
    pthread_mutex_lock(&entry->mutex);
  }
  
  pthread_mutex_unlock(&entry->mutex);

  fprintf(stderr, "[CLIENT] Sent %zu bytes\n", sent);

  cache_release(manager->cache, entry);
  close(client_fd);
  return NULL;
}

void *loader_thread(void *arg) {
  loader_thread_arg_t *ctx = (loader_thread_arg_t *)arg;
  cache_entry_t *entry = ctx->entry;
  cache_t *cache = ctx->cache;
  free(ctx);

  char host[HOST_SIZE] = {0};
  char path[PATH_SIZE] = {0};

  if (sscanf(entry->key, "http://%255[^/]%1023s", host, path) < 1) {
    fprintf(stderr, "[ERROR] Failed to parse URL from key\n");
    pthread_mutex_lock(&entry->mutex);
    entry->state = FINISHED;
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
    cache_release(cache, entry);
    return NULL;
  }

  printf("[LOADER] Loading %s%s\n", host, path[0] ? path : "/");

  struct addrinfo hints = {0}, *res;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, "80", &hints, &res) != 0) {
    fprintf(stderr, "[ERROR] getaddrinfo failed for %s: %s\n", host, strerror(errno));
    pthread_mutex_lock(&entry->mutex);
    entry->state = FINISHED;
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
    cache_release(cache, entry);
    return NULL;
  }

  int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (server_fd < 0) {
    fprintf(stderr, "[ERROR] socket failed: %s\n", strerror(errno));
    freeaddrinfo(res);
    pthread_mutex_lock(&entry->mutex);
    entry->state = FINISHED;
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
    cache_release(cache, entry);
    return NULL;
  }

  if (connect(server_fd, res->ai_addr, res->ai_addrlen) != 0) {
    fprintf(stderr, "[ERROR] connect failed to %s: %s\n", host, strerror(errno));
    freeaddrinfo(res);
    close(server_fd);
    pthread_mutex_lock(&entry->mutex);
    entry->state = FINISHED;
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
    cache_release(cache, entry);
    return NULL;
  }
  freeaddrinfo(res);

  char request[HTTP_REQUEST_SIZE];
  snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path[0] ? path : "/", host);
  
  if (send_all(server_fd, request, strlen(request)) < 0) {
    fprintf(stderr, "[ERROR] send_all request failed: %s\n", strerror(errno));
    close(server_fd);
    pthread_mutex_lock(&entry->mutex);
    entry->state = FINISHED;
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
    cache_release(cache, entry);
    return NULL;
  }

  char buf[BUF_SIZE];
  size_t total = 0;

  while (1) {
    ssize_t n = recv(server_fd, buf, sizeof(buf), 0);
    
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      fprintf(stderr, "[ERROR] recv from server failed: %s\n", strerror(errno));
      break;
    }
    
    if (n == 0) {
      break;
    }

    pthread_mutex_lock(&entry->mutex);

    if (entry->size + n > entry->capacity) {
      size_t new_capacity = entry->capacity * 2;
      char *new_data = realloc(entry->data, new_capacity);
      if (!new_data) {
        fprintf(stderr, "[ERROR] realloc failed: %s\n", strerror(errno));
        pthread_mutex_unlock(&entry->mutex);
        break;
      }
      entry->data = new_data;
      entry->capacity = new_capacity;
    }

    memcpy(entry->data + entry->size, buf, n);
    entry->size += n;
    total += n;

    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
  }

  pthread_mutex_lock(&entry->mutex);
  entry->state = FINISHED;
  pthread_cond_broadcast(&entry->cond);
  pthread_mutex_unlock(&entry->mutex);

  printf("[LOADER] Completed: %zu bytes\n", total);

  close(server_fd);
  cache_release(cache, entry);
  return NULL;
}
