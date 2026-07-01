#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mock_data.h"
#include "theme.h"

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    tests_run++; \
    printf("  TEST %s ... ", name); \
    { \
        int test_ok = 1;

#define ENDTEST \
        if (test_ok) { tests_passed++; printf("PASS\n"); } \
        else { tests_failed++; printf("FAIL\n"); } \
    }

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("\n    ASSERTION FAILED: %s (line %d)\n", #cond, __LINE__); \
        test_ok = 0; \
    } \
} while(0)

/* ── Mock Data Tests ── */

static void test_mock_data_loads_correctly(void) {
    AppState state;
    memset(&state, 0, sizeof(state));
    loadMockData(&state);

    TEST("loads 5 surahs") {
        ASSERT(state.surahs != NULL);
        ASSERT(state.surahs[0].number == 1);
        ASSERT(state.surahs[4].number == 18);
    } ENDTEST;

    TEST("surah names are correct") {
        ASSERT(strcmp(state.surahs[0].name, "Al-Fatiha") == 0);
        ASSERT(strcmp(state.surahs[1].name, "Al-Ikhlas") == 0);
        ASSERT(strcmp(state.surahs[2].name, "Al-Falaq") == 0);
        ASSERT(strcmp(state.surahs[3].name, "An-Nas") == 0);
        ASSERT(strcmp(state.surahs[4].name, "Al-Kahf") == 0);
    } ENDTEST;

    TEST("surah arabic names present") {
        ASSERT(strlen(state.surahs[0].arabicName) > 0);
        ASSERT(strlen(state.surahs[4].arabicName) > 0);
    } ENDTEST;

    TEST("surah revelation types populated") {
        ASSERT(strcmp(state.surahs[0].revelationType, "Meccan") == 0);
    } ENDTEST;

    TEST("surah ayah counts correct") {
        ASSERT(state.surahs[0].ayahCount == 7);
        ASSERT(state.surahs[1].ayahCount == 4);
        ASSERT(state.surahs[3].ayahCount == 6);
    } ENDTEST;

    TEST("total ayahs > 0") {
        ASSERT(state.totalAyahs > 0);
        ASSERT(state.totalAyahs == 27);
    } ENDTEST;

    TEST("ayah data integrity") {
        ASSERT(state.ayahs[0].surahNumber == 1);
        ASSERT(state.ayahs[0].ayahNumber == 1);
        ASSERT(strlen(state.ayahs[0].arabicText) > 0);
        ASSERT(strlen(state.ayahs[0].translationEn) > 0);
    } ENDTEST;

    TEST("fatiha has 7 ayahs") {
        int count = 0;
        for (int i = 0; i < state.totalAyahs; i++)
            if (state.ayahs[i].surahNumber == 1) count++;
        ASSERT(count == 7);
    } ENDTEST;

    TEST("ikhlas has 4 ayahs") {
        int count = 0;
        for (int i = 0; i < state.totalAyahs; i++)
            if (state.ayahs[i].surahNumber == 112) count++;
        ASSERT(count == 4);
    } ENDTEST;

    TEST("findMockAyah returns correct ayah") {
        Ayah *a = findMockAyah(&state, 1, 1);
        ASSERT(a != NULL);
        ASSERT(a->surahNumber == 1);
        ASSERT(a->ayahNumber == 1);

        a = findMockAyah(&state, 112, 3);
        ASSERT(a != NULL);
        ASSERT(a->surahNumber == 112);
        ASSERT(a->ayahNumber == 3);
        ASSERT(strstr(a->translationEn, "begets") != NULL);
    } ENDTEST;

    TEST("findMockAyah returns NULL for nonexistent") {
        Ayah *a = findMockAyah(&state, 999, 1);
        ASSERT(a == NULL);
        a = findMockAyah(&state, 1, 99);
        ASSERT(a == NULL);
    } ENDTEST;

    TEST("bookmarks loaded") {
        ASSERT(state.hadiths != NULL);
        ASSERT(state.totalHadiths == 1);
        ASSERT(strlen(state.hadiths[0].text) > 0);
    } ENDTEST;

    TEST("prayer times populated") {
        ASSERT(strlen(state.prayer.fajrStr) > 0);
        ASSERT(strlen(state.prayer.maghribStr) > 0);
        ASSERT(state.prayer.fajr > 0);
    } ENDTEST;

    TEST("initial state values correct") {
        ASSERT(state.currentSurah == 1);
        ASSERT(state.currentAyah == 1);
        ASSERT(state.cursorSurah == 0);
        ASSERT(strcmp(state.language, "en") == 0);
        ASSERT(strlen(state.statusMsg) > 0);
    } ENDTEST;
}

/* ── Theme Tests ── */

static void test_themes(void) {
    initThemes();

    TEST("theme count is 3") {
        ASSERT(THEME_COUNT == 3);
    } ENDTEST;

    TEST("dark parchment background correct") {
        Theme *t = getTheme(0);
        ASSERT(t->background.r == 18);
        ASSERT(t->background.g == 15);
        ASSERT(t->background.b == 12);
        ASSERT(t->background.a == 255);
        ASSERT(strcmp(t->name, "Dark Parchment") == 0);
    } ENDTEST;

    TEST("dark parchment accent correct") {
        Theme *t = getTheme(0);
        ASSERT(t->accent.r == 180);
        ASSERT(t->accent.g == 140);
        ASSERT(t->accent.b == 60);
    } ENDTEST;

    TEST("light manuscript colors correct") {
        Theme *t = getTheme(1);
        ASSERT(t->background.r == 245);
        ASSERT(t->background.g == 240);
        ASSERT(t->background.b == 228);
        ASSERT(t->foreground.r == 30);
        ASSERT(t->foreground.g == 25);
        ASSERT(t->foreground.b == 18);
    } ENDTEST;

    TEST("emerald night colors correct") {
        Theme *t = getTheme(2);
        ASSERT(t->background.r == 8);
        ASSERT(t->background.g == 18);
        ASSERT(t->background.b == 14);
        ASSERT(t->accent.r == 50);
        ASSERT(t->accent.g == 180);
        ASSERT(t->accent.b == 110);
    } ENDTEST;

    TEST("getTheme wraps with modulo") {
        Theme *t = getTheme(3);
        ASSERT(strcmp(t->name, "Dark Parchment") == 0);
        t = getTheme(100);
        ASSERT(strcmp(t->name, "Light Manuscript") == 0);
    } ENDTEST;

    TEST("cycleTheme wraps around") {
        AppState s;
        memset(&s, 0, sizeof(s));
        s.currentTheme = 0;
        cycleTheme(&s);
        ASSERT(s.currentTheme == 1);
        cycleTheme(&s);
        ASSERT(s.currentTheme == 2);
        cycleTheme(&s);
        ASSERT(s.currentTheme == 0);
    } ENDTEST;
}

/* ── AppScreen Enum Tests ── */

static void test_screens(void) {
    TEST("app screen enum values") {
        ASSERT(SCREEN_DASHBOARD == 0);
        ASSERT(SCREEN_SURAH_LIST == 1);
        ASSERT(SCREEN_AYAH_READER == 2);
        ASSERT(SCREEN_SEARCH == 3);
        ASSERT(SCREEN_BOOKMARKS == 4);
        ASSERT(SCREEN_SCREENSAVER == 5);
        ASSERT(SCREEN_SURAH_OVERVIEW == 6);
    } ENDTEST;

    TEST("all screens in range 0-6") {
        AppScreen screens[] = {
            SCREEN_DASHBOARD, SCREEN_SURAH_LIST, SCREEN_AYAH_READER,
            SCREEN_SEARCH, SCREEN_BOOKMARKS, SCREEN_SCREENSAVER,
            SCREEN_SURAH_OVERVIEW
        };
        for (int i = 0; i < 7; i++) {
            ASSERT(screens[i] >= 0);
            ASSERT(screens[i] <= 6);
        }
    } ENDTEST;
}

/* ── Run all ── */

int main(void) {
    printf("=== Phase 2 Tests ===\n\n");

    printf("[Mock Data]\n");
    test_mock_data_loads_correctly();

    printf("\n[Theme System]\n");
    test_themes();

    printf("\n[App Screen Enum]\n");
    test_screens();

    printf("\n=== Results: %d tests, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
