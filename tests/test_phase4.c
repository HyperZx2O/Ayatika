#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "theme.h"
#include "mock_data.h"
#include "ui.h"

#define W 1280
#define H 720

static int failures = 0;

static void check(int cond, const char *label) {
    if (!cond) {
        failures++;
        TraceLog(LOG_WARNING, "FAIL: %s", label);
    } else {
        TraceLog(LOG_INFO, "PASS: %s", label);
    }
}

static void test_theme_colors(void) {
    TraceLog(LOG_INFO, "--- Theme Color Tests ---");
    check(THEME_COUNT == 3, "THEME_COUNT is 3");
    Theme *t0 = getTheme(0);
    check(strcmp(t0->name, "Dark Parchment") == 0, "Theme 0 name = Dark Parchment");
    check(t0->background.r == 18 && t0->background.g == 15 && t0->background.b == 12,
          "Dark Parchment background {18,15,12}");
    check(t0->accent.r == 180 && t0->accent.g == 140 && t0->accent.b == 60,
          "Dark Parchment accent {180,140,60}");
    Theme *t1 = getTheme(1);
    check(strcmp(t1->name, "Light Manuscript") == 0, "Theme 1 name = Light Manuscript");
    check(t1->background.r == 245 && t1->background.g == 240 && t1->background.b == 228,
          "Light Manuscript background {245,240,228}");
    check(t1->foreground.r == 30 && t1->foreground.g == 25 && t1->foreground.b == 18,
          "Light Manuscript foreground {30,25,18}");
    Theme *t2 = getTheme(2);
    check(strcmp(t2->name, "Emerald Night") == 0, "Theme 2 name = Emerald Night");
    check(t2->background.r == 8 && t2->background.g == 18 && t2->background.b == 14,
          "Emerald Night background {8,18,14}");
    check(t2->accent.r == 50 && t2->accent.g == 180 && t2->accent.b == 110,
          "Emerald Night accent {50,180,110}");
}

static void test_theme_wrapping(void) {
    TraceLog(LOG_INFO, "--- Theme Wrapping Tests ---");
    check(strcmp(getTheme(3)->name, "Dark Parchment") == 0,
          "getTheme(3) wraps to Dark Parchment");
    check(strcmp(getTheme(100)->name, "Light Manuscript") == 0,
          "getTheme(100) wraps to Light Manuscript");
    AppState s;
    memset(&s, 0, sizeof(s));
    s.currentTheme = 0;
    cycleTheme(&s);
    check(s.currentTheme == 1, "cycleTheme 0->1");
    cycleTheme(&s);
    check(s.currentTheme == 2, "cycleTheme 1->2");
    cycleTheme(&s);
    check(s.currentTheme == 0, "cycleTheme 2->0 (wraps)");
}

static void test_theme_contrast(void) {
    TraceLog(LOG_INFO, "--- Theme Contrast Tests ---");
    for (int i = 0; i < THEME_COUNT; i++) {
        Theme *t = getTheme(i);
        int bgLuma = t->background.r * 299 + t->background.g * 587 + t->background.b * 114;
        int fgLuma = t->foreground.r * 299 + t->foreground.g * 587 + t->foreground.b * 114;
        int contrast = abs(bgLuma - fgLuma);
        char label[64];
        snprintf(label, sizeof(label), "Theme %d (%s) contrast sufficient (%d)", i, t->name, contrast);
        check(contrast > 40000, label);
    }
}

static void test_visual_themes(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(W, H, "Ayatika — Phase 4: Theme System");
    SetTargetFPS(30);
    SetExitKey(0);
    initThemes();
    initFonts();
    AppState state;
    memset(&state, 0, sizeof(state));
    state.currentScreen = SCREEN_DASHBOARD;
    state.currentTheme = 0;
    strncpy(state.language, "en", 7);
    loadMockData(&state);
    int currentDemoTheme = 0;
    double lastSwitch = GetTime();
    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_SPACE))
            currentDemoTheme = (currentDemoTheme + 1) % THEME_COUNT;
        if (IsKeyPressed(KEY_LEFT))
            currentDemoTheme = (currentDemoTheme - 1 + THEME_COUNT) % THEME_COUNT;
        if (GetTime() - lastSwitch > 4.0) {
            currentDemoTheme = (currentDemoTheme + 1) % THEME_COUNT;
            lastSwitch = GetTime();
        }
        Theme *t = getTheme(currentDemoTheme);
        state.currentTheme = currentDemoTheme;
        int panelW = sw / 3;
        int panelH = sh - 80;
        BeginDrawing();
        ClearBackground(t->background);
        for (int i = 0; i < THEME_COUNT; i++) {
            Theme *p = getTheme(i);
            int x = i * panelW;
            if (i == currentDemoTheme) {
                DrawRectangle(x, 0, panelW - 2, panelH, p->surface);
                DrawRectangleLinesEx((Rectangle){x, 0, panelW - 2, panelH}, 3, p->accent);
                DrawText("ACTIVE", x + panelW / 2 - 28, 10, 14, p->accent);
            } else {
                DrawRectangle(x, 0, panelW - 2, panelH, p->background);
                DrawRectangleLinesEx((Rectangle){x, 0, panelW - 2, panelH}, 1, p->border);
            }
            int cy = 40;
            DrawText(p->name, x + 12, cy, 18, p->accent); cy += 30;
            DrawRectangle(x + 12, cy, 40, 18, p->background); cy += 24;
            DrawText("bg", x + 12, cy, 12, p->muted); cy += 18;
            DrawRectangle(x + 12, cy, 40, 18, p->surface); cy += 24;
            DrawText("surface", x + 12, cy, 12, p->muted); cy += 18;
            DrawText("Foreground text", x + 12, cy, 16, p->foreground); cy += 22;
            DrawText("Muted label", x + 12, cy, 13, p->muted); cy += 20;
            DrawText("Accent highlight", x + 12, cy, 16, p->accent); cy += 22;
            DrawRectangleLinesEx((Rectangle){x + 12, cy, 80, 22}, 1, p->border);
            DrawText("Bordered", x + 16, cy + 4, 12, p->foreground);
        }
        DrawText("← / → or SPACE to switch | Auto-cycles 4s | ENTER/ESC to exit",
                 20, sh - 30, 14, getTheme(currentDemoTheme)->muted);
        char buf[64];
        snprintf(buf, sizeof(buf), "Active: %s", t->name);
        DrawText(buf, sw - 260, sh - 30, 16, t->accent);
        EndDrawing();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            break;
    }
    closeFonts();
    if (state.surahs) free(state.surahs);
    if (state.ayahs) free(state.ayahs);
    if (state.hadiths) free(state.hadiths);
    CloseWindow();
}

int main(void) {
    TraceLog(LOG_INFO, "=== Phase 4: Theme System Tests ===");
    initThemes();
    test_theme_colors();
    test_theme_wrapping();
    test_theme_contrast();
    TraceLog(LOG_INFO, "\n--- Visual Theme Gallery (opens window) ---");
    test_visual_themes();
    TraceLog(LOG_INFO, "\n=== Results: %s ===\n",
             failures > 0 ? "SOME CHECKS FAILED" : "ALL PASSED");
    return failures > 0 ? 1 : 0;
}
