/* ============================================================
 * input.c — Vim-style keybinding engine
 * Owned by: Frontend Engineer
 *
 * Responsibilities:
 *   - Key dispatch table: key + screen context -> action function
 *   - Navigation (j/k/g/G), open/back, bookmark, search, theme,
 *     focus mode toggles
 *   - Calls Backend's saveBookmark()/bookmarkExists() and
 *     Systems' playClickSfx()/playSurahSwitchSfx()
 *
 * See member2.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include <string.h>
#include "input.h"
#include "quran.h"

/* TODO: define KeyBinding struct + action functions + dispatch table */

void handleInput(AppState *state) {
    (void)state;
    /* TODO: loop bindings table, check IsKeyPressed, dispatch */
}

int isAnyKeyPressed(void) {
    /* TODO: implement idle-timer reset check */
    return 0;
}
