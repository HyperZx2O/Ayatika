#ifndef THEME_H
#define THEME_H

#include <raylib.h>
#include "quran.h"

#define THEME_COUNT 4

typedef struct {
    char  name[32];
    Color background;
    Color surface;
    Color foreground;
    Color muted;
    Color accent;
    Color border;
} Theme;

void   initThemes(void);
Theme *getTheme(int index);
void   cycleTheme(AppState *state);
void   applyTitleBarTheme(Theme *theme);

#endif
