#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "input.h"
#include "theme.h"
#include "ui.h"
#include "audio.h"
#include "screensaver.h"
#include "search.h"

/* ── Surah-list digit jump (digits only — letters open the palette, so no
 *    keybinding conflicts by construction). ── */
static char listJumpBuf[16] = "";
static void listJumpClear(void) { listJumpBuf[0] = '\0'; }
const char *getListJumpBuf(void) { return listJumpBuf; }

/* Bookmark tag editor state (size mirrors struct Bookmark) */
static int   bmEditing = 0;
static char  bmTag[128] = "";

/* IsKeyPressed is edge-only, so held keys never repeat. This gives
   nav keys OS-style repeat: instant on press, then 25/s after 0.45s held.
   Single-key tracker — last pressed wins, good enough for cursor movement. */
int navRepeat(int key) {
    static int tracked = -1;
    static double next = 0;
    if (IsKeyPressed(key)) {
        tracked = key;
        next = GetTime() + 0.45;
        return 1;
    }
    if (tracked == key) {
        if (IsKeyDown(key)) {
            if (GetTime() >= next) {
                next = GetTime() + 1.0 / 25.0;
                return 1;
            }
        } else tracked = -1;
    }
    return 0;
}

/* nothing drains the char queue on non-text screens anymore, so
   flush leftovers on every transition — else a stale 't' pops into the
   next jump box or palette. */
static void drainCharQueue(void) {
    while (GetCharPressed() > 0) {}
}

/* ── Helper: save current screen before transitioning ── */
static void pushScreen(AppState *state, AppScreen target) {
    listJumpClear();
    drainCharQueue();
    state->previousScreen = state->currentScreen;
    state->currentScreen = target;
}

/* ── Helpers over live API data (no mock) ── */
static int surahIndexByNumber(AppState *state, int surahNum) {
    if (!state || !state->surahs) return -1;
    for (int i = 0; i < state->surahCount; i++)
        if (state->surahs[i].number == surahNum) return i;
    return -1;
}

static int ayahCountForSurah(AppState *state, int surahNum) {
    int idx = surahIndexByNumber(state, surahNum);
    if (idx < 0) return 0;
    return state->surahs[idx].ayahCount;
}

/* ============================================================
 * ACTION FUNCTIONS
 * ============================================================ */

static void moveCursorDown(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            if (state->dashboardCursor < 4) state->dashboardCursor++;
            break;
        case SCREEN_READING_HUB:
            if (state->hubCursor < 1) state->hubCursor++;
            break;
        case SCREEN_SURAH_LIST: {
            if (state->surahCount > 0 && state->cursorSurah < state->surahCount - 1) state->cursorSurah++;
            break;
        }
        case SCREEN_AYAH_READER: {
            int max = ayahCountForSurah(state, state->currentSurah);
            if (max > 0 && state->currentAyah < max) state->currentAyah++;
            break;
        }
        case SCREEN_HADITH:
            if (state->hadithCursor < state->totalHadiths - 1) state->hadithCursor++;
            break;
        case SCREEN_BOOKMARKS: {
            /* count via DB; keypresses are rare, query is tiny. */
            Bookmark tmp[256];
            int n = loadBookmarks(tmp, 256);
            if (n > 0 && state->cursorSurah < n - 1) state->cursorSurah++;
            break;
        }
        default: break;
    }
}

static void moveCursorUp(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            if (state->dashboardCursor > 0) state->dashboardCursor--;
            break;
        case SCREEN_READING_HUB:
            if (state->hubCursor > 0) state->hubCursor--;
            break;
        case SCREEN_SURAH_LIST:
            if (state->cursorSurah > 0) state->cursorSurah--;
            break;
        case SCREEN_AYAH_READER:
            if (state->currentAyah > 1) state->currentAyah--;
            break;
        case SCREEN_HADITH:
            if (state->hadithCursor > 0) state->hadithCursor--;
            break;
        case SCREEN_BOOKMARKS:
            if (state->cursorSurah > 0) state->cursorSurah--;
            break;
        default: break;
    }
}

static void moveCursorRight(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            if (state->dashboardCursor < 4) state->dashboardCursor++;
            break;
        case SCREEN_READING_HUB:
            if (state->hubCursor < 1) state->hubCursor++;
            break;
        case SCREEN_SURAH_LIST:
        case SCREEN_AYAH_READER: {
            if (state->surahCount > 0 && state->cursorSurah < state->surahCount - 1) state->cursorSurah++;
            break;
        }
        default: break;
    }
}

static void moveCursorLeft(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            if (state->dashboardCursor > 0) state->dashboardCursor--;
            break;
        case SCREEN_READING_HUB:
            if (state->hubCursor > 0) state->hubCursor--;
            break;
        case SCREEN_SURAH_LIST:
        case SCREEN_AYAH_READER:
            if (state->cursorSurah > 0) state->cursorSurah--;
            break;
        default: break;
    }
}

static void goToTop(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD: state->dashboardCursor = 0; break;
        case SCREEN_READING_HUB: state->hubCursor = 0; break;
        case SCREEN_SURAH_LIST: state->cursorSurah = 0; break;
        case SCREEN_AYAH_READER: state->currentAyah = 1; break;
        case SCREEN_HADITH: state->hadithCursor = 0; break;
        default: break;
    }
}

static void goToBottom(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD: state->dashboardCursor = 4; break;
        case SCREEN_READING_HUB: state->hubCursor = 1; break;
        case SCREEN_SURAH_LIST:
            if (state->surahCount > 0) state->cursorSurah = state->surahCount - 1;
            break;
        case SCREEN_AYAH_READER: {
            int max = ayahCountForSurah(state, state->currentSurah);
            if (max > 0) state->currentAyah = max;
            break;
        }
        case SCREEN_HADITH: state->hadithCursor = state->totalHadiths - 1; break;
        default: break;
    }
}

static void goBack(AppState *state);

static void openSelected(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            pushScreen(state, SCREEN_READING_HUB);
            state->hubCursor = 0;
            break;
        case SCREEN_READING_HUB:
            if (state->hubCursor == 0) {
                /* Surah tile → surah overview */
                pushScreen(state, SCREEN_SURAH_OVERVIEW);
            } else {
                /* Hadith tile → hadith page */
                state->hadithCursor = 0;
                pushScreen(state, SCREEN_HADITH);
            }
            break;
        case SCREEN_SURAH_LIST:
            if (state->cursorSurah >= 0 && state->cursorSurah < state->surahCount) {
                state->currentSurah = state->surahs[state->cursorSurah].number;
                pushScreen(state, SCREEN_SURAH_OVERVIEW);
            }
            break;
        case SCREEN_SURAH_OVERVIEW:
            /* Don't use pushScreen — preserves previousScreen so
               Esc from the reader goes back to Reading Hub, not in a loop. */
            state->currentScreen = SCREEN_AYAH_READER;
            state->currentAyah = 1;
            state->cursorSurah = surahIndexByNumber(state, state->currentSurah);
            break;
        case SCREEN_AYAH_READER: {
            int target = -1;
            if (state->cursorSurah >= 0 && state->cursorSurah < state->surahCount)
                target = state->surahs[state->cursorSurah].number;
            if (target > 0 && target != state->currentSurah) {
                state->currentSurah = target;
                pushScreen(state, SCREEN_SURAH_OVERVIEW);
            }
            break;
        }
        case SCREEN_BOOKMARKS: {
            /* direct DB read; no bookmark cache layer. */
            Bookmark rows[64];
            int n = loadBookmarks(rows, 64);
            if (n > 0) {
                int idx = (state->cursorSurah < n) ? state->cursorSurah : 0;
                if (idx < 0) idx = 0;
                state->currentSurah = rows[idx].surahNumber;
                state->currentAyah = rows[idx].ayahNumber;
                state->cursorSurah = surahIndexByNumber(state, state->currentSurah);
                pushScreen(state, SCREEN_AYAH_READER);
            }
            break;
        }
        case SCREEN_HADITH:
            /* Enter on hadith page: jump to the surah referenced or just go back */
            goBack(state);
            break;
        default: break;
    }
}

static void goBack(AppState *state) {
    listJumpClear();
    drainCharQueue();
    if (state->currentScreen == SCREEN_DASHBOARD ||
        state->currentScreen == SCREEN_READING_HUB) {
        state->currentScreen = SCREEN_DASHBOARD;
        return;
    }
    state->currentScreen = state->previousScreen;
    if (state->currentScreen == SCREEN_AYAH_READER && state->currentAyah < 1)
        state->currentAyah = 1;
}

static void openFinder(AppState *state) {
    /* overlay opens on the surah tab; / switches after. */
    listJumpClear();
    drainCharQueue();
    state->paletteQuery[0] = '\0';
    state->paletteSelection = 0;
    state->paletteMode = 0;
    state->searchQuery[0] = '\0';
    state->searchResultCount = 0;
    state->showGoToPalette = 1;
}

static void openSearch(AppState *state) {
    openFinder(state);
    state->paletteMode = 1;
}

static void openBookmarks(AppState *state) {
    pushScreen(state, SCREEN_BOOKMARKS);
    state->cursorSurah = 0;
}

static void addBookmark(AppState *state) {
    if (state->currentScreen != SCREEN_AYAH_READER) return;
    if (bookmarkExists(state->currentSurah, state->currentAyah)) {
        setStatus(state, 0, "Already bookmarked: %d:%d", state->currentSurah, state->currentAyah);
        return;
    }
    /* same inline-editor pattern as latitude — statics + seams. */
    bmEditing = 1;
    bmTag[0] = '\0';
    drainCharQueue();
}

int  isEditingBookmark(void) { return bmEditing; }
const char *getBookmarkTag(void) { return bmTag; }

static void bookmarkEditorConfirm(AppState *state) {
    bmEditing = 0;
    Bookmark bm;
    memset(&bm, 0, sizeof(bm));
    bm.surahNumber = state->currentSurah;
    bm.ayahNumber = state->currentAyah;
    snprintf(bm.tag, sizeof(bm.tag), "%s", bmTag);
    /* first bookmark ever also teaches retrieval — fires at 0→1 only. */
    Bookmark probe[1];
    int firstEver = (loadBookmarks(probe, 1) == 0);
    if (saveBookmark(&bm)) {
        showBookmarkPopupRef(state->currentSurah, state->currentAyah);
        if (firstEver)
            setStatus(state, 0, "First bookmark saved — press m to revisit");
        else
            setStatus(state, 0, "Bookmarked %d:%d", state->currentSurah, state->currentAyah);
    } else {
        setStatus(state, 1, "Couldn't save bookmark — try again");
    }
}

static void bookmarkEditorCancel(void) {
    bmEditing = 0;
}

static void deleteBookmarkAtCursor(AppState *state) {
    if (state->currentScreen != SCREEN_BOOKMARKS) return;
    /* Ctrl+D is half-page — don't delete under it. */
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) return;
    Bookmark rows[256];
    int n = loadBookmarks(rows, 256);
    if (n <= 0) return;
    int idx = state->cursorSurah;
    if (idx < 0) idx = 0;
    if (idx >= n) idx = n - 1;
    if (deleteBookmark(rows[idx].id)) {
        setStatus(state, 0, "Deleted %d:%d", rows[idx].surahNumber, rows[idx].ayahNumber);
        if (state->cursorSurah >= n - 1 && state->cursorSurah > 0) state->cursorSurah--;
    } else {
        setStatus(state, 1, "Couldn't delete bookmark — try again");
    }
}

static void toggleFocusMode(AppState *state) {
    if (state->currentScreen != SCREEN_AYAH_READER) return;
    state->focusMode = !state->focusMode;
    setStatus(state, 0, "Focus mode %s", state->focusMode ? "ON" : "OFF");
}

static void cycleThemeAction(AppState *state) {
    cycleTheme(state);
    Theme *t = getTheme(state->currentTheme);
    applyTitleBarTheme(t);
    setStatus(state, 0, "Theme: %s", t->name);
}

static void goToDashboard(AppState *state) {
    listJumpClear();
    drainCharQueue();
    state->currentScreen = SCREEN_DASHBOARD;
}

/* 'g' (either mode) vs Vim 'G' (Shift+g) share one physical key. */
static int shiftHeld(void) {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

static void goToTopVim(AppState *state) {
    if (!shiftHeld()) return; /* plain g belongs to the dashboard */
    goToTop(state);
}

static void toggleHelp(AppState *state) {
    state->showHelp = !state->showHelp;
}

int settingsCursor = 0;

/* Latitude inline-edit state */
static int   latEditing = 0;
static char  latEditBuf[16] = "";
static int   latEditLen = 0;

int  getSettingsCursor(void) { return settingsCursor; }
int  isEditingLat(void)      { return latEditing; }
const char *getLatEditBuf(void) { return latEditBuf; }

static void latEditStart(AppState *state) {
    latEditing = 1;
    snprintf(latEditBuf, sizeof(latEditBuf), "%.2f", (double)state->latitude);
    latEditLen = (int)strlen(latEditBuf);
}

static void latEditConfirm(AppState *state) {
    latEditing = 0;
    if (latEditLen > 0) {
        float v = (float)atof(latEditBuf);
        if (v < -90.0f) v = -90.0f;
        if (v >  90.0f) v =  90.0f;
        state->latitude = v;
    }
}

static void latEditCancel(void) {
    latEditing = 0;
}

static void latEditKey(int key) {
    if (key == KEY_BACKSPACE) {
        if (latEditLen > 0) latEditBuf[--latEditLen] = '\0';
        return;
    }
    /* Accept digits, minus, plus, dot */
    char ch = 0;
    if (key >= KEY_ZERO && key <= KEY_NINE) ch = '0' + (key - KEY_ZERO);
    else if (key == KEY_PERIOD)  ch = '.';
    else if (key == KEY_COMMA)  ch = '.';
    else if (key == KEY_MINUS)  ch = '-';
    else if (key == KEY_EQUAL)  ch = '+';   /* shift + minus on most layouts */
    if (ch && latEditLen < (int)sizeof(latEditBuf) - 1)
        latEditBuf[latEditLen++] = ch;
}

static void settingsMoveUp(AppState *state) {
    (void)state;
    if (settingsCursor > 0) settingsCursor--;
}

static void settingsMoveDown(AppState *state) {
    (void)state;
    if (settingsCursor < SETTINGS_ROW_COUNT - 1) settingsCursor++;
}

static void settingsToggle(AppState *state) {
    switch (settingsCursor) {
        case 0: state->vimMotions = !state->vimMotions; break;
        case 1: /* fontScale — cycle 0.8 / 1.0 / 1.2 / 1.5 */
            if (state->fontScale < 0.9f) state->fontScale = 1.0f;
            else if (state->fontScale < 1.1f) state->fontScale = 1.2f;
            else if (state->fontScale < 1.4f) state->fontScale = 1.5f;
            else state->fontScale = 0.8f;
            break;
        case 2: /* idleSeconds — cycle 30 / 60 / 120 / 300 */
            if (state->idleSeconds <= 30) state->idleSeconds = 60;
            else if (state->idleSeconds <= 60) state->idleSeconds = 120;
            else if (state->idleSeconds <= 120) state->idleSeconds = 300;
            else state->idleSeconds = 30;
            break;
        case 3: state->autoResume = !state->autoResume; break;
        case 4: cycleTheme(state); applyTitleBarTheme(getTheme(state->currentTheme)); break;
        case 5: state->calcMethod = (state->calcMethod + 1) % 3; break;
        case 6: latEditStart(state); break;
        case 7: /* test buttons play through the real audio path. */
            playReminder();
            setStatus(state, 0, "Playing reminder…");
            break;
        case 8:
            firePrayerAlarm(state);
            setStatus(state, 0, "Prayer alarm test…");
            break;
    }
}

static void openSettings(AppState *state) {
    settingsCursor = 0;
    pushScreen(state, SCREEN_SETTINGS);
}

/* digits-only buffer — number jump, live as you type. */
static void listJumpReposition(AppState *state) {
    if (!listJumpBuf[0] || !state->surahs) return;
    int idx = surahIndexByNumber(state, atoi(listJumpBuf));
    if (idx >= 0) state->cursorSurah = idx;
}

/* half-page reuses the steppers — clamping comes free. */
static void halfPageDown(AppState *state) {
    for (int i = 0; i < 10; i++) moveCursorDown(state);
}

static void halfPageUp(AppState *state) {
    for (int i = 0; i < 10; i++) moveCursorUp(state);
}

/* surah match count for nav clamping (matches recomputed on open). */
static int paletteCount(AppState *state) {
    static int tmp[128];
    return paletteFilter(state, state->paletteQuery, tmp, 128);
}

/* single open path for Finder Enter + row click, both tabs. */
void openFinderMatch(AppState *state) {
    if (!state->showGoToPalette) return;
    if (state->paletteMode == 0) {
        static int m[128];
        int n = paletteFilter(state, state->paletteQuery, m, 128);
        int sel = state->paletteSelection;
        if (sel < 0) sel = 0;
        if (sel >= n) sel = n - 1;
        if (n <= 0 || !state->surahs) return;
        int idx = m[sel];
        if (idx < 0 || idx >= state->surahCount) return;
        state->currentSurah = state->surahs[idx].number;
        state->cursorSurah = idx;
        state->showGoToPalette = 0;
        listJumpClear();
        pushScreen(state, SCREEN_SURAH_OVERVIEW);
    } else {
        int n = state->searchResultCount;
        int sel = state->paletteSelection;
        if (sel < 0) sel = 0;
        if (sel >= n) sel = n - 1;
        if (n <= 0) return;
        state->currentSurah = state->searchResults[sel].surahNumber;
        state->currentAyah = state->searchResults[sel].ayahNumber;
        state->cursorSurah = surahIndexByNumber(state, state->currentSurah);
        state->showGoToPalette = 0;
        listJumpClear();
        pushScreen(state, SCREEN_AYAH_READER);
    }
}

/* only jobless letters detour to the palette — bound keys keep
   their jobs (b/m/d/f/t/s/o/g everywhere, j/k/h/l in Vim mode), so typing
   a name never hijacks navigation. */
static int isPaletteDetourKey(AppState *state, int ch) {
    if (ch >= 'A' && ch <= 'Z') ch += 32;
    if (ch < 'a' || ch > 'z') return 0;
    switch (ch) {
        case 'b': case 'm': case 'd': case 'f':
        case 't': case 's': case 'o': case 'g':
            return 0;
        case 'j': case 'k': case 'h': case 'l':
            return state->vimMotions ? 0 : 1;
        default: return 1;
    }
}

/* ============================================================
 * KEY BINDING TABLE — two sets: vim motions and arrow keys
 * ============================================================ */

typedef void (*ActionFn)(AppState *state);

typedef struct {
    int     key;
    int     screen;    /* -1 = any screen */
    ActionFn action;
} KeyBinding;

/* Shared bindings — active in both vim and arrow modes */
static KeyBinding sharedBindings[] = {
    { KEY_ENTER,   -1,                  openSelected     },
    { KEY_ESCAPE,  -1,                  goBack           },
    { KEY_SLASH,   -1,                  openSearch       },
    { KEY_B,       SCREEN_AYAH_READER,  addBookmark      },
    { KEY_M,       -1,                  openBookmarks    },
    { KEY_D,       SCREEN_BOOKMARKS,  deleteBookmarkAtCursor },
    { KEY_F,       SCREEN_AYAH_READER,  toggleFocusMode  },
    { KEY_T,       -1,                  cycleThemeAction },
    { KEY_S,       -1,                  openSettings     },
    { KEY_F1,      -1,                  toggleHelp       },
    { KEY_O,       -1,                  openFinder       },
};
#define SHARED_COUNT (int)(sizeof(sharedBindings) / sizeof(sharedBindings[0]))

/* Vim-style bindings (j/k/h/l/G/g) */
static KeyBinding vimBindings[] = {
    { KEY_J,       -1,                  moveCursorDown   },
    { KEY_K,       -1,                  moveCursorUp     },
    { KEY_L,       -1,                  moveCursorRight  },
    { KEY_H,       -1,                  moveCursorLeft   },
    { KEY_G,       -1,                  goToTopVim       },
    { KEY_END,     -1,                  goToBottom       },
};
#define VIM_COUNT (int)(sizeof(vimBindings) / sizeof(vimBindings[0]))

/* Arrow-key bindings (arrows, PgUp/PgDn) */
static KeyBinding arrowBindings[] = {
    { KEY_DOWN,    -1,                  moveCursorDown   },
    { KEY_UP,      -1,                  moveCursorUp     },
    { KEY_RIGHT,   -1,                  moveCursorRight  },
    { KEY_LEFT,    -1,                  moveCursorLeft   },
    { KEY_PAGE_UP, -1,                  goToTop          },
    { KEY_PAGE_DOWN,-1,                  goToBottom       },
};
#define ARROW_COUNT (int)(sizeof(arrowBindings) / sizeof(arrowBindings[0]))

/* ============================================================
 * PUBLIC API
 * ============================================================ */

void handleInput(AppState *state) {
    /* main.c owns lastInputTime; touching it here killed idle detect. */

    /* Screensaver: any key or mouse movement exits to the origin screen */
    if (state->currentScreen == SCREEN_SCREENSAVER) {
        if (isAnyKeyPressed() || GetMouseDelta().x != 0 || GetMouseDelta().y != 0) {
            goBack(state);
            stopAzan(); /* alarm acknowledged — silence it */
            state->lastInputTime = GetTime();
        }
        return;
    }

    /* Help overlay is modal — Esc closes it, everything else is swallowed
     * (F1 opens it from the shared table while closed). */
    if (state->showHelp) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            toggleHelp(state);
            drainCharQueue(); /* drop keys mashed while help was up */
        }
        return;
    }

    /* Bookmark tag editor owns all keys while open. */
    if (bmEditing) {
        int ch = GetCharPressed();
        while (ch > 0) {
            searchAppendChar(bmTag, sizeof(bmTag), ch);
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) searchBackspace(bmTag);
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
            bookmarkEditorConfirm(state);
        else if (IsKeyPressed(KEY_ESCAPE))
            bookmarkEditorCancel();
        return;
    }

    /* Settings screen has its own navigation — bypass normal bindings */
    if (state->currentScreen == SCREEN_SETTINGS) {
        /* Latitude edit mode — capture all keys while editing */
        if (latEditing) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
                latEditConfirm(state);
            else if (IsKeyPressed(KEY_ESCAPE))
                latEditCancel();
            else {
                /* Feed every pressed key into the editor */
                for (int k = KEY_ZERO; k <= KEY_NINE; k++)
                    if (IsKeyPressed(k)) latEditKey(k);
                if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_COMMA)) latEditKey(KEY_PERIOD);
                if (IsKeyPressed(KEY_MINUS))  latEditKey(KEY_MINUS);
                if (IsKeyPressed(KEY_BACKSPACE)) latEditKey(KEY_BACKSPACE);
            }
            return;
        }
        /* Normal settings navigation */
        if (navRepeat(KEY_K) || navRepeat(KEY_UP))   settingsMoveUp(state);
        if (navRepeat(KEY_J) || navRepeat(KEY_DOWN)) settingsMoveDown(state);
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) settingsToggle(state);
        if (IsKeyPressed(KEY_ESCAPE)) goBack(state);
        return;
    }

    /* Hadith page owns Enter (modal) and Tab (filter); j/k fall through.
       openSelected's HADITH→goBack stays as the fallback via shared table. */
    if (state->currentScreen == SCREEN_HADITH) {
        if (state->showHadithModal) {
            if (IsKeyPressed(KEY_ESCAPE)) state->showHadithModal = 0;
            return;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (state->totalHadiths > 0) state->showHadithModal = 1;
            return;
        }
        if (IsKeyPressed(KEY_TAB) && !state->showGoToPalette) {
            state->hadithFilter = (state->hadithFilter + 1) % 3;
            state->hadithCursor = 0;
        }
    }

    /* Finder owns all keys while open (arrows + j/k both live, Tab switches tab). */
    if (state->showGoToPalette) {
        int changed = 0;
        int ch = GetCharPressed();
        while (ch > 0) {
            searchAppendChar(state->paletteQuery, sizeof(state->paletteQuery), ch);
            changed = 1;
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            searchBackspace(state->paletteQuery);
            changed = 1;
        }
        if (IsKeyPressed(KEY_TAB)) {
            state->paletteMode = !state->paletteMode;
            changed = 1;
        }
        if (changed) {
            state->paletteSelection = 0;
            if (state->paletteMode == 1) {
                /* ayah engine reads searchQuery — mirror the overlay query. */
                snprintf(state->searchQuery, sizeof(state->searchQuery), "%s", state->paletteQuery);
                runSearch(state, state->searchResults, &state->searchResultCount);
            }
        }
        int palCount = (state->paletteMode == 0)
            ? paletteCount(state)
            : state->searchResultCount;
        if (palCount == 0) state->paletteSelection = 0;
        if (navRepeat(KEY_DOWN) || navRepeat(KEY_J))
            state->paletteSelection = searchMoveSelection(state->paletteSelection, palCount - 1, 1);
        if (navRepeat(KEY_UP) || navRepeat(KEY_K))
            state->paletteSelection = searchMoveSelection(state->paletteSelection, palCount - 1, -1);
        if (IsKeyPressed(KEY_ESCAPE)) {
            state->showGoToPalette = 0;
            return;
        }
        if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) && palCount > 0) {
            int sel = state->paletteSelection;
            if (sel < 0) sel = 0;
            if (sel >= palCount) sel = palCount - 1;
            state->paletteSelection = sel;
            openFinderMatch(state);
        }
        return;
    }

/* Surah-list jump: digits jump to number live, jobless letters detour to
 * the palette prefilled (single name implementation, zero key conflicts). */
    if (state->currentScreen == SCREEN_SURAH_LIST) {
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ch >= '0' && ch <= '9') {
                searchAppendChar(listJumpBuf, sizeof(listJumpBuf), ch);
                listJumpReposition(state);
            } else if (isPaletteDetourKey(state, ch)) {
                openFinder(state);
                searchAppendChar(state->paletteQuery, sizeof(state->paletteQuery), ch);
                state->paletteSelection = 0;
            }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && listJumpBuf[0]) {
            searchBackspace(listJumpBuf);
            listJumpReposition(state);
        }
        if (IsKeyPressed(KEY_ESCAPE) && listJumpBuf[0]) {
            listJumpClear();
            return; /* swallow — don't fall through to goBack */
        }
    }

    /* Half-page jumps, both modes (Ctrl+D guard lives in deleteBookmarkAtCursor). */
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_D)) halfPageDown(state);
        if (IsKeyPressed(KEY_U)) halfPageUp(state);
    }

    /* 'g' goes home in both modes (Shift+G still goes top, via goToTopVim). */
    if (IsKeyPressed(KEY_G) && !shiftHeld()) goToDashboard(state);

    /* Shared bindings (always active) */
    for (int i = 0; i < SHARED_COUNT; i++) {
        if (!IsKeyPressed(sharedBindings[i].key)) continue;
        if (sharedBindings[i].screen != -1 &&
            state->currentScreen != (AppScreen)sharedBindings[i].screen) continue;
        sharedBindings[i].action(state);
    }

    /* Mode-specific bindings (movement repeats while held) */
    KeyBinding *modeBindings = state->vimMotions ? vimBindings : arrowBindings;
    int modeCount = state->vimMotions ? VIM_COUNT : ARROW_COUNT;
    for (int i = 0; i < modeCount; i++) {
        if (!navRepeat(modeBindings[i].key)) continue;
        if (modeBindings[i].screen != -1 &&
            state->currentScreen != (AppScreen)modeBindings[i].screen) continue;
        modeBindings[i].action(state);
    }
}

int isAnyKeyPressed(void) {
    /* state-only — never GetCharPressed() here; it eats the queue
       that typing features (palette, list jump, search) drain in handleInput. */
    for (int k = KEY_SPACE; k <= KEY_MENU; k++)
        if (IsKeyPressed(k)) return 1;
    for (int b = MOUSE_BUTTON_LEFT; b <= MOUSE_BUTTON_MIDDLE; b++)
        if (IsMouseButtonPressed(b)) return 1;
    return 0;
}

