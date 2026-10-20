/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>
typedef struct {
    uint64_t frame;
    bool media;
} output_frame;
struct video_audio_host {
    video_audio_config config;
    thrd_t owner, worker;
    mtx_t mutex;
    cnd_t changed;
    float *pcm, *scratch;
    output_frame *ledger;
    size_t pcm_head, pcm_count, output_head, output_count, output_media;
    uint64_t generation, target, next_decode, position, submitted, acknowledged;
    uint64_t rendered_media, consumed_media, silence, consumed_silence, rejected;
    bool stop, seek_pending, playing, eof, failed;
};
static void reject(video_audio_host *h) {
    if (h->rejected != UINT64_MAX)
        h->rejected++;
}
static bool owner(video_audio_host *h) {
    return h && thrd_equal(h->owner, thrd_current());
}
static void wait_retry(video_audio_host *h) {
    struct timespec until;
    timespec_get(&until, TIME_UTC);
    until.tv_nsec += 10000000;
    if (until.tv_nsec >= 1000000000) {
        until.tv_sec++;
        until.tv_nsec -= 1000000000;
    }
    cnd_timedwait(&h->changed, &h->mutex, &until);
}
static int worker(void *user) {
    video_audio_host *h = user;
    mtx_lock(&h->mutex);
    while (!h->stop) {
        uint64_t generation = h->generation;
        if (h->seek_pending) {
            uint64_t target = h->target;
            h->seek_pending = false;
            mtx_unlock(&h->mutex);
            bool ok = h->config.decoder.seek(h->config.decoder.user, generation, target);
            mtx_lock(&h->mutex);
            if (generation == h->generation && !h->stop)
                h->failed = !ok;
            continue;
        }
        if (h->failed || h->eof || h->pcm_count == h->config.capacity_frames) {
            cnd_wait(&h->changed, &h->mutex);
            continue;
        }
        size_t capacity = h->config.capacity_frames - h->pcm_count;
        if (capacity > h->config.decode_chunk_frames)
            capacity = h->config.decode_chunk_frames;
        mtx_unlock(&h->mutex);
        uint64_t first = 0;
        size_t frames = 0;
        video_audio_read result = h->config.decoder.read(h->config.decoder.user, generation,
                                                         h->scratch, capacity, &first, &frames);
        mtx_lock(&h->mutex);
        if (h->stop || generation != h->generation)
            continue;
        if (result == VIDEO_AUDIO_AGAIN) {
            wait_retry(h);
            continue;
        }
        if (result == VIDEO_AUDIO_EOF) {
            h->eof = true;
            continue;
        }
        bool valid = result == VIDEO_AUDIO_DATA && frames && frames <= capacity &&
                     first == h->next_decode && frames <= UINT64_MAX - first;
        for (size_t i = 0; valid && i < frames * 2; i++)
            valid = isfinite(h->scratch[i]);
        if (!valid) {
            h->failed = true;
            continue;
        }
        for (size_t i = 0; i < frames; i++) {
            size_t slot = (h->pcm_head + h->pcm_count + i) % h->config.capacity_frames;
            memcpy(h->pcm + slot * 2, h->scratch + i * 2, 2 * sizeof(float));
        }
        h->pcm_count += frames;
        h->next_decode += frames;
    }
    mtx_unlock(&h->mutex);
    return 0;
}
video_audio_host *video_audio_create(video_audio_config config) {
    if (!config.capacity_frames || config.capacity_frames > 65536 || !config.decode_chunk_frames ||
        config.decode_chunk_frames > config.capacity_frames || !config.decoder.seek ||
        !config.decoder.read || !config.decoder.interrupt || !config.output.start ||
        !config.output.stop)
        return NULL;
    video_audio_host *h = calloc(1, sizeof(*h));
    if (!h)
        return NULL;
    h->config = config;
    h->owner = thrd_current();
    h->generation = 1;
    h->seek_pending = true;
    h->pcm = calloc(config.capacity_frames * 2, sizeof(float));
    h->scratch = calloc(config.decode_chunk_frames * 2, sizeof(float));
    h->ledger = calloc(config.capacity_frames, sizeof(output_frame));
    if (!h->pcm || !h->scratch || !h->ledger)
        goto memory;
    if (mtx_init(&h->mutex, mtx_plain) != thrd_success)
        goto memory;
    if (cnd_init(&h->changed) != thrd_success)
        goto mutex;
    if (thrd_create(&h->worker, worker, h) != thrd_success)
        goto condition;
    return h;
condition:
    cnd_destroy(&h->changed);
mutex:
    mtx_destroy(&h->mutex);
memory:
    free(h->ledger);
    free(h->scratch);
    free(h->pcm);
    free(h);
    return NULL;
}
bool video_audio_play(video_audio_host *h) {
    if (!owner(h))
        return false;
    mtx_lock(&h->mutex);
    if (h->failed || h->stop) {
        mtx_unlock(&h->mutex);
        return false;
    }
    if (h->playing) {
        mtx_unlock(&h->mutex);
        return true;
    }
    h->playing = true;
    mtx_unlock(&h->mutex);
    if (h->config.output.start(h->config.output.user, h))
        return true;
    mtx_lock(&h->mutex);
    h->playing = false;
    mtx_unlock(&h->mutex);
    return false;
}
bool video_audio_pause(video_audio_host *h) {
    if (!owner(h) || !h->config.output.stop(h->config.output.user, false))
        return false;
    mtx_lock(&h->mutex);
    h->playing = false;
    mtx_unlock(&h->mutex);
    return true;
}
bool video_audio_seek(video_audio_host *h, uint64_t target) {
    if (!owner(h))
        return false;
    mtx_lock(&h->mutex);
    bool valid = h->generation != UINT64_MAX;
    mtx_unlock(&h->mutex);
    if (!valid || !h->config.output.stop(h->config.output.user, true))
        return false;
    mtx_lock(&h->mutex);
    uint64_t obsolete = h->generation++;
    h->target = h->next_decode = h->position = target;
    h->pcm_head = h->pcm_count = h->output_head = h->output_count = h->output_media = 0;
    h->submitted = h->acknowledged = 0;
    h->eof = h->failed = h->playing = false;
    h->seek_pending = true;
    cnd_broadcast(&h->changed);
    mtx_unlock(&h->mutex);
    h->config.decoder.interrupt(h->config.decoder.user, obsolete);
    return true;
}
bool video_audio_destroy(video_audio_host *h) {
    if (!owner(h) || !h->config.output.stop(h->config.output.user, true))
        return false;
    mtx_lock(&h->mutex);
    h->stop = true;
    h->playing = false;
    cnd_broadcast(&h->changed);
    mtx_unlock(&h->mutex);
    h->config.decoder.interrupt(h->config.decoder.user, UINT64_MAX);
    thrd_join(h->worker, NULL);
    cnd_destroy(&h->changed);
    mtx_destroy(&h->mutex);
    free(h->ledger);
    free(h->scratch);
    free(h->pcm);
    free(h);
    return true;
}
bool video_audio_render(video_audio_host *h, float *stereo, size_t frames,
                        video_audio_ticket *ticket) {
    if (!h || !stereo || !ticket || !frames)
        return false;
    mtx_lock(&h->mutex);
    if (!h->playing || h->failed || h->stop ||
        frames > h->config.capacity_frames - h->output_count ||
        frames > UINT64_MAX - h->submitted) {
        reject(h);
        mtx_unlock(&h->mutex);
        return false;
    }
    size_t media = frames < h->pcm_count ? frames : h->pcm_count;
    if (media > UINT64_MAX - h->rendered_media || frames - media > UINT64_MAX - h->silence) {
        h->failed = true;
        reject(h);
        mtx_unlock(&h->mutex);
        return false;
    }
    *ticket = (video_audio_ticket){h->generation, h->submitted, frames};
    uint64_t first = h->next_decode - h->pcm_count;
    for (size_t i = 0; i < frames; i++) {
        output_frame *entry =
            &h->ledger[(h->output_head + h->output_count + i) % h->config.capacity_frames];
        *entry = (output_frame){.media = i < media, .frame = first + (i < media ? i : 0)};
        if (i < media) {
            size_t slot = (h->pcm_head + i) % h->config.capacity_frames;
            memcpy(stereo + i * 2, h->pcm + slot * 2, 2 * sizeof(float));
        } else {
            stereo[i * 2] = stereo[i * 2 + 1] = 0;
        }
    }
    h->pcm_head = (h->pcm_head + media) % h->config.capacity_frames;
    h->pcm_count -= media;
    h->output_count += frames;
    h->output_media += media;
    h->submitted += frames;
    h->rendered_media += media;
    h->silence += frames - media;
    cnd_broadcast(&h->changed);
    mtx_unlock(&h->mutex);
    return true;
}
bool video_audio_consume(video_audio_host *h, uint64_t generation, uint64_t sequence,
                         size_t frames) {
    if (!h || !frames)
        return false;
    mtx_lock(&h->mutex);
    if (!h->playing || h->stop || generation != h->generation || sequence != h->acknowledged ||
        frames > h->output_count) {
        reject(h);
        mtx_unlock(&h->mutex);
        return false;
    }
    for (size_t i = 0; i < frames; i++) {
        output_frame entry = h->ledger[(h->output_head + i) % h->config.capacity_frames];
        if (entry.media) {
            h->position = entry.frame + 1;
            h->consumed_media++;
            h->output_media--;
        } else {
            h->consumed_silence++;
        }
    }
    h->output_head = (h->output_head + frames) % h->config.capacity_frames;
    h->output_count -= frames;
    h->acknowledged += frames;
    mtx_unlock(&h->mutex);
    return true;
}
video_audio_snapshot video_audio_poll(video_audio_host *h) {
    if (!h)
        return (video_audio_snapshot){.failed = true};
    mtx_lock(&h->mutex);
    video_audio_snapshot s = {.generation = h->generation,
                              .position_frames = h->position,
                              .submitted_frames = h->submitted,
                              .consumed_frames = h->acknowledged,
                              .pcm_frames = h->pcm_count,
                              .output_frames = h->output_count,
                              .output_media_frames = h->output_media,
                              .rendered_media = h->rendered_media,
                              .consumed_media = h->consumed_media,
                              .silence_frames = h->silence,
                              .consumed_silence = h->consumed_silence,
                              .rejected = h->rejected,
                              .playing = h->playing,
                              .decoder_eof = h->eof,
                              .ended = h->eof && !h->pcm_count && !h->output_media,
                              .failed = h->failed,
                              .buffering =
                                  h->playing && !h->pcm_count && !h->output_media && !h->eof};
    mtx_unlock(&h->mutex);
    return s;
}
