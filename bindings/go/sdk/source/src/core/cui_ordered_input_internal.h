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
#endif
