/* ============================================================
 * search.c — Fuzzy search engine
 * Owned by: Systems & Features Engineer
 *
 * Responsibilities:
 *   - Fuzzy match search query against all Ayah translations
 *     and Surah names using fts_fuzzy_match
 *   - Rank and return top results
 *   - Render the search screen with live-typed query and results
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include "search.h"

/* TODO: #include "../lib/fts_fuzzy_match.h" once added */

int compareResults(const void *a, const void *b) {
    return ((SearchResult *)b)->score - ((SearchResult *)a)->score;
}

void runSearch(AppState *state, SearchResult *results, int *resultCount) {
    (void)state; (void)results;
    *resultCount = 0;
    /* TODO: linear scan all ayahs with fts_fuzzy_match, qsort by score */
}

void drawSearch(AppState *state, SearchResult *results, int resultCount) {
    (void)state; (void)results; (void)resultCount;
    /* TODO: search bar, typed query handling, ranked result list,
       Enter navigates to selected Ayah */
}
