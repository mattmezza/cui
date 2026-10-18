/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_ACTIVATION_ASSERT_H
#define CUI_TEST_ACTIVATION_ASSERT_H
#include "cui/cui.h"
#include <assert.h>
static void assert_pointer_sample_equal(cui_pointer_activation_sample a,
                                        cui_pointer_activation_sample b) {
    assert(a.x == b.x && a.y == b.y);
    assert(a.modifiers == b.modifiers && a.click_count == b.click_count);
    assert(a.type == b.type && a.button == b.button);
    assert(a.contact == b.contact && a.device == b.device);
}
static void assert_activation_equal(cui_activation_snapshot a, cui_activation_snapshot b) {
    /* Aggregate copies need not preserve padding bytes. Compare every field. */
    assert(a.target == b.target && a.frame_sequence == b.frame_sequence);
    assert(a.event_index == b.event_index && a.event_type == b.event_type);
    assert(a.key == b.key && a.modifiers == b.modifiers);
    assert(a.has_press == b.has_press && a.has_release == b.has_release &&
           a.has_key_bounds == b.has_key_bounds);
    assert_pointer_sample_equal(a.press, b.press);
    assert_pointer_sample_equal(a.release, b.release);
    assert(a.key_bounds.x == b.key_bounds.x && a.key_bounds.y == b.key_bounds.y &&
           a.key_bounds.w == b.key_bounds.w && a.key_bounds.h == b.key_bounds.h);
}
#endif
