/* SPDX-License-Identifier: MIT */
#ifndef CUI_TIME_FIELD_V1_INTERNAL_H
#define CUI_TIME_FIELD_V1_INTERNAL_H
#include "cui/cui_time_field_v1.h"
bool cui_time_field_v1_equal(cui_time, cui_time);
cui_str cui_time_field_v1_name(const cui_time_field_options_v1 *, unsigned);
cui_time_field_result_v1 cui_time_field_v1_compose(cui_context *, cui_id, cui_str,
                                                   cui_time_field_state_v1 *, cui_time *,
                                                   const cui_time_field_options_v1 *);
void cui_time_field_v1_observe(cui_context *, cui_id, cui_time, cui_time *,
                               const cui_time_field_options_v1 *, cui_time_handler,
                               cui_time_field_result_v1 *);
#endif
