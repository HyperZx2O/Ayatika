#ifndef SCREENSAVER_H
#define SCREENSAVER_H

/* ============================================================
 * screensaver.h — Azan screensaver and sleeping cat animation
 * Owned by: Systems & Features Engineer
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include "quran.h"

void initScreensaver(void);
void drawScreensaver(AppState *state);
void drawCat(AppState *state);
void resetScreensaver(void);
void closeScreensaver(void);

#endif /* SCREENSAVER_H */
