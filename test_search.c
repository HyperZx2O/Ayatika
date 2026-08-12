/* ============================================================
 * test_search.c — Phase 6 test harness for the fuzzy search engine.
 * Compile + run (standalone, headless — no window needed):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc -Ilib test_search.c \
 *       src/search.c src/mock_data.c -lraylib -lm -o test_search
 *
 * (search.c now contains the drawSearch UI from Phase 7, so the link
 * line needs raylib even though this harness never opens a window.)
 *
 * Exercises runSearch against the mock dataset: relevance for the
 * seeded queries, the surah-name boost, the minimum-length guard,
 * descending score sort, and preview truncation. If mock data is
 * changed, the exact-count checks below must be revisited.
 *
 * Prints PASS/FAIL per check; exits non-zero if any fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "search.h"
#include "mock_data.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

static void setQuery(AppState *state, const char *q) {
    strncpy(state->searchQuery, q, sizeof(state->searchQuery) - 1);
    state->searchQuery[sizeof(state->searchQuery) - 1] = '\0';
}

static int isSortedDesc(SearchResult *results, int count) {
    for (int i = 1; i < count; i++)
        if (results[i - 1].score < results[i].score)
            return 0;
    return 1;
}

int main(void) {
    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadMockData(&state);

    SearchResult results[MAX_SEARCH_RESULTS];
    int count;

    /* Empty and too-short queries must be safe no-ops */
    setQuery(&state, "");
    runSearch(&state, results, &count);
    check("empty query returns 0 results", count == 0);

    setQuery(&state, "a");
    runSearch(&state, results, &count);
    check("single-char query returns 0 results", count == 0);

    /* Seeded relevance queries — top hit pinned to mock data.
       Note: fuzzy matching is subsequence-based, so a query may also
       match shorter/longer ayahs as a character subsequence (e.g.
       "prayer" also matches 2:157), which is correct behaviour. */
    setQuery(&state, "mercy");
    runSearch(&state, results, &count);
    check("'mercy' returns results", count > 0);
    check("'mercy' result has non-zero score", count > 0 && results[0].score > 0);
    check("'mercy' results sorted by score", isSortedDesc(results, count));

    setQuery(&state, "prayer");
    runSearch(&state, results, &count);
    check("'prayer' returns results", count > 0);
    check("'prayer' top result is 2:238",
          count > 0 && results[0].surahNumber == 2 && results[0].ayahNumber == 238);
    check("'prayer' results sorted by score", isSortedDesc(results, count));

    setQuery(&state, "light");
    runSearch(&state, results, &count);
    check("'light' returns results", count > 0);
    check("'light' top result is 2:257",
          count > 0 && results[0].surahNumber == 2 && results[0].ayahNumber == 257);

    /* Surah-name matching with boost */
    setQuery(&state, "Al-Fatiha");
    runSearch(&state, results, &count);
    check("'Al-Fatiha' returns at least 5 results", count >= 5);
    check("'Al-Fatiha' top result is a boosted surah 1 match (>=500)",
          count > 0 && results[0].surahNumber == 1 && results[0].score >= 500);
    check("'Al-Fatiha' top 5 results all from surah 1",
          count >= 5 && results[0].surahNumber == 1
                      && results[1].surahNumber == 1
                      && results[4].surahNumber == 1);
    check("'Al-Fatiha' results sorted by score", isSortedDesc(results, count));

    /* Preview truncation on a long ayah (2:255, Ayat al-Kursi).
       "Kursi" is used instead of a longer word because fts_fuzzy_match
       has a hardcoded recursion limit of 10, which long patterns can
       exhaust against a ~450-char string. */
    setQuery(&state, "Kursi");
    runSearch(&state, results, &count);
    check("'Kursi' returns exactly 1 result", count == 1);
    check("long-ayah preview is truncated to 119 chars",
          count == 1 && strlen(results[0].preview) == 119
                      && results[0].preview[119] == '\0');

    /* Full dataset scan without crash */
    for (int q = 0; q < 50; q++) {
        setQuery(&state, "most");
        runSearch(&state, results, &count);
        setQuery(&state, "Allah");
        runSearch(&state, results, &count);
    }
    check("repeated scans over full dataset do not crash", 1);
    check("'Allah' caps at MAX_SEARCH_RESULTS",
          (setQuery(&state, "Allah"), runSearch(&state, results, &count),
           count <= MAX_SEARCH_RESULTS));

    if (failures > 0) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("All search checks passed\n");
    return 0;
}
