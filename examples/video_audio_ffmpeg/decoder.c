/* SPDX-License-Identifier: MIT */
#include "decoder.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <threads.h>
struct video_audio_ffmpeg {
    char *path;
    video_ffmpeg_limits limits;
    atomic_uint_fast64_t cancelled, active;
    mtx_t stats_mutex;
    video_ffmpeg_snapshot stats;
    AVFormatContext *format;
    AVCodecContext *codec;
    AVFrame *frame;
    AVPacket *packet;
    SwrContext *swr;
    float *spill;
    size_t spill_head, spill_count;
    uint64_t generation, emit, input_samples, converted;
    int64_t origin, anchor, rescale_last;
    int stream, rate;
    enum AVSampleFormat sample_format;
    bool ready, anchored, demux_eof, flush_sent, codec_eof, finished;
};
static void status(video_audio_ffmpeg *s, video_ffmpeg_status value) {
    mtx_lock(&s->stats_mutex);
    s->stats.status = value;
    mtx_unlock(&s->stats_mutex);
}
static int interrupted(void *user) {
    video_audio_ffmpeg *s = user;
    return atomic_load(&s->active) <= atomic_load(&s->cancelled);
}
static bool error(video_audio_ffmpeg *s, video_ffmpeg_status value) {
    status(s, interrupted(s) ? VIDEO_FFMPEG_CANCELLED : value);
    return false;
}
static void close_file(video_audio_ffmpeg *s) {
    swr_free(&s->swr);
    av_packet_free(&s->packet);
    av_frame_free(&s->frame);
    avcodec_free_context(&s->codec);
    avformat_close_input(&s->format);
}
static bool pcm_codec(enum AVCodecID id) {
    return id == AV_CODEC_ID_PCM_S16LE || id == AV_CODEC_ID_PCM_S24LE ||
           id == AV_CODEC_ID_PCM_S32LE || id == AV_CODEC_ID_PCM_F32LE;
}
static bool seek_file(void *user, uint64_t generation, uint64_t target) {
    video_audio_ffmpeg *s = user;
    s->ready = false;
    s->generation = generation;
    atomic_store(&s->active, generation);
    close_file(s);
    s->emit = target;
    s->input_samples = s->converted = 0;
    s->spill_head = s->spill_count = 0;
    s->anchored = s->demux_eof = s->flush_sent = s->codec_eof = s->finished = false;
    s->rescale_last = AV_NOPTS_VALUE;
    mtx_lock(&s->stats_mutex);
    s->stats = (video_ffmpeg_snapshot){.generation = generation};
    mtx_unlock(&s->stats_mutex);
    if (interrupted(s))
        return error(s, VIDEO_FFMPEG_CANCELLED);
    if (target > s->limits.timeline_frames)
        return error(s, VIDEO_FFMPEG_LIMIT);
    /* Immutable regular local files only; never accept a FIFO/device as input.
     * This is a preflight, not a sandbox against concurrent path replacement. */
    struct stat info;
    if (stat(s->path, &info) != 0)
        return error(s, VIDEO_FFMPEG_IO);
#ifdef _WIN32
    if ((info.st_mode & _S_IFMT) != _S_IFREG)
#else
    if (!S_ISREG(info.st_mode))
#endif
        return error(s, VIDEO_FFMPEG_UNSUPPORTED);
    s->format = avformat_alloc_context();
    if (!s->format)
        return error(s, VIDEO_FFMPEG_DECODE);
    s->format->probesize = (int64_t)s->limits.packet_bytes;
    s->format->max_streams = 16;
    s->format->max_analyze_duration = AV_TIME_BASE;
    s->format->max_probe_packets = (int)s->limits.packets;
    s->format->interrupt_callback = (AVIOInterruptCB){interrupted, s};
    AVDictionary *options = NULL;
    av_dict_set(&options, "protocol_whitelist", "file", 0);
    av_dict_set(&options, "format_whitelist", "matroska,webm,wav", 0);
    int result = avformat_open_input(&s->format, s->path, NULL, &options);
    av_dict_free(&options);
    if (result < 0 || avformat_find_stream_info(s->format, NULL) < 0)
        return error(s, VIDEO_FFMPEG_IO);
    s->stream = av_find_best_stream(s->format, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (s->stream < 0)
        return error(s, VIDEO_FFMPEG_UNSUPPORTED);
    AVStream *stream = s->format->streams[s->stream];
    AVCodecParameters *p = stream->codecpar;
    if (!pcm_codec(p->codec_id) || p->sample_rate < 8000 || p->sample_rate > 192000 ||
        p->ch_layout.nb_channels < 1 || p->ch_layout.nb_channels > 2 || p->initial_padding ||
        p->trailing_padding || p->seek_preroll || s->format->start_time == AV_NOPTS_VALUE ||
        stream->time_base.num <= 0 || stream->time_base.den <= 0)
        return error(s, VIDEO_FFMPEG_UNSUPPORTED);
    s->origin = av_rescale_q(s->format->start_time, AV_TIME_BASE_Q, (AVRational){1, 48000});
    mtx_lock(&s->stats_mutex);
    s->stats.common_origin_us = s->format->start_time;
    mtx_unlock(&s->stats_mutex);
    const AVCodec *codec = avcodec_find_decoder(p->codec_id);
    s->codec = codec ? avcodec_alloc_context3(codec) : NULL;
    s->packet = av_packet_alloc();
    s->frame = av_frame_alloc();
    if (!s->codec || !s->packet || !s->frame || avcodec_parameters_to_context(s->codec, p) < 0)
        return error(s, VIDEO_FFMPEG_DECODE);
    s->codec->thread_count = 1;
    s->rate = p->sample_rate;
    if (avcodec_open2(s->codec, codec, NULL) < 0)
        return error(s, VIDEO_FFMPEG_DECODE);
    s->ready = true;
    return true;
}
/* 1=frame, 0=codec EOF, -1=error/cancel. One worker owns all codec/demux calls. */
static int next_frame(video_audio_ffmpeg *s) {
    for (;;) {
        if (interrupted(s)) {
            error(s, VIDEO_FFMPEG_CANCELLED);
            return -1;
        }
        av_frame_unref(s->frame);
        int result = avcodec_receive_frame(s->codec, s->frame);
        if (result >= 0)
            return 1;
        if (result == AVERROR_EOF)
            return 0;
        if (result != AVERROR(EAGAIN)) {
            error(s, VIDEO_FFMPEG_DECODE);
            return -1;
        }
        if (s->demux_eof) {
            if (s->flush_sent || avcodec_send_packet(s->codec, NULL) < 0) {
                error(s, VIDEO_FFMPEG_DECODE);
                return -1;
            }
            s->flush_sent = true;
            continue;
        }
        result = av_read_frame(s->format, s->packet);
        if (result == AVERROR_EOF) {
            s->demux_eof = true;
            continue;
        }
        if (result < 0) {
            error(s, VIDEO_FFMPEG_IO);
            return -1;
        }
        mtx_lock(&s->stats_mutex);
        uint64_t packets = ++s->stats.packets;
        if (s->format->streams[s->packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            s->stats.video_packets++;
        mtx_unlock(&s->stats_mutex);
        if (packets > s->limits.packets || s->packet->size < 0 ||
            (size_t)s->packet->size > s->limits.packet_bytes) {
            av_packet_unref(s->packet);
            error(s, VIDEO_FFMPEG_LIMIT);
            return -1;
        }
        bool audio = s->packet->stream_index == s->stream;
        result = audio ? avcodec_send_packet(s->codec, s->packet) : 0;
        av_packet_unref(s->packet);
        if (result < 0) {
            error(s, VIDEO_FFMPEG_DECODE);
            return -1;
        }
    }
}
static bool convert(video_audio_ffmpeg *s, bool drain) {
    AVFrame *f = s->frame;
    int samples = drain ? 0 : f->nb_samples;
    if (!drain) {
        if (samples <= 0 || (size_t)samples > s->limits.frame_samples ||
            f->sample_rate != s->rate ||
            av_channel_layout_compare(&f->ch_layout, &s->codec->ch_layout) ||
            av_frame_get_side_data(f, AV_FRAME_DATA_SKIP_SAMPLES))
            return error(s, VIDEO_FFMPEG_UNSUPPORTED);
        int64_t pts =
            f->best_effort_timestamp != AV_NOPTS_VALUE ? f->best_effort_timestamp : f->pts;
        if (pts == AV_NOPTS_VALUE)
            return error(s, VIDEO_FFMPEG_TIMESTAMP);
        AVRational tb = s->format->streams[s->stream]->time_base;
        int64_t start = av_rescale_delta(tb, pts, (AVRational){1, s->rate}, samples,
                                         &s->rescale_last, (AVRational){1, 48000});
        if (start < s->origin || s->origin < 0 ||
            start - s->origin > (int64_t)s->limits.timeline_frames)
            return error(s, VIDEO_FFMPEG_TIMESTAMP);
        start -= s->origin;
        if (!s->anchored) {
            if ((uint64_t)start > s->limits.leading_gap_frames)
                return error(s, VIDEO_FFMPEG_TIMESTAMP);
            s->anchor = start;
            s->sample_format = (enum AVSampleFormat)f->format;
            s->anchored = true;
            AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
            if (swr_alloc_set_opts2(&s->swr, &stereo, AV_SAMPLE_FMT_FLT, 48000, &f->ch_layout,
                                    s->sample_format, s->rate, 0, NULL) < 0 ||
                swr_init(s->swr) < 0)
                return error(s, VIDEO_FFMPEG_DECODE);
        }
        int64_t expected =
            s->anchor + av_rescale_q((int64_t)s->input_samples, (AVRational){1, s->rate},
                                     (AVRational){1, 48000});
        int64_t tolerance = av_rescale_rnd((int64_t)tb.num * 48000, 1, tb.den, AV_ROUND_UP) + 1;
        if (f->format != s->sample_format || llabs(start - expected) > tolerance)
            return error(s, VIDEO_FFMPEG_TIMESTAMP);
        s->input_samples += (uint64_t)samples;
        if (s->input_samples >
            s->limits.timeline_frames * (uint64_t)s->rate / 48000 + s->limits.frame_samples)
            return error(s, VIDEO_FFMPEG_LIMIT);
    }
    if (!s->swr) {
        s->finished = true;
        return true;
    }
    int bound = swr_get_out_samples(s->swr, samples);
    if (bound < 0 || (size_t)bound > s->limits.converted_frames)
        return error(s, VIDEO_FFMPEG_LIMIT);
    uint8_t *out[] = {(uint8_t *)s->spill};
    const uint8_t *in[2] = {drain ? NULL : f->extended_data[0], drain ? NULL : f->extended_data[1]};
    int count =
        swr_convert(s->swr, out, (int)s->limits.converted_frames, drain ? NULL : in, samples);
    if (count < 0)
        return error(s, VIDEO_FFMPEG_DECODE);
    if (!count && drain) {
        s->finished = true;
        return true;
    }
    if (s->converted + (uint64_t)count + (uint64_t)s->anchor > s->limits.timeline_frames)
        return error(s, VIDEO_FFMPEG_LIMIT);
    for (int i = 0; i < count * 2; i++)
        if (!isfinite(s->spill[i]))
            return error(s, VIDEO_FFMPEG_DECODE);
    s->spill_head = 0;
    s->spill_count = (size_t)count;
    s->converted += (uint64_t)count;
    mtx_lock(&s->stats_mutex);
    s->stats.input_samples = s->input_samples;
    s->stats.converted_frames = s->converted;
    mtx_unlock(&s->stats_mutex);
    return true;
}
static video_audio_read read_pcm(void *user, uint64_t generation, float *pcm, size_t capacity,
                                 uint64_t *first, size_t *frames) {
    video_audio_ffmpeg *s = user;
    *frames = 0;
    if (!s->ready || !capacity || generation != s->generation)
        return VIDEO_AUDIO_ERROR;
    for (;;) {
        if (interrupted(s)) {
            status(s, VIDEO_FFMPEG_CANCELLED);
            return VIDEO_AUDIO_AGAIN;
        }
        if (s->spill_count) {
            uint64_t begin = (uint64_t)s->anchor + s->converted - s->spill_count;
            if (begin < s->emit) {
                size_t skip =
                    s->emit - begin < s->spill_count ? (size_t)(s->emit - begin) : s->spill_count;
                s->spill_head += skip;
                s->spill_count -= skip;
                continue;
            }
            size_t n = capacity;
            if (begin > s->emit) {
                uint64_t gap = begin - s->emit;
                if (gap > s->limits.leading_gap_frames) {
                    error(s, VIDEO_FFMPEG_TIMESTAMP);
                    return VIDEO_AUDIO_ERROR;
                }
                if (n > gap)
                    n = (size_t)gap;
                memset(pcm, 0, n * 2 * sizeof(float));
            } else {
                if (n > s->spill_count)
                    n = s->spill_count;
                memcpy(pcm, s->spill + s->spill_head * 2, n * 2 * sizeof(float));
                s->spill_head += n;
                s->spill_count -= n;
            }
            *first = s->emit;
            *frames = n;
            s->emit += n;
            return VIDEO_AUDIO_DATA;
        }
        if (s->finished) {
            status(s, VIDEO_FFMPEG_EOF);
            return VIDEO_AUDIO_EOF;
        }
        if (!s->codec_eof) {
            int result = next_frame(s);
            if (result < 0)
                return interrupted(s) ? VIDEO_AUDIO_AGAIN : VIDEO_AUDIO_ERROR;
            s->codec_eof = !result;
        }
        if (!convert(s, s->codec_eof))
            return interrupted(s) ? VIDEO_AUDIO_AGAIN : VIDEO_AUDIO_ERROR;
    }
}
static void interrupt_service(void *user, uint64_t through) {
    video_audio_ffmpeg *s = user;
    uint_fast64_t old = atomic_load(&s->cancelled);
    while (old < through && !atomic_compare_exchange_weak(&s->cancelled, &old, through)) {
    }
}
video_audio_ffmpeg *video_ffmpeg_create(const char *path, video_ffmpeg_limits limits) {
    if (!path)
        return NULL;
    size_t length = strlen(path);
    if (!length || length > 4095 || strchr(path, ':') || strchr(path, '\n') || strchr(path, '\r'))
        return NULL;
    if (!limits.packet_bytes)
        limits.packet_bytes = 8 * 1024 * 1024;
    if (!limits.frame_samples)
        limits.frame_samples = 8192;
    if (!limits.converted_frames)
        limits.converted_frames = 65536;
    if (!limits.packets)
        limits.packets = 8192;
    if (!limits.timeline_frames)
        limits.timeline_frames = 480000;
    if (!limits.leading_gap_frames)
        limits.leading_gap_frames = limits.timeline_frames < 12000 ? limits.timeline_frames : 12000;
    if (limits.packet_bytes > 8 * 1024 * 1024 || limits.frame_samples > 8192 ||
        limits.converted_frames > 65536 || limits.packets > 1048576 ||
        limits.timeline_frames > 480000 || limits.leading_gap_frames > limits.timeline_frames)
        return NULL;
    video_audio_ffmpeg *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->limits = limits;
    s->path = malloc(length + 1);
    s->spill = calloc(limits.converted_frames * 2, sizeof(float));
    if (!s->path || !s->spill || mtx_init(&s->stats_mutex, mtx_plain) != thrd_success) {
        free(s->path);
        free(s->spill);
        free(s);
        return NULL;
    }
    memcpy(s->path, path, length + 1);
    atomic_init(&s->cancelled, 0);
    atomic_init(&s->active, 1);
    return s;
}
video_audio_decoder video_ffmpeg_interface(video_audio_ffmpeg *s) {
    return (video_audio_decoder){seek_file, read_pcm, interrupt_service, s};
}
video_ffmpeg_snapshot video_ffmpeg_poll(video_audio_ffmpeg *s) {
    mtx_lock(&s->stats_mutex);
    video_ffmpeg_snapshot result = s->stats;
    mtx_unlock(&s->stats_mutex);
    return result;
}
void video_ffmpeg_destroy(video_audio_ffmpeg *s) {
    if (s) {
        close_file(s);
        mtx_destroy(&s->stats_mutex);
        free(s->spill);
        free(s->path);
        free(s);
    }
}
