/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_menu_tree_state menu;
static cui_id selected;
static bool checked, consume, redirect;
static cui_menu_entry items[] = {
    {.id = 10, .label = {"Run", 3}, .kind = CUI_MENU_ACTION},
    {.id = 11, .label = {"Check", 5}, .kind = CUI_MENU_CHECK, .checked = &checked}};
static cui_component_event_metadata actions[32];
static cui_menu_command commands[32];
static size_t action_calls, menu_calls, paint_calls;
static unsigned phases[64], phase_count;
static void activated(void *user, const cui_component_event_metadata *event) {
    assert(user == &action_calls && action_calls < 32);
    assert(event->target == 2 && event->kind == CUI_COMPONENT_EVENT_ACTIVATE);
    assert(event->flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(event->activation.target == 2);
    actions[action_calls++] = *event;
    phases[phase_count++] = 1;
    if (redirect)
        checked = true;
    if (consume) {
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t e = 0; e < count; e++)
            if (cui_event_target(ui, e) == 11)
                cui_consume_event(ui, e);
    }
}
static void command(void *user, const cui_menu_command *event) {
    assert(user == &menu_calls && menu_calls < 32);
    assert(event->event.target == 3 && event->command == selected);
    commands[menu_calls++] = *event;
    phases[phase_count++] = 2;
}
static void content(cui_context *context, void *user) {
    assert(context == ui && user == &paint_calls);
    paint_calls++;
    if (phase_count)
        assert(phases[phase_count - 1] != 3);
    phases[phase_count++] = 3;
    cui_text(ui, cui_string("Custom action"), 12, (cui_color){1, 2, 3, 255});
}
static cui_dropdown_event_result frame(bool styled, bool observers, bool passive, bool legacy,
                                       cui_dropdown_button_options options) {
    cui_theme theme = cui_theme_light();
    phase_count = 0;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 400, .inert = passive});
    options.menu.popup.viewport = (cui_rect){0, 0, 640, 480};
    options.menu.popup.width = 220;
    options.menu.keep_open_checks = true;
    options.content = content;
    options.content_user = &paint_calls;
    cui_dropdown_event_handlers handlers = {{observers ? activated : NULL, &action_calls},
                                            {observers ? command : NULL, &menu_calls}};
    cui_dropdown_event_result result;
    memset(&result, 0, sizeof(result));
    if (styled) {
        if (legacy)
            result.legacy =
                cui_dropdown_button(ui, 100, 2, 3, &theme, cui_string("Action"),
                                    cui_string("Commands"), items, 2, &selected, &menu, options);
        else
            result = cui_dropdown_button_events(ui, 100, 2, 3, &theme, cui_string("Action"),
                                                cui_string("Commands"), items, 2, &selected, &menu,
                                                options, handlers);
    } else {
        if (legacy)
            result.legacy = cui_base_dropdown_button(
                ui, options.hide_action ? 0 : 2, options.hide_menu ? 0 : 3, cui_string("Action"),
                cui_string("Commands"), items, 2, &selected, &menu, options.disabled,
                options.action_disabled, options.action_loading, .016f, true);
        else
            result = cui_base_dropdown_button_events(
                ui, options.hide_action ? 0 : 2, options.hide_menu ? 0 : 3, cui_string("Action"),
                cui_string("Commands"), items, 2, &selected, &menu, options.disabled,
                options.action_disabled, options.action_loading, .016f, true, handlers);
        if (!options.hide_action) {
            cui_begin_box(ui, 2, (cui_box){.width = 120, .height = 36});
            cui_end_box(ui);
        }
        if (!options.hide_menu) {
            cui_begin_box(ui, 3, (cui_box){.width = 36, .height = 36});
            cui_end_box(ui);
        }
        if (menu.overlay.open) {
            cui_rect bounds = {0, 100, 220, 80};
            cui_overlay_set_bounds(&menu.overlay, bounds);
            assert(cui_begin_focus_scope(ui, menu.overlay.id, false));
            cui_begin_portal(ui, menu.overlay.id, (cui_box){.background = theme.popover}, bounds,
                             100);
            for (size_t i = 0; i < 2; i++) {
                cui_base_menu_entry(ui, &items[i], options.disabled, false, menu.overlay.id);
                cui_begin_box(ui, items[i].id, (cui_box){.width = 220, .height = 36});
                cui_text(ui, items[i].label, 12, theme.text);
                cui_end_box(ui);
            }
            cui_end_box(ui);
            cui_end_focus_scope(ui);
        }
    }
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (styled)
        assert(draw.count > 0);
    if (!legacy)
        assert(result.valid);
    return result;
}
static void click(cui_id id, bool touch) {
    cui_rect b = cui_bounds(ui, id);
    assert(b.w > 0 && b.h > 0);
    for (unsigned up = 0; up < 2; up++)
        assert(cui_queue_event(
            ui, (cui_event){.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                            .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                            .pointer_id = touch ? 42 : 0,
                            .x = b.x + 2,
                            .y = b.y + 2}));
}
static cui_semantic semantic(cui_id id) {
    size_t count;
    const cui_semantic *values = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (values[i].id == id)
            return values[i];
    return (cui_semantic){0};
}
int main(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        ui = cui_create(
            (cui_config){.width = 640, .height = 480, .max_elements = 512, .max_commands = 1024});
        assert(ui);
        cui_menu_tree_init(&menu, 700);
        selected = 0;
        checked = false;
        action_calls = menu_calls = paint_calls = 0;
        cui_dropdown_button_options options = {0};
        frame(styled != 0, true, false, false, options);
        cui_rect old = cui_bounds(ui, 2);
        frame(styled != 0, false, false, true, options);
        cui_rect current = cui_bounds(ui, 2);
        assert(!memcmp(&old, &current, sizeof(old)));
        click(2, false);
        click(2, false);
        cui_dropdown_event_result result = frame(styled != 0, true, false, false, options);
        assert(result.action_count == 2 && !result.command_count && result.legacy.action_activated);
        assert(actions[0].source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
        click(2, true);
        result = frame(styled != 0, true, false, false, options);
        assert(result.action_count == 1 &&
               actions[action_calls - 1].activation.release.type == CUI_POINTER_TOUCH);
        cui_focus(ui, 2);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        result = frame(styled != 0, true, false, false, options);
        assert(result.action_count == 1 &&
               actions[action_calls - 1].source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
        assert(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
        result = frame(styled != 0, true, false, false, options);
        assert(result.action_count == 1 &&
               actions[action_calls - 1].source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
        click(2, false);
        result = frame(styled != 0, false, false, false, options);
        assert(!result.action_count && result.legacy.action_activated);
        options.action_loading = true;
        click(2, false);
        result = frame(styled != 0, true, false, false, options);
        assert(!result.action_count && !result.legacy.action_activated && semantic(2).details.busy);
        options.action_loading = false;
        options.action_disabled = true;
        click(2, false);
        result = frame(styled != 0, true, false, false, options);
        assert(!result.action_count && semantic(2).disabled);
        options.action_disabled = false;
        /* Explicit modeless popup permits both independently routed halves in
         * one frame; no acceptance or target is injected by this fixture. */
        menu.overlay.modal = false;
        frame(styled != 0, true, false, false, options);
        click(3, false);
        result = frame(styled != 0, true, false, false, options);
        assert(menu.overlay.open && !result.command_count);
        if (!styled) {
            click(11, false);
            click(11, false);
            assert(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
            assert(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
            result = frame(styled != 0, true, false, false, options);
            assert(result.action_count == 2 && result.command_count == 2 && !checked &&
                   menu.overlay.open);
            assert(phases[0] == 1 && phases[1] == 1 && phases[2] == 2 && phases[3] == 2);
            assert(actions[action_calls - 1].event_index >
                   commands[menu_calls - 2].event.event_index);
            redirect = true;
            assert(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
            click(11, false);
            result = frame(styled != 0, true, false, false, options);
            redirect = false;
            assert(result.action_count == 1 && result.command_count == 1 && !checked);
            consume = true;
            assert(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
            click(11, false);
            result = frame(styled != 0, true, false, false, options);
            consume = false;
            assert(result.action_count == 1 && !result.command_count && !checked);
        } else {
            assert(!cui_accessibility_action_supported(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
            assert(cui_queue_accessibility_action(ui, 11, CUI_ACCESSIBILITY_ACTIVATE));
            assert(cui_queue_accessibility_action(ui, 11, CUI_ACCESSIBILITY_ACTIVATE));
            result = frame(true, true, false, false, options);
            assert(!result.action_count && result.command_count == 2 && !checked &&
                   menu.overlay.open);
        }
        click(11, false);
        result = frame(styled != 0, false, false, false, options);
        assert(!result.command_count && result.legacy.menu_activated && checked);
        options.action_loading = true;
        frame(styled != 0, true, false, false, options);
        assert(cui_queue_accessibility_action(ui, 11, CUI_ACCESSIBILITY_ACTIVATE));
        result = frame(styled != 0, true, false, false, options);
        assert(result.command_count == 1 && !result.action_count && semantic(2).details.busy);
        options.action_loading = false;
        options.hide_menu = true;
        result = frame(styled != 0, true, false, false, options);
        assert(!menu.overlay.open && !result.command_count && !semantic(3).id);
        options.hide_menu = false;
        options.hide_action = true;
        frame(styled != 0, true, false, false, options);
        assert(!semantic(2).id);
        click(3, false);
        frame(styled != 0, true, false, false, options);
        click(10, false);
        result = frame(styled != 0, true, false, false, options);
        assert(result.command_count == 1 && !result.action_count && !menu.overlay.open);
        options.hide_action = false;
        options.disabled = true;
        frame(styled != 0, true, false, false, options);
        click(2, false);
        result = frame(styled != 0, true, false, false, options);
        assert(!result.action_count);
        options.disabled = false;
        result = frame(styled != 0, true, true, false, options);
        assert(!result.submitted && !result.action_count && !result.command_count);
        frame(styled != 0, true, false, false, options);
        click(2, false);
        result = frame(styled != 0, false, false, true, options);
        assert(result.legacy.action_activated);
        if (styled) {
            options.variant = CUI_DROPDOWN_BUTTON_LINK;
            frame(true, true, false, false, options);
            assert(semantic(2).role == CUI_ROLE_LINK && semantic(3).role == CUI_ROLE_LINK);
            assert(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
            result = frame(true, true, false, false, options);
            assert(!result.action_count && !result.legacy.action_activated);
            assert(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
            result = frame(true, false, false, true, options);
            assert(!result.legacy.action_activated); /* Same legacy mismatch. */
        }
        size_t before = action_calls + menu_calls;
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_end_frame(ui);
        assert(action_calls + menu_calls == before);
        result = cui_base_dropdown_button_events(
            ui, 2, 2, cui_string("Action"), cui_string("Commands"), items, 2, &selected, &menu,
            false, false, false, .016f, true, (cui_dropdown_event_handlers){0});
        assert(!result.valid && !result.action_count && !result.command_count);
        cui_component_event_metadata saved = actions[0];
        cui_menu_command saved_command = commands[0];
        cui_destroy(ui);
        assert(saved.target == 2 && saved.activation.target == 2 && saved_command.command == 11);
    }
    return 0;
}
