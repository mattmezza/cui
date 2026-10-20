/* SPDX-License-Identifier: MIT */
#ifndef SPEECH_RECOGNIZER_MACOS_H
#define SPEECH_RECOGNIZER_MACOS_H
#include "host.h"
/* Optional native service. All owner operations require macOS main thread.
 * locale is a BCP-47 tag, or NULL for current locale. Creation never prompts.
 * Destroy only after its C11 provider host is destroyed; late callback leases
 * are independent and may still outlive this service. No microphone access. */
typedef struct speech_macos_service speech_macos_service;
speech_macos_service *speech_macos_service_create(const char *locale);
speech_recognition_service speech_macos_service_ops(speech_macos_service *);
void speech_macos_service_destroy(speech_macos_service *);
#endif
