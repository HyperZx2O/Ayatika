#include <raylib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fribidi/fribidi.h>
#include "ui.h"
#include "theme.h"

Font arabicFont;
Font uiFont;

void initFonts(void) {
    arabicFont = LoadFontEx("assets/Amiri.ttf", 48, NULL, 0x10FFFF);
    if (arabicFont.texture.id > 0)
        SetTextureFilter(arabicFont.texture, TEXTURE_FILTER_BILINEAR);
    else
        arabicFont = GetFontDefault(); /* fallback if file missing */
    uiFont = GetFontDefault();
}

void closeFonts(void) {
    if (arabicFont.texture.id > 0 && arabicFont.texture.id != uiFont.texture.id)
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
        case SCREEN_SEARCH:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            DrawRectangle(sw/2-200, sh/2-20, 400, 40, t->surface);
            DrawRectangleLines(sw/2-200, sh/2-20, 400, 40, t->accent);
            DrawText("[Search screen — built by Systems & Features role]",
                     sw/2-190, sh/2-8, 14, t->muted);
            break;
        case SCREEN_BOOKMARKS:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawBookmarks(state);
            break;
        case SCREEN_SCREENSAVER:
            ClearBackground(BLACK);
            DrawText("[Screensaver — built by Systems & Features role]",
                     sw/2-170, sh/2-8, 14, t->muted);
            break;
        case SCREEN_SURAH_OVERVIEW:
            ClearBackground(t->background);
            DrawRectangle(0, 0, sw, sh, t->background);
            drawSurahOverview(state);
            break;
    }
}

/* forward declarations for static helpers */
static void nextPrayerInfo(AppState *state, char *name, int nameSz, char *countdown, int cdSz, float *progress);

static Rectangle cardRect(int index) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int mx = 40, my = 40, gx = 20, gy = 20;
    int ch = sh - TOPBAR_H - FOOTER_H;
    int cw = (sw - 2*mx - gx) / 2;
    int cardH = (ch - 2*my - 2*gy) / 3;
    int x0 = mx, y0 = TOPBAR_H + my;
    int row = index / 2, col = index % 2;
    return (Rectangle){x0 + col*(cw+gx), y0 + row*(cardH+gy), cw, cardH};
}

/* ── Bookmark popup timer ── */
static double bookmarkPopupTime = 0;

void showBookmarkPopup(void) {
    bookmarkPopupTime = GetTime();
}

static int isBookmarked(int surah, int ayah) {
    for (int i = 0; i < mockBookmarkCount; i++)
        if (mockBookmarks[i].surahNumber == surah &&
            mockBookmarks[i].ayahNumber == ayah)
            return 1;
    return 0;
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
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    drawTopBar(state);
    int mx = 40, my = 40, gx = 20, gy = 20;
    int ch = sh - TOPBAR_H - FOOTER_H;
    int cw = (sw - 2*mx - gx) / 2;
    int cardH = (ch - 2*my - 2*gy) / 3;
    /* signature data-bar colors (theme-invariant) */
    const Color violet = {140, 108, 217, 255};
    /* card backgrounds + borders */
    Rectangle cards[6];
    for (int i = 0; i < 6; i++) {
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
        int px = r.x + 12, py = r.y + 20;
        DrawText("GREETING", px, py, 12, t->muted); py += 16;
        time_t now = time(NULL);
        struct tm *lt = localtime(&now);
        const char *greet = "Good evening";
        if (lt->tm_hour >= 5 && lt->tm_hour < 12) greet = "Good morning";
        else if (lt->tm_hour >= 12 && lt->tm_hour < 17) greet = "Good afternoon";
        DrawText(greet, px, py + 8, 18, t->foreground); py += 28;
        char sub[128];
        snprintf(sub, sizeof(sub), "You're in %s", state->surahs[state->currentSurah-1].name);
        DrawText(sub, px, py + 4, 14, t->muted);
    }
    /* CLOCK */
    {
        Rectangle r = cards[1];
        int px = r.x + 12, py = r.y + 20;
        DrawText("CLOCK", px, py, 12, t->muted); py += 12;
        time_t now = time(NULL);
        struct tm *lt = localtime(&now);
        char tbuf[16], dbuf[64];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
        static const char *wd[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        static const char *mo[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        snprintf(dbuf, sizeof(dbuf), "%s, %s %d", wd[lt->tm_wday], mo[lt->tm_mon], lt->tm_mday);
        float tw = MeasureText(tbuf, 40);
        DrawText(tbuf, r.x + (r.width - tw)/2, py + 10, 40, t->accent);
        tw = MeasureText(dbuf, 14);
        DrawText(dbuf, r.x + (r.width - tw)/2, py + 58, 14, t->muted);
    }
    /* WAQT */
    {
        Rectangle r = cards[2];
        int px = r.x + 12, py = r.y + 20;
        DrawText("WAQT", px, py, 12, t->muted); py += 16;
        char name[32], cd[16]; float prog;
        nextPrayerInfo(state, name, sizeof(name), cd, sizeof(cd), &prog);
        char line[64];
        snprintf(line, sizeof(line), "%s — %s", name, cd);
        DrawText(line, px, py + 4, 16, t->foreground); py += 24;
        int barW = cw - 24;
        DrawRectangleRounded((Rectangle){px, py, (float)barW, 8}, 0.3f, 4, t->border);
        DrawRectangleRounded((Rectangle){px, py, (float)barW * prog, 8}, 0.3f, 4, violet);
        py += 20;
        snprintf(line, sizeof(line), "Fajr %s    Dhuhr %s", state->prayer.fajrStr, state->prayer.dhuhrStr);
        DrawText(line, px, py, 12, t->muted); py += 16;
        snprintf(line, sizeof(line), "Asr %s    Maghrib %s", state->prayer.asrStr, state->prayer.maghribStr);
        DrawText(line, px, py, 12, t->muted); py += 16;
        snprintf(line, sizeof(line), "Isha %s", state->prayer.ishaStr);
        DrawText(line, px, py, 12, t->muted);
    }
    /* AYAH OF THE DAY */
    {
        Rectangle r = cards[3];
        int w = cw - 24;
        int da = getDailyAyahIndex();
        Ayah *a = NULL;
        if (da < state->totalAyahs) a = &state->ayahs[da];
        DrawText("AYAH OF THE DAY", r.x + 12, r.y + 20, 12, t->muted);
        if (a) {
            drawArabicTextCentered(a->arabicText,
                (Rectangle){r.x + 12, r.y + 44, (float)w, (float)(cardH - 80)},
                26, t->foreground);
            char trans[80];
            strncpy(trans, a->translationEn, sizeof(trans)-1);
            trans[sizeof(trans)-1] = '\0';
            int tl = strlen(trans);
            if (tl > 55) { trans[54] = '.'; trans[55] = '.'; trans[56] = '.'; trans[57] = '\0'; }
            DrawTextEx(uiFont, trans, (Vector2){r.x + 12, r.y + (float)cardH - 48}, 13, 1, t->muted);
            char ref[32];
            snprintf(ref, sizeof(ref), "%d:%d — %s", a->surahNumber, a->ayahNumber,
                     state->surahs[a->surahNumber-1].name);
            float tw = MeasureText(ref, 12);
            DrawText(ref, r.x + cw - 12 - tw, r.y + (float)cardH - 28, 12, t->accent);
        }
    }
    /* HADITH */
    {
        Rectangle r = cards[4];
        int px = r.x + 12, py = r.y + 20;
        DrawText("HADITH", px, py, 12, t->muted); py += 16;
        if (state->totalHadiths > 0) {
            Hadith *h = &state->hadiths[0];
            DrawText(h->name, px, py + 2, 13, t->muted); py += 18;
            char txt[200];
            strncpy(txt, h->text, sizeof(txt)-1);
            txt[sizeof(txt)-1] = '\0';
            if (strlen(h->text) > sizeof(txt)-5) { strcat(txt, "..."); }
            DrawText(txt, px, py + 2, 14, t->foreground); py += 22;
            float tw = MeasureText(h->collection, 12);
            DrawText(h->collection, px + cw - 24 - tw, py + 18, 12, t->accent);
        }
    }
    /* CONTINUE READING */
    {
        Rectangle r = cards[5];
        int px = r.x + 12, py = r.y + 20;
        DrawText("CONTINUE READING", px, py, 12, t->muted); py += 16;
        int sn = state->currentSurah, an = state->currentAyah;
        char ln[64];
        snprintf(ln, sizeof(ln), "%s, Ayah %d", state->surahs[sn-1].name, an);
        DrawText(ln, px, py + 14, 20, t->accent); py += 30;
        DrawText("bookmarked recently", px, py + 4, 12, t->muted); py += 20;
        DrawText("Press Enter to resume", px, py + 4, 14, t->foreground);
    }
    drawFooter(state);
}

void drawSurahList(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    drawTopBar(state);
    drawSidebar(state);
    int px = SIDEBAR_W + 20;
    DrawText("Select a surah to begin reading", px, TOPBAR_H + 20, 18, t->muted);
    drawFooter(state);
}

void drawAyahReader(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    drawTopBar(state);
    drawSidebar(state);
    drawFooter(state);

    int mx = SIDEBAR_W + 40;
    int my = TOPBAR_H + 30;
    int mainW = sw - SIDEBAR_W - 80;

    if (state->focusMode)
        DrawRectangle(SIDEBAR_W, TOPBAR_H, sw - SIDEBAR_W,
                      sh - TOPBAR_H - FOOTER_H, (Color){0, 0, 0, 160});

    Ayah *ayah = findMockAyah(state, state->currentSurah, state->currentAyah);
    if (!ayah) {
        const char *msg = "No ayah loaded for this reference";
        int tw = MeasureText(msg, 16);
        DrawText(msg, (sw - tw) / 2, sh / 2, 16, t->muted);
        return;
    }

    /* Bookmark indicator — star glyph, top-right of main panel */
    if (isBookmarked(state->currentSurah, state->currentAyah))
        DrawText("\xe2\x98\x85", sw - 45, my - 2, 20, t->accent);

    /* Arabic text — right-aligned */
    drawArabicText(ayah->arabicText,
                   (Vector2){(float)(sw - 60), (float)my},
                   34, t->foreground);

    /* Translation — word-wrapped within main panel */
    char *translation = (strcmp(state->language, "bn") == 0)
                        ? ayah->translationBn : ayah->translationEn;
    drawWrappedText(translation,
                    (Rectangle){(float)mx, (float)(my + 90),
                                (float)mainW, (float)(sh - FOOTER_H - my - 120)},
                    16, t->muted);

    /* Reference — bottom-left of main panel */
    char ref[32];
    snprintf(ref, sizeof(ref), "%d:%d", state->currentSurah, state->currentAyah);
    DrawText(ref, mx, sh - FOOTER_H - 30, 13, t->muted);
}

void drawBookmarks(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    drawTopBar(state);
    drawFooter(state);

    int listY = TOPBAR_H;
    int listH = sh - TOPBAR_H - FOOTER_H;

    if (mockBookmarkCount == 0) {
        const char *msg = "No bookmarks yet -- press b while reading to save your place.";
        int tw = MeasureText(msg, 16);
        DrawText(msg, (sw - tw) / 2, listY + listH / 2 - 8, 16, t->muted);
        return;
    }

    int rowH = 58;
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
            DrawRectangle(20, y, sw - 40, rowH, t->surface);
            DrawRectangleLines(20, y, sw - 40, rowH, t->border);
        }
        DrawLine(40, y + rowH, sw - 40, y + rowH, t->border);

        Bookmark *bm = &mockBookmarks[i];

        /* Reference */
        char ref[32];
        snprintf(ref, sizeof(ref), "%d:%d", bm->surahNumber, bm->ayahNumber);
        DrawText(ref, 40, y + 8, 16, t->accent);

        /* Tag */
        const char *tag = bm->tag[0] ? bm->tag : "(untagged)";
        Color tagColor = bm->tag[0] ? t->foreground : t->muted;
        DrawText(tag, 120, y + 8, 16, tagColor);

        /* Surah name */
        if (bm->surahNumber >= 1 && bm->surahNumber <= 114 && state->surahs) {
            Surah *s = &state->surahs[bm->surahNumber - 1];
            if (s->number > 0)
                DrawText(s->name, 120, y + 28, 13, t->muted);
        }

        /* Note (truncated) */
        if (bm->note[0]) {
            char note[80];
            strncpy(note, bm->note, sizeof(note) - 1);
            note[sizeof(note) - 1] = '\0';
            if ((int)strlen(bm->note) > 75) {
                note[74] = '.'; note[75] = '.'; note[76] = '.'; note[77] = '\0';
            }
            DrawText(note, 40, y + 44, 12, t->muted);
        }
    }
}

void drawSurahOverview(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    /* Full-screen scrim */
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 200});

    /* Centered card */
    int cw = 600, ch = 320;
    int cx = (sw - cw) / 2, cy = (sh - ch) / 2;
    DrawRectangleRounded((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                         0.08f, 8, t->surface);
    DrawRectangleRoundedLines((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                              0.08f, 8, t->border);

    if (state->currentSurah < 1 || state->currentSurah > 114) return;
    Surah *s = &state->surahs[state->currentSurah - 1];
    if (s->number == 0) return;

    /* Arabic name — centered */
    drawArabicTextCentered(s->arabicName,
        (Rectangle){(float)cx, (float)(cy + 20), (float)cw, 80},
        42, t->accent);

    /* English name — centered */
    float nameW = MeasureText(s->name, 22);
    DrawText(s->name, cx + (cw - (int)nameW) / 2, cy + 110, 22, t->foreground);

    /* Badges */
    DrawRectangleRounded((Rectangle){(float)(cx + 60), (float)(cy + 148), 100, 24},
                         0.4f, 4, t->accent);
    DrawText(s->revelationType, cx + 68, cy + 153, 14, t->background);
    char cnt[32];
    snprintf(cnt, sizeof(cnt), "%d Ayahs", s->ayahCount);
    DrawRectangleRounded((Rectangle){(float)(cx + 180), (float)(cy + 148), 90, 24},
                         0.4f, 4, t->border);
    DrawText(cnt, cx + 190, cy + 153, 14, t->foreground);

    /* Context blurb — word-wrapped */
    drawWrappedText(s->context,
        (Rectangle){(float)(cx + 30), (float)(cy + 192), (float)(cw - 60), 80},
        14, t->muted);

    /* Footer prompt */
    const char *prompt = "Press any key to begin reading";
    float pw = MeasureText(prompt, 13);
    DrawText(prompt, cx + (cw - (int)pw) / 2, cy + ch - 30, 13, t->muted);
}

void drawTopBar(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    DrawRectangle(0, 0, sw, TOPBAR_H, t->surface);
    DrawLine(0, TOPBAR_H, sw, TOPBAR_H, t->border);
    DrawText("Ayatika", 20, 16, 22, t->accent);
    char buf[128];
    snprintf(buf, sizeof(buf), "Next: %s", state->prayer.dhuhrStr[0] ? state->prayer.dhuhrStr : "--:--");
    DrawText(buf, sw - 220, 18, 16, t->foreground);
}

void drawSidebar(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sh = GetScreenHeight();
    int listY = TOPBAR_H, listH = sh - TOPBAR_H - FOOTER_H;
    int rowH = 48;
    DrawRectangle(0, listY, SIDEBAR_W, listH, t->surface);
    int total = 0;
    for (int i = 0; i < 114 && state->surahs[i].number > 0; i++) total++;
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
        DrawText(num, 12, y + 6, 14, active ? t->background : t->muted);
        DrawText(s->name, 48, y + 5, 16, active ? t->background : t->foreground);
        char cnt[16];
        snprintf(cnt, sizeof(cnt), "%d ayahs", s->ayahCount);
        DrawText(cnt, 48, y + 24, 12, active ? t->background : t->muted);
        int dotX = SIDEBAR_W - 20, dotY = y + rowH/2;
        Color dc = active ? t->background :
                   (strcmp(s->revelationType, "Meccan") == 0 ? t->accent : t->border);
        DrawCircle(dotX, dotY, 4, dc);
    }
}

void drawFooter(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, sh - FOOTER_H, sw, FOOTER_H, t->surface);
    DrawLine(0, sh - FOOTER_H, sw, sh - FOOTER_H, t->border);
    DrawText(state->statusMsg, 20, sh - FOOTER_H + 12, 14, t->muted);
    DrawText("F1 = help   j/k/h/l = navigate   Enter = open   / = search   m = bookmarks",
             sw - 520, sh - FOOTER_H + 12, 13, t->muted);
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

void drawWaqtPanel(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int marginX = 40, marginY = 40, gutterX = 20, gutterY = 20;
    int contentH = sh - TOPBAR_H - FOOTER_H;
    int cardW = (sw - 2*marginX - gutterX) / 2;
    int cardH = (contentH - 2*marginY - 2*gutterY) / 3;
    int gridX = marginX;
    int gridY = TOPBAR_H + marginY;
    int cx = gridX + 1 * (cardW + gutterX);
    int cy = gridY + 1 * (cardH + gutterY);
    DrawRectangleRounded((Rectangle){cx, cy, cardW, cardH}, 0.06f, 8, t->surface);
    DrawRectangleRoundedLinesEx((Rectangle){cx, cy, cardW, cardH}, 0.06f, 8, 1, t->border);
    int px = cx + 12, py = cy + 20;
    DrawText("WAQT", px, py, 12, t->muted);
    py += 20;
    char name[32], cd[16];
    float prog;
    nextPrayerInfo(state, name, sizeof(name), cd, sizeof(cd), &prog);
    char line[64];
    snprintf(line, sizeof(line), "%s — %s", name, cd);
    DrawText(line, px, py + 4, 16, t->foreground);
    py += 24;
    const Color violet = {140, 108, 217, 255};
    int barW = cardW - 24;
    DrawRectangleRounded((Rectangle){px, py, (float)barW * prog, 8}, 0.3f, 4, violet);
    DrawRectangleRounded((Rectangle){px, py, (float)barW, 8}, 0.3f, 4, t->border);
    DrawRectangleRounded((Rectangle){px, py, (float)barW * prog, 8}, 0.3f, 4, violet);
    py += 20;
    snprintf(line, sizeof(line), "Fajr %s    Dhuhr %s", state->prayer.fajrStr, state->prayer.dhuhrStr);
    DrawText(line, px, py, 12, t->muted);
    py += 16;
    snprintf(line, sizeof(line), "Asr %s    Maghrib %s", state->prayer.asrStr, state->prayer.maghribStr);
    DrawText(line, px, py, 12, t->muted);
    py += 16;
    snprintf(line, sizeof(line), "Isha %s", state->prayer.ishaStr);
    DrawText(line, px, py, 12, t->muted);
}

void drawHelpOverlay(AppState *state) {
    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 180});

    int cw = 520, ch = 420;
    int cx = (sw - cw) / 2, cy = (sh - ch) / 2;
    DrawRectangleRounded((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                         0.06f, 8, t->surface);
    DrawRectangleRoundedLines((Rectangle){(float)cx, (float)cy, (float)cw, (float)ch},
                              0.06f, 8, t->border);

    DrawText("Keyboard Shortcuts", cx + 20, cy + 16, 18, t->accent);
    DrawLine(cx + 20, cy + 42, cx + cw - 20, cy + 42, t->border);

    static const char *keys[][2] = {
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
        {"Home",      "Go to dashboard"},
        {"F1",        "Toggle this help"},
    };
    int rows = (int)(sizeof(keys) / sizeof(keys[0]));
    for (int i = 0; i < rows; i++) {
        int y = cy + 58 + i * 30;
        DrawText(keys[i][0], cx + 24, y, 14, t->accent);
        DrawText(keys[i][1], cx + 150, y, 14, t->foreground);
    }

    const char *close = "Press F1 to close";
    float cw2 = MeasureText(close, 13);
    DrawText(close, cx + (cw - (int)cw2) / 2, cy + ch - 28, 13, t->muted);
}

void drawBookmarkPopup(AppState *state) {
    if (bookmarkPopupTime <= 0) return;
    double elapsed = GetTime() - bookmarkPopupTime;
    if (elapsed > 2.0) { bookmarkPopupTime = 0; return; }

    Theme *t = getTheme(state->currentTheme);
    int sw = GetScreenWidth();
    float alpha = elapsed < 1.5 ? 1.0f : 1.0f - (float)((elapsed - 1.5) / 0.5);

    const char *msg = "Bookmark saved";
    int tw = MeasureText(msg, 16);
    int pw = tw + 40, ph = 36;
    int px = (sw - pw) / 2, py = 70;

    Color bg = t->surface; bg.a = (unsigned char)(200 * alpha);
    Color fg = t->accent;  fg.a = (unsigned char)(255 * alpha);

    DrawRectangleRounded((Rectangle){(float)px, (float)py, (float)pw, (float)ph},
                         0.3f, 4, bg);
    DrawText(msg, px + 20, py + 10, 16, fg);
}

void drawFocusDim(AppState *state) {
    if (!state->focusMode) return;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(SIDEBAR_W, TOPBAR_H, sw - SIDEBAR_W,
                  sh - TOPBAR_H - FOOTER_H, (Color){0, 0, 0, 160});
}

static int reorderArabic(const char *text, char *visualOut, int outSize) {
    (void)outSize;
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

    fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual, len, visualOut);
    return 1;
}

void drawArabicText(const char *text, Vector2 pos, float size, Color color) {
    char visual[4096];
    if (!reorderArabic(text, visual, sizeof(visual))) {
        DrawTextEx(uiFont, text, pos, size, 1, color);
        return;
    }
    Font f = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    DrawTextEx(f, visual, pos, size, 1, color);
}

void drawArabicTextCentered(const char *text, Rectangle bounds, float size, Color color) {
    char visual[4096];
    if (!reorderArabic(text, visual, sizeof(visual))) {
        float tw = MeasureTextEx(uiFont, text, size, 1).x;
        Vector2 pos = {bounds.x + (bounds.width - tw) / 2, bounds.y};
        DrawTextEx(uiFont, text, pos, size, 1, color);
        return;
    }
    Font f = arabicFont.texture.id > 0 ? arabicFont : uiFont;
    float tw = MeasureTextEx(f, visual, size, 1).x;
    Vector2 pos = {bounds.x + (bounds.width - tw) / 2, bounds.y};
    DrawTextEx(f, visual, pos, size, 1, color);
}
