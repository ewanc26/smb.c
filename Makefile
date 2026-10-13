# SMBC — host build. C23, warnings-as-errors.
# `make` builds the playable frontend (requires SDL2); `make test`
# builds and runs the headless suite — no SDL needed for tests.

CC      ?= cc
CSTD    := -std=c23
WARN    := -Wall -Wextra -Werror -Wpedantic
CFLAGS  ?= -O2
BUILD   := build
BIN     := $(BUILD)/smbc

SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS   := $(shell pkg-config --libs sdl2)

LIB_SRCS := $(shell find src -name '*.c' ! -path 'src/host/*')
HOST_SRCS := $(shell find src/host -name '*.c')
HOST_LIB_SRCS := src/host/palette.c src/host/capture.c src/host/keymap.c src/host/padset.c src/host/settings.c
TEST_SRCS := $(shell find tests -name '*.c')
TEST_BINS := $(patsubst tests/%.c,$(BUILD)/tests/%,$(TEST_SRCS))

.PHONY: all test clean

all: $(BIN)

$(BIN): $(HOST_SRCS) $(LIB_SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CSTD) $(WARN) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ $(SDL_LIBS)

$(BUILD)/tests/%: tests/%.c $(LIB_SRCS) $(HOST_LIB_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CSTD) $(WARN) $(CFLAGS) -o $@ $^ -lm

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || exit 1; done

clean:
	rm -rf $(BUILD)
