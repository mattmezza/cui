/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
static cui_bridge_app *create(uint32_t events, cui_bridge_node *root) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 800,
                                  .height = 800,
                                  .max_events = events};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", root) ==
           CUI_BRIDGE_OK);
    return app;
}
static void focus(cui_bridge_app *app, cui_bridge_node input) {
    cui_bridge_rect b;
    assert(cui_bridge_node_bounds(app, input, &b) == CUI_BRIDGE_OK);
    assert(b.width > 0 && b.height > 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, b.x + 4, b.y + 4) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, b.x + 4, b.y + 4) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}
static void type(cui_bridge_app *app, const char *text, cui_bridge_status expected) {
    assert(cui_bridge_queue_text(app, text) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == expected);
}
static void empty(cui_bridge_app *app) {
    cui_bridge_notification_v1 value;
    char text[4097];
    uint32_t required;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, CUI_BRIDGE_VALUE_TEXT_V1,
                                                 &value, text, sizeof text,
                                                 &required) == CUI_BRIDGE_EMPTY);
}
static cui_bridge_notification_v1 poll(cui_bridge_app *app, uint32_t mask, bool old, char *text) {
    cui_bridge_notification_v1 value;
    uint32_t required;
    cui_bridge_status status =
        old ? cui_bridge_poll_notification_v1(app, sizeof value, &value, text, 4097, &required)
            : cui_bridge_poll_value_notification_v1(app, sizeof value, mask, &value, text, 4097,
                                                    &required);
    assert(status == CUI_BRIDGE_OK && required == value.text_length + 1 &&
           value.legacy.text_length == value.text_length);
    assert(value.kind == CUI_BRIDGE_NOTIFY_CHANGE && value.node.slot == value.legacy.node.slot);
    return value;
}
static void producers(void) {
    cui_bridge_kind kinds[] = {CUI_BRIDGE_INPUT, CUI_BRIDGE_TEXTAREA, CUI_BRIDGE_EDITOR,
                               CUI_BRIDGE_INPUT_GROUP};
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++) {
        cui_bridge_node root, input;
        cui_bridge_app *app = create(16, &root);
        assert(cui_bridge_node_create(app, root, kinds[i], "input", &input) == CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(app, input, "") == CUI_BRIDGE_OK);
        assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
        empty(app);
        focus(app, input);
        empty(app);
        assert(cui_bridge_queue_text(app, "A") == CUI_BRIDGE_OK);
        assert(cui_bridge_queue_text(app, "\xc3\xa9") == CUI_BRIDGE_OK);
        assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
        /* One legacy aggregate record for two edits in this frame. */
        assert(cui_bridge_set_text(app, input, "later") == CUI_BRIDGE_OK);
        char text[4097];
        cui_bridge_notification_v1 value = poll(app, CUI_BRIDGE_VALUE_TEXT_V1, false, text);
        assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_TEXT &&
               value.node_kind == (uint32_t)kinds[i]);
        assert(!strcmp(text, "A\xc3\xa9") && value.text_length == 3 &&
               value.flags == CUI_BRIDGE_NOTIFY_HAS_FRAME);
        empty(app);
        focus(app, input);
        type(app, "x", CUI_BRIDGE_OK);
        value = poll(app, 0, true, text);
        assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY);
        char legacy[4097];
        strcpy(legacy, text);
        type(app, "y", CUI_BRIDGE_OK);
        value = poll(app, CUI_BRIDGE_VALUE_SCALAR_V1, false, text);
        assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY &&
               strlen(text) == strlen(legacy) + 1);
        assert(cui_bridge_set_disabled(app, input, 1) == CUI_BRIDGE_OK);
        assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
        type(app, "z", CUI_BRIDGE_OK);
        empty(app);
        assert(cui_bridge_node_remove(app, input) == CUI_BRIDGE_OK);
        cui_bridge_destroy(app);
        assert(value.text_length == strlen(text)); /* caller storage survives origin/app */
    }
}
static void capacity_and_interleave(void) {
    cui_bridge_node root, input;
    cui_bridge_app *app = create(4, &root);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_INPUT, "input", &input) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    focus(app, input);
    char maximum[4097];
    memset(maximum, 'x', 4096);
    maximum[4096] = 0;
    type(app, maximum, CUI_BRIDGE_OK);
    cui_bridge_notification_v1 value, before;
    memset(&value, 0xa5, sizeof value);
    before = value;
    char text[4097] = "same";
    uint32_t required = 77;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, 1u << CUI_BRIDGE_PAYLOAD_LEGACY,
                                                 &value, text, sizeof text,
                                                 &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(&value, &before, sizeof value) && required == 77 && !strcmp(text, "same"));
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value - 1, CUI_BRIDGE_VALUE_TEXT_V1,
                                                 &value, text, sizeof text,
                                                 &required) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, CUI_BRIDGE_VALUE_TEXT_V1,
                                                 &value, text, 4096,
                                                 &required) == CUI_BRIDGE_CAPACITY);
    assert(required == 4097 && !memcmp(&value, &before, sizeof value) && !strcmp(text, "same"));
    assert(cui_bridge_set_text(app, input, "different") == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_value_notification_v1(app, sizeof value, CUI_BRIDGE_VALUE_TEXT_V1,
                                                 &value, text, required,
                                                 &required) == CUI_BRIDGE_OK);
    assert(value.payload_kind == CUI_BRIDGE_PAYLOAD_TEXT && value.text_length == 4096 &&
           !strcmp(text, maximum));
    /* Setter/model-only changes remain silent. */
    assert(cui_bridge_set_text(app, input, "a") == CUI_BRIDGE_OK);
    focus(app, input);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 0) ==
           CUI_BRIDGE_OK);
    /* Public bridge keys have no Backspace/Delete/Ctrl+A. Do not invent an
     * empty CHANGE from a setter just to claim deletion-input coverage. */
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    empty(app);
    type(app, "b", CUI_BRIDGE_OK);
    type(app, "c", CUI_BRIDGE_OK);
    cui_bridge_event_ex legacy;
    assert(cui_bridge_poll_event_ex(app, &legacy, text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(legacy.text_length == 2);
    value = poll(app, CUI_BRIDGE_VALUE_TEXT_V1, false, text);
    assert(value.text_length == 3);
    empty(app);
    cui_bridge_destroy(app);
}
static void overflow(void) {
    cui_bridge_node root, input;
    cui_bridge_app *app = create(1, &root);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_INPUT, "input", &input) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    focus(app, input);
    type(app, "a", CUI_BRIDGE_OK);
    type(app, "b", CUI_BRIDGE_CAPACITY);
    char text[4097];
    uint32_t required;
    assert(cui_bridge_get_text(app, input, text, sizeof text, &required) == CUI_BRIDGE_OK &&
           !strcmp(text, "ab"));
    cui_bridge_notification_v1 value = poll(app, CUI_BRIDGE_VALUE_TEXT_V1, false, text);
    assert(value.text_length == 1 && !strcmp(text, "a"));
    empty(app);
    cui_bridge_destroy(app);
}
int main(void) {
    assert(cui_bridge_notification_value_payloads_v1() ==
           (CUI_BRIDGE_VALUE_SCALAR_V1 | CUI_BRIDGE_VALUE_TEXT_V1 | CUI_BRIDGE_VALUE_SELECTION_V1));
    producers();
    capacity_and_interleave();
    overflow();
    return 0;
}
