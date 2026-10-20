/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <SDL3/SDL.h>
#include <limits.h>
#include <stdlib.h>

typedef struct {
    SDL_AudioStream *stream;
    speech_input_deliver deliver;
    void *user;
    unsigned channels;
    bool failed;
} sdl_capture;

static void SDLCALL captured(void *user, SDL_AudioStream *stream, int additional, int total) {
    sdl_capture *capture = user;
    (void)additional;
    (void)total;
    if (capture->failed) {
        (void)SDL_ClearAudioStream(stream);
        return;
    }
    int available = SDL_GetAudioStreamAvailable(stream);
    if (available < 0 || available > 8192) {
        capture->failed = true;
        (void)capture->deliver(capture->user, NULL, 0,
                               available < 0 ? CUI_SPEECH_INPUT_ERROR : CUI_SPEECH_CAPACITY);
    } else {
        int frame_bytes = (int)(capture->channels * sizeof(int16_t));
        /* Read only the initial snapshot, bounded to 4096 samples/callback. */
        available -= available % frame_bytes;
        while (available > 0 && !capture->failed) {
            int16_t samples[256];
            int request = available < (int)sizeof samples ? available : (int)sizeof samples;
            request -= request % frame_bytes;
            int bytes = SDL_GetAudioStreamData(stream, samples, request);
            if (bytes <= 0 || bytes % frame_bytes) {
                capture->failed = true;
                (void)capture->deliver(capture->user, NULL, 0, CUI_SPEECH_INPUT_ERROR);
                break;
            }
            available -= bytes;
            if (!capture->deliver(capture->user, samples, (size_t)bytes / sizeof samples[0],
                                  CUI_SPEECH_OK))
                capture->failed = true;
        }
    }
    if (capture->failed)
        (void)SDL_ClearAudioStream(stream);
}

static cui_speech_error open_device(void *user, cui_speech_format format,
                                    speech_input_deliver deliver, void *delivery_user,
                                    void **token) {
    (void)user;
    *token = NULL;
    if (!SDL_WasInit(SDL_INIT_AUDIO))
        return CUI_SPEECH_UNSUPPORTED;
    if (!format.channels || format.channels > 256 || !format.sample_rate ||
        format.sample_rate > INT_MAX)
        return CUI_SPEECH_INVALID;
    sdl_capture *capture = calloc(1, sizeof *capture);
    if (!capture)
        return CUI_SPEECH_CAPACITY;
    capture->deliver = deliver;
    capture->user = delivery_user;
    capture->channels = format.channels;
    SDL_AudioSpec spec = {SDL_AUDIO_S16, format.channels, (int)format.sample_rate};
    capture->stream =
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, captured, capture);
    if (!capture->stream || !SDL_ResumeAudioStreamDevice(capture->stream)) {
        if (capture->stream)
            SDL_DestroyAudioStream(capture->stream);
        free(capture);
        /* SDL failure strings are not portable permission/no-device codes. */
        return CUI_SPEECH_INPUT_ERROR;
    }
    *token = capture;
    return CUI_SPEECH_OK;
}

static void close_device(void *user, void *token) {
    (void)user;
    sdl_capture *capture = token;
    /* Detach the producer under SDL's stream lock before destroying its user
     * storage. This waits out any callback currently holding that same lock. */
    (void)SDL_SetAudioStreamPutCallback(capture->stream, NULL, NULL);
    SDL_DestroyAudioStream(capture->stream);
    free(capture);
}

speech_input_device_ops speech_input_sdl_device_ops(void) {
    return (speech_input_device_ops){NULL, open_device, close_device};
}
