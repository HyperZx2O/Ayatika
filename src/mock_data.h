#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include <raylib.h>

#define MOCK_TOTAL_SURAHS 114
#define MOCK_MAX_SEARCH_RESULTS 15

typedef struct {
    int    number;
    char   name[64];
    char   arabicName[128];
    char   meaning[128];
    char   revelationType[16];
    int    ayahCount;
    char   context[512];
} MockSurah;

typedef struct {
    int    surahNumber;
    int    ayahNumber;
    char   arabicText[2048];
    char   translationEn[2048];
    char   translationBn[2048];
    char   audioUrl[256];
} MockAyah;

typedef struct {
    int    id;
    int    surahNumber;
    int    ayahNumber;
    char   tag[128];
    char   note[1024];
    long   timestamp;
} MockBookmark;

typedef struct {
    char   name[64];
    char   text[1024];
    char   narrator[128];
    char   collection[32];
} MockHadith;

typedef struct {
    float  fajr, sunrise, dhuhr, asr, maghrib, isha;
    char   fajrStr[16], dhuhrStr[16], asrStr[16], maghribStr[16], ishaStr[16];
    int    prohibitedActive;
    char   prohibitedLabel[64];
} MockPrayerTimes;

typedef struct {
    int    score;
    int    surahNumber;
    int    ayahNumber;
    char   preview[120];
} MockSearchResult;

typedef enum {
    MOCK_SCREEN_DASHBOARD = 0,
    MOCK_SCREEN_SURAH_LIST,
    MOCK_SCREEN_AYAH_READER,
    MOCK_SCREEN_SEARCH,
    MOCK_SCREEN_BOOKMARKS,
    MOCK_SCREEN_SCREENSAVER,
    MOCK_SCREEN_SURAH_OVERVIEW
} MockAppScreen;

typedef struct {
    int            currentSurah;
    int            currentAyah;
    int            cursorSurah;
    MockAppScreen  currentScreen;
    MockAppScreen  previousScreen;

    MockSurah     *surahs;
    MockAyah      *ayahs;
    int            totalAyahs;
    MockHadith    *hadiths;
    int            totalHadiths;

    MockPrayerTimes prayer;

    int            currentTheme;
    int            focusMode;
    int            showHelp;
    char           statusMsg[256];

    int            dashboardCursor;

    double         lastInputTime;
    int            catVisible;

    char           searchQuery[256];
    MockSearchResult searchResults[MOCK_MAX_SEARCH_RESULTS];
    int            searchResultCount;

    char           language[8];
} MockAppState;

void loadMockData(MockAppState *state);
MockAyah *findMockAyah(MockAppState *state, int surahNum, int ayahNum);

#endif
