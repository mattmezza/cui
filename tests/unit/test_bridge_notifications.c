/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
/* Freeze the old public record layout independently of the new schema. */
typedef struct {
    cui_bridge_event_type type;
    cui_bridge_node node;
    uint64_t frame_sequence;
} old_event;
typedef struct {
    cui_bridge_event_type type;
    cui_bridge_node node;
    uint64_t frame_sequence;
    uint32_t checked;
    double value;
    uint32_t text_length;
    cui_bridge_node selected;
} old_event_ex;
_Static_assert(CUI_BRIDGE_ABI_VERSION == 1u, "legacy ABI version changed");
_Static_assert(sizeof(old_event) == sizeof(cui_bridge_event), "legacy event size changed");
_Static_assert(sizeof(old_event_ex) == sizeof(cui_bridge_event_ex), "legacy extended size changed");
_Static_assert(offsetof(old_event_ex, value) == offsetof(cui_bridge_event_ex, value),
               "legacy offset changed");
_Static_assert(offsetof(old_event_ex, selected) == offsetof(cui_bridge_event_ex, selected),
               "legacy offset changed");
static cui_bridge_app *create(uint32_t max_events, cui_bridge_node *root) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 3000,
                                  .height = 1000,
                                  .max_events = max_events};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", root) ==
           CUI_BRIDGE_OK);
    return app;
}
static cui_bridge_node node(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                            const char *key) {
    cui_bridge_node value;
    assert(cui_bridge_node_create(app, parent, kind, key, &value) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, value, key) == CUI_BRIDGE_OK);
    return value;
}
static void queue_click(cui_bridge_app *app, cui_bridge_rect bounds) {
    float x = bounds.x + bounds.width * .5f, y = bounds.y + bounds.height * .5f;
    assert(bounds.width > 0 && bounds.height > 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
}
static void press(cui_bridge_app *app, cui_bridge_node target, cui_bridge_status expected) {
    cui_bridge_rect bounds;
    assert(cui_bridge_node_bounds(app, target, &bounds) == CUI_BRIDGE_OK);
    queue_click(app, bounds);
    assert(cui_bridge_step(app, .016f) == expected);
}
static cui_bridge_notification_v1 poll(cui_bridge_app *app, char *text, uint32_t capacity) {
    cui_bridge_notification_v1 result;
    uint32_t required;
    assert(cui_bridge_poll_notification_v1(app, sizeof result, &result, text, capacity,
                                           &required) == CUI_BRIDGE_OK);
    assert(result.struct_size == sizeof result && result.schema_version == 1);
    assert(required == result.text_length + 1 && result.legacy.text_length == result.text_length);
    assert(result.node.slot == result.legacy.node.slot &&
           result.node.generation == result.legacy.node.generation);
    assert(result.frame_sequence == result.legacy.frame_sequence);
    return result;
}
static void booleans_and_queue(void) {
    cui_bridge_node root;
    cui_bridge_app *app = create(2, &root);
    cui_bridge_node check = node(app, root, CUI_BRIDGE_CHECKBOX, "check");
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, check, CUI_BRIDGE_OK);
    press(app, check, CUI_BRIDGE_OK);
    press(app, check, CUI_BRIDGE_CAPACITY);
    uint32_t checked;
    assert(cui_bridge_get_checked(app, check, &checked) == CUI_BRIDGE_OK && checked == 1);
    char text[4097];
    cui_bridge_notification_v1 first = poll(app, text, sizeof text);
    assert(first.kind == CUI_BRIDGE_NOTIFY_CHANGE &&
           first.payload_kind == CUI_BRIDGE_PAYLOAD_BOOLEAN);
    assert(first.node_kind == CUI_BRIDGE_CHECKBOX && first.payload.boolean_value == 1 &&
           first.legacy.checked == 1);
    assert(first.flags == CUI_BRIDGE_NOTIFY_HAS_FRAME && !text[0]);
    cui_bridge_event_ex second;
    uint32_t required;
    assert(cui_bridge_poll_event_ex(app, &second, text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(second.checked == 0 && second.frame_sequence > first.frame_sequence);
    cui_bridge_notification_v1 sentinel;
    memset(&sentinel, 0xa5, sizeof sentinel);
    cui_bridge_notification_v1 before = sentinel;
    strcpy(text, "unchanged");
    required = 77;
    assert(cui_bridge_poll_notification_v1(app, sizeof sentinel, &sentinel, text, sizeof text,
                                           &required) == CUI_BRIDGE_EMPTY);
    assert(!memcmp(&sentinel, &before, sizeof before) && required == 77 &&
           !strcmp(text, "unchanged"));
    cui_bridge_destroy(app);
    assert(first.payload.boolean_value == 1); /* owned scalar snapshot survives app */

    app = create(8, &root);
    cui_bridge_kind kinds[] = {CUI_BRIDGE_SWITCH, CUI_BRIDGE_TOGGLE};
    for (size_t i = 0; i < 2; ++i) {
        cui_bridge_node control = node(app, root, kinds[i], i ? "toggle" : "switch");
        assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
        press(app, control, CUI_BRIDGE_OK);
        first = poll(app, text, sizeof text);
        assert(first.node_kind == (uint32_t)kinds[i] &&
               first.payload_kind == CUI_BRIDGE_PAYLOAD_BOOLEAN);
        assert(first.payload.boolean_value == 1);
    }
    cui_bridge_destroy(app);
}
static void legacy_text_and_stale(void) {
    cui_bridge_node root;
    cui_bridge_app *app = create(16, &root);
    cui_bridge_node button = node(app, root, CUI_BRIDGE_BUTTON, "button");
    cui_bridge_node input = node(app, root, CUI_BRIDGE_INPUT, "input");
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, button, CUI_BRIDGE_OK);
    press(app, input, CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "\xc3\xa9") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    /* Discard stale origin first, then preserve the next live event on CAPACITY. */
    assert(cui_bridge_node_remove(app, button) == CUI_BRIDGE_OK);
    cui_bridge_node replacement = node(app, root, CUI_BRIDGE_BUTTON, "replacement");
    assert(replacement.slot == button.slot && replacement.generation != button.generation);
    cui_bridge_notification_v1 result, before;
    memset(&result, 0xa5, sizeof result);
    before = result;
    char text[4097] = "old";
    uint32_t required = 99;
    assert(cui_bridge_poll_notification_v1(app, sizeof result - 1, &result, text, sizeof text,
                                           &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(&result, &before, sizeof result) && required == 99 && !strcmp(text, "old"));
    assert(cui_bridge_poll_notification_v1(app, sizeof result, NULL, text, sizeof text,
                                           &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_poll_notification_v1(app, sizeof result, &result, text, sizeof text, NULL) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_poll_notification_v1(app, sizeof result, &result, text, 1, &required) ==
           CUI_BRIDGE_CAPACITY);
    assert(required > 1 && !memcmp(&result, &before, sizeof result) && !strcmp(text, "old"));
    assert(cui_bridge_poll_notification_v1(app, sizeof result, &result, NULL, 0, &required) ==
           CUI_BRIDGE_CAPACITY);
    uint32_t exact = required;
    result = poll(app, text, exact);
    assert(result.node.slot == input.slot && result.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY);
    assert(result.node_kind == CUI_BRIDGE_INPUT && strstr(text, "\xc3\xa9"));
    char saved[4097];
    strcpy(saved, text);
    assert(cui_bridge_set_text(app, input, "later") == CUI_BRIDGE_OK);
    assert(!strcmp(text, saved));
    /* Old filtered poll and V1 share consumption, without replay. */
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, replacement, CUI_BRIDGE_OK);
    cui_bridge_event legacy;
    assert(cui_bridge_poll_event(app, &legacy) == CUI_BRIDGE_OK &&
           legacy.node.generation == replacement.generation);
    press(app, replacement, CUI_BRIDGE_OK);
    result = poll(app, text, sizeof text);
    assert(result.kind == CUI_BRIDGE_NOTIFY_ACTIVATE &&
           result.payload_kind == CUI_BRIDGE_PAYLOAD_NONE);
    assert(result.node.generation == replacement.generation);
    assert(cui_bridge_set_text(app, input, "") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, input, CUI_BRIDGE_OK);
    char maximum[4097];
    memset(maximum, 'x', 4096);
    maximum[4096] = 0;
    assert(cui_bridge_queue_text(app, maximum) == CUI_BRIDGE_OK);
    maximum[0] = 'z'; /* queued input owns its bytes before bridge snapshotting */
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_notification_v1(app, sizeof result, &result, text, 4096, &required) ==
           CUI_BRIDGE_CAPACITY);
    assert(required == 4097);
    result = poll(app, text, sizeof text);
    assert(result.text_length == 4096 && result.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY);
    assert(text[0] == 'x' && text[4095] == 'x' && text[4096] == 0);
    cui_bridge_destroy(app);
    assert(strstr(saved, "\xc3\xa9"));
}
static void tables(void) {
    cui_bridge_node root;
    cui_bridge_app *app = create(32, &root);
    cui_bridge_node table = node(app, root, CUI_BRIDGE_DATA_TABLE, "table");
    const char *headers[16], *cells[16];
    for (size_t i = 0; i < 16; ++i) {
        headers[i] = "Header";
        cells[i] = "Cell";
    }
    assert(cui_bridge_data_table_set(app, table, headers, 16, cells, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_set_selection_mode(app, table, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, table, 2800, 150) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    for (uint32_t column = 0; column < 16; ++column) {
        cui_bridge_rect cell;
        assert(cui_bridge_data_table_cell_bounds(app, table, 0, column, &cell) == CUI_BRIDGE_OK);
        queue_click(app, cell);
        assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    }
    char text[4097];
    for (uint32_t column = 1; column <= 16; ++column) {
        cui_bridge_notification_v1 result = poll(app, text, sizeof text);
        assert(result.payload_kind == CUI_BRIDGE_PAYLOAD_TABLE &&
               result.node_kind == CUI_BRIDGE_DATA_TABLE);
        assert(result.payload.table.reason == CUI_BRIDGE_TABLE_SELECTION);
        assert(result.payload.table.row == 1 && result.payload.table.column == column);
        assert(result.legacy.checked == column && result.legacy.value == 1);
        char expected[32];
        snprintf(expected, sizeof expected, "\"column\":%u", column);
        assert(strstr(text, expected));
    }
    cui_bridge_rect header;
    assert(cui_bridge_data_table_header_bounds(app, table, 15, &header) == CUI_BRIDGE_OK);
    queue_click(app, header);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_notification_v1 sorted = poll(app, text, sizeof text);
    assert(sorted.payload.table.reason == CUI_BRIDGE_TABLE_SORT &&
           sorted.payload.table.sort_column == 16);
    assert(sorted.payload.table.sort_direction != 0 && strstr(text, "\"reason\":\"sort\""));
    cui_bridge_destroy(app);
}
static void selected_handle(void) {
    cui_bridge_node root;
    cui_bridge_app *app = create(8, &root);
    cui_bridge_node group = node(app, root, CUI_BRIDGE_RADIO_GROUP, "group");
    cui_bridge_node selected = node(app, group, CUI_BRIDGE_ITEM, "selected");
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, selected, CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, selected) == CUI_BRIDGE_OK);
    cui_bridge_node replacement = node(app, group, CUI_BRIDGE_ITEM, "new-item");
    assert(replacement.slot == selected.slot && replacement.generation != selected.generation);
    char text[32];
    cui_bridge_notification_v1 result = poll(app, text, sizeof text);
    assert(result.node.slot == group.slot && result.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY);
    assert(result.legacy.selected.slot == selected.slot);
    assert(result.legacy.selected.generation == selected.generation);
    assert(result.legacy.selected.generation != replacement.generation);
    cui_bridge_destroy(app);
}
static void programmatic_legacy(void) {
    cui_bridge_node root;
    cui_bridge_app *app = create(8, &root);
    cui_bridge_node questionnaire = node(app, root, CUI_BRIDGE_QUESTIONNAIRE, "survey");
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_submit(app, questionnaire) == CUI_BRIDGE_OK);
    char text[4097];
    cui_bridge_notification_v1 result = poll(app, text, sizeof text);
    assert(result.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY);
    assert(result.node_kind == CUI_BRIDGE_QUESTIONNAIRE && result.frame_sequence == 0 &&
           result.flags == 0);
    cui_bridge_destroy(app);
}
static void surface(void) {
    cui_bridge_node root;
    cui_bridge_app *app = create(8, &root);
    cui_bridge_node dialog = node(app, root, CUI_BRIDGE_DIALOG, "dialog");
    assert(cui_bridge_set_open(app, dialog, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ESCAPE, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    char text[8];
    cui_bridge_notification_v1 result = poll(app, text, sizeof text);
    assert(result.kind == CUI_BRIDGE_NOTIFY_LIFECYCLE &&
           result.payload_kind == CUI_BRIDGE_PAYLOAD_SURFACE);
    assert(result.payload.surface_result == (uint32_t)result.legacy.value);
    cui_bridge_destroy(app);
}
int main(void) {
    assert(cui_bridge_notification_v1_size() == sizeof(cui_bridge_notification_v1));
    assert(cui_bridge_notification_versions() == 1u);
    booleans_and_queue();
    legacy_text_and_stale();
    tables();
    selected_handle();
    programmatic_legacy();
    surface();
    puts("V1 bridge notifications: enqueue snapshots, legacy ABI/queue, capacities and allowlist "
         "PASS");
    return 0;
}
