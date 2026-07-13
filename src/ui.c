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

Font arabicFont;
Font uiFont;

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
    s.helpH  = SCL(420);

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

static void collectArabicCodepoints(AppState *state, int *out, int *outCount, int maxCount) {
    /* Build a string of all Arabic text from mock data, then extract unique codepoints */
    static char allText[32768];
    int pos = 0;

    /* Surah arabic names */
    for (int i = 0; i < 7 && i < TOTAL_SURAHS; i++) {
        if (!state->surahs[i].arabicName[0]) continue;
        int len = (int)strlen(state->surahs[i].arabicName);
        if (pos + len + 1 < (int)sizeof(allText)) {
            memcpy(allText + pos, state->surahs[i].arabicName, len);
            pos += len;
            allText[pos++] = ' ';
        }
    }

    /* Ayah arabic text */
    for (int i = 0; i < state->totalAyahs; i++) {
        if (!state->ayahs[i].arabicText[0]) continue;
        int len = (int)strlen(state->ayahs[i].arabicText);
        if (pos + len + 1 < (int)sizeof(allText)) {
            memcpy(allText + pos, state->ayahs[i].arabicText, len);
            pos += len;
            allText[pos++] = ' ';
        }
    }
    allText[pos] = '\0';

    /* Use RayLib to extract codepoints from the concatenated UTF-8 string */
    int totalCount = 0;
    int *allCodepoints = LoadCodepoints(allText, &totalCount);

    /* Deduplicate using a seen-set (just check against output array) */
    *outCount = 0;
    for (int i = 0; i < totalCount && *outCount < maxCount; i++) {
        int cp = allCodepoints[i];
        int seen = 0;
        for (int j = 0; j < *outCount; j++) {
            if (out[j] == cp) { seen = 1; break; }
        }
        if (!seen) out[(*outCount)++] = cp;
    }

    UnloadCodepoints(allCodepoints);

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

    /* Load JetBrains Mono for UI text — Latin + punctuation only */
    int uiCodepoints[320];
    int uiCount = 0;
    for (int i = 0x0020; i <= 0x007E; i++) uiCodepoints[uiCount++] = i;
    for (int i = 0x00A0; i <= 0x00FF; i++) uiCodepoints[uiCount++] = i;
    for (int i = 0x2000; i <= 0x206F; i++) uiCodepoints[uiCount++] = i;

    uiFont = LoadFontEx("assets/JetBrainsMono-Regular.ttf", 96, uiCodepoints, uiCount);
    if (uiFont.texture.id > 0)
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
    else
        uiFont = GetFontDefault();
}

void closeFonts(void) {
    if (uiFont.texture.id > 0 && uiFont.texture.id != arabicFont.texture.id)
        UnloadFont(uiFont);
    if (arabicFont.texture.id > 0)
        UnloadFont(arabicFont);
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
        case SCREEN_SEARCH: {
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            int boxW = (int)(500 * S.factor), boxH = (int)(56 * S.factor);
            int bx = sw/2-boxW/2, by = sh/2-boxH/2;
            DrawRectangleRounded((Rectangle){(float)bx, (float)by, (float)boxW, (float)boxH},
                                 0.12f, 8, t->surface);
            DrawRectangleRoundedLines((Rectangle){(float)bx, (float)by, (float)boxW, (float)boxH},
                                      0.12f, 8, t->accent);
            /* Search icon + hint */
            DrawTextEx(uiFont, "\xe2\x9c\x93", (Vector2){(float)(bx + S.cardPadX + S.gx), (float)(by + (boxH - S.fs16) / 2)}, S.fs16, 1, t->muted);
            const char *searchHint = "Search surahs, ayahs, and topics";
            DrawTextEx(uiFont, searchHint, (Vector2){(float)(bx + S.cardPadX + S.gx + S.fs16 + S.gy/2), (float)(by + (boxH - S.fs14) / 2)}, S.fs14, 1, t->muted);
            /* Keyboard shortcut hint */
            const char *searchSub = "Press Enter to search, Esc to close";
            int twSub = MeasureTextEx(uiFont, searchSub, S.fs13, 1).x;
            DrawTextEx(uiFont, searchSub, (Vector2){(float)(sw/2 - twSub/2), (float)(by + boxH + S.gy)}, S.fs13, 1, t->muted);
            break;
        }
        case SCREEN_BOOKMARKS:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawBookmarks(state);
            break;
        case SCREEN_SCREENSAVER: {
            ClearBackground(BLACK);
            /* Floating digital clock with Ayatika branding */
            time_t now = time(NULL);
            struct tm *lt = localtime(&now);
            char tbuf[32];
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
            float tw = MeasureTextEx(uiFont, tbuf, S.fs48, 1).x;
            float tx = (sw - tw) / 2;
            /* Center the clock+date group vertically */
            float groupH = S.fs48 + S.gy + S.fs18;
            float ty = sh / 2 - groupH / 2;
            /* Draw subtle glow effect */
            Color glowColor = t->accent;
            glowColor.a = 40;
            for (int g = 1; g <= 4; g++)
                DrawTextEx(uiFont, tbuf, (Vector2){(float)(tx - g*0.5f), (float)(ty)}, S.fs48, 1, glowColor);
            DrawTextEx(uiFont, tbuf, (Vector2){(float)(tx), (float)(ty)}, S.fs48, 1, t->accent);

            /* Date subtitle */
            static const char *wd[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
            static const char *mo[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
            char dbuf[64];
            snprintf(dbuf, sizeof(dbuf), "%s, %s %d", wd[lt->tm_wday], mo[lt->tm_mon], lt->tm_mday);
            float dw = MeasureTextEx(uiFont, dbuf, S.fs18, 1).x;
            DrawTextEx(uiFont, dbuf, (Vector2){(float)((sw - dw)/2), (float)(ty + S.fs48 + S.gy)}, S.fs18, 1, t->muted);

            /* Ayatika branding at bottom */
            const char *brand = "Ayatika";
            float bw = MeasureTextEx(uiFont, brand, S.fs14, 1).x;
            DrawTextEx(uiFont, brand, (Vector2){(float)((sw - bw)/2), (float)(sh - S.footerH - S.gy)}, S.fs14, 1, t->muted);
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

void showBookmarkPopup(void) {
    bookmarkPopupTime = GetTime();
}

/* Forward declaration needed because drawDashboard calls reorderArabic before its definition */
static int reorderArabic(const char *text, char *visualOut, int outSize);

static int isBookmarked(int surah, int ayah) {
    for (int i = 0; i < mockBookmarkCount; i++)
        if (mockBookmarks[i].surahNumber == surah &&
            mockBookmarks[i].ayahNumber == ayah)
            return 1;
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

static void drawWrappedText(const char *text, Rectangle bounds, int fontSize, Color color) {
    if (!text || !*text) return;
    Font f = uiFont;
    int y = (int)bounds.y;
    int lineH = fontSize + 4;
    const char *lineStart = text;

    while (*lineStart && y + fontSize <= (int)(bounds.y + bounds.height)) {
        char lineBuf[2048];
        const char *p = lineStart;
        const char *lastSpace = NULL;
        int flushed = 0;

        while (*p && *p != '\n') {
            int len = p - lineStart + 1;
            if (len >= (int)sizeof(lineBuf)) break;

            memcpy(lineBuf, lineStart, len);
            lineBuf[len] = '\0';
            float w = MeasureTextEx(f, lineBuf, fontSize, 1).x;

            if (w > bounds.width && len > 1) {
                int flushLen;
                const char *nextStart;
                if (lastSpace) {
                    flushLen = lastSpace - lineStart;
                    nextStart = lastSpace + 1;
                } else {
                    flushLen = p - lineStart;
                    nextStart = p;
                }
                memcpy(lineBuf, lineStart, flushLen);
                lineBuf[flushLen] = '\0';
                DrawTextEx(f, lineBuf, (Vector2){bounds.x, (float)y}, fontSize, 1, color);
                y += lineH;
                lineStart = nextStart;
                flushed = 1;
                break;
            }

            if (*p == ' ') lastSpace = p;
            p++;
        }

        if (!flushed) {
            int len = p - lineStart;
            if (len > 0) {
                memcpy(lineBuf, lineStart, len);
                lineBuf[len] = '\0';
                DrawTextEx(f, lineBuf, (Vector2){bounds.x, (float)y}, fontSize, 1, color);
                y += lineH;
            }
            if (*p == '\n') lineStart = p + 1;
            else break;
        }
    }
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
        snprintf(sub, sizeof(sub), "You're in %s", state->surahs[state->currentSurah-1].name);
        DrawTextEx(uiFont, sub, (Vector2){(float)(px), (float)(py)}, S.fs14, 1, t->muted);
    }
    /* PRAYER */
    {
        Rectangle r = cards[1];
        int px = r.x + S.cardPadX + S.gx, py = r.y + S.cardPadY;
        char name[32], cd[16]; float prog;
        nextPrayerInfo(state, name, sizeof(name), cd, sizeof(cd), &prog);
        char line[64];
        snprintf(line, sizeof(line), "%s -- %s", name, cd);
        DrawTextEx(uiFont, line, (Vector2){(float)(px), (float)(py)}, S.fs18, 1, t->foreground); py += S.fs18 + S.gy;
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
            Surah *surah = &state->surahs[a->surahNumber - 1];

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

            /* ── Arabic text — centered, auto-shrink to fit width ── */
            int arabicTop = sep1Y + S.gy/2;
            int arabicAvailable = (int)(innerH * 0.25f);

            char visual[4096];
            int haveVisual = reorderArabic(a->arabicText, visual, sizeof(visual));
            Font af = arabicFont.texture.id > 0 ? arabicFont : uiFont;
            float arSize = (float)S.fs34;
            float arSp = arSize * 0.12f;
            float arW = haveVisual
                ? MeasureTextEx(af, visual, arSize, arSp).x
                : MeasureTextEx(uiFont, a->arabicText, arSize, 1).x;
            float arMin = (float)S.fs14;
            while (arW > innerW && arSize > arMin) {
                arSize -= 2;
                if (arSize < arMin) arSize = arMin;
                arSp = arSize * 0.12f;
                arW = haveVisual
                    ? MeasureTextEx(af, visual, arSize, arSp).x
                    : MeasureTextEx(uiFont, a->arabicText, arSize, 1).x;
            }
            if (haveVisual)
                drawArabicVisualCentered(visual,
                    (Rectangle){(float)innerX, (float)arabicTop, (float)innerW, (float)arabicAvailable},
                    arSize, t->foreground);
            else
                drawArabicTextCentered(a->arabicText,
                    (Rectangle){(float)innerX, (float)arabicTop, (float)innerW, (float)arabicAvailable},
                    arSize, t->foreground);

            /* ── Bottom separator ── */
            int sep2Y = arabicTop + arabicAvailable + S.gy/2;
            DrawLine(innerX, sep2Y, (int)(r.x + r.width - S.cardPadX - S.gx), sep2Y, t->border);

            /* ── Full translation (wrapped, not truncated) ── */
            int transTop = sep2Y + S.gy/2;
            int transBottom = (int)(r.y + r.height - S.cardPadY - S.fs13 - S.gy);
            char *translation = (strcmp(state->language, "bn") == 0)
                                ? a->translationBn : a->translationEn;
            drawWrappedText(translation,
                (Rectangle){(float)innerX, (float)transTop,
                            (float)innerW, (float)(transBottom - transTop)},
                S.fs14, t->muted);

            /* ── Footer reference line ── */
            char footerRef[128];
            snprintf(footerRef, sizeof(footerRef), "%s (%d) -- Ayah %d of %d",
                     surah->name, surah->number, a->ayahNumber, surah->ayahCount);
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
        if (state->totalHadiths > 0) {
            Hadith *h = &state->hadiths[0];
            DrawTextEx(uiFont, h->name, (Vector2){(float)(px), (float)(py)}, S.fs13, 1, t->muted); py += S.fs13 + S.gy/2;
            char txt[200];
            strncpy(txt, h->text, sizeof(txt)-1);
            txt[sizeof(txt)-1] = '\0';
            if (strlen(h->text) > sizeof(txt)-5) { strcat(txt, "..."); }
            DrawTextEx(uiFont, txt, (Vector2){(float)(px), (float)(py)}, S.fs14, 1, t->foreground); py += S.fs14 + S.gy/2;
            float tw = MeasureTextEx(uiFont, h->collection, S.fs12, 1).x;
            DrawTextEx(uiFont, h->collection, (Vector2){(float)(px + halfW - 2 * (S.cardPadX + S.gx) - tw), (float)(py)}, S.fs12, 1, t->accent);
        }
    }
    /* CONTINUE READING */
    {
        Rectangle r = cards[4];
        int px = r.x + S.cardPadX + S.gx, py = r.y + S.cardPadY;
        int sn = state->currentSurah, an = state->currentAyah;
        char ln[96];
        snprintf(ln, sizeof(ln), "%s, Ayah %d", state->surahs[sn-1].name, an);
        DrawTextEx(uiFont, ln, (Vector2){(float)(px), (float)(py)}, S.fs22, 1, t->accent); py += S.fs22 + S.gy;
        long ts = getMockBookmarkTimestamp(state->currentSurah, state->currentAyah);
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
    drawFooter(state);
}

/* ── Helper: draw ayah content (text + translation + ref) ── */
static void drawAyahContent(AppState *state, Theme *t, int sw, int sh) {
    int mx = SIDEBAR_W + S.mx;
    int my = TOPBAR_H + S.my;
    int mainW = sw - SIDEBAR_W - 2 * S.mx;

    Ayah *ayah = findMockAyah(state, state->currentSurah, state->currentAyah);
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

    /* Arabic — right-aligned to the right margin, shrink to fit width */
    char vis[4096];
    int haveVisual = reorderArabic(ayah->arabicText, vis, sizeof(vis));
    Font af = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    float arSize = (float)S.fs34;
    float arSp = arSize * 0.12f;
    float arW = haveVisual
        ? MeasureTextEx(af, vis, arSize, arSp).x
        : MeasureTextEx(uiFont, ayah->arabicText, arSize, 1).x;
    float arMin = (float)S.fs14;
    while (arW > mainW - starInset && arSize > arMin) {
        arSize -= 2;
        if (arSize < arMin) arSize = arMin;
        arSp = arSize * 0.12f;
        arW = haveVisual
            ? MeasureTextEx(af, vis, arSize, arSp).x
            : MeasureTextEx(uiFont, ayah->arabicText, arSize, 1).x;
    }
    float arRight = (float)(rightMargin - starInset);
    float arLeft = arRight - arW;
    if (haveVisual)
        DrawTextEx(af, vis, (Vector2){arLeft, (float)my}, arSize, arSp, t->foreground);
    else
        DrawTextEx(uiFont, ayah->arabicText, (Vector2){arLeft, (float)my}, arSize, 1, t->foreground);

    char *translation = (strcmp(state->language, "bn") == 0)
                        ? ayah->translationBn : ayah->translationEn;
    drawWrappedText(translation,
                    (Rectangle){(float)mx, (float)(my + S.fs34 + S.gy),
                                (float)mainW, (float)(sh - FOOTER_H - my - S.fs34 - S.gy - S.fs13 - S.gy)},
                    S.fs16, t->muted);

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

    /* 1. Render full scene to texture */
    if (focusTarget.texture.id > 0) {
        BeginTextureMode(focusTarget);
            ClearBackground(t->background);
            drawTopBar(state);
            drawSidebar(state);
            drawFooter(state);
            drawAyahContent(state, t, sw, sh);
        EndTextureMode();

        /* 2. Box blur: draw texture 9 times with offsets at low alpha */
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
        Ayah *ayah = findMockAyah(state, state->currentSurah, state->currentAyah);
        if (ayah) {
            float innerPad = scaledRect.x + S.mx * S.factor;
            float innerW = scaledRect.width - 2 * S.mx * S.factor;
            float innerTop = scaledRect.y + S.cardPadY * S.factor;
            float innerBottom = scaledRect.y + scaledRect.height - S.cardPadY * S.factor;

            /* ── Arabic text — fit-to-width, then center ── */
            char vis[4096];
            int haveVisual = reorderArabic(ayah->arabicText, vis, sizeof(vis));
            Font af = arabicFont.texture.id > 0 ? arabicFont : uiFont;
            float arSize = S.fs42 * S.factor;
            float arSp = arSize * 0.12f;
            float arW = haveVisual
                ? MeasureTextEx(af, vis, arSize, arSp).x
                : MeasureTextEx(uiFont, ayah->arabicText, arSize, 1).x;
            float arMin = S.fs14 * S.factor;
            while (arW > innerW && arSize > arMin) {
                arSize -= 2 * S.factor;
                if (arSize < arMin) arSize = arMin;
                arSp = arSize * 0.12f;
                arW = haveVisual
                    ? MeasureTextEx(af, vis, arSize, arSp).x
                    : MeasureTextEx(uiFont, ayah->arabicText, arSize, 1).x;
            }
            float arY = innerTop + (innerBottom - innerTop) * 0.10f;
            if (haveVisual)
                drawArabicVisualCentered(vis,
                    (Rectangle){innerPad, arY, innerW, arSize * 1.6f},
                    arSize, t->foreground);
            else
                drawArabicTextCentered(ayah->arabicText,
                    (Rectangle){innerPad, arY, innerW, arSize * 1.6f},
                    arSize, t->foreground);

            /* ── Translation — wrapped below the Arabic ── */
            char *translation = (strcmp(state->language, "bn") == 0)
                                ? ayah->translationBn : ayah->translationEn;
            float refH = S.fs14 * S.factor;
            float hintH = S.fs12 * S.factor;
            float trTop = arY + arSize * 1.6f + S.gy * S.factor;
            float trBottom = innerBottom - (refH + S.gy * S.factor + hintH + S.gy * S.factor);
            drawWrappedText(translation,
                (Rectangle){innerPad, trTop, innerW, trBottom - trTop},
                S.fs16 * S.factor, t->muted);

            /* ── Reference + hint (pinned to bottom) ── */
            char ref[128];
            Surah *s = &state->surahs[state->currentSurah - 1];
            snprintf(ref, sizeof(ref), "%s: %d", s->name, state->currentAyah);
            float refW = MeasureTextEx(uiFont, ref, refH, 1).x;
            float refY = innerBottom - (refH + S.gy * S.factor + hintH);
            DrawTextEx(uiFont, ref, (Vector2){center.x - refW / 2, refY}, refH, 1, t->accent);

            const char *hint = "Press F to exit";
            float hintW = MeasureTextEx(uiFont, hint, hintH, 1).x;
            DrawTextEx(uiFont, hint, (Vector2){center.x - hintW / 2,
                       refY + refH + S.gy * S.factor}, hintH, 1, t->muted);
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
    snprintf(countStr, sizeof(countStr), "%d saved", mockBookmarkCount);
    DrawTextEx(uiFont, countStr, (Vector2){(float)(S.mx + S.fs22 + S.gx), (float)(titleY + (S.fs22 - S.fs14) / 2)}, S.fs14, 1, t->muted);

    int headerH = S.my + S.fs22 + S.gy/2;
    listY += headerH;
    listH -= headerH;

    DrawLine(S.mx, listY, sw - S.mx, listY, t->border);
    listY += S.gy/2;
    listH -= S.gy/2;

    if (mockBookmarkCount == 0) {
        const char *msg = "No bookmarks yet -- press b while reading to save your place.";
        int tw = MeasureTextEx(uiFont, msg, S.fs16, 1).x;
        DrawTextEx(uiFont, msg, (Vector2){(float)((sw - tw) / 2), (float)(listY + listH / 2 - S.fs16/2)}, S.fs16, 1, t->muted);
        return;
    }

    int rowH = S.bookmarkRowH;
    int visible = listH / rowH;
    static int bmScrollOff = 0;
    static int bmCursor = 0;

    if (bmCursor < 0) bmCursor = 0;
    if (bmCursor >= mockBookmarkCount) bmCursor = mockBookmarkCount - 1;
    if (bmCursor < bmScrollOff) bmScrollOff = bmCursor;
    if (bmCursor >= bmScrollOff + visible) bmScrollOff = bmCursor - visible + 1;
    if (mockBookmarkCount <= visible) bmScrollOff = 0;
    else if (bmScrollOff > mockBookmarkCount - visible) bmScrollOff = mockBookmarkCount - visible;
    if (bmScrollOff < 0) bmScrollOff = 0;

    for (int i = bmScrollOff; i < mockBookmarkCount && i < bmScrollOff + visible; i++) {
        int y = listY + (i - bmScrollOff) * rowH;
        int active = (i == bmCursor);

        if (active) {
            DrawRectangle(S.gx, y, sw - 2*S.gx, rowH, t->surface);
            DrawRectangleLines(S.gx, y, sw - 2*S.gx, rowH, t->border);
        }
        DrawLine(S.mx, y + rowH, sw - S.mx, y + rowH, t->border);

        Bookmark *bm = &mockBookmarks[i];

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

        if (bm->surahNumber >= 1 && bm->surahNumber <= 114 && state->surahs) {
            Surah *s = &state->surahs[bm->surahNumber - 1];
            if (s->number > 0)
                DrawTextEx(uiFont, s->name, (Vector2){(float)(tagX), (float)(y + S.gy/2 + S.fs16 + S.gy/4)}, S.fs13, 1, t->muted);
        }

        if (bm->note[0]) {
            char note[80];
            strncpy(note, bm->note, sizeof(note) - 1);
            note[sizeof(note) - 1] = '\0';
            if ((int)strlen(bm->note) > 75) {
                note[74] = '.'; note[75] = '.'; note[76] = '.'; note[77] = '\0';
            }
            DrawTextEx(uiFont, note, (Vector2){(float)(S.mx), (float)(y + S.gy/2 + S.fs16 + S.gy/4 + S.fs13 + S.gy/4)}, S.fs12, 1, t->muted);
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

    if (state->currentSurah < 1 || state->currentSurah > 114) return;
    Surah *s = &state->surahs[state->currentSurah - 1];
    if (s->number == 0) return;

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

    const char *prompt = "Press any key to begin reading";
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
        "Trigger Screensaver",
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
            case 8: val = "▶ Play"; break;
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
    snprintf(countStr, sizeof(countStr), "%d collections", state->totalHadiths);
    DrawTextEx(uiFont, countStr, (Vector2){(float)(S.mx + (int)(160 * S.factor)), (float)(titleY + (S.fs22 - S.fs14) / 2)}, S.fs14, 1, t->muted);

    int headerH = S.my + S.fs22 + S.gy/2;
    listY += headerH;
    listH -= headerH;

    DrawLine(S.mx, listY, sw - S.mx, listY, t->border);
    listY += S.gy/2;
    listH -= S.gy/2;

    if (state->totalHadiths <= 0 || !state->hadiths) {
        const char *msg = "No hadiths loaded.";
        int tw = MeasureTextEx(uiFont, msg, S.fs16, 1).x;
        DrawTextEx(uiFont, msg, (Vector2){(float)((sw - tw) / 2), (float)(listY + listH / 2 - S.fs16/2)}, S.fs16, 1, t->muted);
        return;
    }

    int rowH = (int)(80 * S.factor);
    int visible = listH / rowH;
    int total = state->totalHadiths;
    int cursor = state->hadithCursor;

    /* Clamp cursor */
    if (cursor < 0) { state->hadithCursor = 0; cursor = 0; }
    if (cursor >= total) { state->hadithCursor = total - 1; cursor = total - 1; }

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

        /* Card background */
        if (active)
            DrawRectangleRounded((Rectangle){(float)S.mx, (float)y,
                                            (float)(sw - 2 * S.mx), (float)(rowH - 4)},
                                 0.04f, 6, t->surface);

        /* Accent bar on active row */
        if (active)
            DrawRectangle(S.mx, y + (int)(6 * S.factor), (int)(3 * S.factor), rowH - (int)(16 * S.factor), t->accent);

        Hadith *h = &state->hadiths[i];

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

        /* Narrator */
        float narratorX = S.mx + S.gx + badgeW + S.gx;
        DrawTextEx(uiFont, h->narrator,
                   (Vector2){narratorX, (float)(y + S.gy/2 + 2)},
                   S.fs12, 1, t->muted);

        /* Hadith text — wrapped */
        int textY = y + S.gy/2 + (S.fs12 + 4) + S.gy/2;
        drawWrappedText(h->text,
            (Rectangle){(float)(S.mx + S.gx), (float)textY,
                        (float)(sw - 2 * S.mx - 2 * S.gx), (float)(rowH - textY + y - S.gy/2)},
            S.fs14, active ? t->foreground : t->muted);
    }
}

void drawTopBar(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    DrawRectangle(0, 0, sw, TOPBAR_H, t->surface);
    DrawLine(0, TOPBAR_H, sw, TOPBAR_H, t->border);
    DrawTextEx(uiFont, "Ayatika", (Vector2){(float)(S.mx - S.gx), (float)((TOPBAR_H - S.fs26)/2 + S.fs26/4)}, S.fs26, 1, t->accent);
    char buf[128];
    snprintf(buf, sizeof(buf), "Next: %s", state->prayer.dhuhrStr[0] ? state->prayer.dhuhrStr : "--:--");
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
    for (int i = scrollOff; i < total && i < scrollOff + visible; i++) {
        int y = listY + (i - scrollOff) * rowH;
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
        Color dc = active ? t->background :
                   (strcmp(s->revelationType, "Meccan") == 0 ? t->accent : t->border);
        DrawCircle(dotX, dotY, S.dotRadius, dc);
    }
}

void drawFooter(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, sh - FOOTER_H, sw, FOOTER_H, t->surface);
    DrawLine(0, sh - FOOTER_H, sw, sh - FOOTER_H, t->border);
    DrawTextEx(uiFont, state->statusMsg, (Vector2){(float)(S.mx - S.gx), (float)(sh - FOOTER_H + (FOOTER_H - S.fs14)/2)}, S.fs14, 1, t->muted);
    const char *helpRight = state->vimMotions
        ? "F1 = help   j/k/h/l = navigate   Enter = open   / = search   m = bookmarks"
        : "F1 = help   arrows = navigate   Enter = open   / = search   m = bookmarks";
    DrawTextEx(uiFont, helpRight, (Vector2){(float)(sw - (int)(520 * S.factor)), (float)(sh - FOOTER_H + (FOOTER_H - S.fs13)/2)}, S.fs13, 1, t->muted);
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
        {"Enter",     "Open selected item"},
        {"Esc",       "Go back"},
        {"/",         "Open search"},
        {"b",         "Bookmark current ayah (Reader)"},
        {"m",         "Open bookmarks"},
        {"f",         "Toggle focus mode (Reader)"},
        {"t",         "Cycle themes"},
        {"s",         "Open settings"},
        {"Home",      "Go to dashboard"},
        {"F1",        "Toggle this help"},
    };
    static const char *arrowKeys[][2] = {
        {"Up / Down",    "Move cursor up / down"},
        {"Left / Right", "Navigate / scroll"},
        {"PgUp / PgDn",  "Go to top / bottom"},
        {"Enter",        "Open selected item"},
        {"Esc",          "Go back"},
        {"/",            "Open search"},
        {"b",            "Bookmark current ayah (Reader)"},
        {"m",            "Open bookmarks"},
        {"f",            "Toggle focus mode (Reader)"},
        {"t",            "Cycle themes"},
        {"s",            "Open settings"},
        {"Home",         "Go to dashboard"},
        {"F1",           "Toggle this help"},
    };
    const char *helpClose = "Press F1 to close";

    if (state->vimMotions) {
        int vrows = (int)(sizeof(vimKeys) / sizeof(vimKeys[0]));
        for (int i = 0; i < vrows; i++) {
            int y = cy + (int)(58 * S.factor) + i * (int)(30 * S.factor);
            DrawTextEx(uiFont, vimKeys[i][0], (Vector2){(float)(cx + (int)(24 * S.factor)), (float)(y)}, S.fs14, 1, t->accent);
            DrawTextEx(uiFont, vimKeys[i][1], (Vector2){(float)(cx + (int)(150 * S.factor)), (float)(y)}, S.fs14, 1, t->foreground);
        }
    } else {
        int arows = (int)(sizeof(arrowKeys) / sizeof(arrowKeys[0]));
        for (int i = 0; i < arows; i++) {
            int y = cy + (int)(58 * S.factor) + i * (int)(30 * S.factor);
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

    const char *msg = "Bookmark saved";
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
    FriBidiChar logical[2048];
    FriBidiStrIndex len = fribidi_charset_to_unicode(
        FRIBIDI_CHAR_SET_UTF8, text, strlen(text), logical);
    if (len <= 0) { visualOut[0] = '\0'; return 0; }
    if (len >= 2048) len = 2047;

    FriBidiChar visual[2048];
    FriBidiParType baseDir = FRIBIDI_PAR_RTL;
    FriBidiLevel levels[2048];
    FriBidiStrIndex map[2048];
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
    char visual[4096];
    const char *src = reorderArabic(text, visual, sizeof(visual)) ? visual : text;
    drawArabicVisualCentered(src, bounds, size, color);
}

void drawArabicVisualCentered(const char *visualText, Rectangle bounds, float size, Color color) {
    Font f = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    float sp = size * 0.12f;
    float tw = MeasureTextEx(f, visualText, size, sp).x;
    Vector2 pos = {bounds.x + (bounds.width - tw) / 2, bounds.y};
    DrawTextEx(f, visualText, pos, size, sp, color);
}
