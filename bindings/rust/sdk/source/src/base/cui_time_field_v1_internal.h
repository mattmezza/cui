/* SPDX-License-Identifier: MIT */
#ifndef CUI_TIME_FIELD_V1_INTERNAL_H
#define CUI_TIME_FIELD_V1_INTERNAL_H
#include "cui/cui_time_field_v1.h"
typedef struct {
    cui_id segment, root;
    uint64_t frame, generation;
    size_t length;
    bool awaiting_commit;
    char text[CUI_TIME_TEXT_MAX_BYTES_V1];
} cui_tfw_preedit;
typedef struct {
    cui_time_field_frame_entry_v1 entry;
    cui_time_field_state_v1 state;
    cui_time before, value;
    size_t rejected;
    cui_tfw_preedit preedit;
    bool submitted, text_current;
} cui_tfw_frame_field;
cui_tfw_frame_field *cui_tfw_frame_start(cui_context *, cui_id, cui_str);
cui_time_field_result_v1 cui_tfw_frame_apply(cui_context *, cui_tfw_frame_field *,
                                             cui_time_handler);
bool cui_time_field_v1_equal(cui_time, cui_time);
cui_str cui_time_field_v1_name(const cui_time_field_options_v1 *, unsigned);
cui_time_field_result_v1 cui_time_field_v1_compose(cui_context *, cui_id, cui_str,
                                                   cui_time_field_state_v1 *, cui_time *,
                                                   const cui_time_field_options_v1 *);
void cui_time_field_v1_observe(cui_context *, cui_id, cui_time, cui_time *,
                               const cui_time_field_options_v1 *, cui_time_handler,
                               cui_time_field_result_v1 *);
#endif
