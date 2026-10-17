/* SPDX-License-Identifier: MIT */
#ifndef CUI_EXAMPLE_VIDEO_DECODER_H
#define CUI_EXAMPLE_VIDEO_DECODER_H
#include "cui/cui_video_controls.h"
/* Example-only silent local-file adapter. No FFmpeg types cross this boundary.
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
} video_decoder_status;
video_decoder *video_decoder_open(const char *path, uint64_t source_id, uint64_t generation);
bool video_decoder_seek(video_decoder *, uint64_t generation, double seconds);
bool video_decoder_acquire(video_decoder *, uint64_t generation, double due, video_lease *);
bool video_decoder_release(video_decoder *, video_lease *);
video_decoder_status video_decoder_poll(video_decoder *);
/* Requires every acquired frame released. Joins worker; no UI callbacks occur. */
bool video_decoder_close(video_decoder *);
#endif
