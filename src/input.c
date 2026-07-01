#include <raylib.h>
#include "input.h"

void handleInput(AppState *state) {
    (void)state;
    /* Phase 7: keybinding dispatch table */
}

int isAnyKeyPressed(void) {
    for (int k = KEY_SPACE; k <= KEY_Z; k++)
        if (IsKeyPressed(k)) return 1;
    return 0;
}
