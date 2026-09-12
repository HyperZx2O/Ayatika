#ifndef TEST_DATA_H
#define TEST_DATA_H

#include "quran.h"

/* deterministic fixture for harnesses (pins search relevance);
// the app itself loads live API data — this never ships. */
void loadTestData(AppState *state);

#endif

