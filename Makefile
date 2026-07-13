# Ayatika — Build configuration
# Run `make` to build, `make run` to build and execute, `make clean` to remove the binary.

# CONFIGURE: set CC and WINDRES to your MinGW-w64 paths
# If they are on your PATH, simply: CC = gcc / WINDRES = windres
CC       = C:/msys64/mingw64/bin/gcc
WINDRES  = C:/msys64/mingw64/bin/windres
CFLAGS   = -std=c11 -Wall -Wextra -O2 -I./src -I./lib -IC:/msys64/mingw64/include
LDFLAGS  = -LC:/msys64/mingw64/lib -lmingw32 -lraylib -lm -lcurl -lfribidi -lwinmm -lgdi32 -lopengl32
LIBS     = -mwindows $(LDFLAGS)
SRC      = $(wildcard src/*.c) $(wildcard lib/cJSON.c) $(wildcard lib/sqlite3.c)
TARGET   = ayatika

.PHONY: all run test test_phase5 test_phase6 test_phase7 clean

all: $(TARGET)

ayatika_manifest.o: manifest.rc ayatika.exe.manifest
	$(WINDRES) manifest.rc ayatika_manifest.o

$(TARGET): $(SRC) ayatika_manifest.o
	$(CC) $(CFLAGS) $(SRC) ayatika_manifest.o $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: test_phase2 test_phase4 test_phase5 test_phase6 test_phase7

test_phase2: tests/test_phase2.c src/mock_data.c src/theme.c
	$(CC) $(CFLAGS) tests/test_phase2.c src/mock_data.c src/theme.c $(LDFLAGS) -o tests/test_phase2
	./tests/test_phase2

test_phase3: tests/test_phase3.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase3.c src/mock_data.c src/theme.c src/ui.c src/quran.c $(LDFLAGS) -o tests/test_phase3
	./tests/test_phase3

test_phase4: tests/test_phase4.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase4.c src/mock_data.c src/theme.c src/ui.c src/quran.c $(LDFLAGS) -o tests/test_phase4
	./tests/test_phase4

# ── Phase 5: Dashboard & Surah List ──
test_phase5: tests/test_phase5.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase5.c src/mock_data.c src/theme.c src/ui.c src/quran.c \
		$(LDFLAGS) -o tests/test_phase5
	./tests/test_phase5

# ── Phase 6: Ayah Reader & Surah Overview ──
test_phase6: tests/test_phase6.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase6.c src/mock_data.c src/theme.c src/ui.c src/quran.c \
		$(LDFLAGS) -o tests/test_phase6
	./tests/test_phase6

# ── Phase 7: Input System & Vim Keybindings ──
test_phase7: tests/test_phase7.c src/mock_data.c src/theme.c src/ui.c src/input.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase7.c src/mock_data.c src/theme.c src/ui.c src/input.c src/quran.c \
		$(LDFLAGS) -o tests/test_phase7
	./tests/test_phase7

clean:
	rm -f $(TARGET) ayatika_manifest.o tests/test_phase2 tests/test_phase3 tests/test_phase4 tests/test_phase5 tests/test_phase6 tests/test_phase7
