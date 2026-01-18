#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "cache.h"
#include "thread_funcs.h"
#include "cache_manager.h"

static volatile sig_atomic_t shutdown_flag = 0;

static void handle_sigint(int signo) {
  (void)signo;
  shutdown_flag = 1;
}

int main(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  cache_t cache;
  cache_init(&cache, CACHE_MAX_ENTRIES);

  cache_manager_t manager;
  cache_manager_init(&manager, &cache);

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(1234);

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(listen_fd);
    return 1;
  }

  if (listen(listen_fd, MAX_CONNECTIONS_REQUESTS) < 0) {
    perror("listen");
    close(listen_fd);
    return 1;
  }

  printf("HTTP proxy listening on port %d\n", ntohs(addr.sin_port));

  while (!shutdown_flag) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      if (errno == EINTR && shutdown_flag) {
        break;
      }
      perror("accept");
      continue;
    }

    client_thread_arg_t *arg = malloc(sizeof(client_thread_arg_t));
    if (!arg) {
      perror("malloc");
      close(client_fd);
      continue;
    }

    arg->client_fd = client_fd;
    arg->manager = &manager;

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_thread, arg) != 0) {
      perror("pthread_create");
      close(client_fd);
      free(arg);
      continue;
    }

    pthread_detach(tid);
  }

  close(listen_fd);
  cache_manager_destroy(&manager);
  cache_destroy(&cache);
  fprintf(stderr, "\nProxy terminated\n");

  return 0;
}
