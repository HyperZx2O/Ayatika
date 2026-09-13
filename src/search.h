#ifndef SEARCH_H
#define SEARCH_H

/* ============================================================
 * search.h — Fuzzy search engine
 * Owned by: Systems & Features Engineer
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include "quran.h"

void runSearch(AppState *state, SearchResult *results, int *resultCount); /* fuzzy-match the query against ayah translations + surah names; fill top-ranked results */
int  compareResults(const void *a, const void *b); /* qsort comparator for SearchResult, descending by score */

/* Test seams — small pure logic for the Finder's keyboard behaviour
   (typing, backspace, j/k navigation), verified headlessly. raylib has
   no key-injection API, so harnesses drive these directly instead of
   pressing real keys. */
void searchAppendChar(char *query, int maxLen, int ch);        /* append printable char, capped at maxLen-1 */
void searchBackspace(char *query);                             /* remove last char, no-op on empty */
int  searchMoveSelection(int current, int maxIndex, int delta); /* clamped j/k movement */

/* Go-to palette — fuzzy surah jumper (empty query = all, digits = number
   prefix, else fuzzy name match, best first). Returns match count. */
int  paletteFilter(AppState *state, const char *query, int *outIdx, int maxOut);

#endif /* SEARCH_H */
