/* SPDX-License-Identifier: MIT */
#include "cui/cui_component_events.h"
#include <assert.h>
#include <stdio.h>

static void scene(cui_context *ui) {
    assert(cui_begin_frame(ui, 160, 80, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 160, .height = 80});
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 80, 30});
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
}
static void finish(cui_context *ui) {
    cui_end_box(ui);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
}
static cui_component_event_metadata accepted(cui_context *ui, size_t index) {
    cui_activation_snapshot snapshot;
    assert(cui_event_activation_snapshot(ui, index, &snapshot));
    cui_component_event_metadata metadata;
    assert(cui_component_event_from_activation(&metadata, CUI_COMPONENT_EVENT_ACTIVATE, &snapshot));
    assert(metadata.flags == (CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX |
                              CUI_COMPONENT_EVENT_HAS_SOURCE | CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    assert(metadata.target == 10 && metadata.event_index == index);
    assert(metadata.frame_sequence == cui_frame_sequence(ui));
    assert(memcmp(&metadata.activation, &snapshot, sizeof snapshot) == 0);
    assert(cui_consume_event(ui, index));
    /* Consumption rules differ by input channel; retained metadata is independent. */
    assert(metadata.activation.target == 10);
    return metadata;
}
static void metadata_test(void) {
    cui_component_event_metadata value;
    assert(cui_component_event_init(&value, CUI_COMPONENT_EVENT_LIFECYCLE, 20,
                                    CUI_COMPONENT_EVENT_SOURCE_UNKNOWN));
    assert(value.flags == 0 && value.frame_sequence == 0 && value.event_index == 0);
    assert(!value.activation.target && !value.activation.has_press);
    assert(cui_component_event_init(&value, CUI_COMPONENT_EVENT_REQUEST, 20,
                                    CUI_COMPONENT_EVENT_SOURCE_PROGRAMMATIC));
    assert(value.flags == CUI_COMPONENT_EVENT_HAS_SOURCE);
    assert(!value.activation.target && value.frame_sequence == 0);
    cui_component_event_metadata before = value;
    assert(!cui_component_event_init(&value, (cui_component_event_kind)-1, 20,
                                     CUI_COMPONENT_EVENT_SOURCE_UNKNOWN));
    assert(!cui_component_event_init(&value, CUI_COMPONENT_EVENT_CHANGE, 0,
                                     CUI_COMPONENT_EVENT_SOURCE_UNKNOWN));
    assert(!cui_component_event_init(&value, CUI_COMPONENT_EVENT_CHANGE, 20,
                                     (cui_component_event_source)99));
    assert(!cui_component_event_init(NULL, CUI_COMPONENT_EVENT_COMMIT, 20,
                                     CUI_COMPONENT_EVENT_SOURCE_UNKNOWN));
    assert(!cui_component_event_set_position(&value, 0, 0));
    assert(!cui_component_event_set_position(&value, 1, SIZE_MAX));
    assert(memcmp(&value, &before, sizeof value) == 0);
    assert(cui_component_event_set_position(&value, UINT64_MAX, 0));
    assert(value.frame_sequence == UINT64_MAX && value.event_index == 0);
    assert(!(value.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    cui_activation_snapshot bad = {.target = 10, .frame_sequence = 1,
                                    .event_type = CUI_EVENT_TEXT};
    before = value;
    assert(!cui_component_event_from_activation(&value, CUI_COMPONENT_EVENT_SUBMIT, &bad));
    assert(!cui_component_event_from_activation(&value, CUI_COMPONENT_EVENT_SUBMIT, NULL));
    bad.event_type = CUI_EVENT_KEY_DOWN;
    bad.target = 0;
    assert(!cui_component_event_from_activation(&value, CUI_COMPONENT_EVENT_SUBMIT, &bad));
    bad.target = 10;
    bad.frame_sequence = 0;
    assert(!cui_component_event_from_activation(&value, CUI_COMPONENT_EVENT_ACTIVATE, &bad));
    bad.frame_sequence = 1;
    bad.event_index = SIZE_MAX;
    assert(!cui_component_event_from_activation(&value, CUI_COMPONENT_EVENT_ACTIVATE, &bad));
    assert(!cui_component_event_from_activation(NULL, CUI_COMPONENT_EVENT_ACTIVATE, &bad));
    assert(!cui_component_event_set_position(NULL, 1, 0));
    assert(memcmp(&value, &before, sizeof value) == 0);
    for (int kind = CUI_COMPONENT_EVENT_ACTIVATE; kind <= CUI_COMPONENT_EVENT_LIFECYCLE; ++kind) {
        assert(cui_component_event_init(&value, (cui_component_event_kind)kind, 10,
                                        CUI_COMPONENT_EVENT_SOURCE_UNKNOWN));
        assert(value.kind == (cui_component_event_kind)kind && value.flags == 0);
    }

    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    scene(ui);
    finish(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 20, .y = 20,
                                          .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 42,
                                          .pointer_device = 5}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 24, .y = 20,
                                          .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 42,
                                          .pointer_device = 5, .click_count = 2}));
    scene(ui);
    cui_component_event_metadata pointer = accepted(ui, 1);
    assert(pointer.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(pointer.activation.has_press && pointer.activation.has_release);
    assert(pointer.activation.press.contact == 42 && pointer.activation.release.device == 5);
    assert(pointer.activation.release.click_count == 2);
    before = pointer;
    assert(!cui_component_event_set_position(&pointer, pointer.frame_sequence + 1, 1));
    assert(memcmp(&pointer, &before, sizeof pointer) == 0);
    assert(cui_component_event_set_position(&pointer, pointer.frame_sequence, 1));
    finish(ui);
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER,
                                          .modifiers = CUI_MOD_ALT}));
    scene(ui);
    cui_component_event_metadata keyboard = accepted(ui, 0);
    assert(keyboard.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(keyboard.activation.has_key_bounds && !keyboard.activation.has_press);
    assert(keyboard.activation.key == CUI_KEY_ENTER && keyboard.activation.modifiers == CUI_MOD_ALT);
    finish(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                          .accessibility_target = 10,
                                          .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    scene(ui);
    cui_component_event_metadata accessibility = accepted(ui, 0);
    assert(accessibility.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(!accessibility.activation.has_press && !accessibility.activation.has_release &&
           !accessibility.activation.has_key_bounds);
    finish(ui);
    cui_destroy(ui);
    assert(pointer.activation.press.x == 20 && pointer.activation.release.x == 24);
    assert(keyboard.activation.key_bounds.x == 10 && accessibility.target == 10);
}
static void rejected_copy(cui_str source, size_t limit, size_t capacity,
                          cui_component_event_copy_status expected) {
    char buffer[16] = "unchanged";
    char before[sizeof buffer];
    memcpy(before, buffer, sizeof buffer);
    cui_str out = {"sentinel", 8};
    assert(cui_component_event_copy_utf8(source, limit, buffer, capacity, &out) == expected);
    assert(memcmp(buffer, before, sizeof buffer) == 0);
    assert(out.data != buffer && out.length == 8);
}
static void copy_test(void) {
    const char text[] = "A\xc3\xa9\xf0\x9f\x98\x80";
    char buffer[sizeof text];
    cui_str out;
    assert(cui_component_event_copy_utf8((cui_str){text, sizeof text - 1}, sizeof text - 1,
                                          buffer, sizeof buffer, &out) == CUI_COMPONENT_EVENT_COPY_OK);
    assert(out.data == buffer && out.length == sizeof text - 1 && !strcmp(buffer, text));
    assert(cui_component_event_copy_utf8((cui_str){NULL, 0}, 0, buffer, 1, &out) ==
           CUI_COMPONENT_EVENT_COPY_OK);
    assert(!buffer[0] && out.length == 0);
    rejected_copy((cui_str){text, sizeof text - 1}, sizeof text - 2, sizeof buffer,
                  CUI_COMPONENT_EVENT_COPY_LIMIT);
    rejected_copy((cui_str){text, sizeof text - 1}, SIZE_MAX, sizeof text - 1,
                  CUI_COMPONENT_EVENT_COPY_CAPACITY);
    rejected_copy((cui_str){NULL, 1}, 1, 2, CUI_COMPONENT_EVENT_COPY_INVALID_ARGUMENT);
    rejected_copy((cui_str){NULL, 0}, 0, 0, CUI_COMPONENT_EVENT_COPY_CAPACITY);
    /* Must reject impossible sizes/over-limit before inspecting source bytes. */
    rejected_copy((cui_str){"x", SIZE_MAX}, SIZE_MAX, SIZE_MAX, CUI_COMPONENT_EVENT_COPY_LIMIT);
    const char *invalid[] = {"\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\x80", "\xe2\x82"};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i)
        rejected_copy((cui_str){invalid[i], strlen(invalid[i])}, 16, 16,
                      CUI_COMPONENT_EVENT_COPY_INVALID_UTF8);
    rejected_copy((cui_str){"a\0b", 3}, 3, 4, CUI_COMPONENT_EVENT_COPY_INVALID_UTF8);
    char overlap[16] = "hello";
    assert(cui_component_event_copy_utf8((cui_str){overlap, 5}, 5, overlap + 1, 15, &out) ==
           CUI_COMPONENT_EVENT_COPY_OK);
    assert(out.data == overlap + 1 && !strcmp(out.data, "hello"));
    assert(cui_component_event_copy_utf8((cui_str){overlap + 1, 5}, 5, overlap, 16, &out) ==
           CUI_COMPONENT_EVENT_COPY_OK);
    assert(!strcmp(overlap, "hello"));
    assert(cui_component_event_copy_utf8((cui_str){"", 0}, 0, NULL, 0, &out) ==
           CUI_COMPONENT_EVENT_COPY_INVALID_ARGUMENT);
    assert(cui_component_event_copy_utf8((cui_str){"", 0}, 0, buffer, sizeof buffer, NULL) ==
           CUI_COMPONENT_EVENT_COPY_INVALID_ARGUMENT);
}
int main(void) {
    metadata_test();
    copy_test();
    puts("component metadata: actual pointer/key/accessibility evidence and bounded UTF-8 copies PASS");
    return 0;
}
