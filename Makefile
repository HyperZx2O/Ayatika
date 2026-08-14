# Ayatika — Build configuration
# Run `make` to build, `make run` to build and execute, `make test` to build
# and run the systems integration harness, `make clean` to remove the binary.
# ponytail: vendored lib/cJSON.c / lib/sqlite3.c are wildcarded so the build
# works before the Backend lands them; LIBS carries only what is installed and
# used today. Backend/Frontend re-add -lcurl/-lsqlite3/-lfribidi with their code.

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I./src -I./lib
LIBS    = -lraylib -lm
SRC     = $(wildcard src/*.c) $(wildcard lib/cJSON.c) $(wildcard lib/sqlite3.c)
TARGET  = ayatika

.PHONY: all run clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

# Phase 9: `make test` builds and runs every harness (audio, search, search
# UI, screensaver, cat, and the full integration harness). Each prints
# PASS/FAIL per check and exits non-zero if any check fails.
test: test_audio test_search test_search_ui test_screensaver test_cat test_systems
	./test_search
	./test_audio
	./test_search_ui
	./test_screensaver
	./test_cat
	./test_systems --auto

test_audio: test_audio.c src/audio.c src/mock_data.c
	$(CC) $(CFLAGS) src/audio.c src/mock_data.c test_audio.c $(LIBS) -o test_audio

test_search: test_search.c src/search.c src/mock_data.c
	$(CC) $(CFLAGS) src/search.c src/mock_data.c test_search.c $(LIBS) -o test_search

test_search_ui: test_search_ui.c src/search.c src/mock_data.c
	$(CC) $(CFLAGS) src/search.c src/mock_data.c test_search_ui.c $(LIBS) -o test_search_ui

test_screensaver: test_screensaver.c src/audio.c src/screensaver.c src/mock_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/mock_data.c test_screensaver.c $(LIBS) -o test_screensaver

test_cat: test_cat.c src/audio.c src/screensaver.c src/mock_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/mock_data.c test_cat.c $(LIBS) -o test_cat

test_systems: test_systems.c src/audio.c src/screensaver.c src/search.c src/mock_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/search.c src/mock_data.c test_systems.c $(LIBS) -o test_systems

clean:
	rm -f $(TARGET) test_audio test_search test_search_ui test_screensaver test_cat test_systems
