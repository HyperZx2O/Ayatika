#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include <raylib.h>
#include "quran.h"

/* Mock type aliases — layout-compatible with quran.h types.
   Existing tests use Mock* names; these ensure they compile. */
typedef Surah         MockSurah;
typedef Ayah          MockAyah;
typedef Bookmark      MockBookmark;
typedef Hadith        MockHadith;
typedef PrayerTimes   MockPrayerTimes;
typedef SearchResult  MockSearchResult;
typedef AppState      MockAppState;
typedef AppScreen     MockAppScreen;

#define MOCK_SCREEN_DASHBOARD    SCREEN_DASHBOARD
#define MOCK_SCREEN_SURAH_LIST   SCREEN_SURAH_LIST
#define MOCK_SCREEN_AYAH_READER  SCREEN_AYAH_READER
#define MOCK_SCREEN_SEARCH       SCREEN_SEARCH
#define MOCK_SCREEN_BOOKMARKS    SCREEN_BOOKMARKS
#define MOCK_SCREEN_SCREENSAVER  SCREEN_SCREENSAVER
#define MOCK_SCREEN_SURAH_OVERVIEW SCREEN_SURAH_OVERVIEW

#define MOCK_TOTAL_SURAHS 114
#define MOCK_MAX_SEARCH_RESULTS 15

void loadMockData(AppState *state);
Ayah *findMockAyah(AppState *state, int surahNum, int ayahNum);

/* Mock bookmarks — shared between ui.c and input.c */
extern Bookmark mockBookmarks[];
extern int mockBookmarkCount;
int addMockBookmark(int surah, int ayah);

#endif
