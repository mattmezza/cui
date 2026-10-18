/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_id selected = 10;
static cui_choice_popup popup;
static cui_choice_item items[] = {
    {10, {"First", 5}, false}, {20, {"Disabled", 8}, true}, {30, {"Last", 4}, false}};
static cui_menu_command copies[32];
static size_t calls, painted;
static bool redirect, consume;
static float max_height;
static void body(cui_context *context, const cui_choice_item *item, size_t index, void *user) {
    assert(context == ui && user == &painted && index < 3 && item->id == items[index].id);
    painted++;
    cui_text(ui, cui_string("Custom row"), 12, (cui_color){10, 20, 30, 255});
}
static void command(void *user, const cui_menu_command *event) {
    assert(user == &calls && calls < 32 && popup.overlay.open);
    assert(event->event.target == 2 && event->event.kind == CUI_COMPONENT_EVENT_ACTIVATE);
    assert(event->command == selected);
    copies[calls++] = *event;
    if (redirect)
        selected = 10;
    if (consume) {
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t i = event->event.event_index + 1; i < count; i++)
            cui_consume_event(ui, i);
    }
}
static cui_menu_command_result frame(bool observer, bool disabled, bool passive, bool legacy) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 160, .height = 40, .inert = passive});
    cui_choice_popup_options options = {.viewport = {0, 0, 400, 300},
                                        .width = 240,
                                        .disabled = disabled,
                                        .max_height = max_height,
                                        .render = body,
                                        .render_user = &painted};
    cui_menu_command_result result;
    memset(&result, 0, sizeof(result));
    if (legacy)
        result.legacy_activated =
            cui_menu(ui, 2, &theme, cui_string("Commands"), items, 3, &selected, &popup, options);
    else
        result =
            cui_menu_events(ui, 2, &theme, cui_string("Commands"), items, 3, &selected, &popup,
                            options, (cui_menu_command_handler){observer ? command : NULL, &calls});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (!legacy)
        assert(result.valid);
    return result;
}
static void click(cui_id id, bool touch) {
    cui_rect bounds = cui_bounds(ui, id);
    assert(bounds.w > 0 && bounds.h > 0);
    for (unsigned up = 0; up < 2; up++)
        assert(cui_queue_event(
            ui, (cui_event){.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                            .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                            .pointer_id = touch ? 42 : 0,
                            .x = bounds.x + 2,
                            .y = bounds.y + 2}));
}
static void open_popup(void) {
    assert(!popup.overlay.open);
    click(2, false);
    frame(true, false, false, false);
    assert(popup.overlay.open);
}
static void key(cui_key value) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = value}));
}
/* Exercise the private seam with legacy List hooks: the public Menu options
 * intentionally do not grow new callback fields. */
bool cui_choice_private_list_commands(cui_context *, const cui_choice_item *, size_t, cui_id *,
                                      cui_list_state *, cui_list_options, cui_id,
                                      cui_menu_command_handler, cui_menu_command_result *);
static unsigned order;
static void legacy_activate(cui_id item, void *user) {
    assert(item == 10 && user == &order && order == 0);
    order = 1;
    selected = 999;
}
static void legacy_confirm(cui_id item, bool secondary, void *user) {
    assert(item == 10 && !secondary && user == &order && order == 1);
    order = 2;
}
static void after_legacy(void *user, const cui_menu_command *event) {
    assert(user == &order && order == 2 && selected == 999 && event->command == 10);
    assert(event->event.target == 2 && event->event.kind == CUI_COMPONENT_EVENT_ACTIVATE);
    order = 3;
}
static void test_legacy_hook_order(void) {
    max_height = 0;
    /* Restore ordinary layout after the prior omitted submission. */
    cui_overlay_close(ui, &popup.overlay);
    frame(true, false, false, false);
    open_popup();
    click(10, false);
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 160, .height = 40});
    assert(cui_overlay_begin(ui, &popup.overlay));
    cui_menu_command_result result;
    memset(&result, 0, sizeof(result));
    cui_list_options options = {.menu = true,
                                .on_activate = legacy_activate,
                                .on_confirm = legacy_confirm,
                                .activation_user = &order};
    assert(cui_choice_private_list_commands(ui, items, 3, &selected, &popup.list, options, 2,
                                            (cui_menu_command_handler){after_legacy, &order},
                                            &result));
    cui_overlay_end(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && order == 3 && result.command_count == 1);
}
int main(void) {
    ui = cui_create(
        (cui_config){.width = 400, .height = 300, .max_elements = 512, .max_commands = 1024});
    assert(ui);
    cui_choice_popup_init(&popup, 700);
    frame(true, false, false, false);
    open_popup();
    assert(painted == 3);
    click(10, false);
    click(10, false);
    click(30, false);
    cui_menu_command_result result = frame(true, false, false, false);
    assert(result.legacy_activated && result.command_count == 3 && selected == 30);
    assert(!popup.overlay.open && calls == 3);
    assert(copies[0].command == 10 && copies[1].command == 10 && copies[2].command == 30);
    assert(copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(copies[0].event.event_index < copies[1].event.event_index);
    assert(!(copies[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    open_popup();
    click(30, true);
    result = frame(true, false, false, false);
    assert(result.command_count == 1 && !popup.overlay.open);
    open_popup();
    key(CUI_KEY_HOME);
    key(CUI_KEY_ENTER);
    result = frame(true, false, false, false);
    assert(selected == 10 && result.command_count == 1);
    assert(copies[calls - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    open_popup();
    /* A supported accessibility target does not override the legacy cursor. */
    assert(cui_queue_accessibility_action(ui, 30, CUI_ACCESSIBILITY_ACTIVATE));
    result = frame(true, false, false, false);
    assert(result.command_count == 1 && selected == 10 && copies[calls - 1].command == 10);
    assert(copies[calls - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    open_popup();
    key(CUI_KEY_END); /* Existing reducer confirms its active cursor. */
    assert(cui_accessibility_action_supported(ui, 30, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(ui, 30, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(ui, 30, CUI_ACCESSIBILITY_ACTIVATE));
    result = frame(true, false, false, false);
    assert(result.command_count == 2 && selected == 30 && !popup.overlay.open);
    assert(copies[calls - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    open_popup();
    click(10, false);
    result = frame(false, false, false, false);
    assert(selected == 10 && result.legacy_activated && !result.command_count);
    open_popup();
    redirect = true;
    click(30, false);
    click(10, false);
    result = frame(true, false, false, false);
    redirect = false;
    assert(selected == 10 && result.command_count == 2 && result.legacy_activated);
    open_popup();
    consume = true;
    click(30, false);
    click(10, false);
    result = frame(true, false, false, false);
    consume = false;
    assert(selected == 30 && result.command_count == 1);
    open_popup();
    click(10, false);
    result = frame(true, true, false, false);
    assert(selected == 30 && !result.command_count && !popup.overlay.open);
    result = frame(true, false, true, false);
    assert(!result.submitted && !result.command_count);
    frame(true, false, false, false);
    open_popup();
    click(10, false);
    result = frame(false, false, false, true);
    assert(result.legacy_activated && selected == 10);
    size_t before_paint = painted;
    click(2, false);
    frame(false, false, false, true);
    assert(popup.overlay.open && painted == before_paint + 3);
    click(20, false);
    result = frame(true, false, false, false);
    assert(!result.command_count && popup.overlay.open && selected == 10);
    click(2, false);
    frame(true, false, false, false);
    assert(!popup.overlay.open);
    max_height = 60;
    open_popup();
    cui_rect row = cui_bounds(ui, 10);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = row.x + 2, .y = row.y + 2, .dy = -2}));
    click(10, false);
    result = frame(true, false, false, false);
    assert(!result.command_count && selected == 10 && popup.overlay.open);
    size_t before = calls;
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_end_frame(ui);
    assert(calls == before);
    cui_theme theme = cui_theme_light();
    result = cui_menu_events(ui, 0, &theme, cui_string("Commands"), items, 3, &selected, &popup,
                             (cui_choice_popup_options){.viewport = {0, 0, 400, 300}},
                             (cui_menu_command_handler){command, &calls});
    assert(!result.valid && !result.command_count);
    test_legacy_hook_order();
    cui_menu_command saved = copies[0];
    cui_destroy(ui);
    assert(saved.command == 10 && saved.event.target == 2);
    return 0;
}
