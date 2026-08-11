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
#include <ctype.h>
#include "search.h"
#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "../lib/fts_fuzzy_match.h"

/* ponytail: fts_fuzzy_match scores matches in long strings negative
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

    /* ponytail: 6500-entry stack array — the Quran has ~6236 ayahs, each
       SearchResult is ~136 bytes → ~885 KB, within the desktop app stack.
       If that ever becomes a problem, switch to a running top-N list. */
    SearchResult all[6500];
    int allCount = 0;

    for (int i = 0; i < state->totalAyahs && allCount < 6500; i++) {
        Ayah *ayah = &state->ayahs[i];

        /* Search in English translation */
        char textLower[2048];
        toLowerStr(ayah->translationEn, textLower, sizeof(textLower));

        int score = 0;
        if (fts_fuzzy_match(queryLower, textLower, &score)) {
            all[allCount].score       = score;
            all[allCount].surahNumber = ayah->surahNumber;
            all[allCount].ayahNumber  = ayah->ayahNumber;
            strncpy(all[allCount].preview, ayah->translationEn, 119);
            all[allCount].preview[119] = '\0';
            allCount++;
        }

        /* Also search the Surah name — score is reset because
           fts_fuzzy_match leaves outScore untouched on a non-match,
           so a stale translation score would create a false hit. */
        score = 0;
        if (ayah->surahNumber >= 1 && ayah->surahNumber <= TOTAL_SURAHS) {
            char surahNameLower[64];
            toLowerStr(state->surahs[ayah->surahNumber - 1].name,
                       surahNameLower, sizeof(surahNameLower));
            if (fts_fuzzy_match(queryLower, surahNameLower, &score)) {
                /* Boost Surah name matches above translation matches */
                all[allCount].score       = score + 500;
                all[allCount].surahNumber = ayah->surahNumber;
                all[allCount].ayahNumber  = ayah->ayahNumber;
                strncpy(all[allCount].preview, ayah->translationEn, 119);
                all[allCount].preview[119] = '\0';
                allCount++;
            }
        }
    }

    qsort(all, allCount, sizeof(SearchResult), compareResults);

    *resultCount = (allCount < MAX_SEARCH_RESULTS) ? allCount : MAX_SEARCH_RESULTS;
    for (int i = 0; i < *resultCount; i++)
        results[i] = all[i];
}

void drawSearch(AppState *state, SearchResult *results, int resultCount) {
    (void)state; (void)results; (void)resultCount;
    /* TODO (Phase 7): search bar, typed query handling, ranked result list,
       Enter navigates to selected Ayah */
}
