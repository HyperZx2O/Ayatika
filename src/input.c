#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "theme.h"
#include "ui.h"

/* ── Helper: save current screen before transitioning ── */
static void pushScreen(AppState *state, AppScreen target) {
    state->previousScreen = state->currentScreen;
    state->currentScreen = target;
}

/* ── Helper: count ayahs loaded for a surah in mock data ── */
static int mockAyahCount(AppState *state, int surahNum) {
    int count = 0;
    for (int i = 0; i < state->totalAyahs; i++)
        if (state->ayahs[i].surahNumber == surahNum)
            count++;
    return count;
}

/* ── Helper: find bookmark index for surah/ayah, -1 if none ── */
static int findBookmarkIndex(int surah, int ayah) {
    for (int i = 0; i < mockBookmarkCount; i++)
        if (mockBookmarks[i].surahNumber == surah &&
            mockBookmarks[i].ayahNumber == ayah)
            return i;
    return -1;
}

/* ── Helper: count surahs loaded in mock data ── */
static int mockSurahCount(AppState *state) {
    return state->surahCount;
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
            int max = mockSurahCount(state);
            if (max > 0 && state->cursorSurah < max - 1) state->cursorSurah++;
            break;
        }
        case SCREEN_AYAH_READER: {
            int max = mockAyahCount(state, state->currentSurah);
            if (max > 0 && state->currentAyah < max) state->currentAyah++;
            break;
        }
        case SCREEN_HADITH:
            if (state->hadithCursor < state->totalHadiths - 1) state->hadithCursor++;
            break;
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
            int max = mockSurahCount(state);
            if (max > 0 && state->cursorSurah < max - 1) state->cursorSurah++;
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
        case SCREEN_SURAH_LIST: state->cursorSurah = TOTAL_SURAHS - 1; break;
        case SCREEN_AYAH_READER: {
            int max = mockAyahCount(state, state->currentSurah);
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
            state->currentSurah = state->cursorSurah + 1;
            pushScreen(state, SCREEN_SURAH_OVERVIEW);
            break;
        case SCREEN_SURAH_OVERVIEW:
            /* Don't use pushScreen — preserves previousScreen so
               Esc from the reader goes back to Reading Hub, not in a loop. */
            state->currentScreen = SCREEN_AYAH_READER;
            state->currentAyah = 1;
            state->cursorSurah = state->currentSurah - 1;
            break;
        case SCREEN_AYAH_READER: {
            int target = state->cursorSurah + 1;
            if (target != state->currentSurah) {
                state->currentSurah = target;
                pushScreen(state, SCREEN_SURAH_OVERVIEW);
            }
            break;
        }
        case SCREEN_BOOKMARKS:
            if (mockBookmarkCount > 0) {
                int idx = (state->cursorSurah < mockBookmarkCount)
                          ? state->cursorSurah : 0;
                state->currentSurah = mockBookmarks[idx].surahNumber;
                state->currentAyah = mockBookmarks[idx].ayahNumber;
                state->cursorSurah = state->currentSurah - 1;
                pushScreen(state, SCREEN_AYAH_READER);
            }
            break;
        case SCREEN_HADITH:
            /* Enter on hadith page: jump to the surah referenced or just go back */
            goBack(state);
            break;
        default: break;
    }
}

static void goBack(AppState *state) {
    if (state->currentScreen == SCREEN_DASHBOARD ||
        state->currentScreen == SCREEN_READING_HUB) {
        state->currentScreen = SCREEN_DASHBOARD;
        return;
    }
    state->currentScreen = state->previousScreen;
    if (state->currentScreen == SCREEN_AYAH_READER && state->currentAyah < 1)
        state->currentAyah = 1;
}

static void openSearch(AppState *state) {
    pushScreen(state, SCREEN_SEARCH);
    state->searchQuery[0] = '\0';
    state->searchResultCount = 0;
    snprintf(state->statusMsg, sizeof(state->statusMsg), "/");
}

static void openBookmarks(AppState *state) {
    pushScreen(state, SCREEN_BOOKMARKS);
    state->cursorSurah = 0;
}

static void addBookmark(AppState *state) {
    if (state->currentScreen != SCREEN_AYAH_READER) return;
    if (findBookmarkIndex(state->currentSurah, state->currentAyah) >= 0) {
        snprintf(state->statusMsg, sizeof(state->statusMsg),
                 "Already bookmarked: %d:%d", state->currentSurah, state->currentAyah);
        return;
    }
    if (addMockBookmark(state->currentSurah, state->currentAyah)) {
        showBookmarkPopup();
        snprintf(state->statusMsg, sizeof(state->statusMsg),
                 "Bookmarked %d:%d", state->currentSurah, state->currentAyah);
    }
}

static void toggleFocusMode(AppState *state) {
    if (state->currentScreen != SCREEN_AYAH_READER) return;
    state->focusMode = !state->focusMode;
    snprintf(state->statusMsg, sizeof(state->statusMsg),
             "Focus mode %s", state->focusMode ? "ON" : "OFF");
}

static void cycleThemeAction(AppState *state) {
    cycleTheme(state);
    Theme *t = getTheme(state->currentTheme);
    applyTitleBarTheme(t);
    snprintf(state->statusMsg, sizeof(state->statusMsg), "Theme: %s", t->name);
}

static void goToDashboard(AppState *state) {
    state->currentScreen = SCREEN_DASHBOARD;
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

static void triggerScreensaver(AppState *state);

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
        case 5: strncpy(state->language, strcmp(state->language, "bn") == 0 ? "en" : "bn", 7); break;
        case 6: state->calcMethod = (state->calcMethod + 1) % 3; break;
        case 7: latEditStart(state); break;
        case 8: triggerScreensaver(state); break;
    }
}

static void openSettings(AppState *state) {
    state->settingsOrigin = state->currentScreen;
    settingsCursor = 0;
    pushScreen(state, SCREEN_SETTINGS);
}

static void triggerScreensaver(AppState *state) {
    state->previousScreen = state->settingsOrigin;
    state->currentScreen = SCREEN_SCREENSAVER;
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
    { KEY_F,       SCREEN_AYAH_READER,  toggleFocusMode  },
    { KEY_T,       -1,                  cycleThemeAction },
    { KEY_S,       -1,                  openSettings     },
    { KEY_HOME,    -1,                  goToDashboard    },
    { KEY_F1,      -1,                  toggleHelp       },
};
#define SHARED_COUNT (int)(sizeof(sharedBindings) / sizeof(sharedBindings[0]))

/* Vim-style bindings (j/k/h/l/G/g) */
static KeyBinding vimBindings[] = {
    { KEY_J,       -1,                  moveCursorDown   },
    { KEY_K,       -1,                  moveCursorUp     },
    { KEY_L,       -1,                  moveCursorRight  },
    { KEY_H,       -1,                  moveCursorLeft   },
    { KEY_G,       -1,                  goToTop          },
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
    state->lastInputTime = GetTime();

    /* Screensaver: any key or mouse movement exits to the origin screen */
    if (state->currentScreen == SCREEN_SCREENSAVER) {
        if (isAnyKeyPressed() || GetMouseDelta().x != 0 || GetMouseDelta().y != 0) {
            goBack(state);
            state->lastInputTime = GetTime();
        }
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
        if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_UP))   settingsMoveUp(state);
        if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_DOWN)) settingsMoveDown(state);
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) settingsToggle(state);
        if (IsKeyPressed(KEY_ESCAPE)) goBack(state);
        return;
    }

    /* Shared bindings (always active) */
    for (int i = 0; i < SHARED_COUNT; i++) {
        if (!IsKeyPressed(sharedBindings[i].key)) continue;
        if (sharedBindings[i].screen != -1 &&
            state->currentScreen != (AppScreen)sharedBindings[i].screen) continue;
        sharedBindings[i].action(state);
    }

    /* Mode-specific bindings */
    KeyBinding *modeBindings = state->vimMotions ? vimBindings : arrowBindings;
    int modeCount = state->vimMotions ? VIM_COUNT : ARROW_COUNT;
    for (int i = 0; i < modeCount; i++) {
        if (!IsKeyPressed(modeBindings[i].key)) continue;
        if (modeBindings[i].screen != -1 &&
            state->currentScreen != (AppScreen)modeBindings[i].screen) continue;
        modeBindings[i].action(state);
    }
}

int isAnyKeyPressed(void) {
    for (int k = KEY_SPACE; k <= KEY_Z; k++)
        if (IsKeyPressed(k)) return 1;
    return 0;
}
