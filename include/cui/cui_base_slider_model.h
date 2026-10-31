/* SPDX-License-Identifier: MIT */
#ifndef CUI_BASE_SLIDER_MODEL_H
#define CUI_BASE_SLIDER_MODEL_H
#include "cui_component_events.h"
#include "cui_slider_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Unreleased explicit-size Base route; independent of every legacy Slider API.
 * Owned records use exact v1 sizes/tags. No Styled palette, host or binding API. */
enum {
    CUI_SLIDER_MODEL_BEGIN,
    CUI_SLIDER_MODEL_UPDATE,
    CUI_SLIDER_MODEL_COMMIT,
    CUI_SLIDER_MODEL_CANCEL
};
enum {
    CUI_SLIDER_MODEL_INPUT,
    CUI_SLIDER_MODEL_DISABLED,
    CUI_SLIDER_MODEL_READONLY,
    CUI_SLIDER_MODEL_ZERO_SPAN,
    CUI_SLIDER_MODEL_SOURCE_REPLACED,
    CUI_SLIDER_MODEL_DOMAIN_CHANGED,
    CUI_SLIDER_MODEL_MODE_CHANGED,
    CUI_SLIDER_MODEL_AXIS_CHANGED,
    CUI_SLIDER_MODEL_OMITTED,
    CUI_SLIDER_MODEL_CAPTURE_LOST
};
enum {
    CUI_SLIDER_MODEL_DISABLED_BIT = 1u,
    CUI_SLIDER_MODEL_READONLY_BIT = 2u,
    CUI_SLIDER_MODEL_REVERSE_FILL_BIT = 4u
};
typedef struct {
    uint32_t version, struct_size, flags, orientation;
    uint64_t source_generation;
    cui_slider_model_domain_v1 domain;
    cui_str name, start_name, end_name, start_description, end_description;
} cui_slider_model_options_v1;
typedef struct {
    uint32_t version, struct_size;
    float length, cross_extent, track_extent, thumb_extent, axis_inset;
    uint32_t reserved;
    cui_box root, track, fill, start_thumb, end_thumb; /* appearance only; Single ignores start */
} cui_base_slider_model_layout_v1;
enum { CUI_SLIDER_MODEL_HAS_INPUT = 1u, CUI_SLIDER_MODEL_HAS_NUMERIC_VALUE = 2u };
typedef struct {
    uint32_t type, flags, key, modifiers;
    float x, y;
    uint32_t repeat, pointer_type, pointer_button, accessibility_action;
    uint64_t pointer_id, pointer_device;
    cui_id accessibility_target;
    double accessibility_value;
} cui_slider_model_input_v1;
typedef struct {
    uint32_t version, struct_size, phase, reason;
    uint32_t thumb, reserved[3];
    cui_component_event_metadata event;
    uint64_t submission_frame, request_sequence, source_generation;
    cui_slider_model_value_v1 current, requested;
    cui_slider_model_domain_v1 domain, gesture_domain;
    cui_slider_model_input_v1 input;
} cui_slider_model_request_v1;
typedef void (*cui_slider_model_request_fn_v1)(void *, const cui_slider_model_request_v1 *);
typedef struct {
    cui_slider_model_request_fn_v1 callback;
    void *user;
} cui_slider_model_handler_v1;
enum {
    CUI_SLIDER_MODEL_OK,
    CUI_SLIDER_MODEL_INVALID_ARGUMENT,
    CUI_SLIDER_MODEL_INVALID_STATE,
    CUI_SLIDER_MODEL_SEQUENCE_EXHAUSTED,
    CUI_SLIDER_MODEL_CORE_FAILURE
};
enum {
    CUI_SLIDER_MODEL_VALID = 1u,
    CUI_SLIDER_MODEL_SUBMITTED = 2u,
    CUI_SLIDER_MODEL_COUNT_OVERFLOW = 4u
};
typedef struct {
    uint32_t version, struct_size, status, flags;
    uint64_t submission_frame, begin_count, request_count, commit_count, cancel_count;
} cui_slider_model_result_v1;
enum {
    CUI_SLIDER_MODEL_ROOT,
    CUI_SLIDER_MODEL_TRACK,
    CUI_SLIDER_MODEL_FILL,
    CUI_SLIDER_MODEL_START_THUMB,
    CUI_SLIDER_MODEL_END_THUMB
};
/* Pure identity, never current-frame presence. Zero/unknown input returns0.
 * ROOT=owner; Single omits START_THUMB. Same derivation is used by composition. */
cui_id cui_base_slider_model_part_id_v1(cui_id owner, uint32_t part);
/* Owns its root/parts. Invalid admission leaves state/owner/callbacks/UI unchanged.
 * Fixed positive extents, cross>=max(track,thumb), 0<=inset<length/2. Only box
 * appearance fields participate; all layout fields are inactive. See guide.
 * Requests never accept owner values. current is frozen entry; requested intent
 * evolves in original order. Callback/user/strings borrowed through return;
 * callback may mutate external owner/userdata and enqueue application work, but
 * not active UI/state/options/layout/string storage or recursively submit.
 * Current paint follows ongoing preview only; terminal/blocked calls paint entry.
 * No-handler is read-only. Inert/layout-only/recovery calls do not advance state.
 * Track down retains Core root capture and explicitly focuses chosen thumb
 * (focus_visible=true); ROOT-key delegation preserves actual ROOT metadata.
 * No whole-frame rollback after a later Core failure. Full contract in guide. */
cui_slider_model_result_v1 cui_base_slider_model_events_v1(cui_context *, cui_id owner,
                                                           cui_slider_model_value_v1 confirmed,
                                                           cui_slider_model_state_v1 *,
                                                           const cui_slider_model_options_v1 *,
                                                           const cui_base_slider_model_layout_v1 *,
                                                           cui_slider_model_handler_v1);
#ifdef __cplusplus
}
#endif
#endif
