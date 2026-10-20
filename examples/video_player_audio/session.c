/* SPDX-License-Identifier: MIT */
#include "session.h"
#include "../video_audio_ffmpeg/decoder.h"
#include "../video_audio_sdl/output.h"
#include <math.h>
#include <stdlib.h>
#include <threads.h>
struct video_sound_session {
    thrd_t owner;
    video_decoder *video;
    video_audio_ffmpeg *decoder;
    video_audio_host *audio;
    video_sdl_output *output;
    video_sound_hooks hooks;
    video_sound_status status;
    video_lease held;
    uint64_t audio_generation;
    bool have, discard_frame, intent, closing;
};
static bool owner(video_sound_session *s) {
    return thrd_equal(s->owner, thrd_current());
}
static bool fail(video_sound_session *s, const char *message) {
    s->status.failed = true;
    s->status.error = message;
    s->intent = false;
    if (s->audio)
        video_audio_pause(s->audio);
    s->status.playing = false;
    return false;
}
static bool release(video_sound_session *s) {
    if (s->have && !video_decoder_release(s->video, &s->held))
        return false;
    s->have = false;
    return true;
}
video_sound_session *video_sound_open(const char *path, video_sound_hooks hooks) {
    video_sound_session *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->owner = thrd_current();
    s->hooks = hooks;
    s->status.generation = 1;
    s->audio_generation = 1;
    s->decoder = video_ffmpeg_create(path, (video_ffmpeg_limits){0});
    if (!s->decoder) {
        free(s);
        return NULL;
    }
    s->video = video_decoder_open_common(path, 1, 1);
    s->output = video_sdl_open(12000, 1024);
    if (!s->video || !s->output) {
        video_decoder_close(s->video);
        video_ffmpeg_destroy(s->decoder);
        if (s->output)
            video_sdl_destroy(s->output);
        free(s);
        return NULL;
    }
    s->audio = video_audio_create((video_audio_config){
        12000, 1024, video_ffmpeg_interface(s->decoder), video_sdl_interface(s->output)});
    if (!s->audio) {
        video_decoder_close(s->video);
        video_ffmpeg_destroy(s->decoder);
        video_sdl_destroy(s->output);
        free(s);
        return NULL;
    }
    return s;
}
bool video_sound_play(video_sound_session *s, bool playing) {
    if (!owner(s) || s->closing || s->status.failed)
        return false;
    if (playing && s->status.ended && !video_sound_seek(s, 0))
        return false;
    s->intent = playing;
    if (!playing) {
        if (!video_audio_pause(s->audio))
            return fail(s, "Audio pause did not quiesce");
        s->status.position_frames = video_audio_poll(s->audio).position_frames;
        s->status.playing = false;
    }
    return true;
}
bool video_sound_seek(video_sound_session *s, double seconds) {
    if (!owner(s) || s->closing || s->status.failed || !s->status.ready || !isfinite(seconds))
        return false;
    if (seconds < 0)
        seconds = 0;
    if (seconds > s->status.duration)
        seconds = s->status.duration;
    uint64_t target = (uint64_t)llround(seconds * 48000);
    if (s->status.generation == UINT64_MAX)
        return fail(s, "Session generation exhausted");
    uint64_t next = s->status.generation + 1;
    if (!video_audio_seek(s->audio, target))
        return fail(s, "Audio seek could not discard output");
    bool ok = s->hooks.video_seek
                  ? s->hooks.video_seek(s->hooks.user, s->video, next, (double)target / 48000)
                  : video_decoder_seek(s->video, next, (double)target / 48000);
    if (!ok)
        return fail(s, "Video rejected seek after audio accepted; reopen required");
    s->audio_generation = video_audio_poll(s->audio).generation;
    s->status.generation = next;
    s->status.position_frames = target;
    s->status.seeking = true;
    s->status.playing = false;
    s->status.ended = false;
    s->discard_frame = true;
    return true;
}
bool video_sound_poll(video_sound_session *s) {
    if (!owner(s) || s->closing || s->status.failed)
        return false;
    if (s->discard_frame) {
        if (!release(s))
            return fail(s, "Video lease release failed");
        s->discard_frame = false;
    }
    video_decoder_status v = video_decoder_poll(s->video);
    video_audio_snapshot a = video_audio_poll(s->audio);
    if (v.failed || a.failed)
        return fail(s, "Unsupported media or decoder failure");
    video_ffmpeg_snapshot d = video_ffmpeg_poll(s->decoder);
    bool audio_ready = a.generation == s->audio_generation && d.generation == a.generation &&
                       (a.pcm_frames >= 1024 || a.decoder_eof);
    if (v.ready && audio_ready && !s->status.ready) {
        if (!v.common_clock || v.common_origin_us != d.common_origin_us ||
            v.common_origin_us != 0 || !v.duration_known || v.duration <= 0 || v.duration > 10)
            return fail(s, "Requires known zero common origin and <=10s duration");
        s->status.ready = true;
        s->status.duration = v.duration;
    }
    if (!s->status.ready)
        return true;
    if (s->status.seeking && (!audio_ready || v.clock_generation != s->status.generation))
        return true;
    double due = (double)a.position_frames / 48000;
    video_lease next = {0};
    bool acquire_allowed = !s->have || a.playing || s->status.seeking || a.ended;
    bool acquired = acquire_allowed && video_decoder_acquire(s->video, s->status.generation,
                                                             a.ended ? 10.0 : due, &next);
    if (acquired) {
        if (a.ended && next.frame.frame.timestamp > due) {
            if (!video_decoder_release(s->video, &next))
                return fail(s, "Video tail lease release failed");
        } else {
            if (!release(s))
                return fail(s, "Video lease replacement failed");
            s->held = next;
            s->have = true;
        }
        s->status.seeking = false;
    }
    v = video_decoder_poll(s->video);
    if (s->status.seeking && v.eof && !v.queued)
        s->status.seeking = false;
    if (s->intent && !s->status.seeking && !a.playing && !a.ended) {
        if (!video_audio_play(s->audio))
            return fail(s, "Audio output start failed; reopen required");
    }
    if (!video_sdl_pump(s->output))
        return fail(s, "Audio output invalidated/failed; reopen required");
    a = video_audio_poll(s->audio);
    if (a.generation != s->audio_generation)
        return fail(s, "Audio generation mismatch");
    s->status.position_frames = a.position_frames;
    s->status.playing = a.playing;
    if (a.ended && v.eof && !v.queued) {
        uint64_t end = v.end_frame, at = a.position_frames;
        if (!end && at == (uint64_t)llround(s->status.duration * 48000))
            end = at;
        if (end > at + 4800 || at > end + 4800)
            return fail(s, "Unsupported unequal audio/video endpoints");
        if (!video_audio_pause(s->audio))
            return fail(s, "Audio EOF pause failed");
        s->status.ended = true;
        s->status.playing = false;
        s->intent = false;
    }
    return true;
}
video_sound_status video_sound_snapshot(video_sound_session *s) {
    return s->status;
}
const cui_video_player_frame *video_sound_frame(video_sound_session *s) {
    if (!s->have || s->discard_frame ||
        (s->status.playing &&
         s->held.frame.frame.timestamp > (double)s->status.position_frames / 48000))
        return NULL;
    return &s->held.frame;
}
bool video_sound_close(video_sound_session *s) {
    if (!s)
        return true;
    if (!owner(s))
        return false;
    s->closing = true;
    s->intent = false;
    if (s->audio) {
        if (s->hooks.audio_destroy && !video_audio_pause(s->audio))
            return false;
        bool ok = s->hooks.audio_destroy ? s->hooks.audio_destroy(s->hooks.user, s->audio)
                                         : video_audio_destroy(s->audio);
        if (!ok)
            return false;
        s->audio = NULL;
    }
    if (s->decoder) {
        video_ffmpeg_destroy(s->decoder);
        s->decoder = NULL;
    }
    if (s->output) {
        if (!video_sdl_destroy(s->output))
            return false;
        s->output = NULL;
    }
    if (!release(s) || !video_decoder_close(s->video))
        return false;
    free(s);
    return true;
}
