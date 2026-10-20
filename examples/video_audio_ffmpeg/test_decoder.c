/* SPDX-License-Identifier: MIT */
#include "decoder.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
static size_t assertions;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        assert(x);                                                                                 \
        assertions++;                                                                              \
    } while (0)
static void path(char *out, const char *dir, const char *file) {
    CHECK(snprintf(out, 4096, "%s/%s", dir, file) > 0);
}
static float reference[60000 * 2], actual[60000 * 2];
static size_t load(const char *dir, const char *name) {
    char p[4096];
    path(p, dir, name);
    FILE *f = fopen(p, "rb");
    CHECK(f);
    size_t count = fread(reference, sizeof(float) * 2, 60000, f);
    CHECK(!ferror(f));
    fclose(f);
    return count;
}
static size_t decode(video_audio_decoder d, uint64_t gen, uint64_t target, bool success) {
    CHECK(d.seek(d.user, gen, target));
    size_t total = 0;
    for (size_t calls = 0; calls < 100000; calls++) {
        uint64_t first = 0;
        size_t n = 0;
        video_audio_read r = d.read(d.user, gen, actual + total * 2, 137, &first, &n);
        if (r == VIDEO_AUDIO_EOF) {
            CHECK(success);
            return total;
        }
        if (r == VIDEO_AUDIO_ERROR) {
            CHECK(!success);
            return total;
        }
        CHECK(r == VIDEO_AUDIO_DATA);
        CHECK(n && n <= 137);
        CHECK(first == target + total);
        total += n;
        CHECK(total < 59000);
    }
    CHECK(false);
    return 0;
}
static bool start(void *u, video_audio_host *h) {
    (void)u;
    (void)h;
    return true;
}
static bool stop(void *u, bool discard) {
    (void)u;
    (void)discard;
    return true;
}
static void wait_pcm(video_audio_host *h) {
    for (int i = 0; i < 10000; i++) {
        video_audio_snapshot s = video_audio_poll(h);
        CHECK(!s.failed);
        if (s.pcm_frames >= 128 || s.decoder_eof)
            return;
        struct timespec delay = {0, 1000000};
        thrd_sleep(&delay, NULL);
    }
    CHECK(false);
}
typedef struct {
    video_audio_decoder inner;
    mtx_t mutex;
    cnd_t changed;
    bool entered, released;
} gate;
static bool gated_seek(void *u, uint64_t g, uint64_t f) {
    gate *x = u;
    return x->inner.seek(x->inner.user, g, f);
}
static video_audio_read gated_read(void *u, uint64_t g, float *pcm, size_t cap, uint64_t *first,
                                   size_t *n) {
    gate *x = u;
    video_audio_read result = x->inner.read(x->inner.user, g, pcm, cap, first, n);
    mtx_lock(&x->mutex);
    if (!x->entered) {
        x->entered = true;
        cnd_broadcast(&x->changed);
        while (!x->released)
            cnd_wait(&x->changed, &x->mutex);
    }
    mtx_unlock(&x->mutex);
    return result;
}
static void gated_interrupt(void *u, uint64_t g) {
    gate *x = u;
    x->inner.interrupt(x->inner.user, g);
    mtx_lock(&x->mutex);
    x->released = true;
    cnd_broadcast(&x->changed);
    mtx_unlock(&x->mutex);
}
static void gated_cases(const char *p) {
    for (int shutdown = 0; shutdown < 2; shutdown++) {
        video_audio_ffmpeg *s = video_ffmpeg_create(p, (video_ffmpeg_limits){0});
        CHECK(s);
        gate x = {.inner = video_ffmpeg_interface(s)};
        CHECK(mtx_init(&x.mutex, mtx_plain) == thrd_success);
        CHECK(cnd_init(&x.changed) == thrd_success);
        video_audio_decoder d = {gated_seek, gated_read, gated_interrupt, &x};
        video_audio_host *h =
            video_audio_create((video_audio_config){512, 128, d, {start, stop, NULL}});
        CHECK(h);
        mtx_lock(&x.mutex);
        while (!x.entered)
            cnd_wait(&x.changed, &x.mutex);
        mtx_unlock(&x.mutex);
        if (!shutdown) {
            CHECK(video_audio_seek(h, 7301));
            wait_pcm(h);
            CHECK(video_audio_play(h));
            video_audio_ticket t;
            CHECK(video_audio_render(h, actual, 128, &t));
            for (size_t i = 0; i < 256; i++)
                CHECK(actual[i] == 0.0f);
            CHECK(video_audio_consume(h, t.generation, t.first_sequence, 128));
            CHECK(video_audio_poll(h).position_frames == 7429);
        }
        CHECK(video_audio_destroy(h));
        video_ffmpeg_destroy(s);
        cnd_destroy(&x.changed);
        mtx_destroy(&x.mutex);
    }
}
int main(int argc, char **argv) {
    CHECK(argc == 3);
    const char *mode = argv[1], *dir = argv[2];
    char p[4096];
    path(p, dir,
         !strcmp(mode, "resample") ? "44100-0.mkv"
         : !strcmp(mode, "offset") ? "48000-125.mkv"
                                   : "48000-0.mkv");
    video_audio_ffmpeg *s = video_ffmpeg_create(p, (video_ffmpeg_limits){0});
    CHECK(s);
    video_audio_decoder d = video_ffmpeg_interface(s);
    if (!strcmp(mode, "pcm") || !strcmp(mode, "resample") || !strcmp(mode, "offset")) {
        size_t expected = load(dir, !strcmp(mode, "resample") ? "44100.f32" : "48000.f32");
        size_t gap = !strcmp(mode, "offset") ? 6000 : 0;
        size_t n = decode(d, 1, 0, true);
        CHECK(n == expected + gap);
        CHECK(expected == 24000);
        for (size_t i = 0; i < n * 2; i++)
            CHECK(fabsf(actual[i] - (i < gap * 2 ? 0 : reference[i - gap * 2])) < 0.000002f);
        CHECK(reference[0] > 0.3f);
        CHECK(reference[1] < -0.2f);
        video_ffmpeg_snapshot snap = video_ffmpeg_poll(s);
        CHECK(snap.video_packets == 10);
        CHECK(snap.status == VIDEO_FFMPEG_EOF);
        CHECK(snap.input_samples == (!strcmp(mode, "resample") ? 22050u : 24000u));
        size_t target = 7301;
        n = decode(d, 2, target, true);
        CHECK(n == expected + gap - target);
        for (size_t i = 0; i < n * 2; i++)
            CHECK(fabsf(actual[i] - reference[i + (target - gap) * 2]) < 0.000002f);
        CHECK(decode(d, 3, 40000, true) == 0);
    } else if (!strcmp(mode, "limits")) {
        video_ffmpeg_destroy(s);
        video_ffmpeg_limits cases[] = {{.packet_bytes = 1},
                                       {.converted_frames = 16},
                                       {.packets = 1},
                                       {.timeline_frames = 1000}};
        for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            s = video_ffmpeg_create(p, cases[i]);
            CHECK(s);
            d = video_ffmpeg_interface(s);
            decode(d, 1, 0, false);
            CHECK(video_ffmpeg_poll(s).status == VIDEO_FFMPEG_LIMIT);
            video_ffmpeg_destroy(s);
        }
        path(p, dir, "broken.mkv");
        s = video_ffmpeg_create(p, (video_ffmpeg_limits){0});
        CHECK(s);
        d = video_ffmpeg_interface(s);
        CHECK(!d.seek(d.user, 1, 0));
        CHECK(video_ffmpeg_poll(s).status == VIDEO_FFMPEG_IO);
        CHECK(!video_ffmpeg_create("https://invalid", (video_ffmpeg_limits){0}));
    } else if (!strcmp(mode, "unsupported")) {
        video_ffmpeg_destroy(s);
        const char *files[] = {"compressed.mkv", "late.mkv", "discontinuous.mkv"};
        for (size_t i = 0; i < 3; i++) {
            path(p, dir, files[i]);
            s = video_ffmpeg_create(p, (video_ffmpeg_limits){0});
            CHECK(s);
            d = video_ffmpeg_interface(s);
            if (!i) {
                CHECK(!d.seek(d.user, 1, 0));
                CHECK(video_ffmpeg_poll(s).status == VIDEO_FFMPEG_UNSUPPORTED);
            } else {
                decode(d, 1, 0, false);
                CHECK(video_ffmpeg_poll(s).status == VIDEO_FFMPEG_TIMESTAMP);
            }
            if (i < 2)
                video_ffmpeg_destroy(s);
        }
    } else if (!strcmp(mode, "cancel")) {
        d.interrupt(d.user, 1);
        CHECK(!d.seek(d.user, 1, 0));
        CHECK(video_ffmpeg_poll(s).status == VIDEO_FFMPEG_CANCELLED);
        CHECK(decode(d, 2, 0, true) == 24000);
        d.interrupt(d.user, 1);
        CHECK(decode(d, 3, 7301, true) == 16699);
        d.interrupt(d.user, 3);
        size_t n = 9;
        uint64_t first = 0;
        CHECK(d.read(d.user, 3, actual, 8, &first, &n) == VIDEO_AUDIO_AGAIN);
        CHECK(n == 0);
    } else if (!strcmp(mode, "stale_shutdown")) {
        gated_cases(p);
    } else {
        CHECK(!strcmp(mode, "controller"));
        video_audio_host *h =
            video_audio_create((video_audio_config){512, 128, d, {start, stop, NULL}});
        CHECK(h);
        for (int tries = 0; video_audio_poll(h).pcm_frames < 512; tries++) {
            CHECK(tries < 10000);
            CHECK(!video_audio_poll(h).failed);
            struct timespec delay = {0, 1000000};
            thrd_sleep(&delay, NULL);
        }
        video_ffmpeg_snapshot full = video_ffmpeg_poll(s);
        for (int tries = 0; tries < 100; tries++) {
            CHECK(video_audio_poll(h).pcm_frames == 512);
            CHECK(video_ffmpeg_poll(s).converted_frames == full.converted_frames);
            thrd_yield();
        }
        CHECK(video_audio_play(h));
        wait_pcm(h);
        video_audio_ticket ticket;
        CHECK(video_audio_render(h, actual, 128, &ticket));
        CHECK(video_audio_poll(h).position_frames == 0);
        CHECK(video_audio_consume(h, ticket.generation, ticket.first_sequence, 128));
        CHECK(video_audio_poll(h).position_frames == 128);
        for (size_t i = 0; i < 50; i++) {
            CHECK(video_audio_seek(h, 7301 + i));
            wait_pcm(h);
            CHECK(video_audio_play(h));
            CHECK(video_audio_render(h, actual, 128, &ticket));
            CHECK(video_audio_consume(h, ticket.generation, ticket.first_sequence, 128));
            CHECK(video_audio_poll(h).position_frames == 7429 + i);
            CHECK(video_audio_pause(h));
        }
        CHECK(video_audio_seek(h, 0));
        CHECK(video_audio_play(h));
        for (size_t calls = 0; !video_audio_poll(h).ended; calls++) {
            CHECK(calls < 10000);
            wait_pcm(h);
            video_audio_snapshot snap = video_audio_poll(h);
            size_t frames = snap.pcm_frames < 128 ? snap.pcm_frames : 128;
            if (!frames)
                continue;
            CHECK(video_audio_render(h, actual, frames, &ticket));
            CHECK(video_audio_consume(h, ticket.generation, ticket.first_sequence, frames));
        }
        CHECK(video_audio_poll(h).position_frames == 24000);
        CHECK(video_audio_destroy(h));
    }
    video_ffmpeg_destroy(s);
    printf("%s: %zu assertions\n", mode, assertions);
    return 0;
}
