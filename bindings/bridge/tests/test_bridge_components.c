/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
