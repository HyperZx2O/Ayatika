#ifndef FTS_FUZZY_MATCH_H
#define FTS_FUZZY_MATCH_H

#include <stdint.h>
#include <ctype.h>
#include <string.h>

int fts_fuzzy_match(char const *pattern, char const *str, int *outScore);
int fts_fuzzy_match_simple(char const *pattern, char const *str);

#ifdef FTS_FUZZY_MATCH_IMPLEMENTATION

static int fuzzy_match_recursive(const char *pattern, const char *str, int *outScore,
    const char *strBegin, uint8_t const *srcMatches, uint8_t *matches, int maxMatches,
    int nextMatch, int *recursionCount, int recursionLimit)
{
    ++(*recursionCount);
    if (*recursionCount >= recursionLimit)
        return 0;

    if (*pattern == '\0' || *str == '\0')
        return 0;

    int recursiveMatch = 0;
    uint8_t bestRecursiveMatches[256];
    int bestRecursiveScore = 0;

    int first_match = 1;
    while (*pattern != '\0' && *str != '\0') {
        if (tolower((unsigned char)*pattern) == tolower((unsigned char)*str)) {
            if (nextMatch >= maxMatches)
                return 0;

            if (first_match && srcMatches) {
                memcpy(matches, srcMatches, nextMatch);
                first_match = 0;
            }

            uint8_t recursiveMatches[256];
            int recursiveScore;
            if (fuzzy_match_recursive(pattern, str + 1, &recursiveScore, strBegin,
                    matches, recursiveMatches, sizeof(recursiveMatches), nextMatch,
                    recursionCount, recursionLimit))
            {
                if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                    memcpy(bestRecursiveMatches, recursiveMatches, 256);
                    bestRecursiveScore = recursiveScore;
                }
                recursiveMatch = 1;
            }

            matches[nextMatch++] = (uint8_t)(str - strBegin);
            ++pattern;
        }
        ++str;
    }

    int matched = (*pattern == '\0') ? 1 : 0;

    if (matched) {
        const int sequential_bonus = 15;
        const int separator_bonus = 30;
        const int camel_bonus = 30;
        const int first_letter_bonus = 15;
        const int leading_letter_penalty = -5;
        const int max_leading_letter_penalty = -15;
        const int unmatched_letter_penalty = -1;

        while (*str != '\0')
            ++str;

        *outScore = 100;

        int penalty = leading_letter_penalty * matches[0];
        if (penalty < max_leading_letter_penalty)
            penalty = max_leading_letter_penalty;
        *outScore += penalty;

        int unmatched = (int)(str - strBegin) - nextMatch;
        *outScore += unmatched_letter_penalty * unmatched;

        for (int i = 0; i < nextMatch; ++i) {
            uint8_t currIdx = matches[i];

            if (i > 0) {
                uint8_t prevIdx = matches[i - 1];
                if (currIdx == (prevIdx + 1))
                    *outScore += sequential_bonus;
            }

            if (currIdx > 0) {
                char neighbor = strBegin[currIdx - 1];
                char curr = strBegin[currIdx];
                if (islower((unsigned char)neighbor) && isupper((unsigned char)curr))
                    *outScore += camel_bonus;

                int neighborSeparator = (neighbor == '_' || neighbor == ' ');
                if (neighborSeparator)
                    *outScore += separator_bonus;
            } else {
                *outScore += first_letter_bonus;
            }
        }
    }

    if (recursiveMatch && (!matched || bestRecursiveScore > *outScore)) {
        memcpy(matches, bestRecursiveMatches, maxMatches);
        *outScore = bestRecursiveScore;
        return 1;
    } else if (matched) {
        return 1;
    }

    return 0;
}

int fts_fuzzy_match(char const *pattern, char const *str, int *outScore)
{
    uint8_t matches[256];
    int recursionCount = 0;
    int recursionLimit = 10;
    return fuzzy_match_recursive(pattern, str, outScore, str, 0, matches,
        sizeof(matches), 0, &recursionCount, recursionLimit);
}

int fts_fuzzy_match_simple(char const *pattern, char const *str)
{
    while (*pattern != '\0' && *str != '\0') {
        if (tolower((unsigned char)*pattern) == tolower((unsigned char)*str))
            ++pattern;
        ++str;
    }
    return *pattern == '\0' ? 1 : 0;
}

#endif

#endif
