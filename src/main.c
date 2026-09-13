#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quran.h"
#include "ui.h"
#include "theme.h"
#include "input.h"
#include "audio.h"
#include "screensaver.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Ayatika — القرآن الكريم");
    /* Window/taskbar icon (Explorer uses the embedded .ico instead). */
    Image appIcon = LoadImage("assets/icon.png");
    if (appIcon.data) {
        SetWindowIcon(appIcon);
        UnloadImage(appIcon);
    }
    /* content-driven minimum — below 960×600 the sidebar + content stop coexisting. */
    SetWindowMinSize(960, 600);
    MaximizeWindow(); /* fill the screen on launch, still resizable. */
    applyTitleBarTheme(getTheme(0));
    SetTargetFPS(60);
    SetExitKey(0);

    AppState state;
    memset(&state, 0, sizeof(AppState));
    state.currentScreen = SCREEN_DASHBOARD;
    state.currentTheme  = 0;
    /* sane runtime defaults; loadConfig overrides persisted ones. */
    state.vimMotions = 0;
    state.fontScale = 1.0f;
    state.idleSeconds = 300;
    state.autoResume = 1;
    state.lastInputTime = GetTime();

    loadConfig(&state);
    if (state.fontScale < 0.01f) state.fontScale = 1.0f;
    if (state.idleSeconds <= 0) state.idleSeconds = 300;
    initDatabase();
    if (!loadQuranData(&state)) {
        /* offline first-run still opens; screens guard NULL data. */
        setStatus(&state, 1, "Offline mode — connect to fetch full data");
    }
    loadHadiths(&state);
    if (!state.autoResume) { state.currentSurah = 1; state.currentAyah = 1; }
    if (state.currentSurah < 1) state.currentSurah = 1;
    if (state.currentAyah < 1) state.currentAyah = 1;
    initThemes();
    initFonts(&state);
    initFocusTexture();
    initAudio();
    initScreensaver();
    updatePrayerTimes(&state);

    while (!WindowShouldClose()) {
        S = computeScale(GetScreenWidth(), GetScreenHeight());
        if (state.fontScale > 0.01f) S.factor *= state.fontScale;

        if (isAnyKeyPressed() || GetMouseDelta().x != 0 || GetMouseDelta().y != 0)
            state.lastInputTime = GetTime();

        double idleSeconds = GetTime() - state.lastInputTime;
        /* single idle gate; 0 disables. */
        if (state.idleSeconds > 0 && idleSeconds > (double)state.idleSeconds &&
            state.currentScreen != SCREEN_SCREENSAVER) {
            state.previousScreen = state.currentScreen;
            state.currentScreen = SCREEN_SCREENSAVER;
        }

        updatePrayerTimes(&state);
        updateAudio(&state);
        handleInput(&state);

        BeginDrawing();
            ClearBackground(getTheme(state.currentTheme)->background);
            drawCurrentScreen(&state);
            if (state.showHelp) drawHelpOverlay(&state);
            drawBookmarkPopup(&state);
        EndDrawing();
    }

    saveConfig(&state);
    closeDatabase();
    freeHadiths(&state);
    free(state.surahs);
    free(state.ayahs);
    closeAudio();
    closeScreensaver();
    closeFocusTexture();
    closeFonts();
    CloseWindow();
    return 0;
}

