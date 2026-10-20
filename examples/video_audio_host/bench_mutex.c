/* SPDX-License-Identifier: MIT */
/* Linux/Unix-only offline scheduling observation, not a realtime guarantee. */
#define _POSIX_C_SOURCE 200809L
#include "host.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>
typedef struct {
    uint64_t cursor;
    atomic_uint_fast64_t cancelled;
} source;
static bool seek(void *user, uint64_t generation, uint64_t frame) {
    source *s = user;
    s->cursor = frame;
    return generation > atomic_load(&s->cancelled);
}
static video_audio_read read_pcm(void *user, uint64_t generation, float *pcm, size_t capacity,
                                 uint64_t *first, size_t *frames) {
    source *s = user;
    if (generation <= atomic_load(&s->cancelled))
        return VIDEO_AUDIO_AGAIN;
    *first = s->cursor;
    *frames = capacity;
    for (size_t i = 0; i < capacity; i++) {
        pcm[i * 2] = .25f;
        pcm[i * 2 + 1] = -.25f;
    }
    s->cursor += capacity;
    return VIDEO_AUDIO_DATA;
}
static void interrupt(void *user, uint64_t through) {
    source *s = user;
    atomic_store(&s->cancelled, through);
}
static bool start(void *user, video_audio_host *host) {
    (void)user;
    (void)host;
    return true;
}
static bool stop(void *user, bool discard) {
    (void)user;
    (void)discard;
    return true; /* no external callbacks or retained pointers in this probe */
}
static uint64_t now_ns(void) {
    struct timespec t;
    assert(clock_gettime(CLOCK_MONOTONIC, &t) == 0);
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}
static int compare(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}
static void run(size_t chunk) {
    enum { iterations = 4000, frames = 256 };
    source s = {0};
    atomic_init(&s.cancelled, 0);
    video_audio_host *h =
        video_audio_create((video_audio_config){.capacity_frames = 12000,
                                                .decode_chunk_frames = chunk,
                                                .decoder = {seek, read_pcm, interrupt, &s},
                                                .output = {start, stop, NULL}});
    assert(h);
    uint64_t deadline = now_ns() + UINT64_C(3000000000);
    while (video_audio_poll(h).pcm_frames != 12000) {
        assert(now_ns() < deadline);
        thrd_yield();
    }
    assert(video_audio_play(h));
    uint64_t *durations = malloc(iterations * sizeof(*durations));
    assert(durations);
    float pcm[frames * 2];
    uint64_t media = 0, misses = 0;
    const uint64_t budget = UINT64_C(1000000000) * frames / 48000;
    for (unsigned i = 0; i < iterations; i++) {
        video_audio_ticket t;
        uint64_t begin = now_ns();
        assert(video_audio_render(h, pcm, frames, &t));
        assert(video_audio_consume(h, t.generation, t.first_sequence, frames));
        durations[i] = now_ns() - begin;
        misses += durations[i] > budget;
        /* Validate actual sample classes; do this outside the measured region. */
        for (size_t f = 0; f < frames; f++) {
            assert((pcm[f * 2] == .25f && pcm[f * 2 + 1] == -.25f) ||
                   (pcm[f * 2] == 0 && pcm[f * 2 + 1] == 0));
            media += pcm[f * 2] != 0;
        }
        thrd_yield(); /* outside measurement; allow contending decoder progress */
    }
    video_audio_snapshot p = video_audio_poll(h);
    assert(p.consumed_media == media && p.position_frames == media);
    assert(p.consumed_frames == iterations * frames && !p.output_frames);
    qsort(durations, iterations, sizeof(*durations), compare);
    printf("capacity=12000 chunk=%zu block=256 pairs=4000 media=%llu silence=%llu "
           "pair_ns p50=%llu p95=%llu p99=%llu max=%llu budget_ns=%llu misses=%llu\n",
           chunk, (unsigned long long)media, (unsigned long long)p.consumed_silence,
           (unsigned long long)durations[iterations / 2],
           (unsigned long long)durations[iterations * 95 / 100],
           (unsigned long long)durations[iterations * 99 / 100],
           (unsigned long long)durations[iterations - 1], (unsigned long long)budget,
           (unsigned long long)misses);
    free(durations);
    assert(video_audio_destroy(h));
}
int main(void) {
    run(256);
    run(4096);
    return 0;
}
