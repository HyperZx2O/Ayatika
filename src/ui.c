#include <raylib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fribidi/fribidi.h>
#include "ui.h"
#include "theme.h"
#include "input.h"
#include "search.h"
#include "screensaver.h"

/* Forward declarations for helpers used before their definitions. */
static int reorderArabic(const char *text, char *visualOut, int outSize);
static float drawArabicWrapped(Font af, const char *visual, float xRight, float yTop,
                               float maxW, float maxH, float size0, float sizeMin, Color color);
static void drawWrappedText(const char *text, Rectangle bounds, int fontSize, Color color);
static void drawBookmarkEditor(AppState *state);

Font arabicFont;
Font uiFont;
Font bengaliFont;

Scale S;

/* ── Focus mode render texture ── */
static RenderTexture2D focusTarget = {0};
static int focusTargetW = 0, focusTargetH = 0;
static double focusStartTime = 0;
static int focusWasActive = 0;

void initFocusTexture(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    if (w > 0 && h > 0) {
        focusTarget = LoadRenderTexture(w, h);
        focusTargetW = w;
        focusTargetH = h;
    }
}

void closeFocusTexture(void) {
    if (focusTarget.texture.id > 0) {
        UnloadRenderTexture(focusTarget);
        focusTarget = (RenderTexture2D){0};
    }
    focusTargetW = focusTargetH = 0;
}

static void ensureFocusTexture(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    if (w != focusTargetW || h != focusTargetH) {
        closeFocusTexture();
        if (w > 0 && h > 0) {
            focusTarget = LoadRenderTexture(w, h);
            focusTargetW = w;
            focusTargetH = h;
        }
    }
}

Scale computeScale(int sw, int sh) {
    Scale s;
    s.sw = sw;
    s.sh = sh;

    float fx = (float)sw / 1280.0f;
    float fy = (float)sh / 720.0f;
    s.factor = (fx < fy) ? fx : fy;
    if (s.factor < 0.4f) s.factor = 0.4f;
    if (s.factor > 3.0f) s.factor = 3.0f;

    float f = s.factor;

    #define SCL(v) (int)((v) * f + 0.5f)
    #define SCL_MIN(v, mn) (SCL(v) < (mn) ? (mn) : SCL(v))

    s.topbarH  = SCL(56);
    s.sidebarW = SCL(280);
    s.footerH  = SCL(40);

    s.mx = SCL(40);
    s.my = SCL(40);
    s.gx = SCL(20);
    s.gy = SCL(20);

    s.fs12 = SCL_MIN(12, 10);
    s.fs13 = SCL_MIN(13, 10);
    s.fs14 = SCL_MIN(14, 11);
    s.fs16 = SCL_MIN(16, 12);
    s.fs18 = SCL_MIN(18, 14);
    s.fs20 = SCL_MIN(20, 15);
    s.fs22 = SCL_MIN(22, 16);
    s.fs26 = SCL_MIN(26, 18);
    s.fs34 = SCL_MIN(34, 22);
    s.fs40 = SCL_MIN(40, 26);
    s.fs42 = SCL_MIN(42, 28);
    s.fs48 = SCL_MIN(48, 32);

    s.popupW = SCL(600);
    s.popupH = SCL(320);
    s.helpW  = SCL(520);
    s.helpH  = SCL(480);

    s.sidebarRowH  = SCL(48);
    s.bookmarkRowH = SCL(58);

    s.cardPadX = SCL(12);
    s.cardPadY = SCL(20);
    s.badgeW   = SCL(100);
    s.badgeH   = SCL(24);
    s.badgeGap = SCL(120);
    s.progressH   = SCL(8);
    s.progressGap = SCL(20);
    s.dotRadius   = SCL(4);
    if (s.dotRadius < 2) s.dotRadius = 2;
    s.topbarPrayerXOffset = SCL(220);

    #undef SCL
    #undef SCL_MIN
    return s;
}

static void addCodepoints(const char *text, int *out, int *outCount, int maxCount) {
    if (!text || !*text) return;
    int totalCount = 0;
    int *cps = LoadCodepoints(text, &totalCount);
    if (!cps) return;
    for (int i = 0; i < totalCount && *outCount < maxCount; i++) {
        int seen = 0;
        for (int j = 0; j < *outCount; j++) {
            if (out[j] == cps[i]) { seen = 1; break; }
        }
        if (!seen) out[(*outCount)++] = cps[i];
    }
    UnloadCodepoints(cps);
}

static void collectArabicCodepoints(AppState *state, int *out, int *outCount, int maxCount) {
    /* per-string walk — no giant concat buffer to overflow. */
    *outCount = 0;

    /* Surah arabic names */
    for (int i = 0; i < state->surahCount && *outCount < maxCount; i++)
        addCodepoints(state->surahs[i].arabicName, out, outCount, maxCount);

    /* Ayah arabic text (full 6236-ayah walk; each string is small) */
    for (int i = 0; i < state->totalAyahs && *outCount < maxCount; i++)
        addCodepoints(state->ayahs[i].arabicText, out, outCount, maxCount);

    /* FriBidi shaping converts Arabic letters to Presentation Forms (U+FE70-U+FEFF).
       We MUST also load these ranges so the shaped output has glyphs in the atlas. */
    int presentationRanges[] = {
        0xFB50, 0xFDFF,  /* Arabic Presentation Forms-A */
        0xFE70, 0xFEFF,  /* Arabic Presentation Forms-B */
        0x0600, 0x06FF,  /* Basic Arabic (fallback range) */
    };
    for (int r = 0; r < 6; r += 2) {
        for (int cp = presentationRanges[r]; cp <= presentationRanges[r+1]; cp++) {
            if (*outCount >= maxCount) break;
            int seen = 0;
            for (int j = 0; j < *outCount; j++) {
                if (out[j] == cp) { seen = 1; break; }
            }
            if (!seen) out[(*outCount)++] = cp;
        }
    }
}

void initFonts(AppState *state) {
    /* Collect codepoints including Presentation Forms that FriBidi shapes into */
    int arCodepoints[2048];
    int arCount = 0;

    if (state && state->surahs && state->ayahs) {
        collectArabicCodepoints(state, arCodepoints, &arCount, 2048);
    } else {
        /* Fallback: load core Arabic + Presentation Forms */
        for (int i = 0x0600; i <= 0x06FF; i++) arCodepoints[arCount++] = i;
        for (int i = 0x0750; i <= 0x077F; i++) arCodepoints[arCount++] = i;
        for (int i = 0x08A0; i <= 0x08FF; i++) arCodepoints[arCount++] = i;
        for (int i = 0xFB50; i <= 0xFDFF; i++) arCodepoints[arCount++] = i;
        for (int i = 0xFE70; i <= 0xFEFF; i++) arCodepoints[arCount++] = i;
    }

    printf("Loading Amiri with %d unique codepoints...\n", arCount);
    arabicFont = LoadFontEx("assets/Amiri.ttf", 96, arCodepoints, arCount);
    if (arabicFont.texture.id > 0) {
        SetTextureFilter(arabicFont.texture, TEXTURE_FILTER_BILINEAR);
        printf("Amiri loaded: %dx%d atlas, %d glyphs\n",
               arabicFont.texture.width, arabicFont.texture.height,
               arabicFont.glyphCount);
    } else {
        printf("WARNING: Amiri font failed to load, using default\n");
        arabicFont = GetFontDefault();
    }

    /* Load JetBrains Mono for UI text — Latin + punctuation + ★ */
    int uiCodepoints[320];
    int uiCount = 0;
    for (int i = 0x0020; i <= 0x007E; i++) uiCodepoints[uiCount++] = i;
    for (int i = 0x00A0; i <= 0x00FF; i++) uiCodepoints[uiCount++] = i;
    for (int i = 0x2000; i <= 0x206F; i++) uiCodepoints[uiCount++] = i;
    uiCodepoints[uiCount++] = 0x2605; /* bookmark star */

    uiFont = LoadFontEx("assets/JetBrainsMono-Regular.ttf", 96, uiCodepoints, uiCount);
    if (uiFont.texture.id > 0)
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
    else
        uiFont = GetFontDefault();

    /* Load Hind Siliguri for Bengali translations */
    int bnCodepoints[256];
    int bnCount = 0;
    for (int i = 0x0980; i <= 0x09FF; i++) bnCodepoints[bnCount++] = i;
    for (int i = 0x0020; i <= 0x007E; i++) bnCodepoints[bnCount++] = i;

    bengaliFont = LoadFontEx("assets/HindSiliguri-Regular.ttf", 96, bnCodepoints, bnCount);
    if (bengaliFont.texture.id > 0) {
        SetTextureFilter(bengaliFont.texture, TEXTURE_FILTER_BILINEAR);
        printf("Hind Siliguri loaded: %dx%d atlas, %d glyphs\n",
               bengaliFont.texture.width, bengaliFont.texture.height,
               bengaliFont.glyphCount);
    } else {
        printf("WARNING: Hind Siliguri failed to load, Bengali falls back to UI font\n");
        bengaliFont = uiFont;
    }
}

void closeFonts(void) {
    if (bengaliFont.texture.id > 0 && bengaliFont.texture.id != uiFont.texture.id &&
        bengaliFont.texture.id != arabicFont.texture.id)
        UnloadFont(bengaliFont);
    if (uiFont.texture.id > 0 && uiFont.texture.id != arabicFont.texture.id)
        UnloadFont(uiFont);
    if (arabicFont.texture.id > 0)
        UnloadFont(arabicFont);
}

/* ── Finder overlay: Surahs | Ayahs tabs (input lives in input.c) ── */
#define PALETTE_ROWS 8
#define FINDER_AYAH_ROWS 5

static void drawGoToPalette(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 180});
    int cw = (int)(560 * S.factor), ch = (int)(440 * S.factor);
    int cx = (sw - cw) / 2, cy = (sh - ch) / 2;
    DrawRectangleRounded((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                         0.06f, 8, t->surface);
    DrawRectangleRoundedLines((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                              0.06f, 8, t->border);

    int pad = (int)(24 * S.factor);
    int titleY = cy + (int)(18 * S.factor);
    DrawTextEx(uiFont, "Finder", (Vector2){(float)(cx + pad), (float)titleY}, S.fs22, 1, t->accent);

    /* Tabs, right-aligned + clickable */
    const char *tabS = "Surahs", *tabA = "Ayahs";
    float wS = MeasureTextEx(uiFont, tabS, S.fs16, 1).x;
    float wA = MeasureTextEx(uiFont, tabA, S.fs16, 1).x;
    int tgap = (int)(16 * S.factor);
    float xA = (float)(cx + cw - pad) - wA;
    float xS = xA - tgap - wS;
    int surActive = (state->paletteMode == 0);
    DrawTextEx(uiFont, tabS, (Vector2){xS, (float)(titleY + 2)}, S.fs16, 1, surActive ? t->accent : t->muted);
    DrawTextEx(uiFont, tabA, (Vector2){xA, (float)(titleY + 2)}, S.fs16, 1, surActive ? t->muted : t->accent);
    if (surActive)
        DrawRectangle((int)xS, titleY + 2 + S.fs16 + 2, (int)wS, (int)(2 * S.factor), t->accent);
    else
        DrawRectangle((int)xA, titleY + 2 + S.fs16 + 2, (int)wA, (int)(2 * S.factor), t->accent);
    Vector2 mp = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mp, (Rectangle){xS - 4, (float)titleY, wS + 8, (float)(S.fs16 + 8)})) {
            state->paletteMode = 0;
            state->paletteSelection = 0;
        } else if (CheckCollisionPointRec(mp, (Rectangle){xA - 4, (float)titleY, wA + 8, (float)(S.fs16 + 8)})) {
            state->paletteMode = 1;
            state->paletteSelection = 0;
        }
    }

    int qy = cy + (int)(54 * S.factor), qh = (int)(38 * S.factor);
    DrawRectangle(cx + pad, qy, cw - 2 * pad, qh, t->background);
    DrawRectangleLines(cx + pad, qy, cw - 2 * pad, qh, t->accent);
    char qdisp[72];
    snprintf(qdisp, sizeof(qdisp), "%s|", state->paletteQuery);
    DrawTextEx(uiFont, qdisp, (Vector2){(float)(cx + pad + S.gx), (float)(qy + (qh - S.fs18) / 2)}, S.fs18, 1, t->foreground);

    int rowsDrawn = 0;
    if (surActive) {
        static int matches[128];
        int n = paletteFilter(state, state->paletteQuery, matches, 128);
        int sel = state->paletteSelection;
        if (n == 0 || sel < 0) sel = 0;
        if (n > 0 && sel >= n) sel = n - 1;
        int start = (sel >= PALETTE_ROWS) ? sel - PALETTE_ROWS + 1 : 0;
        int rowH = (int)(36 * S.factor), ry0 = qy + qh + S.gy;
        for (int r = 0; r < PALETTE_ROWS; r++) {
            int mi = start + r;
            if (mi >= n) break;
            int ry = ry0 + r * rowH;
            if (ry + rowH > cy + ch - (int)(28 * S.factor)) break;
            rowsDrawn++;
            Surah *s = &state->surahs[matches[mi]];
            /* hover selects, click opens — mirrors the hadith list. */
            Rectangle rowR = {(float)(cx + pad), (float)ry, (float)(cw - 2 * pad), (float)(rowH - 4)};
            if (CheckCollisionPointRec(GetMousePosition(), rowR)) {
                state->paletteSelection = mi;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    openFinderMatch(state);
            }
            if (mi == sel) {
                DrawRectangleRounded((Rectangle){(float)(cx + pad), (float)ry, (float)(cw - 2 * pad), (float)(rowH - 4)},
                                     0.08f, 4, t->background);
                DrawRectangle(cx + pad, ry + 5, (int)(3 * S.factor), rowH - 14, t->accent);
            }
            char num[8];
            snprintf(num, sizeof(num), "%d", s->number);
            DrawTextEx(uiFont, num, (Vector2){(float)(cx + pad + S.gx), (float)(ry + 5)}, S.fs14, 1, t->accent);
            DrawTextEx(uiFont, s->name, (Vector2){(float)(cx + pad + (int)(56 * S.factor)), (float)(ry + 4)}, S.fs16, 1,
                       mi == sel ? t->foreground : t->muted);
            char meta[48];
            snprintf(meta, sizeof(meta), "%d ayahs, %s", s->ayahCount, s->revelationType);
            float mw = MeasureTextEx(uiFont, meta, S.fs12, 1).x;
            DrawTextEx(uiFont, meta, (Vector2){(float)(cx + cw - pad - mw), (float)(ry + 7)}, S.fs12, 1, t->muted);
        }
        if (rowsDrawn == 0)
            DrawTextEx(uiFont, "No results found.", (Vector2){(float)(cx + pad + S.gx), (float)ry0}, S.fs14, 1, t->muted);
    } else {
        int n = state->searchResultCount;
        int sel = state->paletteSelection;
        if (n == 0 || sel < 0) sel = 0;
        if (n > 0 && sel >= n) sel = n - 1;
        int start = (sel >= FINDER_AYAH_ROWS) ? sel - FINDER_AYAH_ROWS + 1 : 0;
        int rowH = (int)(58 * S.factor), ry0 = qy + qh + S.gy;
        for (int r = 0; r < FINDER_AYAH_ROWS; r++) {
            int mi = start + r;
            if (mi >= n) break;
            int ry = ry0 + r * rowH;
            if (ry + rowH > cy + ch - (int)(28 * S.factor)) break;
            rowsDrawn++;
            SearchResult *sr = &state->searchResults[mi];
            Rectangle rowR = {(float)(cx + pad), (float)ry, (float)(cw - 2 * pad), (float)(rowH - 4)};
            if (CheckCollisionPointRec(GetMousePosition(), rowR)) {
                state->paletteSelection = mi;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    openFinderMatch(state);
            }
            if (mi == sel) {
                DrawRectangleRounded((Rectangle){(float)(cx + pad), (float)ry, (float)(cw - 2 * pad), (float)(rowH - 4)},
                                     0.08f, 4, t->background);
                DrawRectangle(cx + pad, ry + 5, (int)(3 * S.factor), rowH - 14, t->accent);
            }
            char ref[32];
            snprintf(ref, sizeof(ref), "%d:%d", sr->surahNumber, sr->ayahNumber);
            DrawTextEx(uiFont, ref, (Vector2){(float)(cx + pad + S.gx), (float)(ry + 4)}, S.fs14, 1, t->accent);
            /* 48-char ellipsis — overlay rows are narrower than the old screen. */
            char preview[56];
            strncpy(preview, sr->preview, 48);
            preview[48] = '\0';
            if (strlen(sr->preview) > 48) strcat(preview, "...");
            DrawTextEx(uiFont, preview, (Vector2){(float)(cx + pad + S.gx), (float)(ry + 4 + S.fs14 + 2)}, S.fs13, 1,
                       mi == sel ? t->foreground : t->muted);
        }
        if (rowsDrawn == 0) {
            const char *msg = strlen(state->paletteQuery) < 2
                ? "Type at least 2 characters to search..."
                : "No results found.";
            DrawTextEx(uiFont, msg, (Vector2){(float)(cx + pad + S.gx), (float)ry0}, S.fs14, 1, t->muted);
        }
    }
    const char *hint = "Tab switch tab - j/k + Enter to open - Esc to close";
    float hw = MeasureTextEx(uiFont, hint, S.fs12, 1).x;
    DrawTextEx(uiFont, hint, (Vector2){(float)(cx + (cw - hw) / 2), (float)(cy + ch - S.fs12 - 8)}, S.fs12, 1, t->muted);
}

/* ── Bookmark tag editor (input lives in input.c) ── */
static void drawBookmarkEditor(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 180});
    int cw = (int)(560 * S.factor), chh = (int)(220 * S.factor);
    int cx = (sw - cw) / 2, cy = (sh - chh) / 2;
    DrawRectangleRounded((Rectangle){(float)cx, (float)cy, (float)cw, (float)chh},
                         0.06f, 8, t->surface);
    DrawRectangleRoundedLines((Rectangle){(float)cx, (float)cy, (float)cw, (float)chh},
                              0.06f, 8, t->border);

    int pad = (int)(24 * S.factor);
    int blink = (((int)(GetTime() * 2.0) % 2) == 0);
    char title[32];
    snprintf(title, sizeof(title), "Bookmark %d:%d", state->currentSurah, state->currentAyah);
    DrawTextEx(uiFont, title, (Vector2){(float)(cx + pad), (float)(cy + (int)(18 * S.factor))},
               S.fs22, 1, t->accent);

    /* Tag — single line, drop-from-front on overflow (ASCII-only input) */
    int tagY = cy + (int)(58 * S.factor);
    DrawTextEx(uiFont, "Tag", (Vector2){(float)(cx + pad), (float)tagY}, S.fs14, 1, t->muted);
    int tagBoxY = tagY + S.fs14 + S.gy / 2, tagBoxH = (int)(36 * S.factor);
    DrawRectangle(cx + pad, tagBoxY, cw - 2 * pad, tagBoxH, t->background);
    DrawRectangleLines(cx + pad, tagBoxY, cw - 2 * pad, tagBoxH, t->accent);
    {
        char disp[160];
        snprintf(disp, sizeof(disp), "%s%s", getBookmarkTag(), blink ? "|" : "");
        int maxTW = cw - 2 * pad - 2 * S.gx;
        const char *vis = disp;
        while (vis[0] && MeasureTextEx(uiFont, vis, S.fs14, 1).x > maxTW) vis++;
        DrawTextEx(uiFont, vis, (Vector2){(float)(cx + pad + S.gx), (float)(tagBoxY + (tagBoxH - S.fs14) / 2)},
                   S.fs14, 1, t->foreground);
    }

    const char *hint = "Type a tag   Enter save   Esc cancel";
    float hw = MeasureTextEx(uiFont, hint, S.fs12, 1).x;
    DrawTextEx(uiFont, hint, (Vector2){(float)(cx + (cw - hw) / 2), (float)(cy + chh - S.fs12 - 8)},
               S.fs12, 1, t->muted);
}

void drawCurrentScreen(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawDashboard(state);
            break;
        case SCREEN_SURAH_LIST:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawSurahList(state);
            break;
        case SCREEN_AYAH_READER:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawAyahReader(state);
            break;
        case SCREEN_BOOKMARKS:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawBookmarks(state);
            break;
        case SCREEN_SCREENSAVER: {
            /* systems owns the visuals + Azan; ui keeps no fork. */
            drawScreensaver(state);
            break;
        }
        case SCREEN_SURAH_OVERVIEW:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawSurahOverview(state);
            break;
        case SCREEN_SETTINGS:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawSettings(state);
            break;
        case SCREEN_READING_HUB:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawReadingHub(state);
            break;
        case SCREEN_HADITH:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawHadithPage(state);
            break;
    }
    /* palette floats above any screen; help draws above it in main. */
    if (state->showGoToPalette) drawGoToPalette(state);
    if (isEditingBookmark()) drawBookmarkEditor(state);
}

static void nextPrayerInfo(AppState *state, char *name, int nameSz, char *countdown, int cdSz, float *progress);

static Rectangle cardRect(int index) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int mx = S.mx, my = S.my, gx = S.gx, gy = S.gy;
    int ch = sh - TOPBAR_H - FOOTER_H;
    int fullW = sw - 2 * mx;
    int halfW = (fullW - gx) / 2;
    int y0 = TOPBAR_H + my;
    /* Rhythm: top row 28%, middle hero 38%, bottom row fills remaining */
    int topH = (int)(ch * 0.28f);
    int midH = (int)(ch * 0.38f);
    int botH = (sh - FOOTER_H - gy) - (y0 + topH + midH + 2 * gy);
    /* extremes guard — short windows collapse, never invert. */
    if (topH < 24) topH = 24;
    if (midH < 24) midH = 24;
    if (botH < 24) botH = 24;
    switch (index) {
        case 0: return (Rectangle){(float)mx, (float)y0, (float)halfW, (float)topH};
        case 1: return (Rectangle){(float)(mx + halfW + gx), (float)y0, (float)halfW, (float)topH};
        case 2: return (Rectangle){(float)mx, (float)(y0 + topH + gy), (float)fullW, (float)midH};
        case 3: return (Rectangle){(float)mx, (float)(y0 + topH + midH + 2*gy), (float)halfW, (float)botH};
        case 4: return (Rectangle){(float)(mx + halfW + gx), (float)(y0 + topH + midH + 2*gy), (float)halfW, (float)botH};
        default: return (Rectangle){0, 0, 0, 0};
    }
}

static double bookmarkPopupTime = 0;
static char bookmarkPopupMsg[32] = "Bookmark saved";

/* delight — the toast names what was saved. */
void showBookmarkPopupRef(int surah, int ayah) {
    snprintf(bookmarkPopupMsg, sizeof(bookmarkPopupMsg), "Bookmarked %d:%d", surah, ayah);
    bookmarkPopupTime = GetTime();
}

/* Forward declaration needed because drawDashboard calls reorderArabic before its definition */
static int reorderArabic(const char *text, char *visualOut, int outSize);

static int surahIndexByNumber(AppState *state, int surahNum) {
    if (!state || !state->surahs) return -1;
    for (int i = 0; i < state->surahCount; i++)
        if (state->surahs[i].number == surahNum) return i;
    return -1;
}

static Surah *surahByNumber(AppState *state, int surahNum) {
    int i = surahIndexByNumber(state, surahNum);
    return (i >= 0) ? &state->surahs[i] : NULL;
}

static int isBookmarked(int surah, int ayah) {
    return bookmarkExists(surah, ayah);
}

static long bookmarkTimestamp(int surah, int ayah) {
    /* direct DB scan; bookmark lists are tiny. */
    Bookmark rows[256];
    int n = loadBookmarks(rows, 256);
    for (int i = 0; i < n; i++)
        if (rows[i].surahNumber == surah && rows[i].ayahNumber == ayah)
            return rows[i].timestamp;
    return 0;
}

static const char *formatRelativeTime(long timestamp) {
    static char buf[32];
    if (timestamp <= 0) return "bookmarked";
    long diff = (long)time(NULL) - timestamp;
    if (diff < 0) return "just now";
    if (diff < 60) return "just now";
    if (diff < 3600) {
        int m = (int)(diff / 60);
        snprintf(buf, sizeof(buf), "%dm ago", m);
        return buf;
    }
    if (diff < 86400) {
        int h = (int)(diff / 3600);
        snprintf(buf, sizeof(buf), "%dh ago", h);
        return buf;
    }
    int d = (int)(diff / 86400);
    snprintf(buf, sizeof(buf), "%dd ago", d);
    return buf;
}

/* wrapped text with pixel scroll offset; returns total content height. */
static int drawWrappedTextScroll(const char *text, Rectangle bounds, Font f, int fontSize, Color color, int yOff) {
    if (!text || !*text) return 0;
    /* 1.375 leading for body copy — translations and hadith read long. */
    int lineH = fontSize + (fontSize >= 16 ? 6 : 4);
    int totalH = 0;
    const char *lineStart = text;

    while (*lineStart) {
        char lineBuf[2048];
        const char *p = lineStart;
        const char *lastSpace = NULL;
        int flushed = 0;

        while (*p && *p != '\n') {
            int len = (int)(p - lineStart + 1);
            if (len >= (int)sizeof(lineBuf) - 1) len = (int)sizeof(lineBuf) - 1;

            memcpy(lineBuf, lineStart, (size_t)len);
            lineBuf[len] = '\0';
            float w = MeasureTextEx(f, lineBuf, (float)fontSize, 1).x;

            if (w > bounds.width && len > 1) {
                int flushLen;
                const char *nextStart;
                if (lastSpace) {
                    flushLen = (int)(lastSpace - lineStart);
                    nextStart = lastSpace + 1;
                } else {
                    flushLen = (int)(p - lineStart);
                    nextStart = p;
                }
                memcpy(lineBuf, lineStart, (size_t)flushLen);
                lineBuf[flushLen] = '\0';
                int ly = (int)bounds.y + totalH - yOff;
                if (ly + fontSize > bounds.y && ly < bounds.y + bounds.height)
                    DrawTextEx(f, lineBuf, (Vector2){bounds.x, (float)ly}, (float)fontSize, 1, color);
                totalH += lineH;
                lineStart = nextStart;
                flushed = 1;
                break;
            }

            if (*p == ' ') lastSpace = p;
            p++;
        }

        if (!flushed) {
            /* cap like the measure path — a spaceless run can't smash the stack. */
            int len = (int)(p - lineStart);
            if (len > (int)sizeof(lineBuf) - 1) len = (int)sizeof(lineBuf) - 1;
            if (len > 0) {
                memcpy(lineBuf, lineStart, (size_t)len);
                lineBuf[len] = '\0';
                int ly = (int)bounds.y + totalH - yOff;
                if (ly + fontSize > bounds.y && ly < bounds.y + bounds.height)
                    DrawTextEx(f, lineBuf, (Vector2){bounds.x, (float)ly}, (float)fontSize, 1, color);
                totalH += lineH;
            }
            if (*p == '\n') lineStart = p + 1;
            else break;
        }
    }
    return totalH;
}

static void drawWrappedText(const char *text, Rectangle bounds, int fontSize, Color color) {
    drawWrappedTextScroll(text, bounds, uiFont, fontSize, color, 0);
}

/* Bengali translations render in Hind Siliguri, all else in UI font. */
static Font trFont(AppState *state) {
    if (strcmp(state->language, "bn") == 0 && bengaliFont.texture.id > 0)
        return bengaliFont;
    return uiFont;
}

void drawDashboard(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    drawTopBar(state);
    int mx = S.mx, gx = S.gx;
    int fullW = sw - 2 * mx;
    int halfW = (fullW - gx) / 2;

    Rectangle cards[5];
    for (int i = 0; i < 5; i++) {
        cards[i] = cardRect(i);
        DrawRectangleRounded(cards[i], 0.06f, 8, t->surface);
        if (state->dashboardCursor == i)
            DrawRectangleRoundedLinesEx(cards[i], 0.06f, 8, 2, t->accent);
        else
            DrawRectangleRoundedLinesEx(cards[i], 0.06f, 8, 1, t->border);
    }
    /* GREETING */
    {
        Rectangle r = cards[0];
        int px = r.x + S.cardPadX + S.gx, py = r.y + S.cardPadY;
        time_t now = time(NULL);
        struct tm *lt = localtime(&now);
        const char *greet = "Good evening";
        if (lt->tm_hour >= 5 && lt->tm_hour < 12) greet = "Good morning";
        else if (lt->tm_hour >= 12 && lt->tm_hour < 17) greet = "Good afternoon";
        DrawTextEx(uiFont, greet, (Vector2){(float)(px), (float)(py)}, S.fs22, 1, t->foreground); py += S.fs22 + S.gy;
        char sub[128];
        Surah *cur = surahByNumber(state, state->currentSurah);
        snprintf(sub, sizeof(sub), "You're in %s", cur ? cur->name : "Ayatika");
        DrawTextEx(uiFont, sub, (Vector2){(float)(px), (float)(py)}, S.fs14, 1, t->muted);
    }
    /* PRAYER */
    {
        Rectangle r = cards[1];
        int px = r.x + S.cardPadX + S.gx, py = r.y + S.cardPadY;
        char name[32], cd[16]; float prog;
        nextPrayerInfo(state, name, sizeof(name), cd, sizeof(cd), &prog);
        char line[64];
        snprintf(line, sizeof(line), "%s — %s", name, cd);
        DrawTextEx(uiFont, line, (Vector2){(float)(px), (float)(py)}, S.fs22, 1, t->foreground); py += S.fs22 + S.gy;
        int barW = halfW - 2 * (S.cardPadX + S.gx);
        DrawRectangleRounded((Rectangle){(float)px, (float)py, (float)barW, (float)S.progressH}, 0.3f, 4, t->border);
        DrawRectangleRounded((Rectangle){(float)px, (float)py, (float)barW * prog, (float)S.progressH}, 0.3f, 4, t->accent);
        py += S.progressGap;
        snprintf(line, sizeof(line), "Fajr %s    Dhuhr %s", state->prayer.fajrStr, state->prayer.dhuhrStr);
        DrawTextEx(uiFont, line, (Vector2){(float)(px), (float)(py)}, S.fs12, 1, t->muted); py += S.fs12 + S.gy/2;
        snprintf(line, sizeof(line), "Asr %s    Maghrib %s", state->prayer.asrStr, state->prayer.maghribStr);
        DrawTextEx(uiFont, line, (Vector2){(float)(px), (float)(py)}, S.fs12, 1, t->muted); py += S.fs12 + S.gy/2;
        snprintf(line, sizeof(line), "Isha %s", state->prayer.ishaStr);
        DrawTextEx(uiFont, line, (Vector2){(float)(px), (float)(py)}, S.fs12, 1, t->muted);
    }
    /* AYAH OF THE DAY */
    {
        Rectangle r = cards[2];
        int innerX = (int)(r.x + S.cardPadX + S.gx);
        int innerW = fullW - 2 * (S.cardPadX + S.gx);
        int innerY = (int)(r.y + S.cardPadY);
        int innerH = (int)(r.height - 2 * S.cardPadY);

        int da = getDailyAyahIndex(state->totalAyahs);
        Ayah *a = NULL;
        if (da < state->totalAyahs) a = &state->ayahs[da];
        if (a) {
            Surah *surah = surahByNumber(state, a->surahNumber);

            /* ── Header row: title + reference pill ── */
            DrawTextEx(uiFont, "Ayah of the Day",
                       (Vector2){(float)innerX, (float)innerY},
                       S.fs16, 1, t->accent);
            char refPill[64];
            snprintf(refPill, sizeof(refPill), "%d:%d", a->surahNumber, a->ayahNumber);
            float pillW = MeasureTextEx(uiFont, refPill, S.fs12, 1).x + (int)(12 * S.factor);
            float pillX = r.x + r.width - S.cardPadX - S.gx - pillW;
            DrawRectangleRounded((Rectangle){pillX, (float)innerY, pillW, (float)(S.fs14)},
                                 0.3f, 4, t->accent);
            DrawTextEx(uiFont, refPill,
                       (Vector2){pillX + 6 * S.factor, (float)innerY + 1},
                       S.fs12, 1, t->background);

            /* ── Top separator ── */
            int sep1Y = innerY + S.fs16 + S.gy/2;
            DrawLine(innerX, sep1Y, (int)(r.x + r.width - S.cardPadX - S.gx), sep1Y, t->border);

            /* ── Arabic text — hero display size, wrapped block, then separator ── */
            int arabicTop = sep1Y + S.gy/2;
            float arabicMaxH = (float)innerH * 0.55f;

            char visual[8192];
            int haveVisual = reorderArabic(a->arabicText, visual, sizeof(visual));
            Font af = arabicFont.texture.id > 0 ? arabicFont : uiFont;
            float arUsed = drawArabicWrapped(af, haveVisual ? visual : a->arabicText,
                (float)(innerX + innerW), (float)arabicTop,
                (float)innerW, arabicMaxH, (float)S.fs48, (float)S.fs14, t->foreground);

            /* ── Bottom separator ── */
            int sep2Y = arabicTop + (int)arUsed + S.gy/2;
            DrawLine(innerX, sep2Y, (int)(r.x + r.width - S.cardPadX - S.gx), sep2Y, t->border);

            /* ── Full translation (wrapped, not truncated) ── */
            int transTop = sep2Y + S.gy/2;
            int transBottom = (int)(r.y + r.height - S.cardPadY - S.fs13 - S.gy);
            char *translation = (strcmp(state->language, "bn") == 0)
                                ? a->translationBn : a->translationEn;
            drawWrappedTextScroll(translation,
                (Rectangle){(float)innerX, (float)transTop,
                            (float)innerW, (float)(transBottom - transTop)},
                trFont(state), S.fs14, t->muted, 0);

            /* ── Footer reference line ── */
            char footerRef[128];
            if (surah)
                snprintf(footerRef, sizeof(footerRef), "%s (%d) — Ayah %d of %d",
                         surah->name, surah->number, a->ayahNumber, surah->ayahCount);
            else
                snprintf(footerRef, sizeof(footerRef), "%d:%d",
                         a->surahNumber, a->ayahNumber);
            float fw = MeasureTextEx(uiFont, footerRef, S.fs12, 1).x;
            DrawTextEx(uiFont, footerRef,
                       (Vector2){(float)(r.x + r.width - S.cardPadX - S.gx - fw),
                                 (float)(r.y + r.height - S.cardPadY - S.fs12)},
                       S.fs12, 1, t->muted);
        } else {
            const char *msg = "No ayah available for today.";
            int tw = MeasureTextEx(uiFont, msg, S.fs16, 1).x;
            DrawTextEx(uiFont, msg,
                       (Vector2){(float)((sw - tw) / 2),
                                 (float)(r.y + r.height / 2 - S.fs16 / 2)},
                       S.fs16, 1, t->muted);
        }
    }
    /* HADITH */
    {
        Rectangle r = cards[3];
        int px = r.x + S.cardPadX + S.gx, py = r.y + S.cardPadY;
        int innerW = halfW - 2 * (S.cardPadX + S.gx);
        if (state->totalHadiths > 0 && state->hadiths) {
            time_t _ht = time(NULL);
            struct tm *_htm = localtime(&_ht);
            int _hi = _htm ? (_htm->tm_yday % state->totalHadiths) : 0;
            if (_hi < 0) _hi = 0;
            Hadith *h = &state->hadiths[_hi];
            DrawTextEx(uiFont, h->name, (Vector2){(float)(px), (float)(py)}, S.fs13, 1, t->muted); py += S.fs13 + S.gy/2;
            /* word-boundary truncate + wrap in card bounds; collection pinned. */
            char txt[180];
            {
                const char *src = h->text ? h->text : "";
                int lim = (int)sizeof(txt) - 4;
                int len = (int)strlen(src);
                if (len <= lim) {
                    snprintf(txt, sizeof(txt), "%s", src);
                } else {
                    int cut = lim;
                    while (cut > 40 && src[cut] != ' ') cut--;
                    if (cut <= 40) cut = lim;
                    memcpy(txt, src, (size_t)cut);
                    txt[cut] = '\0';
                    strcat(txt, "...");
                }
            }
            int footH = S.fs12 + S.gy / 2;
            int textBottom = (int)(r.y + r.height - S.cardPadY - footH - S.gy / 2);
            drawWrappedText(txt, (Rectangle){(float)px, (float)py, (float)innerW,
                                             (float)(textBottom - py)},
                            S.fs14, t->foreground);
            float tw = MeasureTextEx(uiFont, h->collection, S.fs12, 1).x;
            DrawTextEx(uiFont, h->collection,
                       (Vector2){(float)(r.x + r.width - S.cardPadX - S.gx - tw),
                                 (float)(r.y + r.height - S.cardPadY - S.fs12)},
                       S.fs12, 1, t->accent);
        }
    }
    /* CONTINUE READING */
    {
        Rectangle r = cards[4];
        int px = r.x + S.cardPadX + S.gx, py = r.y + S.cardPadY;
        int sn = state->currentSurah, an = state->currentAyah;
        char ln[96];
        Surah *cs = surahByNumber(state, sn);
        if (cs) snprintf(ln, sizeof(ln), "%s, Ayah %d", cs->name, an);
        else snprintf(ln, sizeof(ln), "Surah %d, Ayah %d", sn, an);
        DrawTextEx(uiFont, ln, (Vector2){(float)(px), (float)(py)}, S.fs22, 1, t->accent); py += S.fs22 + S.gy;
        long ts = bookmarkTimestamp(state->currentSurah, state->currentAyah);
        DrawTextEx(uiFont, formatRelativeTime(ts), (Vector2){(float)(px), (float)(py)}, S.fs12, 1, t->muted); py += S.fs12 + S.gy/2;
        DrawTextEx(uiFont, "Press Enter to resume", (Vector2){(float)(px), (float)(py)}, S.fs16, 1, t->foreground);
    }
    drawFooter(state);
}

void drawSurahList(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    drawTopBar(state);
    drawSidebar(state);
    int px = SIDEBAR_W + S.gx;
    DrawTextEx(uiFont, "Select a surah to begin reading", (Vector2){(float)(px), (float)(TOPBAR_H + S.my/2)}, S.fs18, 1, t->muted);
    /* live digit-jump buffer; letters detour to the palette. */
    const char *jump = getListJumpBuf();
    if (jump[0]) {
        char line[32];
        snprintf(line, sizeof(line), "Go: %s|", jump);
        DrawTextEx(uiFont, line, (Vector2){(float)(px), (float)(TOPBAR_H + S.my/2 + S.fs18 + S.gy/2)}, S.fs16, 1, t->accent);
    }
    drawFooter(state);
}

/* ── Helper: draw ayah content (text + translation + ref) ── */
/* raylib advances the pen per codepoint — including zero-advance
   tashkeel marks (by bitmap width), which spreads Arabic apart and breaks
   joins. These draw marks overstruck with no advance so bases join; bases
   keep raylib's own advance rule. Draw and measure must stay paired. */
static int isArabicMark(int cp) {
    return (cp >= 0x064B && cp <= 0x0655) || cp == 0x0670 ||
           (cp >= 0x06D6 && cp <= 0x06ED);
}

static float shapedGlyphAdvance(Font af, int idx, float scale, float sp, int mark) {
    if (mark) return 0;
    float adv = (float)af.glyphs[idx].advanceX;
    if (adv == 0) adv = (float)af.recs[idx].width;
    return adv * scale + sp;
}

static float measureShaped(Font af, const char *text, float size, float sp) {
    if (af.baseSize <= 0) return 0;
    float scale = size / (float)af.baseSize;
    float w = 0;
    int i = 0;
    while (text[i]) {
        int bytes = 0;
        int cp = GetCodepointNext(text + i, &bytes);
        w += shapedGlyphAdvance(af, GetGlyphIndex(af, cp), scale, sp, isArabicMark(cp));
        i += bytes > 0 ? bytes : 1;
    }
    return w;
}

static void drawShaped(Font af, const char *text, Vector2 pos, float size, float sp, Color color) {
    if (af.baseSize <= 0 || af.texture.id <= 0) return;
    float scale = size / (float)af.baseSize;
    float x = pos.x;
    int i = 0;
    while (text[i]) {
        int bytes = 0;
        int cp = GetCodepointNext(text + i, &bytes);
        int idx = GetGlyphIndex(af, cp);
        DrawTextureRec(af.texture,
            (Rectangle){af.recs[idx].x, af.recs[idx].y, af.recs[idx].width, af.recs[idx].height},
            (Vector2){x + (float)af.glyphs[idx].offsetX * scale,
                      pos.y + (float)af.glyphs[idx].offsetY * scale},
            color);
        x += shapedGlyphAdvance(af, idx, scale, sp, isArabicMark(cp));
        i += bytes > 0 ? bytes : 1;
    }
}

/* one visual-order wrapper for every shaped-Arabic block — lines
   flow top-to-bottom, each right-aligned; the block shrinks until it fits
   maxH. Line spans reference a static arena, valid until the next call. */
#define AWRAP_MAX_LINES 96
static char awArena[8192];
static int awStart[AWRAP_MAX_LINES + 1];
static int awEnd[AWRAP_MAX_LINES + 1];
static char awLine[8192];

/* Break visual-order text into lines <= maxW. Returns line count. */
static int awBreak(const char *visual, Font af, float size, float maxW) {
    float sp = size * 0.12f;
    int n = 0, start = 0, pos = 0;
    awStart[0] = 0;
    const char *p = visual;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p || n >= AWRAP_MAX_LINES) break;
        const char *w = p;
        while (*p && *p != ' ') p++;
        int wlen = (int)(p - w);
        int tpos = pos; /* tentative append: space + word */
        if (tpos > start) awArena[tpos++] = ' ';
        if (tpos + wlen >= (int)sizeof(awArena) - 1) break; /* arena full */
        memcpy(awArena + tpos, w, (size_t)wlen); tpos += wlen;
        awArena[tpos] = '\0';
        if (measureShaped(af, awArena + start, size, sp) > maxW && pos > start) {
            awEnd[n] = pos;      /* flush current line, word moves down */
            n++;
            awStart[n] = pos;
            start = pos;
            memcpy(awArena + pos, w, (size_t)wlen); pos += wlen; /* accept overflow */
            awArena[pos] = '\0';
        } else pos = tpos;
    }
    awArena[pos] = '\0';
    if (pos <= start) return n; /* empty input (or only spaces) */
    awEnd[n] = pos;
    return n + 1;
}

/* Draw the wrapped block right-aligned at (xRight, yTop). Returns height used.
   Result is cached: same text + metrics skips the re-break every frame. */
static float drawArabicWrapped(Font af, const char *visual, float xRight, float yTop,
                               float maxW, float maxH, float size0, float sizeMin, Color color) {
    if (!visual || !*visual || maxW <= 0 || maxH <= 0) return 0;
    /* FNV-1a over the text + metrics; re-break only on change. */
    unsigned h = 2166136261u;
    for (const char *p = visual; *p; p++) { h ^= (unsigned char)*p; h *= 16777619u; }
    h ^= (unsigned)maxW * 31u + (unsigned)(size0 * 4) * 131u + (unsigned)(sizeMin * 4);
    static unsigned cacheKey = 0;
    static int cacheN = 0;
    static float cacheSize = 0;
    float size;
    int n;
    if (h == cacheKey && cacheN > 0) {
        size = cacheSize;
        n = cacheN;
    } else {
        size = size0;
        n = 0;
        for (;;) {
            n = awBreak(visual, af, size, maxW);
            float lh = size + (int)(8 * S.factor);
            if (n <= 0 || n * lh <= maxH || size <= sizeMin) break;
            size -= 2;
            if (size < sizeMin) size = sizeMin;
        }
        cacheKey = h;
        cacheN = n;
        cacheSize = size;
    }
    float lh = size + (int)(8 * S.factor);
    float sp = size * 0.12f;
    for (int i = 0; i < n; i++) {
        int len = awEnd[i] - awStart[i];
        if (len <= 0) continue;
        memcpy(awLine, awArena + awStart[i], (size_t)len);
        awLine[len] = '\0';
        float w = measureShaped(af, awLine, size, sp);
        drawShaped(af, awLine, (Vector2){xRight - w, yTop + i * lh}, size, sp, color);
    }
    return n * lh;
}

static void drawAyahContent(AppState *state, Theme *t, int sw, int sh) {
    int mx = SIDEBAR_W + S.mx;
    int my = TOPBAR_H + S.my;
    int mainW = sw - SIDEBAR_W - 2 * S.mx;

    Ayah *ayah = getAyah(state, state->currentSurah, state->currentAyah);
    if (!ayah) {
        const char *msg = "No ayah loaded for this reference";
        int tw = MeasureTextEx(uiFont, msg, S.fs16, 1).x;
        DrawTextEx(uiFont, msg, (Vector2){(float)((sw - tw) / 2), (float)(sh / 2)}, S.fs16, 1, t->muted);
        return;
    }

    int rightMargin = sw - S.mx;
    int starInset = 0;
    const char *star = "\xe2\x98\x85";
    float starW = MeasureTextEx(uiFont, star, (float)S.fs20, 1).x;
    if (isBookmarked(state->currentSurah, state->currentAyah)) {
        DrawTextEx(uiFont, star, (Vector2){(float)(rightMargin - starW), (float)my}, S.fs20, 1, t->accent);
        starInset = (int)(starW + S.gx);
    }

    /* Arabic — wrapped block, right-aligned; translation flows below it. */
    char vis[8192];
    int haveVisual = reorderArabic(ayah->arabicText, vis, sizeof(vis));
    Font af = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    float arMaxH = (float)(sh - FOOTER_H - my - S.fs13 - S.gy - (S.fs16 + 4) * 3 - S.gy);
    float arUsed = drawArabicWrapped(af, haveVisual ? vis : ayah->arabicText,
        (float)(rightMargin - starInset), (float)my,
        (float)(mainW - starInset), arMaxH, (float)S.fs40, (float)S.fs14, t->foreground);

    char *translation = (strcmp(state->language, "bn") == 0)
                        ? ayah->translationBn : ayah->translationEn;
    /* cap measure near 75ch so wide windows stay readable. */
    int wrapW = mainW;
    int maxMeasure = (int)(720 * S.factor);
    if (wrapW > maxMeasure) wrapW = maxMeasure;
    int trTop = my + (int)arUsed + S.gy;
    float trH = (float)(sh - FOOTER_H - S.fs13 - S.gy) - trTop;
    if (trH < 0) trH = 0;
    drawWrappedTextScroll(translation,
                    (Rectangle){(float)mx, (float)trTop,
                                (float)wrapW, trH},
                    trFont(state), S.fs16, t->muted, 0);

    char ref[32];
    snprintf(ref, sizeof(ref), "%d:%d", state->currentSurah, state->currentAyah);
    DrawTextEx(uiFont, ref, (Vector2){(float)(mx), (float)(sh - FOOTER_H - S.fs13 - S.gy/2)}, S.fs13, 1, t->muted);
}

/* ── Cinematic focus mode: blur + modal ── */
static void drawFocusCinematic(AppState *state, Theme *t, int sw, int sh) {
    ensureFocusTexture();

    /* Record start time on first frame of focus mode */
    if (!focusWasActive && state->focusMode) {
        focusStartTime = GetTime();
    }
    focusWasActive = state->focusMode;

    /* 1. Render full scene to texture (cached 1s — backdrop is near-static) */
    static double lastBlurTime = -10.0;
    static int lastBlurW = 0, lastBlurH = 0;
    if (focusTarget.texture.id > 0 &&
        (GetTime() - lastBlurTime > 1.0 || !focusWasActive ||
         sw != lastBlurW || sh != lastBlurH)) {
        lastBlurTime = GetTime();
        lastBlurW = sw; lastBlurH = sh;
        BeginTextureMode(focusTarget);
            ClearBackground(t->background);
            drawTopBar(state);
            drawSidebar(state);
            drawFooter(state);
            drawAyahContent(state, t, sw, sh);
        EndTextureMode();
    }

    /* 2. Box blur: cached texture drawn 9 times at low alpha, every frame */
    if (focusTarget.texture.id > 0) {
        float blurRadius = 3.0f;
        float alpha = 1.0f / 9.0f;
        for (int bx = -1; bx <= 1; bx++) {
            for (int by = -1; by <= 1; by++) {
                float ox = bx * blurRadius;
                float oy = by * blurRadius;
                DrawTextureRec(focusTarget.texture,
                    (Rectangle){0, 0, (float)sw, (float)-sh},
                    (Vector2){ox, oy},
                    Fade(WHITE, alpha));
            }
        }
    }

    /* 3. Dark scrim */
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 200});

    /* 4. Animation: ease-out scale + fade over 0.3s */
    double elapsed = GetTime() - focusStartTime;
    float progress = (elapsed < 0.3) ? (float)(elapsed / 0.3) : 1.0f;
    /* ease-out cubic */
    float eased = 1.0f - (1.0f - progress) * (1.0f - progress) * (1.0f - progress);
    float modalAlpha = eased;
    float modalScale = 0.85f + 0.15f * eased;

    /* 5. Modal card */
    int cw = (int)(700 * S.factor);
    int ch = (int)(400 * S.factor);
    int cx = (sw - cw) / 2;
    int cy = (sh - ch) / 2;

    /* Apply scale transform around center */
    float oldAlpha = (float)t->surface.a;
    t->surface.a = (unsigned char)(modalAlpha * 255);

    Vector2 center = {(float)(cx + cw / 2), (float)(cy + ch / 2)};
    Rectangle scaledRect = {
        center.x - (cw / 2) * modalScale,
        center.y - (ch / 2) * modalScale,
        cw * modalScale,
        ch * modalScale
    };

    DrawRectangleRounded(scaledRect, 0.06f, 8, t->surface);
    t->surface.a = (unsigned char)(modalAlpha * 255 * 0.3f);
    DrawRectangleRoundedLines(scaledRect, 0.06f, 8, t->accent);
    t->surface.a = oldAlpha;

    /* Content (only draw when mostly faded in) */
    if (progress > 0.3f) {
        Ayah *ayah = getAyah(state, state->currentSurah, state->currentAyah);
        if (ayah) {
            /* S.* already scaled — no second multiply. */
            float innerPad = scaledRect.x + S.mx;
            float innerW = scaledRect.width - 2 * S.mx;
            float innerTop = scaledRect.y + S.cardPadY;
            float innerBottom = scaledRect.y + scaledRect.height - S.cardPadY;

            /* ── Arabic text — wrapped block, translation flows below it ── */
            char vis[8192];
            int haveVisual = reorderArabic(ayah->arabicText, vis, sizeof(vis));
            Font af = arabicFont.texture.id > 0 ? arabicFont : uiFont;
            float arMaxH = (innerBottom - innerTop) * 0.45f;
            float arY = innerTop + (innerBottom - innerTop) * 0.10f;
            float arUsed = drawArabicWrapped(af, haveVisual ? vis : ayah->arabicText,
                innerPad + innerW, arY, innerW, arMaxH,
                (float)S.fs42, (float)S.fs14, t->foreground);

            /* ── Translation — wrapped below the Arabic ── */
            char *translation = (strcmp(state->language, "bn") == 0)
                                ? ayah->translationBn : ayah->translationEn;
            float refH = (float)S.fs14;
            float hintH = (float)S.fs12;
            float trTop = arY + arUsed + S.gy;
            float trBottom = innerBottom - (refH + S.gy + hintH + S.gy);
            drawWrappedTextScroll(translation,
                (Rectangle){innerPad, trTop, innerW, trBottom - trTop},
                trFont(state), S.fs16, t->muted, 0);

            /* ── Reference + hint (pinned to bottom) ── */
            char ref[128];
            Surah *s = surahByNumber(state, state->currentSurah);
            if (s) snprintf(ref, sizeof(ref), "%s: %d", s->name, state->currentAyah);
            else snprintf(ref, sizeof(ref), "%d:%d", state->currentSurah, state->currentAyah);
            float refW = MeasureTextEx(uiFont, ref, refH, 1).x;
            float refY = innerBottom - (refH + S.gy + hintH);
            DrawTextEx(uiFont, ref, (Vector2){center.x - refW / 2, refY}, refH, 1, t->accent);

            const char *hint = "Press F to exit";
            float hintW = MeasureTextEx(uiFont, hint, hintH, 1).x;
            DrawTextEx(uiFont, hint, (Vector2){center.x - hintW / 2,
                       refY + refH + S.gy}, hintH, 1, t->muted);
        }
    }
}

void drawAyahReader(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (state->focusMode) {
        drawFocusCinematic(state, t, sw, sh);
    } else {
        drawTopBar(state);
        drawSidebar(state);
        drawFooter(state);
        drawAyahContent(state, t, sw, sh);
        /* Reset focus start time when exiting focus mode */
        if (focusWasActive) focusStartTime = 0;
        focusWasActive = 0;
    }
}

void drawBookmarks(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    drawTopBar(state);
    drawFooter(state);

    int listY = TOPBAR_H;
    int listH = sh - TOPBAR_H - FOOTER_H;

    /* Header */
    int titleY = listY + (S.my - S.fs22) / 2;
    DrawTextEx(uiFont, "Bookmarks", (Vector2){(float)(S.mx), (float)(titleY)}, S.fs22, 1, t->foreground);
    char countStr[32];
    /* direct DB read each frame; lists are tiny. */
    static Bookmark bmRows[256];
    int bmCount = loadBookmarks(bmRows, 256);
    snprintf(countStr, sizeof(countStr), "%d saved", bmCount);
    DrawTextEx(uiFont, countStr, (Vector2){(float)(S.mx + S.fs22 + S.gx), (float)(titleY + (S.fs22 - S.fs14) / 2)}, S.fs14, 1, t->muted);

    int headerH = S.my + S.fs22 + S.gy/2;
    listY += headerH;
    listH -= headerH;

    DrawLine(S.mx, listY, sw - S.mx, listY, t->border);
    listY += S.gy/2;
    listH -= S.gy/2;

    if (bmCount == 0) {
        const char *msg = "No bookmarks yet — press b while reading to save your place.";
        int tw = MeasureTextEx(uiFont, msg, S.fs16, 1).x;
        DrawTextEx(uiFont, msg, (Vector2){(float)((sw - tw) / 2), (float)(listY + listH / 2 - S.fs16/2)}, S.fs16, 1, t->muted);
        return;
    }

    int rowH = S.bookmarkRowH;
    int visible = listH / rowH;
    /* cursor lives in state (shared with input.c), scroll stays local. */
    int bmCursor = state->cursorSurah;
    static int bmScrollOff = 0;

    if (bmCursor < 0) bmCursor = 0;
    if (bmCount > 0 && bmCursor >= bmCount) bmCursor = bmCount - 1;
    state->cursorSurah = bmCursor; /* write back so Enter/d act on it. */
    if (bmCursor < bmScrollOff) bmScrollOff = bmCursor;
    if (bmCursor >= bmScrollOff + visible) bmScrollOff = bmCursor - visible + 1;
    if (bmCount <= visible) bmScrollOff = 0;
    else if (bmScrollOff > bmCount - visible) bmScrollOff = bmCount - visible;
    if (bmScrollOff < 0) bmScrollOff = 0;

    for (int i = bmScrollOff; i < bmCount && i < bmScrollOff + visible; i++) {
        int y = listY + (i - bmScrollOff) * rowH;
        int active = (i == bmCursor);

        if (active) {
            DrawRectangle(S.gx, y, sw - 2*S.gx, rowH, t->surface);
            DrawRectangleLines(S.gx, y, sw - 2*S.gx, rowH, t->border);
        }
        DrawLine(S.mx, y + rowH, sw - S.mx, y + rowH, t->border);

        Bookmark *bm = &bmRows[i];

        char ref[32];
        snprintf(ref, sizeof(ref), "%d:%d", bm->surahNumber, bm->ayahNumber);
        DrawTextEx(uiFont, ref, (Vector2){(float)(S.mx), (float)(y + S.gy/2)}, S.fs16, 1, t->accent);

        const char *tag = bm->tag[0] ? bm->tag : "(untagged)";
        Color tagColor = bm->tag[0] ? t->foreground : t->muted;
        int tagX = (int)(120 * S.factor);
        DrawTextEx(uiFont, tag, (Vector2){(float)(tagX), (float)(y + S.gy/2)}, S.fs16, 1, tagColor);

        const char *relTime = formatRelativeTime(bm->timestamp);
        float rtW = MeasureTextEx(uiFont, relTime, S.fs12, 1).x;
        DrawTextEx(uiFont, relTime, (Vector2){(float)(sw - S.mx - rtW), (float)(y + S.gy/2 + (S.fs16 - S.fs12) / 2)}, S.fs12, 1, t->muted);

        if (state->surahs) {
            Surah *s = surahByNumber(state, bm->surahNumber);
            if (s && s->number > 0)
                DrawTextEx(uiFont, s->name, (Vector2){(float)(tagX), (float)(y + S.gy/2 + S.fs16 + S.gy/4)}, S.fs13, 1, t->muted);
        }
    }
}

void drawSurahOverview(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 200});

    int cw = S.popupW, ch = S.popupH;
    int cx = (sw - cw) / 2, cy = (sh - ch) / 2;
    DrawRectangleRounded((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                         0.08f, 8, t->surface);
    DrawRectangleRoundedLines((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                              0.08f, 8, t->border);

    Surah *s = surahByNumber(state, state->currentSurah);
    if (!s || s->number == 0) return;

    drawArabicTextCentered(s->arabicName,
        (Rectangle){(float)cx, (float)(cy + S.cardPadY), (float)cw, (float)S.fs42 + S.gy},
        S.fs42, t->accent);

    float nameW = MeasureTextEx(uiFont, s->name, S.fs22, 1).x;
    DrawTextEx(uiFont, s->name, (Vector2){(float)(cx + (cw - (int)nameW) / 2), (float)(cy + S.cardPadY + S.fs42 + S.gy)}, S.fs22, 1, t->foreground);

    float badgeY = cy + S.cardPadY + S.fs42 + S.gy + S.fs22 + S.gy;
    DrawRectangleRounded((Rectangle){(float)(cx + S.mx - S.gx), (float)badgeY,
                                     (float)S.badgeW, (float)S.badgeH},
                         0.4f, 4, t->accent);
    DrawTextEx(uiFont, s->revelationType, (Vector2){(float)(cx + S.mx), (float)(badgeY + S.badgeH/2 - S.fs14/2)}, S.fs14, 1, t->background);
    char cnt[32];
    snprintf(cnt, sizeof(cnt), "%d Ayahs", s->ayahCount);
    DrawRectangleRounded((Rectangle){(float)(cx + S.badgeGap), (float)badgeY,
                                     (float)(S.badgeW - S.gx), (float)S.badgeH},
                         0.4f, 4, t->border);
    DrawTextEx(uiFont, cnt, (Vector2){(float)(cx + S.badgeGap + S.gx/2), (float)(badgeY + S.badgeH/2 - S.fs14/2)}, S.fs14, 1, t->foreground);

    float ctxY = badgeY + S.badgeH + S.gy;
    drawWrappedText(s->context,
        (Rectangle){(float)(cx + S.mx - S.gx), (float)ctxY,
                    (float)(cw - 2 * S.mx + S.gx), (float)(ch - (ctxY - cy) - S.fs13 - S.gy*2)},
        S.fs14, t->muted);

    const char *prompt = "Press Enter to begin reading";
    float pw = MeasureTextEx(uiFont, prompt, S.fs13, 1).x;
    DrawTextEx(uiFont, prompt, (Vector2){(float)(cx + (cw - (int)pw) / 2), (float)(cy + ch - S.fs13 - S.gy/2)}, S.fs13, 1, t->muted);
}

void drawSettings(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int sc = getSettingsCursor();

    /* Scrim */
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 180});

    /* Card — centred, 600x520 reference */
    int cw = (int)(600 * S.factor), ch = (int)(520 * S.factor);
    int cx = (sw - cw) / 2, cy = (sh - ch) / 2;
    DrawRectangleRounded((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                         0.06f, 8, t->surface);
    DrawRectangleRoundedLines((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                              0.06f, 8, t->border);

    /* Title */
    DrawTextEx(uiFont, "Settings", (Vector2){(float)(cx + (int)(24 * S.factor)), (float)(cy + (int)(20 * S.factor))}, S.fs22, 1, t->accent);
    int sepY = cy + (int)(54 * S.factor);
    DrawLine(cx + (int)(20 * S.factor), sepY, cx + cw - (int)(20 * S.factor), sepY, t->border);

    /* Row labels */
    const char *labels[] = {
        "Vim Motions", "Font Scale", "Screensaver (s)", "Auto Resume",
        "Theme", "Language", "Calc Method", "Latitude",
        "Test Reminder", "Test Prayer Alarm",
    };
    int rowCount = SETTINGS_ROW_COUNT;
    int rowH = (ch - (int)(80 * S.factor)) / rowCount;
    int startY = sepY + (int)(10 * S.factor);
    int padX = (int)(24 * S.factor);

    for (int i = 0; i < rowCount; i++) {
        int ry = startY + i * rowH;
        int rh = rowH - (int)(4 * S.factor);
        int isCursor = (i == sc);

        /* Cursor highlight — always visible when this row is focused */
        if (isCursor) {
            DrawRectangleRounded(
                (Rectangle){(float)(cx + padX - S.gx), (float)ry, (float)(cw - 2 * padX + 2 * S.gx), (float)rh},
                0.04f, 4, t->surface);
            DrawRectangle(cx + padX - S.gx, ry + (int)(6 * S.factor),
                          (int)(3 * S.factor), rh - (int)(12 * S.factor), t->accent);
        }

        /* Label */
        Color labelColor = isCursor ? t->foreground : t->muted;
        DrawTextEx(uiFont, labels[i],
                   (Vector2){(float)(cx + padX), (float)(ry + rh / 2 - S.fs14 / 2)},
                   S.fs14, 1, labelColor);

        /* Value — right-aligned, derived from state */
        char numBuf[32];
        const char *val = "";
        switch (i) {
            case 0: val = state->vimMotions ? "ON" : "OFF"; break;
            case 1: snprintf(numBuf, sizeof(numBuf), "%.1fx", (double)state->fontScale); val = numBuf; break;
            case 2: snprintf(numBuf, sizeof(numBuf), "%d", state->idleSeconds); val = numBuf; break;
            case 3: val = state->autoResume ? "ON" : "OFF"; break;
            case 4: val = t->name; break;
            case 5: val = state->language[0] == 'b' ? "Bengali" : "English"; break;
            case 6: val = state->calcMethod == 0 ? "Karachi" : state->calcMethod == 1 ? "MWL" : "ISNA"; break;
            case 7:
                if (isCursor && isEditingLat()) {
                    /* Show editable buffer with blinking cursor */
                    val = getLatEditBuf();
                } else {
                    snprintf(numBuf, sizeof(numBuf), "%.2f", (double)state->latitude);
                    val = numBuf;
                }
                break;
            case 8: val = "Play"; break;
            case 9: val = "Play"; break;
        }
        if (val[0]) {
            float vw = MeasureTextEx(uiFont, val, S.fs14, 1).x;
            /* Clamp value width so it doesn't collide with the label */
            float maxVW = (float)(cw - 2 * padX - (int)(140 * S.factor));
            if (vw > maxVW) {
                /* Truncate with ellipsis */
                char truncated[32];
                strncpy(truncated, val, sizeof(truncated) - 1);
                truncated[sizeof(truncated) - 1] = '\0';
                int len = strlen(truncated);
                while (len > 0 && MeasureTextEx(uiFont, truncated, S.fs14, 1).x > maxVW) {
                    truncated[--len] = '\0';
                    if (len > 3) {
                        truncated[len - 1] = '.';
                        truncated[len - 2] = '.';
                        truncated[len - 3] = '.';
                    }
                }
                val = truncated;
                vw = MeasureTextEx(uiFont, val, S.fs14, 1).x;
            }
            Color vc = isCursor ? t->accent : t->foreground;
            float valRightX = (float)(cx + cw - padX);
            DrawTextEx(uiFont, val,
                       (Vector2){valRightX - vw, (float)(ry + rh / 2 - S.fs14 / 2)},
                       S.fs14, 1, vc);
            /* Blinking cursor when editing latitude — at end of actual text */
            if (isCursor && i == 7 && isEditingLat()) {
                float cursorX = valRightX + (int)(2 * S.factor);
                float cursorY = ry + rh / 2 - S.fs14 / 2;
                if (((int)(GetTime() * 2.0) % 2) == 0)
                    DrawRectangle((int)cursorX, (int)cursorY, (int)(2 * S.factor), S.fs14, t->accent);
            }
        }
    }

    /* Footer hint — clamp width to card interior, fallback to short version */
    const char *hintLong = isEditingLat()
        ? "type numbers  Backspace to delete  Enter to confirm  Esc to cancel"
        : "j/k to move  Enter to toggle  Esc to close";
    const char *hintShort = isEditingLat()
        ? "type  Backspace  Enter=ok  Esc=cancel"
        : "j/k  Enter  Esc";
    float hw = MeasureTextEx(uiFont, hintLong, S.fs12, 1).x;
    float hintMaxW = cw - (int)(40 * S.factor);
    const char *hint = (hw > hintMaxW) ? hintShort : hintLong;
    hw = MeasureTextEx(uiFont, hint, S.fs12, 1).x;
    DrawTextEx(uiFont, hint,
               (Vector2){(float)(cx + (cw - hw) / 2), (float)(cy + ch - (int)(24 * S.factor))},
               S.fs12, 1, t->muted);
}

/* ============================================================
 * READING HUB — two tiles: Surah / Hadith
 * ============================================================ */

void drawReadingHub(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    drawTopBar(state);

    int tileW = (int)(360 * S.factor);
    int tileH = (int)(240 * S.factor);
    int gap = (int)(40 * S.factor);
    /* narrow windows shrink tiles instead of clipping. */
    int maxTileW = (sw - 2 * S.mx - gap) / 2;
    if (maxTileW < tileW) {
        tileW = maxTileW < 80 ? 80 : maxTileW;
        tileH = tileW * 2 / 3;
    }
    int totalW = 2 * tileW + gap;
    int startX = (sw - totalW) / 2;
    int startY = (sh - tileH) / 2;

    for (int i = 0; i < 2; i++) {
        int tx = startX + i * (tileW + gap);
        int ty = startY;
        int selected = (state->hubCursor == i);

        /* Tile card */
        DrawRectangleRounded((Rectangle){(float)tx, (float)ty, (float)tileW, (float)tileH},
                             0.06f, 8, t->surface);

        if (selected) {
            /* Glowing border */
            DrawRectangleRoundedLinesEx((Rectangle){(float)tx, (float)ty, (float)tileW, (float)tileH},
                                        0.06f, 8, 3, t->accent);
            /* Subtle accent glow at top */
            DrawRectangleRounded((Rectangle){(float)(tx + 4), (float)(ty + 4),
                                             (float)(tileW - 8), (float)(4 * S.factor)},
                                 0.02f, 4, t->accent);
        } else {
            DrawRectangleRoundedLinesEx((Rectangle){(float)tx, (float)ty, (float)tileW, (float)tileH},
                                        0.06f, 8, 1, t->border);
        }

        int iconY = ty + (int)(40 * S.factor);
        int labelY = iconY + (int)(48 * S.factor);
        int descY = labelY + (int)(36 * S.factor);

        if (i == 0) {
            /* Surah tile */
            DrawTextEx(uiFont, "Surah",
                       (Vector2){(float)(tx + (tileW - (int)(100 * S.factor)) / 2), (float)labelY},
                       (float)(28 * S.factor), 1, t->foreground);
            DrawTextEx(uiFont, "Browse and read the 114 surahs",
                       (Vector2){(float)(tx + (tileW - (int)(280 * S.factor)) / 2), (float)descY},
                       (float)(14 * S.factor), 1, t->muted);
        } else {
            /* Hadith tile */
            DrawTextEx(uiFont, "Hadith",
                       (Vector2){(float)(tx + (tileW - (int)(100 * S.factor)) / 2), (float)labelY},
                       (float)(28 * S.factor), 1, t->foreground);
            DrawTextEx(uiFont, "Major hadith collections",
                       (Vector2){(float)(tx + (tileW - (int)(260 * S.factor)) / 2), (float)descY},
                       (float)(14 * S.factor), 1, t->muted);
        }
    }

    drawFooter(state);
}

/* ============================================================
 * HADITH PAGE — scrollable card list
 * ============================================================ */

/* ── Hadith filter (0=All, 1=Bukhari, 2=Muslim) is a view, not a copy ── */
static int hadithMatches(AppState *state, int idx) {
    if (state->hadithFilter <= 0) return 1;
    const char *want = state->hadithFilter == 1 ? "Bukhari" : "Muslim";
    return strcmp(state->hadiths[idx].collection, want) == 0;
}

static int filteredHadithCount(AppState *state) {
    int n = 0;
    for (int i = 0; i < state->totalHadiths; i++)
        if (hadithMatches(state, i)) n++;
    return n;
}

static Hadith *filteredHadith(AppState *state, int visibleIdx) {
    int k = 0;
    for (int i = 0; i < state->totalHadiths; i++)
        if (hadithMatches(state, i) && k++ == visibleIdx) return &state->hadiths[i];
    return NULL;
}

static int modalScroll = 0;          /* px offset into modal text */
static int modalTotalH = 0;          /* content height from last frame */
static const Hadith *modalFor = NULL; /* resets scroll on hadith change */
static double modalOpenTime = 0;     /* ease-out entrance, mirrors focus modal */

static void drawHadithModal(Theme *t, Hadith *h) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 200});

    int cw = (int)(640 * S.factor), ch = (int)(420 * S.factor);
    int cx = (sw - cw) / 2, cy = (sh - ch) / 2;

    /* 0.25s ease-out entrance, same language as the focus modal. */
    float progress = (float)((GetTime() - modalOpenTime) / 0.25);
    if (progress > 1) progress = 1;
    float eased = 1.0f - (1.0f - progress) * (1.0f - progress) * (1.0f - progress);
    float mScale = 0.85f + 0.15f * eased;
    Vector2 center = {(float)(cx + cw / 2), (float)(cy + ch / 2)};
    Rectangle card = {
        center.x - (cw / 2) * mScale, center.y - (ch / 2) * mScale,
        cw * mScale, ch * mScale
    };
    unsigned char oldA = t->surface.a;
    t->surface.a = (unsigned char)(eased * 255);
    DrawRectangleRounded(card, 0.06f, 8, t->surface);
    DrawRectangleRoundedLines(card, 0.06f, 8, t->accent);
    t->surface.a = oldA;
    if (progress < 0.3f) return;
    cx = (int)(card.x); cy = (int)(card.y);
    cw = (int)card.width; ch = (int)card.height;

    int px = cx + S.cardPadX + S.gx, py = cy + S.cardPadY;
    int innerW = cw - 2 * (S.cardPadX + S.gx);

    Color badgeColor = t->accent;
    if (strcmp(h->collection, "Muslim") == 0)
        badgeColor = (Color){100, 180, 140, 255};
    float badgeW = MeasureTextEx(uiFont, h->collection, S.fs12, 1).x + (int)(10 * S.factor);
    DrawRectangleRounded((Rectangle){(float)px, (float)py, badgeW, (float)(S.fs12 + 4)},
                         0.3f, 4, badgeColor);
    DrawTextEx(uiFont, h->collection,
               (Vector2){(float)(px + (int)(5 * S.factor)), (float)(py + 2)},
               S.fs12, 1, t->background);
    DrawTextEx(uiFont, h->name,
               (Vector2){(float)(px + badgeW + S.gx), (float)(py + 2)},
               S.fs12, 1, t->muted);
    py += S.fs12 + 4 + S.gy;

    /* scroll state lives here; input.c freezes keys behind the modal. */
    if (h != modalFor) { modalFor = h; modalScroll = 0; modalTotalH = 0; modalOpenTime = GetTime(); }
    int tSize = S.fs16;
    int lineH = tSize + 6;
    int trTop = py;
    int trBottom = cy + ch - S.cardPadY - S.fs13 - S.gy;
    int viewH = trBottom - trTop;
    if (viewH < lineH) viewH = lineH;
    float wheel = GetMouseWheelMove();
    if (wheel != 0) modalScroll -= (int)(wheel * 40);
    /* scroll keys repeat while held, like all nav keys. */
    if (navRepeat(KEY_J) || navRepeat(KEY_DOWN)) modalScroll += lineH;
    if (navRepeat(KEY_K) || navRepeat(KEY_UP)) modalScroll -= lineH;
    if (navRepeat(KEY_PAGE_DOWN)) modalScroll += viewH;
    if (navRepeat(KEY_PAGE_UP)) modalScroll -= viewH;
    if (modalScroll < 0) modalScroll = 0;
    if (modalScroll > modalTotalH - viewH)
        modalScroll = modalTotalH - viewH > 0 ? modalTotalH - viewH : 0;

    Rectangle tr = {(float)px, (float)trTop, (float)(innerW - S.gx), (float)viewH};
    modalTotalH = drawWrappedTextScroll(h->text ? h->text : "", tr, uiFont, tSize, t->foreground, modalScroll);

    /* Scrollbar when content overflows */
    if (modalTotalH > viewH) {
        float trackX = (float)(px + innerW - (int)(4 * S.factor));
        DrawRectangleRounded((Rectangle){trackX, (float)trTop, (float)(4 * S.factor), (float)viewH},
                             0.5f, 4, Fade(t->border, 0.5f));
        float thumbH = (float)viewH * (float)viewH / (float)modalTotalH;
        if (thumbH < 12) thumbH = 12;
        float thumbY = (float)trTop + ((float)viewH - thumbH) *
                       (float)modalScroll / (float)(modalTotalH - viewH);
        DrawRectangleRounded((Rectangle){trackX, thumbY, (float)(4 * S.factor), thumbH},
                             0.5f, 4, t->accent);
    }

    if (h->narrator[0])
        DrawTextEx(uiFont, h->narrator, (Vector2){(float)(px), (float)(cy + ch - S.cardPadY - S.fs13)},
                   S.fs13, 1, t->muted);
    const char *prompt = "Esc to close";
    float pw = MeasureTextEx(uiFont, prompt, S.fs13, 1).x;
    DrawTextEx(uiFont, prompt, (Vector2){(float)(cx + cw - S.cardPadX - S.gx - pw),
               (float)(cy + ch - S.cardPadY - S.fs13)}, S.fs13, 1, t->muted);
}

void drawHadithPage(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    drawTopBar(state);
    drawFooter(state);

    int listY = TOPBAR_H;
    int listH = sh - TOPBAR_H - FOOTER_H;

    /* Header */
    int titleY = listY + (S.my - S.fs22) / 2;
    DrawTextEx(uiFont, "Major Hadiths", (Vector2){(float)(S.mx), (float)(titleY)}, S.fs22, 1, t->foreground);
    char countStr[32];
    int fTotal = (state->hadiths && state->totalHadiths > 0) ? filteredHadithCount(state) : 0;
    snprintf(countStr, sizeof(countStr), "%d shown", fTotal);
    DrawTextEx(uiFont, countStr, (Vector2){(float)(S.mx + (int)(160 * S.factor)), (float)(titleY + (S.fs22 - S.fs14) / 2)}, S.fs14, 1, t->muted);

    /* Source filter tabs (upper section) — clickable */
    const char *tabs[] = { "All", "Bukhari", "Muslim" };
    float tabX = (float)(sw - S.mx);
    for (int i = 2; i >= 0; i--) {
        float tw = MeasureTextEx(uiFont, tabs[i], S.fs13, 1).x + (int)(16 * S.factor);
        tabX -= tw + S.gx / 2;
        Rectangle tabR = {tabX, (float)titleY, tw, (float)(S.fs13 + 8)};
        int active = (state->hadithFilter == i);
        DrawRectangleRounded(tabR, 0.3f, 4, active ? t->accent : t->surface);
        DrawTextEx(uiFont, tabs[i], (Vector2){tabX + (int)(8 * S.factor), (float)(titleY + 4)},
                   S.fs13, 1, active ? t->background : t->muted);
        if (!active && CheckCollisionPointRec(GetMousePosition(), tabR) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state->hadithFilter = i;
            state->hadithCursor = 0;
        }
    }

    int headerH = S.my + S.fs22 + S.gy/2;
    listY += headerH;
    listH -= headerH;

    DrawLine(S.mx, listY, sw - S.mx, listY, t->border);
    listY += S.gy/2;
    listH -= S.gy/2;

    if (state->totalHadiths <= 0 || !state->hadiths) {
        const char *msg = "No hadiths loaded — restart with internet to fetch.";
        int tw = MeasureTextEx(uiFont, msg, S.fs16, 1).x;
        DrawTextEx(uiFont, msg, (Vector2){(float)((sw - tw) / 2), (float)(listY + listH / 2 - S.fs16/2)}, S.fs16, 1, t->muted);
        return;
    }

    int rowH = (int)(80 * S.factor);
    int visible = listH / rowH;
    int total = fTotal;
    int cursor = state->hadithCursor;

    /* Clamp cursor into the filtered view */
    if (cursor < 0) { state->hadithCursor = 0; cursor = 0; }
    if (total > 0 && cursor >= total) { state->hadithCursor = total - 1; cursor = total - 1; }

    /* Scroll offset */
    int scrollOff = 0;
    if (cursor < scrollOff) scrollOff = cursor;
    if (cursor >= scrollOff + visible) scrollOff = cursor - visible + 1;
    if (total <= visible) scrollOff = 0;
    else if (scrollOff > total - visible) scrollOff = total - visible;
    if (scrollOff < 0) scrollOff = 0;

    for (int i = scrollOff; i < total && i < scrollOff + visible; i++) {
        int y = listY + (i - scrollOff) * rowH;
        int active = (i == cursor);

        Rectangle rowR = {(float)S.mx, (float)y,
                          (float)(sw - 2 * S.mx), (float)(rowH - 4)};
        /* click opens the modal; geometry already here. */
        if (CheckCollisionPointRec(GetMousePosition(), rowR) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state->hadithCursor = i;
            state->showHadithModal = 1;
            active = 1;
        }

        /* Card background */
        if (active)
            DrawRectangleRounded(rowR, 0.04f, 6, t->surface);

        /* Accent bar on active row */
        if (active)
            DrawRectangle(S.mx, y + (int)(6 * S.factor), (int)(3 * S.factor), rowH - (int)(16 * S.factor), t->accent);

        Hadith *h = filteredHadith(state, i);
        if (!h) continue;

        /* Collection badge */
        Color badgeColor = t->accent;
        if (strcmp(h->collection, "Muslim") == 0)
            badgeColor = (Color){100, 180, 140, 255};
        float badgeW = MeasureTextEx(uiFont, h->collection, S.fs12, 1).x + (int)(10 * S.factor);
        DrawRectangleRounded((Rectangle){(float)(S.mx + S.gx), (float)(y + S.gy/2),
                                         (float)badgeW, (float)(S.fs12 + 4)},
                             0.3f, 4, badgeColor);
        DrawTextEx(uiFont, h->collection,
                   (Vector2){(float)(S.mx + S.gx + 5 * S.factor), (float)(y + S.gy/2 + 2)},
                   S.fs12, 1, t->background);

        /* Narrator — truncated to row width */
        float narratorX = S.mx + S.gx + badgeW + S.gx;
        {
            float maxW = (float)(sw - S.mx - S.gx) - narratorX;
            char narr[128];
            snprintf(narr, sizeof(narr), "%s", h->narrator);
            while (narr[0] && MeasureTextEx(uiFont, narr, S.fs12, 1).x > maxW && strlen(narr) > 4) {
                narr[strlen(narr) - 4] = '\0';
                strcat(narr, "...");
            }
            DrawTextEx(uiFont, narr,
                       (Vector2){narratorX, (float)(y + S.gy/2 + 2)},
                       S.fs12, 1, t->muted);
        }

        /* Hadith text — wrapped */
        int textY = y + S.gy/2 + (S.fs12 + 4) + S.gy/2;
        drawWrappedText(h->text,
            (Rectangle){(float)(S.mx + S.gx), (float)textY,
                        (float)(sw - 2 * S.mx - 2 * S.gx), (float)(rowH - textY + y - S.gy/2)},
            S.fs14, active ? t->foreground : t->muted);
    }

    if (state->showHadithModal) {
        Hadith *mh = filteredHadith(state, state->hadithCursor);
        if (mh) drawHadithModal(t, mh);
        else state->showHadithModal = 0;
    }
}

void drawTopBar(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    DrawRectangle(0, 0, sw, TOPBAR_H, t->surface);
    DrawLine(0, TOPBAR_H, sw, TOPBAR_H, t->border);
    float titleX = (float)(S.mx - S.gx);
    float titleY = (float)((TOPBAR_H - S.fs26) / 2 + S.fs26 / 4);
    DrawTextEx(uiFont, "Ayatika", (Vector2){titleX, titleY}, S.fs26, 1, t->accent);
    char buf[128];
    snprintf(buf, sizeof(buf), "Next: %s %s", getNextPrayerName(&state->prayer),
             formatCountdown(getNextPrayerTime(&state->prayer)));
    DrawTextEx(uiFont, buf, (Vector2){(float)(sw - S.topbarPrayerXOffset), (float)((TOPBAR_H - S.fs16)/2 + S.fs16/4)}, S.fs16, 1, t->muted);
}

void drawSidebar(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sh = GetScreenHeight();
    int listY = TOPBAR_H, listH = sh - TOPBAR_H - FOOTER_H;
    int rowH = S.sidebarRowH;
    DrawRectangle(0, listY, SIDEBAR_W, listH, t->surface);
    int total = state->surahCount;
    int visible = listH / rowH;
    static int scrollOff = 0;
    if (state->cursorSurah < scrollOff) scrollOff = state->cursorSurah;
    if (state->cursorSurah >= scrollOff + visible) scrollOff = state->cursorSurah - visible + 1;
    if (scrollOff > total - visible) scrollOff = total - visible;
    if (scrollOff < 0) scrollOff = 0;
    /* viewport glides toward scrollOff (exponential ease-out);
       selection itself stays exact — only the scroll animates. */
    static float sbPos = 0;
    {
        float k = GetFrameTime() * 10.0f;
        if (k > 1) k = 1;
        sbPos += ((float)scrollOff - sbPos) * k;
        if (fabsf((float)scrollOff - sbPos) < 0.02f) sbPos = (float)scrollOff;
    }
    int startRow = (int)floorf(sbPos);
    if (startRow < 0) { startRow = 0; sbPos = 0; }
    float yOff = (sbPos - (float)startRow) * (float)rowH;
    for (int i = startRow; i < total && i < startRow + visible + 1; i++) {
        int y = listY + (int)((i - startRow) * rowH - yOff);
        int active = (i == state->cursorSurah);
        if (active) DrawRectangle(0, y, SIDEBAR_W, rowH, t->accent);
        DrawLine(0, y + rowH, SIDEBAR_W, y + rowH, t->border);
        Surah *s = &state->surahs[i];
        char num[8];
        snprintf(num, sizeof(num), "%d.", s->number);
        DrawTextEx(uiFont, num, (Vector2){(float)(S.cardPadX), (float)(y + 6)}, S.fs14, 1, active ? t->background : t->muted);
        DrawTextEx(uiFont, s->name, (Vector2){(float)(S.cardPadX + S.fs14 + S.gx), (float)(y + 5)}, S.fs16, 1, active ? t->background : t->foreground);
        char cnt[16];
        snprintf(cnt, sizeof(cnt), "%d ayahs", s->ayahCount);
        DrawTextEx(uiFont, cnt, (Vector2){(float)(S.cardPadX + S.fs14 + S.gx), (float)(y + 24)}, S.fs12, 1, active ? t->background : t->muted);
        int dotX = SIDEBAR_W - S.gx, dotY = y + rowH/2;
        /* Meccan = filled disc, Medinan = hollow ring — shape backs up color. */
        if (active) DrawCircle(dotX, dotY, S.dotRadius, t->background);
        else if (strcmp(s->revelationType, "Meccan") == 0) DrawCircle(dotX, dotY, S.dotRadius, t->accent);
        else DrawCircleLines(dotX, dotY, (float)S.dotRadius, t->border);
    }
}

void drawFooter(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, sh - FOOTER_H, sw, FOOTER_H, t->surface);
    DrawLine(0, sh - FOOTER_H, sw, sh - FOOTER_H, t->border);
    /* list screen gets jump hints; everywhere else names the finder. */
    const char *helpRight;
    if (state->currentScreen == SCREEN_SURAH_LIST)
        helpRight = "0-9 = jump   type = palette   Ctrl+d/u = page   Enter = open   Esc = clear";
    else if (state->currentScreen == SCREEN_BOOKMARKS)
        helpRight = "Enter = open   d = delete   Esc = back";
    else if (state->vimMotions)
        helpRight = "F1 = help   j/k/h/l = navigate   o = finder   g = dashboard   Enter = open   / = search   m = bookmarks";
    else
        helpRight = "F1 = help   arrows = navigate   o = finder   g = dashboard   Enter = open   / = search   m = bookmarks";
    float hintW = MeasureTextEx(uiFont, helpRight, S.fs13, 1).x;
    /* narrow windows fall back to the short hint so status keeps room. */
    if (hintW > sw * 0.62f) {
        if (state->currentScreen == SCREEN_SURAH_LIST)
            helpRight = "0-9 jump · type find · Enter open";
        else if (state->currentScreen == SCREEN_BOOKMARKS)
            helpRight = "Enter open · d delete";
        else if (state->vimMotions)
            helpRight = "F1 help · j/k move · o finder · g home";
        else
            helpRight = "F1 help · arrows · o finder · g home";
        hintW = MeasureTextEx(uiFont, helpRight, S.fs13, 1).x;
    }
    DrawTextEx(uiFont, helpRight, (Vector2){(float)(sw - S.mx + S.gx - hintW), (float)(sh - FOOTER_H + (FOOTER_H - S.fs13)/2)}, S.fs13, 1, t->muted);
    /* status truncated to whatever space the hint leaves. */
    {
        float maxW = (float)sw - (S.mx - S.gx) - hintW - 3 * S.gx;
        char msg[256];
        snprintf(msg, sizeof(msg), "%s", state->statusMsg);
        while (msg[0] && MeasureTextEx(uiFont, msg, S.fs14, 1).x > maxW && strlen(msg) > 4) {
            msg[strlen(msg) - 4] = '\0';
            strcat(msg, "...");
        }
        if (maxW > 40)
            DrawTextEx(uiFont, msg, (Vector2){(float)(S.mx - S.gx), (float)(sh - FOOTER_H + (FOOTER_H - S.fs14)/2)}, S.fs14, 1, state->statusTone ? t->accent : t->muted);
    }
}

static void nextPrayerInfo(AppState *state, char *name, int nameSz, char *countdown, int cdSz, float *progress) {
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    float cur = lt->tm_hour + lt->tm_min / 60.0f;
    PrayerTimes *p = &state->prayer;
    float times[5] = {p->fajr, p->dhuhr, p->asr, p->maghrib, p->isha};
    const char *names[5] = {"Fajr", "Dhuhr", "Asr", "Maghrib", "Isha"};
    int next = -1;
    float prev = times[4] - 24;
    for (int i = 0; i < 5; i++) {
        if (times[i] > cur + 0.001f) { next = i; break; }
        prev = times[i];
    }
    if (next < 0) { next = 0; prev = times[4]; times[0] += 24; }
    snprintf(name, nameSz, "%s", names[next]);
    float hUntil = times[next] - cur;
    if (hUntil < 0) hUntil += 24;
    int hh = (int)hUntil, mm = (int)((hUntil - hh) * 60);
    snprintf(countdown, cdSz, "%dh %02dm", hh, mm);
    if (progress) {
        float total = times[next] - prev;
        if (total <= 0) total = 4;
        float elap = cur - prev;
        *progress = (elap < 0 ? 0 : (elap > total ? 1 : elap / total));
    }
}

void drawHelpOverlay(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 180});

    int cw = S.helpW, ch = S.helpH;
    int cx = (sw - cw) / 2, cy = (sh - ch) / 2;
    DrawRectangleRounded((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                         0.06f, 8, t->surface);
    DrawRectangleRoundedLines((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                              0.06f, 8, t->border);

    DrawTextEx(uiFont, "Keyboard Shortcuts", (Vector2){(float)(cx + S.mx - S.gx), (float)(cy + (int)(16 * S.factor))}, S.fs18, 1, t->accent);
    DrawLine(cx + S.mx - S.gx, cy + (int)(42 * S.factor),
             cx + cw - (S.mx - S.gx), cy + (int)(42 * S.factor), t->border);

    static const char *vimKeys[][2] = {
        {"j / k",     "Move cursor down / up"},
        {"h / l",     "Dashboard: left / right"},
        {"G / End",   "Go to top / bottom"},
        {"Ctrl+d / u","Half page down / up"},
        {"o",         "Finder, surah jump"},
        {"g",         "Go to dashboard"},
        {"0-9 / type","Jump to number / name (Surah list)"},
        {"Enter",     "Open selected item"},
        {"Esc",       "Go back"},
        {"/",         "Finder, ayah search"},
        {"b",         "Bookmark current ayah (Reader)"},
        {"m",         "Open bookmarks"},
        {"d",         "Delete bookmark (Bookmarks)"},
        {"f",         "Toggle focus mode (Reader)"},
        {"t",         "Cycle themes"},
        {"s",         "Open settings"},
        {"Home",      "Go to dashboard"},
        {"Tab",       "Finder tab / hadith filter"},
        {"F1",        "Open this help / Esc closes"},
    };
    static const char *arrowKeys[][2] = {
        {"Up / Down",    "Move cursor up / down"},
        {"Left / Right", "Navigate / scroll"},
        {"PgUp / PgDn",  "Go to top / bottom"},
        {"Ctrl+d / u",   "Half page down / up"},
        {"o",            "Finder, surah jump"},
        {"g",            "Go to dashboard"},
        {"0-9 / type",   "Jump to number / name (Surah list)"},
        {"Enter",        "Open selected item"},
        {"Esc",          "Go back"},
        {"/",            "Finder, ayah search"},
        {"b",            "Bookmark current ayah (Reader)"},
        {"m",            "Open bookmarks"},
        {"d",            "Delete bookmark (Bookmarks)"},
        {"f",            "Toggle focus mode (Reader)"},
        {"t",            "Cycle themes"},
        {"s",            "Open settings"},
        {"g",            "Go to dashboard"},
        {"Tab",          "Filter hadith source (Hadith)"},
        {"F1",           "Open this help / Esc closes"},
    };
    const char *helpClose = "Press Esc to close";

    /* fit rows into the card whatever the table size. */
    int helpRows = state->vimMotions ? (int)(sizeof(vimKeys) / sizeof(vimKeys[0]))
                                     : (int)(sizeof(arrowKeys) / sizeof(arrowKeys[0]));
    int helpStep = (int)(28 * S.factor);
    if (helpRows > 1) {
        int fitStep = (ch - (int)(58 * S.factor) - (int)(30 * S.factor)) / (helpRows - 1);
        if (fitStep < helpStep) helpStep = fitStep;
        int minStep = (int)(16 * S.factor);
        if (helpStep < minStep) helpStep = minStep;
    }

    if (state->vimMotions) {
        int vrows = (int)(sizeof(vimKeys) / sizeof(vimKeys[0]));
        for (int i = 0; i < vrows; i++) {
            int y = cy + (int)(58 * S.factor) + i * helpStep;
            DrawTextEx(uiFont, vimKeys[i][0], (Vector2){(float)(cx + (int)(24 * S.factor)), (float)(y)}, S.fs14, 1, t->accent);
            DrawTextEx(uiFont, vimKeys[i][1], (Vector2){(float)(cx + (int)(150 * S.factor)), (float)(y)}, S.fs14, 1, t->foreground);
        }
    } else {
        int arows = (int)(sizeof(arrowKeys) / sizeof(arrowKeys[0]));
        for (int i = 0; i < arows; i++) {
            int y = cy + (int)(58 * S.factor) + i * helpStep;
            DrawTextEx(uiFont, arrowKeys[i][0], (Vector2){(float)(cx + (int)(24 * S.factor)), (float)(y)}, S.fs14, 1, t->accent);
            DrawTextEx(uiFont, arrowKeys[i][1], (Vector2){(float)(cx + (int)(150 * S.factor)), (float)(y)}, S.fs14, 1, t->foreground);
        }
    }

    float cw2 = MeasureTextEx(uiFont, helpClose, S.fs13, 1).x;
    DrawTextEx(uiFont, helpClose, (Vector2){(float)(cx + (cw - (int)cw2) / 2), (float)(cy + ch - S.fs13 - 8)}, S.fs13, 1, t->muted);
}

void drawBookmarkPopup(AppState *state) {
    if (bookmarkPopupTime <= 0) return;
    double elapsed = GetTime() - bookmarkPopupTime;
    if (elapsed > 2.0) { bookmarkPopupTime = 0; return; }

    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    float alpha = elapsed < 1.5 ? 1.0f : 1.0f - (float)((elapsed - 1.5) / 0.5);

    const char *msg = bookmarkPopupMsg;
    int tw = MeasureTextEx(uiFont, msg, S.fs16, 1).x;
    int pw = tw + 40, ph = S.fs16 + 20;
    int px = (sw - pw) / 2, py = TOPBAR_H + S.gx/2;

    Color bg = t->surface; bg.a = (unsigned char)(200 * alpha);
    Color fg = t->accent;  fg.a = (unsigned char)(255 * alpha);

    DrawRectangleRounded((Rectangle){(float)px, (float)py, (float)pw, (float)ph},
                         0.3f, 4, bg);
    DrawTextEx(uiFont, msg, (Vector2){(float)(px + 20), (float)(py + (ph - S.fs16)/2)}, S.fs16, 1, fg);
}

static int reorderArabic(const char *text, char *visualOut, int outSize) {
    FriBidiChar logical[4096];
    FriBidiStrIndex len = fribidi_charset_to_unicode(
        FRIBIDI_CHAR_SET_UTF8, text, strlen(text), logical);
    if (len <= 0) { visualOut[0] = '\0'; return 0; }
    if (len >= 4096) len = 4095;

    FriBidiChar visual[4096];
    FriBidiParType baseDir = FRIBIDI_PAR_RTL;
    FriBidiLevel levels[4096];
    FriBidiStrIndex map[4096];
    FriBidiLevel maxLevel = fribidi_log2vis(
        logical, len, &baseDir, visual, map, NULL, levels);
    (void)maxLevel; (void)map;

    /* Ensure output fits in the provided buffer.
       UTF-8 can be up to 4 bytes per codepoint, so len*4 may exceed outSize. */
    int maxLen = (outSize - 1) / 4;
    if (len > maxLen) len = maxLen;
    fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual, len, visualOut);

    /* One-time diagnostic: print first 10 codepoints of visual output */
    static int diagDone = 0;
    if (!diagDone) {
        diagDone = 1;
        printf("Arabic reorder diag: input %d codepoints, visual output:\n", (int)len);
        int n = len < 10 ? len : 10;
        for (int i = 0; i < n; i++) {
            printf("  [%d] U+%04X\n", i, (unsigned)visual[i]);
        }
        /* Also show what the font atlas has */
        printf("Font atlas: %dx%d, %d glyphs\n",
               arabicFont.texture.width, arabicFont.texture.height,
               arabicFont.glyphCount);
        for (int i = 0; i < (arabicFont.glyphCount < 10 ? arabicFont.glyphCount : 10); i++) {
            printf("  glyph[%d] = U+%04X\n", i, (unsigned)arabicFont.glyphs[i].value);
        }
    }

    return 1;
}

void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color) {
    char visual[8192];
    const char *src = reorderArabic(text, visual, sizeof(visual)) ? visual : text;
    drawArabicVisualCentered(src, bounds, size, color);
}

void drawArabicVisualCentered(const char *visualText, Rectangle bounds, float size, Color color) {
    Font f = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    float sp = size * 0.12f;
    float tw = measureShaped(f, visualText, size, sp);
    Vector2 pos = {bounds.x + (bounds.width - tw) / 2, bounds.y};
    drawShaped(f, visualText, pos, size, sp, color);
}


