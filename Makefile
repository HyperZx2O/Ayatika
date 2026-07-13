# Ayatika — Zero-dependency build
# Requirements: gcc (MinGW-w64) on PATH.
# Everything else is vendored in lib/.

CC       = gcc
WINDRES  = windres
CFLAGS   = -std=c11 -Wall -Wextra -O2 -I./src -I./lib/include
LIBS       = -mwindows lib/libraylib.a lib/libglfw3.dll.a lib/libfribidi.a \
            -lmingw32 -lm -lwinmm -lgdi32 -lopengl32
LIBS_NOGUI = lib/libraylib.a lib/libglfw3.dll.a lib/libfribidi.a \
            -lmingw32 -lm -lwinmm -lgdi32 -lopengl32
SRC        = $(wildcard src/*.c) lib/sqlite3.c
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
	$(CC) $(CFLAGS) tests/test_phase2.c src/mock_data.c src/theme.c $(LIBS_NOGUI) -o tests/test_phase2
	./tests/test_phase2

test_phase3: tests/test_phase3.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase3.c src/mock_data.c src/theme.c src/ui.c src/quran.c $(LIBS_NOGUI) -o tests/test_phase3
	./tests/test_phase3

test_phase4: tests/test_phase4.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase4.c src/mock_data.c src/theme.c src/ui.c src/quran.c $(LIBS_NOGUI) -o tests/test_phase4
	./tests/test_phase4

# ── Phase 5: Dashboard & Surah List ──
test_phase5: tests/test_phase5.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase5.c src/mock_data.c src/theme.c src/ui.c src/quran.c \
		$(LIBS_NOGUI) -o tests/test_phase5
	./tests/test_phase5

# ── Phase 6: Ayah Reader & Surah Overview ──
test_phase6: tests/test_phase6.c src/mock_data.c src/theme.c src/ui.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase6.c src/mock_data.c src/theme.c src/ui.c src/quran.c \
		$(LIBS_NOGUI) -o tests/test_phase6
	./tests/test_phase6

# ── Phase 7: Input System & Vim Keybindings ──
test_phase7: tests/test_phase7.c src/mock_data.c src/theme.c src/ui.c src/input.c src/quran.c
	$(CC) $(CFLAGS) tests/test_phase7.c src/mock_data.c src/theme.c src/ui.c src/input.c src/quran.c \
		$(LIBS_NOGUI) -o tests/test_phase7
	./tests/test_phase7

clean:
	rm -f $(TARGET) ayatika_manifest.o tests/test_phase2 tests/test_phase3 tests/test_phase4 tests/test_phase5 tests/test_phase6 tests/test_phase7
