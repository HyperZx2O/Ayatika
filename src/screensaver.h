#ifndef SCREENSAVER_H
#define SCREENSAVER_H

/* ============================================================
 * screensaver.h — Azan screensaver and sleeping cat animation
 * Owned by: Systems & Features Engineer
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include "quran.h"

void initScreensaver(void);    /* load the cat texture + reset animation state; call once at startup */
void drawScreensaver(AppState *state); /* draw pattern/time/prayer, fire the azan once, draw the cat; call every frame */
void drawCat(AppState *state); /* animate + draw the sleeping cat bottom-right; call every frame */
void firePrayerAlarm(AppState *state); /* waqt moment: enter screensaver + play azan; live trigger and test button share this */
void closeScreensaver(void);   /* unload the cat texture; call at exit */

#endif /* SCREENSAVER_H */
