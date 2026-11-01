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
 * All visible segments use ordinary global Tab, not source one-root focus.
 * No IME composition/clipboard shortcut or foreign binding support is claimed. */
#define CUI_TIME_FIELD_VERSION_V1 UINT32_C(1)
#define CUI_TIME_FIELD_NAME_MAX_BYTES_V1 64u
#define CUI_TIME_FIELD_PENDING_MAX_BYTES_V1 CUI_TIME_TOKEN_MAX_BYTES_V1

enum {
    CUI_TIME_FIELD_DISABLED_V1 = 1u << 0,
    CUI_TIME_FIELD_READONLY_V1 = 1u << 1,
    CUI_TIME_FIELD_OWNER_INVALID_V1 = 1u << 2
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
