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

test: test_systems
	./test_systems --auto

test_systems: src/audio.c src/screensaver.c src/search.c src/mock_data.c test_systems.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/search.c src/mock_data.c test_systems.c -lraylib -lm -o test_systems

clean:
	rm -f $(TARGET)
