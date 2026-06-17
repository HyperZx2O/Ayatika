#ifndef SEARCH_H
#define SEARCH_H

/* ============================================================
 * search.h — Fuzzy search engine
 * Owned by: Systems & Features Engineer
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include "quran.h"

void runSearch(AppState *state, SearchResult *results, int *resultCount);
void drawSearch(AppState *state, SearchResult *results, int resultCount);
int  compareResults(const void *a, const void *b);

#endif /* SEARCH_H */
