/* SPDX-License-Identifier: MIT */
#ifndef CUI_ORDERED_INPUT_INTERNAL_H
#define CUI_ORDERED_INPUT_INTERNAL_H
#include "cui/cui.h"

enum { CUI_ORDERED_INPUT_MAX = 8 };
typedef struct {
    bool consume, tab_moved;
} cui_core_ordered_result;
typedef struct {
    cui_id root;
    cui_core_ordered_result (*step)(void *user, const cui_event *, cui_id target);
    void *user;
    bool disabled;
    void (*blur)(void *user);
    cui_id (*text_owner)(void *user);
    /* Select a currently visible child for an already qualified Focus offer.
     * Return false without mutation when the current options exclude it. */
    bool (*focus_child)(void *user, cui_id child);
} cui_core_ordered_input;
/* Reserve context-owned typed storage before dispatch. The step callbacks and
 * their event records are borrowed only inside begin. Steps may edit the
 * supplied working storage and do not call application or CUI code. */
void *cui_core_ordered_reserve(cui_context *, size_t bytes);
bool cui_core_begin_ordered_frame(cui_context *, float width, float height, float dt,
                                  const cui_core_ordered_input *, size_t count);
void *cui_core_ordered_data(cui_context *);
uint64_t cui_core_completed_sequence(const cui_context *);
void cui_core_ordered_publish(cui_context *, cui_id);
void cui_core_ordered_observing(cui_context *, bool);

typedef enum {
    CUI_ORDERED_TAB_UNRESOLVED,
    CUI_ORDERED_TAB_INSIDE,
    CUI_ORDERED_TAB_LEAVE
} cui_core_ordered_tab;
/* One host Tab can be tagged in the queued input for the next ordered frame.
 * Only traversal at an open edge requests leaving the host's scene. Read the
 * decision after completing that frame, before the host's default action. */
bool cui_core_ordered_queue_tab(cui_context *, bool reverse);
cui_core_ordered_tab cui_core_ordered_tab_result(const cui_context *);
/* Register the selected editable child of the currently focused ordered root.
 * Only a submitted delegated text control can become the finalized host owner. */
void cui_core_ordered_text_segment(cui_context *, cui_id root, cui_id segment);
/* Copy the preceding resource into isolated working state before dispatch. */
const void *cui_core_ordered_retained(const cui_context *, cui_id owner, cui_id namespace_id);
#endif
