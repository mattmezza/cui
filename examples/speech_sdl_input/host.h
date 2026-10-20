/* SPDX-License-Identifier: MIT */
#ifndef SPEECH_INPUT_HOST_H
#define SPEECH_INPUT_HOST_H
#include "cui/cui_speech.h"
/* Application-host API; no platform types. All calls except delivery/wake are
 * owner-thread only. open/close must obey the README quiescence contract. */
typedef struct speech_input_host speech_input_host;
typedef bool (*speech_input_deliver)(void *, const int16_t *, size_t, cui_speech_error);
typedef struct {
    void *user;
    cui_speech_error (*open)(void *, cui_speech_format, speech_input_deliver, void *, void **);
    void (*close)(void *, void *);
} speech_input_device_ops;
typedef bool (*speech_input_wake)(void *);
speech_input_host *speech_input_host_create(size_t capacity_samples, speech_input_device_ops,
                                            speech_input_wake, void *wake_user);
/* False while capture is active; first stop/cancel/dispose the CUI state. */
bool speech_input_host_destroy(speech_input_host *);
cui_speech_input speech_input_host_provider(speech_input_host *);
/* Copies at most max_samples from host ring into CUI. now is monotonic seconds.
 * Call cui_speech_pump afterwards, including when this returns false. */
bool speech_input_host_poll(speech_input_host *, double now, size_t max_samples);
/* SDL audio subsystem initialization/termination belongs to the application.
 * Calling this factory opens nothing. open is explicit through Speech start. */
speech_input_device_ops speech_input_sdl_device_ops(void);
#endif
