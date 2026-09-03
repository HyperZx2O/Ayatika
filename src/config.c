/* ============================================================
 * config.c — Config file persistence
 * Owned by: Backend Engineer
 *
 * Responsibilities:
 *   - Read/write data/config.ini for user preferences
 *   - Fall back to Dhaka defaults when no config file exists
 *
 * See member1.md for the full implementation plan.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "config.h"

#define CONFIG_PATH "data/config.ini"

static void ensureDataDir(void) {
    // ponytail: swallow EEXIST only
#ifdef _WIN32
    if (_mkdir("data") != 0 && errno != EEXIST) {}
#else
    if (mkdir("data", 0755) != 0 && errno != EEXIST) {}
#endif
}

void loadConfig(AppState *state) {
    /* Dhaka defaults */
    state->latitude   = 23.8103f;
    state->longitude  = 90.4125f;
    state->calcMethod = 0;
    strncpy(state->language, "en", sizeof(state->language) - 1);
    state->language[sizeof(state->language) - 1] = '\0';
    state->currentSurah = 1;
    state->currentAyah  = 1;

    // ponytail: no mkdir on load — read-only, keep side-effect free
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) return;

    // ponytail: fgets per-line so garbage line doesn't swallow next valid line
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[64], val[128];
        if (sscanf(line, "%63[^=]=%127s", key, val) != 2) continue;
        if (strcmp(key, "latitude")   == 0) state->latitude   = (float)atof(val);
        else if (strcmp(key, "longitude")  == 0) state->longitude  = (float)atof(val);
        else if (strcmp(key, "calcMethod") == 0) state->calcMethod = atoi(val);
        else if (strcmp(key, "language")   == 0) {
            strncpy(state->language, val, sizeof(state->language) - 1);
            state->language[sizeof(state->language) - 1] = '\0';
        }
        else if (strcmp(key, "lastSurah")  == 0) state->currentSurah = atoi(val);
        else if (strcmp(key, "lastAyah")   == 0) state->currentAyah  = atoi(val);
    }
    fclose(f);
}

void saveConfig(AppState *state) {
    ensureDataDir();
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) return;

    fprintf(f, "latitude=%.6f\n",   state->latitude);
    fprintf(f, "longitude=%.6f\n",  state->longitude);
    fprintf(f, "calcMethod=%d\n",   state->calcMethod);
    fprintf(f, "language=%s\n",     state->language);
    fprintf(f, "lastSurah=%d\n",    state->currentSurah);
    fprintf(f, "lastAyah=%d\n",     state->currentAyah);
    fclose(f);
}
