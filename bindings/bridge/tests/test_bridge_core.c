/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_bridge_status click(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect bounds = {0};
    cui_bridge_status status = cui_bridge_node_bounds(app, node, &bounds);
    assert(status == CUI_BRIDGE_OK && bounds.width > 0 && bounds.height > 0);
    float x = bounds.x + bounds.width / 2, y = bounds.y + bounds.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    return cui_bridge_step(app, .016f);
}
int main(void) {
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 320,
                                  .height = 200};
    assert(cui_bridge_abi_version() == CUI_BRIDGE_ABI_VERSION);
    assert(cui_bridge_options_size() == sizeof options);
    options.struct_size--;
    assert(!cui_bridge_create(&options, &status) && status == CUI_BRIDGE_INVALID_ARGUMENT);
    options.struct_size++;
    options.width = FLT_MAX;
    assert(!cui_bridge_create(&options, &status) && status == CUI_BRIDGE_INVALID_ARGUMENT);
    options.width = NAN;
    assert(!cui_bridge_create(&options, &status) && status == CUI_BRIDGE_INVALID_ARGUMENT);
    options.width = 320;
    options.height = FLT_MAX;
    assert(!cui_bridge_create(&options, &status) && status == CUI_BRIDGE_INVALID_ARGUMENT);
    options.height = 200;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, save = {0}, duplicate = {73, 91};
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_ROW, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_gap(app, root, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "save", &save) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "save", &duplicate) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(duplicate.slot == 73 && duplicate.generation == 91);
    assert(cui_bridge_set_text(app, save, "Save changes \xE2\x9C\x93") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, save, "\xC0\xAF") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_text(app, save, "\xED\xA0\x80") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_button_variant(app, save, CUI_BRIDGE_BUTTON_PRIMARY) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_size(app, save, CUI_BRIDGE_SIZE_9XL) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_rounding(app, save, CUI_BRIDGE_ROUND_THEME, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_hover_rgba(app, save, 90, 80, 70, 255) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, save, &bounds) == CUI_BRIDGE_OK);
    assert(bounds.width > 0 && bounds.height > 0);
    assert(click(app, save) == CUI_BRIDGE_OK);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CLICK && event.node.slot == save.slot &&
           event.node.generation == save.generation && event.frame_sequence > 0);
    cui_bridge_event previous = event;
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    assert(memcmp(&event, &previous, sizeof event) == 0);
    cui_bridge_rect unchanged = {11, 12, 13, 14};
    assert(cui_bridge_node_bounds(app, (cui_bridge_node){9999, 1}, &unchanged) ==
           CUI_BRIDGE_STALE_HANDLE);
    assert(unchanged.x == 11 && unchanged.y == 12 && unchanged.width == 13 &&
           unchanged.height == 14);
    assert(cui_bridge_set_button_disabled(app, save, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(click(app, save) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_button_disabled(app, save, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_TAB, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ENTER, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK);
    assert(event.node.slot == save.slot);
    assert(cui_bridge_node_remove(app, save) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, save, "stale") == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "save", &duplicate) ==
           CUI_BRIDGE_OK);
    assert(duplicate.slot == save.slot && duplicate.generation != save.generation);
    assert(cui_bridge_request_close(app) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CLOSED);
    cui_bridge_destroy(app);
    options.max_nodes = 1;
    app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    root = (cui_bridge_node){0};
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_ROW, "root", &root) ==
           CUI_BRIDGE_OK);
    duplicate = (cui_bridge_node){73, 91};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "save", &duplicate) ==
           CUI_BRIDGE_CAPACITY);
    assert(duplicate.slot == 73 && duplicate.generation == 91);
    cui_bridge_destroy(app);
    options.max_nodes = 256;
    app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node parent = {0};
    for (unsigned depth = 0; depth < CUI_BRIDGE_MAX_DEPTH; depth++) {
        char key[32];
        snprintf(key, sizeof key, "level-%u", depth);
        cui_bridge_node child;
        assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_ROW, key, &child) == CUI_BRIDGE_OK);
        parent = child;
    }
    duplicate = (cui_bridge_node){73, 91};
    assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_ROW, "too-deep", &duplicate) ==
           CUI_BRIDGE_CAPACITY);
    assert(duplicate.slot == 73 && duplicate.generation == 91);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    parent = (cui_bridge_node){0};
    for (unsigned depth = 0; depth < CUI_BRIDGE_MAX_DEPTH - 2; depth++) {
        char key[32];
        snprintf(key, sizeof key, "list-level-%u", depth);
        cui_bridge_node child;
        assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_ROW, key, &child) == CUI_BRIDGE_OK);
        parent = child;
    }
    cui_bridge_node list = {0}, item = {0};
    assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_DESCRIPTION_LIST, "deep-list", &list) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, list, CUI_BRIDGE_ITEM, "deep-item", &item) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, item, "Label") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, item, "Value") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    puts("bridge core ok");
}
