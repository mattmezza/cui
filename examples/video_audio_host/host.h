/* SPDX-License-Identifier: MIT */
#ifndef VIDEO_AUDIO_HOST_H
#define VIDEO_AUDIO_HOST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Optional example foundation, not a CUI public API or audio-device transport.
 * Fixed 48000 Hz interleaved stereo float PCM. No rate conversion here. */
typedef struct video_audio_host video_audio_host;
typedef enum {
    VIDEO_AUDIO_DATA,
    VIDEO_AUDIO_AGAIN,
    VIDEO_AUDIO_EOF,
    VIDEO_AUDIO_ERROR
} video_audio_read;
typedef struct {
    /* Worker only. seek/read must cooperate with interrupt; all returned PCM is
     * copied. DATA is nonempty, finite, contiguous, at the requested source rate.
     * AGAIN retries after a bounded wait. No callback may retain PCM pointers. */
    bool (*seek)(void *user, uint64_t generation, uint64_t frame);
    video_audio_read (*read)(void *user, uint64_t generation, float *stereo, size_t capacity,
                             uint64_t *first_frame, size_t *frames);
    /* Owner thread, outside host locks; nonblocking, wakes an in-flight read or
     * seek whose generation is <= through_generation; newer work is unaffected.
     * Cancellation is sticky for that generation, even if its operation starts
     * just after interrupt. Return promptly; do not destroy service. */
    void (*interrupt)(void *user, uint64_t through_generation);
    void *user;
} video_audio_decoder;
typedef struct {
    /* Owner thread, outside locks. start failure guarantees no callbacks started.
     * stop success guarantees every render/consume callback has returned and no
     * more enters until start. discard=false retains pending output for resume;
     * discard=true additionally discards unconsumed output and releases retained
     * host pointers/queued notifications after all final consumption reports.
     * A failed stop MUST NOT be treated as quiescence; host remains allocated. */
    bool (*start)(void *user, video_audio_host *host);
    bool (*stop)(void *user, bool discard);
    void *user;
} video_audio_output;
typedef struct {
    size_t capacity_frames, decode_chunk_frames; /* each 1..65536, chunk <= capacity */
    video_audio_decoder decoder;
    video_audio_output output;
} video_audio_config;
typedef struct {
    uint64_t generation, first_sequence;
    size_t frames;
} video_audio_ticket;
typedef struct {
    uint64_t generation, position_frames;
    uint64_t submitted_frames, consumed_frames; /* current generation, includes silence */
    size_t pcm_frames, output_frames, output_media_frames;
    uint64_t rendered_media, consumed_media, silence_frames, consumed_silence,
        rejected; /* lifetime */
    bool playing, decoder_eof, ended, failed, buffering;
} video_audio_snapshot;
/* create/play/pause/seek/destroy: creating owner thread only, never concurrent.
 * config callbacks/users live through successful destroy. Creation is paused.
 * read/seek run on a single owned C11 worker; decoder is prefilled while paused. */
video_audio_host *video_audio_create(video_audio_config);
bool video_audio_play(video_audio_host *);
bool video_audio_pause(video_audio_host *);
/* Quiesces/discards output, increments generation, clears PCM/ledger, then sends
 * asynchronous worker seek. Restores previous play state only on explicit play.
 * Failed quiescence changes no host state. Seek also clears a decoder failure. */
bool video_audio_seek(video_audio_host *, uint64_t source_frame);
/* false leaves a live, allocated host: output did not prove quiescence or caller
 * is not owner. Never call from an output/decoder callback. No calls may race a
 * successful destroy; stop provides the output callback lifetime boundary. */
bool video_audio_destroy(video_audio_host *);
/* Output transport only. Successful render copies exactly requested frames,
 * padding absent PCM with zeros; each output frame receives a bounded ledger
 * entry. Full ledger/paused/failure reject atomically without touching buffer.
 * Submit ONLY successful renders. Render is submission, never consumption. */
bool video_audio_render(video_audio_host *, float *stereo, size_t frames, video_audio_ticket *);
/* Report frames actually consumed by output, in ticket order, including silence.
 * Partial acknowledgements use first_sequence + already acknowledged frames.
 * Reordered, duplicate, excessive or stale-generation acknowledgements reject
 * without advancing clock. A transport must establish actual consumption;
 * callback demand or queued-byte estimates alone do not satisfy this API. */
bool video_audio_consume(video_audio_host *, uint64_t generation, uint64_t first_sequence,
                         size_t frames);
/* Any thread while host is live. All state is copied under a short mutex.
 * No Core/UI calls or arbitrary user callbacks execute under that mutex. */
video_audio_snapshot video_audio_poll(video_audio_host *);
#endif
