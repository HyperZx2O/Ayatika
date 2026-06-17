/* ============================================================
 * screensaver.c — Azan screensaver and sleeping cat animation
 * Owned by: Systems & Features Engineer
 *
 * Responsibilities:
 *   - Idle screensaver: animated pattern, Bismillah, time,
 *     next prayer countdown, plays Azan once per session
 *   - Sleeping cat sprite sheet animation shown on shorter idle
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include "screensaver.h"
#include "audio.h"

/* TODO: static Texture2D catSheet + frame counters */

void initScreensaver(void) {
    /* TODO: load assets/cat.png, compute frame dimensions */
}

void drawScreensaver(AppState *state) {
    (void)state;
    /* TODO: ClearBackground, geometric animation, Bismillah text,
       current time, next prayer countdown, playAzan() once */
}

void drawCat(AppState *state) {
    (void)state;
    /* TODO: DrawTexturePro with cycling source rectangle */
}

void resetScreensaver(void) {
    /* TODO: reset azanFired flag */
}

void closeScreensaver(void) {
    /* TODO: UnloadTexture(catSheet) */
}
