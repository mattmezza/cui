/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_menu_tree_state menu;
static cui_id action;
static int radio;
static bool checked, update_owner, consume_later;
static cui_menu_entry child[] = {{.id = 21, .label = {"Child", 5}, .kind = CUI_MENU_ACTION}};
static cui_menu_entry entries[] = {
    {.id = 10, .label = {"Run", 3}, .kind = CUI_MENU_ACTION},
    {.id = 11, .label = {"Check", 5}, .kind = CUI_MENU_CHECK, .checked = &checked},
    {.id = 12,
     .label = {"Radio", 5},
     .kind = CUI_MENU_RADIO,
     .radio_value = &radio,
     .radio_option = 7},
    {.id = 15,
     .label = {"More", 4},
     .kind = CUI_MENU_SUBMENU,
     .children = child,
     .child_count = 1}};
static cui_menu_command copies[24];
static size_t calls;
static void command(void *user, const cui_menu_command *event) {
    assert(user == &calls && calls < 24);
    assert(event->event.target == 2 && event->event.kind == CUI_COMPONENT_EVENT_ACTIVATE);
    assert(event->command == action && menu.overlay.trigger == 3);
    if (event->command == 12)
        assert(radio == 7);
    copies[calls++] = *event;
    if (update_owner)
        checked = true;
    if (consume_later) {
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t i = event->event.event_index + 1; i < count; i++)
            cui_consume_event(ui, i);
    }
}
static cui_menu_command_result frame(bool styled, bool observer, bool disabled, bool passive,
                                     bool modal, bool legacy) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    if (modal)
        assert(cui_begin_focus_scope(ui, 8000, true));
    cui_begin_box(ui, 1, (cui_box){.width = 400, .inert = passive});
    cui_begin_box(ui, 2, (cui_box){.width = 360, .height = 260});
    cui_base_button(ui, 3, cui_string("Restore here"), false);
    cui_begin_box(ui, 3, (cui_box){.width = 100, .height = 32});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_menu_tree_options options = {
        .popup = {.viewport = {0, 0, 640, 480}, .width = 220, .disabled = disabled},
        .keep_open_checks = true};
    cui_menu_command_result result;
    memset(&result, 0, sizeof(result));
    cui_menu_command_handler handler = {observer ? command : NULL, &calls};
    /* Scope barrier: context receiver was painted inside a different modal
     * scope, but popup invocation is outside it, matching the legacy guard test. */
    if (modal)
        cui_end_focus_scope(ui);
    if (styled) {
        if (legacy)
            result.legacy_activated = cui_context_menu(ui, 2, &theme, cui_string("Context"),
                                                       entries, 4, &action, &menu, options);
        else
            result = cui_context_menu_events(ui, 2, &theme, cui_string("Context"), entries, 4,
                                             &action, &menu, options, handler);
    } else {
        if (legacy)
            result.legacy_activated =
                cui_base_context_menu(ui, 2, entries, 4, &action, &menu, disabled, .016f, true);
        else
            result = cui_base_context_menu_events(ui, 2, entries, 4, &action, &menu, disabled,
                                                  .016f, true, handler);
        (void)cui_menu_popup_paint(ui, &theme, cui_string("Context"), entries, 4, &menu,
                                   menu.anchor, options);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (!legacy)
        assert(result.valid);
    return result;
}
static void secondary(float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_SECONDARY,
                                           .x = x,
                                           .y = y}));
}
static void key(cui_key value) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = value}));
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
int main(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        ui = cui_create(
            (cui_config){.width = 640, .height = 480, .max_elements = 512, .max_commands = 1024});
        assert(ui);
        cui_menu_tree_init(&menu, 700);
        calls = 0;
        action = 0;
        checked = false;
        radio = 0;
        frame(styled != 0, true, false, false, false, false);
        cui_focus(ui, 3);
        secondary(120, 80);
        cui_menu_command_result result = frame(styled != 0, true, false, false, false, false);
        assert(menu.overlay.open && menu.anchor.x == 120 && menu.anchor.y == 80);
        assert(!result.command_count && !result.legacy_activated && menu.overlay.trigger == 3);
        secondary(200, 180);
        result = frame(styled != 0, true, false, false, false, false);
        assert(!result.command_count && menu.anchor.x == 200 && menu.anchor.y == 180);
        assert(cui_queue_accessibility_action(ui, 11, CUI_ACCESSIBILITY_ACTIVATE));
        assert(cui_queue_accessibility_action(ui, 11, CUI_ACCESSIBILITY_ACTIVATE));
        result = frame(styled != 0, true, false, false, false, false);
        assert(result.command_count == 2 && !checked && action == 11);
        assert(copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
        assert(!(copies[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        assert(cui_queue_accessibility_action(ui, 12, CUI_ACCESSIBILITY_ACTIVATE));
        result = frame(styled != 0, true, false, false, false, false);
        assert(result.command_count == 1 && radio == 7 && action == 12);
        key(CUI_KEY_END);
        key(CUI_KEY_RIGHT);
        key(CUI_KEY_ENTER);
        result = frame(styled != 0, true, false, false, false, false);
        assert(result.command_count == 1 && action == 21 && !menu.overlay.open &&
               cui_focused(ui) == 3);
        /* Programmatic opening contributes no source/command of its own. */
        cui_context_menu_open(ui, &menu, 3, (cui_rect){20, 40, 0, 0});
        result = frame(styled != 0, true, false, false, false, false);
        assert(!result.command_count && menu.overlay.open && menu.anchor.x == 20);
        for (unsigned touch = 0; touch < 2; touch++) {
            click(11, touch != 0);
            result = frame(styled != 0, true, false, false, false, false);
            assert(result.command_count == 1 &&
                   copies[calls - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
        }
        click(11, false);
        result = frame(styled != 0, false, false, false, false, false);
        assert(result.legacy_activated && !result.command_count && checked);
        update_owner = true;
        click(11, false);
        click(11, false);
        result = frame(styled != 0, true, false, false, false, false);
        update_owner = false;
        assert(result.command_count == 2 && checked);
        consume_later = true;
        click(11, false);
        click(10, false);
        result = frame(styled != 0, true, false, false, false, false);
        consume_later = false;
        assert(result.command_count == 1 && menu.overlay.open && action == 11);
        result = frame(styled != 0, true, true, false, false, false);
        assert(!result.command_count && !menu.overlay.open);
        result = frame(styled != 0, true, false, true, false, false);
        assert(!result.submitted && !result.command_count);
        frame(styled != 0, true, false, false, false, false);
        cui_focus(ui, 3);
        secondary(120, 80);
        frame(styled != 0, false, false, false, false, true);
        click(10, false);
        result = frame(styled != 0, false, false, false, false, true);
        assert(result.legacy_activated && !menu.overlay.open);
        frame(styled != 0, true, false, false, true, false);
        secondary(120, 80);
        result = frame(styled != 0, true, false, false, true, false);
        assert(!menu.overlay.open && !result.command_count);
        size_t before = calls;
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_end_frame(ui);
        assert(calls == before);
        cui_menu_command saved = copies[0];
        cui_destroy(ui);
        assert(saved.command == 11 && saved.event.target == 2);
    }
    return 0;
}
