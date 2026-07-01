#ifndef UI_H
#define UI_H

#include <raylib.h>
#include "mock_data.h"

#define TOPBAR_H    56
#define SIDEBAR_W   280
#define FOOTER_H    40

extern Font arabicFont;
extern Font uiFont;

void initFonts(void);
void closeFonts(void);

void drawCurrentScreen(AppState *state);

void drawDashboard(AppState *state);
void drawSurahList(AppState *state);
void drawAyahReader(AppState *state);
void drawBookmarks(AppState *state);
void drawSurahOverview(AppState *state);

void drawTopBar(AppState *state);
void drawSidebar(AppState *state);
void drawFooter(AppState *state);
void drawWaqtPanel(AppState *state);
void drawHelpOverlay(AppState *state);
void drawBookmarkPopup(AppState *state);
void drawFocusDim(AppState *state);

void drawArabicText(const char *text, Vector2 pos, float size, Color color);
void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color);

#endif
