#include "theme.h"

/* lives here (not main.c) so tests link without main. */
#ifdef _WIN32
typedef void *HMODULE;
typedef unsigned long DWORD;
__declspec(dllimport) HMODULE __stdcall LoadLibraryA(const char *);
__declspec(dllimport) void*   __stdcall GetProcAddress(HMODULE, const char *);
__declspec(dllimport) int     __stdcall FreeLibrary(HMODULE);
#include <raylib.h>
#endif

void applyTitleBarTheme(Theme *theme) {
#ifdef _WIN32
    float lum = 0.299f * theme->background.r +
                0.587f * theme->background.g +
                0.114f * theme->background.b;
    int dark = (lum < 128.0f) ? 1 : 0;
    HMODULE hDwm = LoadLibraryA("dwmapi.dll");
    if (hDwm) {
        typedef int (__stdcall *DwmFn)(void*, DWORD, const void*, DWORD);
        DwmFn fn = (DwmFn)(void*)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (fn) fn(GetWindowHandle(), 20, &dark, sizeof(dark));
        FreeLibrary(hDwm);
    }
#else
    (void)theme;
#endif
}

static Theme themes[THEME_COUNT];

void initThemes(void) {
    themes[0] = (Theme){
        "Celestial Night",
        {12, 14, 28, 255}, {20, 22, 38, 255}, {235, 225, 205, 255},
        {130, 125, 145, 255}, {210, 175, 90, 255}, {40, 42, 60, 255}
    };
    themes[1] = (Theme){
        "Moonlit Garden",
        {248, 242, 235, 255}, {255, 252, 248, 255}, {45, 35, 30, 255},
        {110, 95, 85, 255}, {165, 85, 85, 255}, {215, 205, 195, 255}
    };
    themes[2] = (Theme){
        "Peacock Court",
        {10, 25, 28, 255}, {16, 35, 38, 255}, {210, 235, 225, 255},
        {100, 150, 145, 255}, {210, 120, 100, 255}, {30, 65, 68, 255}
    };
    themes[3] = (Theme){
        "Amber Sanctum",
        {55, 45, 35, 255}, {38, 32, 25, 255}, {225, 215, 195, 255},
        {170, 155, 135, 255}, {80, 175, 120, 255}, {75, 65, 50, 255}
    };
}

Theme *getTheme(int index) {
    return &themes[index % THEME_COUNT];
}

void cycleTheme(AppState *state) {
    state->currentTheme = (state->currentTheme + 1) % THEME_COUNT;
}

