/* SPDX-License-Identifier: MIT */
#ifndef VIDEO_AUDIO_FFMPEG_DECODER_H
#define VIDEO_AUDIO_FFMPEG_DECODER_H
#include "../video_audio_host/host.h"
typedef struct video_audio_ffmpeg video_audio_ffmpeg;
typedef enum {
    VIDEO_FFMPEG_OK,
    VIDEO_FFMPEG_EOF,
    VIDEO_FFMPEG_CANCELLED,
    VIDEO_FFMPEG_IO,
    VIDEO_FFMPEG_UNSUPPORTED,
    VIDEO_FFMPEG_TIMESTAMP,
    VIDEO_FFMPEG_LIMIT,
    VIDEO_FFMPEG_DECODE
} video_ffmpeg_status;
typedef struct {
    /* Zero selects 8MiB / 8192 / 65536 / 8192 / 480000 / 12000. */
    size_t packet_bytes, frame_samples, converted_frames, packets;
    uint64_t timeline_frames, leading_gap_frames;
} video_ffmpeg_limits;
typedef struct {
    uint64_t generation, packets, video_packets, input_samples, converted_frames;
    int64_t common_origin_us;
    video_ffmpeg_status status;
} video_ffmpeg_snapshot;
/* Copies a <=4095-byte plain local path. Rejects URL schemes/colon and line
 * breaks; FFmpeg also admits only file protocol + Matroska/WAV demuxers.
 * PCM mono/stereo only; no device, networking, video rendering or Core calls. */
video_audio_ffmpeg *video_ffmpeg_create(const char *path, video_ffmpeg_limits);
video_audio_decoder video_ffmpeg_interface(video_audio_ffmpeg *);
/* Coherent copied snapshot; safe during worker use. */
video_ffmpeg_snapshot video_ffmpeg_poll(video_audio_ffmpeg *);
/* Caller must first successfully destroy/join the controller using this service.
 * No service calls or interrupt callbacks may race this operation. */
void video_ffmpeg_destroy(video_audio_ffmpeg *);
#endif
