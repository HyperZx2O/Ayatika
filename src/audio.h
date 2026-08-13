#ifndef AUDIO_H
#define AUDIO_H

/* ============================================================
 * audio.h — Audio system (Azan, recitation, nature, SFX)
 * Owned by: Systems & Features Engineer
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include "quran.h"

/* Lifecycle */
void initAudio(void);                 /* init audio device + load all sound assets; call once at startup */
void updateAudio(AppState *state);    /* keep music streams alive; call every frame */
void closeAudio(void);                /* unload all assets + close the audio device; call at exit */

/* Azan */
void playAzan(void);                  /* play the azan clip once; no-op if it is already playing */
void stopAzan(void);                  /* stop the azan immediately */
int  isAzanPlaying(void);             /* 1 while the azan is playing */

/* Recitation
 * filePath is a LOCAL file path, not a CDN URL — raylib's
 * LoadMusicStream only reads local files. The Backend downloads
 * the recitation to a temp file first, then calls this with the path. */
void playRecitation(const char *filePath);  /* stream a recitation from a local file, replacing any current one */
void stopRecitation(void);                  /* stop + unload the current recitation stream */
int  isRecitationPlaying(void);             /* 1 while a recitation is streaming */

/* Ambient */
void startNatureSound(void);          /* begin looping the nature ambience at low volume */
void stopNatureSound(void);           /* stop the nature ambience */
void toggleNatureSound(AppState *state);  /* flip state->isNatureSoundOn and start/stop accordingly */

/* SFX */
void playClickSfx(void);              /* short UI click blip, e.g. on bookmark save */
void playSurahSwitchSfx(void);        /* transition sound, e.g. when opening a surah */

#endif /* AUDIO_H */
