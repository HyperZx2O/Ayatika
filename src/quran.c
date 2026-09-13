#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <windows.h>
#include <wininet.h>
#include "quran.h"
#include "cJSON.h"

/* WinINet, not libcurl — system DLL, no extra deps or DLLs to ship. */
static int fetchAndSave(const char *url, const char *filepath) {
    HINTERNET net = InternetOpenA("Ayatika", INTERNET_OPEN_TYPE_DIRECT,
                                  NULL, NULL, 0);
    if (!net) return 0;
    HINTERNET src = InternetOpenUrlA(net, url, NULL, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!src) { InternetCloseHandle(net); return 0; }
    FILE *f = fopen(filepath, "wb");
    if (!f) { InternetCloseHandle(src); InternetCloseHandle(net); return 0; }
    char chunk[8192];
    DWORD got = 0;
    int ok = 1;
    for (;;) {
        if (!InternetReadFile(src, chunk, sizeof(chunk), &got)) { ok = 0; break; }
        if (got == 0) break;
        if (fwrite(chunk, 1, got, f) != got) { ok = 0; break; }
    }
    fclose(f);
    InternetCloseHandle(src);
    InternetCloseHandle(net);
    if (!ok) remove(filepath);
    return ok;
}

/* byte-safe copy — backs off to a UTF-8 boundary so a cut never
   splits a sequence (ar runs to 2283B, bn translations to 3325B). */
static void copyTextSafe(char *dst, const char *src, size_t dstSize) {
    if (dstSize == 0) return;
    size_t n = strlen(src);
    if (n > dstSize - 1) n = dstSize - 1;
    size_t k = 0;
    while (k < n && (src[n - 1 - k] & 0xC0) == 0x80) k++;
    if (k > 0 && k < n) {
        unsigned char lead = (unsigned char)src[n - 1 - k];
        size_t expect = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
        if (k + 1 < expect) n -= (k + 1); /* mid-sequence cut: drop partial char */
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static char *readFile(const char *filepath, long *outLen) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)len, f);
    buf[len] = '\0';
    fclose(f);
    if (outLen) *outLen = len;
    return buf;
}

static int parseQuranJSON(AppState *state, const char *filepath) {
    long len;
    char *buf = readFile(filepath, &len);
    if (!buf) return 0;

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return 0;

    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!data || !cJSON_IsObject(data)) { cJSON_Delete(root); return 0; }

    cJSON *surahs = cJSON_GetObjectItem(data, "surahs");
    if (!surahs || !cJSON_IsArray(surahs)) { cJSON_Delete(root); return 0; }

    int surahCount = cJSON_GetArraySize(surahs);

    int totalAyahs = 0;
    for (int i = 0; i < surahCount; i++) {
        cJSON *surah = cJSON_GetArrayItem(surahs, i);
        cJSON *ayahs = cJSON_GetObjectItem(surah, "ayahs");
        if (ayahs && cJSON_IsArray(ayahs))
            totalAyahs += cJSON_GetArraySize(ayahs);
    }

    if (state->surahs) { free(state->surahs); state->surahs = NULL; }
    if (state->ayahs)  { free(state->ayahs);  state->ayahs  = NULL; }

    state->surahs = calloc((size_t)surahCount, sizeof(Surah));
    state->ayahs  = calloc((size_t)totalAyahs, sizeof(Ayah));

    if (!state->surahs || !state->ayahs) {
        free(state->surahs); state->surahs = NULL;
        free(state->ayahs);  state->ayahs  = NULL;
        cJSON_Delete(root);
        return 0;
    }

    int ayahIdx = 0;
    for (int i = 0; i < surahCount; i++) {
        cJSON *surah = cJSON_GetArrayItem(surahs, i);

        Surah *s = &state->surahs[i];
        {
            cJSON *item = cJSON_GetObjectItem(surah, "number");
            if (cJSON_IsNumber(item)) s->number = item->valueint;
        }
        {
            cJSON *item = cJSON_GetObjectItem(surah, "englishName");
            if (cJSON_IsString(item))
                strncpy(s->name, item->valuestring, sizeof(s->name) - 1);
        }
        {
            cJSON *item = cJSON_GetObjectItem(surah, "name");
            if (cJSON_IsString(item))
                strncpy(s->arabicName, item->valuestring, sizeof(s->arabicName) - 1);
        }
        {
            cJSON *item = cJSON_GetObjectItem(surah, "englishNameTranslation");
            if (cJSON_IsString(item))
                strncpy(s->meaning, item->valuestring, sizeof(s->meaning) - 1);
        }
        {
            cJSON *item = cJSON_GetObjectItem(surah, "revelationType");
            if (cJSON_IsString(item))
                strncpy(s->revelationType, item->valuestring, sizeof(s->revelationType) - 1);
        }

        cJSON *ayahs = cJSON_GetObjectItem(surah, "ayahs");
        if (ayahs && cJSON_IsArray(ayahs)) {
            int ayahCount = cJSON_GetArraySize(ayahs);
            s->ayahCount = ayahCount;

            for (int j = 0; j < ayahCount; j++) {
                cJSON *ayah = cJSON_GetArrayItem(ayahs, j);
                Ayah *a = &state->ayahs[ayahIdx];
                a->surahNumber = s->number;

                cJSON *item = cJSON_GetObjectItem(ayah, "numberInSurah");
                if (cJSON_IsNumber(item)) a->ayahNumber = item->valueint;

                item = cJSON_GetObjectItem(ayah, "text");
                if (cJSON_IsString(item))
                    copyTextSafe(a->arabicText, item->valuestring, sizeof(a->arabicText));

                ayahIdx++;
            }
        }
    }

    state->surahCount = surahCount;
    state->totalAyahs = totalAyahs;

    cJSON_Delete(root);
    return 1;
}

static int mergeTranslation(AppState *state, const char *filepath, const char *lang) {
    long len;
    char *buf = readFile(filepath, &len);
    if (!buf) return 0;

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return 0;

    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!data || !cJSON_IsObject(data)) { cJSON_Delete(root); return 0; }

    cJSON *surahs = cJSON_GetObjectItem(data, "surahs");
    if (!surahs || !cJSON_IsArray(surahs)) { cJSON_Delete(root); return 0; }

    int isBn = (strcmp(lang, "bn") == 0);
    int surahCount = cJSON_GetArraySize(surahs);

    for (int i = 0; i < surahCount; i++) {
        cJSON *surah = cJSON_GetArrayItem(surahs, i);
        cJSON *numItem = cJSON_GetObjectItem(surah, "number");
        int surahNum = cJSON_IsNumber(numItem) ? numItem->valueint : 0;

        cJSON *ayahs = cJSON_GetObjectItem(surah, "ayahs");
        if (!ayahs || !cJSON_IsArray(ayahs)) continue;

        int ayahCount = cJSON_GetArraySize(ayahs);
        for (int j = 0; j < ayahCount; j++) {
            cJSON *ayah = cJSON_GetArrayItem(ayahs, j);
            cJSON *numInSurah = cJSON_GetObjectItem(ayah, "numberInSurah");
            cJSON *text = cJSON_GetObjectItem(ayah, "text");

            if (!cJSON_IsNumber(numInSurah) || !cJSON_IsString(text)) continue;

            int ayahNum = numInSurah->valueint;

            for (int k = 0; k < state->totalAyahs; k++) {
                if (state->ayahs[k].surahNumber == surahNum &&
                    state->ayahs[k].ayahNumber == ayahNum) {
                    if (isBn)
                        copyTextSafe(state->ayahs[k].translationBn, text->valuestring,
                                     sizeof(state->ayahs[k].translationBn));
                    else
                        copyTextSafe(state->ayahs[k].translationEn, text->valuestring,
                                     sizeof(state->ayahs[k].translationEn));
                    break;
                }
            }
        }
    }

    cJSON_Delete(root);
    return 1;
}

static void ensureDataDir(void) {
    // swallow EEXIST only, stdlib mkdir — no wrapper lib
#ifdef _WIN32
    if (_mkdir("data") != 0 && errno != EEXIST) {}
#else
    if (mkdir("data", 0755) != 0 && errno != EEXIST) {}
#endif
}

static void setOfflineMsg(AppState *state) {
    setStatus(state, 1, "Offline mode — connect to fetch full data");
}

int loadQuranData(AppState *state) {
    ensureDataDir();
    setStatus(state, 0, "%s", "");

    if (access("data/quran.json", F_OK) != 0) {
        printf("First run: fetching Quran data...\n");
        if (!fetchAndSave(
                "https://api.alquran.cloud/v1/quran/quran-uthmani",
                "data/quran.json")) {
            setOfflineMsg(state);
            return 0;
        }
    }

    if (!parseQuranJSON(state, "data/quran.json")) {
        // corrupted/null JSON → offline msg, caller can retry after re-fetch
        setOfflineMsg(state);
        return 0;
    }

    if (access("data/translation_en.json", F_OK) != 0) {
        printf("Fetching English translation...\n");
        fetchAndSave(
            "https://api.alquran.cloud/v1/quran/en.sahih",
            "data/translation_en.json");
    }
    mergeTranslation(state, "data/translation_en.json", "en");

    if (access("data/translation_bn.json", F_OK) != 0) {
        printf("Fetching Bengali translation...\n");
        fetchAndSave(
            "https://api.alquran.cloud/v1/quran/bn.bengali",
            "data/translation_bn.json");
    }
    mergeTranslation(state, "data/translation_bn.json", "bn");

    /* overlay static context blurbs onto API surahs; API has no context. */
    for (int i = 0; i < state->surahCount; i++) {
        Surah meta;
        getSurahMeta(state->surahs[i].number, &meta);
        if (!state->surahs[i].context[0])
            snprintf(state->surahs[i].context, sizeof(state->surahs[i].context), "%s", meta.context);
        if (!state->surahs[i].meaning[0])
            snprintf(state->surahs[i].meaning, sizeof(state->surahs[i].meaning), "%s", meta.meaning);
        if (!state->surahs[i].revelationType[0])
            snprintf(state->surahs[i].revelationType, sizeof(state->surahs[i].revelationType), "%s", meta.revelationType);
    }

    return 1;
}

/* bulk Bukhari+Muslim from fawazahmed0/hadith-api (jsDelivr, no key);
   bundled assets/hadiths.json stays as the offline fallback. */
static const char *HADITH_EDITIONS[2] = { "eng-bukhari", "eng-muslim" };
static const char *HADITH_COLLECTIONS[2] = { "Bukhari", "Muslim" };
static const char *HADITH_CACHE[2] = { "data/hadith_bukhari.json", "data/hadith_muslim.json" };

static int fetchHadithEdition(const char *edition, const char *cache) {
    if (access(cache, F_OK) == 0) return 1;
    char urls[4][256];
    snprintf(urls[0], sizeof(urls[0]),
        "https://cdn.jsdelivr.net/gh/fawazahmed0/hadith-api@1/editions/%s.min.json", edition);
    snprintf(urls[1], sizeof(urls[1]),
        "https://cdn.jsdelivr.net/gh/fawazahmed0/hadith-api@1/editions/%s.json", edition);
    snprintf(urls[2], sizeof(urls[2]),
        "https://raw.githubusercontent.com/fawazahmed0/hadith-api/1/editions/%s.min.json", edition);
    snprintf(urls[3], sizeof(urls[3]),
        "https://raw.githubusercontent.com/fawazahmed0/hadith-api/1/editions/%s.json", edition);
    for (int i = 0; i < 4; i++)
        if (fetchAndSave(urls[i], cache)) return 1;
    return 0;
}

/* the API ships placeholder entries with empty text; never import them. */
static int isBlankText(const char *s) {
    if (!s) return 1;
    return strspn(s, " \t\r\n") == strlen(s);
}

/* narrator = "Narrated X" prefix before first ':'; else empty. */
static void splitNarrator(const char *text, char *narr, int narrSz) {
    narr[0] = '\0';
    if (!text || strncmp(text, "Narrated ", 9) != 0) return;
    const char *end = strchr(text, ':');
    if (!end || end - text >= narrSz) return;
    memcpy(narr, text, (size_t)(end - text));
    narr[end - text] = '\0';
}

/* exact-size heap text; corpus runs to ~17KB, no fixed cap fits. */
static char *dupText(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

void freeHadiths(AppState *state) {
    if (!state || !state->hadiths) return;
    for (int i = 0; i < state->totalHadiths; i++) free(state->hadiths[i].text);
    free(state->hadiths);
    state->hadiths = NULL;
    state->totalHadiths = 0;
}

static void freeHadithArray(Hadith *arr, int count) {
    if (!arr) return;
    for (int i = 0; i < count; i++) free(arr[i].text);
    free(arr);
}

static int appendHadithEdition(const char *cache, const char *collection,
                               Hadith **out, int *count, int *cap) {
    long len;
    char *buf = readFile(cache, &len);
    if (!buf) return 0;
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return 0;
    cJSON *arr = cJSON_GetObjectItem(root, "hadiths");
    if (!arr || !cJSON_IsArray(arr)) { cJSON_Delete(root); return 0; }
    int n = cJSON_GetArraySize(arr);
    for (int i = 0; i < n; i++) {
        cJSON *h = cJSON_GetArrayItem(arr, i);
        cJSON *num = cJSON_GetObjectItem(h, "hadithnumber");
        cJSON *text = cJSON_GetObjectItem(h, "text");
        if (!cJSON_IsNumber(num) || !cJSON_IsString(text)) continue;
        if (isBlankText(text->valuestring)) continue;
        if (*count >= *cap) {
            int ncap = *cap ? *cap * 2 : 1024;
            Hadith *tmp = realloc(*out, (size_t)ncap * sizeof(Hadith));
            if (!tmp) { cJSON_Delete(root); return *count > 0; }
            *out = tmp;
            *cap = ncap;
        }
        Hadith *d = &(*out)[*count];
        memset(d, 0, sizeof(*d));
        snprintf(d->name, sizeof(d->name), "%s %d", collection, num->valueint);
        d->text = dupText(text->valuestring);
        if (!d->text) { cJSON_Delete(root); return *count > 0; }
        splitNarrator(text->valuestring, d->narrator, sizeof(d->narrator));
        snprintf(d->collection, sizeof(d->collection), "%s", collection);
        (*count)++;
    }
    cJSON_Delete(root);
    return 1;
}

static int loadBundledHadiths(AppState *state) {
    freeHadiths(state);
    long len;
    char *buf = readFile("assets/hadiths.json", &len);
    if (!buf) return 0;
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return 0;
    cJSON *arr = cJSON_GetObjectItem(root, "hadiths");
    if (!arr || !cJSON_IsArray(arr)) { cJSON_Delete(root); return 0; }
    int n = cJSON_GetArraySize(arr);
    state->hadiths = calloc((size_t)n, sizeof(Hadith));
    if (!state->hadiths) { cJSON_Delete(root); return 0; }
    for (int i = 0; i < n; i++) {
        cJSON *h = cJSON_GetArrayItem(arr, i);
        cJSON *it = cJSON_GetObjectItem(h, "name");
        if (cJSON_IsString(it)) snprintf(state->hadiths[i].name, sizeof(state->hadiths[i].name), "%s", it->valuestring);
        it = cJSON_GetObjectItem(h, "text");
        if (cJSON_IsString(it)) state->hadiths[i].text = dupText(it->valuestring);
        it = cJSON_GetObjectItem(h, "narrator");
        if (cJSON_IsString(it)) snprintf(state->hadiths[i].narrator, sizeof(state->hadiths[i].narrator), "%s", it->valuestring);
        it = cJSON_GetObjectItem(h, "collection");
        if (cJSON_IsString(it)) snprintf(state->hadiths[i].collection, sizeof(state->hadiths[i].collection), "%s", it->valuestring);
    }
    state->totalHadiths = n;
    cJSON_Delete(root);
    return 1;
}

int loadHadiths(AppState *state) {
    freeHadiths(state);
    ensureDataDir();

    Hadith *all = NULL;
    int count = 0, cap = 0, anyCache = 0;
    for (int e = 0; e < 2; e++) {
        printf("Hadiths: %s...\n", HADITH_COLLECTIONS[e]);
        if (!fetchHadithEdition(HADITH_EDITIONS[e], HADITH_CACHE[e])) continue;
        anyCache = 1;
        appendHadithEdition(HADITH_CACHE[e], HADITH_COLLECTIONS[e], &all, &count, &cap);
    }
    if (count > 0) {
        state->hadiths = all;
        state->totalHadiths = count;
        return 1;
    }
    freeHadithArray(all, count);
    if (!anyCache) printf("Hadiths offline — using bundled set.\n");
    return loadBundledHadiths(state);
}

Ayah *getAyah(AppState *state, int surahNum, int ayahNum) {
    if (!state || !state->ayahs) return NULL;
    for (int i = 0; i < state->totalAyahs; i++) {
        if (state->ayahs[i].surahNumber == surahNum &&
            state->ayahs[i].ayahNumber  == ayahNum)
            return &state->ayahs[i];
    }
    return NULL;
}

int getDailyAyahIndex(int totalAyahs) {
    if (totalAyahs <= 0) totalAyahs = 6236;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    /* date-seeded pick — stable all day, reshuffled daily. */
    srand((unsigned)(tm->tm_yday + (tm->tm_year + 1900) * 1000));
    return rand() % totalAyahs;
}

