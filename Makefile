CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS ?= -Iinclude
LDLIBS ?= -lssl -lcrypto

TARGET := bin/hidehttp
OBJECT_DIR := build/obj
SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,$(OBJECT_DIR)/%.o,$(SOURCES))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(OBJECT_DIR)/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf bin build
