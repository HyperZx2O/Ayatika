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

.PHONY: all run test test_phase1 test_phase5 test_phase6 clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CFLAGS_BREW) $(SRC) $(LDFLAGS_BREW) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: test_phase2 test_phase4 test_phase5 test_phase6

test_phase2: tests/test_phase2.c src/mock_data.c src/theme.c
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase2.c src/mock_data.c src/theme.c $(LDFLAGS_BREW) -lraylib -lm -lfribidi -o tests/test_phase2
	./tests/test_phase2

test_phase3: tests/test_phase3.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase3.c src/mock_data.c src/theme.c src/ui.c src/quran.c $(LDFLAGS_BREW) -lraylib -lm -lfribidi -o tests/test_phase3
	./tests/test_phase3

test_phase4: tests/test_phase4.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase4.c src/mock_data.c src/theme.c src/ui.c src/quran.c $(LDFLAGS_BREW) -lraylib -lm -lfribidi -o tests/test_phase4
	./tests/test_phase4

# ── Phase 1: RayLib smoke test ──
test_phase1: tests/test_phase1.c src/mock_data.c src/mock_data.h
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase1.c src/mock_data.c \
		$(LDFLAGS_BREW) -lraylib -lm -lfribidi -o tests/test_phase1
	./tests/test_phase1

# ── Phase 5: Dashboard & Surah List ──
test_phase5: tests/test_phase5.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase5.c src/mock_data.c src/theme.c src/ui.c src/quran.c \
		$(LDFLAGS_BREW) -lraylib -lm -lfribidi -o tests/test_phase5
	./tests/test_phase5

# ── Phase 6: Ayah Reader & Surah Overview ──
test_phase6: tests/test_phase6.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) $(CFLAGS_BREW) tests/test_phase6.c src/mock_data.c src/theme.c src/ui.c src/quran.c \
		$(LDFLAGS_BREW) -lraylib -lm -lfribidi -o tests/test_phase6
	./tests/test_phase6

clean:
	rm -f $(TARGET) tests/test_phase1 tests/test_raylib tests/test_phase2 tests/test_phase3 tests/test_phase4 tests/test_phase5 tests/test_phase6
