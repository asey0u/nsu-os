CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -Iinclude
LDFLAGS = -lpthread

SRC = src/proxy.c src/cache.c src/thread_funcs.c src/http_parser.c
OBJ = $(SRC:.c=.o)

proxy: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o proxy $(LDFLAGS)

clean:
	rm -f proxy $(OBJ)
