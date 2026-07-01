#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include "mock_data.h"
#include "ui.h"
#include "theme.h"
#include "input.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Ayatika — القرآن الكريم");
    SetTargetFPS(60);
    SetExitKey(0);

    AppState state;
    memset(&state, 0, sizeof(AppState));
    state.currentScreen = SCREEN_DASHBOARD;
    state.currentTheme  = 0;
    state.lastInputTime = GetTime();
    strncpy(state.language, "en", 7);

    loadMockData(&state);
    initThemes();
    initFonts();

    while (!WindowShouldClose()) {
        if (isAnyKeyPressed() || GetMouseDelta().x != 0 || GetMouseDelta().y != 0)
            state.lastInputTime = GetTime();

        double idleSeconds = GetTime() - state.lastInputTime;
        state.catVisible = (idleSeconds > 120.0);

        /* Phase 2: placeholder for backend calls */
        /* TODO: updatePrayerTimes(&state); */
        /* Phase 2: placeholder for systems calls */
        /* TODO: updateAudio(&state); */

        handleInput(&state);

        BeginDrawing();
            ClearBackground(getTheme(state.currentTheme)->background);
            drawCurrentScreen(&state);
            if (state.showHelp) drawHelpOverlay(&state);
        EndDrawing();
    }

    closeFonts();
    CloseWindow();
    return 0;
}
