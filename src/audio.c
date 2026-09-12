/* ============================================================
 * audio.c — Audio system (Azan, recitation, nature, SFX)
 * Owned by: Systems & Features Engineer
 *
 * Responsibilities:
 *   - Load and play Azan, click, and Surah-switch sounds
 *   - Loop ambient nature sound at low volume
 *   - Stream recitation audio from CDN URLs
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include <time.h>
#include "audio.h"
#include "screensaver.h"

static Sound clickSound;
static Sound surahSwitchSound;
static Sound azanSound;
static Sound reminderSound;
static Music natureMusic;
static Music recitationStream;

static int clickLoaded       = 0;
static int surahSwitchLoaded = 0;
static int azanLoaded        = 0;
static int reminderLoaded    = 0;
static int natureLoaded      = 0;
static int recitationActive  = 0;

void initAudio(void) {
    InitAudioDevice();

    if (FileExists("assets/click.wav")) {
        clickSound  = LoadSound("assets/click.wav");
        clickLoaded = 1;
    }
    if (FileExists("assets/surah_switch.wav")) {
        surahSwitchSound  = LoadSound("assets/surah_switch.wav");
        surahSwitchLoaded = 1;
    }
    if (FileExists("assets/azan.mp3")) {
        azanSound  = LoadSound("assets/azan.mp3");
        azanLoaded = 1;
    }
    if (FileExists("assets/reminder.mp3")) {
        reminderSound  = LoadSound("assets/reminder.mp3");
        reminderLoaded = 1;
    }
    if (FileExists("assets/nature.ogg")) {
        natureMusic  = LoadMusicStream("assets/nature.ogg");
        natureLoaded = 1;
        SetMusicVolume(natureMusic, 0.25f);
        natureMusic.looping = 1;
    }
}

void updateAudio(AppState *state) {
    checkPrayerAlerts(state);
    if (natureLoaded && state->isNatureSoundOn)
        UpdateMusicStream(natureMusic);
    if (recitationActive)
        UpdateMusicStream(recitationStream);
}

void closeAudio(void) {
    if (clickLoaded)       UnloadSound(clickSound);
    if (surahSwitchLoaded) UnloadSound(surahSwitchSound);
    if (azanLoaded)        UnloadSound(azanSound);
    if (reminderLoaded)    UnloadSound(reminderSound);
    if (natureLoaded)      UnloadMusicStream(natureMusic);
    if (recitationActive)  UnloadMusicStream(recitationStream);
    CloseAudioDevice();
}

void playAzan(void) {
    if (azanLoaded && !IsSoundPlaying(azanSound))
        PlaySound(azanSound);
}

void stopAzan(void) {
    if (azanLoaded) StopSound(azanSound);
}

int isAzanPlaying(void) {
    return azanLoaded && IsSoundPlaying(azanSound);
}

void playReminder(void) {
    if (reminderLoaded && !IsSoundPlaying(reminderSound))
        PlaySound(reminderSound);
}

void stopReminder(void) {
    if (reminderLoaded) StopSound(reminderSound);
}

int isReminderPlaying(void) {
    return reminderLoaded && IsSoundPlaying(reminderSound);
}

/* reminder at T-5min, azan at T-0, once per (yday, prayer).
   Called every frame from updateAudio; static keys re-arm each prayer/day. */
void checkPrayerAlerts(AppState *state) {
    static int firedReminderKey = -1;
    static int firedAzanKey = -1;
    if (!state) return;
    if (!azanLoaded && !reminderLoaded) return;
    PrayerTimes *pt = &state->prayer;
    if (pt->fajr == 0 && pt->dhuhr == 0 && pt->asr == 0 &&
        pt->maghrib == 0 && pt->isha == 0) return; /* offline */
    float target = getNextPrayerTime(pt);
    if (target <= 0) return;
    float diff = target - prayerNowHours();
    if (diff < 0) diff += 24.0f;
    float minsLeft = diff * 60.0f;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int key = (tm ? tm->tm_yday : 0) * 10 + nextPrayerIndex(pt);
    if (minsLeft <= 0.75f) {
        if (firedAzanKey != key) {
            stopReminder();
            firePrayerAlarm(state);
            firedAzanKey = key;
        }
    } else if (minsLeft <= 5.0f) {
        if (firedReminderKey != key && firedAzanKey != key)
            playReminder(), firedReminderKey = key;
    }
}

void playRecitation(const char *filePath) {
    if (!filePath || filePath[0] == '\0') return;
    if (recitationActive) {
        StopMusicStream(recitationStream);
        UnloadMusicStream(recitationStream);
        recitationActive = 0;
    }
    if (!FileExists(filePath)) return;
    recitationStream = LoadMusicStream(filePath);
    if (recitationStream.frameCount > 0) {
        recitationStream.looping = 0;
        PlayMusicStream(recitationStream);
        recitationActive = 1;
    }
}

void stopRecitation(void) {
    if (recitationActive) {
        StopMusicStream(recitationStream);
        UnloadMusicStream(recitationStream);
        recitationActive = 0;
    }
}

int isRecitationPlaying(void) {
    return recitationActive;
}

void startNatureSound(void) {
    if (natureLoaded && !IsMusicStreamPlaying(natureMusic))
        PlayMusicStream(natureMusic);
}

void stopNatureSound(void) {
    if (natureLoaded) StopMusicStream(natureMusic);
}

void toggleNatureSound(AppState *state) {
    state->isNatureSoundOn = !state->isNatureSoundOn;
    if (state->isNatureSoundOn) startNatureSound();
    else                        stopNatureSound();
}

void playClickSfx(void) {
    if (clickLoaded) PlaySound(clickSound);
}

void playSurahSwitchSfx(void) {
    if (surahSwitchLoaded) PlaySound(surahSwitchSound);
}

