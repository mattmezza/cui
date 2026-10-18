/* SPDX-License-Identifier: MIT */
#include "curl_transport.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
typedef struct { af_result *result; af_cancel_fn cancel; void *user; } transfer;
static size_t receive(char *data, size_t size, size_t count, void *user) {
    transfer *t=user;
    if (count && size > SIZE_MAX/count) { t->result->status=AF_TOO_LARGE; return 0; }
    size_t bytes=size*count;
    if (bytes > AF_BODY_MAX-t->result->size) { t->result->status=AF_TOO_LARGE; return 0; }
    memcpy(t->result->body+t->result->size,data,bytes); t->result->size+=bytes;
    return bytes;
}
static int progress(void *user, curl_off_t total, curl_off_t now, curl_off_t up, curl_off_t sent) {
    (void)total; (void)now; (void)up; (void)sent;
    transfer *t=user; return t->cancel(t->user) ? 1 : 0;
}
bool af_curl_initialize(void) {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return false;
    const curl_version_info_data *info=curl_version_info(CURLVERSION_NOW);
    /* NOSIGNAL + synchronous DNS cannot promise the configured timeout. */
    if (!info || !(info->features&CURL_VERSION_ASYNCHDNS) || !(info->features&CURL_VERSION_SSL)) {
        curl_global_cleanup(); return false;
    }
    return true;
}
void af_curl_shutdown(void) { curl_global_cleanup(); }
void af_curl_transport(void *unused, const char *url, af_cancel_fn cancel, void *user, af_result *r) {
    (void)unused;
    if (cancel(user)) { r->status=AF_CANCELLED; return; }
    CURL *easy=curl_easy_init();
    if (!easy) { r->status=AF_FAILURE; snprintf(r->error,sizeof r->error,"curl initialization failed"); return; }
    transfer t={r,cancel,user}; r->status=AF_SUCCESS;
    CURLcode code=CURLE_OK;
#define SET(option,value) do { if (code == CURLE_OK) code=curl_easy_setopt(easy,option,value); } while (0)
    SET(CURLOPT_URL,url); SET(CURLOPT_PROTOCOLS_STR,"https"); SET(CURLOPT_REDIR_PROTOCOLS_STR,"https");
    SET(CURLOPT_FOLLOWLOCATION,1L); SET(CURLOPT_MAXREDIRS,3L);
    SET(CURLOPT_CONNECTTIMEOUT_MS,2000L); SET(CURLOPT_TIMEOUT_MS,8000L); SET(CURLOPT_NOSIGNAL,1L);
    SET(CURLOPT_SSL_VERIFYPEER,1L); SET(CURLOPT_SSL_VERIFYHOST,2L);
    SET(CURLOPT_WRITEFUNCTION,receive); SET(CURLOPT_WRITEDATA,&t);
    SET(CURLOPT_NOPROGRESS,0L); SET(CURLOPT_XFERINFOFUNCTION,progress); SET(CURLOPT_XFERINFODATA,&t);
#undef SET
    if (code == CURLE_OK) code=curl_easy_perform(easy);
    if (code == CURLE_OK) code=curl_easy_getinfo(easy,CURLINFO_RESPONSE_CODE,&r->http_status);
    if (cancel(user)) r->status=AF_CANCELLED;
    else if (r->status == AF_TOO_LARGE) snprintf(r->error,sizeof r->error,"response exceeds %d bytes",AF_BODY_MAX);
    else if (code != CURLE_OK) { r->status=AF_FAILURE; snprintf(r->error,sizeof r->error,"%s",curl_easy_strerror(code)); }
    else if (r->http_status < 200 || r->http_status >= 300) {
        r->status=AF_FAILURE; snprintf(r->error,sizeof r->error,"HTTP %ld",r->http_status);
    }
    r->body[r->size]=0; curl_easy_cleanup(easy);
}
