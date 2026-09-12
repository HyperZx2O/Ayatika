/* live-backend smoke test; skips gracefully when offline with no cache. */
#include <stdio.h>
#include <string.h>
#include "quran.h"

static int failures = 0;
static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

int main(void) {
    /* back up user config; the round-trip check rewrites it. */
    char cfgBak[1024] = {0};
    long cfgLen = 0;
    FILE *cf = fopen("data/config.ini", "rb");
    if (cf) {
        cfgLen = fread(cfgBak, 1, sizeof(cfgBak) - 1, cf);
        fclose(cf);
    }
    AppState state;
    memset(&state, 0, sizeof(AppState));

    /* Config defaults */
    loadConfig(&state);
    check("config defaults Dhaka", state.latitude > 23.8f && state.latitude < 23.9f);
    check("config defaults lang en", strcmp(state.language, "en") == 0);

    /* Quran data: live API or disk cache; offline fresh = graceful 0 */
    int ok = loadQuranData(&state);
    if (!ok) {
        printf("[SKIP] offline with no cache — remaining checks skipped\n");
        check("offline returns 0 without crash", 1);
        return 0;
    }
    check("surahCount == 114", state.surahCount == 114);
    check("totalAyahs == 6236", state.totalAyahs == 6236);
    Ayah *a = getAyah(&state, 1, 1);
    check("getAyah(1,1) non-empty", a && a->arabicText[0]);
    check("getAyah(999,1) NULL", getAyah(&state, 999, 1) == NULL);
    /* 2:282 is 2283B — proves the 4096 fields + safe copy hold it whole. */
    Ayah *long1 = getAyah(&state, 2, 282);
    check("2:282 loads past old 2048 cap",
          long1 && strlen(long1->arabicText) > 2047 && strlen(long1->arabicText) < 4096);
    if (long1) {
        /* no split UTF-8 sequence: every lead byte's run is complete */
        const unsigned char *p = (const unsigned char *)long1->arabicText;
        int ok = 1;
        while (*p && ok) {
            size_t e = *p < 0x80 ? 1 : *p < 0xE0 ? 2 : *p < 0xF0 ? 3 : 4;
            for (size_t i = 1; i < e; i++) if ((p[i] & 0xC0) != 0x80) { ok = 0; break; }
            p += e;
        }
        check("2:282 arabic is intact UTF-8", ok);
    }
    check("daily index in range", getDailyAyahIndex(state.totalAyahs) < state.totalAyahs);

    Surah m;
    getSurahMeta(1, &m);
    check("meta 1 is Fatiha/7", m.ayahCount == 7);
    check("context enriched", state.surahs[0].context[0] != '\0');

    /* Hadiths from bundled JSON */
    check("hadiths loaded", loadHadiths(&state) && state.totalHadiths >= 12);

    /* Prayer sanity for Dhaka */
    state.latitude = 23.8103f; state.longitude = 90.4125f; state.calcMethod = 0;
    state.lastPrayerUpdate = 0;
    updatePrayerTimes(&state);
    check("fajr sane", state.prayer.fajr > 3.0f && state.prayer.fajr < 6.0f);
    check("isha sane", state.prayer.isha > 17.5f && state.prayer.isha < 20.5f);
    check("countdown non-negative", formatCountdown(getNextPrayerTime(&state.prayer))[0] != '-');
    /* waqt-alert helpers — math only, no audio device needed. */
    check("now sane", prayerNowHours() >= 0.0f && prayerNowHours() < 24.0f);
    {
        const char *names[5] = { "Fajr", "Dhuhr", "Asr", "Maghrib", "Isha" };
        int idx = nextPrayerIndex(&state.prayer);
        check("next idx in range", idx >= 0 && idx < 5);
        check("next idx matches name", strcmp(getNextPrayerName(&state.prayer), names[idx]) == 0);
    }

    /* DB round-trip with sentinel tag */
    check("initDatabase", initDatabase());
    Bookmark bm;
    memset(&bm, 0, sizeof(bm));
    bm.surahNumber = 112; bm.ayahNumber = 1;
    strncpy(bm.tag, "__backend_test__", sizeof(bm.tag) - 1);
    check("saveBookmark", saveBookmark(&bm));
    check("bookmarkExists", bookmarkExists(112, 1));
    Bookmark rows[256];
    int n = loadBookmarks(rows, 256);
    int id = -1;
    for (int i = 0; i < n; i++)
        if (rows[i].surahNumber == 112 && rows[i].ayahNumber == 1 &&
            strcmp(rows[i].tag, "__backend_test__") == 0) id = rows[i].id;
    check("saved row reloads", id >= 0);
    if (id >= 0) {
        check("deleteBookmark", deleteBookmark(id));
        Bookmark rows2[256];
        int n2 = loadBookmarks(rows2, 256);
        int stillThere = 0;
        for (int i = 0; i < n2; i++)
            if (rows2[i].id == id) stillThere = 1;
        check("sentinel gone after delete", !stillThere);
    }
    /* same-second saves tie on timestamp — draw and delete loads
       must agree, and positional delete must hit the intended row. */
    Bookmark t1, t2;
    memset(&t1, 0, sizeof(t1)); memset(&t2, 0, sizeof(t2));
    t1.surahNumber = 113; t1.ayahNumber = 1;
    t2.surahNumber = 114; t2.ayahNumber = 1;
    strncpy(t1.tag, "__tie_a__", sizeof(t1.tag) - 1);
    strncpy(t2.tag, "__tie_b__", sizeof(t2.tag) - 1);
    check("tie save A", saveBookmark(&t1));
    check("tie save B", saveBookmark(&t2));
    Bookmark r1[256], r2[256];
    int n1 = loadBookmarks(r1, 256), n2 = loadBookmarks(r2, 256);
    int sameOrder = (n1 == n2);
    for (int i = 0; sameOrder && i < n1; i++)
        if (r1[i].id != r2[i].id) sameOrder = 0;
    check("consecutive loads agree", sameOrder);
    int tieId = -1;
    for (int i = 0; i < n1; i++)
        if (strcmp(r1[i].tag, "__tie_a__") == 0) { tieId = r1[i].id; break; }
    check("tie row found", tieId >= 0);
    if (tieId >= 0) {
        check("tie delete", deleteBookmark(tieId));
        Bookmark r3[256];
        int n3 = loadBookmarks(r3, 256);
        int aGone = 1, bKept = 0;
        for (int i = 0; i < n3; i++) {
            if (strcmp(r3[i].tag, "__tie_a__") == 0) aGone = 0;
            if (strcmp(r3[i].tag, "__tie_b__") == 0) bKept = 1;
        }
        check("intended row gone, other kept", aGone && bKept);
        for (int i = 0; i < n3; i++)
            if (strcmp(r3[i].tag, "__tie_b__") == 0) deleteBookmark(r3[i].id);
    }
    closeDatabase();

    /* Config round-trip */
    state.latitude = 23.5f; state.currentSurah = 36;
    saveConfig(&state);
    AppState s2;
    memset(&s2, 0, sizeof(AppState));
    loadConfig(&s2);
    check("config round-trip", s2.latitude > 23.4f && s2.latitude < 23.6f && s2.currentSurah == 36);

    /* restore user config whatever happened. */
    if (cfgLen > 0) {
        FILE *w = fopen("data/config.ini", "wb");
        if (w) { fwrite(cfgBak, 1, (size_t)cfgLen, w); fclose(w); }
    } else {
        remove("data/config.ini");
    }

    if (failures) { printf("%d check(s) FAILED\n", failures); return 1; }
    printf("All backend checks passed\n");
    return 0;
}

