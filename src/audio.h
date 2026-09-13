#ifndef AUDIO_H
#define AUDIO_H

/* ============================================================
 * audio.h — Audio system (Azan + reminder alerts)
 * Owned by: Systems & Features Engineer
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include "quran.h"

/* Lifecycle */
void initAudio(void);                 /* init audio device + load all sound assets; call once at startup */
void updateAudio(AppState *state);    /* waqt alerts; call every frame */
void closeAudio(void);                /* unload all assets + close the audio device; call at exit */

/* Azan */
void playAzan(void);                  /* play the azan clip once; no-op if it is already playing */
void stopAzan(void);                  /* stop the azan immediately */

/* Reminder (5 min before waqt) */
void playReminder(void);              /* play the reminder clip once; no-op if already playing */
void stopReminder(void);              /* stop the reminder immediately */

/* Waqt alerts — reminder at T-5min, azan at T-0; call every frame via updateAudio */
void checkPrayerAlerts(AppState *state);

#endif /* AUDIO_H */
