#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include <raylib.h>
#include "quran.h"

void loadMockData(AppState *state);
Ayah *findMockAyah(AppState *state, int surahNum, int ayahNum);

/* Mock bookmarks — shared between ui.c and input.c */
extern Bookmark mockBookmarks[];
extern int mockBookmarkCount;
int addMockBookmark(int surah, int ayah);
long getMockBookmarkTimestamp(int surah, int ayah);

#endif
