/* Stub curl/curl.h for standalone backend testing.
 * The real curl headers are provided by libcurl at build time.
 * This stub makes curl_easy_init() return NULL so fetches fail
 * gracefully — the test driver handles this case. */
#ifndef CURL_STUB_H
#define CURL_STUB_H

#include <stddef.h>

typedef void CURL;
typedef int CURLcode;

#define CURLE_OK 0
#define CURLE_FAILED_INIT 2

#define CURLOPT_URL            10002
#define CURLOPT_WRITEFUNCTION  20011
#define CURLOPT_WRITEDATA      10001
#define CURLOPT_FOLLOWLOCATION  52
#define CURLOPT_TIMEOUT         13
#define CURLOPT_SSL_VERIFYPEER  64
#define CURLOPT_SSL_VERIFYHOST  81

static CURL *curl_easy_init(void) { return NULL; }
static CURLcode curl_easy_setopt(CURL *c, int opt, ...) { (void)c; (void)opt; return 0; }
static CURLcode curl_easy_perform(CURL *c) { (void)c; return CURLE_FAILED_INIT; }
static void curl_easy_cleanup(CURL *c) { (void)c; }
static const char *curl_easy_strerror(CURLcode c) { (void)c; return "stub: no libcurl"; }

#endif
