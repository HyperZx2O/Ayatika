#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mock_data.h"

#define W 800
#define H 600

static int failures = 0;

static void check(int cond, const char *label) {
    if (!cond) {
        failures++;
        TraceLog(LOG_WARNING, "FAIL: %s", label);
    } else {
        TraceLog(LOG_INFO, "PASS: %s", label);
    }
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(W, H, "Ayatika — Phase 1 Test");
    SetTargetFPS(30);
    SetExitKey(0);

    MockAppState state;
    memset(&state, 0, sizeof(state));
    loadMockData(&state);

    /* ── Verification ── */
    check(state.totalAyahs > 0, "loadMockData ran (totalAyahs > 0)");

    check(state.surahs != NULL, "surahs pointer non-NULL");
    check(state.surahs[0].number == 1, "Surah 1 is Al-Fatiha (number=1)");
    check(strlen(state.surahs[0].arabicName) > 0, "Surah 1 has Arabic name");
    check(strlen(state.surahs[0].name) > 0, "Surah 1 has English name");
    check(state.surahs[1].number == 2, "Surah 2 is Al-Baqarah");
    check(state.surahs[2].number == 112, "Surah 112 is Al-Ikhlas");
    check(state.surahs[3].number == 113, "Surah 113 is Al-Falaq");
    check(state.surahs[4].number == 114, "Surah 114 is An-Nas");

    int surahCount = 0;
    for (int i = 0; i < 7; i++)
        if (state.surahs[i].number > 0) surahCount++;
    check(surahCount >= 5, "At least 5 Surahs populated");

    check(state.ayahs != NULL, "ayahs pointer non-NULL");
    check(state.totalAyahs >= 10, "At least 10 Ayahs populated");

    MockAyah *bism = findMockAyah(&state, 1, 1);
    check(bism != NULL, "findMockAyah(1,1) finds Bismillah");
    check(bism != NULL && strlen(bism->arabicText) > 0, "Bismillah has Arabic text");
    check(bism != NULL && strlen(bism->translationEn) > 0, "Bismillah has English translation");

    MockAyah *kursi = findMockAyah(&state, 2, 255);
    check(kursi != NULL, "findMockAyah(2,255) finds Ayat al-Kursi");
    check(kursi != NULL && strstr(kursi->translationEn, "Allah") != NULL,
          "Ayat al-Kursi translation mentions Allah");

    MockAyah *nas1 = findMockAyah(&state, 114, 1);
    check(nas1 != NULL, "findMockAyah(114,1) finds An-Nas ayah 1");

    check(state.hadiths != NULL, "hadiths pointer non-NULL");
    check(state.totalHadiths >= 1, "At least 1 Hadith populated");

    check(state.prayer.fajrStr[0] != '\0', "Prayer fajrStr populated");
    check(state.prayer.dhuhrStr[0] != '\0', "Prayer dhuhrStr populated");

    check(state.currentScreen == MOCK_SCREEN_DASHBOARD, "Default screen is DASHBOARD");
    check(strlen(state.statusMsg) > 0, "Status message populated");
    check(strcmp(state.language, "en") == 0, "Default language is 'en'");

    check(findMockAyah(&state, 99, 99) == NULL, "findMockAyah(99,99) returns NULL (out of range)");

    /* ── Display ── */
    int frameCount = 0;
    while (!WindowShouldClose() && frameCount < 150) {
        BeginDrawing();
        ClearBackground((Color){18, 15, 12, 255});

        int y = 20;
        DrawText("Ayatika — Phase 1: RayLib + Mock Data Test", 40, y, 22,
                 (Color){180, 140, 60, 255});
        y += 40;

        char buf[128];
        snprintf(buf, sizeof(buf), "Status: %s", failures > 0 ? "FAILURES DETECTED" : "ALL PASSED");
        DrawText(buf, 40, y, 20, failures > 0 ? RED : GREEN);
        y += 35;

        snprintf(buf, sizeof(buf), "Surahs loaded: %d (need >= 5)", surahCount);
        DrawText(buf, 40, y, 16, (Color){220, 210, 185, 255}); y += 28;

        snprintf(buf, sizeof(buf), "Ayahs loaded:  %d (need >= 10)", state.totalAyahs);
        DrawText(buf, 40, y, 16, (Color){220, 210, 185, 255}); y += 28;

        snprintf(buf, sizeof(buf), "Hadiths loaded: %d (need >= 1)", state.totalHadiths);
        DrawText(buf, 40, y, 16, (Color){220, 210, 185, 255}); y += 28;

        snprintf(buf, sizeof(buf), "Prayer times:   Fajr=%s, Dhuhr=%s, Asr=%s, Maghrib=%s, Isha=%s",
                 state.prayer.fajrStr, state.prayer.dhuhrStr, state.prayer.asrStr,
                 state.prayer.maghribStr, state.prayer.ishaStr);
        DrawText(buf, 40, y, 14, (Color){120, 110, 90, 255}); y += 28;

        snprintf(buf, sizeof(buf), "Language: %s | Theme: %d | Failures: %d",
                 state.language, state.currentTheme, failures);
        DrawText(buf, 40, y, 16, (Color){220, 210, 185, 255}); y += 40;

        if (failures > 0) {
            DrawText("SOME CHECKS FAILED — check raylib log above", 40, y, 18, RED);
        } else {
            DrawText("All checks passed. Press any key to exit.", 40, y, 18, GREEN);
        }

        if (bism) {
            y = 200;
            DrawText("Sample Ayah (Bismillah, English):", 40, y, 14,
                     (Color){120, 110, 90, 255}); y += 20;
            DrawText(bism->translationEn, 40, y, 14, (Color){220, 210, 185, 255});
        }

        EndDrawing();
        frameCount++;

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE))
            break;
    }

    if (state.surahs) free(state.surahs);
    if (state.ayahs) free(state.ayahs);
    if (state.hadiths) free(state.hadiths);

    CloseWindow();
    TraceLog(LOG_INFO, "Phase 1 test exiting. Failures: %d", failures);
    return failures > 0 ? 1 : 0;
}
