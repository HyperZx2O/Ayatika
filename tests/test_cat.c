/* ============================================================
 * test_cat.c — Phase 5 test harness for the sleeping cat
 * sprite animation. Compile + run (standalone, does not need
 * the full app build):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc test_cat.c \
 *       src/screensaver.c src/audio.c tests/test_data.c \
 *       -lraylib -lm -o test_cat
 *
 * Run from the repo root (assets/cat.png reachable) to verify
 * the cat draws without crashing. Run from a
 * directory with no assets/ to verify graceful missing-asset
 * handling (drawCat becomes a no-op, no crash).
 *
 * NOTE: drawCat advances via GetFrameTime(), which raylib only
 * updates between BeginDrawing()/EndDrawing(), so each animated
 * "frame" here is a real drawn frame + WaitTime, mirroring the
 * main loop.
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
    InitWindow(800, 600, "cat test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadTestData(&state);

    initAudio();
    initScreensaver();

    /* ~0.5s of real frames — must draw without crashing */
    for (int i = 0; i < 30; i++) {
        BeginDrawing();
        drawCat(&state);
        EndDrawing();
        WaitTime(1.0 / 60.0);
    }
    check("cat draws without crash", 1);

    /* ~3s more — animation must keep drawing without crashing */
    for (int i = 0; i < 180; i++) {
        BeginDrawing();
        drawCat(&state);
        EndDrawing();
        WaitTime(1.0 / 60.0);
    }
    check("cat keeps drawing without crash", 1);

    /* Integration: drawScreensaver drives the cat on its own. */
    closeScreensaver();
    initScreensaver();

    for (int i = 0; i < 30; i++) {
        BeginDrawing();
        drawScreensaver(&state);
        EndDrawing();
        WaitTime(1.0 / 60.0);
    }
    check("drawScreensaver draws cat without crash", 1);

    closeScreensaver();
    closeAudio();
    CloseWindow();

    if (failures > 0) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("All cat checks passed\n");
    return 0;
}
