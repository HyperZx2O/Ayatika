#include "theme.h"

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
        {155, 140, 130, 255}, {180, 100, 100, 255}, {215, 205, 195, 255}
    };
    themes[2] = (Theme){
        "Peacock Court",
        {10, 25, 28, 255}, {16, 35, 38, 255}, {210, 235, 225, 255},
        {100, 150, 145, 255}, {210, 120, 100, 255}, {30, 65, 68, 255}
    };
    themes[3] = (Theme){
        "Amber Sanctum",
        {55, 45, 35, 255}, {38, 32, 25, 255}, {225, 215, 195, 255},
        {140, 125, 105, 255}, {80, 175, 120, 255}, {75, 65, 50, 255}
    };
}

Theme *getTheme(int index) {
    return &themes[index % THEME_COUNT];
}

void cycleTheme(AppState *state) {
    state->currentTheme = (state->currentTheme + 1) % THEME_COUNT;
}
