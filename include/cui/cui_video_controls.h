/* SPDX-License-Identifier: MIT */
#ifndef CUI_VIDEO_CONTROLS_H
#define CUI_VIDEO_CONTROLS_H
#include "cui_components.h"
#include "cui_video.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_VIDEO_PLAYING,
    CUI_VIDEO_SEEK,
    CUI_VIDEO_SKIP,
    CUI_VIDEO_FULLSCREEN,
    CUI_VIDEO_RATE,
    CUI_VIDEO_MUTE,
    CUI_VIDEO_VOLUME
} cui_video_request_kind;
typedef struct {
    cui_video_request_kind kind;
    uint64_t source_id, generation, sequence;
    cui_slider_phase phase; /* SEEK only. Other requests are COMMIT. */
    union {
        bool enabled;
        double seconds;
        double rate;
        double volume; /* CUI_VIDEO_VOLUME, normalized to [0, 1]. */
    } value;
} cui_video_request;
typedef void (*cui_video_request_fn)(void *, const cui_video_request *);
enum {
    CUI_VIDEO_CAP_PLAY = 1u,
    CUI_VIDEO_CAP_SEEK = 2u,
    CUI_VIDEO_CAP_FULLSCREEN = 4u,
    CUI_VIDEO_CAP_RATE = 8u
};
typedef enum {
    CUI_VIDEO_READY,
    CUI_VIDEO_LOADING,
    CUI_VIDEO_BUFFERING,
    CUI_VIDEO_FAILED
} cui_video_status;
typedef struct {
    cui_video timeline; /* Copied confirmed state; never changed by controls. */
    uint64_t source_id; /* Nonzero, never reused while old requests/frames live. */
    unsigned capabilities;
    cui_video_status status;
    bool fullscreen;
    cui_str status_description;
} cui_video_view;
typedef enum {
    CUI_VIDEO_PART_PLAY,
    CUI_VIDEO_PART_BACK,
    CUI_VIDEO_PART_FORWARD,
    CUI_VIDEO_PART_TIMELINE,
    CUI_VIDEO_PART_FULLSCREEN,
    CUI_VIDEO_PART_RATE,
    CUI_VIDEO_PART_SURFACE,
    CUI_VIDEO_PART_MUTE,
    CUI_VIDEO_PART_VOLUME
} cui_video_part;
/* Stable deterministic child IDs, also useful for scoped focus/testing. */
cui_id cui_video_part_id(cui_id, cui_video_part);
enum {
    CUI_VIDEO_CONTROL_PLAY = 1u,
    CUI_VIDEO_CONTROL_SKIP = 2u,
    CUI_VIDEO_CONTROL_TIMELINE = 4u,
    CUI_VIDEO_CONTROL_FULLSCREEN = 8u,
    CUI_VIDEO_CONTROL_RATE = 16u
};
typedef struct {
    cui_slider_state scrub;
    uint64_t source_id, generation, sequence;
} cui_video_controls_state;
typedef struct {
    unsigned visible;
    float width;
    double skip_seconds, seek_step, requested_rate;
    bool disabled;
    cui_str play_icon, pause_icon, back_icon, forward_icon;
    cui_str fullscreen_icon, leave_fullscreen_icon, rate_icon;
    cui_str play_label, pause_label, back_label, forward_label, timeline_label;
    cui_str fullscreen_label, leave_fullscreen_label, rate_label;
    cui_video_request_fn on_request;
    void *user;
    bool compact; /* Opt-in standard bar: thin timeline, short time, 20px icons.
                   * Buttons/timeline retain >=44 logical unit hit targets. */
} cui_video_controls_options;
cui_video_controls_options cui_video_controls_defaults(void);
/* No callback => read-only. Requests never acknowledge host success. Sequence
 * saturates: UINT64_MAX makes requests read-only until caller retires this state.
 * Options/view are copied before callbacks; callbacks must retain UI/state.
 * Source/generation changes cancel local scrub presentation without issuing a
 * stale request. Hidden parts submit no semantics/hit targets. */
bool cui_video_controls(cui_context *, cui_id, const cui_theme *, cui_video_controls_state *,
                        cui_video_view, cui_video_controls_options);
enum { CUI_VIDEO_AUDIO_CAP_MUTE = 1u, CUI_VIDEO_AUDIO_CAP_VOLUME = 2u };
typedef struct {
    unsigned capabilities;
    bool muted;
    double volume; /* Confirmed gain [0, 1]; required when VOLUME is supported. */
} cui_video_audio_view;
typedef struct {
    cui_slider_state volume_slider;
    uint64_t source_id, generation, frame;
} cui_video_audio_state;
/* Pass NULL audio for legacy controls. Otherwise retain one zero-initialized
 * audio state per control and supply independent host capabilities. Unsupported
 * parts are omitted. Volume drag previews are local; release or a supported
 * key requests a COMMIT. Cancellation and omission discard the preview.
 * Mute requests use value.enabled (true means mute). Volume changes never
 * imply unmute. The host publishes confirmed state after applying requests. */
bool cui_video_controls_with_audio(cui_context *, cui_id, const cui_theme *,
                                   cui_video_controls_state *, cui_video_view,
                                   cui_video_controls_options, const cui_video_audio_view *,
                                   cui_video_audio_state *);
typedef struct {
    uint64_t source_id;
    cui_video_frame frame;
} cui_video_player_frame;
typedef struct {
    cui_video_surface_options surface;
    bool controls;
    cui_video_controls_options controls_options;
} cui_video_player_options;
/* Standard reusable 320x180 contained surface + compact controls. Includes
 * playback, +/-10s skip, timeline and capability-gated fullscreen; no rate or
 * volume by default. Set both surface geometry and controls width to resize.
 * No callback means read-only; host owns media acquisition/decoding/fullscreen. */
cui_video_player_options cui_video_player_defaults(void);
/* Null frame displays an empty surface-sized box (loading/error UI remains host
 * content). Mismatched source/generation fails before any UI submission. Frame
 * pixels stay borrowed through ALL renderer consumption; no lease is acquired. */
bool cui_video_player(cui_context *, cui_id, const cui_theme *, cui_video_controls_state *,
                      cui_video_view, const cui_video_player_frame *, cui_video_player_options);
/* Same player with optional confirmed audio controls. No device/codec ownership. */
bool cui_video_player_with_audio(cui_context *, cui_id, const cui_theme *,
                                 cui_video_controls_state *, cui_video_view,
                                 const cui_video_player_frame *, cui_video_player_options,
                                 const cui_video_audio_view *, cui_video_audio_state *);
#ifdef __cplusplus
}
#endif
#endif
