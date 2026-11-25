/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_DATE_SEMANTIC_DETAILS_H
#define CUI_TEST_DATE_SEMANTIC_DETAILS_H
#include "cui/cui.h"
#include <string.h>

/* Compare fields and borrowed string contents across independent contexts. */
// clang-format off
#define CUI_TEST_DATE_DETAILS_SCALARS(X) \
    X(expanded) X(selected) X(readonly) X(multiline) X(required) X(invalid) \
    X(multiselectable) X(busy) X(scrollable) X(orientation) X(controls) X(labelled_by) \
    X(parent) X(focus_target) X(minimum) X(maximum) X(level) X(pos_in_set) X(set_size) \
    X(row_index) X(column_index) X(row_count) X(column_count) X(scroll_x) X(scroll_y) \
    X(scroll_max_x) X(scroll_max_y) X(scroll_axis_lock) X(scroll_trap_horizontal) \
    X(scroll_gesture_lock) X(focusable) X(pointer_activatable) X(handles_tab) X(cursor) \
    X(no_focus) X(gesture_pan) X(gesture_zoom) X(touch_drag_priority) X(touch_scroll_disabled) \
    X(no_pointer_focus) X(blocks_pointer_activation) X(secret) X(password) \
    X(text_input_configured) X(text_input_config.type) X(text_input_config.purpose) \
    X(release_key_activation)
// clang-format on
static bool details_equal(const cui_semantic_details *a, const cui_semantic_details *b) {
#define SAME(field)                                                                                \
    if (a->field != b->field)                                                                      \
        return false;
    CUI_TEST_DATE_DETAILS_SCALARS(SAME)
#undef SAME
    return a->description.length == b->description.length &&
           (!a->description.length ||
            !memcmp(a->description.data, b->description.data, a->description.length));
}
#endif
