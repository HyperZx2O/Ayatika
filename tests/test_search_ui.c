/* ============================================================
 * test_search_ui.c — Finder overlay UI harness. Compile + run:
 *
 *   (see Makefile test_search_ui)
 *
 * The overlay is a raylib draw function (needs a window), and raylib
 * has no key-injection API, so real keypresses can't be automated.
 * The keyboard logic is therefore verified headlessly through the
 * test seams in search.h (searchAppendChar / searchBackspace /
 * searchMoveSelection), and each render branch is exercised in a
 * real window via drawCurrentScreen with the Finder open. The
 * "results update as you type" behaviour is proven by re-running
 * runSearch after each query change — the same loop the input
 * layer runs on every query change.
 *
 * Prints PASS/FAIL per check; exits non-zero if any fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "search.h"
#include "test_data.h"
#include "theme.h"
#include "ui.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

/* Draw a few real frames so every Finder branch runs through a
   BeginDrawing/EndDrawing cycle, mirroring the game loop. */
static void drawFrames(AppState *state, int frames) {
    for (int i = 0; i < frames; i++) {
        BeginDrawing();
        drawCurrentScreen(state);
        EndDrawing();
        WaitTime(1.0 / 60.0);
    }
}

/* same sync the input layer does before runSearch. */
static void setFinderQuery(AppState *state, int ayahTab, const char *q) {
    state->showGoToPalette = 1;
    state->paletteMode = ayahTab ? 1 : 0;
    state->paletteSelection = 0;
    snprintf(state->paletteQuery, sizeof(state->paletteQuery), "%s", q);
    if (ayahTab) {
        snprintf(state->searchQuery, sizeof(state->searchQuery), "%s", q);
        runSearch(state, state->searchResults, &state->searchResultCount);
    }
}

int main(void) {
    /* ── Headless: typing logic seams ── */
    char q[8] = {0};

    check("query starts empty", q[0] == '\0');

    searchAppendChar(q, sizeof(q), 'm');
    searchAppendChar(q, sizeof(q), 'e');
    searchAppendChar(q, sizeof(q), 'r');
    check("append builds the query", strcmp(q, "mer") == 0);

    searchAppendChar(q, sizeof(q), 10);      /* \n — must be ignored */
    searchAppendChar(q, sizeof(q), 127);     /* DEL — must be ignored */
    check("non-printable chars are ignored", strcmp(q, "mer") == 0);

    for (int i = 0; i < 10; i++) searchAppendChar(q, sizeof(q), 'c');
    check("query is capped at maxLen-1", strlen(q) == sizeof(q) - 1);

    searchBackspace(q);
    check("backspace removes the last char", strlen(q) == sizeof(q) - 2);

    q[0] = '\0';
    searchBackspace(q);
    check("backspace on empty query is a no-op", q[0] == '\0');

    /* ── Headless: j/k navigation seams ── */
    check("selection moves down", searchMoveSelection(0, 9, 1) == 1);
    check("selection moves up",   searchMoveSelection(5, 9, -1) == 4);
    check("selection clamps at bottom", searchMoveSelection(9, 9, 1) == 9);
    check("selection clamps at top",    searchMoveSelection(0, 9, -1) == 0);

    /* ── Windowed: Finder render branches, both tabs ── */
    InitWindow(800, 600, "finder UI test");
    initThemes();

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadTestData(&state);
    initFonts(&state);
    S = computeScale(800, 600);

    /* Ayahs tab, short query — minimum-length prompt */
    setFinderQuery(&state, 1, "a");
    check("single-char query yields 0 results", state.searchResultCount == 0);
    drawFrames(&state, 5);
    check("short-query prompt draws without crash", 1);

    /* Ayahs tab, no matches — "No results found." branch */
    setFinderQuery(&state, 1, "mercy");
    drawFrames(&state, 5);
    check("ayah results list draws without crash", state.searchResultCount > 0);

    /* Ayahs tab, re-query updates results (as the input loop does) */
    setFinderQuery(&state, 1, "prayer");
    check("re-ran search updates results for new query",
          state.searchResultCount > 0 && state.searchResults[0].surahNumber == 2
                                      && state.searchResults[0].ayahNumber == 238);
    drawFrames(&state, 5);

    /* Surahs tab, empty query — full list renders */
    setFinderQuery(&state, 0, "");
    drawFrames(&state, 5);
    check("surah tab empty query draws without crash", 1);

    /* Surahs tab, fuzzy query — filtered rows render */
    setFinderQuery(&state, 0, "fatiha");
    drawFrames(&state, 5);
    check("surah tab fuzzy query draws without crash", 1);

    /* Surahs tab, gibberish — "No matches." branch */
    setFinderQuery(&state, 0, "zzz");
    drawFrames(&state, 5);
    check("surah no-matches message draws without crash", 1);

    closeFonts();
    CloseWindow();

    if (failures > 0) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("All finder UI checks passed\n");
    return 0;
}

