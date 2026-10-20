/* SPDX-License-Identifier: MIT */
#include "../video_audio_sdl/output.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <threads.h>
static atomic_bool posting;
static int post_events(void *u) {
    (void)u;
    while (atomic_load(&posting)) {
        SDL_Event e = {0};
        e.type = SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED;
        e.adevice.which = 1;
        e.adevice.recording = false;
        SDL_PushEvent(&e);
        thrd_yield();
    }
    return 0;
}
int main(void) {
    assert(SDL_setenv_unsafe("SDL_AUDIODRIVER", "dummy", 1) == 0);
    assert(SDL_Init(SDL_INIT_AUDIO));
    for (int type = 0; type < 3; type++) {
        video_sdl_output *o = video_sdl_open(256, 64);
        assert(o);
        SDL_Event e = {0};
        e.type = type == 0   ? SDL_EVENT_AUDIO_DEVICE_ADDED
                 : type == 1 ? SDL_EVENT_AUDIO_DEVICE_REMOVED
                             : SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED;
        e.adevice.recording = false;
        assert(SDL_PushEvent(&e));
        /* Backend format is checked even before any controller/render attachment.
         * No controller pointer is needed by event-watch callbacks. */
        video_audio_output api = video_sdl_interface(o);
        assert(!api.start(api.user, NULL));
        assert(video_sdl_poll(o).faulted);
        assert(video_sdl_poll(o).drained_frames == 0);
        assert(api.stop(api.user, true));
        assert(!api.start(api.user, NULL));
        assert(api.stop(api.user, true));
        assert(video_sdl_destroy(o));
    }
    video_sdl_output *o = video_sdl_open(256, 64);
    assert(o);
    SDL_Event capture = {0};
    capture.type = SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED;
    capture.adevice.recording = true;
    assert(SDL_PushEvent(&capture));
    video_audio_output api = video_sdl_interface(o);
    assert(api.start(api.user, NULL));
    assert(api.stop(api.user, true));
    assert(video_sdl_destroy(o));
    atomic_init(&posting, true);
    thrd_t thread;
    assert(thrd_create(&thread, post_events, NULL) == thrd_success);
    for (int i = 0; i < 100; i++) {
        o = video_sdl_open(256, 64);
        if (o)
            assert(video_sdl_destroy(o));
        SDL_FlushEvents(SDL_EVENT_AUDIO_DEVICE_ADDED, SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED);
    }
    atomic_store(&posting, false);
    thrd_join(thread, NULL);
    SDL_Quit();
    puts("PASS actual SDL sticky output-device watches, capture ignored, concurrent "
         "dispatch/remove lifetime (100 opens)");
    return 0;
}
