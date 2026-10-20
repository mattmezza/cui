/* SPDX-License-Identifier: MIT */
/* Offline feasibility fixture: no SDL_Init, audio device, or recognition. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *message) {
    fprintf(stderr, "%s: %s\n", message, SDL_GetError());
    return 1;
}

static int identity(void) {
    const SDL_AudioSpec format = {SDL_AUDIO_S16, 1, 16000};
    const int16_t input[] = {INT16_MIN, -1000, -1, 0, 1, 1000, INT16_MAX};
    int16_t output[sizeof input / sizeof input[0]];
    SDL_AudioStream *stream = SDL_CreateAudioStream(&format, &format);
    if (!stream)
        return fail("identity stream");
    int result = 0;
    if (!SDL_PutAudioStreamData(stream, input, 3 * sizeof input[0]) ||
        !SDL_PutAudioStreamData(stream, input + 3, sizeof input - 3 * sizeof input[0]) ||
        !SDL_FlushAudioStream(stream))
        result = fail("identity input");
    else if (SDL_GetAudioStreamData(stream, output, sizeof output) != (int)sizeof output ||
             memcmp(input, output, sizeof input) != 0 || SDL_GetAudioStreamAvailable(stream) != 0)
        result = fail("identity count, samples, or trailing bytes");
    SDL_DestroyAudioStream(stream);
    return result;
}

static int resample(void) {
    const SDL_AudioSpec source = {SDL_AUDIO_F32, 2, 48000};
    const SDL_AudioSpec target = {SDL_AUDIO_S16, 1, 16000};
    /* 100 ms, split into ten 10 ms input chunks. Different channel values
     * exercise downmixing: the mono interior should be approximately .25. */
    float samples[960];
    int16_t output[1600];
    for (int i = 0; i < 960; i += 2) {
        samples[i] = .125f;
        samples[i + 1] = .375f;
    }
    SDL_AudioStream *stream = SDL_CreateAudioStream(&source, &target);
    if (!stream)
        return fail("resampling stream");
    int result = 0;
    for (int i = 0; i < 10 && !result; i++)
        if (!SDL_PutAudioStreamData(stream, samples, sizeof samples))
            result = fail("resampling chunk");
    if (!result && !SDL_FlushAudioStream(stream))
        result = fail("resampling flush");
    if (!result && (SDL_GetAudioStreamData(stream, output, sizeof output) != (int)sizeof output ||
                    SDL_GetAudioStreamAvailable(stream) != 0))
        result = fail("resampling frame count or trailing bytes");
    /* Ignore edge filter transients. Allow rounding/filter differences within
     * 100 PCM16 units of .25 full scale; this is not a bit-exact filter test. */
    for (int i = 160; i < 1440 && !result; i++)
        if (output[i] < 8092 || output[i] > 8292)
            result = fail("resampling interior signal tolerance");
    SDL_DestroyAudioStream(stream);
    return result;
}

int main(void) {
    if (identity() || resample())
        return 1;
    puts("PASS: exact PCM16 identity; 4800 stereo float frames -> 1600 mono PCM16 frames");
    puts("No audio subsystem or device initialized; no capture or recognition tested.");
    return 0;
}
