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

/* Test seams — small pure logic extracted from drawSearch so the search
   screen's keyboard behaviour (typing, backspace, j/k navigation) can be
   verified headlessly. raylib has no key-injection API, so the harness
   drives these directly instead of pressing real keys. */
void searchAppendChar(char *query, int maxLen, int ch);        /* append printable char, capped at maxLen-1 */
void searchBackspace(char *query);                             /* remove last char, no-op on empty */
int  searchMoveSelection(int current, int maxIndex, int delta); /* clamped j/k movement */

#endif /* SEARCH_H */
