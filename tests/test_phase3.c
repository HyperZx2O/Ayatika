#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fribidi/fribidi.h>
#include "mock_data.h"
#include "ui.h"
#include "theme.h"

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

/* ── Headless FriBidi tests (no window needed) ── */

static void test_fribidi_basics(void) {
    TraceLog(LOG_INFO, "--- FriBidi Basic Tests ---");

    /* Test 1: FriBidi initializes and reorders a known RTL string */
    {
        const char *bismillah = "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ";
        FriBidiChar logical[2048];
        FriBidiStrIndex len = fribidi_charset_to_unicode(
            FRIBIDI_CHAR_SET_UTF8, bismillah, strlen(bismillah), logical);
        check(len > 0, "fribidi_charset_to_unicode produces output");

        FriBidiChar visual[2048];
        FriBidiParType baseDir = FRIBIDI_PAR_RTL;
        FriBidiLevel levels[2048];
        FriBidiLevel maxLevel = fribidi_log2vis(
            logical, len, &baseDir, visual, NULL, NULL, levels);
        check(maxLevel > 0, "fribidi_log2vis reorders (level > 0)");
        (void)maxLevel;

        char visualUtf8[4096];
        fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual, len, visualUtf8);
        check(strlen(visualUtf8) > 0, "fribidi_unicode_to_charset produces non-empty output");
    }

    /* Test 2: Short LTR string should still work (no crash) */
    {
        const char *english = "Hello World";
        FriBidiChar logical[2048];
        FriBidiStrIndex len = fribidi_charset_to_unicode(
            FRIBIDI_CHAR_SET_UTF8, english, strlen(english), logical);
        check(len > 0, "English string converts to FriBidiChar");

        FriBidiChar visual[2048];
        FriBidiParType baseDir = FRIBIDI_PAR_RTL;
        FriBidiLevel levels[2048];
        FriBidiLevel maxLevel = fribidi_log2vis(
            logical, len, &baseDir, visual, NULL, NULL, levels);
        check(maxLevel >= 0, "English string reorders without error");
        (void)maxLevel;

        char visualUtf8[4096];
        fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual, len, visualUtf8);
        check(strlen(visualUtf8) > 0, "English visual output non-empty");
    }

    /* Test 3: Multiple mock ayahs reorder without error */
    {
        const char *ayats[] = {
            "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ",
            "الْحَمْدُ لِلَّهِ رَبِّ الْعَالَمِينَ",
            "قُلْ هُوَ اللَّهُ أَحَدٌ",
            "إِنَّا أَعْطَيْنَاكَ الْكَوْثَرَ",
        };
        int n = sizeof(ayats) / sizeof(ayats[0]);
        const char *labels[] = {
            "Ayah 0 (Bismillah) converts",
            "Ayah 1 (Hamd) converts",
            "Ayah 2 (Ikhlas) converts",
            "Ayah 3 (Kawthar) converts",
        };
        for (int i = 0; i < n; i++) {
            FriBidiChar logical[2048];
            FriBidiStrIndex len = fribidi_charset_to_unicode(
                FRIBIDI_CHAR_SET_UTF8, ayats[i], strlen(ayats[i]), logical);
            check(len > 0, labels[i]);

            FriBidiChar visual[2048];
            FriBidiParType baseDir = FRIBIDI_PAR_RTL;
            FriBidiLevel levels[2048];
            fribidi_log2vis(logical, len, &baseDir, visual, NULL, NULL, levels);

            char out[4096];
            fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual, len, out);
            check(strlen(out) > 0, labels[i]);
        }
    }
}

/* ── Visual window test ── */

static void test_visual_rendering(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(W, H, "Ayatika — Phase 3: Arabic RTL Text Rendering");
    SetTargetFPS(30);
    SetExitKey(0);

    initThemes();
    initFonts(NULL);

    Theme *t = getTheme(0);

    AppState state;
    memset(&state, 0, sizeof(state));
    state.currentScreen = SCREEN_DASHBOARD;
    state.currentTheme = 0;
    strncpy(state.language, "en", 7);
    loadMockData(&state);

    int frameCount = 0;
    while (!WindowShouldClose() && frameCount < 300) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        BeginDrawing();
        ClearBackground(t->background);

        /* Title */
        DrawText("Phase 3 — Arabic RTL Text Rendering", 40, 16, 22, t->accent);

        /* Test 1: Bismillah */
        drawArabicText("بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ",
                       (Vector2){40, 60}, 36, t->foreground);
        DrawText("→ Bismillah", 40, 100, 14, t->muted);

        /* Test 2: Multiple ayahs from mock data */
        int y = 140;
        const char *testAyahs[] = {
            state.ayahs[0].arabicText,      /* Fatiha 1:1 */
            state.ayahs[6].arabicText,      /* Fatiha 1:7 */
            state.ayahs[10].arabicText,     /* Baqarah 2:255 */
        };
        for (int i = 0; i < 3; i++) {
            drawArabicText(testAyahs[i], (Vector2){40, (float)y}, 28, t->foreground);
            y += 50;
        }
        DrawText("→ 3 mock ayahs (Fatiha 1:1, 1:7, Baqarah 2:255)", 40, y, 14, t->muted);
        y += 40;

        /* Test 3: Centered text within a 400px-wide rectangle */
        DrawRectangleLines(sw/2 - 200, y, 400, 100, t->border);
        drawArabicTextCentered("بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ",
                               (Rectangle){(float)(sw/2 - 200), (float)y, 400, 40},
                               32, t->foreground);
        DrawText("→ Centered Bismillah (within box above)", 40, y + 110, 14, t->muted);
        y += 130;

        /* Test 4: Long centered text */
        DrawRectangleLines(sw/2 - 200, y, 400, 100, t->border);
        drawArabicTextCentered("إِنَّا أَعْطَيْنَاكَ الْكَوْثَرَ",
                               (Rectangle){(float)(sw/2 - 200), (float)y, 400, 40},
                               28, t->accent);
        DrawText("→ Centered Al-Kawthar (within box above)", 40, y + 110, 14, t->muted);
        y += 130;

        /* Footer */
        DrawText("Press ENTER/ESC to exit | Visual: verify RTL, ligatures, diacritics",
                 40, sh - 30, 14, t->muted);

        /* Status */
        char buf[128];
        snprintf(buf, sizeof(buf), "Failures: %d", failures);
        DrawText(buf, sw - 140, 20, 16, failures > 0 ? RED : GREEN);

        EndDrawing();
        frameCount++;

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE))
            break;
    }

    closeFonts();

    if (state.surahs) free(state.surahs);
    if (state.ayahs) free(state.ayahs);
    if (state.hadiths) free(state.hadiths);

    CloseWindow();
}

int main(void) {
    TraceLog(LOG_INFO, "=== Phase 3 Tests ===");

    test_fribidi_basics();

    TraceLog(LOG_INFO, "\n--- Visual Rendering Test (opens window) ---");
    test_visual_rendering();

    TraceLog(LOG_INFO, "\n=== Results: %s ===\n",
             failures > 0 ? "SOME CHECKS FAILED" : "ALL PASSED");
    return failures > 0 ? 1 : 0;
}
