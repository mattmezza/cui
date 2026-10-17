/* SPDX-License-Identifier: MIT */
#include "decoder.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
/* Three slots bound memory independently of movie duration. Worker may convert
 * one reserved slot, queue others; leased slots are never touched by worker. */
typedef struct {
    unsigned char *pixels;
    int width, height, stride;
    uint64_t revision, generation;
    double pts;
    enum { FREE, WRITING, QUEUED, LEASED } state;
} frame_slot;
struct video_decoder {
    thrd_t thread;
    mtx_t mutex;
    cnd_t changed;
    char *path;
    uint64_t source, generation, command;
    double seek_to;
    bool stop, seek, seek_preview;
    video_decoder_status status;
    frame_slot slots[3];
};
static void fail(video_decoder *d) {
    mtx_lock(&d->mutex);
    d->status.failed = true;
    cnd_broadcast(&d->changed);
    mtx_unlock(&d->mutex);
}
static bool publish(video_decoder *d, AVFrame *frame, struct SwsContext **scaler,
                    uint64_t generation, uint64_t command, double pts) {
    mtx_lock(&d->mutex);
    unsigned slot = 3;
    while (!d->stop && d->command == command) {
        for (unsigned i = 0; i < 3; ++i)
            if (d->slots[i].state == FREE) {
                slot = i;
                break;
            }
        if (slot < 3)
            break;
        cnd_wait(&d->changed, &d->mutex);
    }
    if (d->stop || d->command != command) {
        mtx_unlock(&d->mutex);
        return true;
    }
    frame_slot *s = &d->slots[slot];
    s->state = WRITING;
    mtx_unlock(&d->mutex);
    if (frame->width <= 0 || frame->height <= 0 || frame->width > 4096 || frame->height > 4096)
        return false;
    int stride = frame->width * 4;
    size_t bytes = (size_t)stride * (size_t)frame->height;
    if (s->width != frame->width || s->height != frame->height) {
        unsigned char *next = malloc(bytes);
        if (!next)
            return false;
        free(s->pixels);
        s->pixels = next;
        s->width = frame->width;
        s->height = frame->height;
        s->stride = stride;
    }
    *scaler = sws_getCachedContext(*scaler, frame->width, frame->height,
                                   (enum AVPixelFormat)frame->format, frame->width, frame->height,
                                   AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    if (!*scaler)
        return false;
    uint8_t *output[4] = {s->pixels, NULL, NULL, NULL};
    int strides[4] = {stride, 0, 0, 0};
    if (sws_scale(*scaler, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height,
                  output, strides) != frame->height)
        return false;
    mtx_lock(&d->mutex);
    if (d->stop || d->command != command)
        s->state = FREE;
    else {
        s->generation = generation;
        s->pts = pts;
        if (s->revision == UINT64_MAX) {
            mtx_unlock(&d->mutex);
            return false;
        }
        ++s->revision;
        s->state = QUEUED;
    }
    cnd_broadcast(&d->changed);
    mtx_unlock(&d->mutex);
    return true;
}
static int interrupted(void *user) {
    video_decoder *d = user;
    mtx_lock(&d->mutex);
    bool stop = d->stop;
    mtx_unlock(&d->mutex);
    return stop ? 1 : 0;
}
static int decode(void *user) {
    video_decoder *d = user;
    AVFormatContext *format = NULL;
    AVCodecContext *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    struct SwsContext *scaler = NULL;
    bool ok = false;
    format = avformat_alloc_context();
    if (!format)
        goto done;
    format->interrupt_callback = (AVIOInterruptCB){interrupted, d};
    AVDictionary *options = NULL;
    av_dict_set(&options, "protocol_whitelist", "file", 0);
    int opened = avformat_open_input(&format, d->path, NULL, &options);
    av_dict_free(&options);
    if (opened < 0 || avformat_find_stream_info(format, NULL) < 0)
        goto done;
    const AVCodec *decoder = NULL;
    int stream = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (stream < 0 || !decoder)
        goto done;
    AVStream *video = format->streams[stream];
    codec = avcodec_alloc_context3(decoder);
    if (!codec || avcodec_parameters_to_context(codec, video->codecpar) < 0)
        goto done;
    codec->thread_count = 1;
    if (avcodec_open2(codec, decoder, NULL) < 0)
        goto done;
    /* Bound decoder threading too; this example is for software local playback. */
    frame = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet)
        goto done;
    double origin =
        video->start_time == AV_NOPTS_VALUE ? 0 : video->start_time * av_q2d(video->time_base);
    double duration = video->duration != AV_NOPTS_VALUE ? video->duration * av_q2d(video->time_base)
                      : format->duration != AV_NOPTS_VALUE ? (double)format->duration / AV_TIME_BASE
                                                           : 0;
    mtx_lock(&d->mutex);
    d->status.ready = true;
    d->status.duration_known = isfinite(duration) && duration > 0;
    d->status.duration = d->status.duration_known ? duration : 0;
    uint64_t command = d->command, generation = d->generation;
    mtx_unlock(&d->mutex);
    double discard_before = 0;
    bool draining = false;
    for (;;) {
        mtx_lock(&d->mutex);
        while (d->status.eof && !d->seek && !d->stop)
            cnd_wait(&d->changed, &d->mutex);
        bool stop = d->stop, seek = d->seek;
        double target = d->seek_to;
        if (seek) {
            command = d->command;
            generation = d->generation;
            d->seek = false;
            d->status.eof = false;
        }
        mtx_unlock(&d->mutex);
        if (stop) {
            ok = true;
            break;
        }
        if (seek) {
            double timestamp = (target + origin) / av_q2d(video->time_base);
            if (!isfinite(timestamp) || timestamp < INT64_MIN || timestamp >= INT64_MAX ||
                avformat_seek_file(format, stream, INT64_MIN, (int64_t)timestamp, INT64_MAX, 0) < 0)
                goto done;
            avcodec_flush_buffers(codec);
            av_packet_unref(packet);
            discard_before = target;
            draining = false;
        }
        int received = avcodec_receive_frame(codec, frame);
        if (received == 0) {
            if (frame->best_effort_timestamp == AV_NOPTS_VALUE)
                goto done;
            double pts = frame->best_effort_timestamp * av_q2d(video->time_base) - origin;
            if (!isfinite(pts))
                goto done;
            if (pts >= discard_before && !publish(d, frame, &scaler, generation, command, pts))
                goto done;
            av_frame_unref(frame);
            continue;
        }
        if (received == AVERROR_EOF) {
            mtx_lock(&d->mutex);
            if (d->command == command)
                d->status.eof = true;
            mtx_unlock(&d->mutex);
            continue;
        }
        if (received != AVERROR(EAGAIN) || draining)
            goto done;
        int read;
        do {
            av_packet_unref(packet);
            read = av_read_frame(format, packet);
        } while (read >= 0 && packet->stream_index != stream);
        if (read == AVERROR_EOF) {
            if (avcodec_send_packet(codec, NULL) < 0)
                goto done;
            draining = true;
        } else if (read < 0 || avcodec_send_packet(codec, packet) < 0)
            goto done;
    }
done:
    if (!ok)
        fail(d);
    sws_freeContext(scaler);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codec);
    avformat_close_input(&format);
    return ok ? 0 : 1;
}
video_decoder *video_decoder_open(const char *path, uint64_t source, uint64_t generation) {
    if (!path || !source || !generation)
        return NULL;
    video_decoder *d = calloc(1, sizeof *d);
    if (!d)
        return NULL;
    d->path = malloc(strlen(path) + 1);
    if (!d->path) {
        free(d);
        return NULL;
    }
    strcpy(d->path, path);
    d->source = source;
    d->generation = generation;
    if (mtx_init(&d->mutex, mtx_plain) != thrd_success) {
        free(d->path);
        free(d);
        return NULL;
    }
    if (cnd_init(&d->changed) != thrd_success) {
        mtx_destroy(&d->mutex);
        free(d->path);
        free(d);
        return NULL;
    }
    if (thrd_create(&d->thread, decode, d) != thrd_success) {
        cnd_destroy(&d->changed);
        mtx_destroy(&d->mutex);
        free(d->path);
        free(d);
        return NULL;
    }
    return d;
}
bool video_decoder_seek(video_decoder *d, uint64_t generation, double seconds) {
    if (!d || !generation || !isfinite(seconds) || seconds < 0)
        return false;
    mtx_lock(&d->mutex);
    bool ok =
        !d->status.failed && !d->stop && generation > d->generation && d->command < UINT64_MAX;
    if (ok) {
        ++d->command;
        d->generation = generation;
        d->seek_to = seconds;
        d->seek = true;
        d->seek_preview = true;
        d->status.eof = false;
        for (unsigned i = 0; i < 3; ++i)
            if (d->slots[i].state == QUEUED)
                d->slots[i].state = FREE;
        cnd_broadcast(&d->changed);
    }
    mtx_unlock(&d->mutex);
    return ok;
}
bool video_decoder_acquire(video_decoder *d, uint64_t generation, double due, video_lease *out) {
    if (!d || !out || !isfinite(due) || due < 0)
        return false;
    mtx_lock(&d->mutex);
    unsigned best = 3;
    for (unsigned i = 0; i < 3; ++i) {
        frame_slot *s = &d->slots[i];
        if (s->state == QUEUED && s->generation == generation &&
            ((d->seek_preview && due >= d->seek_to) || s->pts <= due) &&
            (best == 3 ||
             (d->seek_preview ? s->pts < d->slots[best].pts : s->pts > d->slots[best].pts)))
            best = i;
    }
    if (best < 3) {
        frame_slot *s = &d->slots[best];
        s->state = LEASED;
        d->seek_preview = false;
        *out = (video_lease){
            .slot = best,
            .frame = {.source_id = d->source,
                      .frame = {.bitmap = {s->pixels, (unsigned)s->width, (unsigned)s->height,
                                           (size_t)s->stride, s->revision},
                                .generation = s->generation,
                                .timestamp = s->pts}}};
        ++d->status.acquired;
        for (unsigned i = 0; i < 3; ++i)
            if (d->slots[i].state == QUEUED && d->slots[i].generation == generation &&
                d->slots[i].pts < s->pts)
                d->slots[i].state = FREE;
        cnd_broadcast(&d->changed);
    }
    mtx_unlock(&d->mutex);
    return best < 3;
}
bool video_decoder_release(video_decoder *d, video_lease *lease) {
    if (!d || !lease || lease->slot >= 3 || !lease->frame.source_id)
        return false;
    mtx_lock(&d->mutex);
    frame_slot *s = &d->slots[lease->slot];
    bool ok = s->state == LEASED && lease->frame.source_id == d->source &&
              lease->frame.frame.bitmap.pixels == s->pixels &&
              lease->frame.frame.bitmap.revision == s->revision &&
              lease->frame.frame.generation == s->generation;
    if (ok) {
        s->state = FREE;
        ++d->status.released;
        *lease = (video_lease){0};
        cnd_broadcast(&d->changed);
    }
    mtx_unlock(&d->mutex);
    return ok;
}
video_decoder_status video_decoder_poll(video_decoder *d) {
    if (!d)
        return (video_decoder_status){.failed = true};
    mtx_lock(&d->mutex);
    video_decoder_status status = d->status;
    for (unsigned i = 0; i < 3; ++i) {
        status.queued += d->slots[i].state == QUEUED;
        status.leased += d->slots[i].state == LEASED;
    }
    mtx_unlock(&d->mutex);
    return status;
}
bool video_decoder_close(video_decoder *d) {
    if (!d)
        return true;
    mtx_lock(&d->mutex);
    for (unsigned i = 0; i < 3; ++i)
        if (d->slots[i].state == LEASED) {
            mtx_unlock(&d->mutex);
            return false;
        }
    d->stop = true;
    cnd_broadcast(&d->changed);
    mtx_unlock(&d->mutex);
    thrd_join(d->thread, NULL);
    for (unsigned i = 0; i < 3; ++i)
        free(d->slots[i].pixels);
    cnd_destroy(&d->changed);
    mtx_destroy(&d->mutex);
    free(d->path);
    free(d);
    return true;
}
