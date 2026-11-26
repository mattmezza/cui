/* SPDX-License-Identifier: MIT */
#ifndef CUI_PAGINATION_INTERNAL_H
#define CUI_PAGINATION_INTERNAL_H
#include "cui/cui_numeric_selection_events.h"
typedef struct {
    size_t page, input_index; /* Zero page means no menu selection. */
} cui_pagination_jump;
bool cui_base_pagination_reduce_internal(cui_context *, cui_id, size_t *, size_t total,
                                         cui_pagination_options, cui_pagination_jump);
cui_numeric_selection_event_result
cui_base_pagination_observe_internal(cui_context *, cui_id, size_t *, size_t total,
                                     cui_pagination_options, cui_pagination_jump,
                                     cui_pagination_event_handler);
#endif
