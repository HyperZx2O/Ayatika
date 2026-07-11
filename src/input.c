#include <raylib.h>
#include <stdio.h>
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

/* ============================================================
 * ACTION FUNCTIONS
 * ============================================================ */

static void moveCursorDown(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            if (state->dashboardCursor < 5) state->dashboardCursor++;
            break;
        case SCREEN_SURAH_LIST:
            state->cursorSurah++;
            break;
        case SCREEN_AYAH_READER: {
            int max = mockAyahCount(state, state->currentSurah);
            if (max > 0 && state->currentAyah < max) state->currentAyah++;
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
        case SCREEN_SURAH_LIST:
            if (state->cursorSurah > 0) state->cursorSurah--;
            break;
        case SCREEN_AYAH_READER:
            if (state->currentAyah > 1) state->currentAyah--;
            break;
        default: break;
    }
}

static void moveCursorRight(AppState *state) {
    if (state->currentScreen == SCREEN_DASHBOARD)
        if (state->dashboardCursor < 5) state->dashboardCursor++;
}

static void moveCursorLeft(AppState *state) {
    if (state->currentScreen == SCREEN_DASHBOARD)
        if (state->dashboardCursor > 0) state->dashboardCursor--;
}

static void goToTop(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD: state->dashboardCursor = 0; break;
        case SCREEN_SURAH_LIST: state->cursorSurah = 0; break;
        case SCREEN_AYAH_READER: state->currentAyah = 1; break;
        default: break;
    }
}

static void goToBottom(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD: state->dashboardCursor = 5; break;
        case SCREEN_SURAH_LIST: state->cursorSurah = TOTAL_SURAHS - 1; break;
        case SCREEN_AYAH_READER: {
            int max = mockAyahCount(state, state->currentSurah);
            if (max > 0) state->currentAyah = max;
            break;
        }
        default: break;
    }
}

static void openSelected(AppState *state) {
    switch (state->currentScreen) {
        case SCREEN_DASHBOARD:
            if (state->dashboardCursor == 3 || state->dashboardCursor == 5) {
                pushScreen(state, SCREEN_AYAH_READER);
                state->currentAyah = 1;
            }
            break;
        case SCREEN_SURAH_LIST:
            state->currentSurah = state->cursorSurah + 1;
            pushScreen(state, SCREEN_SURAH_OVERVIEW);
            break;
        case SCREEN_SURAH_OVERVIEW:
            pushScreen(state, SCREEN_AYAH_READER);
            state->currentAyah = 1;
            break;
        case SCREEN_BOOKMARKS:
            if (mockBookmarkCount > 0) {
                int idx = (state->cursorSurah < mockBookmarkCount)
                          ? state->cursorSurah : 0;
                state->currentSurah = mockBookmarks[idx].surahNumber;
                state->currentAyah = mockBookmarks[idx].ayahNumber;
                pushScreen(state, SCREEN_AYAH_READER);
            }
            break;
        default: break;
    }
}

static void goBack(AppState *state) {
    if (state->currentScreen == SCREEN_DASHBOARD) return;
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
    snprintf(state->statusMsg, sizeof(state->statusMsg), "Theme: %s", t->name);
}

static void goToDashboard(AppState *state) {
    state->currentScreen = SCREEN_DASHBOARD;
}

static void toggleHelp(AppState *state) {
    state->showHelp = !state->showHelp;
}

/* ============================================================
 * KEY BINDING TABLE
 * ============================================================ */

typedef void (*ActionFn)(AppState *state);

typedef struct {
    int     key;
    int     screen;    /* -1 = any screen */
    ActionFn action;
} KeyBinding;

static KeyBinding bindings[] = {
    { KEY_J,       -1,                  moveCursorDown   },
    { KEY_K,       -1,                  moveCursorUp     },
    { KEY_L,       SCREEN_DASHBOARD,    moveCursorRight  },
    { KEY_H,       SCREEN_DASHBOARD,    moveCursorLeft   },
    { KEY_G,       -1,                  goToTop          },
    { KEY_END,     -1,                  goToBottom       },
    { KEY_ENTER,   -1,                  openSelected     },
    { KEY_ESCAPE,  -1,                  goBack           },
    { KEY_SLASH,   -1,                  openSearch       },
    { KEY_B,       SCREEN_AYAH_READER,  addBookmark      },
    { KEY_M,       -1,                  openBookmarks    },
    { KEY_F,       SCREEN_AYAH_READER,  toggleFocusMode  },
    { KEY_T,       -1,                  cycleThemeAction },
    { KEY_HOME,    -1,                  goToDashboard    },
    { KEY_F1,      -1,                  toggleHelp       },
};

#define BINDING_COUNT (int)(sizeof(bindings) / sizeof(bindings[0]))

/* ============================================================
 * PUBLIC API
 * ============================================================ */

void handleInput(AppState *state) {
    state->lastInputTime = GetTime();

    for (int i = 0; i < BINDING_COUNT; i++) {
        if (!IsKeyPressed(bindings[i].key)) continue;
        if (bindings[i].screen != -1 &&
            state->currentScreen != (AppScreen)bindings[i].screen) continue;
        bindings[i].action(state);
    }
}

int isAnyKeyPressed(void) {
    for (int k = KEY_SPACE; k <= KEY_Z; k++)
        if (IsKeyPressed(k)) return 1;
    return 0;
}
