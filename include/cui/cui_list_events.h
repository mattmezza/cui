/* SPDX-License-Identifier: MIT */
#ifndef CUI_LIST_EVENTS_H
#define CUI_LIST_EVENTS_H
#include "cui_choice.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    cui_selection_event_result events;
    bool legacy_activated;
} cui_list_event_result;
/* Per-transition CHANGE after legacy activate/confirm, before final semantics/
 * Styled paint. Same-item confirmation is activation, not necessarily CHANGE.
 * Callback/user borrowed synchronously; missing callback ignores user and reduces.
 * Do not mutate/rebind owner, state, descriptors or borrowed resources, recurse,
 * destroy storage/context or unwind foreign exceptions in typed callbacks.
 * Copy pointer-free payloads or change unrelated application state; defer model
 * edits/removal/teardown until submission and renderer consumption finish.
 * Existing legacy callbacks retain their old timing and owner-update behavior. */
cui_list_event_result cui_base_list_events(cui_context *, cui_id control,
    const cui_choice_item *, size_t count, cui_id *selected,
    cui_list_state *, cui_list_options, cui_selection_handler);
cui_list_event_result cui_list_events(cui_context *, cui_id container,
    const cui_theme *, const cui_choice_item *, size_t count, cui_id *selected,
    cui_list_state *, cui_list_options, cui_selection_handler);
#ifdef __cplusplus
}
#endif
#endif
