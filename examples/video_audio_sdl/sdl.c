/* SPDX-License-Identifier: MIT */
#include "output.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
typedef struct {
    SDL_AudioStream *stream;
} backend;
static bool lock(void *u) {
    return SDL_LockAudioStream(((backend *)u)->stream);
}
static void unlock(void *u) {
    SDL_UnlockAudioStream(((backend *)u)->stream);
}
static bool format(void *u) {
    SDL_AudioStream *stream = ((backend *)u)->stream;
    SDL_AudioSpec in, out;
    return SDL_GetAudioStreamFormat(stream, &in, &out) && in.freq == 48000 && out.freq == 48000 &&
           in.channels == 2 && out.channels == 2 && in.format == SDL_AUDIO_F32 &&
           out.format == SDL_AUDIO_F32 && SDL_GetAudioStreamFrequencyRatio(stream) == 1.0f;
}
static int queued(void *u) {
    return SDL_GetAudioStreamQueued(((backend *)u)->stream);
}
static bool put(void *u, const float *pcm, size_t bytes) {
    return SDL_PutAudioStreamData(((backend *)u)->stream, pcm, (int)bytes);
}
static bool pause_device(void *u) {
    return SDL_PauseAudioStreamDevice(((backend *)u)->stream);
}
static bool resume_device(void *u) {
    return SDL_ResumeAudioStreamDevice(((backend *)u)->stream);
}
static bool clear(void *u) {
    return SDL_ClearAudioStream(((backend *)u)->stream);
}
static void close_backend(void *u) {
    backend *b = u;
    SDL_DestroyAudioStream(b->stream);
    free(b);
}
video_sdl_output *video_sdl_open(size_t max, size_t chunk) {
    if (SDL_GetVersion() != SDL_VERSIONNUM(3, 4, 18))
        return NULL;
    backend *b = calloc(1, sizeof(*b));
    if (!b)
        return NULL;
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 2, 48000};
    /* No callbacks, userdata, no-copy leases or pointers to the controller are
     * ever registered with SDL. Destroy stream closes its logical device. */
    b->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!b->stream) {
        free(b);
        return NULL;
    }
    video_sdl_backend ops = {lock,         unlock,        format, queued,        put,
                             pause_device, resume_device, clear,  close_backend, b};
    video_sdl_output *s = video_sdl_create(ops, max, chunk);
    if (!s)
        close_backend(b);
    return s;
}
