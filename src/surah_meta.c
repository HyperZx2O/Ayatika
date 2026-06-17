/* ============================================================
 * surah_meta.c — Static Surah metadata
 * Owned by: Backend Engineer
 *
 * Responsibilities:
 *   - Hardcoded array of all 114 Surahs: name, meaning,
 *     revelation type (Meccan/Medinan), ayah count, and a
 *     short context blurb for the Surah overview card.
 *   - This data never changes, so no network call is needed.
 *
 * See member1.md for the full implementation plan and the
 * first 5 entries as a template — complete all 114.
 * ============================================================ */

#include "quran.h"

static const Surah SURAH_META[TOTAL_SURAHS] = {
    /* TODO: fill in all 114 entries.
       Template for entry 1:
       {1, "Al-Fatiha", "الفاتحة", "The Opening", "Meccan", 7,
        "The opening prayer of the Quran, recited in every unit of Salah."},
    */
    {0} /* placeholder so the array is not empty before completion */
};

void getSurahMeta(int surahNum, Surah *out) {
    if (surahNum < 1 || surahNum > TOTAL_SURAHS) return;
    /* TODO: *out = SURAH_META[surahNum - 1]; once array is filled */
    (void)out;
}
