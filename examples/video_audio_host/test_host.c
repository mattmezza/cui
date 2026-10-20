/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
typedef struct {
    mtx_t mutex;
    cnd_t changed;
    uint64_t cursor, length, cancelled;
    unsigned calls;
    bool block, entered, block_seek, entered_seek, bad_sample, bad_position, fail_seek;
    bool read_error, empty, excessive, late_data;
} decoder;
typedef struct {
    unsigned starts, stops;
    bool start_fails, stop_fails, discard;
    atomic_bool hold, entered;
    thrd_t thread;
    bool threaded;
    video_audio_host *host;
    video_audio_ticket ticket;
} output;
static void nap(void) {
    struct timespec t = {0, 1000000};
    thrd_sleep(&t, NULL);
}
static float sample(uint64_t frame) {
    return (float)(frame % 31 + 1) / 32;
}
static bool seek(void *user, uint64_t generation, uint64_t frame) {
    decoder *d = user;
    mtx_lock(&d->mutex);
    d->entered_seek = true;
    cnd_broadcast(&d->changed);
    while (d->block_seek && generation > d->cancelled)
        cnd_wait(&d->changed, &d->mutex);
    d->cursor = frame;
    bool ok = !d->fail_seek && generation > d->cancelled;
    mtx_unlock(&d->mutex);
    return ok;
}
static video_audio_read read_pcm(void *user, uint64_t generation, float *pcm, size_t capacity,
                                 uint64_t *first, size_t *frames) {
    decoder *d = user;
    mtx_lock(&d->mutex);
    d->calls++;
    d->entered = true;
    cnd_broadcast(&d->changed);
    while (d->block && generation > d->cancelled)
        cnd_wait(&d->changed, &d->mutex);
    if (generation <= d->cancelled && !d->late_data) {
        mtx_unlock(&d->mutex);
        return VIDEO_AUDIO_AGAIN;
    }
    if (d->read_error) {
        mtx_unlock(&d->mutex);
        return VIDEO_AUDIO_ERROR;
    }
    if (d->empty || d->excessive) {
        *frames = d->empty ? 0 : capacity + 1;
        *first = d->cursor;
        mtx_unlock(&d->mutex);
        return VIDEO_AUDIO_DATA;
    }
    if (d->cursor >= d->length) {
        mtx_unlock(&d->mutex);
        return VIDEO_AUDIO_EOF;
    }
    *first = d->cursor + (d->bad_position ? 1 : 0);
    *frames = d->length - d->cursor < capacity ? (size_t)(d->length - d->cursor) : capacity;
    for (size_t i = 0; i < *frames; i++) {
        pcm[i * 2] = sample(d->cursor + i);
        pcm[i * 2 + 1] = -sample(d->cursor + i);
    }
    if (d->bad_sample)
        pcm[0] = NAN;
    d->cursor += *frames;
    mtx_unlock(&d->mutex);
    return VIDEO_AUDIO_DATA;
}
static void interrupt(void *user, uint64_t through) {
    decoder *d = user;
    mtx_lock(&d->mutex);
    if (through > d->cancelled)
        d->cancelled = through;
    cnd_broadcast(&d->changed);
    mtx_unlock(&d->mutex);
}
static int held_callback(void *user) {
    output *o = user;
    atomic_store(&o->entered, true);
    while (atomic_load(&o->hold))
        thrd_yield();
    assert(video_audio_consume(o->host, o->ticket.generation, o->ticket.first_sequence,
                               o->ticket.frames));
    return 0;
}
static bool start(void *user, video_audio_host *host) {
    output *o = user;
    o->starts++;
    o->host = host;
    return !o->start_fails;
}
static bool stop(void *user, bool discard) {
    output *o = user;
    o->stops++;
    if (o->stop_fails)
        return false;
    if (o->threaded) {
        atomic_store(&o->hold, false);
        thrd_join(o->thread, NULL);
        o->threaded = false;
    }
    o->discard = discard;
    if (discard)
        o->host = NULL;
    return true;
}
static void init(decoder *d, output *o, uint64_t length) {
    *d = (decoder){.length = length};
    *o = (output){0};
    assert(mtx_init(&d->mutex, mtx_plain) == thrd_success);
    assert(cnd_init(&d->changed) == thrd_success);
    atomic_init(&o->hold, false);
    atomic_init(&o->entered, false);
}
static void fini(decoder *d) {
    cnd_destroy(&d->changed);
    mtx_destroy(&d->mutex);
}
static video_audio_config config(decoder *d, output *o) {
    return (video_audio_config){.capacity_frames = 8,
                                .decode_chunk_frames = 4,
                                .decoder = {seek, read_pcm, interrupt, d},
                                .output = {start, stop, o}};
}
static void wait_pcm(video_audio_host *h, size_t frames) {
    for (unsigned i = 0; i < 3000; i++) {
        video_audio_snapshot s = video_audio_poll(h);
        assert(!s.failed);
        if (s.pcm_frames == frames)
            return;
        nap();
    }
    assert(!"PCM wait timed out");
}
static void wait_eof(video_audio_host *h) {
    for (unsigned i = 0; i < 3000; i++) {
        if (video_audio_poll(h).decoder_eof)
            return;
        nap();
    }
    assert(!"EOF wait timed out");
}
static void wait_failed(video_audio_host *h) {
    for (unsigned i = 0; i < 3000; i++) {
        if (video_audio_poll(h).failed)
            return;
        nap();
    }
    assert(!"failure wait timed out");
}
static void pcm_equal(const float *pcm, uint64_t first, size_t frames) {
    for (size_t i = 0; i < frames; i++) {
        assert(pcm[i * 2] == sample(first + i));
        assert(pcm[i * 2 + 1] == -sample(first + i));
    }
}
static void accounting(void) {
    decoder d;
    output o;
    init(&d, &o, 12);
    video_audio_host *h = video_audio_create(config(&d, &o));
    assert(h);
    wait_pcm(h, 8);
    mtx_lock(&d.mutex);
    assert(d.calls == 2); /* worker cannot decode ahead beyond the hard PCM cap */
    mtx_unlock(&d.mutex);
    float pcm[16];
    for (size_t i = 0; i < 16; i++)
        pcm[i] = 99;
    video_audio_ticket a = {0}, b;
    assert(!video_audio_render(h, pcm, 4, &a) && pcm[0] == 99);
    assert(video_audio_play(h));
    assert(video_audio_render(h, pcm, 8, &a));
    pcm_equal(pcm, 0, 8);
    video_audio_snapshot s = video_audio_poll(h);
    assert(s.position_frames == 0 && s.rendered_media == 8 && s.output_frames == 8);
    assert(!video_audio_render(h, pcm, 1, &b)); /* output staging is independently bounded */
    assert(!video_audio_consume(h, a.generation, a.first_sequence + 1, 2));
    assert(!video_audio_consume(h, a.generation, a.first_sequence, 9));
    assert(video_audio_consume(h, a.generation, a.first_sequence, 3));
    assert(video_audio_poll(h).position_frames == 3);
    assert(!video_audio_consume(h, a.generation, a.first_sequence, 3));
    assert(video_audio_pause(h) && !video_audio_poll(h).playing && !o.discard);
    assert(!video_audio_consume(h, a.generation, 3, 5));
    assert(video_audio_play(h));
    assert(video_audio_consume(h, a.generation, 3, 5));
    wait_pcm(h, 4);
    wait_eof(h);
    assert(!video_audio_poll(h).ended);
    assert(video_audio_render(h, pcm, 8, &b));
    pcm_equal(pcm, 8, 4);
    for (size_t i = 8; i < 16; i++)
        assert(pcm[i] == 0);
    assert(video_audio_poll(h).position_frames == 8); /* submission is not time */
    assert(video_audio_consume(h, b.generation, b.first_sequence, 4));
    s = video_audio_poll(h);
    assert(s.position_frames == 12 && s.ended && s.consumed_media == 12 && s.silence_frames == 4);
    assert(video_audio_consume(h, b.generation, b.first_sequence + 4, 4));
    s = video_audio_poll(h);
    assert(s.position_frames == 12 && s.consumed_silence == 4);
    assert(s.submitted_frames == 16 && s.consumed_frames == 16); /* silence is not media time */
    assert(video_audio_destroy(h));
    fini(&d);
}
static void backpressure(void) {
    decoder d;
    output o;
    init(&d, &o, 32);
    video_audio_host *h = video_audio_create(config(&d, &o));
    assert(h);
    wait_pcm(h, 8);
    assert(video_audio_play(h));
    float pcm[16];
    video_audio_ticket ticket;
    assert(video_audio_render(h, pcm, 8, &ticket));
    wait_pcm(h, 8);
    video_audio_snapshot p = video_audio_poll(h);
    assert(p.pcm_frames == 8 && p.output_frames == 8 && p.output_media_frames == 8);
    assert(p.position_frames == 0 && p.consumed_frames == 0);
    mtx_lock(&d.mutex);
    assert(d.calls == 4 && d.cursor == 16);
    mtx_unlock(&d.mutex);
    pcm[0] = 99;
    assert(!video_audio_render(h, pcm, 1, &ticket) && pcm[0] == 99);
    assert(video_audio_destroy(h)); /* shutdown unblocks a full-ring producer */
    assert(!o.host && o.discard);
    fini(&d);
}
static void generations(void) {
    decoder d;
    output o;
    init(&d, &o, 64);
    video_audio_host *h = video_audio_create(config(&d, &o));
    assert(h);
    wait_pcm(h, 8);
    assert(video_audio_play(h));
    float pcm[16];
    video_audio_ticket old, next;
    assert(video_audio_render(h, pcm, 4, &old));
    assert(video_audio_seek(h, 17));
    assert(o.discard && !video_audio_poll(h).playing);
    wait_pcm(h, 8);
    assert(video_audio_poll(h).position_frames == 17);
    assert(video_audio_play(h));
    assert(!video_audio_consume(h, old.generation, old.first_sequence, 4));
    assert(video_audio_render(h, pcm, 6, &next));
    pcm_equal(pcm, 17, 6);
    assert(next.generation == old.generation + 1 && next.first_sequence == 0);
    assert(video_audio_consume(h, next.generation, 0, 6));
    assert(video_audio_poll(h).position_frames == 23);
    /* Ring wrap and seeks cannot publish an obsolete producer result. */
    for (unsigned i = 0; i < 20; i++) {
        assert(video_audio_seek(h, i));
        wait_pcm(h, 8);
        assert(video_audio_play(h));
        assert(video_audio_render(h, pcm, 7, &next));
        pcm_equal(pcm, i, 7);
        assert(video_audio_consume(h, next.generation, 0, 7));
        assert(video_audio_poll(h).position_frames == i + 7);
    }
    assert(video_audio_destroy(h)); /* wakes worker if PCM capacity wait is full */
    fini(&d);
}
static void failures(void) {
    decoder d;
    output o;
    init(&d, &o, 20);
    video_audio_config c = config(&d, &o);
    c.capacity_frames = 0;
    assert(!video_audio_create(c));
    c = config(&d, &o);
    c.decode_chunk_frames = 9;
    assert(!video_audio_create(c));
    d.bad_sample = true;
    video_audio_host *h = video_audio_create(config(&d, &o));
    assert(h);
    wait_failed(h);
    assert(!video_audio_play(h));
    mtx_lock(&d.mutex);
    d.bad_sample = false;
    d.bad_position = true;
    mtx_unlock(&d.mutex);
    assert(video_audio_seek(h, 0));
    wait_failed(h);
    mtx_lock(&d.mutex);
    d.bad_position = false;
    d.fail_seek = true;
    mtx_unlock(&d.mutex);
    assert(video_audio_seek(h, 0));
    wait_failed(h);
    mtx_lock(&d.mutex);
    d.fail_seek = false;
    mtx_unlock(&d.mutex);
    assert(video_audio_seek(h, 0));
    wait_pcm(h, 8);
    for (unsigned mode = 0; mode < 3; mode++) {
        mtx_lock(&d.mutex);
        d.read_error = mode == 0;
        d.empty = mode == 1;
        d.excessive = mode == 2;
        mtx_unlock(&d.mutex);
        assert(video_audio_seek(h, 0));
        wait_failed(h);
    }
    mtx_lock(&d.mutex);
    d.read_error = d.empty = d.excessive = false;
    mtx_unlock(&d.mutex);
    assert(video_audio_seek(h, 0));
    wait_pcm(h, 8);
    o.start_fails = true;
    assert(!video_audio_play(h) && !video_audio_poll(h).playing);
    o.start_fails = false;
    assert(video_audio_play(h));
    o.stop_fails = true;
    video_audio_snapshot before = video_audio_poll(h);
    assert(!video_audio_pause(h) && !video_audio_seek(h, 2) && !video_audio_destroy(h));
    assert(video_audio_poll(h).generation == before.generation && video_audio_poll(h).playing);
    o.stop_fails = false;
    assert(video_audio_destroy(h));
    fini(&d);
}
static void cancellation(void) {
    decoder d;
    output o;
    init(&d, &o, 64);
    d.block = true;
    d.late_data = true; /* cancelled read still returns DATA; host must discard it */
    video_audio_host *h = video_audio_create(config(&d, &o));
    assert(h && video_audio_play(h));
    mtx_lock(&d.mutex);
    while (!d.entered)
        cnd_wait(&d.changed, &d.mutex);
    mtx_unlock(&d.mutex);
    float pcm[8];
    video_audio_ticket silence;
    assert(video_audio_render(h, pcm, 4, &silence));
    assert(video_audio_consume(h, silence.generation, 0, 4));
    assert(video_audio_poll(h).position_frames == 0 && video_audio_poll(h).buffering);
    for (size_t i = 0; i < 8; i++)
        assert(pcm[i] == 0);
    assert(video_audio_seek(h, 9)); /* cancels blocked generation 1, not generation 2 */
    mtx_lock(&d.mutex);
    d.block = false;
    cnd_broadcast(&d.changed);
    mtx_unlock(&d.mutex);
    wait_pcm(h, 8);
    assert(video_audio_play(h));
    assert(video_audio_render(h, pcm, 4, &o.ticket));
    pcm_equal(pcm, 9, 4);
    atomic_store(&o.hold, true);
    assert(thrd_create(&o.thread, held_callback, &o) == thrd_success);
    o.threaded = true;
    while (!atomic_load(&o.entered))
        thrd_yield();
    assert(video_audio_destroy(h)); /* stop joins callback before storage is freed */
    assert(!o.threaded && !o.host);
    fini(&d);
    init(&d, &o, 64);
    d.block = true;
    h = video_audio_create(config(&d, &o));
    assert(h);
    mtx_lock(&d.mutex);
    while (!d.entered)
        cnd_wait(&d.changed, &d.mutex);
    mtx_unlock(&d.mutex);
    assert(video_audio_destroy(h)); /* interrupt wakes blocked decoder before join */
    fini(&d);
    init(&d, &o, 64);
    d.block_seek = true;
    h = video_audio_create(config(&d, &o));
    assert(h);
    mtx_lock(&d.mutex);
    while (!d.entered_seek)
        cnd_wait(&d.changed, &d.mutex);
    mtx_unlock(&d.mutex);
    assert(video_audio_destroy(h)); /* interrupt also wakes initial blocked seek */
    fini(&d);
}
typedef struct {
    video_audio_host *host;
    atomic_bool done;
    uint64_t media;
} stream;
static int stream_output(void *user) {
    stream *s = user;
    float pcm[6];
    while (s->media < 257) {
        video_audio_ticket t;
        if (!video_audio_render(s->host, pcm, 3, &t)) {
            thrd_yield();
            continue;
        }
        for (unsigned i = 0; i < 3; i++) {
            if (pcm[i * 2] != 0) {
                assert(pcm[i * 2] == sample(s->media));
                assert(pcm[i * 2 + 1] == -sample(s->media));
                s->media++;
            } else {
                assert(pcm[i * 2 + 1] == 0);
            }
        }
        assert(video_audio_consume(s->host, t.generation, t.first_sequence, 1));
        assert(video_audio_consume(s->host, t.generation, t.first_sequence + 1, 2));
    }
    atomic_store(&s->done, true);
    return 0;
}
static int wrong_owner(void *user) {
    video_audio_host *h = user;
    assert(!video_audio_play(h) && !video_audio_pause(h));
    assert(!video_audio_seek(h, 9) && !video_audio_destroy(h));
    assert(video_audio_poll(h).generation == 1);
    return 0;
}
static void concurrent_stream(void) {
    decoder d;
    output o;
    init(&d, &o, 257);
    video_audio_host *h = video_audio_create(config(&d, &o));
    assert(h);
    thrd_t thread;
    assert(thrd_create(&thread, wrong_owner, h) == thrd_success);
    thrd_join(thread, NULL);
    assert(video_audio_play(h));
    stream s = {.host = h};
    atomic_init(&s.done, false);
    assert(thrd_create(&thread, stream_output, &s) == thrd_success);
    for (unsigned i = 0; i < 3000 && !atomic_load(&s.done); i++) {
        video_audio_snapshot p = video_audio_poll(h);
        assert(!p.failed && p.pcm_frames <= 8 && p.output_frames <= 8);
        assert(p.position_frames <= 257 && p.output_media_frames <= p.output_frames);
        nap();
    }
    assert(atomic_load(&s.done));
    thrd_join(thread, NULL);
    wait_eof(h);
    video_audio_snapshot p = video_audio_poll(h);
    assert(s.media == 257 && p.position_frames == 257 && p.consumed_media == 257 && p.ended);
    assert(video_audio_destroy(h));
    fini(&d);
}
int main(int argc, char **argv) {
    const struct {
        const char *name;
        void (*run)(void);
    } cases[] = {{"accounting", accounting},     {"backpressure", backpressure},
                 {"generations", generations},   {"failures", failures},
                 {"cancellation", cancellation}, {"concurrent", concurrent_stream}};
    bool ran = false;
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (argc == 1 || (argc == 2 && !strcmp(argv[1], cases[i].name))) {
            cases[i].run();
            printf("%s PASS\n", cases[i].name);
            ran = true;
        }
    }
    return ran ? 0 : 2;
}
