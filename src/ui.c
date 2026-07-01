#include <raylib.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "theme.h"

Font arabicFont;
Font uiFont;

void initFonts(void) {
    /* Phase 3: arabicFont = LoadFontEx("assets/Amiri.ttf", 48, NULL, 0x10FFFF); */
    /* Phase 3: SetTextureFilter(arabicFont.texture, TEXTURE_FILTER_BILINEAR); */
    uiFont = GetFontDefault();
}

void closeFonts(void) {
    /* Phase 3: UnloadFont(arabicFont); */
}

void drawCurrentScreen(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawDashboard(state);
            break;
        case SCREEN_SURAH_LIST:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawSurahList(state);
            break;
        case SCREEN_AYAH_READER:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawAyahReader(state);
            break;
        case SCREEN_SEARCH:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            DrawRectangle(sw/2-200, sh/2-20, 400, 40, t->surface);
            DrawRectangleLines(sw/2-200, sh/2-20, 400, 40, t->accent);
            DrawText("[Search screen — built by Systems & Features role]",
                     sw/2-190, sh/2-8, 14, t->muted);
            break;
        case SCREEN_BOOKMARKS:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawBookmarks(state);
            break;
        case SCREEN_SCREENSAVER:
            ClearBackground(BLACK);
            DrawText("[Screensaver — built by Systems & Features role]",
                     sw/2-170, sh/2-8, 14, t->muted);
            break;
        case SCREEN_SURAH_OVERVIEW:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawSurahOverview(state);
            break;
    }
}

void drawDashboard(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, TOPBAR_H, t->surface);
    DrawLine(0, TOPBAR_H, sw, TOPBAR_H, t->border);
    DrawText("Ayatika", 20, 16, 22, t->accent);

    char prayerInfo[64];
    snprintf(prayerInfo, sizeof(prayerInfo), "%s — %s",
             "Next Prayer", state->prayer.fajrStr);
    DrawText(prayerInfo, sw - 280, 18, 18, t->foreground);

    DrawRectangle(0, TOPBAR_H, SIDEBAR_W, sh - TOPBAR_H - FOOTER_H, t->surface);
    DrawLine(SIDEBAR_W, TOPBAR_H, SIDEBAR_W, sh - FOOTER_H, t->border);

    DrawRectangle(0, sh - FOOTER_H, sw, FOOTER_H, t->surface);
    DrawLine(0, sh - FOOTER_H, sw, sh - FOOTER_H, t->border);
    DrawText(state->statusMsg, 20, sh - FOOTER_H + 12, 14, t->muted);
    DrawText("? = help   j/k = navigate   Enter = open   b = bookmark",
             sw - 480, sh - FOOTER_H + 12, 13, t->muted);
}

void drawSurahList(AppState *state) {
    (void)state;
}

void drawAyahReader(AppState *state) {
    (void)state;
}

void drawBookmarks(AppState *state) {
    (void)state;
}

void drawSurahOverview(AppState *state) {
    (void)state;
}

void drawTopBar(AppState *state) {
    (void)state;
}

void drawSidebar(AppState *state) {
    (void)state;
}

void drawFooter(AppState *state) {
    (void)state;
}

void drawWaqtPanel(AppState *state) {
    (void)state;
}

void drawHelpOverlay(AppState *state) {
    (void)state;
}

void drawBookmarkPopup(AppState *state) {
    (void)state;
}

void drawFocusDim(AppState *state) {
    (void)state;
}

void drawArabicText(const char *text, Vector2 pos, float size, Color color) {
    /* Phase 3: FriBidi reorder + DrawTextEx */
    DrawTextEx(uiFont, text, pos, size, 1, color);
}

void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color) {
    /* Phase 3: FriBidi reorder + center + draw */
    float tw = MeasureTextEx(uiFont, text, size, 1).x;
    Vector2 pos = {bounds.x + (bounds.width - tw) / 2, bounds.y};
    DrawTextEx(uiFont, text, pos, size, 1, color);
}
