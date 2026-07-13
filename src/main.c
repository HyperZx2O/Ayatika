#include <raylib.h>
#include <stdlib.h>
#include <string.h>

/* Minimal Windows API declarations for dynamic loading of DwmSetWindowAttribute.
 * We avoid #include <windows.h> because it conflicts with raylib types. */
typedef void *HMODULE;
typedef unsigned long DWORD;
__declspec(dllimport) HMODULE __stdcall LoadLibraryA(const char *);
__declspec(dllimport) void*   __stdcall GetProcAddress(HMODULE, const char *);
__declspec(dllimport) int     __stdcall FreeLibrary(HMODULE);

#include "mock_data.h"
#include "ui.h"
#include "theme.h"
#include "input.h"

/* ── Apply dark/light title bar to match current theme ──
 * Uses LoadLibrary/GetProcAddress to avoid linking against dwmapi.lib.
 * dwmapi.dll is always present on modern Windows, so this is safe. */
void applyTitleBarTheme(Theme *theme) {
    float lum = 0.299f * theme->background.r +
                0.587f * theme->background.g +
                0.114f * theme->background.b;
    int dark = (lum < 128.0f) ? 1 : 0;
    HMODULE hDwm = LoadLibraryA("dwmapi.dll");
    if (hDwm) {
        /* DwmSetWindowAttribute(HWND, DWORD, LPCVOID, DWORD) */
        typedef int (__stdcall *DwmFn)(void*, DWORD, const void*, DWORD);
        DwmFn fn = (DwmFn)(void*)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (fn) fn(GetWindowHandle(), 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
        FreeLibrary(hDwm);
    }
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Ayatika — القرآن الكريم");
    applyTitleBarTheme(getTheme(0));
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
    initFonts(&state);
    initFocusTexture();

    while (!WindowShouldClose()) {
        S = computeScale(GetScreenWidth(), GetScreenHeight());
        if (state.fontScale > 0.01f) S.factor *= state.fontScale;

        if (isAnyKeyPressed() || GetMouseDelta().x != 0 || GetMouseDelta().y != 0)
            state.lastInputTime = GetTime();

        double idleSeconds = GetTime() - state.lastInputTime;
        state.catVisible = (idleSeconds > (double)state.idleSeconds);

        handleInput(&state);

        BeginDrawing();
            ClearBackground(getTheme(state.currentTheme)->background);
            drawCurrentScreen(&state);
            if (state.showHelp) drawHelpOverlay(&state);
        EndDrawing();
    }

    closeFocusTexture();
    closeFonts();
    CloseWindow();
    return 0;
}
