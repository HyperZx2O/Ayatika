#ifndef UI_H
#define UI_H

/* ============================================================
 * ui.h — Layout, panels, and screen rendering
 * Owned by: Frontend Engineer
 *
 * See member2.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include "quran.h"

/* Layout constants */
#define TOPBAR_H    56
#define SIDEBAR_W   280
#define FOOTER_H    40

/* Fonts — exposed for other modules */
extern Font arabicFont;
extern Font uiFont;

/* Init / close */
void initFonts(void);
void closeFonts(void);

/* Main draw dispatch */
void drawCurrentScreen(AppState *state);

/* Individual screens */
void drawDashboard(AppState *state);
void drawSurahList(AppState *state);
void drawAyahReader(AppState *state);
void drawBookmarks(AppState *state);
void drawSurahOverview(AppState *state);

/* Sub-components */
void drawTopBar(AppState *state);
void drawSidebar(AppState *state);
void drawFooter(AppState *state);
void drawWaqtPanel(AppState *state);
void drawHelpOverlay(AppState *state);
void drawBookmarkPopup(AppState *state);
void drawFocusDim(AppState *state);

/* Arabic text rendering (uses FriBidi internally) */
void drawArabicText(const char *text, Vector2 pos, float size, Color color);
void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color);

#endif /* UI_H */
