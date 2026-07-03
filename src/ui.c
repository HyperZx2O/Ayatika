#include <raylib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fribidi/fribidi.h>
#include "ui.h"
#include "theme.h"

Font arabicFont;
Font uiFont;

void initFonts(void) {
    arabicFont = LoadFontEx("assets/Amiri.ttf", 48, NULL, 0x10FFFF);
    if (arabicFont.texture.id > 0)
        SetTextureFilter(arabicFont.texture, TEXTURE_FILTER_BILINEAR);
    else
        arabicFont = GetFontDefault(); /* fallback if file missing */
    uiFont = GetFontDefault();
}

void closeFonts(void) {
    if (arabicFont.texture.id > 0 && arabicFont.texture.id != uiFont.texture.id)
        UnloadFont(arabicFont);
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

static int reorderArabic(const char *text, char *visualOut, int outSize) {
    (void)outSize;
    FriBidiChar logical[2048];
    FriBidiStrIndex len = fribidi_charset_to_unicode(
        FRIBIDI_CHAR_SET_UTF8, text, strlen(text), logical);
    if (len <= 0) { visualOut[0] = '\0'; return 0; }
    if (len >= 2048) len = 2047;

    FriBidiChar visual[2048];
    FriBidiParType baseDir = FRIBIDI_PAR_RTL;
    FriBidiLevel levels[2048];
    FriBidiStrIndex map[2048];
    FriBidiLevel maxLevel = fribidi_log2vis(
        logical, len, &baseDir, visual, map, NULL, levels);
    (void)maxLevel; (void)map;

    fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual, len, visualOut);
    return 1;
}

void drawArabicText(const char *text, Vector2 pos, float size, Color color) {
    char visual[4096];
    if (!reorderArabic(text, visual, sizeof(visual))) {
        DrawTextEx(uiFont, text, pos, size, 1, color);
        return;
    }
    Font f = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    DrawTextEx(f, visual, pos, size, 1, color);
}

void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color) {
    char visual[4096];
    if (!reorderArabic(text, visual, sizeof(visual))) {
        float tw = MeasureTextEx(uiFont, text, size, 1).x;
        Vector2 pos = {bounds.x + (bounds.width - tw) / 2, bounds.y};
        DrawTextEx(uiFont, text, pos, size, 1, color);
        return;
    }
    Font f = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    float tw = MeasureTextEx(f, visual, size, 1).x;
    Vector2 pos = {bounds.x + (bounds.width - tw) / 2, bounds.y};
    DrawTextEx(f, visual, pos, size, 1, color);
}
