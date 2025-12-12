CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./src
DEBUG_FLAGS = -g -O0
RELEASE_FLAGS = -O2

# SRC basico
# SRC = app/main.c

# Si agrego más files
# SRC = $(wildcard **/*.c)

## Makefile inteligente
SRC_DIRS = app src
SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))

# Objetos para compilación incremental
OBJS = $(SRCS:.c=.o)

TARGET = bin/server
TARGET_DEBUG = bin/server_debug


.PHONY: all debug run rdebug valgrind clean

all: $(TARGET)

# ---- RELEASE BUILD ---- #
$(TARGET): $(OBJS)
	mkdir -p bin
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $(TARGET) $(OBJS)

# ---- DEBUG BUILD ---- #
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJS)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET_DEBUG) $(OBJS)

# ---- INCREMENTAL COMPILATION ---- #
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- RUN ---- #
run: $(TARGET)
	./$(TARGET)

rdebug: $(TARGET_DEBUG)
	./$(TARGET_DEBUG)

# ---- VALGRIND ---- #
valgrind: $(TARGET_DEBUG)
	valgrind --leak-check=full --track-origins=yes ./$(TARGET_DEBUG)

# ---- CLEAN ---- #
clean:
	rm -rf bin
	find . -name "*.o" -delete