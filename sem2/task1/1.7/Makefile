CC := gcc
CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Iinclude -fsanitize=address -fsanitize=leak
LDFLAGS := -fsanitize=address -fsanitize=leak -lpthread

SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build

UTHREAD_SRC := $(SRC_DIR)/myuthreads.c

UTHREAD_OBJ := $(BUILD_DIR)/myuthreads.o
TEST_OBJ := $(BUILD_DIR)/test.o

TARGET := test_runner

.PHONY: all test clean

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(UTHREAD_OBJ): $(UTHREAD_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_DIR)/test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(UTHREAD_OBJ) $(TEST_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
