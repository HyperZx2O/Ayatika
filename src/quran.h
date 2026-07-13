#ifndef QURAN_H
#define QURAN_H

#include <raylib.h>

/* ============================================================
 * AYATIKA — SHARED HEADER
 * Owned by: Backend Engineer
 * Used by:  Frontend Engineer, Systems & Features Engineer
 *
 * This file defines every struct and function signature shared
 * across the project. Any change here must be discussed with
 * the whole team before committing — see ARCHITECTURE.md.
 * ============================================================ */

#define MAX_SEARCH_RESULTS 15
#define TOTAL_SURAHS       114

/* ── Core data structures ── */

typedef struct {
    int    number;                 /* 1–114 */
    char   name[64];                /* e.g. "Al-Fatiha" */
    char   arabicName[128];         /* e.g. "الفاتحة" */
    char   meaning[128];             /* e.g. "The Opening" */
    char   revelationType[16];      /* "Meccan" or "Medinan" */
    int    ayahCount;
    char   context[512];            /* short backstory for the overview card */
} Surah;

typedef struct {
    int    surahNumber;
    int    ayahNumber;
    char   arabicText[2048];
    char   translationEn[2048];
    char   translationBn[2048];
    char   audioUrl[256];           /* CDN URL for recitation */
} Ayah;

typedef struct {
    int    id;
    int    surahNumber;
    int    ayahNumber;
    char   tag[128];
    char   note[1024];
    long   timestamp;
} Bookmark;

typedef struct {
    char   name[64];
    char   text[1024];
    char   narrator[128];
    char   collection[32];          /* "Bukhari" or "Muslim" */
} Hadith;

typedef struct {
    float  fajr;
    float  sunrise;
    float  dhuhr;
    float  asr;
    float  maghrib;
    float  isha;
    char   fajrStr[16];
    char   sunriseStr[16];
    char   dhuhrStr[16];
    char   asrStr[16];
    char   maghribStr[16];
    char   ishaStr[16];
    int    prohibitedActive;        /* 1 if currently in a prohibited time */
    char   prohibitedLabel[64];
} PrayerTimes;

typedef struct {
    int    score;
    int    surahNumber;
    int    ayahNumber;
    char   preview[120];            /* first ~120 chars of matching translation */
} SearchResult;

typedef enum {
    SCREEN_DASHBOARD = 0,
    SCREEN_SURAH_LIST,
    SCREEN_AYAH_READER,
    SCREEN_SEARCH,
    SCREEN_BOOKMARKS,
    SCREEN_SCREENSAVER,
    SCREEN_SURAH_OVERVIEW,
    SCREEN_SETTINGS,
    SCREEN_READING_HUB,
    SCREEN_HADITH
} AppScreen;

/* ── Application state — accumulates fields from all 3 members.
 *    Discuss with the team before adding/removing a field. ── */

typedef struct {
    /* Navigation */
    int            currentSurah;
    int            currentAyah;
    int            cursorSurah;       /* highlighted item in sidebar */
    AppScreen      currentScreen;
    AppScreen      previousScreen;

    /* Data (Backend) */
    Surah         *surahs;             /* array of surahCount entries */
    int            surahCount;         /* number of surahs actually loaded */
    Ayah          *ayahs;               /* flat array of all ayahs */
    int            totalAyahs;
    Hadith        *hadiths;
    int            totalHadiths;

    /* Prayer (Backend) */
    PrayerTimes    prayer;
    double         lastPrayerUpdate;

    /* UI (Frontend) */
    int            currentTheme;
    int            dashboardCursor;     /* 0–5, panel focus on dashboard */
    int            focusMode;          /* 1 = dimmed background active */
    int            showHelp;
    char           statusMsg[256];

    /* Audio (Systems) */
    int            isPlayingRecitation;
    int            isNatureSoundOn;

    /* Idle / screensaver (Systems) */
    double         lastInputTime;
    int            catVisible;

    /* Search (Systems) */
    char           searchQuery[256];
    SearchResult   searchResults[MAX_SEARCH_RESULTS];
    int            searchResultCount;

    /* Config (Backend) */
    float          latitude;
    float          longitude;
    int            calcMethod;          /* 0 = Karachi, 1 = MWL, 2 = ISNA */
    char           language[8];         /* "en" or "bn" */

    /* Navigation extras */
    int            hubCursor;           /* 0 = Surah tile, 1 = Hadith tile in reading hub */
    int            hadithCursor;        /* selected hadith in hadith page */
    AppScreen      settingsOrigin;      /* screen active before settings was opened */

    /* Settings */
    int            vimMotions;          /* 1 = vim j/k/h/l bindings, 0 = arrows */
    float          fontScale;           /* UI font multiplier, default 1.0 */
    int            idleSeconds;         /* screensaver delay in seconds */
    int            autoResume;          /* auto-resume last reading position */
} AppState;

/* ============================================================
 * PUBLIC API
 * ============================================================ */

/* quran.c */
int    getDailyAyahIndex(int totalAyahs);

#endif /* QURAN_H */
