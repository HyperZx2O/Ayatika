/* ============================================================
 * test_audio.c — Phase 2 test harness for the audio system.
 * Compile + run (standalone, does not need the full app build):
 *
 *   gcc -std=c11 -Wall -Wextra -Isrc test_audio.c src/audio.c \
 *       src/mock_data.c -lraylib -lm -o test_audio
 *
 * Run from the repo root (assets/ must be reachable) to verify
 * SFX + nature-sound playback. Run from a directory with no
 * assets/ to verify graceful missing-asset handling.
 * ============================================================ */

#include <string.h>
#include "raylib.h"
#include "audio.h"
#include "mock_data.h"

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

    toggleNatureSound(&state);
    WaitTime(2.0);
    toggleNatureSound(&state);

    closeAudio();
    CloseWindow();

    return 0;
}
