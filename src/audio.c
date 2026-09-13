/* ============================================================
 * audio.c — Audio system (Azan + reminder alerts)
 * Owned by: Systems & Features Engineer
 *
 * Responsibilities:
 *   - Load and play Azan and reminder sounds
 *   - Fire waqt alerts (reminder at T-5min, azan at T-0)
 *
 * See member3.md for the full implementation plan.
 * ============================================================ */

#include <raylib.h>
#include <time.h>
#include "audio.h"
#include "screensaver.h"

static Sound azanSound;
static Sound reminderSound;

static int azanLoaded        = 0;
static int reminderLoaded    = 0;

void initAudio(void) {
    InitAudioDevice();

    if (FileExists("assets/azan.mp3")) {
        azanSound  = LoadSound("assets/azan.mp3");
        azanLoaded = 1;
    }
    if (FileExists("assets/reminder.mp3")) {
        reminderSound  = LoadSound("assets/reminder.mp3");
        reminderLoaded = 1;
    }
}

void updateAudio(AppState *state) {
    checkPrayerAlerts(state);
}

void closeAudio(void) {
    if (azanLoaded)        UnloadSound(azanSound);
    if (reminderLoaded)    UnloadSound(reminderSound);
    CloseAudioDevice();
}

void playAzan(void) {
    if (azanLoaded && !IsSoundPlaying(azanSound))
        PlaySound(azanSound);
}

void stopAzan(void) {
    if (azanLoaded) StopSound(azanSound);
}

void playReminder(void) {
    if (reminderLoaded && !IsSoundPlaying(reminderSound))
        PlaySound(reminderSound);
}

void stopReminder(void) {
    if (reminderLoaded) StopSound(reminderSound);
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

