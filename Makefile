CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS ?= -Iinclude
LDLIBS ?= -lssl -lcrypto

TARGET := bin/hidehttp
GENERATED_DIR := build/generated
OBJECT_DIR := build/obj
EMBEDDED := $(GENERATED_DIR)/embedded_assets.c
SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,$(OBJECT_DIR)/%.o,$(SOURCES)) $(OBJECT_DIR)/embedded_assets.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(EMBEDDED): assets/web/index.html assets/web/style.css scripts/embed-assets.sh
	mkdir -p $(dir $@)
	sh scripts/embed-assets.sh $@ assets/web/index.html assets/web/style.css

$(OBJECT_DIR)/%.o: src/%.c $(EMBEDDED)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJECT_DIR)/embedded_assets.o: $(EMBEDDED)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf bin build
