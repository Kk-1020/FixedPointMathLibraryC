# Makefile — Fixed-Point Math Library
#
# Targets:
#   make          Build the library (libfixedpoint.a)
#   make test     Build + run the test suite
#   make clean    Remove build artifacts
#
# Flags mirror embedded toolchain discipline:
#   -O2 -Wall -Wextra -Wpedantic   — catch common bugs
#   -fno-builtin                   — no implicit FPU calls
#   -std=c99                       — embedded-portable C standard
#
# Note: -lm is only needed for the test harness (math.h sin/cos reference).
#       The library itself (libfixedpoint.a) has zero runtime FP dependency.

CC      = gcc
AR      = ar
CFLAGS  = -std=c99 -O2 -Wall -Wextra -Wpedantic -fno-builtin \
           -I include
LDFLAGS = -lm

SRC_DIR  = src
TEST_DIR = test
OBJ_DIR  = build

SRCS    = $(SRC_DIR)/fixedpoint.c $(SRC_DIR)/trig.c
OBJS    = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
LIB     = libfixedpoint.a
TEST    = $(OBJ_DIR)/test_fixedpoint

.PHONY: all test clean

all: $(LIB)

## Build the static library
$(LIB): $(OBJS)
	$(AR) rcs $@ $^
	@echo "Built: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

## Build and run tests
test: $(TEST)
	@echo ""
	./$(TEST)

$(TEST): $(TEST_DIR)/test_fixedpoint.c $(LIB) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $< $(LIB) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(LIB)
	@echo "Cleaned."
