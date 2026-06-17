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
void initAudio(void);
void updateAudio(AppState *state);   /* call every frame in main loop */
void closeAudio(void);

/* Azan */
void playAzan(void);
void stopAzan(void);
int  isAzanPlaying(void);

/* Recitation */
void playRecitation(const char *audioUrl);
void stopRecitation(void);
int  isRecitationPlaying(void);

/* Ambient */
void startNatureSound(void);
void stopNatureSound(void);
void toggleNatureSound(AppState *state);

/* SFX */
void playClickSfx(void);
void playSurahSwitchSfx(void);

#endif /* AUDIO_H */
