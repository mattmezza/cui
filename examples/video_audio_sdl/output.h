/* SPDX-License-Identifier: MIT */
#ifndef VIDEO_AUDIO_SDL_OUTPUT_H
#define VIDEO_AUDIO_SDL_OUTPUT_H
#include "../video_audio_host/host.h"
typedef struct video_sdl_output video_sdl_output;
/* Private example injection seam. All operations are owner-thread calls.
 * lock serializes queue/put/clear/format with mixer drain; no application
 * callbacks may run. pause must freeze further drain before successful return.
 * put copies exactly bytes on success; failure is never consumption evidence.
 * format must verify BOTH ends 48000 Hz F32 native-endian stereo, ratio 1.
 * queued returns exact input bytes or -1; clear discards, never consumes. */
typedef struct {
    bool (*lock)(void *);
    void (*unlock)(void *);
    bool (*format)(void *);
    int (*queued)(void *);
    bool (*put)(void *, const float *, size_t bytes);
    bool (*pause)(void *);
    bool (*resume)(void *);
    bool (*clear)(void *);
    void (*close)(void *);
    void *user;
} video_sdl_backend;
typedef struct {
    uint64_t submitted_frames, drained_frames;
    size_t pending_frames,
        tickets; /* successful puts not yet reconciled; not a live device query */
    bool running, faulted, attached;
} video_sdl_snapshot;
/* Ownership transfers on success only. Backend starts paused/empty. Max/chunk
 * are 1..65536 frames, chunk <= max and <= controller capacity. */
video_sdl_output *video_sdl_create(video_sdl_backend, size_t max_frames, size_t chunk_frames);
/* SDL audio subsystem must be initialized by caller. Opens default output
 * paused, callback=NULL, strict pinned SDL 3.4.18 only. Never called by tests
 * without SDL_AUDIODRIVER=dummy. No SDL types escape this header. */
video_sdl_output *video_sdl_open(size_t max_frames, size_t chunk_frames);
video_audio_output video_sdl_interface(video_sdl_output *);
/* Owner thread only, including snapshot/destroy. Poll each host iteration;
 * reconcile software mixer drain then submit at most one bounded chunk.
 * false faults transport: owner must seek/discard before restarting.
 * No DAC/hardware/audible clock or deadline guarantee. */
bool video_sdl_pump(video_sdl_output *);
video_sdl_snapshot video_sdl_poll(video_sdl_output *);
/* Only after successful controller destroy/discard detached it. false keeps
 * object/backend allocated. No host/output calls may race destruction. */
bool video_sdl_destroy(video_sdl_output *);
#endif
