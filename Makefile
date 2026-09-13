# Ayatika — Build configuration
# `make` builds the live app (no mock data), `make run` runs it,
# `make test` builds + runs all harnesses, `make clean` removes binaries.

CC      = gcc
# lib/include BEFORE lib — <raylib.h> must hit the real header,
# not the lib/raylib.h backend stub. -I./lib still covers cJSON/sqlite3.
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I./src -I./lib/include -I./lib
LDFLAGS = -L./lib
WINLIBS = -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm
# HTTP via system WinINet (-lwininet); no libcurl in tree
# (it would need nghttp2/zstd/brotli/openssl — dependency hell for one GET).
LIBS    = $(WINLIBS) -lfribidi -lwininet -lm -lws2_32
# sqlite3/cJSON compile vendored; system -lsqlite3 not needed.
SRC     = $(wildcard src/*.c)
# vendored libs build silent (-w) via implicit rule; strict flags
# stay on our code only.
LIBOBJ  = lib/cJSON.o lib/sqlite3.o
lib/cJSON.o lib/sqlite3.o: CFLAGS := -std=c11 -O2 -w
# embed the app manifest (PerMonitorV2 DPI + asInvoker) via windres.
WINDRES = windres
RES     = manifest.res
TARGET  = ayatika

.PHONY: all run clean test test-headless

all: $(TARGET)

$(RES): manifest.rc ayatika.exe.manifest
	$(WINDRES) $< -O coff -o $@

$(TARGET): $(SRC) $(LIBOBJ) $(RES)
	$(CC) $(CFLAGS) $(SRC) $(LIBOBJ) $(RES) $(LDFLAGS) $(LIBS) -mwindows -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

# `make test-headless` runs only the window-free harnesses:
# backend (data, prayer incl. alert schedule, db, config) + search.
# Runs anywhere, no screen or sound needed — suitable for CI.
test-headless: test_backend test_search
	./tests/test_backend
	./tests/test_search

# `make test` builds and runs every harness. Each prints PASS/FAIL per
# check and exits non-zero if any check fails.
test: test_backend test_audio test_search test_search_ui test_screensaver test_cat test_systems
	./tests/test_backend
	./tests/test_search
	./tests/test_audio
	./tests/test_search_ui
	./tests/test_screensaver
	./tests/test_cat
	./tests/test_systems --auto

test_backend: tests/test_backend.c src/quran.c src/prayer.c src/db.c src/surah_meta.c src/config.c $(LIBOBJ)
	$(CC) $(CFLAGS) src/quran.c src/prayer.c src/db.c src/surah_meta.c src/config.c $(LIBOBJ) tests/test_backend.c $(LDFLAGS) -lwininet -lws2_32 -lm -o tests/test_backend

test_audio: tests/test_audio.c src/audio.c src/prayer.c src/screensaver.c src/search.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c
	$(CC) $(CFLAGS) src/audio.c src/prayer.c src/screensaver.c src/search.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c tests/test_audio.c $(LDFLAGS) $(LIBS) -o tests/test_audio

test_search: tests/test_search.c src/audio.c src/screensaver.c src/search.c src/prayer.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/search.c src/prayer.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c tests/test_search.c $(LDFLAGS) $(LIBS) -lm -o tests/test_search

test_search_ui: tests/test_search_ui.c src/audio.c src/screensaver.c src/search.c src/prayer.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/search.c src/prayer.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c tests/test_search_ui.c $(LDFLAGS) $(LIBS) -lm -o tests/test_search_ui

test_screensaver: tests/test_screensaver.c src/audio.c src/screensaver.c tests/test_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/search.c src/prayer.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c tests/test_screensaver.c $(LDFLAGS) $(LIBS) -o tests/test_screensaver

test_cat: tests/test_cat.c src/audio.c src/screensaver.c tests/test_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/search.c src/prayer.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c tests/test_cat.c $(LDFLAGS) $(LIBS) -o tests/test_cat

test_systems: tests/test_systems.c src/audio.c src/screensaver.c src/search.c tests/test_data.c
	$(CC) $(CFLAGS) src/audio.c src/screensaver.c src/search.c src/prayer.c src/ui.c src/theme.c src/input.c src/quran.c src/db.c src/config.c src/surah_meta.c $(LIBOBJ) tests/test_data.c tests/test_systems.c $(LDFLAGS) $(LIBS) -o tests/test_systems

clean:
ifeq ($(OS),Windows_NT)
	-del /f /q $(TARGET).exe $(RES) tests\*.exe lib\*.o 2>nul
else
	rm -f $(TARGET) $(RES) tests/test_backend tests/test_audio tests/test_search tests/test_search_ui tests/test_screensaver tests/test_cat tests/test_systems lib/cJSON.o lib/sqlite3.o
endif

