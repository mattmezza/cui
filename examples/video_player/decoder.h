/* SPDX-License-Identifier: MIT */
#ifndef CUI_EXAMPLE_VIDEO_DECODER_H
#define CUI_EXAMPLE_VIDEO_DECODER_H
#include "cui/cui_video_controls.h"
/* Example-only local-file video adapter; it opens no audio output.
 * No FFmpeg types cross this boundary.
 * Every acquired lease must be released once, AFTER renderer consumption.
 * These calls are UI-thread operations; decoding/conversion runs on a worker. */
typedef struct video_decoder video_decoder;
typedef struct {
    cui_video_player_frame frame;
    unsigned slot;
} video_lease;
typedef struct {
    bool ready, failed, eof, duration_known;
    double duration;
    unsigned queued, leased;
    uint64_t acquired, released;
    /* Additive example-only common-clock metadata; legacy open leaves false.
     * end_frame is the greatest decoded frame endpoint in the current worker
     * generation, not a presentation/audio completion indication. */
    bool common_clock;
    int64_t common_origin_us;
    uint64_t clock_generation, end_frame;
} video_decoder_status;
video_decoder *video_decoder_open(const char *path, uint64_t source_id, uint64_t generation);
/* Optional sound example mode: known container origin, rational 48k PTS,
 * FFV1 only, <=10s normalized frames. Preserves legacy stream-relative open. */
video_decoder *video_decoder_open_common(const char *path, uint64_t source_id, uint64_t generation);
bool video_decoder_seek(video_decoder *, uint64_t generation, double seconds);
bool video_decoder_acquire(video_decoder *, uint64_t generation, double due, video_lease *);
bool video_decoder_release(video_decoder *, video_lease *);
video_decoder_status video_decoder_poll(video_decoder *);
/* Requires every acquired frame released. Joins worker; no UI callbacks occur. */
bool video_decoder_close(video_decoder *);
#endif
