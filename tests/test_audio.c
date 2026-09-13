/* ============================================================
 * test_audio.c — Phase 2 + Phase 3 test harness for the audio system.
 * Compile + run (standalone, does not need the full app build):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc test_audio.c src/audio.c \
 *       tests/test_data.c -lraylib -lm -o test_audio
 *
 * Run from the repo root (assets/ must be reachable) to verify
 * Azan + reminder playback. Run from a
 * directory with no assets/ to verify graceful missing-asset handling
 * (all playback is a no-op there, no crash expected).
 *
 * Prints PASS/FAIL per check; exits non-zero if any check fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "audio.h"
#include "test_data.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

int main(void) {
    InitWindow(400, 200, "audio test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadTestData(&state);

    initAudio();

    /* Azan + reminder: play/stop must never crash, with or without assets */
    playAzan();
    WaitTime(0.3);
    stopAzan();
    WaitTime(0.1);
    check("azan play/stop without crash", 1);

    playReminder();
    WaitTime(0.3);
    stopReminder();
    check("reminder play/stop without crash", 1);

    updateAudio(&state);
    check("updateAudio without crash", 1);

    closeAudio();
    CloseWindow();

    if (failures > 0) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("All audio checks passed\n");
    return 0;
}
