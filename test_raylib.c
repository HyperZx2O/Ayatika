#include "raylib.h"
#include "src/mock_data.h"
#include "src/audio.h"
#include "src/screensaver.h"
#include "src/search.h"
#include <string.h>

int main(void) {
    InitWindow(400, 200, "audio test");
    InitAudioDevice();

    if (IsAudioDeviceReady()) {
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

        startNatureSound();
        stopNatureSound();

        playAzan();
        WaitTime(1.0);
        stopAzan();

        closeAudio();

        initScreensaver();
        closeScreensaver();
    }

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
