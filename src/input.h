#ifndef INPUT_H
#define INPUT_H

#include "quran.h"

void handleInput(AppState *state);
int  isAnyKeyPressed(void);
int  getSettingsCursor(void);
int  isEditingLat(void);
const char *getLatEditBuf(void);
int  isEditingBookmark(void);   /* bookmark tag editor open */
const char *getBookmarkTag(void);
const char *getListJumpBuf(void); /* surah-list digit-jump buffer, "" when idle */
void openFinderMatch(AppState *state); /* open current Finder selection (click + Enter, both tabs) */
int  navRepeat(int key); /* edge + held-repeat for nav keys (0.45s delay, 25/s) */
#define SETTINGS_ROW_COUNT 10

#endif
