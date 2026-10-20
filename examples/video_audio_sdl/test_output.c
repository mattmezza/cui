/* SPDX-License-Identifier: MIT */
#include "output.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
static size_t checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        assert(x);                                                                                 \
        checks++;                                                                                  \
    } while (0)
typedef struct {
    int bytes, override;
    bool locked, paused, closed, bad_format, fail_put, fail_pause, fail_clear, fail_resume,
        fail_lock;
} fake;
static bool lock(void *u) {
    fake *f = u;
    CHECK(!f->locked);
    if (f->fail_lock)
        return false;
    f->locked = true;
    return true;
}
static void unlock(void *u) {
    fake *f = u;
    CHECK(f->locked);
    f->locked = false;
}
static bool format(void *u) {
    fake *f = u;
    CHECK(f->locked);
    return !f->bad_format;
}
static int queued(void *u) {
    fake *f = u;
    CHECK(f->locked);
    return f->override ? f->override : f->bytes;
}
static bool put(void *u, const float *pcm, size_t bytes) {
    fake *f = u;
    CHECK(f->locked);
    CHECK(bytes % 8 == 0);
    CHECK(pcm);
    f->bytes += (int)bytes;
    return !f->fail_put;
}
static bool pause_fake(void *u) {
    fake *f = u;
    CHECK(!f->locked);
    if (f->fail_pause)
        return false;
    f->paused = true;
    return true;
}
static bool resume_fake(void *u) {
    fake *f = u;
    CHECK(!f->locked);
    if (f->fail_resume)
        return false;
    f->paused = false;
    return true;
}
static bool clear(void *u) {
    fake *f = u;
    CHECK(f->locked);
    CHECK(f->paused);
    if (f->fail_clear)
        return false;
    f->bytes = 0;
    return true;
}
static void close_fake(void *u) {
    fake *f = u;
    CHECK(!f->locked);
    CHECK(f->paused);
    CHECK(!f->closed);
    f->closed = true;
}
static video_sdl_backend backend(fake *f) {
    return (video_sdl_backend){lock,       unlock,      format, queued,     put,
                               pause_fake, resume_fake, clear,  close_fake, f};
}
typedef struct {
    uint64_t at;
    atomic_bool stalled;
    atomic_uint_fast64_t cancelled;
} source;
static bool seek(void *u, uint64_t g, uint64_t at) {
    source *s = u;
    if (g <= atomic_load(&s->cancelled))
        return false;
    s->at = at;
    return true;
}
static video_audio_read read_pcm(void *u, uint64_t g, float *pcm, size_t cap, uint64_t *first,
                                 size_t *n) {
    source *s = u;
    *n = 0;
    if (g <= atomic_load(&s->cancelled) || atomic_load(&s->stalled))
        return VIDEO_AUDIO_AGAIN;
    if (s->at >= 1024)
        return VIDEO_AUDIO_EOF;
    *n = 1024 - s->at;
    if (*n > cap)
        *n = cap;
    memset(pcm, 0, *n * 8);
    *first = s->at;
    s->at += *n;
    return VIDEO_AUDIO_DATA;
}
static void interrupt(void *u, uint64_t g) {
    source *s = u;
    atomic_store(&s->cancelled, g);
}
static void delay(void) {
    struct timespec t = {0, 1000000};
    thrd_sleep(&t, NULL);
}
static void wait_pcm(video_audio_host *h) {
    for (int i = 0; i < 10000; i++) {
        video_audio_snapshot s = video_audio_poll(h);
        CHECK(!s.failed);
        if (s.pcm_frames >= 64 || s.decoder_eof)
            return;
        delay();
    }
    CHECK(false);
}
static void drain(fake *f, size_t n) {
    CHECK(!f->locked);
    CHECK(!f->paused);
    CHECK(f->bytes >= (int)n * 8);
    f->bytes -= (int)n * 8;
}
static void setup_cases(void) {
    fake f = {.bad_format = true};
    CHECK(!video_sdl_create(backend(&f), 256, 64));
    CHECK(!f.closed);
    f.bad_format = false;
    f.bytes = 8;
    CHECK(!video_sdl_create(backend(&f), 256, 64));
    f.bytes = 0;
    CHECK(!video_sdl_create(backend(&f), 256, 257));
    f.fail_lock = true;
    CHECK(!video_sdl_create(backend(&f), 256, 64));
    f.fail_lock = false;
    f.fail_pause = true;
    CHECK(!video_sdl_create(backend(&f), 256, 64));
    f.fail_pause = false;
    video_sdl_output *out = video_sdl_create(backend(&f), 256, 64);
    CHECK(out);
    source src = {0};
    atomic_init(&src.stalled, false);
    atomic_init(&src.cancelled, 0);
    video_audio_host *h = video_audio_create(
        (video_audio_config){256, 64, {seek, read_pcm, interrupt, &src}, video_sdl_interface(out)});
    CHECK(h);
    f.fail_resume = true;
    CHECK(!video_audio_play(h));
    CHECK(!video_audio_poll(h).playing);
    CHECK(f.paused);
    f.fail_resume = false;
    CHECK(video_audio_play(h));
    CHECK(video_audio_destroy(h));
    CHECK(video_sdl_destroy(out));
    CHECK(f.closed);
    printf("setup: %zu assertions\n", checks);
}
static void unbound_probe(void) {
    CHECK(SDL_setenv_unsafe("SDL_AUDIODRIVER", "dummy", 1) == 0);
    CHECK(SDL_Init(SDL_INIT_AUDIO));
    printf("SDL %d revision %s driver %s\n", SDL_GetVersion(), SDL_GetRevision(),
           SDL_GetCurrentAudioDriver());
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 2, 48000};
    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, &spec);
    CHECK(stream);
    float input[64], output[64];
    for (size_t i = 0; i < 64; i++)
        input[i] = (float)i / 128.0f;
    CHECK(SDL_PutAudioStreamData(stream, input, sizeof(input)));
    CHECK(SDL_GetAudioStreamQueued(stream) == (int)sizeof(input));
    CHECK(SDL_GetAudioStreamData(stream, output, 24) == 24);
    CHECK(memcmp(output, input, 24) == 0);
    CHECK(SDL_GetAudioStreamQueued(stream) == (int)sizeof(input) - 24);
    CHECK(SDL_GetAudioStreamData(stream, output, 40) == 40);
    CHECK(memcmp(output, input + 6, 40) == 0);
    CHECK(SDL_GetAudioStreamQueued(stream) == (int)sizeof(input) - 64);
    CHECK(SDL_ClearAudioStream(stream));
    CHECK(SDL_GetAudioStreamQueued(stream) == 0);
    CHECK(SDL_GetAudioStreamData(stream, output, sizeof(output)) == 0);
    SDL_DestroyAudioStream(stream);
    SDL_Quit();
    printf("unbound: %zu assertions\n", checks);
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    const char *mode = argv[1];
    if (!strcmp(mode, "setup")) {
        setup_cases();
        return 0;
    }
    if (!strcmp(mode, "unbound")) {
        unbound_probe();
        return 0;
    }
    bool dummy = !strcmp(mode, "dummy");
    if (dummy) {
        CHECK(SDL_setenv_unsafe("SDL_AUDIODRIVER", "dummy", 1) == 0);
        CHECK(SDL_Init(SDL_INIT_AUDIO));
        printf("SDL %d revision %s driver %s\n", SDL_GetVersion(), SDL_GetRevision(),
               SDL_GetCurrentAudioDriver());
        CHECK(!strcmp(SDL_GetCurrentAudioDriver(), "dummy"));
    }
    fake f = {0};
    source src = {0};
    atomic_init(&src.stalled, !strcmp(mode, "silence"));
    atomic_init(&src.cancelled, 0);
    video_sdl_output *out =
        dummy ? video_sdl_open(256, 64) : video_sdl_create(backend(&f), 256, 64);
    CHECK(out);
    video_audio_decoder decoder = {seek, read_pcm, interrupt, &src};
    video_audio_host *h =
        video_audio_create((video_audio_config){256, 64, decoder, video_sdl_interface(out)});
    CHECK(h);
    if (strcmp(mode, "silence"))
        wait_pcm(h);
    CHECK(video_audio_play(h));
    if (!strcmp(mode, "drain")) {
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 0);
        drain(&f, 16);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 16);
        CHECK(video_sdl_pump(out));
        CHECK(video_sdl_pump(out));
        CHECK(video_sdl_pump(out));
        CHECK(video_sdl_poll(out).pending_frames == 256);
        CHECK(video_sdl_poll(out).tickets <= 5);
        drain(&f, 100);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 116);
    } else if (!strcmp(mode, "silence")) {
        CHECK(video_sdl_pump(out));
        drain(&f, 64);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 0);
        CHECK(video_audio_poll(h).consumed_silence == 64);
        atomic_store(&src.stalled, false);
        wait_pcm(h);
        drain(&f, 64);
        CHECK(video_sdl_pump(out));
        drain(&f, 64);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 64);
    } else if (!strcmp(mode, "pause")) {
        CHECK(video_sdl_pump(out));
        drain(&f, 16);
        CHECK(video_audio_pause(h));
        CHECK(f.paused);
        CHECK(video_audio_poll(h).position_frames == 16);
        CHECK(f.bytes == 48 * 8);
        CHECK(video_sdl_pump(out));
        CHECK(f.bytes == 48 * 8);
        CHECK(video_audio_play(h));
        drain(&f, 48);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 64);
    } else if (!strcmp(mode, "generations")) {
        CHECK(video_sdl_pump(out));
        drain(&f, 16);
        CHECK(video_audio_seek(h, 400));
        CHECK(video_audio_poll(h).position_frames == 400);
        CHECK(f.bytes == 0);
        CHECK(!video_sdl_poll(out).attached);
        wait_pcm(h);
        CHECK(video_audio_play(h));
        CHECK(video_sdl_pump(out));
        drain(&f, 64);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 464);
    } else if (!strcmp(mode, "failures")) {
        f.fail_put = true;
        CHECK(!video_sdl_pump(out));
        CHECK(video_sdl_poll(out).faulted);
        CHECK(f.paused);
        CHECK(video_sdl_poll(out).submitted_frames == 0);
        CHECK(video_audio_poll(h).position_frames == 0);
        CHECK(video_audio_pause(h));
        CHECK(!video_audio_play(h));
        f.fail_clear = true;
        CHECK(!video_audio_seek(h, 100));
        CHECK(video_audio_poll(h).position_frames == 0);
        f.fail_clear = false;
        f.fail_put = false;
        CHECK(video_audio_seek(h, 100));
        wait_pcm(h);
        CHECK(video_audio_play(h));
        CHECK(video_sdl_pump(out));
        drain(&f, 64);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 164);
    } else if (!strcmp(mode, "units")) {
        int bad[] = {-1, 1, 4096};
        for (size_t i = 0; i < 3; i++) {
            CHECK(video_sdl_pump(out));
            f.override = bad[i];
            CHECK(!video_sdl_pump(out));
            CHECK(video_sdl_poll(out).faulted);
            CHECK(video_audio_poll(h).position_frames == 0);
            f.override = 0;
            CHECK(video_audio_seek(h, 0));
            wait_pcm(h);
            CHECK(video_audio_play(h));
        }
        f.bad_format = true;
        CHECK(!video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames == 0);
        f.bad_format = false;
    } else if (!strcmp(mode, "teardown")) {
        CHECK(video_sdl_pump(out));
        CHECK(!video_sdl_destroy(out));
        f.fail_pause = true;
        CHECK(!video_audio_destroy(h));
        CHECK(!f.closed);
        f.fail_pause = false;
        f.fail_lock = true;
        CHECK(!video_audio_destroy(h));
        f.fail_lock = false;
        f.fail_clear = true;
        CHECK(!video_audio_destroy(h));
        CHECK(!f.closed);
        f.fail_clear = false;
    } else {
        CHECK(dummy);
        CHECK(video_sdl_pump(out));
        for (int i = 0; i < 1000 && video_audio_poll(h).position_frames == 0; i++) {
            delay();
            CHECK(video_sdl_pump(out));
        }
        CHECK(video_audio_poll(h).position_frames > 0);
        CHECK(video_audio_pause(h));
        uint64_t position = video_audio_poll(h).position_frames;
        for (int i = 0; i < 30; i++) {
            delay();
            CHECK(video_sdl_pump(out));
            CHECK(video_audio_poll(h).position_frames == position);
        }
        CHECK(video_audio_play(h));
        for (int i = 0; i < 1000 && video_audio_poll(h).position_frames == position; i++) {
            delay();
            CHECK(video_sdl_pump(out));
        }
        CHECK(video_audio_poll(h).position_frames > position);
        CHECK(video_audio_seek(h, 400));
        wait_pcm(h);
        CHECK(video_audio_play(h));
        CHECK(video_sdl_pump(out));
        for (int i = 0; i < 1000 && video_audio_poll(h).position_frames == 400; i++) {
            delay();
            CHECK(video_sdl_pump(out));
        }
        CHECK(video_audio_poll(h).position_frames > 400);
        CHECK(video_audio_seek(h, 0));
        wait_pcm(h);
        CHECK(video_audio_play(h));
        for (int i = 0; i < 4; i++)
            CHECK(video_sdl_pump(out));
        video_sdl_snapshot before = video_sdl_poll(out);
        CHECK(before.pending_frames <= 256);
        for (int i = 0; i < 80; i++)
            delay(); /* deliberately starve owner pump */
        CHECK(video_audio_poll(h).position_frames == 0);
        CHECK(video_sdl_poll(out).submitted_frames == before.submitted_frames);
        CHECK(video_sdl_pump(out));
        CHECK(video_audio_poll(h).position_frames > 0);
        CHECK(video_audio_poll(h).position_frames <= before.submitted_frames);
    }
    CHECK(video_audio_destroy(h));
    CHECK(!video_sdl_poll(out).attached);
    CHECK(video_sdl_destroy(out));
    if (dummy)
        SDL_Quit();
    else
        CHECK(f.closed);
    printf("%s: %zu assertions\n", mode, checks);
    return 0;
}
