/* ============================================================
 * search.c — Fuzzy search engine
 * Owned by: Systems & Features Engineer
 *
 * Responsibilities:
 *   - Fuzzy match search query against all Ayah translations
 *     and Surah names using fts_fuzzy_match
 *   - Rank and return top results (ayah engine + surah palette filter)
 *
 * Rendering lives in ui.c (unified Finder overlay); input in input.c.
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "search.h"
#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "../lib/fts_fuzzy_match.h"

/* fts_fuzzy_match scores matches in long strings negative
   (unmatched_letter_penalty = -1 per unmatched char), so a 508-char
   verse can never return a positive score. We therefore do NOT gate on
   score > 0 — the score is only used for ranking. recursionLimit in
   lib/fts_fuzzy_match.h was raised 10 → 256 so deep matches succeed. */

static void toLowerStr(const char *in, char *out, int maxLen) {
    int i;
    for (i = 0; i < maxLen - 1 && in[i]; i++)
        out[i] = (char)tolower((unsigned char)in[i]);
    out[i] = '\0';
}

int compareResults(const void *a, const void *b) {
    return ((SearchResult *)b)->score - ((SearchResult *)a)->score;
}

void runSearch(AppState *state, SearchResult *results, int *resultCount) {
    *resultCount = 0;
    if (strlen(state->searchQuery) < 2) return;   /* need at least 2 chars */

    char queryLower[256];
    toLowerStr(state->searchQuery, queryLower, sizeof(queryLower));

    /* static, not stack — 6500 x ~136B ≈ 885KB blew the 1MB stack. */
    static SearchResult all[6500];
    int allCount = 0;

    int useBn = (state->language[0] == 'b');

    for (int i = 0; i < state->totalAyahs && allCount < 6500; i++) {
        Ayah *ayah = &state->ayahs[i];

        /* Search the active-language translation */
        const char *tr = (useBn && ayah->translationBn[0]) ? ayah->translationBn : ayah->translationEn;
        char textLower[2048];
        toLowerStr(tr, textLower, sizeof(textLower));

        int score = 0;
        if (fts_fuzzy_match(queryLower, textLower, &score)) {
            all[allCount].score       = score;
            all[allCount].surahNumber = ayah->surahNumber;
            all[allCount].ayahNumber  = ayah->ayahNumber;
            snprintf(all[allCount].preview, sizeof(all[allCount].preview), "%.119s", tr);
            allCount++;
        }

        /* Also search the Surah name — score is reset because
           fts_fuzzy_match leaves outScore untouched on a non-match,
           so a stale translation score would create a false hit. */
        score = 0;
        if (ayah->surahNumber >= 1 && ayah->surahNumber <= state->surahCount && state->surahs) {
            char surahNameLower[64];
            toLowerStr(state->surahs[ayah->surahNumber - 1].name,
                       surahNameLower, sizeof(surahNameLower));
            if (fts_fuzzy_match(queryLower, surahNameLower, &score)) {
                /* Boost Surah name matches above translation matches */
                all[allCount].score       = score + 500;
                all[allCount].surahNumber = ayah->surahNumber;
                all[allCount].ayahNumber  = ayah->ayahNumber;
                snprintf(all[allCount].preview, sizeof(all[allCount].preview), "%.119s", tr);
                allCount++;
            }
        }
    }

    qsort(all, allCount, sizeof(SearchResult), compareResults);

    *resultCount = (allCount < MAX_SEARCH_RESULTS) ? allCount : MAX_SEARCH_RESULTS;
    for (int i = 0; i < *resultCount; i++)
        results[i] = all[i];
}

/* ── Go-to palette filter (see search.h) ── */

typedef struct { int score; int idx; } PalHit;
static int cmpPalHit(const void *a, const void *b) {
    return ((const PalHit *)b)->score - ((const PalHit *)a)->score;
}

int paletteFilter(AppState *state, const char *query, int *outIdx, int maxOut) {
    if (!state || !state->surahs || !outIdx || maxOut <= 0) return 0;
    if (!query || !query[0]) {
        int n = state->surahCount < maxOut ? state->surahCount : maxOut;
        for (int i = 0; i < n; i++) outIdx[i] = i;
        return n;
    }
    /* Digits → surah-number prefix ("11" finds 11, 110-114). */
    int digits = 1;
    for (const char *p = query; *p; p++)
        if (!isdigit((unsigned char)*p)) { digits = 0; break; }
    if (digits) {
        int n = 0, qlen = (int)strlen(query);
        for (int i = 0; i < state->surahCount && n < maxOut; i++) {
            char num[8];
            snprintf(num, sizeof(num), "%d", state->surahs[i].number);
            if (strncmp(num, query, (size_t)qlen) == 0) outIdx[n++] = i;
        }
        return n;
    }
    /* Else fuzzy over lowercase surah names, best first. */
    char qLower[64];
    toLowerStr(query, qLower, sizeof(qLower));
    static PalHit hits[128];
    int n = 0;
    for (int i = 0; i < state->surahCount && n < 128; i++) {
        char nameLower[64];
        toLowerStr(state->surahs[i].name, nameLower, sizeof(nameLower));
        int score = 0;
        if (fts_fuzzy_match(qLower, nameLower, &score))
            hits[n++] = (PalHit){score, i};
    }
    qsort(hits, (size_t)n, sizeof(PalHit), cmpPalHit);
    int out = n < maxOut ? n : maxOut;
    for (int i = 0; i < out; i++) outIdx[i] = hits[i].idx;
    return out;
}

/* ── Test seams (see search.h) ── */

void searchAppendChar(char *query, int maxLen, int ch) {
    int len = strlen(query);
    if (ch < 32 || ch > 126) return;      /* printable ASCII only */
    if (len >= maxLen - 1) return;        /* keep room for NUL */
    query[len]     = (char)ch;
    query[len + 1] = '\0';
}

void searchBackspace(char *query) {
    int len = strlen(query);
    if (len > 0) query[len - 1] = '\0';
}

int searchMoveSelection(int current, int maxIndex, int delta) {
    int next = current + delta;
    if (next < 0)      next = 0;
    if (next > maxIndex) next = maxIndex;
    return next;
}


