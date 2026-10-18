/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include <assert.h>
#include <string.h>
/* Compact IDs intentionally retain the radio12/submenu15 paint-ID regression. */
enum { ACTION_ID = 10, CHECK_ID = 11, RADIO_ID = 12, DISABLED_ID = 13,
       LABEL_ID = 14, SUBMENU_ID = 15, CHILD_ID = 21 };
static cui_context *ui;
static cui_menu_tree_state menu;
static cui_id action;
static bool checked;
static int radio;
static cui_menu_entry children[] = {{.id = CHILD_ID, .label = {"Child", 5}, .kind = CUI_MENU_ACTION}};
static cui_menu_entry entries[] = {
    {.id = ACTION_ID, .label = {"Run", 3}, .kind = CUI_MENU_ACTION},
    {.id = CHECK_ID, .label = {"Check", 5}, .kind = CUI_MENU_CHECK, .checked = &checked},
    {.id = RADIO_ID, .label = {"Radio", 5}, .kind = CUI_MENU_RADIO, .radio_value = &radio, .radio_option = 7},
    {.id = DISABLED_ID, .label = {"Disabled", 8}, .kind = CUI_MENU_ACTION, .disabled = true},
    {.id = LABEL_ID, .label = {"Label", 5}, .kind = CUI_MENU_LABEL},
    {.id = SUBMENU_ID, .label = {"More", 4}, .kind = CUI_MENU_SUBMENU, .children = children, .child_count = 1}};
static cui_menu_command copies[32];
static bool checked_at_call[32];
static size_t calls;
static bool update_owner, consume_later;
static void activated(void *user, const cui_menu_command *command) {
    assert(user == &calls && calls < 32 && command->event.target == 2);
    assert(command->event.kind == CUI_COMPONENT_EVENT_ACTIVATE && command->command == action);
    copies[calls] = *command; checked_at_call[calls] = checked; calls++;
    if (command->command == ACTION_ID || command->command == CHILD_ID)
        assert(!menu.overlay.open && !menu.depth);
    else assert(menu.overlay.open);
    if (update_owner) checked = true;
    if (consume_later) {
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t i = command->event.event_index + 1; i < count; i++) cui_consume_event(ui, i);
    }
}
static cui_menu_command_result frame(bool styled, bool observer, bool disabled,
                                      bool passive, bool legacy) {
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 400, .inert = passive});
    cui_menu_command_handler handler = {observer ? activated : NULL, &calls};
    cui_menu_command_result result;
    memset(&result, 0, sizeof(result));
    if (styled) {
        cui_theme theme = cui_theme_light();
        cui_menu_tree_options options = {.popup = {.viewport = {0, 0, 640, 480}, .disabled = disabled},
                                          .keep_open_checks = true};
        if (legacy) result.legacy_activated = cui_menu_tree(ui, 2, &theme, cui_string("Menu"),
            entries, 6, &action, &menu, options);
        else result = cui_menu_tree_events(ui, 2, &theme, cui_string("Menu"), entries, 6,
            &action, &menu, options, handler);
    } else {
        if (legacy) result.legacy_activated = cui_base_menu_tree(ui, 2, cui_string("Menu"),
            entries, 6, &action, &menu, disabled, .016f, true);
        else result = cui_base_menu_tree_events(ui, 2, cui_string("Menu"), entries, 6,
            &action, &menu, disabled, .016f, true, handler);
        cui_begin_box(ui, 2, (cui_box){.width = 200, .height = 32}); cui_end_box(ui);
        if (menu.overlay.open && cui_overlay_begin(ui, &menu.overlay)) {
            cui_begin_box(ui, 700, (cui_box){.width = 240});
            for (size_t i = 0; i < 6; i++) {
                cui_base_menu_entry(ui, &entries[i], false, false, 700);
                cui_begin_box(ui, entries[i].id, (cui_box){.width = 240, .height = 32}); cui_end_box(ui);
            }
            cui_end_box(ui); cui_overlay_end(ui);
        }
    }
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    if (styled && menu.overlay.open && radio == 7) {
        bool radio_glyph = false, submenu_glyph = false;
        cui_rect radio_bounds = cui_bounds(ui, RADIO_ID), sub_bounds = cui_bounds(ui, SUBMENU_ID);
        for (size_t i = 0; i < draw.count; i++) {
            const cui_draw_command *command = &draw.commands[i];
            if (command->type != CUI_DRAW_IMAGE) continue;
            cui_bitmap bitmap = command->bitmap.bitmap;
            size_t coverage = 0;
            for (unsigned y = 0; y < bitmap.height; y++)
                for (unsigned x = 0; x < bitmap.width; x++)
                    coverage += bitmap.pixels[y * bitmap.stride + x * 4 + 3] != 0;
            if (command->bounds.y >= radio_bounds.y &&
                command->bounds.y + command->bounds.h <= radio_bounds.y + radio_bounds.h)
                radio_glyph |= bitmap.width == 16 && coverage > 10;
            if (command->bounds.y >= sub_bounds.y &&
                command->bounds.y + command->bounds.h <= sub_bounds.y + sub_bounds.h)
                submenu_glyph |= bitmap.width == 12 && coverage > 10;
        }
        assert(radio_glyph && submenu_glyph); /* Both real raster glyphs survive. */
    }
    assert(cui_last_error(ui) == CUI_OK);
    if (!legacy) assert(result.valid);
    return result;
}
static void key(cui_key key) { assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key})); }
static void open_menu(bool styled) {
    assert(!menu.overlay.open); cui_focus(ui, 2); key(CUI_KEY_DOWN);
    frame(styled, true, false, false, false); assert(menu.overlay.open);
}
static void click(cui_id id, bool touch) {
    cui_rect bounds = cui_bounds(ui, id); assert(bounds.w > 0 && bounds.h > 0);
    for (unsigned up = 0; up < 2; up++) assert(cui_queue_event(ui, (cui_event){
        .type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE, .pointer_id = touch ? 42 : 0,
        .x = bounds.x + 2, .y = bounds.y + 2}));
}
int main(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        ui = cui_create((cui_config){.width = 640, .height = 480, .max_elements = 512, .max_commands = 1024});
        assert(ui); cui_menu_tree_init(&menu, 700); action = 0; checked = false; radio = 0; calls = 0;
        frame(styled != 0, true, false, false, false); open_menu(styled != 0);
        key(CUI_KEY_DOWN); key(CUI_KEY_ENTER); key(CUI_KEY_ENTER);
        cui_menu_command_result result = frame(styled != 0, true, false, false, false);
        assert(result.command_count == 2 && result.legacy_activated && action == CHECK_ID && !checked);
        assert(checked_at_call[0] && !checked_at_call[1] && copies[0].command == copies[1].command);
        assert(copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
        assert(!(copies[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        key(CUI_KEY_DOWN); key(CUI_KEY_ENTER); key(CUI_KEY_ENTER);
        result = frame(styled != 0, true, false, false, false);
        assert(result.command_count == 2 && radio == 7 && action == RADIO_ID);
        assert(cui_accessibility_action_supported(ui, CHECK_ID, CUI_ACCESSIBILITY_ACTIVATE));
        assert(cui_queue_accessibility_action(ui, CHECK_ID, CUI_ACCESSIBILITY_ACTIVATE));
        result = frame(styled != 0, true, false, false, false);
        assert(result.command_count == 1 && checked && copies[calls-1].command == CHECK_ID);
        assert(copies[calls-1].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
        assert(!(copies[calls-1].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        assert(cui_accessibility_action_supported(ui, RADIO_ID, CUI_ACCESSIBILITY_ACTIVATE));
        assert(cui_queue_accessibility_action(ui, RADIO_ID, CUI_ACCESSIBILITY_ACTIVATE));
        result = frame(styled != 0, true, false, false, false);
        assert(result.command_count == 1 && radio == 7 && copies[calls-1].command == RADIO_ID);
        assert(copies[calls-1].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
        for (unsigned touch = 0; touch < 2; touch++) {
            click(CHECK_ID, touch != 0); result = frame(styled != 0, true, false, false, false);
            assert(result.command_count == 1 && copies[calls-1].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
        }
        click(DISABLED_ID, false); result = frame(styled != 0, true, false, false, false);
        assert(!result.command_count && menu.overlay.open);
        click(LABEL_ID, false); result = frame(styled != 0, true, false, false, false);
        assert(!result.command_count);
        entries[1].checked = NULL;
        frame(styled != 0, true, false, false, false);
        click(CHECK_ID, false); result = frame(styled != 0, true, false, false, false);
        assert(!result.command_count);
        entries[1].checked = &checked;
        frame(styled != 0, true, false, false, false);
        click(CHECK_ID, false); result = frame(styled != 0, false, false, false, false);
        assert(result.legacy_activated && !result.command_count && !checked);
        update_owner = true; click(CHECK_ID, false); click(CHECK_ID, false);
        result = frame(styled != 0, true, false, false, false); update_owner = false;
        assert(result.command_count == 2 && checked);
        consume_later = true; click(CHECK_ID, false); click(RADIO_ID, false);
        result = frame(styled != 0, true, false, false, false); consume_later = false;
        assert(result.command_count == 1 && action == CHECK_ID);
        /* Closing action stops before a later command, even in one input batch. */
        click(ACTION_ID, false); click(CHECK_ID, false);
        result = frame(styled != 0, true, false, false, false);
        assert(result.command_count == 1 && !menu.overlay.open && action == ACTION_ID);
        open_menu(styled != 0);
        assert(cui_accessibility_action_supported(ui, ACTION_ID, CUI_ACCESSIBILITY_ACTIVATE));
        assert(cui_queue_accessibility_action(ui, ACTION_ID, CUI_ACCESSIBILITY_ACTIVATE));
        result = frame(styled != 0, true, false, false, false);
        assert(result.command_count == 1 && copies[calls-1].command == ACTION_ID); /* Same-ID action repeats. */
        open_menu(styled != 0); key(CUI_KEY_END); key(CUI_KEY_RIGHT); key(CUI_KEY_ENTER);
        result = frame(styled != 0, true, false, false, false);
        assert(result.command_count == 1 && action == CHILD_ID && !menu.overlay.open);
        assert(copies[calls-1].command == CHILD_ID && copies[calls-1].event.target == 2);
        assert(!(copies[calls-1].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        open_menu(styled != 0); click(CHECK_ID, false);
        result = frame(styled != 0, true, true, false, false);
        assert(!result.command_count && result.submitted && !menu.overlay.open);
        result = frame(styled != 0, true, false, true, false);
        assert(!result.command_count && !result.submitted);
        frame(styled != 0, true, false, false, false); open_menu(styled != 0);
        click(CHECK_ID, false); result = frame(styled != 0, false, false, false, true);
        assert(result.legacy_activated && !result.command_count);
        key(CUI_KEY_ESCAPE); frame(styled != 0, true, false, false, false);
        size_t before = calls; assert(cui_begin_frame(ui, 640, 480, .016f)); cui_end_frame(ui);
        assert(calls == before);
        cui_menu_command saved = copies[0]; cui_destroy(ui);
        assert(saved.command == CHECK_ID && saved.event.target == 2);
    }
    return 0;
}
