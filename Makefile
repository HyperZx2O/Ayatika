# Ayatika — Build configuration
# Run `make` to build, `make run` to build and execute, `make clean` to remove the binary.

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I./src -I./lib
LIBS    = -lraylib -lm -lcurl -lsqlite3 -lfribidi
SRC     = $(wildcard src/*.c) lib/cJSON.c lib/sqlite3.c
TARGET  = ayatika

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
