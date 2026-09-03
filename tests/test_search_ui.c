/* ============================================================
 * test_search_ui.c — Phase 7 test harness for the search
 * screen UI. Compile + run (standalone):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc test_search_ui.c \
 *       src/search.c src/mock_data.c -lraylib -lm -o test_search_ui
 *
 * drawSearch is a raylib draw function (needs a window), and raylib
 * has no key-injection API, so real keypresses can't be automated.
 * The keyboard logic is therefore verified headlessly through the
 * test seams in search.h (searchAppendChar / searchBackspace /
 * searchMoveSelection), and each render branch is exercised in a
 * real window. The "results update as you type" behaviour is proven
 * by re-running runSearch after each query change — the same loop
 * the real app runs every frame on the search screen.
 *
 * Prints PASS/FAIL per check; exits non-zero if any fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "search.h"
#include "mock_data.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

/* Draw a few real frames so every drawSearch branch runs through a
   BeginDrawing/EndDrawing cycle, mirroring the game loop. */
static void drawFrames(AppState *state, SearchResult *results, int resultCount, int frames) {
    for (int i = 0; i < frames; i++) {
        BeginDrawing();
        drawSearch(state, results, resultCount);
        EndDrawing();
        WaitTime(1.0 / 60.0);
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

    /* ── Windowed: render branches ── */
    InitWindow(800, 600, "search UI test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadMockData(&state);

    SearchResult results[MAX_SEARCH_RESULTS];
    int resultCount = 0;

    /* Short query — minimum-length prompt */
    strncpy(state.searchQuery, "a", sizeof(state.searchQuery) - 1);
    state.searchQuery[sizeof(state.searchQuery) - 1] = '\0';
    runSearch(&state, results, &resultCount);
    check("single-char query yields 0 results", resultCount == 0);
    drawFrames(&state, results, resultCount, 5);
    check("short-query prompt draws without crash", 1);

    /* Long query, no matches — "No results found." branch.
       Driven directly with resultCount == 0 so the branch is
       guaranteed reachable (fuzzy subsequence matching makes finding
       a string absent from the whole dataset unreliable). */
    strncpy(state.searchQuery, "mercy", sizeof(state.searchQuery) - 1);
    state.searchQuery[sizeof(state.searchQuery) - 1] = '\0';
    drawFrames(&state, results, 0, 5);
    check("no-results message draws without crash", 1);

    /* Query with matches — results list renders */
    runSearch(&state, results, &resultCount);
    check("'mercy' returns results", resultCount > 0);
    drawFrames(&state, results, resultCount, 10);
    check("results list draws without crash", 1);

    /* "Results update as you type" — change the query, re-run the
       search (as the real loop does every frame), results change. */
    strncpy(state.searchQuery, "prayer", sizeof(state.searchQuery) - 1);
    state.searchQuery[sizeof(state.searchQuery) - 1] = '\0';
    runSearch(&state, results, &resultCount);
    check("re-ran search updates results for new query",
          resultCount > 0 && results[0].surahNumber == 2
                          && results[0].ayahNumber == 238);
    drawFrames(&state, results, resultCount, 5);

    /* Surah-name boost path renders too */
    strncpy(state.searchQuery, "Al-Fatiha", sizeof(state.searchQuery) - 1);
    state.searchQuery[sizeof(state.searchQuery) - 1] = '\0';
    runSearch(&state, results, &resultCount);
    check("surah-name search returns results", resultCount >= 5);
    drawFrames(&state, results, resultCount, 5);

    CloseWindow();

    if (failures > 0) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("All search UI checks passed\n");
    return 0;
}
