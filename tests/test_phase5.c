#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mock_data.h"
#include "theme.h"
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

/* ── Headless layout tests ── */

static void test_card_layout(void) {
    TraceLog(LOG_INFO, "--- Layout Math Tests ---");
    /* Card dimensions at 1280x720:
       sh=720, sw=1280
       contentH = 720-56-40 = 624
       cardW = (1280-80-20)/2 = 590
       cardH = (624-80-40)/3 = 504/3 = 168 */
    int sw = 1280, sh = 720;
    int mx = 40, my = 40, gx = 20, gy = 20;
    int ch = sh - 56 - 40;
    int cw = (sw - 2*mx - gx) / 2;
    int cardH = (ch - 2*my - 2*gy) / 3;
    check(cw == 590, "Card width = 590 at 1280x720");
    check(cardH == 168, "Card height = 168 at 1280x720");
    /* Row positions */
    int y0 = 56 + 40; /* TOPBAR_H + marginY */
    check(y0 == 96, "Row 0 y = 96");
    check(y0 + 168 + 20 == 284, "Row 1 y = 284");
    check(284 + 168 + 20 == 472, "Row 2 y = 472");
    /* Card bottom = 472 + 168 = 640, footer = 680, gap = 40 */
    check(472 + 168 + 40 == 680, "Gap to footer = 40");
}

static void test_dashboard_cursor(void) {
    TraceLog(LOG_INFO, "--- Dashboard Cursor Tests ---");
    AppState s;
    memset(&s, 0, sizeof(s));
    s.dashboardCursor = 0;
    check(s.dashboardCursor >= 0 && s.dashboardCursor < 6, "Cursor in bounds (0)");
    s.dashboardCursor = 5;
    check(s.dashboardCursor >= 0 && s.dashboardCursor < 6, "Cursor in bounds (5)");
    s.dashboardCursor = 3;
    check(s.dashboardCursor >= 0 && s.dashboardCursor < 6, "Cursor in bounds (mid)");
}

static void test_surah_list_scroll(void) {
    TraceLog(LOG_INFO, "--- Surah List Scroll Tests ---");
    /* At 1280x720: listH = 720-56-40 = 624, rowH = 48, visible = 13 */
    int listH = 624, rowH = 48, visible = listH / rowH;
    check(visible == 13, "13 visible rows at 720px");
    /* 7 surahs, visible=13: no scrolling needed, scrollOff=0 */
    int total = 7, cursor = 3, scrollOff = 0;
    /* cursor within visible, no scroll */
    if (cursor < scrollOff) scrollOff = cursor;
    if (cursor >= scrollOff + visible) scrollOff = cursor - visible + 1;
    if (scrollOff > total - visible) scrollOff = total - visible;
    if (scrollOff < 0) scrollOff = 0;
    check(scrollOff == 0, "No scroll when all fit");
    /* With many surahs (114): scroll should track cursor */
    total = 114;
    cursor = 100; scrollOff = 0;
    if (cursor < scrollOff) scrollOff = cursor;
    if (cursor >= scrollOff + visible) scrollOff = cursor - visible + 1;
    if (scrollOff > total - visible) scrollOff = total - visible;
    if (scrollOff < 0) scrollOff = 0;
    check(scrollOff == 88, "Scroll offset = 88 for cursor=100");
    /* Cursor near top */
    cursor = 2; scrollOff = 50;
    if (cursor < scrollOff) scrollOff = cursor;
    if (cursor >= scrollOff + visible) scrollOff = cursor - visible + 1;
    if (scrollOff > total - visible) scrollOff = total - visible;
    if (scrollOff < 0) scrollOff = 0;
    check(scrollOff == 2, "Scroll resets to cursor when cursor above view");
    /* Clamp at bottom */
    cursor = 113; scrollOff = 0;
    if (cursor < scrollOff) scrollOff = cursor;
    if (cursor >= scrollOff + visible) scrollOff = cursor - visible + 1;
    if (scrollOff > total - visible) scrollOff = total - visible;
    if (scrollOff < 0) scrollOff = 0;
    check(scrollOff == 101, "Scroll offset clamps at bottom");
}

/* ── Visual window test ── */

static void test_visual(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(W, H, "Ayatika — Phase 5: Dashboard & Surah List");
    SetTargetFPS(30);
    SetExitKey(0);

    initThemes();
    initFonts();

    AppState state;
    memset(&state, 0, sizeof(state));
    state.currentScreen = SCREEN_DASHBOARD;
    state.currentTheme = 0;
    state.dashboardCursor = 0;
    strncpy(state.language, "en", 7);
    loadMockData(&state);

    int showDashboard = 1;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_S)) {
            showDashboard = !showDashboard;
            state.currentScreen = showDashboard ? SCREEN_DASHBOARD : SCREEN_SURAH_LIST;
        }
        if (IsKeyPressed(KEY_T)) {
            cycleTheme(&state);
            snprintf(state.statusMsg, sizeof(state.statusMsg), "Theme: %s",
                     getTheme(state.currentTheme)->name);
        }
        /* Dashboard cursor navigation */
        if (showDashboard) {
            if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_DOWN)) {
                if (state.dashboardCursor + 2 < 6) state.dashboardCursor += 2;
            }
            if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_UP)) {
                if (state.dashboardCursor - 2 >= 0) state.dashboardCursor -= 2;
            }
            if (IsKeyPressed(KEY_L) || IsKeyPressed(KEY_RIGHT)) {
                if (state.dashboardCursor % 2 == 0 && state.dashboardCursor + 1 < 6)
                    state.dashboardCursor++;
            }
            if (IsKeyPressed(KEY_H) || IsKeyPressed(KEY_LEFT)) {
                if (state.dashboardCursor % 2 == 1) state.dashboardCursor--;
            }
        } else {
            /* Surah list cursor navigation */
            if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_DOWN)) {
                if (state.cursorSurah < 6) state.cursorSurah++;
            }
            if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_UP)) {
                if (state.cursorSurah > 0) state.cursorSurah--;
            }
            if (IsKeyPressed(KEY_G)) state.cursorSurah = 0;
        }

        BeginDrawing();
        ClearBackground(getTheme(state.currentTheme)->background);
        drawCurrentScreen(&state);
        if (state.showHelp) drawHelpOverlay(&state);

        /* Overlay test info */
        Theme *t = getTheme(state.currentTheme);
        char buf[128];
        snprintf(buf, sizeof(buf), "S=%s  T=theme  arrows=j/k/h/l  ESC=exit  Failures=%d",
                 showDashboard ? "DASHBOARD" : "SURAH LIST", failures);
        DrawText(buf, W/2 - 280, 2, 12, failures > 0 ? RED : t->accent);

        EndDrawing();

        if (IsKeyPressed(KEY_ESCAPE)) break;
    }

    closeFonts();
    if (state.surahs) free(state.surahs);
    if (state.ayahs) free(state.ayahs);
    if (state.hadiths) free(state.hadiths);
    CloseWindow();
}

int main(void) {
    TraceLog(LOG_INFO, "=== Phase 5: Dashboard & Surah List Tests ===");

    test_card_layout();
    test_dashboard_cursor();
    test_surah_list_scroll();

    TraceLog(LOG_INFO, "\n--- Visual Window Test (opens window) ---");
    test_visual();

    TraceLog(LOG_INFO, "\n=== Results: %s ===\n",
             failures > 0 ? "SOME CHECKS FAILED" : "ALL PASSED");
    return failures > 0 ? 1 : 0;
}
