/* SPDX-License-Identifier: MIT */
#ifndef CUI_TIME_FIELD_V1_H
#define CUI_TIME_FIELD_V1_H
#include "cui_picker_events.h"
#include "cui_time_text.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Additive localized widget. Exact version/size and zero reserved fields.
 * State is caller-owned per widget/context; initialize explicitly. Borrowed
 * options/locale/names/theme/handler remain immutable throughout submission.
 * In the ordinary route each visible segment participates in global Tab.
 * Host clipboard commands are outside this widget's editing contract. */
#define CUI_TIME_FIELD_VERSION_V1 UINT32_C(1)
#define CUI_TIME_FIELD_NAME_MAX_BYTES_V1 64u
#define CUI_TIME_FIELD_PENDING_MAX_BYTES_V1 CUI_TIME_TOKEN_MAX_BYTES_V1

enum {
    CUI_TIME_FIELD_DISABLED_V1 = 1u << 0,
    CUI_TIME_FIELD_READONLY_V1 = 1u << 1,
    CUI_TIME_FIELD_OWNER_INVALID_V1 = 1u << 2,
    CUI_TIME_FIELD_IME_V1 = 1u << 3
};
typedef enum {
    CUI_TIME_FIELD_HOUR_V1 = 0,
    CUI_TIME_FIELD_MINUTE_V1 = 1,
    CUI_TIME_FIELD_SECOND_V1 = 2,
    CUI_TIME_FIELD_PERIOD_V1 = 3
} cui_time_field_segment_v1;
typedef struct {
    uint32_t struct_size, version, flags, reserved;
    cui_time_text_options_v1 text;
    uint64_t edit_generation;
    cui_str segment_names[4];
} cui_time_field_options_v1;
typedef struct {
    uint32_t struct_size, version, reserved[2];
    uint32_t segment, digit_count, draft, validation;
    uint32_t last_cycle, last_precision, pending_length, reserved_state;
    uint64_t edit_generation, last_frame_sequence;
    cui_time last_value;
    char pending[CUI_TIME_FIELD_PENDING_MAX_BYTES_V1];
    uint32_t reserved_tail;
} cui_time_field_state_v1;
typedef struct {
    cui_time_text_status_v1 status;
    bool changed;
    size_t change_count;
    size_t rejected_count;
    cui_time_text_status_v1 validation;
} cui_time_field_result_v1;

cui_time_text_status_v1 cui_time_field_options_init_v1(cui_time_field_options_v1 *out);
cui_time_text_status_v1 cui_time_field_state_init_v1(cui_time_field_state_v1 *out, cui_time value,
                                                     const cui_time_field_options_v1 *options);
cui_time_text_status_v1 cui_time_field_clear_validation_v1(cui_time_field_state_v1 *state);
cui_time_field_result_v1 cui_base_time_field_v1(cui_context *, cui_id, cui_str name,
                                                cui_time_field_state_v1 *, cui_time *,
                                                const cui_time_field_options_v1 *,
                                                cui_time_handler);
cui_time_field_result_v1 cui_time_field_v1(cui_context *, cui_id, const cui_theme *, cui_str name,
                                           cui_time_field_state_v1 *, cui_time *,
                                           const cui_time_field_options_v1 *, cui_time_handler);
/* Explicit sizes use the shared design scale at theme.font_size/16 density,
 * centered segments, input/selection colors and a separate field focus ring.
 * INHERIT uses cui_time_field_v1 appearance. Invalid sizes reject before
 * submission. Options, state, editing, focus routing and callback rules match
 * cui_time_field_v1. The control fits its localized content. */
cui_time_field_result_v1 cui_time_field_with_size_v1(cui_context *, cui_id, const cui_theme *,
                                                     cui_str name, cui_time_field_state_v1 *,
                                                     cui_time *, const cui_time_field_options_v1 *,
                                                     cui_design_size, cui_time_handler);
/* Optional ordered frame and one global Tab stop per field. Declare the
 * fields that this ordinary frame will submit, before input is dispatched.
 * Copies the entry array. State, value, options and token storage are borrowed
 * until cui_end_frame. Keep each state and value distinct and unchanged until its
 * matching submission applies edits.
 * Changing a field's options, visibility or enabled state takes effect at
 * the next begin. With CUI_TIME_FIELD_IME_V1, offer IME events through
 * cui_queue_text_input_event_v1 using the finalized segment owner and generation. */
#define CUI_TIME_FIELD_FRAME_MAX_V1 8u
typedef struct {
    cui_id id;
    cui_time_field_state_v1 *state;
    cui_time *value;
    const cui_time_field_options_v1 *options;
} cui_time_field_frame_entry_v1;
cui_time_text_status_v1 cui_begin_time_field_frame_v1(cui_context *, float width, float height,
                                                      float dt,
                                                      const cui_time_field_frame_entry_v1 *,
                                                      size_t count);
/* Submit each declared ID once. Base uses the same root and segment IDs as
 * the localized field. Styled paints the current selected segment.
 * Results and observers use the existing aggregate change contract. */
cui_time_field_result_v1 cui_base_time_field_from_frame_v1(cui_context *, cui_id, cui_str name,
                                                           cui_time_handler);
cui_time_field_result_v1 cui_time_field_from_frame_v1(cui_context *, cui_id, const cui_theme *,
                                                      cui_str name, cui_design_size,
                                                      cui_time_handler);
/* With CUI_TIME_FIELD_IME_V1, query after submitting this frame's widget.
 * Empty unless the focused segment has an active composition; if segment is
 * supplied, it receives that segment ID or zero. Text is borrowed from the
 * context until the next begin_frame or destruction. Copy to keep it longer.
 * Preedit uses one bounded context resource, up to CUI_TIME_TEXT_MAX_BYTES_V1
 * UTF-8 bytes. Commit accepts a formatted time or a token for one segment. */
cui_str cui_time_field_preedit_v1(cui_context *, cui_id, cui_id *segment);
/* changed is final value difference, independent of handler; change_count is
 * actual aggregate callbacks0/1, never accepted edits. Callback follows final
 * semantics/Base or paint/Styled. Copied CHANGE values survive teardown.
 * External owner/userdata changes are allowed; do not mutate active UI/model/
 * state/options/theme/token storage or recursively submit during the callback.
 * Validation errors reject values without clamping. Owner out-of-domain values
 * render invalid; minute precision rejects hidden seconds. Omission/mode/owner
 * changes clear pending input on the next ordinary writable submission.
 * Full formatted TEXT up to160 bytes is atomic; Core copies queued text_data.
 * No pointers are retained. Ordinary ready, painted writable frames edit only. */
#ifdef __cplusplus
}
#endif
#endif
