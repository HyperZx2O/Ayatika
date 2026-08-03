/* ============================================================
 * audio.c — Audio system (Azan, recitation, nature, SFX)
 * Owned by: Systems & Features Engineer
 *
 * Responsibilities:
 *   - Load and play Azan, click, and Surah-switch sounds
 *   - Loop ambient nature sound at low volume
 *   - Stream recitation audio from CDN URLs
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include "audio.h"

static Sound clickSound;
static Sound surahSwitchSound;
static Music natureMusic;

static int clickLoaded       = 0;
static int surahSwitchLoaded = 0;
static int natureLoaded      = 0;

void initAudio(void) {
    InitAudioDevice();

    if (FileExists("assets/click.wav")) {
        clickSound  = LoadSound("assets/click.wav");
        clickLoaded = 1;
    }
    if (FileExists("assets/surah_switch.wav")) {
        surahSwitchSound  = LoadSound("assets/surah_switch.wav");
        surahSwitchLoaded = 1;
    }
    if (FileExists("assets/nature.ogg")) {
        natureMusic  = LoadMusicStream("assets/nature.ogg");
        natureLoaded = 1;
        SetMusicVolume(natureMusic, 0.25f);
        natureMusic.looping = 1;
    }
}

void updateAudio(AppState *state) {
    if (natureLoaded && state->isNatureSoundOn)
        UpdateMusicStream(natureMusic);
}

void closeAudio(void) {
    if (clickLoaded)       UnloadSound(clickSound);
    if (surahSwitchLoaded) UnloadSound(surahSwitchSound);
    if (natureLoaded)      UnloadMusicStream(natureMusic);
    CloseAudioDevice();
}

void playAzan(void)               { /* TODO (Phase 3) */ }
void stopAzan(void)                { /* TODO (Phase 3) */ }
int  isAzanPlaying(void)           { /* TODO (Phase 3) */ return 0; }

void playRecitation(const char *audioUrl) { (void)audioUrl; /* TODO (Phase 3) */ }
void stopRecitation(void)                  { /* TODO (Phase 3) */ }
int  isRecitationPlaying(void)             { /* TODO (Phase 3) */ return 0; }

void startNatureSound(void) {
    if (natureLoaded && !IsMusicStreamPlaying(natureMusic))
        PlayMusicStream(natureMusic);
}

void stopNatureSound(void) {
    if (natureLoaded) StopMusicStream(natureMusic);
}

void toggleNatureSound(AppState *state) {
    state->isNatureSoundOn = !state->isNatureSoundOn;
    if (state->isNatureSoundOn) startNatureSound();
    else                        stopNatureSound();
}

void playClickSfx(void) {
    if (clickLoaded) PlaySound(clickSound);
}

void playSurahSwitchSfx(void) {
    if (surahSwitchLoaded) PlaySound(surahSwitchSound);
}
