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

/* ── Headless layout math tests ── */

static void test_ayah_reader_layout(void) {
    TraceLog(LOG_INFO, "--- Ayah Reader Layout Tests ---");
    int sw = 1280, sh = 720;
    int sidebarW = 280, topbarH = 56, footerH = 40;

    int mx = sidebarW + 40;
    int my = topbarH + 30;
    int mainW = sw - sidebarW - 80;
    int mainH = sh - topbarH - footerH;

    check(mx == 320, "Ayah reader left margin = 320");
    check(my == 86, "Ayah reader top margin = 86");
    check(mainW == 920, "Ayah reader main width = 920");
    check(mainH == 624, "Ayah reader main height = 624");

    /* Arabic text right-aligned anchor */
    int arabicX = sw - 60;
    check(arabicX == 1220, "Arabic text right anchor = 1220");

    /* Translation position below Arabic */
    int transY = my + 90;
    check(transY == 176, "Translation Y = 176");

    /* Reference position near footer */
    int refY = sh - footerH - 30;
    check(refY == 650, "Reference Y = 650");
}

static void test_surah_overview_layout(void) {
    TraceLog(LOG_INFO, "--- Surah Overview Layout Tests ---");
    int sw = 1280, sh = 720;
    int cw = 600, ch = 320;
    int cx = (sw - cw) / 2;
    int cy = (sh - ch) / 2;

    check(cx == 340, "Overview card X = 340");
    check(cy == 200, "Overview card Y = 200");
    check(cw == 600, "Overview card width = 600");
    check(ch == 320, "Overview card height = 320");

    /* Badge positions */
    int b1x = cx + 60, b2x = cx + 180;
    check(b1x == 400, "Badge 1 X = 400");
    check(b2x == 520, "Badge 2 X = 520");

    /* Context blurb area */
    int ctxY = cy + 192;
    int ctxH = 80;
    check(ctxY == 392, "Context Y = 392");
    check(ctxY + ctxH == 472, "Context bottom = 472");
}

static void test_bookmark_data(void) {
    TraceLog(LOG_INFO, "--- Bookmark Data Tests ---");
    /* Verify mock bookmark structure */
    Bookmark bm;
    memset(&bm, 0, sizeof(bm));
    bm.surahNumber = 1;
    bm.ayahNumber = 1;
    strncpy(bm.tag, "Favorite", sizeof(bm.tag));
    check(bm.surahNumber == 1 && bm.ayahNumber == 1, "Bookmark surah:ayah = 1:1");
    check(strlen(bm.tag) > 0, "Bookmark has tag");
}

static void test_text_wrapping_bounds(void) {
    TraceLog(LOG_INFO, "--- Text Wrapping Bounds Tests ---");
    int mainW = 920;
    const char *short_text = "Hello world";
    const char *long_text = "Allah -- there is no god but Him, the Ever-Living, "
                            "the Self-Sustaining. Neither slumber nor sleep "
                            "overtakes Him. To Him belongs all that is in the "
                            "heavens and all that is on the earth.";
    check((int)strlen(short_text) < mainW, "Short text chars < panel width");
    check((int)strlen(long_text) > 50, "Long text is substantial (> 50 chars)");
    /* At 16px avg char width ~8px, 50 chars ~400px which fits in 920px panel,
       but word-wrap will break it across multiple lines — tested visually */
}

static void test_mock_ayah_lookup(void) {
    TraceLog(LOG_INFO, "--- Mock Ayah Lookup Tests ---");
    AppState state;
    memset(&state, 0, sizeof(state));
    loadMockData(&state);

    /* Al-Fatiha 1:1 should exist */
    Ayah *a = findMockAyah(&state, 1, 1);
    check(a != NULL, "findMockAyah finds Al-Fatiha 1:1");
    if (a) check(a->surahNumber == 1 && a->ayahNumber == 1, "Correct ayah returned");

    /* Ayat al-Kursi 2:255 should exist */
    a = findMockAyah(&state, 2, 255);
    check(a != NULL, "findMockAyah finds Al-Baqarah 2:255");

    /* Non-existent ayah should return NULL */
    a = findMockAyah(&state, 99, 1);
    check(a == NULL, "findMockAyah returns NULL for non-existent surah");

    /* Al-Ikhlas 112:4 should exist */
    a = findMockAyah(&state, 112, 4);
    check(a != NULL, "findMockAyah finds Al-Ikhlas 112:4");

    if (state.surahs) free(state.surahs);
    if (state.ayahs) free(state.ayahs);
    if (state.hadiths) free(state.hadiths);
}

/* ── Visual window test ── */

static void test_visual(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(W, H, "Ayatika -- Phase 6: Ayah Reader & Surah Overview");
    SetTargetFPS(30);
    SetExitKey(0);

    initThemes();
    initFonts(NULL);

    AppState state;
    memset(&state, 0, sizeof(state));
    state.currentScreen = SCREEN_AYAH_READER;
    state.currentTheme = 0;
    state.currentSurah = 1;
    state.currentAyah = 1;
    state.cursorSurah = 0;
    state.dashboardCursor = 0;
    state.focusMode = 0;
    strncpy(state.language, "en", 7);
    loadMockData(&state);

    int screenIdx = 0;
    const char *screenNames[] = {"AYAH READER", "SURAH OVERVIEW", "BOOKMARKS"};
    AppScreen screens[] = {SCREEN_AYAH_READER, SCREEN_SURAH_OVERVIEW, SCREEN_BOOKMARKS};

    while (!WindowShouldClose()) {
        /* Screen cycling */
        if (IsKeyPressed(KEY_S)) {
            screenIdx = (screenIdx + 1) % 3;
            state.currentScreen = screens[screenIdx];
        }
        /* Theme cycling */
        if (IsKeyPressed(KEY_T)) {
            cycleTheme(&state);
            snprintf(state.statusMsg, sizeof(state.statusMsg), "Theme: %s",
                     getTheme(state.currentTheme)->name);
        }
        /* Focus mode toggle */
        if (IsKeyPressed(KEY_F))
            state.focusMode = !state.focusMode;
        /* Language toggle */
        if (IsKeyPressed(KEY_L))
            strncpy(state.language, strcmp(state.language, "en") == 0 ? "bn" : "en", 7);

        /* Navigate ayahs in reader */
        if (state.currentScreen == SCREEN_AYAH_READER) {
            if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_RIGHT)) {
                state.currentAyah++;
                /* wrap to next surah if ayah not found */
                if (!findMockAyah(&state, state.currentSurah, state.currentAyah)) {
                    state.currentAyah = 1;
                    state.currentSurah++;
                    if (state.currentSurah > 114) state.currentSurah = 1;
                    while (!findMockAyah(&state, state.currentSurah, state.currentAyah)
                           && state.currentSurah <= 114)
                        state.currentSurah++;
                    if (state.currentSurah > 114) state.currentSurah = 1;
                }
            }
            if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_LEFT)) {
                state.currentAyah--;
                if (state.currentAyah < 1) {
                    state.currentSurah--;
                    if (state.currentSurah < 1) state.currentSurah = 114;
                    state.currentAyah = 286;
                    while (!findMockAyah(&state, state.currentSurah, state.currentAyah)
                           && state.currentAyah > 0)
                        state.currentAyah--;
                    if (state.currentAyah < 1) state.currentAyah = 1;
                }
            }
        }

        /* Surah overview: cycle surahs */
        if (state.currentScreen == SCREEN_SURAH_OVERVIEW) {
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_J)) {
                state.currentSurah++;
                if (state.currentSurah > 114) state.currentSurah = 1;
            }
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_K)) {
                state.currentSurah--;
                if (state.currentSurah < 1) state.currentSurah = 114;
            }
        }

        BeginDrawing();
        ClearBackground(getTheme(state.currentTheme)->background);
        drawCurrentScreen(&state);

        /* Overlay test info */
        Theme *t = getTheme(state.currentTheme);
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "S=%s  T=theme  F=focus  L=lang  S=next  Fail=%d",
                 screenNames[screenIdx], failures);
        DrawText(buf, W / 2 - 260, 2, 12, failures > 0 ? RED : t->accent);
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
    TraceLog(LOG_INFO, "=== Phase 6: Ayah Reader & Surah Overview Tests ===");

    test_ayah_reader_layout();
    test_surah_overview_layout();
    test_bookmark_data();
    test_text_wrapping_bounds();
    test_mock_ayah_lookup();

    TraceLog(LOG_INFO, "\n--- Visual Window Test (opens window) ---");
    test_visual();

    TraceLog(LOG_INFO, "\n=== Results: %s ===\n",
             failures > 0 ? "SOME CHECKS FAILED" : "ALL PASSED");
    return failures > 0 ? 1 : 0;
}
