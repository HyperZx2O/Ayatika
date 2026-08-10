/* ============================================================
 * screensaver.c — Azan screensaver and sleeping cat animation
 * Owned by: Systems & Features Engineer
 *
 * Responsibilities:
 *   - Idle screensaver: animated geometric pattern, current
 *     time, next prayer line; plays Azan once per session
 *   - Sleeping cat sprite sheet animation (Phase 5)
 *
 * See context/plan.md Phase 4 for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "screensaver.h"
#include "audio.h"

#define CAT_FRAME_COUNT 6   /* frames in assets/cat.png; adjust if sheet differs */

static int azanFired = 0;   /* only play Azan once per screensaver session */

static Texture2D catSheet;
static int       catLoaded       = 0;
static int       catFrameCount   = CAT_FRAME_COUNT;
static int       catFrameWidth   = 0;
static int       catFrameHeight  = 0;
static int       catCurrentFrame = 0;
static float     catFrameTimer   = 0.0f;
static float     catFrameSpeed   = 0.15f;  /* seconds per frame */

void initScreensaver(void) {
    azanFired        = 0;
    catCurrentFrame  = 0;
    catFrameTimer    = 0.0f;
    if (FileExists("assets/cat.png")) {
        catSheet      = LoadTexture("assets/cat.png");
        catLoaded     = 1;
        catFrameWidth  = catSheet.width / catFrameCount;
        catFrameHeight = catSheet.height;
    }
}

void resetScreensaver(void) {
    azanFired = 0;   /* allow Azan to play again next session */
}

void closeScreensaver(void) {
    if (catLoaded) {
        UnloadTexture(catSheet);
        catLoaded = 0;
    }
}

void drawScreensaver(AppState *state) {
    (void)state;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground(BLACK);

    /* Play Azan once when screensaver first fires */
    if (!azanFired && !isAzanPlaying()) {
        playAzan();
        azanFired = 1;
    }

    /* Pulsing geometric pattern — rotating lines */
    float t  = (float)GetTime();
    int   cx = sw / 2;
    int   cy = sh / 2 - 60;

    for (int i = 0; i < 8; i++) {
        float angle  = (i * 45.0f + t * 10.0f) * DEG2RAD;
        float radius = 80.0f + sinf(t * 0.8f + i) * 15.0f;
        Vector2 p1 = { cx + cosf(angle) * (radius - 20),
                       cy + sinf(angle) * (radius - 20) };
        Vector2 p2 = { cx + cosf(angle) * (radius + 20),
                       cy + sinf(angle) * (radius + 20) };
        DrawLineEx(p1, p2, 1.5f, (Color){180, 140, 60, 120});
    }

    DrawCircleLines(cx, cy, 60.0f + sinf(t) * 5.0f,
                    (Color){180, 140, 60, 80});

    /* Current time */
    time_t now = time(NULL);
    struct tm *now_tm = localtime(&now);
    char timeStr[16];
    int hour = now_tm->tm_hour % 12;
    if (hour == 0) hour = 12;
    snprintf(timeStr, sizeof(timeStr), "%d:%02d %s",
             hour, now_tm->tm_min, now_tm->tm_hour >= 12 ? "PM" : "AM");
    DrawText(timeStr,
             sw/2 - MeasureText(timeStr, 52)/2,
             sh/2 + 80, 52, (Color){220, 210, 185, 255});

    /* Next prayer — stub for now, real call comes from Backend */
    const char *prayerLine = "Next: Dhuhr";
    DrawText(prayerLine,
             sw/2 - MeasureText(prayerLine, 18)/2,
             sh/2 + 150, 18, (Color){120, 110, 90, 255});

    DrawText("Press any key to return",
             sw/2 - MeasureText("Press any key to return", 14)/2,
             sh - 40, 14, (Color){80, 75, 60, 255});

    /* Sleeping cat — bottom-right corner */
    drawCat(state);
}

void drawCat(AppState *state) {
    (void)state;
    if (!catLoaded) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    /* Advance animation frame */
    catFrameTimer += GetFrameTime();
    if (catFrameTimer >= catFrameSpeed) {
        catCurrentFrame = (catCurrentFrame + 1) % catFrameCount;
        catFrameTimer   = 0.0f;
    }

    /* Source rectangle: current frame from sprite sheet */
    Rectangle src = {
        (float)(catCurrentFrame * catFrameWidth),
        0.0f,
        (float)catFrameWidth,
        (float)catFrameHeight
    };

    /* Destination: bottom-right corner */
    float scale = 2.5f;   /* pixel art looks better scaled up */
    Rectangle dst = {
        (float)(sw - (int)(catFrameWidth  * scale) - 20),
        (float)(sh - (int)(catFrameHeight * scale) - 50),
        (float)(catFrameWidth  * scale),
        (float)(catFrameHeight * scale)
    };

    DrawTexturePro(catSheet, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
}

int getCatCurrentFrame(void) {
    return catCurrentFrame;
}
