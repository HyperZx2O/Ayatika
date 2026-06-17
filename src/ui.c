/* ============================================================
 * ui.c — Layout, panels, and screen rendering
 * Owned by: Frontend Engineer
 *
 * Responsibilities:
 *   - Dashboard, Surah list, Ayah reader, bookmarks,
 *     Surah overview card layouts
 *   - Arabic RTL text rendering via FriBidi + Amiri font
 *   - Help overlay and focus dim mode
 *
 * See member2.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include <string.h>
#include "ui.h"

Font arabicFont;
Font uiFont;

void initFonts(void) {
    /* TODO: arabicFont = LoadFontEx("assets/Amiri.ttf", 48, NULL, 0x10FFFF); */
    uiFont = GetFontDefault();
}

void closeFonts(void) {
    /* TODO: UnloadFont(arabicFont); */
}

void drawCurrentScreen(AppState *state) {
    (void)state;
    /* TODO: switch (state->currentScreen) { ... } dispatch to draw functions
       or to Systems' drawScreensaver() / drawSearch() */
}

void drawDashboard(AppState *state)      { (void)state; /* TODO */ }
void drawSurahList(AppState *state)      { (void)state; /* TODO */ }
void drawAyahReader(AppState *state)     { (void)state; /* TODO */ }
void drawBookmarks(AppState *state)      { (void)state; /* TODO */ }
void drawSurahOverview(AppState *state)  { (void)state; /* TODO */ }

void drawTopBar(AppState *state)         { (void)state; /* TODO */ }
void drawSidebar(AppState *state)        { (void)state; /* TODO */ }
void drawFooter(AppState *state)         { (void)state; /* TODO */ }
void drawWaqtPanel(AppState *state)      { (void)state; /* TODO */ }
void drawHelpOverlay(AppState *state)    { (void)state; /* TODO */ }
void drawBookmarkPopup(AppState *state)  { (void)state; /* TODO */ }
void drawFocusDim(AppState *state)       { (void)state; /* TODO */ }

void drawArabicText(const char *text, Vector2 pos, float size, Color color) {
    (void)text; (void)pos; (void)size; (void)color;
    /* TODO: FriBidi reorder, then DrawTextEx(arabicFont, ...) */
}

void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color) {
    (void)text; (void)bounds; (void)size; (void)color;
    /* TODO: implement */
}
