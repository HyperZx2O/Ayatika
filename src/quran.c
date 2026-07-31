#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <curl/curl.h>
/* quran.h includes <raylib.h>, whose function names (CloseWindow, ShowCursor,
   DrawText, LoadImage, PlaySound) collide with Windows API macros/functions
   pulled in by <curl/curl.h>.  No backend struct uses raylib types, so we
   short-circuit raylib.h via its include guard to avoid the conflict. */
#define RAYLIB_H
#include "quran.h"
#include "../lib/cJSON.h"

typedef struct {
    char   *data;
    size_t  size;
} CurlBuffer;

static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    CurlBuffer *buf = (CurlBuffer *)userp;
    char *tmp = realloc(buf->data, buf->size + total + 1);
    if (!tmp) return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->size, contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static int fetchAndSave(const char *url, const char *filepath) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    CurlBuffer buf = { malloc(1), 0 };
    if (!buf.data) { curl_easy_cleanup(curl); return 0; }
    buf.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
        free(buf.data);
        return 0;
    }

    FILE *f = fopen(filepath, "w");
    if (!f) { free(buf.data); return 0; }
    fputs(buf.data, f);
    fclose(f);
    free(buf.data);
    return 1;
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
                    strncpy(a->arabicText, item->valuestring, sizeof(a->arabicText) - 1);

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
                        strncpy(state->ayahs[k].translationBn, text->valuestring,
                                sizeof(state->ayahs[k].translationBn) - 1);
                    else
                        strncpy(state->ayahs[k].translationEn, text->valuestring,
                                sizeof(state->ayahs[k].translationEn) - 1);
                    break;
                }
            }
        }
    }

    cJSON_Delete(root);
    return 1;
}

static void ensureDataDir(void) {
    mkdir("data");
}

int loadQuranData(AppState *state) {
    ensureDataDir();

    if (access("data/quran.json", F_OK) != 0) {
        printf("First run: fetching Quran data...\n");
        if (!fetchAndSave(
                "https://api.alquran.cloud/v1/quran/quran-uthmani",
                "data/quran.json"))
            return 0;
    }

    if (!parseQuranJSON(state, "data/quran.json")) return 0;

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

    return 1;
}

Ayah *getAyah(AppState *state, int surahNum, int ayahNum) {
    for (int i = 0; i < state->totalAyahs; i++) {
        if (state->ayahs[i].surahNumber == surahNum &&
            state->ayahs[i].ayahNumber  == ayahNum)
            return &state->ayahs[i];
    }
    return NULL;
}

int getAyahIndex(AppState *state, int surahNum, int ayahNum) {
    for (int i = 0; i < state->totalAyahs; i++) {
        if (state->ayahs[i].surahNumber == surahNum &&
            state->ayahs[i].ayahNumber  == ayahNum)
            return i;
    }
    return -1;
}

int getDailyAyahIndex(int totalAyahs) {
    (void)totalAyahs;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int doy = tm->tm_yday;
    return doy % 6236;
}
