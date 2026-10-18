/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
/* Independent copy of the shipped V1 storage, including native padding. */
typedef union { uint32_t boolean_value; double scalar_value; cui_bridge_node selected;
    struct { uint32_t reason, row, column, sort_column, sort_direction; } table;
    uint32_t surface_result; } frozen_value;
typedef struct {
    uint32_t struct_size, schema_version, kind, payload_kind, flags, node_kind;
    cui_bridge_node node; uint64_t frame_sequence; uint32_t text_length;
    frozen_value payload; cui_bridge_event_ex legacy;
} frozen_notification;
_Static_assert(sizeof(frozen_notification) == sizeof(cui_bridge_notification_v1), "V1 layout changed");
_Static_assert(offsetof(frozen_notification, legacy) == offsetof(cui_bridge_notification_v1, legacy), "V1 offset changed");
static cui_bridge_app *create(uint32_t events, cui_bridge_node *root) {
    cui_bridge_options options = {.struct_size = sizeof options, .abi_version = CUI_BRIDGE_ABI_VERSION,
        .backend = CUI_BRIDGE_BACKEND_NONE, .width = 800, .height = 800, .max_events = events};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", root) == CUI_BRIDGE_OK);
    return app;
}
static cui_bridge_node node(cui_bridge_app *app, cui_bridge_node root, cui_bridge_kind kind) {
    cui_bridge_node result; char key[32];
    (void)snprintf(key, sizeof key, "control-%u", (unsigned)kind);
    assert(cui_bridge_node_create(app, root, kind, key, &result) == CUI_BRIDGE_OK);
    return result;
}
static void click(cui_bridge_app *app, cui_bridge_rect b) {
    assert(b.width > 0 && b.height > 0);
    float x = b.x + b.width / 2, y = b.y + b.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
}
static void target(cui_bridge_app *app, cui_bridge_node control) {
    cui_bridge_rect b; assert(cui_bridge_node_bounds(app, control, &b) == CUI_BRIDGE_OK); click(app, b);
}
static cui_bridge_notification_v1 poll(cui_bridge_app *app, uint32_t mask, bool old) {
    cui_bridge_notification_v1 value; char text[4097]; uint32_t required;
    cui_bridge_status status = old ? cui_bridge_poll_notification_v1(app, sizeof value, &value,
        text, sizeof text, &required) : cui_bridge_poll_value_notification_v1(app, sizeof value,
        mask, &value, text, sizeof text, &required);
    assert(status == CUI_BRIDGE_OK && !text[0] && required == 1 && !value.text_length);
    assert(value.struct_size == sizeof value && value.schema_version == 1);
    return value;
}
static void empty(cui_bridge_app *app) {
    cui_bridge_notification_v1 output, before; memset(&output, 0xa5, sizeof output); before = output;
    char text[8] = "same"; uint32_t required = 42;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof output, CUI_BRIDGE_VALUE_SCALAR_V1,
        &output, text, sizeof text, &required) == CUI_BRIDGE_EMPTY);
    assert(!memcmp(&output, &before, sizeof output) && required == 42 && !strcmp(text, "same"));
}
static void number_step(cui_bridge_app *app, cui_bridge_node control, cui_bridge_status expected) {
    target(app, control);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == expected);
}
static void number_and_protocol(void) {
    cui_bridge_node root; cui_bridge_app *app = create(16, &root);
    cui_bridge_node number = node(app, root, CUI_BRIDGE_NUMBER_INPUT);
    assert(cui_bridge_set_range(app, number, 0, 100000000, .25) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, number, 16777216.25) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); empty(app);
    number_step(app, number, CUI_BRIDGE_OK);
    /* This is a real key mutation, not a setter-generated event. */
    cui_bridge_notification_v1 output, before; memset(&output, 0xa5, sizeof output); before = output;
    char text[8] = "same"; uint32_t required = 42;
    uint32_t masks[] = {1u << CUI_BRIDGE_PAYLOAD_LEGACY, UINT32_MAX};
    for (size_t i = 0; i < sizeof masks / sizeof masks[0]; i++) {
        assert(cui_bridge_poll_value_notification_v1(app, sizeof output, masks[i], &output,
            text, sizeof text, &required) == CUI_BRIDGE_INVALID_ARGUMENT);
        assert(!memcmp(&output, &before, sizeof output) && required == 42 && !strcmp(text, "same"));
    }
    assert(cui_bridge_poll_value_notification_v1(app, sizeof output - 1, CUI_BRIDGE_VALUE_SCALAR_V1,
        &output, text, sizeof text, &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_poll_value_notification_v1(app, sizeof output, CUI_BRIDGE_VALUE_SCALAR_V1,
        NULL, text, sizeof text, &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_poll_value_notification_v1(app, sizeof output, CUI_BRIDGE_VALUE_SCALAR_V1,
        &output, text, sizeof text, NULL) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(&output, &before, sizeof output) && required == 42 && !strcmp(text, "same"));
    assert(cui_bridge_poll_value_notification_v1(app, sizeof output, CUI_BRIDGE_VALUE_SCALAR_V1,
        &output, text, 0, &required) == CUI_BRIDGE_CAPACITY);
    assert(required == 1 && !memcmp(&output, &before, sizeof output) && !strcmp(text, "same"));
    assert(cui_bridge_set_value(app, number, 3) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_value_notification_v1(app, sizeof output, CUI_BRIDGE_VALUE_SCALAR_V1,
        &output, text, 1, &required) == CUI_BRIDGE_OK && required == 1 && !text[0]);
    assert(output.kind == CUI_BRIDGE_NOTIFY_CHANGE && output.payload_kind == CUI_BRIDGE_PAYLOAD_SCALAR);
    assert(output.node_kind == CUI_BRIDGE_NUMBER_INPUT && output.payload.scalar_value == 16777216.5);
    assert(output.legacy.value == output.payload.scalar_value && output.flags == CUI_BRIDGE_NOTIFY_HAS_FRAME);
    assert(output.node.slot == number.slot && output.legacy.node.generation == number.generation);
    empty(app);
    number_step(app, number, CUI_BRIDGE_OK); output = poll(app, 0, false);
    assert(output.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && output.legacy.value == 3.25);
    number_step(app, number, CUI_BRIDGE_OK); output = poll(app, 0, true);
    assert(output.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && output.legacy.value == 3.5);
    frozen_value zero; memset(&zero, 0, sizeof zero);
    assert(!memcmp(&output.payload, &zero, sizeof zero) && !output.legacy.checked &&
        !output.legacy.selected.slot && !output.legacy.selected.generation && !output.legacy.text_length);
    assert(cui_bridge_set_disabled(app, number, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    number_step(app, number, CUI_BRIDGE_OK); empty(app);
    cui_bridge_destroy(app); assert(output.legacy.value == 3.5);
}
static void other_scalars(void) {
    cui_bridge_node root; cui_bridge_app *app = create(16, &root);
    cui_bridge_node slider = node(app, root, CUI_BRIDGE_SLIDER);
    cui_bridge_node rating = node(app, root, CUI_BRIDGE_RATING);
    cui_bridge_node page = node(app, root, CUI_BRIDGE_PAGINATION);
    cui_bridge_node progress = node(app, root, CUI_BRIDGE_PROGRESS);
    assert(cui_bridge_set_value(app, slider, .2) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, rating, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, page, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, progress, .5) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); empty(app);
    target(app, slider); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_notification_v1 value = poll(app, CUI_BRIDGE_VALUE_SCALAR_V1, false);
    assert(value.node_kind == CUI_BRIDGE_SLIDER && value.payload_kind == CUI_BRIDGE_PAYLOAD_SCALAR);
    assert(value.payload.scalar_value == (double)(float)value.payload.scalar_value);
    assert(value.payload.scalar_value == value.legacy.value && value.payload.scalar_value != .2);
    cui_bridge_rect b;
    assert(cui_bridge_numeric_part_bounds(app, rating, 12, &b) == CUI_BRIDGE_OK); click(app, b);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    value = poll(app, CUI_BRIDGE_VALUE_SCALAR_V1, false);
    assert(value.node_kind == CUI_BRIDGE_RATING && value.payload.scalar_value == 3 && value.legacy.value == 3);
    assert(cui_bridge_numeric_part_bounds(app, page, 3, &b) == CUI_BRIDGE_OK); click(app, b);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    value = poll(app, CUI_BRIDGE_VALUE_SCALAR_V1, false);
    assert(value.node_kind == CUI_BRIDGE_PAGINATION && value.payload.scalar_value == 2 && value.legacy.value == 2);
    empty(app); cui_bridge_destroy(app);
}
static void overflow_and_interleave(void) {
    cui_bridge_node root; cui_bridge_app *app = create(2, &root);
    cui_bridge_node number = node(app, root, CUI_BRIDGE_NUMBER_INPUT);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    number_step(app, number, CUI_BRIDGE_OK); number_step(app, number, CUI_BRIDGE_OK);
    number_step(app, number, CUI_BRIDGE_CAPACITY);
    double current; assert(cui_bridge_get_value(app, number, &current) == CUI_BRIDGE_OK && current == 3);
    cui_bridge_event_ex legacy; char text[16]; uint32_t required;
    assert(cui_bridge_poll_event_ex(app, &legacy, text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(legacy.value == 1);
    cui_bridge_notification_v1 value = poll(app, CUI_BRIDGE_VALUE_SCALAR_V1, false);
    assert(value.payload.scalar_value == 2 && value.frame_sequence > legacy.frame_sequence);
    empty(app);
    number_step(app, number, CUI_BRIDGE_OK);
    cui_bridge_node button = node(app, root, CUI_BRIDGE_BUTTON);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); target(app, button);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event click_event;
    assert(cui_bridge_poll_event(app, &click_event) == CUI_BRIDGE_OK && click_event.node.slot == button.slot);
    empty(app); cui_bridge_destroy(app);
}
static void non_scalar_and_hidden(void) {
    cui_bridge_node root; cui_bridge_app *app = create(16, &root);
    cui_bridge_node check = node(app, root, CUI_BRIDGE_CHECKBOX);
    cui_bridge_node input = node(app, root, CUI_BRIDGE_INPUT);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    target(app, check); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    target(app, input); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "plain UTF-8: \xc3\xa9") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_notification_v1 value = poll(app, CUI_BRIDGE_VALUE_SCALAR_V1, false);
    assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_BOOLEAN && value.payload.boolean_value == 1);
    char text[64]; uint32_t required;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, CUI_BRIDGE_VALUE_SCALAR_V1,
        &value, text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && !strcmp(text, "plain UTF-8: \xc3\xa9"));
    empty(app);
    cui_bridge_node panel = node(app, root, CUI_BRIDGE_COLLAPSIBLE);
    cui_bridge_node number = node(app, panel, CUI_BRIDGE_NUMBER_INPUT);
    assert(cui_bridge_set_checked(app, panel, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    number_step(app, number, CUI_BRIDGE_OK);
    value = poll(app, CUI_BRIDGE_VALUE_SCALAR_V1, false); assert(value.payload.scalar_value == 1);
    /* Existing closed-content/layout suppression, not a new bridge inert API. */
    assert(cui_bridge_set_checked(app, panel, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); empty(app);
    cui_bridge_destroy(app);
}
int main(void) {
    assert(cui_bridge_notification_versions() == 1 && cui_bridge_notification_v1_size() == sizeof(cui_bridge_notification_v1));
    assert(cui_bridge_notification_value_payloads_v1() == (CUI_BRIDGE_VALUE_SCALAR_V1 | CUI_BRIDGE_VALUE_TEXT_V1 | CUI_BRIDGE_VALUE_SELECTION_V1));
    number_and_protocol(); other_scalars(); overflow_and_interleave(); non_scalar_and_hidden(); return 0;
}
