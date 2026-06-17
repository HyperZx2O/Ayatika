/* ============================================================
 * theme.c — Color palette system
 * Owned by: Frontend Engineer
 *
 * Responsibilities:
 *   - Define Theme struct instances: Dark Parchment,
 *     Light Manuscript, Emerald Night
 *   - Runtime theme switching via state->currentTheme index
 *
 * See member2.md for the full implementation plan.
 * ============================================================ */

#include "theme.h"

static Theme themes[THEME_COUNT];

void initThemes(void) {
    /* TODO: populate themes[0], themes[1], themes[2] with
       Dark Parchment, Light Manuscript, Emerald Night palettes */
    (void)themes;
}

Theme *getTheme(int index) {
    return &themes[index % THEME_COUNT];
}

void cycleTheme(AppState *state) {
    state->currentTheme = (state->currentTheme + 1) % THEME_COUNT;
}
