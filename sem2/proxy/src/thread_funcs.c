#include "thread_funcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdint.h>

static void proxy_direct_request(int client_fd, char *req, ssize_t req_len) {
  char host[256] = {0};
  char path[1024] = {0};

  sscanf(req, "%*s http://%255[^/]%1023s", host, path);

  struct addrinfo hints = {0}, *res;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, "80", &hints, &res) != 0) {
    perror("getaddrinfo");
    return;
  }

  int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  connect(server_fd, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  send(server_fd, req, req_len, 0);

  char buf[BUF_SIZE];
  ssize_t n;
  while ((n = recv(server_fd, buf, sizeof(buf), 0)) > 0) {
    send(client_fd, buf, n, 0);
  }

  close(server_fd);
}


int on_url(http_parser *parser, const char *at, size_t len) {
  char *url = (char *) parser->data;

  if (strlen(url) + len < MAX_URL_LEN) {
    strncat(url, at, len);
  }
  return 0;
}

void *client_thread(void *arg) {
  int client_fd = (int)(intptr_t)arg;

  char buf[BUF_SIZE];
  char url[MAX_URL_LEN] = {0};

  http_parser parser;
  http_parser_settings settings = {0};
  settings.on_url = on_url;

  http_parser_init(&parser, HTTP_REQUEST);
  parser.data = url;

  ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
  if (n <= 0) {
    close(client_fd);
    return NULL;
  }

  http_parser_execute(&parser, &settings, buf, n);

  if (url[0] == '\0') {
    close(client_fd);
    return NULL;
  }

  enum http_method method = parser.method;


  if (method != HTTP_GET) {
    proxy_direct_request(client_fd, buf, n);
    close(client_fd);
    return NULL;
  }

  cache_entry_t *entry = cache_get(url);

  if (!entry) {
    entry = cache_create(url);

    pthread_t loader;
    pthread_create(&loader, NULL, loader_thread, entry);
    pthread_detach(loader);
  }

  size_t sent = 0;

  pthread_mutex_lock(&entry->mutex);
  while (!entry->finished || sent < entry->size) {

    while (sent >= entry->size && !entry->finished) {
      pthread_cond_wait(&entry->cond, &entry->mutex);
    }

    size_t to_send = entry->size - sent;
    if (to_send > 0) {
      send(client_fd, entry->data + sent, to_send, 0);
      sent += to_send;
    }
  }
  pthread_mutex_unlock(&entry->mutex);

  cache_release(entry);
  close(client_fd);
  return NULL;
}

void *loader_thread(void *arg) {
  cache_entry_t *entry = (cache_entry_t *)arg;

  char host[256] = {0};
  char path[1024] = {0};

  sscanf(entry->url, "http://%255[^/]%1023s", host, path);

  struct addrinfo hints = {0}, *res;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, "80", &hints, &res) != 0) {
    perror("getaddrinfo");
    return NULL;
  }

  int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  connect(server_fd, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  char request[2048];
  snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path[0] ? path : "/", host);
  send(server_fd, request, strlen(request), 0);

  char buf[BUF_SIZE];

  while (1) {
    ssize_t n = recv(server_fd, buf, sizeof(buf), 0);
    if (n <= 0) {
      break;
    }

    pthread_mutex_lock(&entry->mutex);

    if (entry->size + n > entry->capacity) {
      entry->capacity *= 2;
      entry->data = realloc(entry->data, entry->capacity);
    }

    memcpy(entry->data + entry->size, buf, n);
    entry->size += n;

    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
  }

  pthread_mutex_lock(&entry->mutex);
  entry->finished = 1;
  entry->loading = 0;
  pthread_cond_broadcast(&entry->cond);
  pthread_mutex_unlock(&entry->mutex);

  close(server_fd);
  return NULL;
}
