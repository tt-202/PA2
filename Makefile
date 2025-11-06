CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
TARGET = chash
SOURCES = chash.c hash_operations.c hash_utuls.c rwlock.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c hash.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) hash.log

.PHONY: all clean

