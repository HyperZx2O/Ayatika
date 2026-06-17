/* ============================================================
 * quran.c — Quran data fetching, parsing, and caching
 * Owned by: Backend Engineer
 *
 * Responsibilities:
 *   - Fetch Quran text and translations from AlQuran.cloud (libcurl)
 *   - Parse JSON responses into Surah/Ayah structs (cJSON)
 *   - Cache data to disk on first run, load from cache afterward
 *   - Provide lookup helpers: getAyah(), getDailyAyahIndex()
 *
 * See member1.md for the full implementation plan.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "quran.h"

/* TODO: libcurl fetch + write callback */
/* TODO: cJSON parse into state->surahs / state->ayahs */
/* TODO: cache to data/quran.json, data/translation_en.json, data/translation_bn.json */

int loadQuranData(AppState *state) {
    (void)state;
    /* TODO: implement */
    return 0;
}

Ayah *getAyah(AppState *state, int surahNum, int ayahNum) {
    for (int i = 0; i < state->totalAyahs; i++) {
        if (state->ayahs[i].surahNumber == surahNum &&
            state->ayahs[i].ayahNumber  == ayahNum)
            return &state->ayahs[i];
    }
    return NULL;
}

int getAyahIndex(AppState *state, int surahNum, int ayahNum) {
    for (int i = 0; i < state->totalAyahs; i++) {
        if (state->ayahs[i].surahNumber == surahNum &&
            state->ayahs[i].ayahNumber  == ayahNum)
            return i;
    }
    return -1;
}

int getDailyAyahIndex(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int doy = tm->tm_yday;
    return doy % 6236;
}
