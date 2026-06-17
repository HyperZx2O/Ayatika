#ifndef INPUT_H
#define INPUT_H

/* ============================================================
 * input.h — Vim-style keybinding engine
 * Owned by: Frontend Engineer
 *
 * See member2.md for the full implementation plan.
 * ============================================================ */

#include "quran.h"

void handleInput(AppState *state);
int  isAnyKeyPressed(void);

#endif /* INPUT_H */
