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
void resetScreensaver(void);   /* allow the azan to fire again; call when leaving the screensaver */
void closeScreensaver(void);   /* unload the cat texture; call at exit */
int  getCatCurrentFrame(void); /* test seam: current cat animation frame index */

// ponytail: Hadith-of-the-Day lives here to avoid a 1-fn file; Frontend moves it to ui.c at integration
void drawHadithPanel(AppState *state, int x, int y, int w, int h); /* deterministic tm_yday % totalHadiths */
int  getHadithIndexForYday(int yday, int total); /* test seam */

#endif /* SCREENSAVER_H */
