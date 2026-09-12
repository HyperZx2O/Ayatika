/* ============================================================
 * test_systems.c — Phase 8 integration harness.
 * Wires audio, screensaver, and search together in one window,
 * mirroring how the Frontend's main.c calls them at full
 * integration (see ARCHITECTURE.md "Integration Dependency Order").
 *
 * Compile + run:
 *   make test              (builds + runs in --auto mode)
 *
 * Modes:
 *   interactive (no args) — 1=finder, 2=screensaver, 3=dashboard,
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
#include "test_data.h"
#include "theme.h"
#include "ui.h"

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

/* Finder entry + query sync, mirroring input.c. */
static void openFinderAyah(AppState *state) {
    state->showGoToPalette = 1;
    state->paletteMode = 1;
    state->paletteSelection = 0;
}

static void setQuery(AppState *state, const char *q) {
    snprintf(state->paletteQuery, sizeof(state->paletteQuery), "%s", q);
    snprintf(state->searchQuery, sizeof(state->searchQuery), "%s", q);
    runSearch(state, state->searchResults, &state->searchResultCount);
}

/* One update/draw frame, exactly as the real game loop will do:
   updateAudio() every frame, then the real drawCurrentScreen. */
static void drawFrame(AppState *state) {
    updateAudio(state);
    BeginDrawing();
    if (state->showGoToPalette || state->currentScreen == SCREEN_SCREENSAVER)
        drawCurrentScreen(state);   /* screen + Finder overlay / screensaver */
    else {
        ClearBackground(BLACK);
        DrawText("Systems Test - 1=finder 2=screensaver 3=dashboard",
                 20, 20, 18, WHITE);
        DrawText("Audio: C=click S=switch N=nature A=azan", 20, 50, 16, GRAY);
    }
    EndDrawing();
}

/* Scripted integration sequence for `make test`. */
static int runAuto(AppState *state) {
    int haveAssets = FileExists("assets/azan.mp3");
    int haveCat    = FileExists("assets/cat.png");

    /* ── Finder ayah search against the mock dataset ── */
    goToScreen(state, SCREEN_DASHBOARD);
    openFinderAyah(state);
    setQuery(state, "mercy");
    check("finder: 'mercy' returns results", state->searchResultCount > 0);
    check("finder: top 'mercy' score > 0",
          state->searchResultCount > 0 && state->searchResults[0].score > 0);

    setQuery(state, "Al-Fatiha");
    check("finder: 'Al-Fatiha' returns boosted surah-1 match",
          state->searchResultCount > 0 && state->searchResults[0].surahNumber == 1
                                       && state->searchResults[0].score >= 500);
    for (int i = 0; i < 5; i++) drawFrame(state);
    state->showGoToPalette = 0;

    /* ── Screensaver draws silent; the alarm owns the Azan ── */
    goToScreen(state, SCREEN_SCREENSAVER);
    drawFrame(state);
    WaitTime(0.4);
    check("screensaver: idle draws silent", isAzanPlaying() == 0);

    for (int i = 0; i < 60; i++) {
        drawFrame(state);
        WaitTime(1.0 / 60.0);
    }
    firePrayerAlarm(state);
    WaitTime(0.4);
    if (haveAssets) {
        check("alarm: enters screensaver", state->currentScreen == SCREEN_SCREENSAVER);
        check("alarm: azan plays", isAzanPlaying() == 1);
    } else {
        check("alarm: no-op when asset missing", isAzanPlaying() == 0);
    }
    stopAzan();
    WaitTime(0.1);
    check("alarm: azan stops cleanly", isAzanPlaying() == 0);

    if (haveCat)
        check("screensaver: cat animation advances", getCatCurrentFrame() > 0);
    else
        check("screensaver: cat no-op when asset missing", getCatCurrentFrame() == 0);

    /* ── Leave + re-enter stays silent; alarm re-fires on demand ── */
    goToScreen(state, SCREEN_DASHBOARD);
    drawFrame(state);
    goToScreen(state, SCREEN_SCREENSAVER);
    drawFrame(state);
    WaitTime(0.4);
    check("screensaver: re-entry stays silent", isAzanPlaying() == 0);
    firePrayerAlarm(state);
    WaitTime(0.4);
    if (haveAssets)
        check("alarm: plays again on demand", isAzanPlaying() == 1);
    else
        check("alarm: still no-op when asset missing", isAzanPlaying() == 0);
    stopAzan();

    /* ── Dashboard + audio keys ── */
    goToScreen(state, SCREEN_DASHBOARD);
    drawFrame(state);
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

static int runInteractive(AppState *state) {
    state->lastInputTime = GetTime();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE) || GetMouseDelta().x != 0.0f)
            state->lastInputTime = GetTime();

        if (IsKeyPressed(KEY_ONE))   { goToScreen(state, SCREEN_DASHBOARD); openFinderAyah(state); }
        if (IsKeyPressed(KEY_TWO))   { state->showGoToPalette = 0; goToScreen(state, SCREEN_SCREENSAVER); }
        if (IsKeyPressed(KEY_THREE)) { state->showGoToPalette = 0; goToScreen(state, SCREEN_DASHBOARD); }

        if (state->showGoToPalette && state->paletteMode == 1)
            runSearch(state, state->searchResults, &state->searchResultCount);

        drawFrame(state);

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
    state.currentScreen = SCREEN_DASHBOARD;
    strncpy(state.language, "en", 7);
    loadTestData(&state);
    initThemes();
    initFonts(&state);
    S = computeScale(1280, 720);
    initAudio();
    initScreensaver();

    int rc = 0;
    if (argc > 1 && strcmp(argv[1], "--auto") == 0)
        rc = runAuto(&state);
    else
        runInteractive(&state);

    closeAudio();
    closeScreensaver();
    closeFonts();
    CloseWindow();

    if (rc > 0) {
        printf("%d integration check(s) FAILED\n", rc);
        return 1;
    }
    printf("All integration checks passed\n");
    return 0;
}

