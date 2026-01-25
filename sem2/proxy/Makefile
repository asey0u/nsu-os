CC = gcc
CFLAGS ?= -std=gnu11 -Wall -Wextra -Iinclude
LDFLAGS ?= -lpthread

SRCS = src/proxy.c src/cache.c src/thread_funcs.c src/http_parser.c src/list.c src/cache_manager.c
OBJS = $(SRCS:.c=.o)

proxy: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o proxy $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f proxy $(OBJS)
