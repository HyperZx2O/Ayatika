/* ============================================================
 * test_cat.c — Phase 5 test harness for the sleeping cat
 * sprite animation. Compile + run (standalone, does not need
 * the full app build):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc test_cat.c \
 *       src/screensaver.c src/audio.c src/mock_data.c \
 *       -lraylib -lm -o test_cat
 *
 * Run from the repo root (assets/cat.png reachable) to verify
 * the animation cycles through every frame. Run from a
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
#include "mock_data.h"

#define EXPECTED_FRAMES 6   /* must match CAT_FRAME_COUNT in screensaver.c */

static int failures = 0;

static void check(const char *name, int ok) {
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

int main(void) {
    InitWindow(800, 600, "cat test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadMockData(&state);

    int haveCat = FileExists("assets/cat.png");

    initAudio();
    initScreensaver();

    check("cat frame starts at 0", getCatCurrentFrame() == 0);

    if (haveCat) {
        /* ~0.5s of real frames — the frame counter must advance */
        for (int i = 0; i < 30; i++) {
            BeginDrawing();
            drawCat(&state);
            EndDrawing();
            WaitTime(1.0 / 60.0);
        }
        check("cat frame advances over time", getCatCurrentFrame() > 0);

        /* ~3s more — must cycle through ALL frames and wrap around.
           A bitmask of seen frames catches stuck/missing/out-of-range
           frames at once. */
        unsigned seen = 0;
        for (int i = 0; i < 180; i++) {
            BeginDrawing();
            drawCat(&state);
            EndDrawing();
            seen |= (1u << getCatCurrentFrame());
            WaitTime(1.0 / 60.0);
        }
        check("cat cycles through all frames and wraps",
              seen == (1u << EXPECTED_FRAMES) - 1u);
    } else {
        /* Missing asset — drawCat must be a no-op, no crash */
        for (int i = 0; i < 30; i++) {
            BeginDrawing();
            drawCat(&state);
            EndDrawing();
            WaitTime(1.0 / 60.0);
        }
        check("cat no-op when asset missing (frame stays 0)",
              getCatCurrentFrame() == 0);
    }

    /* Integration: drawScreensaver must drive the cat on its own.
       Re-init resets the animation state, then only drawScreensaver
       runs — the frame advancing proves it draws the cat. */
    closeScreensaver();
    initScreensaver();
    check("cat animation state resets on re-init", getCatCurrentFrame() == 0);

    for (int i = 0; i < 30; i++) {
        BeginDrawing();
        drawScreensaver(&state);
        EndDrawing();
        WaitTime(1.0 / 60.0);
    }
    check("drawScreensaver draws cat without crash", 1);
    if (haveCat)
        check("drawScreensaver drives cat animation", getCatCurrentFrame() > 0);
    else
        check("cat no-op via drawScreensaver when asset missing",
              getCatCurrentFrame() == 0);

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
