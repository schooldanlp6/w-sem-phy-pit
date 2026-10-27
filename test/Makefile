# Host build of the radar parser + OLED layout (plain gcc, no ESP-IDF needed).
#
#   make            build and run the tests
#   make preview    also draw the OLED screens as ASCII art
#   make clean      remove the test binary

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g
SRC_DIR := ../main
BUILD   := build
BIN     := $(BUILD)/test_ld2450

SRCS := test_ld2450.c $(SRC_DIR)/ld2450.c $(SRC_DIR)/display.c

.PHONY: all test preview clean

all: test

$(BIN): $(SRCS) $(wildcard $(SRC_DIR)/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -D_DEFAULT_SOURCE -I$(SRC_DIR) $(SRCS) -lm -o $@

test: $(BIN)
	./$(BIN)

preview: $(BIN)
	./$(BIN) preview

clean:
	rm -f $(BIN)
