#include "theme.h"

static Theme themes[THEME_COUNT];

void initThemes(void) {
    themes[0] = (Theme){
        "Dark Parchment",
        {18, 15, 12, 255}, {28, 24, 20, 255}, {220, 210, 185, 255},
        {120, 110, 90, 255}, {180, 140, 60, 255}, {50, 44, 36, 255}
    };
    themes[1] = (Theme){
        "Light Manuscript",
        {245, 240, 228, 255}, {255, 252, 244, 255}, {30, 25, 18, 255},
        {140, 130, 110, 255}, {100, 70, 30, 255}, {200, 190, 170, 255}
    };
    themes[2] = (Theme){
        "Emerald Night",
        {8, 18, 14, 255}, {14, 28, 22, 255}, {180, 225, 200, 255},
        {80, 130, 100, 255}, {50, 180, 110, 255}, {25, 60, 45, 255}
    };
}

Theme *getTheme(int index) {
    return &themes[index % THEME_COUNT];
}

void cycleTheme(AppState *state) {
    state->currentTheme = (state->currentTheme + 1) % THEME_COUNT;
}
