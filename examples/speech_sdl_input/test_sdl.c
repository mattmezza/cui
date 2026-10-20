/* SPDX-License-Identifier: MIT */
/* Linux linker injection: real unbound conversion streams, fake device ops. */
#include "host.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static SDL_AudioStream *stream;
static bool initialized = true, fail_open, fail_resume, reject_delivery;
static unsigned closed, detached, delivered;
static cui_speech_error last_error;
static int16_t last_sample;
extern bool __real_SDL_SetAudioStreamPutCallback(SDL_AudioStream *, SDL_AudioStreamCallback,
                                                 void *);
extern void __real_SDL_DestroyAudioStream(SDL_AudioStream *);
SDL_InitFlags __wrap_SDL_WasInit(SDL_InitFlags flags) {
    return initialized ? flags : 0;
}
SDL_AudioStream *__wrap_SDL_OpenAudioDeviceStream(SDL_AudioDeviceID device,
                                                  const SDL_AudioSpec *spec,
                                                  SDL_AudioStreamCallback callback, void *user) {
    CHECK(device == SDL_AUDIO_DEVICE_DEFAULT_RECORDING);
    if (fail_open)
        return NULL;
    stream = SDL_CreateAudioStream(spec, spec);
    CHECK(stream && __real_SDL_SetAudioStreamPutCallback(stream, callback, user));
    return stream;
}
bool __wrap_SDL_ResumeAudioStreamDevice(SDL_AudioStream *value) {
    CHECK(value == stream);
    return !fail_resume;
}
bool __wrap_SDL_SetAudioStreamPutCallback(SDL_AudioStream *value, SDL_AudioStreamCallback callback,
                                          void *user) {
    CHECK(value == stream && !callback && !user);
    detached++;
    return __real_SDL_SetAudioStreamPutCallback(value, callback, user);
}
void __wrap_SDL_DestroyAudioStream(SDL_AudioStream *value) {
    CHECK(value == stream);
    closed++;
    __real_SDL_DestroyAudioStream(value);
    stream = NULL;
}
static bool deliver(void *user, const int16_t *samples, size_t count, cui_speech_error error) {
    CHECK(user == &delivered);
    if (count)
        last_sample = samples[count - 1];
    delivered += (unsigned)count;
    last_error = error;
    return !reject_delivery;
}
int main(void) {
    speech_input_device_ops ops = speech_input_sdl_device_ops();
    cui_speech_format format = {16000, 1};
    void *token = NULL;
    initialized = false;
    CHECK(ops.open(ops.user, format, deliver, &delivered, &token) == CUI_SPEECH_UNSUPPORTED &&
          !token);
    initialized = true;
    fail_open = true;
    CHECK(ops.open(ops.user, format, deliver, &delivered, &token) == CUI_SPEECH_INPUT_ERROR &&
          !token);
    fail_open = false;
    fail_resume = true;
    CHECK(ops.open(ops.user, format, deliver, &delivered, &token) == CUI_SPEECH_INPUT_ERROR &&
          !token);
    CHECK(closed == 1);
    fail_resume = false;
    CHECK(ops.open(ops.user, format, deliver, &delivered, &token) == CUI_SPEECH_OK && token);
    int16_t samples[5000];
    for (size_t i = 0; i < 5000; i++)
        samples[i] = (int16_t)i;
    CHECK(SDL_PutAudioStreamData(stream, samples, 400));
    CHECK(delivered == 200 && last_sample == 199 && last_error == CUI_SPEECH_OK);
    reject_delivery = true;
    CHECK(SDL_PutAudioStreamData(stream, samples, 400));
    unsigned previous = delivered;
    CHECK(SDL_PutAudioStreamData(stream, samples, 400));
    CHECK(delivered == previous && SDL_GetAudioStreamAvailable(stream) == 0);
    ops.close(ops.user, token);
    CHECK(closed == 2 && detached == 1);
    reject_delivery = false;
    CHECK(ops.open(ops.user, format, deliver, &delivered, &token) == CUI_SPEECH_OK);
    CHECK(SDL_PutAudioStreamData(stream, samples, sizeof samples));
    CHECK(last_error == CUI_SPEECH_CAPACITY && SDL_GetAudioStreamAvailable(stream) == 0);
    ops.close(ops.user, token);
    CHECK(closed == 3 && detached == 2);
    initialized = false;
    CHECK(!SDL_WasInit(SDL_INIT_AUDIO)); /* No real initialization occurred. */
    puts("PASS: injected SDL open/resume cleanup, callback drain/reject/overflow, detach/close");
    return 0;
}
