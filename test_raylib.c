#include "raylib.h"
#include "src/mock_data.h"
#include "src/audio.h"
#include "src/screensaver.h"
#include "src/search.h"
#include <string.h>

int main(void) {
    InitWindow(400, 200, "audio test");

    AppState state;
    memset(&state, 0, sizeof(AppState));
    loadMockData(&state);

    /* initAudio() owns the audio device — do not call InitAudioDevice()
       here too, it is not re-entrant and crashes on a live device. */
    initAudio();
    playClickSfx();
    WaitTime(1.0);
    playSurahSwitchSfx();
    WaitTime(1.0);

    toggleNatureSound(&state);
    WaitTime(2.0);
    toggleNatureSound(&state);

    startNatureSound();
    stopNatureSound();

    playAzan();
    WaitTime(1.0);
    stopAzan();

    closeAudio();

    initScreensaver();
    closeScreensaver();
    CloseWindow();

    return 0;
}
