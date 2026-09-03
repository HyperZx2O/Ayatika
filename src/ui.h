#ifndef UI_H
#define UI_H

#include <raylib.h>
#include "quran.h"
#include "mock_data.h"

/* ── Scaling system ── */
typedef struct {
    float factor;       /* min(sw/1280, sh/720), clamped [0.4, 3.0] */
    int sw, sh;

    /* Layout */
    int topbarH;        /* ref 56  */
    int sidebarW;       /* ref 280 */
    int footerH;        /* ref 40  */

    /* Margins & gutters */
    int mx, my;         /* ref 40, 40 */
    int gx, gy;         /* ref 20, 20 */

    /* Font sizes */
    int fs12, fs13, fs14, fs16, fs18, fs20, fs22, fs26, fs34, fs40, fs42, fs48;

    /* Popups */
    int popupW, popupH;     /* surah overview: ref 600x320 */
    int helpW, helpH;       /* help overlay: ref 520x420 */

    /* Row heights */
    int sidebarRowH;        /* ref 48 */
    int bookmarkRowH;       /* ref 58 */

    /* Misc pixel values */
    int cardPadX, cardPadY; /* ref 12, 20 */
    int badgeW, badgeH;     /* ref 100x24 */
    int badgeGap;           /* ref 120 */
    int progressH;          /* ref 8 */
    int progressGap;        /* ref 20 */
    int dotRadius;          /* ref 4 */
    int topbarPrayerXOffset;/* ref 220 */
} Scale;

extern Scale S;
Scale computeScale(int sw, int sh);

/* Transitional macros — map old defines to scaled values */
#define TOPBAR_H    S.topbarH
#define SIDEBAR_W   S.sidebarW
#define FOOTER_H    S.footerH

extern Font arabicFont;
extern Font uiFont;

void initFonts(AppState *state);
void closeFonts(void);
void initFocusTexture(void);
void closeFocusTexture(void);

void drawCurrentScreen(AppState *state);

void drawDashboard(AppState *state);
void drawSurahList(AppState *state);
void drawAyahReader(AppState *state);
void drawBookmarks(AppState *state);
void drawSurahOverview(AppState *state);
void drawSettings(AppState *state);
void drawReadingHub(AppState *state);
void drawHadithPage(AppState *state);

void drawTopBar(AppState *state);
void drawSidebar(AppState *state);
void drawFooter(AppState *state);
void drawHelpOverlay(AppState *state);
void drawBookmarkPopup(AppState *state);
void showBookmarkPopup(void);

void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color);
void drawArabicVisualCentered(const char *visualText, Rectangle bounds, float size, Color color);

#endif
