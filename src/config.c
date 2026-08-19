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
#include "config.h"

#define CONFIG_PATH "data/config.ini"

void loadConfig(AppState *state) {
    /* Dhaka defaults */
    state->latitude   = 23.8103f;
    state->longitude  = 90.4125f;
    state->calcMethod = 0;
    strncpy(state->language, "en", sizeof(state->language) - 1);
    state->language[sizeof(state->language) - 1] = '\0';
    state->currentSurah = 1;
    state->currentAyah  = 1;

    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) return;

    char key[64], val[128];
    while (fscanf(f, "%63[^=]=%127s\n", key, val) == 2) {
        if (strcmp(key, "latitude")   == 0) state->latitude   = (float)atof(val);
        if (strcmp(key, "longitude")  == 0) state->longitude  = (float)atof(val);
        if (strcmp(key, "calcMethod") == 0) state->calcMethod = atoi(val);
        if (strcmp(key, "language")   == 0) {
            strncpy(state->language, val, sizeof(state->language) - 1);
            state->language[sizeof(state->language) - 1] = '\0';
        }
        if (strcmp(key, "lastSurah")  == 0) state->currentSurah = atoi(val);
        if (strcmp(key, "lastAyah")   == 0) state->currentAyah  = atoi(val);
    }
    fclose(f);
}

void saveConfig(AppState *state) {
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
