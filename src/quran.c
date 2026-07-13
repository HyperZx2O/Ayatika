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

int getDailyAyahIndex(int totalAyahs) {
    if (totalAyahs <= 0) return 0;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int doy = tm->tm_yday;
    return doy % totalAyahs;
}
