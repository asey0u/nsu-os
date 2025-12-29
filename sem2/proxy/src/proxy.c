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
#include "config.h"

static int listen_fd = -1;

static void handle_sigint(int signo) {
  (void)signo;
  if (listen_fd != -1) {
    close(listen_fd);
  }
  fprintf(stderr, "\nProxy terminated\n");
  exit(0);
}

int main(void) {
  signal(SIGINT, handle_sigint);

  cache_init(CACHE_MAX_SIZE);

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    perror("socket");
    exit(1);
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
    exit(1);
  }

  if (listen(listen_fd, 128) < 0) {
    perror("listen");
    close(listen_fd);
    exit(1);
  }

  printf("HTTP proxy listening on port %d\n", ntohs(addr.sin_port));

  while (1) { // main busy-wait loop
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_thread, (void *)(intptr_t)client_fd) != 0) {
      perror("pthread_create");
      close(client_fd);
      continue;
    }

    pthread_detach(tid);
  }

  return 0;
}
