/* ============================================================
 * test_audio.c — Phase 2 + Phase 3 test harness for the audio system.
 * Compile + run (standalone, does not need the full app build):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc test_audio.c src/audio.c \
 *       src/mock_data.c -lraylib -lm -o test_audio
 *
 * Run from the repo root (assets/ must be reachable) to verify
 * SFX + nature-sound playback, Azan, and recitation. Run from a
 * directory with no assets/ to verify graceful missing-asset handling
 * (all playback checks are skipped there, no crash expected).
 *
 * Prints PASS/FAIL per check; exits non-zero if any check fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "audio.h"
#include "mock_data.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

int main(void) {
    InitWindow(400, 200, "audio test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadMockData(&state);

    initAudio();
    playClickSfx();
    WaitTime(1.0);
    playSurahSwitchSfx();
    WaitTime(1.0);

    int haveAssets = FileExists("assets/click.wav");

    toggleNatureSound(&state);
    WaitTime(2.0);
    toggleNatureSound(&state);
    check("nature sound stops after toggle off", state.isNatureSoundOn == 0);

    /* Phase 3 — Azan */
    check("azan not playing before play", isAzanPlaying() == 0);
    playAzan();
    WaitTime(0.3);
    if (haveAssets)
        check("azan playing after play", isAzanPlaying() == 1);
    else
        check("azan is a no-op when asset missing", isAzanPlaying() == 0);
    WaitTime(0.5);
    stopAzan();
    WaitTime(0.1);
    check("azan stopped after stop", isAzanPlaying() == 0);

    /* Phase 3 — Recitation */
    check("recitation not active before play", isRecitationPlaying() == 0);
    playRecitation("assets/nature.ogg");
    if (haveAssets)
        check("recitation active after play", isRecitationPlaying() == 1);
    else
        check("recitation no-op when asset missing", isRecitationPlaying() == 0);
    updateAudio(&state);
    WaitTime(1.0);

    playRecitation("assets/click.wav");
    if (haveAssets)
        check("recitation switches without leak (still active)",
              isRecitationPlaying() == 1);
    updateAudio(&state);
    WaitTime(1.0);

    stopRecitation();
    check("recitation stopped after stop", isRecitationPlaying() == 0);

    playRecitation("");
    check("empty path is a no-op", isRecitationPlaying() == 0);

    playRecitation("assets/missing_file.ogg");
    check("missing file is a no-op", isRecitationPlaying() == 0);

    closeAudio();
    CloseWindow();

    if (failures > 0) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("All audio checks passed\n");
    return 0;
}
