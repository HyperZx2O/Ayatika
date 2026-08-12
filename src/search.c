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
#include <stdio.h>
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

/* ── Search screen ──
 * Self-contained UI for standalone testing. Hardcoded dark colours —
 * the Frontend theme system takes over at integration (Phase 8).
 * Note: this screen handles its own keyboard input for the test; at
 * integration the Frontend's input.c routes keys and only the draw
 * part of this function stays. */

static int selectedResult = 0;      /* j/k cursor into the result list */
static char lastQuery[256] = "";    /* resets the cursor on query change */

void drawSearch(AppState *state, SearchResult *results, int resultCount) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground((Color){18, 15, 12, 255});

    /* Search bar */
    DrawRectangle(40, 20, sw - 80, 48, (Color){28, 24, 20, 255});
    DrawRectangleLines(40, 20, sw - 80, 48, (Color){180, 140, 60, 255});
    DrawText("Search:", 56, 34, 16, (Color){120, 110, 90, 255});

    char displayQuery[280];
    snprintf(displayQuery, sizeof(displayQuery), "%s|", state->searchQuery);
    DrawText(displayQuery, 140, 33, 18, (Color){220, 210, 185, 255});

    /* Handle typing */
    int key = GetCharPressed();
    while (key > 0) {
        searchAppendChar(state->searchQuery, sizeof(state->searchQuery), key);
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE))
        searchBackspace(state->searchQuery);

    /* Reset the j/k cursor whenever the query changes */
    if (strcmp(state->searchQuery, lastQuery) != 0) {
        selectedResult = 0;
        strncpy(lastQuery, state->searchQuery, sizeof(lastQuery) - 1);
        lastQuery[sizeof(lastQuery) - 1] = '\0';
    }
    if (resultCount > 0 && selectedResult >= resultCount)
        selectedResult = resultCount - 1;

    /* Results */
    if (strlen(state->searchQuery) < 2) {
        DrawText("Type at least 2 characters to search...",
                 sw/2 - 180, sh/2, 16, (Color){120, 110, 90, 255});
        return;
    }

    if (resultCount == 0) {
        DrawText("No results found.", sw/2 - 80, sh/2, 16, (Color){120, 110, 90, 255});
        return;
    }

    char countStr[64];
    snprintf(countStr, sizeof(countStr), "Top %d results", resultCount);
    DrawText(countStr, 44, 78, 13, (Color){120, 110, 90, 255});

    for (int i = 0; i < resultCount; i++) {
        int y = 100 + (i * 58);
        if (y > sh) break;      /* don't draw past the window bottom */
        int isActive = (i == selectedResult);

        if (isActive)
            DrawRectangle(40, y - 4, sw - 80, 52, (Color){28, 24, 20, 255});
        DrawRectangleLines(40, y - 4, sw - 80, 52, (Color){50, 44, 36, 255});

        char ref[32];
        snprintf(ref, sizeof(ref), "%d:%d",
                 results[i].surahNumber, results[i].ayahNumber);
        DrawText(ref, 56, y + 4, 14, (Color){180, 140, 60, 255});

        DrawText(results[i].preview, 110, y + 4, 14,
                 isActive ? (Color){220, 210, 185, 255}
                          : (Color){120, 110, 90, 255});
    }
}
