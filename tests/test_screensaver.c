/* ============================================================
 * test_screensaver.c — screensaver + prayer-alarm harness.
 *
 * Idle screensaver entry is visual-only (silent); the Azan fires
 * only through firePrayerAlarm (real waqt or the settings test
 * button). Run from the repo root (assets/ reachable). Run from
 * a directory with no assets/ to verify graceful missing-asset
 * handling (Azan becomes a no-op, no crash).
 *
 * Prints PASS/FAIL per check; exits non-zero if any fails.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "audio.h"
#include "screensaver.h"
#include "test_data.h"

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

int main(void) {
    InitWindow(800, 600, "screensaver test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadTestData(&state);

    initAudio();
    initScreensaver();

    /* Idle entry — silent by design (alarm owns the Azan now) */
    drawScreensaver(&state);
    WaitTime(0.4);
    check("idle screensaver draws without crash", 1);

    /* Exercise the animation for ~1s */
    for (int i = 0; i < 60; i++) {
        drawScreensaver(&state);
        WaitTime(1.0 / 60.0);
    }
    check("repeated draws do not crash", 1);

    /* Prayer alarm — screensaver + Azan together */
    firePrayerAlarm(&state);
    WaitTime(0.4);
    check("alarm enters screensaver", state.currentScreen == SCREEN_SCREENSAVER);
    check("alarm remembers origin", state.previousScreen == SCREEN_DASHBOARD);

    /* Re-fire must not clobber the origin screen */
    firePrayerAlarm(&state);
    check("alarm re-fire keeps origin", state.previousScreen == SCREEN_DASHBOARD);

    /* Stop the Azan (what screensaver exit does) */
    stopAzan();
    WaitTime(0.1);
    check("azan stops without crash", 1);

    /* Re-init path draws cleanly */
    closeScreensaver();
    initScreensaver();
    drawScreensaver(&state);
    check("re-init draws without crash", 1);

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
