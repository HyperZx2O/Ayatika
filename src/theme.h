#ifndef THEME_H
#define THEME_H

/* ============================================================
 * theme.h — Color palette system
 * Owned by: Frontend Engineer
 *
 * See member2.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include "quran.h"

#define THEME_COUNT 3

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

#endif /* THEME_H */
