/* SPDX-License-Identifier: MIT */
#ifndef VIDEO_SOUND_SESSION_H
#define VIDEO_SOUND_SESSION_H
#include "../video_audio_host/host.h"
#include "../video_player/decoder.h"
typedef struct video_sound_session video_sound_session;
typedef struct {
    uint64_t generation, position_frames;
    double duration;
    bool ready, seeking, playing, ended, failed;
    const char *error; /* static text */
} video_sound_status;
/* Example-private service seam. NULL functions use actual production services.
 * Hooks/users live through successful close; no callbacks from worker/SDL. */
typedef struct {
    bool (*video_seek)(void *, video_decoder *, uint64_t, double);
    bool (*audio_destroy)(void *, video_audio_host *);
    void *user;
} video_sound_hooks;
/* SDL audio initialized by caller; normal owner polling must first dispatch
 * initial device enumeration before guarded output creation. No event pump here. All session
 * functions are owner-thread only. Opens actual local file workers and default SDL output;
 * automation MUST force dummy before SDL init. Audio profile: zero container origin, <=10s
 * FFV1+PCM. */
video_sound_session *video_sound_open(const char *path, video_sound_hooks);
bool video_sound_poll(video_sound_session *);
video_sound_status video_sound_snapshot(video_sound_session *);
/* Requested intent can be accepted while loading; confirmed playing remains
 * false until both workers/preview are ready and output actually starts. */
bool video_sound_play(video_sound_session *, bool);
bool video_sound_seek(video_sound_session *, double seconds);
/* Borrowed until next poll/successful close. Seek does NOT free a frame borrowed
 * earlier in the same UI composition; renderer must finish before next poll.
 * Paused future preview is visible; playing hides it until mixer clock is due. */
const cui_video_player_frame *video_sound_frame(video_sound_session *);
/* Caller finishes renderer use first. Failure retains live resources/session
 * for retry; never free userdata/Core/window until successful close. */
bool video_sound_close(video_sound_session *);
#endif
