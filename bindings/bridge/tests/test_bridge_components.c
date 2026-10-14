/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void press(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, node, &r) == CUI_BRIDGE_OK);
    float x = r.x + r.width * .5f, y = r.y + r.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}
static void test_chrome(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, bar = {0}, left = {0}, middle = {0}, right = {0};
    cui_bridge_node title = {0}, caption = {0}, toolbar = {0}, item = {0}, button = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_STATUS_BAR, "status", &bar) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, bar, CUI_BRIDGE_ITEM, "left", &left) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, bar, CUI_BRIDGE_ITEM, "middle", &middle) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, bar, CUI_BRIDGE_ITEM, "right", &right) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, left, "Ready") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, middle, "12 items") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, right, "Connected") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, middle, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, right, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, right, 3) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TITLE_BAR, "title", &title) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, title, CUI_BRIDGE_ITEM, "caption", &caption) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, caption, "Document") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TOOLBAR, "toolbar", &toolbar) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, toolbar, CUI_BRIDGE_ITEM, "action", &item) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, item, CUI_BRIDGE_BUTTON, "button", &button) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, button, "Save") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, bar, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_node_bounds(app, title, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_node_bounds(app, toolbar, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_node_bounds(app, button, &r) == CUI_BRIDGE_OK && r.width > 0);
    press(app, button);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CLICK && event.node.slot == button.slot);
    assert(cui_bridge_set_disabled(app, toolbar, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, button);
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_message_icon(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, icon = {0}, message = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ICON, "icon", &icon) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_MESSAGE, "message", &message) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, icon, "check") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, icon, 24, 24) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, message, "Hello") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, message, "Ada") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, message, 280, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, icon, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_node_bounds(app, message, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_set_text(app, icon, "missing") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_destroy(app);
}
static void test_attachment(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, attachment = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ATTACHMENT, "attachment", &attachment) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, attachment, "report.pdf") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, attachment, "1.2 MB") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, attachment, CUI_BRIDGE_ATTACHMENT_UPLOADING) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, attachment, .4) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, attachment, 1.1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_visual_size(app, attachment, 240, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, attachment, &r) == CUI_BRIDGE_OK && r.width > 0);
    press(app, attachment);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    double value = 0;
    assert(cui_bridge_get_value(app, attachment, &value) == CUI_BRIDGE_OK && value == .4);
    assert(cui_bridge_set_attachment_status(app, attachment, CUI_BRIDGE_ATTACHMENT_FAILED) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, attachment, (cui_bridge_attachment_status)9) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_destroy(app);
}
static void test_image(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, first = {0}, second = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_IMAGE, "first", &first) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_IMAGE, "second", &second) == CUI_BRIDGE_OK);
    uint8_t *pixels = malloc(16);
    assert(pixels);
    for (unsigned i = 0; i < 16; ++i)
        pixels[i] = (uint8_t)(i * 13);
    assert(cui_bridge_set_image_rgba(app, first, pixels, 2, 2, 8, 16) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_image_rgba(app, second, pixels, 2, 2, 8, 15) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_image_rgba(app, second, pixels, 2, 2, 8, 16) == CUI_BRIDGE_OK);
    free(pixels);
    assert(cui_bridge_set_text(app, first, "First image") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, first, 40, 30) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, first, &r) == CUI_BRIDGE_OK && r.width == 40 &&
           r.height == 30);
    assert(cui_bridge_node_bounds(app, second, &r) == CUI_BRIDGE_OK && r.width == 2 &&
           r.height == 2);
    uint8_t replacement[16] = {0};
    assert(cui_bridge_set_image_rgba(app, first, replacement, 2, 2, 8, 16) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
static void test_tree(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, tree = {0}, branch = {0}, child = {0}, sibling = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TREE, "tree", &tree) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tree, CUI_BRIDGE_ITEM, "branch", &branch) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tree, CUI_BRIDGE_ITEM, "child", &child) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tree, CUI_BRIDGE_ITEM, "sibling", &sibling) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, branch, "Folder") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, child, "Entry") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, sibling, "Other") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, child, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, child, 64) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_visual_size(app, tree, 200, 120) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, branch, &r) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, child, &r) == CUI_BRIDGE_INVALID_STATE);
    press(app, branch);
    cui_bridge_node selected = {0};
    assert(cui_bridge_get_selected(app, tree, &selected) == CUI_BRIDGE_OK &&
           selected.slot == branch.slot);
    cui_bridge_event_ex event = {0};
    char payload[16] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.selected.slot == branch.slot);
    assert(cui_bridge_node_bounds(app, branch, &r) == CUI_BRIDGE_OK);
    float toggle_x = r.x + 12, toggle_y = r.y + r.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, toggle_x, toggle_y) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, toggle_x, toggle_y) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t expanded = 0;
    assert(cui_bridge_get_checked(app, branch, &expanded) == CUI_BRIDGE_OK && expanded == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == branch.slot &&
           event.checked == 1);
    assert(cui_bridge_node_bounds(app, child, &r) == CUI_BRIDGE_OK);
    press(app, child);
    assert(cui_bridge_get_selected(app, tree, &selected) == CUI_BRIDGE_OK &&
           selected.slot == child.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.selected.slot == child.slot && event.selected.generation == child.generation);
    assert(cui_bridge_set_disabled(app, child, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, child);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_node_remove(app, child) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, tree, &selected) == CUI_BRIDGE_OK && selected.slot == 0);
    cui_bridge_node replacement = {0};
    assert(cui_bridge_node_create(app, tree, CUI_BRIDGE_ITEM, "replacement", &replacement) ==
           CUI_BRIDGE_OK);
    assert(replacement.slot == child.slot && replacement.generation != child.generation);
    assert(cui_bridge_get_selected(app, tree, &selected) == CUI_BRIDGE_OK && selected.slot == 0);
    cui_bridge_destroy(app);
}
static void test_time_field(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, field = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TIME_FIELD, "time", &field) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, field, "Appointment") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_time(app, field, (cui_bridge_time){9, 45, 30}) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_time(app, field, (cui_bridge_time){24, 0, 0}) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_time actual = {0};
    assert(cui_bridge_get_time(app, field, &actual) == CUI_BRIDGE_OK && actual.hour == 9 &&
           actual.minute == 45 && actual.second == 30);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, field, &r) == CUI_BRIDGE_OK && r.width > 0);
    press(app, field);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_time(app, field, &actual) == CUI_BRIDGE_OK);
    assert(actual.hour != 9 || actual.minute != 45 || actual.second != 30);
    cui_bridge_event_ex event = {0};
    char payload[16] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == field.slot &&
           event.text_length == 8 && needed == 9);
    assert(cui_bridge_set_disabled(app, field, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_color_picker(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 700};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, picker = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLOR_PICKER, "picker", &picker) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_color_rgba(app, picker, UINT32_C(0xFF0000FF)) == CUI_BRIDGE_OK);
    uint32_t rgba = 0;
    assert(cui_bridge_get_color_rgba(app, picker, &rgba) == CUI_BRIDGE_OK &&
           rgba == UINT32_C(0xFF0000FF));
    assert(cui_bridge_set_visual_size(app, picker, 240, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_color_part_bounds(app, picker, 1, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_color_part_bounds(app, picker, 4, &r) == CUI_BRIDGE_INVALID_ARGUMENT);
    float x = r.x + r.width * .75f, y = r.y + r.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_color_rgba(app, picker, &rgba) == CUI_BRIDGE_OK &&
           rgba != UINT32_C(0xFF0000FF));
    cui_bridge_event_ex event = {0};
    char payload[16] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.value == rgba && event.text_length == 9 &&
           needed == 10);
    assert(cui_bridge_set_disabled(app, picker, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_color_part_bounds(app, picker, 1, &r) == CUI_BRIDGE_OK);
    x = r.x + r.width * .25f;
    y = r.y + r.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_date_pickers(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 800,
                            .height = 650};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, calendar = {0}, picker = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CALENDAR, "calendar", &calendar) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DATE_PICKER, "picker", &picker) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, picker, "Choose date") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_date(app, calendar, (cui_bridge_date){2024, 3, 5}) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_date(app, picker, (cui_bridge_date){2024, 3, 5}) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_date(app, picker, (cui_bridge_date){2024, 2, 30}) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_calendar_day_bounds(app, calendar, (cui_bridge_date){2024, 3, 10}, &r) ==
           CUI_BRIDGE_OK);
    float x = r.x + r.width * .5f, y = r.y + r.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_date date = {0};
    assert(cui_bridge_get_date(app, calendar, &date) == CUI_BRIDGE_OK && date.year == 2024 &&
           date.month == 3 && date.day == 10);
    cui_bridge_event_ex event = {0};
    char payload[16] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.value == 20240310 &&
           event.text_length == 10 && needed == 11);
    assert(cui_bridge_set_disabled(app, calendar, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_calendar_day_bounds(app, calendar, (cui_bridge_date){2024, 3, 11}, &r) ==
           CUI_BRIDGE_OK);
    x = r.x + r.width * .5f;
    y = r.y + r.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    press(app, picker);
    uint32_t open = 0;
    assert(cui_bridge_get_open(app, picker, &open) == CUI_BRIDGE_OK && open == 1);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ENTER, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_date(app, picker, &date) == CUI_BRIDGE_OK && date.day == 6);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == picker.slot && event.value == 20240306 && event.text_length == 10);
    assert(cui_bridge_set_open(app, picker, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_open(app, picker, &open) == CUI_BRIDGE_OK && open == 0);
    cui_bridge_destroy(app);
}
static void test_otp(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, otp = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_OTP_INPUT, "otp", &otp) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_total(app, otp, 4) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_total(app, otp, 257) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_text(app, otp, "12") == CUI_BRIDGE_OK);
    char text[16] = "unchanged";
    uint32_t needed = 0;
    assert(cui_bridge_get_text(app, otp, text, 2, &needed) == CUI_BRIDGE_CAPACITY && needed == 3 &&
           text[0] == 'u');
    assert(cui_bridge_get_text(app, otp, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 3 && text[0] == '1');
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, otp, &r) == CUI_BRIDGE_OK && r.width > 0);
    press(app, otp);
    assert(cui_bridge_queue_text(app, "3") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, otp, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 4 && text[2] == '3');
    cui_bridge_event_ex event = {0};
    char payload[16] = {0};
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == otp.slot &&
           event.text_length == 3 && payload[2] == '3');
    assert(cui_bridge_set_disabled(app, otp, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "4") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_nested_group_stack(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 600,
                                  .height = 400,
                                  .max_nodes = 256};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node parent = {0}, root = {0};
    assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    parent = root;
    /* Each selected Item renders another Tabs group through its panel callback. */
    for (unsigned i = 0; i < 16; ++i) {
        cui_bridge_node tabs = {0}, item = {0};
        char tab_key[32], item_key[32];
        snprintf(tab_key, sizeof tab_key, "tabs-%u", i);
        snprintf(item_key, sizeof item_key, "item-%u", i);
        assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_TABS, tab_key, &tabs) ==
               CUI_BRIDGE_OK);
        assert(cui_bridge_node_create(app, tabs, CUI_BRIDGE_ITEM, item_key, &item) ==
               CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(app, item, "Nested") == CUI_BRIDGE_OK);
        assert(cui_bridge_set_selected(app, tabs, item) == CUI_BRIDGE_OK);
        parent = item;
    }
    cui_bridge_node leaf = {0};
    assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_LABEL, "leaf", &leaf) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, leaf, "End") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
static void test_form(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 700,
                            .height = 500};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, form = {0}, name_field = {0}, name_input = {0};
    cui_bridge_node action_field = {0}, submit = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_FORM, "form", &form) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, form, CUI_BRIDGE_ITEM, "name-field", &name_field) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, name_field, CUI_BRIDGE_INPUT, "name", &name_input) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, form, CUI_BRIDGE_ITEM, "action-field", &action_field) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, action_field, CUI_BRIDGE_BUTTON, "submit", &submit) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, form, "Profile") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, name_field, "Name") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, name_field, "Shown publicly") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, action_field, "Action") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, submit, "Save") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, form, 360, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, form, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_node_bounds(app, name_input, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_node_bounds(app, submit, &r) == CUI_BRIDGE_OK && r.width > 0);
    press(app, submit);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK && event.node.slot == submit.slot);
    assert(cui_bridge_set_disabled(app, action_field, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, submit);
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_disabled(app, form, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, name_input);
    assert(cui_bridge_queue_text(app, "X") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    char text[8] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_get_text(app, name_input, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 1);
    cui_bridge_destroy(app);
}
static void test_input_group(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, group = {0}, addon = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_INPUT_GROUP, "search", &group) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "go", &addon) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, addon, "Go") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, addon, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, addon, 4) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_detail_text(app, group, "Search") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, group, 320, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, group, &r) == CUI_BRIDGE_OK && r.width > 0);
    assert(cui_bridge_node_bounds(app, addon, &r) == CUI_BRIDGE_OK && r.width > 0);
    press(app, addon);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK && event.node.slot == addon.slot);
    press(app, group);
    assert(cui_bridge_queue_text(app, "a") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    char text[16] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_get_text(app, group, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 2 && text[0] == 'a');
    cui_bridge_event_ex change = {0};
    char payload[16] = {0};
    assert(cui_bridge_poll_event_ex(app, &change, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(change.node.slot == group.slot && change.text_length == 1 && payload[0] == 'a');
    assert(cui_bridge_set_disabled(app, group, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, addon);
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_notification(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, note = {0}, second = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_NOTIFICATION, "note", &note) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_NOTIFICATION, "second", &second) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, note, "Saved") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, note, "Your file was saved") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, second, "Another notification") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, note, 2) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_open(app, note, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, second, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, note, 1) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_text(app, note, "Changed") == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_step(app, .4f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, note, &bounds) == CUI_BRIDGE_OK);
    assert(bounds.width > 0 && bounds.height > 0);
    cui_bridge_rect second_bounds = {0};
    assert(cui_bridge_node_bounds(app, second, &second_bounds) == CUI_BRIDGE_OK);
    assert(second_bounds.y != bounds.y);
    uint32_t open = 0;
    assert(cui_bridge_get_open(app, note, &open) == CUI_BRIDGE_OK && open == 1);
    press(app, note);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK);
    assert(event.node.slot == note.slot && event.type == CUI_BRIDGE_EVENT_CLICK);
    assert(cui_bridge_get_open(app, note, &open) == CUI_BRIDGE_OK && open == 0);
    assert(cui_bridge_step(app, .3f) == CUI_BRIDGE_OK);
    cui_bridge_event_ex extended = {0};
    char payload[8] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &extended, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(extended.node.slot == note.slot && extended.type == CUI_BRIDGE_EVENT_CLOSE);
    assert(cui_bridge_get_open(app, second, &open) == CUI_BRIDGE_OK && open == 1);
    assert(cui_bridge_set_open(app, second, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .3f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &extended, payload, sizeof payload, &needed) ==
               CUI_BRIDGE_OK &&
           extended.type == CUI_BRIDGE_EVENT_CLOSE && extended.node.slot == second.slot);
    assert(cui_bridge_set_text(app, note, "Again") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, note, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, note, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .3f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &extended, payload, sizeof payload, &needed) ==
               CUI_BRIDGE_OK &&
           extended.type == CUI_BRIDGE_EVENT_CLOSE);
    assert(cui_bridge_set_open(app, second, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, second) == CUI_BRIDGE_OK);
    cui_bridge_node replacement = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_NOTIFICATION, "replacement",
                                  &replacement) == CUI_BRIDGE_OK);
    assert(replacement.slot == second.slot && replacement.generation != second.generation);
    assert(cui_bridge_step(app, .3f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &extended, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_focus_trap(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, outside = {0}, trap = {0}, inside = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "outside", &outside) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_FOCUS_TRAP, "trap", &trap) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, trap, CUI_BRIDGE_BUTTON, "inside", &inside) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, outside, "Outside") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, inside, "Inside") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, trap, 2) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_open(app, trap, 1) == CUI_BRIDGE_OK);
    uint32_t open = 0;
    assert(cui_bridge_get_open(app, trap, &open) == CUI_BRIDGE_OK && open == 1);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, outside);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    press(app, inside);
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK && event.node.slot == inside.slot);
    assert(cui_bridge_set_open(app, trap, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_open(app, trap, &open) == CUI_BRIDGE_OK && open == 0);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, outside);
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK && event.node.slot == outside.slot);
    cui_bridge_destroy(app);
}
static void test_message_scroller(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, scroller = {0}, items[20] = {{0}};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_MESSAGE_SCROLLER, "messages", &scroller) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, scroller, 280, 100) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, scroller, 0) == CUI_BRIDGE_INVALID_STATE);
    for (unsigned i = 0; i < 20; i++) {
        char key[20];
        snprintf(key, sizeof key, "message-%u", i);
        assert(cui_bridge_node_create(app, scroller, CUI_BRIDGE_ITEM, key, &items[i]) ==
               CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(app, items[i], key) == CUI_BRIDGE_OK);
        assert(cui_bridge_set_detail_text(app, items[i], "Sender") == CUI_BRIDGE_OK);
        assert(cui_bridge_set_step_status(app, items[i], i % 2) == CUI_BRIDGE_OK);
    }
    assert(cui_bridge_set_step_status(app, items[0], 2) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    double offset = 0;
    assert(cui_bridge_get_value(app, scroller, &offset) == CUI_BRIDGE_OK && offset > 0);
    assert(cui_bridge_set_value(app, scroller, -1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_value(app, scroller, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, scroller, &offset) == CUI_BRIDGE_OK && offset == 0);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, items[0], &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, items[1]) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_node replacement = {0};
    assert(cui_bridge_node_create(app, scroller, CUI_BRIDGE_ITEM, "replacement", &replacement) ==
           CUI_BRIDGE_OK);
    assert(replacement.slot == items[1].slot && replacement.generation != items[1].generation);
    assert(cui_bridge_set_text(app, replacement, "Replacement") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
static void test_carousel(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, carousel = {0}, slides[3] = {{0}};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CAROUSEL, "carousel", &carousel) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, carousel, 240, 100) == CUI_BRIDGE_OK);
    for (unsigned i = 0; i < 3; i++) {
        char key[16];
        snprintf(key, sizeof key, "slide-%u", i);
        assert(cui_bridge_node_create(app, carousel, CUI_BRIDGE_ITEM, key, &slides[i]) ==
               CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(app, slides[i], key) == CUI_BRIDGE_OK);
    }
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_node selected = {0};
    assert(cui_bridge_get_selected(app, carousel, &selected) == CUI_BRIDGE_OK &&
           selected.slot == slides[0].slot);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, carousel, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_scroll(app, bounds.x + 30, bounds.y + 30, -1, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, carousel, &selected) == CUI_BRIDGE_OK &&
           selected.slot == slides[1].slot);
    cui_bridge_event_ex event = {0};
    char payload[8] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.selected.slot == slides[1].slot &&
           event.selected.generation == slides[1].generation);
    assert(cui_bridge_set_selected(app, carousel, slides[2]) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, carousel, &selected) == CUI_BRIDGE_OK &&
           selected.slot == slides[2].slot);
    assert(cui_bridge_set_disabled(app, carousel, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_scroll(app, bounds.x + 30, bounds.y + 30, 1, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, carousel, &selected) == CUI_BRIDGE_OK &&
           selected.slot == slides[2].slot);
    assert(cui_bridge_node_remove(app, slides[2]) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, carousel, &selected) == CUI_BRIDGE_OK &&
           selected.slot == 0);
    cui_bridge_node replacement = {0};
    assert(cui_bridge_node_create(app, carousel, CUI_BRIDGE_ITEM, "replacement", &replacement) ==
           CUI_BRIDGE_OK);
    assert(replacement.slot == slides[2].slot && replacement.generation != slides[2].generation);
    assert(cui_bridge_get_selected(app, carousel, &selected) == CUI_BRIDGE_OK &&
           selected.slot == 0);
    cui_bridge_destroy(app);
}
static void test_resizable(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, group = {0}, first = {0}, second = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RESIZABLE, "split", &group) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "first", &first) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "second", &second) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, first, "First panel") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, second, "Second panel") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, group, 420, 160) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, first, 90) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_value(app, first, 240) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    double before = 0, after = 0;
    assert(cui_bridge_get_value(app, first, &before) == CUI_BRIDGE_OK && before >= 100);
    cui_bridge_rect handle = {0};
    assert(cui_bridge_resizable_handle_bounds(app, group, first, &handle) == CUI_BRIDGE_OK);
    assert(cui_bridge_resizable_handle_bounds(app, group, second, &handle) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    float x = handle.x + handle.width / 2, y = handle.y + handle.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, x + 30, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x + 30, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, first, &after) == CUI_BRIDGE_OK && after > before);
    cui_bridge_event_ex event = {0};
    char payload[8] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == first.slot &&
           event.value > before);
    assert(cui_bridge_set_disabled(app, group, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_resizable_handle_bounds(app, group, first, &handle) == CUI_BRIDGE_OK);
    x = handle.x + handle.width / 2;
    y = handle.y + handle.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, x - 30, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x - 30, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, first, &before) == CUI_BRIDGE_OK && before == after);
    cui_bridge_destroy(app);
}
static void test_text_view(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, view = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TEXT_VIEW, "view", &view) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, view, 320, 120) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, view, "**Hello** world") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_text_view_select(app, view, 0, 5) == CUI_BRIDGE_OK);
    char selected[32] = "preserve";
    uint32_t needed = 0;
    assert(cui_bridge_text_view_copy_selection(app, view, selected, 2, &needed) ==
           CUI_BRIDGE_CAPACITY);
    assert(strcmp(selected, "preserve") == 0 && needed == 6);
    assert(cui_bridge_text_view_copy_selection(app, view, selected, sizeof selected, &needed) ==
           CUI_BRIDGE_OK);
    assert(strcmp(selected, "Hello") == 0 && needed == 6);
    assert(cui_bridge_text_view_select(app, view, 50, 51) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_text_view_format(app, view, 3) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_text_view_format(app, view, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, view, "<b>HTML</b> text") == CUI_BRIDGE_OK);
    assert(cui_bridge_text_view_select(app, view, 0, 4) == CUI_BRIDGE_OK);
    assert(cui_bridge_text_view_copy_selection(app, view, selected, sizeof selected, &needed) ==
           CUI_BRIDGE_OK);
    assert(strcmp(selected, "HTML") == 0);
    assert(cui_bridge_set_text_view_format(app, view, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, view, "Plain text") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, view, &bounds) == CUI_BRIDGE_OK && bounds.width > 0);
    assert(cui_bridge_text_view_select(app, view, 0, 0) == CUI_BRIDGE_OK);
    float x = bounds.x + 10, y = bounds.y + 14;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, x + 50, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x + 50, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event_ex event = {0};
    char snapshot[32] = {0};
    assert(cui_bridge_poll_event_ex(app, &event, snapshot, sizeof snapshot, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == view.slot &&
           event.text_length > 0);
    assert(cui_bridge_set_disabled(app, view, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, x + 50, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x + 50, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, snapshot, sizeof snapshot, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_editor(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, editor = {0}, other = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_EDITOR, "editor", &editor) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_EDITOR, "other", &other) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, editor, "first\nsecond") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, editor, "c") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, editor, 300, 120) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, other, 300, 120) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, editor);
    assert(cui_bridge_queue_text(app, "\xE2\x9C\x93") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event_ex event = {0};
    char snapshot[4097] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, snapshot, sizeof snapshot, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == editor.slot &&
           strstr(snapshot, "\xE2\x9C\x93"));
    char text[4097] = {0};
    assert(cui_bridge_get_text(app, editor, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(strstr(text, "\xE2\x9C\x93"));
    assert(cui_bridge_get_text(app, other, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 1);
    assert(cui_bridge_set_disabled(app, editor, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "X") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, snapshot, sizeof snapshot, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
}
static void test_chart(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, chart = {0}, second = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CHART, "revenue", &chart) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CHART, "cost", &second) == CUI_BRIDGE_OK);
    char label[] = "North";
    cui_bridge_chart_datum data[] = {{label, 10}, {"South", 20}};
    assert(cui_bridge_chart_set_data(app, chart, data, 2) == CUI_BRIDGE_OK);
    label[0] = 'X';
    char readback[16] = "sentinel";
    uint32_t needed = 0;
    double value = -1;
    assert(cui_bridge_chart_get_datum(app, chart, 0, readback, 2, &needed, &value) ==
           CUI_BRIDGE_CAPACITY);
    assert(strcmp(readback, "sentinel") == 0 && value == -1 && needed == 6);
    assert(cui_bridge_chart_get_datum(app, chart, 0, readback, sizeof readback, &needed, &value) ==
           CUI_BRIDGE_OK);
    assert(strcmp(readback, "North") == 0 && value == 10);
    assert(cui_bridge_chart_set_data(app, second, data, 65) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_chart_set_kind(app, chart, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_chart_set_kind(app, chart, 4) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_visual_size(app, chart, 256, 160) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, chart, 513, 160) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, chart, &bounds) == CUI_BRIDGE_OK && bounds.width > 0);
    assert(cui_bridge_chart_set_kind(app, chart, 3) == CUI_BRIDGE_OK);
    cui_bridge_chart_datum bad[] = {{"negative", -1}};
    assert(cui_bridge_chart_set_data(app, chart, bad, 1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, chart) == CUI_BRIDGE_OK);
    assert(cui_bridge_chart_get_datum(app, chart, 0, readback, sizeof readback, &needed, &value) ==
           CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
static void test_table(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, table = {0}, head = {0}, body = {0};
    cui_bridge_node h1 = {0}, h2 = {0}, c1 = {0}, c2 = {0}, invalid = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TABLE, "scores", &table) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, table, CUI_BRIDGE_BUTTON, "invalid", &invalid) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_create(app, table, CUI_BRIDGE_ITEM, "head", &head) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, head, CUI_BRIDGE_ITEM, "name", &h1) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, head, CUI_BRIDGE_ITEM, "score", &h2) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, table, CUI_BRIDGE_ITEM, "body", &body) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, body, CUI_BRIDGE_ITEM, "alice", &c1) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, body, CUI_BRIDGE_ITEM, "95", &c2) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, body, CUI_BRIDGE_BUTTON, "invalid", &invalid) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_text(app, h1, "Name") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, h2, "Score") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, c1, "Alice") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, c2, "95") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, table, 400, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect tr = {0}, hr = {0}, br = {0}, cr = {0};
    assert(cui_bridge_node_bounds(app, table, &tr) == CUI_BRIDGE_OK && tr.width > 0);
    assert(cui_bridge_node_bounds(app, head, &hr) == CUI_BRIDGE_OK && hr.height > 0);
    assert(cui_bridge_node_bounds(app, body, &br) == CUI_BRIDGE_OK && br.y > hr.y);
    assert(cui_bridge_node_bounds(app, c2, &cr) == CUI_BRIDGE_OK && cr.x > br.x);
    assert(cui_bridge_set_text(app, c2, "96") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, body) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, c2, &cr) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
static void test_data_table(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, table = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DATA_TABLE, "results", &table) ==
           CUI_BRIDGE_OK);
    char first[] = "Name";
    const char *headers[] = {first, "Score"};
    const char *cells[] = {"Alice", "95", "Bob", "87"};
    assert(cui_bridge_data_table_set(app, table, headers, 2, cells, 2) == CUI_BRIDGE_OK);
    first[0] = 'X';
    char copied[16] = "sentinel";
    uint32_t required = 0;
    assert(cui_bridge_data_table_get_cell(app, table, 0, 0, copied, 2, &required) ==
           CUI_BRIDGE_CAPACITY);
    assert(strcmp(copied, "sentinel") == 0 && required == 6);
    assert(cui_bridge_data_table_get_cell(app, table, 0, 0, copied, sizeof copied, &required) ==
           CUI_BRIDGE_OK);
    assert(strcmp(copied, "Alice") == 0);
    const char *invalid[] = {"okay", NULL};
    assert(cui_bridge_data_table_set(app, table, headers, 2, invalid, 1) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_data_table_get_cell(app, table, 1, 0, copied, sizeof copied, &required) ==
           CUI_BRIDGE_OK);
    assert(strcmp(copied, "Bob") == 0);
    assert(cui_bridge_set_visual_size(app, table, 320, 150) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect cell = {0}, header = {0};
    assert(cui_bridge_data_table_cell_bounds(app, table, 1, 0, &cell) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_header_bounds(app, table, 0, &header) == CUI_BRIDGE_OK);
    assert(cell.y > header.y && cell.width > 0);
    float x = cell.x + cell.width * .5f, y = cell.y + cell.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t row = 0, column = 0, sort_column = 0, sort_direction = 0;
    assert(cui_bridge_data_table_get_state(app, table, &row, &column, &sort_column,
                                           &sort_direction) == CUI_BRIDGE_OK);
    assert(row == 2);
    x = header.x + header.width * .5f;
    y = header.y + header.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_get_state(app, table, &row, &column, &sort_column,
                                           &sort_direction) == CUI_BRIDGE_OK);
    assert(sort_column == 1 && sort_direction != 0);
    cui_bridge_event_ex event = {.value = 123};
    char payload[128] = {0}, too_small[4] = "old";
    assert(cui_bridge_poll_event_ex(app, &event, too_small, sizeof too_small, &required) ==
           CUI_BRIDGE_CAPACITY);
    assert(event.value == 123 && strcmp(too_small, "old") == 0 && required > sizeof too_small);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == table.slot &&
           event.value == 2 && strstr(payload, "\"reason\":\"selection\"") &&
           strstr(payload, "\"row\":2") && strstr(payload, "\"sortColumn\":0"));
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(strstr(payload, "\"reason\":\"sort\"") && strstr(payload, "\"sortColumn\":1") &&
           event.value == 2);
    const char *reordered[] = {"Bob", "87", "Alice", "95"};
    assert(cui_bridge_data_table_set(app, table, headers, 2, reordered, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_get_cell(app, table, 0, 0, copied, sizeof copied, &required) ==
               CUI_BRIDGE_OK &&
           strcmp(copied, "Bob") == 0);
    assert(cui_bridge_data_table_set_selection_mode(app, table, 3) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_data_table_set_selection_mode(app, table, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_cell_bounds(app, table, 0, 1, &cell) == CUI_BRIDGE_OK);
    x = cell.x + cell.width * .5f;
    y = cell.y + cell.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_get_state(app, table, &row, &column, &sort_column,
                                           &sort_direction) == CUI_BRIDGE_OK);
    assert(row == 1 && column == 2);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(strstr(payload, "\"reason\":\"selection\"") && strstr(payload, "\"column\":2"));
    assert(cui_bridge_set_disabled(app, table, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t disabled_sort = 0;
    assert(cui_bridge_data_table_get_state(app, table, &row, &column, &sort_column,
                                           &disabled_sort) == CUI_BRIDGE_OK);
    assert(disabled_sort == sort_direction);
    assert(cui_bridge_node_remove(app, table) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_get_state(app, table, &row, &column, &sort_column,
                                           &sort_direction) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
static void test_plot(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, plot = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_PLOT, "samples", &plot) == CUI_BRIDGE_OK);
    cui_bridge_plot_point points[] = {{20, 20}, {60, 60}, {100, 30}};
    assert(cui_bridge_plot_set_points(app, plot, points, 3) == CUI_BRIDGE_OK);
    points[0].x = 999;
    cui_bridge_plot_point readback = {0};
    assert(cui_bridge_plot_get_point(app, plot, 0, &readback) == CUI_BRIDGE_OK && readback.x == 20);
    cui_bridge_plot_point invalid[] = {{0, 0}, {INFINITY, 1}};
    assert(cui_bridge_plot_set_points(app, plot, invalid, 2) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_plot_get_point(app, plot, 2, &readback) == CUI_BRIDGE_OK &&
           readback.x == 100);
    assert(cui_bridge_set_visual_size(app, plot, 240, 120) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, plot, &bounds) == CUI_BRIDGE_OK && bounds.width > 0);
    assert(cui_bridge_node_remove(app, plot) == CUI_BRIDGE_OK);
    assert(cui_bridge_plot_get_point(app, plot, 0, &readback) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
static void test_command(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, command = {0}, open = {0}, close = {0}, invalid = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COMMAND, "actions", &command) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, command, CUI_BRIDGE_BUTTON, "bad", &invalid) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_create(app, command, CUI_BRIDGE_ITEM, "open", &open) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, command, CUI_BRIDGE_ITEM, "close", &close) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, open, "Open") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, close, "Close") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, close, "exit") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, command, 300, 150) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_node selected = {0};
    assert(cui_bridge_get_selected(app, command, &selected) == CUI_BRIDGE_OK &&
           selected.slot == open.slot);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_command_item_bounds(app, command, close, &bounds) == CUI_BRIDGE_OK);
    float x = bounds.x + bounds.width * .5f, y = bounds.y + bounds.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, command, &selected) == CUI_BRIDGE_OK &&
           selected.slot == close.slot);
    cui_bridge_event_ex event = {0};
    char payload[4097] = {0};
    uint32_t required = 0;
    bool clicked = false;
    while (cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK)
        if (event.type == CUI_BRIDGE_EVENT_CLICK && event.node.slot == close.slot)
            clicked = true;
    assert(clicked);
    assert(cui_bridge_set_text(app, command, "exit") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, command, &selected) == CUI_BRIDGE_OK &&
           selected.slot == close.slot);
    assert(cui_bridge_set_disabled(app, close, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, command, &selected) == CUI_BRIDGE_OK && selected.slot == 0);
    assert(cui_bridge_node_remove(app, command) == CUI_BRIDGE_OK);
    assert(cui_bridge_command_item_bounds(app, command, close, &bounds) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
static void test_virtual_list(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, list = {0}, items[20] = {{0}};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_VIRTUAL_LIST, "items", &list) ==
           CUI_BRIDGE_OK);
    for (unsigned i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof key, "item-%u", i);
        assert(cui_bridge_node_create(app, list, CUI_BRIDGE_ITEM, key, &items[i]) == CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(app, items[i], key) == CUI_BRIDGE_OK);
    }
    assert(cui_bridge_set_visual_size(app, list, 200, 96) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect surface = {0}, first = {0}, last = {0};
    assert(cui_bridge_node_bounds(app, list, &surface) == CUI_BRIDGE_OK && surface.height > 0);
    assert(cui_bridge_virtual_list_item_bounds(app, list, items[0], &first) == CUI_BRIDGE_OK);
    assert(cui_bridge_virtual_list_item_bounds(app, list, items[19], &last) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_queue_scroll(app, surface.x + 20, surface.y + 20, 0, -1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    double offset = 0;
    assert(cui_bridge_get_value(app, list, &offset) == CUI_BRIDGE_OK && offset > 0);
    cui_bridge_event_ex event = {0};
    char text[8] = {0};
    uint32_t required = 0;
    assert(cui_bridge_poll_event_ex(app, &event, text, sizeof text, &required) == CUI_BRIDGE_OK &&
           event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == list.slot &&
           event.value > 0);
    assert(cui_bridge_set_value(app, list, 500) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_virtual_list_item_bounds(app, list, items[19], &last) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_disabled(app, list, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, list, &offset) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_scroll(app, surface.x + 20, surface.y + 20, 0, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    double disabled_offset = 0;
    assert(cui_bridge_get_value(app, list, &disabled_offset) == CUI_BRIDGE_OK &&
           disabled_offset == offset);
    assert(cui_bridge_node_remove(app, items[19]) == CUI_BRIDGE_OK);
    assert(cui_bridge_virtual_list_item_bounds(app, list, items[19], &last) ==
           CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
static void test_sidebar(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, sidebar = {0}, home = {0}, settings = {0}, invalid = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SIDEBAR, "navigation", &sidebar) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, sidebar, CUI_BRIDGE_BUTTON, "invalid", &invalid) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_create(app, sidebar, CUI_BRIDGE_ITEM, "home", &home) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, sidebar, CUI_BRIDGE_ITEM, "settings", &settings) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, settings, CUI_BRIDGE_BUTTON, "invalid", &invalid) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_text(app, home, "Home") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, settings, "Settings") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, sidebar, 240, 180) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, home, &bounds) == CUI_BRIDGE_OK && bounds.height > 0);
    press(app, settings);
    cui_bridge_node selected = {0};
    assert(cui_bridge_get_selected(app, sidebar, &selected) == CUI_BRIDGE_OK &&
           selected.slot == settings.slot);
    cui_bridge_event_ex event = {0};
    char payload[16] = {0};
    uint32_t required = 0;
    bool selected_event = false, clicked = false;
    while (cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK) {
        if (event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == sidebar.slot &&
            event.selected.slot == settings.slot)
            selected_event = true;
        if (event.type == CUI_BRIDGE_EVENT_CLICK && event.node.slot == settings.slot)
            clicked = true;
    }
    assert(selected_event && clicked);
    assert(cui_bridge_set_open(app, sidebar, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t open = 1;
    assert(cui_bridge_get_open(app, sidebar, &open) == CUI_BRIDGE_OK && open == 0);
    assert(cui_bridge_set_open(app, sidebar, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_disabled(app, home, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, home);
    assert(cui_bridge_get_selected(app, sidebar, &selected) == CUI_BRIDGE_OK &&
           selected.slot == settings.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_node_remove(app, sidebar) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_open(app, sidebar, &open) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
static void test_theme_presets(void) {
    uint32_t count = cui_bridge_theme_preset_count();
    assert(count >= 3);
    char name[128] = "untouched";
    uint32_t length = 777;
    assert(cui_bridge_theme_preset_name(0, name, 1, &length) == CUI_BRIDGE_CAPACITY);
    assert(strcmp(name, "untouched") == 0 && length == 777);
    assert(cui_bridge_theme_preset_name(count, name, sizeof name, &length) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_theme_preset_name(0, name, sizeof name, &length) == CUI_BRIDGE_OK);
    assert(strlen(name) == length && length > 0);
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 320,
                            .height = 240};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    uint32_t selected = 777;
    assert(cui_bridge_get_theme_preset(app, &selected) == CUI_BRIDGE_EMPTY && selected == 777);
    assert(cui_bridge_set_theme_preset(app, count) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_theme_preset(app, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_theme_preset(app, &selected) == CUI_BRIDGE_OK && selected == 0);
    cui_bridge_node root = {0}, button = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "button", &button) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, button, "Action") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_theme_preset(app, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_theme_preset(app, &selected) == CUI_BRIDGE_OK && selected == 1);
    cui_bridge_destroy(app);
}
static void test_root_view(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 320,
                            .height = 240};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, button = {0}, nested = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ROOT_VIEW, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ROOT_VIEW, "nested", &nested) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "button", &button) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, button, "Apply") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_root_padding(app, button, 5) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_root_padding(app, root, -1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_root_padding(app, root, 20) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, root, &bounds) == CUI_BRIDGE_OK);
    assert(bounds.width == 320 && bounds.height == 240);
    assert(cui_bridge_node_bounds(app, button, &bounds) == CUI_BRIDGE_OK && bounds.x >= 20);
    press(app, button);
    cui_bridge_event event = {0};
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_OK && event.node.slot == button.slot);
    assert(cui_bridge_set_root_padding(app, root, 40) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, button, &bounds) == CUI_BRIDGE_OK && bounds.x >= 40);
    cui_bridge_destroy(app);
}
static void test_clipboard(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 320,
                            .height = 240};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    char output[64] = "untouched";
    uint32_t length = 999;
    assert(cui_bridge_clipboard_set(app, "\xC0\xAF") == CUI_BRIDGE_INVALID_UTF8);
    char *large = malloc(65538), *readback = malloc(65537);
    assert(large && readback);
    for (size_t i = 0; i < 32768; i++) {
        large[i * 2] = (char)0xc3;
        large[i * 2 + 1] = (char)0xa9;
    }
    large[65536] = '\0';
    assert(cui_bridge_clipboard_set(app, large) == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    large[65536] = 'x';
    large[65537] = '\0';
    assert(cui_bridge_clipboard_set(app, large) == CUI_BRIDGE_CAPACITY);
    large[65536] = '\0';
    assert(cui_bridge_clipboard_set(app, "copy") == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(cui_bridge_clipboard_get(app, output, sizeof output, &length) ==
           CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(strcmp(output, "untouched") == 0 && length == 999);
    cui_bridge_destroy(app);
    const char *font = getenv("CUI_BRIDGE_NATIVE_FONT");
    if (!font || !font[0]) {
        free(large);
        free(readback);
        return;
    }
    o.backend = CUI_BRIDGE_BACKEND_SDL;
    o.font_path_utf8 = font;
    app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_clipboard_set(app, "copy \xE2\x9C\x93") == CUI_BRIDGE_OK);
    assert(cui_bridge_clipboard_get(app, output, 1, &length) == CUI_BRIDGE_CAPACITY);
    assert(strcmp(output, "untouched") == 0 && length == 999);
    assert(cui_bridge_clipboard_get(app, output, sizeof output, &length) == CUI_BRIDGE_OK);
    assert(length == strlen("copy \xE2\x9C\x93") && strcmp(output, "copy \xE2\x9C\x93") == 0);
    assert(cui_bridge_clipboard_set(app, large) == CUI_BRIDGE_OK);
    assert(cui_bridge_clipboard_get(app, readback, 65536, &length) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_clipboard_get(app, readback, 65537, &length) == CUI_BRIDGE_OK);
    assert(length == 65536 && memcmp(readback, large, 65537) == 0);
    assert(cui_bridge_clipboard_set(app, "") == CUI_BRIDGE_OK);
    assert(cui_bridge_clipboard_get(app, output, sizeof output, &length) == CUI_BRIDGE_OK);
    assert(length == 0 && output[0] == '\0');
    cui_bridge_destroy(app);
    free(large);
    free(readback);
}
static void test_speech(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 320,
                            .height = 240};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, speech = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SPEECH, "speech", &speech) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, speech, &bounds) == CUI_BRIDGE_OK && bounds.width > 0);
    assert(cui_bridge_speech_report(app, speech, "early", 1) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_speech_start(app, speech) == CUI_BRIDGE_OK);
    assert(cui_bridge_speech_start(app, speech) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_speech_report(app, speech, "bad\xC0\xAF", 0) == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_speech_report(app, speech, "hel", 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_speech_report(app, speech, "hello", 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    char transcript[32] = "untouched";
    uint32_t length = 777, checked = 0;
    assert(cui_bridge_speech_transcript(app, speech, transcript, 1, &length) ==
           CUI_BRIDGE_CAPACITY);
    assert(strcmp(transcript, "untouched") == 0 && length == 777);
    assert(cui_bridge_speech_transcript(app, speech, transcript, sizeof transcript, &length) ==
           CUI_BRIDGE_OK);
    assert(length == 5 && strcmp(transcript, "hello") == 0);
    assert(cui_bridge_get_checked(app, speech, &checked) == CUI_BRIDGE_OK && checked == 1);
    cui_bridge_event_ex event = {0};
    char payload[32] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == speech.slot && event.value == 0 && event.checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.value == 1 && strcmp(payload, "hel") == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.value == 1 && strcmp(payload, "hello") == 0);
    assert(cui_bridge_speech_finish(app, speech) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.value == 2 && event.checked == 0 && strcmp(payload, "hello") == 0);
    assert(cui_bridge_set_disabled(app, speech, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_speech_start(app, speech) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, speech) == CUI_BRIDGE_OK);
    assert(cui_bridge_speech_start(app, speech) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
static void test_questionnaire(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 480,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, form = {0}, q1 = {0}, q2 = {0};
    cui_bridge_node a = {0}, b = {0}, c = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_QUESTIONNAIRE, "survey", &form) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, form, CUI_BRIDGE_ITEM, "color", &q1) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, q1, CUI_BRIDGE_ITEM, "red", &a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, q1, CUI_BRIDGE_ITEM, "blue", &b) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, form, CUI_BRIDGE_ITEM, "shape", &q2) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, q2, CUI_BRIDGE_ITEM, "round", &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, q1, "Choose a color") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, a, "Red") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, b, "Blue") == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_set_question(app, q1, 1, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_set_question(app, q2, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_node current = {0};
    uint32_t complete = 7, selected = 7;
    assert(cui_bridge_questionnaire_get_state(app, form, &current, &complete) == CUI_BRIDGE_OK);
    assert(current.slot == q1.slot && complete == 0);
    assert(cui_bridge_questionnaire_next(app, form) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_questionnaire_submit(app, form) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_text(app, a, "Changed") == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, b) == CUI_BRIDGE_INVALID_STATE);
    press(app, a);
    press(app, b);
    assert(cui_bridge_questionnaire_choice_selected(app, b, &selected) == CUI_BRIDGE_OK &&
           selected == 1);
    assert(cui_bridge_questionnaire_choice_selected(app, a, &selected) == CUI_BRIDGE_OK &&
           selected == 0);
    cui_bridge_event_ex event = {0};
    char payload[128] = {0};
    uint32_t needed = 0;
    event.value = 777;
    assert(cui_bridge_poll_event_ex(app, &event, payload, 1, &needed) == CUI_BRIDGE_CAPACITY);
    assert(event.value == 777 && needed > 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == form.slot && strstr(payload, "\"selected\":[1]") != NULL);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == form.slot && strstr(payload, "\"selected\":[2]") != NULL);
    assert(cui_bridge_questionnaire_next(app, form) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_get_state(app, form, &current, &complete) == CUI_BRIDGE_OK);
    assert(current.slot == q2.slot && complete == 0);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, c, &(cui_bridge_rect){0}) == CUI_BRIDGE_OK);
    press(app, c);
    assert(cui_bridge_questionnaire_previous(app, form) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_get_state(app, form, &current, &complete) == CUI_BRIDGE_OK);
    assert(current.slot == q1.slot);
    assert(cui_bridge_questionnaire_submit(app, form) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_submit(app, form) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_get_state(app, form, &current, &complete) == CUI_BRIDGE_OK &&
           complete == 1);
    unsigned completed = 0, submitted = 0;
    while (cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK) {
        if (event.value == 2) {
            completed++;
            assert(strstr(payload, "\"complete\":1") != NULL);
        }
        if (event.value == 3) {
            submitted++;
            assert(strstr(payload, "\"complete\":1") != NULL);
        }
    }
    assert(completed == 1 && submitted == 2);
    assert(cui_bridge_node_remove(app, form) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_get_state(app, form, &current, &complete) ==
           CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
static void test_dock(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, dock = {0}, first = {0}, second = {0};
    cui_bridge_node content = {0}, current = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DOCK, "dock", &dock) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, dock, CUI_BRIDGE_ITEM, "one", &first) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, dock, CUI_BRIDGE_ITEM, "two", &second) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, first, "First") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, second, "Second") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, first, CUI_BRIDGE_LABEL, "content", &content) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, content, "Panel body") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, dock, 480, 300) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_get_active(app, dock, &current) == CUI_BRIDGE_OK &&
           current.slot == first.slot);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_dock_panel_bounds(app, dock, second, &bounds) == CUI_BRIDGE_OK &&
           bounds.width > 0);
    assert(cui_bridge_set_text(app, second, "Later") == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, second) == CUI_BRIDGE_INVALID_STATE);
    char before_model[4096] = {0};
    uint32_t before_required = 0;
    assert(cui_bridge_dock_serialize(app, dock, before_model, sizeof before_model,
                                     &before_required) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_move_split(app, dock, second, first, 2, .5, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_dock_move_split(app, dock, second, first, 0, .4, 0) == CUI_BRIDGE_OK);
    char model[4096] = "untouched";
    uint32_t required = 0;
    assert(cui_bridge_dock_serialize(app, dock, model, 1, &required) == CUI_BRIDGE_CAPACITY);
    assert(strcmp(model, "untouched") == 0 && required > 1);
    assert(cui_bridge_dock_serialize(app, dock, model, sizeof model, &required) == CUI_BRIDGE_OK);
    assert(strlen(model) + 1 == required && strcmp(model, before_model) != 0);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_activate(app, dock, second) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_get_active(app, dock, &current) == CUI_BRIDGE_OK &&
           current.slot == second.slot);
    cui_bridge_event_ex event = {0};
    char payload[8] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == dock.slot && event.selected.slot == second.slot &&
           event.selected.generation == second.generation);
    assert(cui_bridge_set_disabled(app, dock, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_activate(app, dock, first) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, dock) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_get_active(app, dock, &current) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
static void test_settings(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 900,
                            .height = 650};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, settings = {0}, general = {0}, advanced = {0};
    cui_bridge_node group = {0}, other_group = {0}, toggle = {0}, checkbox = {0};
    cui_bridge_node input = {0}, number = {0}, current = {0};
    cui_bridge_node second_settings = {0}, second_page = {0};
    cui_bridge_node second_group = {0}, second_toggle = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SETTINGS, "settings", &settings) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, settings, CUI_BRIDGE_ITEM, "general", &general) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, general, CUI_BRIDGE_ITEM, "group", &group) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_SWITCH, "switch", &toggle) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_CHECKBOX, "check", &checkbox) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_INPUT, "name", &input) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_NUMBER_INPUT, "count", &number) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, settings, CUI_BRIDGE_ITEM, "advanced", &advanced) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, advanced, CUI_BRIDGE_ITEM, "other", &other_group) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, general, "General") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, advanced, "Advanced") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, group, "Preferences") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, toggle, "Enable") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, checkbox, "Confirm") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, input, "Name") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, number, "Count") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, settings, 720, 500) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SETTINGS, "second-settings",
                                  &second_settings) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, second_settings, CUI_BRIDGE_ITEM, "second-page",
                                  &second_page) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, second_page, CUI_BRIDGE_ITEM, "second-group",
                                  &second_group) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, second_group, CUI_BRIDGE_SWITCH, "second-switch",
                                  &second_toggle) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, second_settings, 300, 200) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_settings_get_page(app, settings, &current) == CUI_BRIDGE_OK &&
           current.slot == general.slot);
    assert(cui_bridge_settings_get_page(app, second_settings, &current) == CUI_BRIDGE_OK &&
           current.slot == second_page.slot);
    assert(cui_bridge_node_remove(app, group) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_text(app, general, "Changed") == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_detail_text(app, general, "Changed") == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_settings_field_bounds(app, settings, toggle, &bounds) == CUI_BRIDGE_OK &&
           bounds.width > 0 && bounds.height > 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t checked = 0;
    assert(cui_bridge_get_checked(app, toggle, &checked) == CUI_BRIDGE_OK && checked == 1);
    cui_bridge_event_ex event = {0};
    char payload[16] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == toggle.slot && event.checked == 1);
    assert(cui_bridge_settings_field_bounds(app, settings, input, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "Zoë") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    char input_text[16] = {0};
    assert(cui_bridge_get_text(app, input, input_text, sizeof input_text, &needed) ==
           CUI_BRIDGE_OK);
    assert(strcmp(input_text, "Zoë") == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == input.slot && strcmp(payload, "Zoë") == 0);
    assert(cui_bridge_set_disabled(app, checkbox, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_settings_field_bounds(app, settings, checkbox, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_checked(app, checkbox, &checked) == CUI_BRIDGE_OK && checked == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_settings_set_page(app, settings, advanced) == CUI_BRIDGE_OK);
    assert(cui_bridge_settings_get_page(app, settings, &current) == CUI_BRIDGE_OK &&
           current.slot == advanced.slot);
    assert(cui_bridge_settings_set_page(app, settings, general) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == settings.slot && event.selected.slot == advanced.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == settings.slot && event.selected.slot == general.slot);
    assert(cui_bridge_settings_reset_page(app, settings, general) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_checked(app, toggle, &checked) == CUI_BRIDGE_OK && checked == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == toggle.slot && event.checked == 0);
    bool saw_input_reset = false;
    while (cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) == CUI_BRIDGE_OK)
        if (event.node.slot == input.slot) {
            saw_input_reset = true;
            assert(event.text_length == 0);
        }
    assert(saw_input_reset);
    assert(cui_bridge_get_text(app, input, input_text, sizeof input_text, &needed) ==
               CUI_BRIDGE_OK &&
           strcmp(input_text, "") == 0);
    assert(cui_bridge_set_text(app, input, "Host") == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, input, input_text, sizeof input_text, &needed) ==
               CUI_BRIDGE_OK &&
           strcmp(input_text, "Host") == 0);
    assert(cui_bridge_set_value(app, number, 7) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    double number_value = 0;
    assert(cui_bridge_get_value(app, number, &number_value) == CUI_BRIDGE_OK && number_value == 7);
    assert(cui_bridge_settings_reset_page(app, settings, general) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, number, &number_value) == CUI_BRIDGE_OK && number_value == 0);
    while (cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK) {
    }
    char too_long[258];
    memset(too_long, 'a', sizeof too_long - 1);
    too_long[257] = 0;
    assert(cui_bridge_settings_set_query(app, settings, too_long) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_settings_set_query(app, settings, "\xC0") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_settings_set_query(app, settings, "Advanced") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    bounds = (cui_bridge_rect){11, 12, 13, 14};
    assert(cui_bridge_settings_field_bounds(app, settings, toggle, &bounds) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(bounds.x == 11 && bounds.y == 12 && bounds.width == 13 && bounds.height == 14);
    assert(cui_bridge_settings_get_page(app, second_settings, &current) == CUI_BRIDGE_OK &&
           current.slot == second_page.slot);
    assert(cui_bridge_get_checked(app, second_toggle, &checked) == CUI_BRIDGE_OK && checked == 0);
    assert(cui_bridge_node_remove(app, settings) == CUI_BRIDGE_OK);
    assert(cui_bridge_settings_get_page(app, settings, &current) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
    o.max_events = 1;
    app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    root = settings = general = group = toggle = checkbox = (cui_bridge_node){0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SETTINGS, "settings", &settings) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, settings, CUI_BRIDGE_ITEM, "page", &general) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, general, CUI_BRIDGE_ITEM, "group", &group) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_SWITCH, "one", &toggle) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_CHECKBOX, "two", &checkbox) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_checked(app, toggle, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_settings_reset_page(app, settings, general) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_get_checked(app, toggle, &checked) == CUI_BRIDGE_OK && checked == 1);
    cui_bridge_destroy(app);
}
static void test_model_event_capacity(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480,
                            .max_events = 1};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, dock = {0}, first = {0}, second = {0}, current = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DOCK, "dock", &dock) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, dock, CUI_BRIDGE_ITEM, "first", &first) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, dock, CUI_BRIDGE_ITEM, "second", &second) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_activate(app, dock, second) == CUI_BRIDGE_OK);
    assert(cui_bridge_dock_activate(app, dock, first) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_dock_get_active(app, dock, &current) == CUI_BRIDGE_OK &&
           current.slot == second.slot);
    cui_bridge_event_ex event = {0};
    char payload[128] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == dock.slot && event.selected.slot == second.slot);
    assert(cui_bridge_dock_activate(app, dock, first) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.selected.slot == first.slot && event.selected.generation == first.generation);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);

    app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    root = (cui_bridge_node){0};
    cui_bridge_node survey = {0}, q1 = {0}, q2 = {0}, choice_a = {0}, choice_b = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_QUESTIONNAIRE, "survey", &survey) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, survey, CUI_BRIDGE_ITEM, "one", &q1) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, q1, CUI_BRIDGE_ITEM, "choice-one", &choice_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, survey, CUI_BRIDGE_ITEM, "two", &q2) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, q2, CUI_BRIDGE_ITEM, "choice-two", &choice_b) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, choice_a);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_next(app, survey) == CUI_BRIDGE_OK);
    assert(cui_bridge_questionnaire_previous(app, survey) == CUI_BRIDGE_CAPACITY);
    uint32_t complete = 0;
    assert(cui_bridge_questionnaire_get_state(app, survey, &current, &complete) == CUI_BRIDGE_OK &&
           current.slot == q2.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == survey.slot && strstr(payload, "\"current\":2") != NULL);
    assert(cui_bridge_questionnaire_previous(app, survey) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(strstr(payload, "\"current\":1") != NULL);
    cui_bridge_destroy(app);

    app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    root = (cui_bridge_node){0};
    cui_bridge_node speech_a = {0}, speech_b = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SPEECH, "a", &speech_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SPEECH, "b", &speech_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_speech_start(app, speech_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_speech_start(app, speech_b) == CUI_BRIDGE_CAPACITY);
    uint32_t active = 99;
    assert(cui_bridge_get_checked(app, speech_b, &active) == CUI_BRIDGE_OK && active == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == speech_a.slot);
    assert(cui_bridge_speech_start(app, speech_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == speech_b.slot && event.checked == 1);
    cui_bridge_destroy(app);
}
int main(void) {
    test_nested_group_stack();
    test_chrome();
    test_message_icon();
    test_attachment();
    test_image();
    test_tree();
    test_time_field();
    test_color_picker();
    test_date_pickers();
    test_otp();
    test_form();
    test_input_group();
    test_notification();
    test_focus_trap();
    test_message_scroller();
    test_carousel();
    test_resizable();
    test_text_view();
    test_editor();
    test_chart();
    test_table();
    test_data_table();
    test_plot();
    test_command();
    test_virtual_list();
    test_sidebar();
    test_theme_presets();
    test_root_view();
    test_clipboard();
    test_speech();
    test_questionnaire();
    test_dock();
    test_settings();
    test_model_event_capacity();
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, first = {0}, second = {0}, slider = {0}, progress = {0};
    cui_bridge_node input = {0}, area = {0}, number = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CHECKBOX, "first", &first) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CHECKBOX, "second", &second) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SLIDER, "slider", &slider) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_PROGRESS, "progress", &progress) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_INPUT, "input", &input) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TEXTAREA, "area", &area) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_NUMBER_INPUT, "number", &number) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, first, "First \xE2\x9C\x93") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, first, "\xC0\xAF") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_text(app, second, "Second") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_range(app, slider, 10, 100, 1) == CUI_BRIDGE_OK);
    double value = -1;
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == 10);
    assert(cui_bridge_set_range(app, slider, -50, -10, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == -10);
    assert(cui_bridge_set_range(app, slider, -INFINITY, 10, 1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_range(app, slider, 0, 1e100, 1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == -10);
    assert(cui_bridge_set_range(app, slider, 0, 10, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, slider, 5) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, progress, .5) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, slider, NAN) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_text(app, input, "hello") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, area, "first\nsecond") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, number, "Count") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_range(app, number, 0, 20, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, number, 10) == CUI_BRIDGE_OK);
    char text[32] = "preserved";
    uint32_t needed = 0;
    assert(cui_bridge_get_text(app, input, text, 3, &needed) == CUI_BRIDGE_CAPACITY);
    assert(needed == 6 && text[0] == 'p');
    assert(cui_bridge_get_text(app, input, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(needed == 6 && text[0] == 'h');
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == 5);
    assert(cui_bridge_get_value(app, number, &value) == CUI_BRIDGE_OK && value == 10);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, first);
    uint32_t checked = 0;
    assert(cui_bridge_get_checked(app, first, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_get_checked(app, second, &checked) == CUI_BRIDGE_OK && checked == 0);
    cui_bridge_event_ex event = {0};
    cui_bridge_node selected = {0};
    char payload[32] = {0};
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == first.slot &&
           event.frame_sequence && event.checked == 1 && needed == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    press(app, first);
    press(app, first);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.checked == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.checked == 1);
    assert(cui_bridge_queue_text(app, "\xC0\xAF") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_disabled(app, input, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, input);
    assert(cui_bridge_queue_text(app, "X") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, input, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(needed == 6);
    assert(cui_bridge_set_disabled(app, input, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, input);
    assert(cui_bridge_queue_text(app, "Z") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, input, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(needed == 7);
    assert(cui_bridge_poll_event_ex(app, &event, payload, 2, &needed) == CUI_BRIDGE_CAPACITY);
    assert(needed == 7);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == input.slot &&
           event.text_length == 6 && needed == 7);
    assert(cui_bridge_queue_text(app, "Y") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "Q") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.text_length == 7 && needed == 8);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.text_length == 8 && needed == 9);
    assert(cui_bridge_set_disabled(app, second, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, second);
    assert(cui_bridge_get_checked(app, second, &checked) == CUI_BRIDGE_OK && checked == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_node_remove(app, first) == CUI_BRIDGE_OK);
    checked = 99;
    assert(cui_bridge_get_checked(app, first, &checked) == CUI_BRIDGE_STALE_HANDLE &&
           checked == 99);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node trigger = {0}, dialog = {0}, dialog_body = {0};
    cui_bridge_node alert_dialog = {0}, sheet = {0}, popover = {0}, popover_body = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "trigger", &trigger) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, trigger, "Open") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DIALOG, "dialog", &dialog) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, dialog, CUI_BRIDGE_LABEL, "dialog-body", &dialog_body) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, dialog, "Dialog") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, dialog_body, "Body") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ALERT_DIALOG, "alert-dialog",
                                  &alert_dialog) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, alert_dialog, "Confirm") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, alert_dialog, "Proceed?") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SHEET, "sheet", &sheet) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_POPOVER, "popover", &popover) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, popover, CUI_BRIDGE_LABEL, "popover-body", &popover_body) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, popover_body, "Popover body") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, popover, 1) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_anchor(app, popover, popover) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_anchor(app, popover, trigger) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, dialog, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t open = 0;
    assert(cui_bridge_get_open(app, dialog, &open) == CUI_BRIDGE_OK && open == 1);
    cui_bridge_rect surface_bounds = {0};
    assert(cui_bridge_node_bounds(app, dialog_body, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, dialog, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, dialog, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ESCAPE, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event_ex surface_event = {0};
    char surface_payload[16] = {0};
    uint32_t surface_needed = 0;
    assert(cui_bridge_poll_event_ex(app, &surface_event, surface_payload, sizeof surface_payload,
                                    &surface_needed) == CUI_BRIDGE_OK);
    assert(surface_event.type == CUI_BRIDGE_EVENT_CLOSE && surface_event.node.slot == dialog.slot &&
           surface_event.value == CUI_BRIDGE_SURFACE_CLOSED);
    assert(cui_bridge_poll_event_ex(app, &surface_event, surface_payload, sizeof surface_payload,
                                    &surface_needed) == CUI_BRIDGE_OK);
    assert(surface_event.type == CUI_BRIDGE_EVENT_CLOSE && surface_event.node.slot == dialog.slot &&
           surface_event.value == CUI_BRIDGE_SURFACE_CANCELLED);
    assert(cui_bridge_set_open(app, alert_dialog, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ESCAPE, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &surface_event, surface_payload, sizeof surface_payload,
                                    &surface_needed) == CUI_BRIDGE_OK);
    assert(surface_event.type == CUI_BRIDGE_EVENT_CLOSE &&
           surface_event.node.slot == alert_dialog.slot && surface_event.value == 2);
    assert(cui_bridge_set_open(app, sheet, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_open(app, sheet, &open) == CUI_BRIDGE_OK && open == 1);
    assert(cui_bridge_set_open(app, sheet, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &surface_event, surface_payload, sizeof surface_payload,
                                    &surface_needed) == CUI_BRIDGE_OK);
    assert(surface_event.type == CUI_BRIDGE_EVENT_CLOSE && surface_event.node.slot == sheet.slot);
    assert(cui_bridge_set_open(app, popover, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, popover_body, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, popover, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &surface_event, surface_payload, sizeof surface_payload,
                                    &surface_needed) == CUI_BRIDGE_OK);
    assert(surface_event.type == CUI_BRIDGE_EVENT_CLOSE && surface_event.node.slot == popover.slot);
    assert(cui_bridge_node_remove(app, trigger) == CUI_BRIDGE_OK);
    cui_bridge_node trigger_replacement = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "trigger-again",
                                  &trigger_replacement) == CUI_BRIDGE_OK);
    assert(trigger_replacement.slot == trigger.slot &&
           trigger_replacement.generation != trigger.generation);
    assert(cui_bridge_set_open(app, popover, 1) == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node hover_trigger = {0}, tooltip = {0}, card = {0}, card_body = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "hover-trigger", &hover_trigger) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, hover_trigger, "Hover here") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TOOLTIP, "tooltip", &tooltip) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_HOVER_CARD, "card", &card) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, card, CUI_BRIDGE_LABEL, "card-body", &card_body) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, tooltip, "Helpful tip") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, card, "More details") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, card_body, "Extra context") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_anchor(app, tooltip, hover_trigger) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_anchor(app, card, hover_trigger) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, hover_trigger, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(
               app, CUI_BRIDGE_POINTER_MOVE, surface_bounds.x + surface_bounds.width * .5f,
               surface_bounds.y + surface_bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_open(app, tooltip, &open) == CUI_BRIDGE_OK && open == 1);
    assert(cui_bridge_get_open(app, card, &open) == CUI_BRIDGE_OK && open == 1);
    assert(cui_bridge_node_bounds(app, card_body, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &surface_event, surface_payload, sizeof surface_payload,
                                    &surface_needed) == CUI_BRIDGE_OK);
    assert(surface_event.type == CUI_BRIDGE_EVENT_CHANGE && surface_event.checked == 1);
    assert(cui_bridge_poll_event_ex(app, &surface_event, surface_payload, sizeof surface_payload,
                                    &surface_needed) == CUI_BRIDGE_OK);
    assert(surface_event.type == CUI_BRIDGE_EVENT_CHANGE && surface_event.checked == 1);
    assert(cui_bridge_set_disabled(app, tooltip, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, 590, 390) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_open(app, tooltip, &open) == CUI_BRIDGE_OK && open == 0);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    const cui_bridge_kind visuals[] = {
        CUI_BRIDGE_AVATAR, CUI_BRIDGE_BUBBLE,   CUI_BRIDGE_EMPTY_VIEW, CUI_BRIDGE_GROUP_BOX,
        CUI_BRIDGE_KBD,    CUI_BRIDGE_SKELETON, CUI_BRIDGE_SPINNER,    CUI_BRIDGE_TAG,
        CUI_BRIDGE_MARKER, CUI_BRIDGE_SHIMMER};
    cui_bridge_node visual_nodes[sizeof visuals / sizeof visuals[0]] = {{0}};
    for (size_t i = 0; i < sizeof visuals / sizeof visuals[0]; i++) {
        char key[20];
        snprintf(key, sizeof key, "visual-%zu", i);
        assert(cui_bridge_node_create(app, root, visuals[i], key, &visual_nodes[i]) ==
               CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(app, visual_nodes[i], "Sample") == CUI_BRIDGE_OK);
    }
    assert(cui_bridge_set_detail_text(app, visual_nodes[2], "Nothing found") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, visual_nodes[0], 32, 32) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, visual_nodes[5], 100, 12) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, visual_nodes[5], -1, 12) == CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_node inside_group = {0};
    assert(cui_bridge_node_create(app, visual_nodes[3], CUI_BRIDGE_LABEL, "group-child",
                                  &inside_group) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, inside_group, "Grouped") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    for (size_t i = 0; i < sizeof visuals / sizeof visuals[0]; i++)
        assert(cui_bridge_node_bounds(app, visual_nodes[i], &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, inside_group, &surface_bounds) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node list = {0}, list_a = {0}, list_b = {0};
    cui_bridge_node menu = {0}, menu_a = {0};
    cui_bridge_node pagination = {0}, rating = {0}, stepper = {0}, step_a = {0}, step_b = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_LIST, "list", &list) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, list, CUI_BRIDGE_ITEM, "list-a", &list_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, list, CUI_BRIDGE_ITEM, "list-b", &list_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, list_a, "Alpha") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, list_b, "Beta") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_MENU, "menu", &menu) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, menu, CUI_BRIDGE_ITEM, "menu-a", &menu_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, menu, "Actions") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, menu_a, "Export") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_PAGINATION, "pagination", &pagination) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_total(app, pagination, 20) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, pagination, 10) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RATING, "rating", &rating) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_total(app, rating, 5) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, rating, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_STEPPER, "stepper", &stepper) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, stepper, CUI_BRIDGE_ITEM, "step-a", &step_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, stepper, CUI_BRIDGE_ITEM, "step-b", &step_b) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, step_a, "Start") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, step_b, "Finish") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, step_b, "Complete") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_step_status(app, step_a, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, list_b);
    assert(cui_bridge_get_selected(app, list, &selected) == CUI_BRIDGE_OK &&
           selected.slot == list_b.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.selected.slot == list_b.slot);
    press(app, menu);
    assert(cui_bridge_node_bounds(app, menu_a, &surface_bounds) == CUI_BRIDGE_OK);
    press(app, menu_a);
    assert(cui_bridge_get_selected(app, menu, &selected) == CUI_BRIDGE_OK &&
           selected.slot == menu_a.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.selected.slot == menu_a.slot);
    assert(cui_bridge_numeric_part_bounds(app, pagination, 3, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(
               app, CUI_BRIDGE_POINTER_DOWN, surface_bounds.x + surface_bounds.width * .5f,
               surface_bounds.y + surface_bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(
               app, CUI_BRIDGE_POINTER_UP, surface_bounds.x + surface_bounds.width * .5f,
               surface_bounds.y + surface_bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    double numeric_value = 0;
    assert(cui_bridge_get_value(app, pagination, &numeric_value) == CUI_BRIDGE_OK &&
           numeric_value == 11);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == pagination.slot && event.value == 11);
    assert(cui_bridge_numeric_part_bounds(app, rating, 14, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(
               app, CUI_BRIDGE_POINTER_DOWN, surface_bounds.x + surface_bounds.width * .5f,
               surface_bounds.y + surface_bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(
               app, CUI_BRIDGE_POINTER_UP, surface_bounds.x + surface_bounds.width * .5f,
               surface_bounds.y + surface_bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, rating, &numeric_value) == CUI_BRIDGE_OK &&
           numeric_value == 5);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == rating.slot && event.value == 5);
    assert(cui_bridge_numeric_part_bounds(app, stepper, 2, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(
               app, CUI_BRIDGE_POINTER_DOWN, surface_bounds.x + surface_bounds.width * .5f,
               surface_bounds.y + surface_bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(
               app, CUI_BRIDGE_POINTER_UP, surface_bounds.x + surface_bounds.width * .5f,
               surface_bounds.y + surface_bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, stepper, &selected) == CUI_BRIDGE_OK &&
           selected.slot == step_b.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == stepper.slot && event.selected.slot == step_b.slot);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node descriptions = {0}, description_item = {0};
    cui_bridge_node scrollable = {0}, scroll_content = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DESCRIPTION_LIST, "descriptions",
                                  &descriptions) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, descriptions, CUI_BRIDGE_ITEM, "description",
                                  &description_item) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, description_item, "Name") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, description_item, "Matteo") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SCROLLABLE, "scrollable", &scrollable) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, scrollable, CUI_BRIDGE_LABEL, "scroll-content",
                                  &scroll_content) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, scroll_content, "Scrollable content") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, scrollable, 180, 60) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_scroll_extent(app, scrollable, 180, 300) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, descriptions, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, scrollable, &surface_bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_scroll(app, surface_bounds.x + 20, surface_bounds.y + 20, 0, -1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    double offset = 0;
    assert(cui_bridge_get_value(app, scrollable, &offset) == CUI_BRIDGE_OK && offset > 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == scrollable.slot && event.value > 0);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node radio = {0}, sw = {0}, toggle = {0}, badge = {0};
    cui_bridge_node collapsible = {0}, panel_label = {0}, alert = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RADIO, "radio", &radio) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SWITCH, "switch", &sw) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TOGGLE, "toggle", &toggle) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BADGE, "badge", &badge) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLLAPSIBLE, "details", &collapsible) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, collapsible, CUI_BRIDGE_LABEL, "inside", &panel_label) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ALERT, "alert", &alert) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, radio, "Radio") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, sw, "Switch") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, toggle, "Toggle") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, badge, "New") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, collapsible, "Details") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, panel_label, "Panel body") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, alert, "Heads up") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, alert, "Action is required") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, badge, &bounds) == CUI_BRIDGE_OK && bounds.width > 0);
    assert(cui_bridge_node_bounds(app, panel_label, &bounds) == CUI_BRIDGE_INVALID_STATE);
    press(app, collapsible);
    assert(cui_bridge_get_checked(app, collapsible, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == collapsible.slot && event.checked == 1);
    assert(cui_bridge_node_bounds(app, panel_label, &bounds) == CUI_BRIDGE_OK);
    press(app, radio);
    assert(cui_bridge_get_checked(app, radio, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == radio.slot && event.checked == 1);
    press(app, sw);
    assert(cui_bridge_get_checked(app, sw, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == sw.slot && event.checked == 1);
    press(app, toggle);
    assert(cui_bridge_get_checked(app, toggle, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == toggle.slot && event.checked == 1);
    assert(cui_bridge_set_disabled(app, sw, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, sw);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node select = {0}, select_a = {0}, select_b = {0};
    cui_bridge_node combo = {0}, combo_a = {0};
    cui_bridge_node dropdown = {0}, dropdown_a = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SELECT, "select", &select) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, select, CUI_BRIDGE_ITEM, "select-a", &select_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, select, CUI_BRIDGE_ITEM, "select-b", &select_b) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, select, "Select one") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, select, "Choose") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, select_a, "Apple") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, select_b, "Banana") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COMBOBOX, "combo", &combo) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, combo, CUI_BRIDGE_ITEM, "combo-a", &combo_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, combo, "Search") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, combo_a, "Cherry") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DROPDOWN_BUTTON, "dropdown", &dropdown) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, dropdown, CUI_BRIDGE_ITEM, "dropdown-a", &dropdown_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, dropdown, "Run") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, dropdown, "More") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, dropdown_a, "Export") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, select);
    assert(cui_bridge_node_bounds(app, select_b, &bounds) == CUI_BRIDGE_OK);
    press(app, select_b);
    assert(cui_bridge_get_selected(app, select, &selected) == CUI_BRIDGE_OK &&
           selected.slot == select_b.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.selected.slot == select_b.slot &&
           event.selected.generation == select_b.generation);
    assert(cui_bridge_set_text(app, combo, "Ch") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, combo, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 3);
    press(app, combo);
    assert(cui_bridge_queue_text(app, "e") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, combo, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 4);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == combo.slot && event.text_length == 3);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ESCAPE, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_dropdown_part_bounds(app, dropdown, 0, &bounds) == CUI_BRIDGE_OK &&
           bounds.width > 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width * .25f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width * .25f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event legacy = {0};
    assert(cui_bridge_poll_event(app, &legacy) == CUI_BRIDGE_OK);
    assert(legacy.type == CUI_BRIDGE_EVENT_CLICK && legacy.node.slot == dropdown.slot);
    assert(cui_bridge_dropdown_part_bounds(app, dropdown, 1, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, dropdown_a, &bounds) == CUI_BRIDGE_OK);
    press(app, dropdown_a);
    assert(cui_bridge_get_selected(app, dropdown, &selected) == CUI_BRIDGE_OK &&
           selected.slot == dropdown_a.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.selected.slot == dropdown_a.slot);
    assert(cui_bridge_set_disabled(app, select, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, select);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_get_selected(app, select, &selected) == CUI_BRIDGE_OK &&
           selected.slot == select_b.slot);
    assert(cui_bridge_set_text(app, combo, "ZZ") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, combo);
    assert(cui_bridge_node_bounds(app, combo_a, &bounds) == CUI_BRIDGE_INVALID_STATE);
    char too_long[4099];
    for (size_t i = 0; i < sizeof too_long - 1; i++)
        too_long[i] = 'x';
    too_long[sizeof too_long - 1] = 0;
    assert(cui_bridge_set_text(app, combo, too_long) == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_get_text(app, combo, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 3);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node group = {0}, option_a = {0}, option_b = {0};
    cui_bridge_node tabs = {0}, tab_a = {0}, tab_b = {0}, panel = {0};
    cui_bridge_node accordion = {0}, fold = {0}, fold_content = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RADIO_GROUP, "group", &group) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "a", &option_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "b", &option_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, option_a, "A") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, option_b, "B") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TABS, "tabs", &tabs) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tabs, CUI_BRIDGE_ITEM, "tab-a", &tab_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tabs, CUI_BRIDGE_ITEM, "tab-b", &tab_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tab_b, CUI_BRIDGE_LABEL, "panel", &panel) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, tab_a, "First tab") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, tab_b, "Second tab") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, panel, "Second content") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ACCORDION, "accordion", &accordion) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, accordion, CUI_BRIDGE_ITEM, "fold", &fold) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, fold, CUI_BRIDGE_LABEL, "fold-content", &fold_content) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, fold, "Expand") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, fold_content, "Expanded content") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, option_a);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == option_a.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == group.slot && event.value == option_a.slot &&
           event.selected.slot == option_a.slot &&
           event.selected.generation == option_a.generation);
    press(app, option_b);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == option_b.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.value == option_b.slot && event.selected.generation == option_b.generation);
    assert(cui_bridge_set_selected(app, tabs, tab_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, panel, &bounds) == CUI_BRIDGE_INVALID_STATE);
    press(app, tab_b);
    assert(cui_bridge_get_selected(app, tabs, &selected) == CUI_BRIDGE_OK &&
           selected.slot == tab_b.slot);
    assert(cui_bridge_node_bounds(app, panel, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == tabs.slot && event.value == tab_b.slot &&
           event.selected.generation == tab_b.generation);
    press(app, fold);
    assert(cui_bridge_get_checked(app, fold, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_node_bounds(app, fold_content, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == fold.slot && event.checked == 1);
    assert(cui_bridge_node_remove(app, option_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK && selected.slot == 0);
    cui_bridge_node replacement = {0};
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "replacement", &replacement) ==
           CUI_BRIDGE_OK);
    assert(replacement.slot == option_b.slot && replacement.generation != option_b.generation);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK && selected.slot == 0);
    assert(cui_bridge_set_selected(app, group, option_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_disabled(app, option_a, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, option_a);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == option_a.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
    puts("bridge components ok");
}
