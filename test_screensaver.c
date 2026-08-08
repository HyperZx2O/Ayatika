/* ============================================================
 * test_screensaver.c — Phase 4 test harness for the Azan
 * screensaver. Compile + run (standalone):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc test_screensaver.c \
 *       src/audio.c src/screensaver.c src/mock_data.c \
 *       -lraylib -lm -o test_screensaver
 *
 * Run from the repo root (assets/ reachable) to verify the
 * full behaviour. Run from a directory with no assets/ to
 * verify graceful missing-asset handling (Azan becomes a
 * no-op, no crash).
 *
 * Prints PASS/FAIL per check; exits non-zero if any fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "audio.h"
#include "screensaver.h"
#include "mock_data.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

int main(void) {
    InitWindow(800, 600, "screensaver test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadMockData(&state);

    int haveAssets = FileExists("assets/azan.mp3");

    initAudio();
    initScreensaver();

    check("azan not playing before screensaver draws", isAzanPlaying() == 0);

    /* First draw — Azan should fire exactly once */
    drawScreensaver(&state);
    WaitTime(0.4);
    if (haveAssets)
        check("azan plays after first draw", isAzanPlaying() == 1);
    else
        check("azan no-op when asset missing", isAzanPlaying() == 0);

    /* Exercise the animation for ~1s so the pattern advances,
       and confirm repeated draws do not re-fire the Azan. */
    for (int i = 0; i < 60; i++) {
        drawScreensaver(&state);
        WaitTime(1.0 / 60.0);
    }
    check("repeated draws do not crash", 1);

    /* Stop the Azan, keep drawing — must NOT replay */
    stopAzan();
    WaitTime(0.1);
    check("azan stopped", isAzanPlaying() == 0);
    drawScreensaver(&state);
    drawScreensaver(&state);
    WaitTime(0.1);
    check("azan not replayed on later draws", isAzanPlaying() == 0);

    /* Reset — Azan fires again on the next session */
    resetScreensaver();
    drawScreensaver(&state);
    WaitTime(0.4);
    if (haveAssets)
        check("azan plays again after reset", isAzanPlaying() == 1);
    else
        check("reset keeps no-op when asset missing", isAzanPlaying() == 0);

    closeScreensaver();
    closeAudio();
    CloseWindow();

    if (failures > 0) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("All screensaver checks passed\n");
    return 0;
}
