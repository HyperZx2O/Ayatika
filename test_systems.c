/* ============================================================
 * test_systems.c — Phase 8 integration harness.
 * Wires audio, screensaver, and search together in one window,
 * mirroring how the Frontend's main.c will call them at full
 * integration (see ARCHITECTURE.md "Integration Dependency Order").
 *
 * Compile + run:
 *   make test              (builds + runs in --auto mode)
 *   gcc -std=c11 -Wall -Wextra -Isrc -Ilib src/audio.c \
 *       src/screensaver.c src/search.c src/mock_data.c \
 *       test_systems.c -lraylib -lm -o test_systems
 *
 * Modes:
 *   interactive (no args) — 1=search, 2=screensaver, 3=dashboard,
 *     C=click, S=switch, N=nature, A=azan, ESC to quit.
 *   --auto — scripted run of the same code paths with PASS/FAIL
 *     checks; used by `make test`. Exits 1 if any check fails.
 *
 * Run from the repo root (assets/ reachable).
 * Prints PASS/FAIL per check; exits non-zero if any fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "audio.h"
#include "screensaver.h"
#include "search.h"
#include "mock_data.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

/* Leaving the screensaver must reset it so the Azan can fire again
   next session — shared by the interactive keys and the --auto run. */
static void goToScreen(AppState *state, AppScreen screen) {
    if (state->currentScreen == SCREEN_SCREENSAVER && screen != SCREEN_SCREENSAVER)
        resetScreensaver();
    state->currentScreen = screen;
}

static void setQuery(AppState *state, const char *q) {
    strncpy(state->searchQuery, q, sizeof(state->searchQuery) - 1);
    state->searchQuery[sizeof(state->searchQuery) - 1] = '\0';
}

/* One update/draw frame, exactly as the real game loop will do:
   updateAudio() every frame, then dispatch on currentScreen. */
static void drawFrame(AppState *state, SearchResult *results, int resultCount) {
    updateAudio(state);
    BeginDrawing();
    switch (state->currentScreen) {
        case SCREEN_SEARCH:
            drawSearch(state, results, resultCount);
            break;
        case SCREEN_SCREENSAVER:
            drawScreensaver(state);   /* draws the cat internally */
            break;
        default:
            ClearBackground(BLACK);
            DrawText("Systems Test - 1=search 2=screensaver 3=dashboard",
                     20, 20, 18, WHITE);
            DrawText("Audio: C=click S=switch N=nature A=azan", 20, 50, 16, GRAY);
            break;
    }
    EndDrawing();
}

/* Scripted integration sequence for `make test`. */
static int runAuto(AppState *state, SearchResult *results, int *resultCount) {
    int haveAssets = FileExists("assets/azan.mp3");
    int haveCat    = FileExists("assets/cat.png");

    /* ── Search against the full mock dataset ── */
    goToScreen(state, SCREEN_SEARCH);
    setQuery(state, "mercy");
    runSearch(state, results, resultCount);
    check("search: 'mercy' returns results", *resultCount > 0);
    check("search: top 'mercy' score > 0", *resultCount > 0 && results[0].score > 0);

    setQuery(state, "Al-Fatiha");
    runSearch(state, results, resultCount);
    check("search: 'Al-Fatiha' returns boosted surah-1 match",
          *resultCount > 0 && results[0].surahNumber == 1 && results[0].score >= 500);
    for (int i = 0; i < 5; i++) drawFrame(state, results, *resultCount);

    /* ── Screensaver: Azan fires exactly once per session ── */
    goToScreen(state, SCREEN_SCREENSAVER);
    drawFrame(state, results, *resultCount);
    WaitTime(0.4);
    if (haveAssets)
        check("screensaver: azan plays after first draw", isAzanPlaying() == 1);
    else
        check("screensaver: azan no-op when asset missing", isAzanPlaying() == 0);

    for (int i = 0; i < 60; i++) {
        drawFrame(state, results, *resultCount);
        WaitTime(1.0 / 60.0);
    }
    stopAzan();
    WaitTime(0.1);
    drawFrame(state, results, *resultCount);
    drawFrame(state, results, *resultCount);
    WaitTime(0.1);
    check("screensaver: azan not replayed on later draws", isAzanPlaying() == 0);

    if (haveCat)
        check("screensaver: cat animation advances", getCatCurrentFrame() > 0);
    else
        check("screensaver: cat no-op when asset missing", getCatCurrentFrame() == 0);

    /* ── Leave + re-enter: reset allows the Azan to fire again ── */
    goToScreen(state, SCREEN_DASHBOARD);
    drawFrame(state, results, *resultCount);
    goToScreen(state, SCREEN_SCREENSAVER);
    drawFrame(state, results, *resultCount);
    WaitTime(0.4);
    if (haveAssets)
        check("screensaver: azan plays again after reset", isAzanPlaying() == 1);
    else
        check("screensaver: reset keeps no-op when asset missing", isAzanPlaying() == 0);

    /* ── Dashboard + audio keys ── */
    goToScreen(state, SCREEN_DASHBOARD);
    drawFrame(state, results, *resultCount);
    check("dashboard draws without crash", 1);

    playClickSfx();
    playSurahSwitchSfx();
    check("click + surah-switch sfx play without crash", 1);

    state->isNatureSoundOn = 0;
    toggleNatureSound(state);
    check("nature toggles on", state->isNatureSoundOn == 1);
    toggleNatureSound(state);
    check("nature toggles off", state->isNatureSoundOn == 0);

    playAzan();
    if (haveAssets)
        check("audio: azan plays on demand", isAzanPlaying() == 1);
    else
        check("audio: azan no-op when asset missing", isAzanPlaying() == 0);
    stopAzan();
    check("audio: azan stops cleanly", isAzanPlaying() == 0);

    return failures;
}

static int runInteractive(AppState *state, SearchResult *results, int *resultCount) {
    state->lastInputTime = GetTime();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE) || GetMouseDelta().x != 0.0f)
            state->lastInputTime = GetTime();

        if (IsKeyPressed(KEY_ONE))   goToScreen(state, SCREEN_SEARCH);
        if (IsKeyPressed(KEY_TWO))   goToScreen(state, SCREEN_SCREENSAVER);
        if (IsKeyPressed(KEY_THREE)) goToScreen(state, SCREEN_DASHBOARD);

        if (state->currentScreen == SCREEN_SEARCH)
            runSearch(state, results, resultCount);

        drawFrame(state, results, *resultCount);

        if (IsKeyPressed(KEY_C)) playClickSfx();
        if (IsKeyPressed(KEY_S)) playSurahSwitchSfx();
        if (IsKeyPressed(KEY_N)) toggleNatureSound(state);
        if (IsKeyPressed(KEY_A)) playAzan();
    }
    return 0;
}

int main(int argc, char **argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Ayatika - Systems Test");
    SetTargetFPS(60);

    AppState state;
    memset(&state, 0, sizeof(AppState));
    state.currentScreen = SCREEN_SEARCH;
    strncpy(state.language, "en", 7);
    loadMockData(&state);
    initAudio();
    initScreensaver();

    SearchResult results[MAX_SEARCH_RESULTS];
    int resultCount = 0;

    int rc = 0;
    if (argc > 1 && strcmp(argv[1], "--auto") == 0)
        rc = runAuto(&state, results, &resultCount);
    else
        runInteractive(&state, results, &resultCount);

    closeAudio();
    closeScreensaver();
    CloseWindow();

    if (rc > 0) {
        printf("%d integration check(s) FAILED\n", rc);
        return 1;
    }
    printf("All integration checks passed\n");
    return 0;
}
