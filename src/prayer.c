/* ============================================================
 * prayer.c — Prayer time (Waqt) calculation
 * Owned by: Backend Engineer
 *
 * Responsibilities:
 *   - Calculate Fajr, Sunrise, Dhuhr, Asr, Maghrib, Isha
 *     using the PrayTimes v2.5 algorithm given lat/lng/date
 *   - Detect prohibited prayer time windows
 *   - Provide countdown helpers for the dashboard
 *
 * See member1.md for the full implementation plan.
 * ============================================================ */

/* quran.h includes <raylib.h>; we never use raylib types here, so
   short-circuit its include guard to let this file compile standalone
   (same pattern as quran.c). */
#define RAYLIB_H
#include "prayer.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── Degree-based math (PrayTimes DMath) ── */

#define DTR(d) ((d) * M_PI / 180.0)
#define RTD(r) ((r) * 180.0 / M_PI)

#define SIN(d)  sin(DTR(d))
#define COS(d)  cos(DTR(d))
#define TAN(d)  tan(DTR(d))
#define ASIN(d) RTD(asin(d))
#define ACOS(d) RTD(acos(d))
#define ARCCOT(x) RTD(atan(1.0 / (x)))
#define ARCTAN2(y, x) RTD(atan2((y), (x)))

static double fixAngle(double a) {
    a = a - 360.0 * floor(a / 360.0);
    return a < 0 ? a + 360.0 : a;
}

static double fixHour(double a) {
    a = a - 24.0 * floor(a / 24.0);
    return a < 0 ? a + 24.0 : a;
}

/* ── Solar position (PrayTimes v2.5) ── */

static double julian(int year, int month, int day) {
    if (month <= 2) { year -= 1; month += 12; }
    double A = floor(year / 100.0);
    double B = 2.0 - A + floor(A / 4.0);
    return floor(365.25 * (year + 4716.0)) +
           floor(30.6001 * (month + 1.0)) + day + B - 1524.5;
}

/* Ref: http://aa.usno.navy.mil/faq/docs/SunApprox.php */
static void sunPosition(double jd, double *decl, double *eqt) {
    double D = jd - 2451545.0;
    double g = fixAngle(357.529 + 0.98560028 * D);
    double q = fixAngle(280.459 + 0.98564736 * D);
    double L = fixAngle(q + 1.915 * SIN(g) + 0.020 * SIN(2.0 * g));
    double e = 23.439 - 0.00000036 * D;

    double RA = ARCTAN2(COS(e) * SIN(L), COS(L)) / 15.0;
    if (eqt)  *eqt  = q / 15.0 - fixHour(RA);
    if (decl) *decl = ASIN(SIN(e) * SIN(L));
}

static double midDay(double jd) {
    double eqt;
    sunPosition(jd, NULL, &eqt);
    return fixHour(12.0 - eqt);
}

static double sunAngleTime(double angle, double jd, double lat, int ccw) {
    double decl, eqt;
    sunPosition(jd, &decl, &eqt);
    double noon = fixHour(12.0 - eqt);
    double t = (1.0 / 15.0) *
               ACOS((-SIN(angle) - SIN(decl) * SIN(lat)) /
                    (COS(decl) * COS(lat)));
    return noon + (ccw ? -t : t);
}

static double asrTime(double jd, double lat) {
    double decl;
    sunPosition(jd, &decl, NULL);
    /* ponytail: Hanafi factor 2, Bangladesh's official standard (Shafi would be 1) */
    double angle = -ARCCOT(2.0 + TAN(fabs(lat - decl)));
    return sunAngleTime(angle, jd, lat, 0);
}

/* ── Internal helpers ── */

static double timeDiff(double t1, double t2) {
    return fixHour(t2 - t1);
}

static float roundMinute(double h) {
    h = fixHour(h + 0.5 / 60.0);
    int hour = (int)h;
    int min  = (int)((h - hour) * 60.0);
    return hour + min / 60.0f;
}

static float currentHour(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    return t->tm_hour + t->tm_min / 60.0f + t->tm_sec / 3600.0f;
}

static void floatToTimeStr(float h, char *out) {
    int hour = (int)h;
    int min  = (int)((h - hour) * 60);
    char *ampm = hour >= 12 ? "PM" : "AM";
    if (hour > 12) hour -= 12;
    if (hour == 0) hour = 12;
    out[0] = (char)('0' + hour / 10);
    out[1] = (char)('0' + hour % 10);
    out[2] = ':';
    out[3] = (char)('0' + min / 10);
    out[4] = (char)('0' + min % 10);
    out[5] = ' ';
    out[6] = ampm[0];
    out[7] = ampm[1];
    out[8] = '\0';
}

/* ── Public API ── */

void updatePrayerTimes(AppState *state) {
    if (state->lastPrayerUpdate != 0 &&
        (time(NULL) - (time_t)state->lastPrayerUpdate) < 3600) return;
    state->lastPrayerUpdate = (double)time(NULL);

    time_t now = time(NULL);
    struct tm ltm;
    struct tm gtm;
    { struct tm *p = localtime(&now); ltm = *p; }
    { struct tm *p = gmtime(&now);    gtm = *p; }
    double tzOffset = (double)(now - mktime(&gtm)) / 3600.0;

    double fajrAngle, ishaAngle;
    switch (state->calcMethod) {
        case 1:  fajrAngle = 18; ishaAngle = 17; break;   /* MWL   */
        case 2:  fajrAngle = 15; ishaAngle = 15; break;   /* ISNA  */
        default: fajrAngle = 18; ishaAngle = 18; break;   /* Karachi */
    }

    double lat = state->latitude, lng = state->longitude;
    double jDate = julian(ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday)
                   - lng / 360.0;

    double fajr = 5, sunrise = 6, dhuhr = 12, asr = 13, maghrib = 18, isha = 18;
    for (int i = 0; i < 1; i++) {
        fajr    = sunAngleTime(fajrAngle, jDate + fajr / 24.0,    lat, 1);
        sunrise = sunAngleTime(0.833,     jDate + sunrise / 24.0, lat, 1);
        dhuhr   = midDay(jDate + dhuhr / 24.0);
        asr     = asrTime(jDate + asr / 24.0, lat);
        maghrib = sunAngleTime(0,         jDate + maghrib / 24.0, lat, 0);
        isha    = sunAngleTime(ishaAngle, jDate + isha / 24.0,    lat, 0);
    }

    double adj = tzOffset - lng / 15.0;
    fajr += adj; sunrise += adj; dhuhr += adj; asr += adj;
    maghrib += adj; isha += adj;

    /* NightMiddle adjustment for high latitudes (prevents NaN). */
    double night = timeDiff(maghrib, sunrise); /* ponytail: NightMiddle only */
    if (isnan(fajr) || timeDiff(fajr, sunrise) > night * 0.5)
        fajr = sunrise - night * 0.5;
    if (isnan(isha) || timeDiff(maghrib, isha) > night * 0.5)
        isha = maghrib + night * 0.5;

    PrayerTimes pt = {0};
    pt.fajr    = roundMinute(fajr);
    pt.sunrise = roundMinute(sunrise);
    pt.dhuhr   = roundMinute(dhuhr);
    pt.asr     = roundMinute(asr);
    pt.maghrib = roundMinute(maghrib);
    pt.isha    = roundMinute(isha);

    floatToTimeStr(pt.fajr,    pt.fajrStr);
    floatToTimeStr(pt.sunrise, pt.sunriseStr);
    floatToTimeStr(pt.dhuhr,   pt.dhuhrStr);
    floatToTimeStr(pt.asr,     pt.asrStr);
    floatToTimeStr(pt.maghrib, pt.maghribStr);
    floatToTimeStr(pt.isha,    pt.ishaStr);

    pt.prohibitedActive = isProhibitedTime(&pt);
    state->prayer = pt;
}

int isProhibitedTime(PrayerTimes *pt) {
    if (!pt) return 0;
    float cur = currentHour();

    if (fabsf(cur - pt->sunrise) < 0.25f) {
        strncpy(pt->prohibitedLabel, "Sunrise — prayer prohibited", 63);
        return 1;
    }
    if (fabsf(cur - pt->dhuhr) < 0.05f) {
        strncpy(pt->prohibitedLabel, "Solar noon — prayer prohibited", 63);
        return 1;
    }
    if (cur >= pt->maghrib - 1.0f / 6.0f && cur < pt->maghrib) {
        strncpy(pt->prohibitedLabel, "Pre-sunset — prayer prohibited", 63);
        return 1;
    }
    pt->prohibitedLabel[0] = '\0';
    return 0;
}

char *getNextPrayerName(PrayerTimes *pt) {
    if (!pt) return "Fajr";
    float cur = currentHour();
    float times[5] = { pt->fajr, pt->dhuhr, pt->asr, pt->maghrib, pt->isha };
    char *names[5] = { "Fajr", "Dhuhr", "Asr", "Maghrib", "Isha" };
    for (int i = 0; i < 5; i++)
        if (times[i] > cur) return names[i];
    return "Fajr";
}

float getNextPrayerTime(PrayerTimes *pt) {
    if (!pt) return 0.0f;
    float cur = currentHour();
    float times[5] = { pt->fajr, pt->dhuhr, pt->asr, pt->maghrib, pt->isha };
    for (int i = 0; i < 5; i++)
        if (times[i] > cur) return times[i];
    return pt->fajr + 24.0f;
}

char *formatCountdown(float targetTime) {
    static char buf[32];
    float diff = targetTime - currentHour();
    if (diff < 0) diff += 24.0f;
    int h = (int)diff;
    int m = (int)((diff - h) * 60.0f + 0.5f);
    if (m == 60) { m = 0; h++; }
    snprintf(buf, sizeof(buf), "%dh %02dm", h, m);
    return buf;
}
