# Ayatika — Build configuration
# Run `make` to build, `make run` to build and execute, `make clean` to remove the binary.

CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -O2 -I./src -I./lib
LIBS     = -lraylib -lm -lcurl -lsqlite3 -lfribidi
SRC      = $(wildcard src/*.c) $(wildcard lib/cJSON.c) $(wildcard lib/sqlite3.c)
TARGET   = ayatika

# Homebrew paths (ARM macOS)
BREW_PREFIX = /opt/homebrew
CFLAGS_BREW = -I$(BREW_PREFIX)/include
LDFLAGS_BREW = -L$(BREW_PREFIX)/lib

.PHONY: all run test test_phase1 clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CFLAGS_BREW) $(SRC) $(LDFLAGS_BREW) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: tests/test_phase2.c src/mock_data.c src/theme.c
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase2.c src/mock_data.c src/theme.c $(LDFLAGS_BREW) -lraylib -lm -o tests/test_phase2
	./tests/test_phase2

# ── Phase 1: RayLib smoke test ──
test_phase1: tests/test_phase1.c src/mock_data.c src/mock_data.h
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase1.c src/mock_data.c \
		$(LDFLAGS_BREW) -lraylib -lm -lfribidi -o tests/test_phase1
	./tests/test_phase1

clean:
	rm -f $(TARGET) tests/test_phase1 tests/test_raylib tests/test_phase2
