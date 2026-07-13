#ifndef INPUT_H
#define INPUT_H

#include "quran.h"
#include "mock_data.h"

void handleInput(AppState *state);
int  isAnyKeyPressed(void);
int  getSettingsCursor(void);
int  isEditingLat(void);
const char *getLatEditBuf(void);
#define SETTINGS_ROW_COUNT 9

#endif
