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
#include <string.h>
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

    // ponytail: ASCII placeholder until Frontend's drawArabicTextCentered lands
    const char *bismillah = "[Bismillah - Arabic text]";
    DrawText(bismillah, cx - MeasureText(bismillah, 24)/2, cy + 110, 24, (Color){220, 210, 185, 255});

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

// ponytail: deterministic Hadith-of-Day; stdlib time only, no extra deps
int getHadithIndexForYday(int yday, int total) {
    if (total <= 0) return 0;
    int idx = yday % total;
    if (idx < 0) idx += total;
    return idx;
}

void drawHadithPanel(AppState *state, int x, int y, int w, int h) {
    if (!state || !state->hadiths || state->totalHadiths == 0) return;
    if (w < 40 || h < 40) return;
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    int idx = getHadithIndexForYday(tmv ? tmv->tm_yday : 0, state->totalHadiths);
    Hadith *hd = &state->hadiths[idx];

    Color panel = (Color){28, 24, 20, 255};
    Color muted = (Color){120, 110, 90, 255};
    Color text  = (Color){220, 210, 185, 255};
    Color accent= (Color){180, 140, 60, 255};

    DrawRectangleRounded((Rectangle){(float)x,(float)y,(float)w,(float)h}, 0.05f, 6, panel);
    // narrator
    DrawText(hd->narrator, x+12, y+10, 13, muted);
    // body: simple word-wrap at w-24, truncate if overflows h
    int maxW = w - 24;
    int lineH = 16;
    int curY = y + 30;
    char buf[1024]; strncpy(buf, hd->text, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    char *word = strtok(buf, " ");
    char line[256] = {0};
    while (word) {
        char test[300];
        if (line[0]) snprintf(test, sizeof(test), "%s %s", line, word);
        else snprintf(test, sizeof(test), "%s", word);
        if (MeasureText(test, 14) > maxW && line[0]) {
            if (curY + lineH > y + h - 24) break;
            DrawText(line, x+12, curY, 14, text);
            curY += lineH;
            strncpy(line, word, sizeof(line)-1);
        } else {
            strncpy(line, test, sizeof(line)-1);
        }
        word = strtok(NULL, " ");
    }
    if (line[0] && curY + lineH <= y + h - 22) DrawText(line, x+12, curY, 14, text);
    // collection bottom-right
    DrawText(hd->collection, x + w - MeasureText(hd->collection, 12) - 12, y + h - 18, 12, accent);
}
