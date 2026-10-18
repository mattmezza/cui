/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
static cui_bridge_app *create(uint32_t events, cui_bridge_kind kind, cui_bridge_node *group,
                              cui_bridge_node *first, cui_bridge_node *second) {
    cui_bridge_options options = {.struct_size = sizeof options, .abi_version = CUI_BRIDGE_ABI_VERSION,
        .backend = CUI_BRIDGE_BACKEND_NONE, .width = 800, .height = 800, .max_events = events};
    cui_bridge_status status; cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK); cui_bridge_node root;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, kind, "group", group) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, *group, CUI_BRIDGE_ITEM, "first", first) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, *group, CUI_BRIDGE_ITEM, "second", second) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, *first, "First") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, *second, "Second") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_selected(app, *group, *first) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); return app;
}
static void queue_click(cui_bridge_app *app, cui_bridge_rect b) {
    assert(b.width > 0 && b.height > 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, b.x + b.width / 2, b.y + b.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, b.x + b.width / 2, b.y + b.height / 2) == CUI_BRIDGE_OK);
}
static void target(cui_bridge_app *app, cui_bridge_node value) {
    cui_bridge_rect b; assert(cui_bridge_node_bounds(app, value, &b) == CUI_BRIDGE_OK); queue_click(app, b);
}
static void open(cui_bridge_app *app, cui_bridge_node group) {
    target(app, group); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}
static void activate(cui_bridge_app *app, cui_bridge_kind kind, cui_bridge_node group,
                     cui_bridge_node item, uint32_t index, cui_bridge_status expected) {
    if (kind == CUI_BRIDGE_SELECT || kind == CUI_BRIDGE_MENU) open(app, group);
    if (kind == CUI_BRIDGE_STEPPER) {
        cui_bridge_rect b; assert(cui_bridge_numeric_part_bounds(app, group, index, &b) == CUI_BRIDGE_OK); queue_click(app, b);
    } else target(app, item);
    assert(cui_bridge_step(app, .016f) == expected);
}
static void empty(cui_bridge_app *app) {
    cui_bridge_notification_v1 value; char text[4097]; uint32_t required;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, CUI_BRIDGE_VALUE_SELECTION_V1,
        &value, text, sizeof text, &required) == CUI_BRIDGE_EMPTY);
}
static cui_bridge_notification_v1 poll(cui_bridge_app *app, bool old, uint32_t mask) {
    cui_bridge_notification_v1 value; char text[4097]; uint32_t required;
    cui_bridge_status status = old ? cui_bridge_poll_notification_v1(app, sizeof value, &value, text, sizeof text, &required)
        : cui_bridge_poll_value_notification_v1(app, sizeof value, mask, &value, text, sizeof text, &required);
    assert(status == CUI_BRIDGE_OK && !text[0] && required == 1);
    assert(value.kind == CUI_BRIDGE_NOTIFY_CHANGE && value.legacy.value == value.legacy.selected.slot);
    return value;
}
static void producers(void) {
    cui_bridge_kind kinds[] = {CUI_BRIDGE_RADIO_GROUP, CUI_BRIDGE_TABS, CUI_BRIDGE_SELECT,
                              CUI_BRIDGE_LIST, CUI_BRIDGE_STEPPER};
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++) {
        cui_bridge_node group, first, second; cui_bridge_app *app = create(16, kinds[i], &group, &first, &second);
        empty(app); activate(app, kinds[i], group, second, 2, CUI_BRIDGE_OK);
        /* A subsequent application commit must not rewrite the queued handle. */
        assert(cui_bridge_set_selected(app, group, first) == CUI_BRIDGE_OK);
        cui_bridge_notification_v1 value = poll(app, false, CUI_BRIDGE_VALUE_SELECTION_V1);
        assert(value.node_kind == (uint32_t)kinds[i] && value.node.slot == group.slot);
        assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_SELECTION && value.payload.selected.slot == second.slot);
        assert(value.payload.selected.generation == second.generation && value.flags == CUI_BRIDGE_NOTIFY_HAS_FRAME);
        assert(!memcmp(&value.payload.selected, &value.legacy.selected, sizeof second)); empty(app);
        assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
        activate(app, kinds[i], group, second, 2, CUI_BRIDGE_OK);
        value = poll(app, true, 0); assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && value.legacy.selected.slot == second.slot);
        activate(app, kinds[i], group, first, 1, CUI_BRIDGE_OK);
        value = poll(app, false, CUI_BRIDGE_VALUE_SCALAR_V1 | CUI_BRIDGE_VALUE_TEXT_V1);
        assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && value.legacy.selected.slot == first.slot);
        if (kinds[i] != CUI_BRIDGE_SELECT) {
            assert(cui_bridge_set_disabled(app, group, 1) == CUI_BRIDGE_OK);
            assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
            activate(app, kinds[i], group, second, 2, CUI_BRIDGE_OK); empty(app);
        } else {
            open(app, group);
            assert(cui_bridge_set_disabled(app, group, 1) == CUI_BRIDGE_OK);
            target(app, second); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); empty(app);
        }
        assert(cui_bridge_set_selected(app, group, group) == CUI_BRIDGE_INVALID_ARGUMENT);
        cui_bridge_destroy(app); assert(value.legacy.selected.generation == first.generation);
    }
}
static void repeated_and_aggregate(void) {
    cui_bridge_node group, first, second; cui_bridge_app *app = create(8, CUI_BRIDGE_LIST, &group, &first, &second);
    activate(app, CUI_BRIDGE_LIST, group, first, 1, CUI_BRIDGE_OK);
    cui_bridge_notification_v1 value = poll(app, false, CUI_BRIDGE_VALUE_SELECTION_V1);
    assert(value.payload.selected.slot == first.slot); /* Existing same-item activation notification. */
    activate(app, CUI_BRIDGE_LIST, group, second, 2, CUI_BRIDGE_OK);
    activate(app, CUI_BRIDGE_LIST, group, first, 1, CUI_BRIDGE_OK);
    char text[32]; uint32_t required; cui_bridge_event_ex legacy;
    assert(cui_bridge_poll_event_ex(app, &legacy, text, sizeof text, &required) == CUI_BRIDGE_OK && legacy.selected.slot == second.slot);
    value = poll(app, false, CUI_BRIDGE_VALUE_SELECTION_V1); assert(value.payload.selected.slot == first.slot);
    assert(cui_bridge_set_selected(app, group, (cui_bridge_node){0}) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); empty(app); /* Clear setter is silent. */
    cui_bridge_destroy(app);
    app = create(8, CUI_BRIDGE_SELECT, &group, &first, &second); open(app, group);
    target(app, second); target(app, first);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK); empty(app);
    /* Existing Select final aggregate suppresses a return to initial value. */
    cui_bridge_destroy(app);
}
static void lifetime_capacity_overflow(void) {
    cui_bridge_node group, first, second; cui_bridge_app *app = create(1, CUI_BRIDGE_LIST, &group, &first, &second);
    activate(app, CUI_BRIDGE_LIST, group, second, 2, CUI_BRIDGE_OK);
    activate(app, CUI_BRIDGE_LIST, group, first, 1, CUI_BRIDGE_CAPACITY);
    cui_bridge_node current; assert(cui_bridge_get_selected(app, group, &current) == CUI_BRIDGE_OK && current.slot == first.slot);
    assert(cui_bridge_node_remove(app, second) == CUI_BRIDGE_OK);
    cui_bridge_node replacement;
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "replacement", &replacement) == CUI_BRIDGE_OK);
    assert(replacement.slot == second.slot && replacement.generation != second.generation);
    cui_bridge_notification_v1 value, before; memset(&value, 0xa5, sizeof value); before = value;
    char text[8] = "same"; uint32_t required = 77;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, UINT32_MAX,
        &value, text, sizeof text, &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value - 1, CUI_BRIDGE_VALUE_SELECTION_V1,
        &value, text, sizeof text, &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(required == 77 && !memcmp(&value, &before, sizeof value) && !strcmp(text, "same"));
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, CUI_BRIDGE_VALUE_SELECTION_V1,
        &value, text, 0, &required) == CUI_BRIDGE_CAPACITY);
    assert(required == 1 && !memcmp(&value, &before, sizeof value) && !strcmp(text, "same"));
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, CUI_BRIDGE_VALUE_SELECTION_V1,
        &value, text, 1, &required) == CUI_BRIDGE_OK);
    assert(value.payload.selected.slot == second.slot && value.payload.selected.generation == second.generation);
    assert(value.payload.selected.generation != replacement.generation); empty(app); cui_bridge_destroy(app);
    assert(value.payload.selected.generation == second.generation);
}
static void omitted_commands(void) {
    cui_bridge_node group, first, second; cui_bridge_app *app = create(8, CUI_BRIDGE_MENU, &group, &first, &second);
    activate(app, CUI_BRIDGE_MENU, group, second, 2, CUI_BRIDGE_OK);
    cui_bridge_notification_v1 value = poll(app, false, CUI_BRIDGE_VALUE_SELECTION_V1);
    assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && value.legacy.selected.slot == second.slot);
    cui_bridge_destroy(app);
    app = create(8, CUI_BRIDGE_DROPDOWN_BUTTON, &group, &first, &second);
    cui_bridge_rect trigger;
    assert(cui_bridge_dropdown_part_bounds(app, group, 1, &trigger) == CUI_BRIDGE_OK);
    queue_click(app, trigger); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    target(app, second); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    value = poll(app, false, CUI_BRIDGE_VALUE_SELECTION_V1);
    assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && value.legacy.selected.slot == second.slot);
    cui_bridge_destroy(app);
    app = create(8, CUI_BRIDGE_COMBOBOX, &group, &first, &second);
    target(app, group); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "Second") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    char text[4097]; uint32_t required;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value,
        CUI_BRIDGE_VALUE_SELECTION_V1 | CUI_BRIDGE_VALUE_TEXT_V1, &value,
        text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && !strcmp(text, "Second"));
    target(app, second); assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value,
        CUI_BRIDGE_VALUE_SELECTION_V1 | CUI_BRIDGE_VALUE_TEXT_V1, &value,
        text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && value.legacy.selected.slot == second.slot);
    empty(app); cui_bridge_destroy(app);
}
int main(void) {
    assert(cui_bridge_notification_value_payloads_v1() == (CUI_BRIDGE_VALUE_SCALAR_V1 | CUI_BRIDGE_VALUE_TEXT_V1 | CUI_BRIDGE_VALUE_SELECTION_V1));
    producers(); repeated_and_aggregate(); lifetime_capacity_overflow(); omitted_commands(); return 0;
}
