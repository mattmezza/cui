/* SPDX-License-Identifier: MIT */
#include "session.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
static size_t assertions;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        assert(x);                                                                                 \
        assertions++;                                                                              \
    } while (0)
static void delay(void) {
    struct timespec t = {0, 1000000};
    thrd_sleep(&t, NULL);
}
static video_sound_status tick(video_sound_session *s) {
    CHECK(video_sound_poll(s));
    return video_sound_snapshot(s);
}
static void ready(video_sound_session *s) {
    for (int i = 0; i < 10000; i++) {
        video_sound_status a = tick(s);
        if (a.ready && !a.seeking)
            return;
        delay();
    }
    CHECK(false);
}
static bool reject(void *u, video_decoder *v, uint64_t g, double t) {
    (void)v;
    (void)g;
    (void)t;
    (*(int *)u)++;
    return false;
}
static bool close_once(void *u, video_audio_host *a) {
    int *count = u;
    if (!(*count)++)
        return false;
    return video_audio_destroy(a);
}
static void complete(video_sound_session *s) {
    CHECK(video_sound_play(s, true));
    bool red = false, blue = false;
    for (int i = 0; i < 10000; i++) {
        video_sound_status a = tick(s);
        const cui_video_player_frame *f = video_sound_frame(s);
        if (f) {
            CHECK(f->frame.generation == a.generation);
            CHECK(f->frame.timestamp <= a.position_frames / 48000.0);
            const unsigned char *p = f->frame.bitmap.pixels;
            if (f->frame.timestamp < 1) {
                CHECK(p[0] > 240 && p[2] < 10);
                red = true;
            } else {
                CHECK(p[2] > 240 && p[0] < 10);
                blue = true;
            }
        }
        if (a.ended) {
            CHECK(a.position_frames == 96000);
            CHECK(red && blue);
            return;
        }
        delay();
    }
    CHECK(false);
}
int main(int argc, char **argv) {
    CHECK(argc == 3);
    CHECK(SDL_setenv_unsafe("SDL_AUDIODRIVER", "dummy", 1) == 0);
    CHECK(SDL_Init(SDL_INIT_AUDIO));
    const char *mode = argv[1];
    char path[4096];
    CHECK(snprintf(path, sizeof(path), "%s/%s", argv[2],
                   !strcmp(mode, "endpoints") ? "audio-short.mkv" : "fixture.mkv") > 0);
    int hook_count = 0;
    video_sound_hooks hooks = {0};
    if (!strcmp(mode, "failures"))
        hooks = (video_sound_hooks){reject, close_once, &hook_count};
    video_sound_session *s = video_sound_open(path, hooks);
    CHECK(s);
    if (!strcmp(mode, "missing")) {
        CHECK(video_sound_close(s));
        CHECK(snprintf(path, sizeof(path), "%s/missing.mkv", argv[2]) > 0);
        s = video_sound_open(path, (video_sound_hooks){0});
        CHECK(s);
        bool failed = false;
        for (int i = 0; i < 10000; i++) {
            if (!video_sound_poll(s)) {
                failed = true;
                break;
            }
            delay();
        }
        CHECK(failed);
        CHECK(video_sound_snapshot(s).failed);
    } else if (!strcmp(mode, "combined")) {
        ready(s);
        complete(s);
        CHECK(video_sound_play(s, true));
        CHECK(video_sound_snapshot(s).generation == 2);
        CHECK(video_sound_snapshot(s).position_frames == 0);
        ready(s);
        tick(s);
        CHECK(video_sound_snapshot(s).playing);
        CHECK(video_sound_close(s));
        CHECK(snprintf(path, sizeof(path), "%s/resampled.mkv", argv[2]) > 0);
        s = video_sound_open(path, (video_sound_hooks){0});
        CHECK(s);
        ready(s);
        complete(s);
    } else if (!strcmp(mode, "preview")) {
        ready(s);
        CHECK(video_sound_play(s, true));
        for (int i = 0; i < 10000 && video_sound_snapshot(s).position_frames < 12000; i++) {
            tick(s);
            delay();
        }
        CHECK(video_sound_play(s, false));
        uint64_t at = video_sound_snapshot(s).position_frames;
        const cui_video_player_frame *f = video_sound_frame(s);
        CHECK(f);
        const unsigned char *pixels = f->frame.bitmap.pixels;
        unsigned char first = pixels[0];
        uint64_t revision = f->frame.bitmap.revision;
        for (int i = 0; i < 40; i++) {
            CHECK(video_sound_frame(s)->frame.bitmap.revision == revision);
            CHECK(tick(s).position_frames == at);
            delay();
        }
        CHECK(video_sound_seek(s, 1.25));
        CHECK(pixels[0] == first);
        CHECK(video_sound_snapshot(s).position_frames == 60000);
        ready(s);
        f = video_sound_frame(s);
        CHECK(f);
        CHECK(f->frame.timestamp >= 1.25);
        CHECK(tick(s).position_frames == 60000);
        CHECK(video_sound_play(s, true));
        tick(s);
        f = video_sound_frame(s);
        if (f)
            CHECK(f->frame.timestamp <= video_sound_snapshot(s).position_frames / 48000.0);
        CHECK(video_sound_seek(s, 2));
        ready(s);
        for (int i = 0; i < 1000 && !video_sound_snapshot(s).ended; i++) {
            tick(s);
            delay();
        }
        CHECK(video_sound_snapshot(s).ended);
    } else if (!strcmp(mode, "origin")) {
        CHECK(video_sound_close(s));
        CHECK(snprintf(path, sizeof(path), "%s/video-offset.mkv", argv[2]) > 0);
        video_decoder *v = video_decoder_open_common(path, 7, 1);
        CHECK(v);
        video_lease lease = {0};
        bool acquired = false;
        for (int i = 0; i < 10000; i++) {
            CHECK(!video_decoder_poll(v).failed);
            if (video_decoder_acquire(v, 1, 0.2, &lease)) {
                acquired = true;
                break;
            }
            delay();
        }
        CHECK(acquired);
        CHECK(video_decoder_poll(v).common_origin_us == 0);
        CHECK(lease.frame.frame.timestamp == 0.125);
        CHECK(lease.frame.frame.timestamp <= 0.2);
        CHECK(video_decoder_release(v, &lease));
        CHECK(video_decoder_close(v));
        s = video_sound_open(path, (video_sound_hooks){0});
        CHECK(s);
        ready(s);
        CHECK(video_sound_seek(s, 1.25));
        ready(s);
        CHECK(video_sound_snapshot(s).position_frames == 60000);
    } else if (!strcmp(mode, "invalidation")) {
        ready(s);
        CHECK(video_sound_play(s, true));
        tick(s);
        uint64_t position = video_sound_snapshot(s).position_frames;
        SDL_Event event = {0};
        event.type = SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED;
        event.adevice.recording = false;
        CHECK(SDL_PushEvent(&event));
        CHECK(!video_sound_poll(s));
        CHECK(video_sound_snapshot(s).failed);
        CHECK(video_sound_snapshot(s).position_frames == position);
        CHECK(strstr(video_sound_snapshot(s).error, "output"));
    } else if (!strcmp(mode, "failures")) {
        ready(s);
        CHECK(!video_sound_seek(s, 1.25));
        CHECK(video_sound_snapshot(s).failed);
        CHECK(hook_count == 1);
        hook_count = 0;
        CHECK(!video_sound_close(s));
        CHECK(!video_sound_poll(s));
        CHECK(video_sound_snapshot(s).failed);
        CHECK(hook_count == 1);
    } else {
        CHECK(!strcmp(mode, "endpoints"));
        ready(s);
        CHECK(video_sound_play(s, true));
        bool failed = false;
        for (int i = 0; i < 10000; i++) {
            if (!video_sound_poll(s)) {
                failed = true;
                break;
            }
            delay();
        }
        CHECK(failed);
        CHECK(strstr(video_sound_snapshot(s).error, "endpoints"));
    }
    CHECK(video_sound_close(s));
    SDL_Quit();
    printf("%s: %zu assertions\n", mode, assertions);
    return 0;
}
