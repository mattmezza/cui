/* SPDX-License-Identifier: MIT */
#ifndef SPEECH_RECOGNIZER_HOST_H
#define SPEECH_RECOGNIZER_HOST_H
#include "cui/cui_speech.h"
/* Application-host interfaces, not public framework APIs. */
#define SPEECH_RECOGNIZER_ENDPOINTS 4
#define SPEECH_RECOGNIZER_EVENTS 8
#define SPEECH_RECOGNIZER_TEXT 4096

typedef struct speech_recognizer_host speech_recognizer_host;
typedef struct speech_recognizer_endpoint speech_recognizer_endpoint;
typedef enum {
    SPEECH_AUTH_UNKNOWN,
    SPEECH_AUTH_GRANTED,
    SPEECH_AUTH_DENIED,
    SPEECH_AUTH_RESTRICTED
} speech_authorization;
typedef struct {
    bool supported, usage_description, locale_supported, on_device, available;
    speech_authorization authorization;
    cui_speech_format format;
} speech_service_status;
/* Mutex implementations are private platform code. lock/unlock must succeed;
 * create returns NULL on failure. Never dispatch synchronously to owner/UI. */
typedef struct {
    void *(*create)(void);
    void (*destroy)(void *);
    void (*lock)(void *);
    void (*unlock)(void *);
} speech_mutex_ops;
typedef bool (*speech_recognizer_wake)(void *);
/* Services may complete synchronously. Retain an endpoint before storing it
 * for asynchronous use; release every lease when its callback block dies.
 * close releases a token exactly once, but need not stop late OS callbacks.
 * Callback leases contain no host/sink pointer. append copies before return. */
typedef struct {
    void *user;
    speech_service_status (*query)(void *);
    cui_speech_error (*authorize)(void *, speech_recognizer_endpoint *, void **token);
    cui_speech_error (*start)(void *, speech_recognizer_endpoint *, bool on_device, void **token);
    cui_speech_error (*append)(void *, void *, const int16_t *, size_t);
    void (*finish)(void *, void *);
    void (*close)(void *, void *);
} speech_recognition_service;

speech_recognizer_host *speech_recognizer_host_create(speech_recognition_service, speech_mutex_ops,
                                                      speech_recognizer_wake, void *);
/* Owner-thread only. Reject destruction of an active CUI session. Pending
 * permission is detached safely; retired callback leases may outlive the host. */
bool speech_recognizer_host_destroy(speech_recognizer_host *);
/* Explicit user action only: this may show Speech consent UI in a native
 * service. Never calls microphone authorization or capture operations. */
cui_speech_error speech_recognizer_host_prepare(speech_recognizer_host *);
void speech_recognizer_host_refresh(speech_recognizer_host *);
speech_service_status speech_recognizer_host_status(const speech_recognizer_host *);
cui_speech_error speech_recognizer_host_last_error(const speech_recognizer_host *);
bool speech_recognizer_format_supported(cui_speech_format);
cui_speech_recognizer speech_recognizer_host_provider(speech_recognizer_host *);
/* Report at most budget events, then caller must pump CUI (including stop
 * deadlines). Never call from service/wake/event callbacks. */
bool speech_recognizer_host_poll(speech_recognizer_host *, size_t budget);

bool speech_recognizer_endpoint_retain(speech_recognizer_endpoint *);
void speech_recognizer_endpoint_release(speech_recognizer_endpoint *);
bool speech_recognizer_endpoint_authorized(speech_recognizer_endpoint *, speech_authorization);
/* One atomic event can contain text and an error: error takes precedence over
 * final, preserving text as hypothesis and failing the session. UTF-8 span is
 * copied before return; embedded NUL and malformed/oversized text fail. */
bool speech_recognizer_endpoint_result(speech_recognizer_endpoint *, cui_str, bool final,
                                       bool ends_utterance, cui_speech_error);
bool speech_recognizer_endpoint_error(speech_recognizer_endpoint *, cui_speech_error);
/* POSIX mutex implementation for Linux/macOS example builds, no public types. */
speech_mutex_ops speech_recognizer_posix_mutex(void);
#endif
