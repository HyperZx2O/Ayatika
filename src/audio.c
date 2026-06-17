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

/* TODO: static Sound/Music handles + loaded flags */

void initAudio(void) {
    InitAudioDevice();
    /* TODO: load azan.mp3, click.wav, surah_switch.wav, nature.ogg */
}

void updateAudio(AppState *state) {
    (void)state;
    /* TODO: UpdateMusicStream for nature + recitation streams */
}

void closeAudio(void) {
    /* TODO: unload all sounds/music */
    CloseAudioDevice();
}

void playAzan(void)               { /* TODO */ }
void stopAzan(void)                { /* TODO */ }
int  isAzanPlaying(void)           { /* TODO */ return 0; }

void playRecitation(const char *audioUrl) { (void)audioUrl; /* TODO */ }
void stopRecitation(void)                  { /* TODO */ }
int  isRecitationPlaying(void)             { /* TODO */ return 0; }

void startNatureSound(void)                { /* TODO */ }
void stopNatureSound(void)                 { /* TODO */ }
void toggleNatureSound(AppState *state)    { (void)state; /* TODO */ }

void playClickSfx(void)            { /* TODO */ }
void playSurahSwitchSfx(void)      { /* TODO */ }
