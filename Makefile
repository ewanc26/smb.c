# SMBC — host build. C23, warnings-as-errors.
# `make` builds the headless binary; `make test` builds and runs the suite.

CC      ?= cc
CSTD    := -std=c23
WARN    := -Wall -Wextra -Werror -Wpedantic
CFLAGS  ?= -O2
BUILD   := build
BIN     := $(BUILD)/smbc

LIB_SRCS := $(shell find src -name '*.c' ! -name 'main.c')
TEST_SRCS := $(shell find tests -name '*.c')
TEST_BINS := $(patsubst tests/%.c,$(BUILD)/tests/%,$(TEST_SRCS))

.PHONY: all test clean

all: $(BIN)

$(BIN): src/main.c $(LIB_SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CSTD) $(WARN) $(CFLAGS) -o $@ $^

$(BUILD)/tests/%: tests/%.c $(LIB_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CSTD) $(WARN) $(CFLAGS) -o $@ $^

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || exit 1; done

clean:
	rm -rf $(BUILD)
