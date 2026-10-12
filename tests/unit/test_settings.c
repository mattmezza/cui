/* SPDX-License-Identifier: MIT */
#include "cui/cui_settings.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned changes, renders, resets;
static cui_settings_render_options rendered;
static cui_draw_list drawing;
static bool custom_theme;
static void changed(void *user) {
    (void)user;
    changes++;
}
static void render(cui_context *ui, cui_id id, const cui_theme *theme,
                   cui_settings_render_options options, void *user) {
    (void)id;
    (void)user;
    renders++;
    rendered = options;
    cui_text_with_line_height(ui, cui_string("Custom field"), 14, theme->text, 17.5f);
}
static bool dirty(void *user) {
    return *(bool *)user;
}
static void reset(void *user) {
    *(bool *)user = false;
    resets++;
}
static cui_settings_result frame(cui_context *ui, const cui_setting_page *pages, size_t count,
                                 cui_settings_state *state, cui_settings_options options) {
    cui_theme theme = cui_theme_light();
    if (custom_theme) {
        theme.group_box = (cui_color){11, 87, 139, 255};
        theme.group_box_foreground = (cui_color){203, 41, 66, 255};
    }
    assert(cui_begin_frame(ui, 1000, 600, .016f));
    cui_settings_result result = cui_settings(ui, 11, &theme, pages, count, state, options);
    drawing = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void test_filter(void) {
    bool value = true, custom = true;
    cui_str keywords[] = {cui_string("MFA")};
    cui_setting_item items[] = {{.title = {"Enable feature", 14},
                                 .description = {"Basic preference", 16},
                                 .kind = CUI_SETTING_SWITCH,
                                 .boolean = &value,
                                 .has_default = true,
                                 .default_boolean = false,
                                 .on_change = changed},
                                {.custom_item = true,
                                 .keywords = keywords,
                                 .keyword_count = 1,
                                 .render = render,
                                 .dirty = dirty,
                                 .reset = reset,
                                 .user = &custom}};
    cui_setting_group groups[] = {{.title = {"Not searchable", 14}, .items = items, .count = 1},
                                  {.title = {"Security", 8}, .items = items + 1, .count = 1}};
    cui_setting_page pages[] = {{.title = {"General", 7}, .groups = groups, .count = 2},
                                {.title = {"Second", 6}, .groups = groups, .count = 1}};
    cui_settings_page_state workspace[2];
    cui_settings_state state;
    assert(cui_settings_state_init(&state, workspace, 2, NULL));
    cui_settings_options options = cui_settings_default();
    options.default_page = 1;
    assert(cui_settings_filter(pages, 2, &state, options).visible_pages == 2);
    assert(state.selected_page == 1);
    options.query = cui_string("mFa");
    cui_settings_result result = cui_settings_filter(pages, 2, &state, options);
    assert(result.selection_changed && state.selected_page == 0 && result.visible_groups == 1 &&
           result.visible_items == 1);
    state.selected_group = 1;
    assert(!cui_settings_filter(pages, 2, &state, options).selection_changed);
    options.query = cui_string("no results");
    assert(cui_settings_filter(pages, 2, &state, options).visible_pages == 0 &&
           state.selected_group == 1);
    options.query = (cui_str){0};
    assert(!cui_settings_filter(pages, 2, &state, options).selection_changed &&
           state.selected_page == 0 && state.selected_group == 1);
    options.query = cui_string("basic");
    assert(cui_settings_filter(pages, 2, &state, options).selection_changed &&
           state.selected_group == CUI_SETTINGS_NO_GROUP);
    assert(cui_settings_reset_page(&pages[0], options.query));
    assert(!value && custom && changes == 1 && resets == 0);
    assert(!cui_setting_item_dirty(&items[0]) && cui_setting_item_dirty(&items[1]));
    assert(cui_settings_reset_page(&pages[0], cui_string("MFA")));
    assert(!custom && resets == 1);
    options.query = cui_string("Not searchable");
    assert(cui_settings_filter(pages, 2, &state, options).visible_pages == 0);
    assert(cui_settings_part_id(11, 0, 0, 0, 5) != cui_settings_part_id(11, 1, 0, 0, 5));
}
static void test_navigation_and_fields(void) {
    cui_context *ui = cui_create((cui_config){.width = 1000, .height = 600});
    assert(ui);
    bool value = false;
    cui_setting_item items[] = {{.title = {"Feature", 7},
                                 .kind = CUI_SETTING_SWITCH,
                                 .boolean = &value,
                                 .on_change = changed},
                                {.title = {"Custom", 6}, .render = render, .disabled = true}};
    cui_setting_group groups[] = {{.title = {"First", 5}, .items = items, .count = 1},
                                  {.title = {"Second", 6}, .items = items + 1, .count = 1}};
    cui_setting_page pages[] = {
        {.title = {"General", 7}, .groups = groups, .count = 2, .default_open = true},
        {.title = {"Other", 5}, .groups = groups, .count = 1}};
    cui_settings_page_state workspace[2];
    cui_settings_state state;
    assert(cui_settings_state_init(&state, workspace, 2, NULL));
    cui_settings_options options = cui_settings_default();
    options.width = 1000;
    options.height = 500;
    assert(!frame(ui, pages, 2, &state, options).invalid);
    assert(renders && rendered.page == 0 && rendered.group == 1 && rendered.disabled &&
           !rendered.vertical);
    cui_id nav = cui_settings_part_id(11, 1, 0, 0, 5);
    cui_focus(ui, nav);
    key(ui, CUI_KEY_ENTER);
    assert(frame(ui, pages, 2, &state, options).selection_changed && state.selected_page == 1);
    cui_id control = cui_settings_part_id(11, 1, 0, 0, 24);
    cui_focus(ui, control);
    key(ui, CUI_KEY_SPACE);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    frame(ui, pages, 2, &state, options);
    assert(value);
    options.width = 600;
    state.selected_page = 0;
    frame(ui, pages, 2, &state, options);
    assert(rendered.vertical);
    items[0].disabled = true;
    control = cui_settings_part_id(11, 0, 0, 0, 24);
    cui_focus(ui, control);
    key(ui, CUI_KEY_SPACE);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    frame(ui, pages, 2, &state, options);
    assert(value);
    options.query = cui_string("absent");
    unsigned previous = renders;
    assert(frame(ui, pages, 2, &state, options).visible_pages == 0 && renders == previous);
    cui_destroy(ui);
}
static void text(cui_context *ui, const char *value) {
    cui_event event = {.type = CUI_EVENT_TEXT};
    assert(strlen(value) < sizeof(event.text));
    memcpy(event.text, value, strlen(value) + 1);
    assert(cui_queue_event(ui, event));
}
static void tall(cui_context *ui, cui_id id, const cui_theme *theme,
                 cui_settings_render_options options, void *user) {
    (void)theme;
    (void)options;
    (void)user;
    cui_begin_box(ui, id, (cui_box){.width = -1, .height = 240});
    cui_end_box(ui);
}
static void test_search_reset_resize_scroll(void) {
    cui_context *ui = cui_create((cui_config){.width = 1000, .height = 600});
    char buffer[64] = "";
    cui_edit *search =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
    assert(search);
    bool first = true, second = true;
    cui_setting_item items[] = {{.title = {"First option", 12},
                                 .kind = CUI_SETTING_CHECKBOX,
                                 .boolean = &first,
                                 .has_default = true,
                                 .disabled = true},
                                {.title = {"Second option", 13},
                                 .kind = CUI_SETTING_SWITCH,
                                 .boolean = &second,
                                 .has_default = true},
                                {.custom_item = true, .render = tall}};
    cui_setting_group groups[] = {{.title = {"First group", 11}, .items = items, .count = 1},
                                  {.title = {"Second group", 12}, .items = items + 1, .count = 1},
                                  {.title = {"Tall group", 10}, .items = items + 2, .count = 1}};
    cui_setting_page page = {
        .title = {"Settings", 8}, .groups = groups, .count = 3, .default_open = true};
    cui_settings_state state;
    cui_settings_page_state workspace[1];
    assert(cui_settings_state_init(&state, workspace, 1, search));
    cui_settings_options options = cui_settings_default();
    options.width = 1000;
    options.height = 220;
    options.variant = CUI_SETTINGS_FILL;
    custom_theme = true;
    frame(ui, &page, 1, &state, options);
    frame(ui, &page, 1, &state, options);
    bool fill = false, foreground = false;
    for (size_t i = 0; i < drawing.count; i++) {
        cui_draw_command command = drawing.commands[i];
        fill |= command.type == CUI_DRAW_RECT && command.color.r == 11 && command.color.g == 87;
        foreground |=
            command.type == CUI_DRAW_TEXT && command.color.r == 203 && command.color.g == 41;
    }
    assert(fill && foreground);
    cui_id handle = cui_resizable_handle_id(cui_settings_part_id(11, 0, 0, 0, 42),
                                            cui_settings_part_id(11, 0, 0, 0, 40));
    cui_focus(ui, handle);
    key(ui, CUI_KEY_RIGHT);
    assert(frame(ui, &page, 1, &state, options).resized && state.sidebar_width == 260);
    cui_id group = cui_settings_part_id(11, 0, 1, 0, 6);
    cui_focus(ui, group);
    key(ui, CUI_KEY_ENTER);
    frame(ui, &page, 1, &state, options);
    frame(ui, &page, 1, &state, options);
    assert(state.selected_group == 1 && state.pages[0].scroll > 0);
    cui_rect viewport = state.pages[0].viewport;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = viewport.x + viewport.w / 2,
                                           .y = viewport.y + 10,
                                           .dy = -10}));
    assert(frame(ui, &page, 1, &state, options).scrolled);
    cui_focus(ui, cui_settings_part_id(11, 0, 0, 0, 4));
    text(ui, "first");
    cui_settings_result result = frame(ui, &page, 1, &state, options);
    assert(result.visible_items == 1 && result.visible_groups == 1 &&
           state.selected_group == CUI_SETTINGS_NO_GROUP && state.pages[0].scroll == 0);
    cui_focus(ui, cui_settings_part_id(11, 0, 0, 0, 34));
    key(ui, CUI_KEY_ENTER);
    result = frame(ui, &page, 1, &state, options);
    assert(result.reset && !first && second);
    cui_edit_select(search, 0, cui_edit_get_state(search).length);
    assert(cui_edit_insert(search, cui_string("missing")) == CUI_EDIT_OK);
    assert(frame(ui, &page, 1, &state, options).visible_pages == 0 && state.selected_page == 0);
    cui_edit_select(search, 0, cui_edit_get_state(search).length);
    assert(cui_edit_insert(search, (cui_str){0}) == CUI_EDIT_OK);
    assert(frame(ui, &page, 1, &state, options).visible_items == 3);
    custom_theme = false;
    cui_edit_destroy(search);
    cui_destroy(ui);
}
static void test_input_number_and_sizes(void) {
    cui_context *ui = cui_create((cui_config){.width = 1000, .height = 600});
    char buffer[64] = "hello", draft[64] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof(buffer),
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    cui_edit *number_edit =
        cui_edit_create((cui_edit_config){.buffer = draft, .capacity = sizeof(draft)});
    assert(edit && number_edit);
    cui_input_state input = {0};
    cui_number_state number_state = {.edit = number_edit};
    double number = 3;
    bool boolean = false;
    cui_setting_item items[] = {
        {.title = {"Text", 4},
         .kind = CUI_SETTING_INPUT,
         .edit = edit,
         .input_state = &input,
         .has_default = true,
         .default_text = {"default", 7}},
        {.title = {"Number", 6},
         .kind = CUI_SETTING_NUMBER,
         .number = &number,
         .number_state = &number_state,
         .number_options = {.has_minimum = true, .minimum = 0, .has_maximum = true, .maximum = 10},
         .has_default = true,
         .default_number = 4},
        {.title = {"Boolean", 7}, .kind = CUI_SETTING_SWITCH, .boolean = &boolean}};
    cui_setting_group group = {.items = items, .count = 3};
    cui_setting_page page = {.title = {"General", 7}, .groups = &group, .count = 1};
    cui_settings_state state;
    cui_settings_page_state workspace[1];
    assert(cui_settings_state_init(&state, workspace, 1, NULL));
    cui_settings_options options = cui_settings_default();
    options.width = 1000;
    options.height = 500;
    frame(ui, &page, 1, &state, options);
    cui_focus(ui, cui_settings_part_id(11, 0, 0, 1, 24));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_A, .modifiers = CUI_MOD_CTRL}));
    text(ui, "999");
    frame(ui, &page, 1, &state, options);
    assert(number == 10 && strcmp(draft, "999") == 0);
    frame(ui, &page, 1, &state, options);
    assert(strcmp(draft, "999") == 0);
    cui_focus(ui, cui_settings_part_id(11, 0, 0, 0, 24));
    frame(ui, &page, 1, &state, options);
    assert(strcmp(draft, "10") == 0);
    assert(cui_edit_select(edit, 4, 1) == CUI_EDIT_OK);
    assert(cui_settings_reset_page(&page, (cui_str){0}));
    assert(strcmp(buffer, "default") == 0 && number == 4);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && strcmp(buffer, "hello") == 0);
    assert(cui_edit_get_state(edit).anchor == 4 && cui_edit_get_state(edit).cursor == 1);
    assert(cui_edit_redo(edit) == CUI_EDIT_OK && strcmp(buffer, "default") == 0);
    for (unsigned i = 0; i < 4; i++) {
        options.size = (cui_settings_size)i;
        frame(ui, &page, 1, &state, options);
        cui_rect input_frame = cui_bounds(ui, cui_settings_part_id(11, 0, 0, 0, 24));
        cui_rect number_frame =
            cui_bounds(ui, cui_numeric_part_id(cui_settings_part_id(11, 0, 0, 1, 24), 0));
        float expected_height = i == CUI_SETTINGS_XSMALL  ? 20
                                : i == CUI_SETTINGS_SMALL ? 24
                                : i == CUI_SETTINGS_LARGE ? 44
                                                          : 32;
        assert(input_frame.h == expected_height && input_frame.w == 256);
        assert(number_frame.h == expected_height && number_frame.w == 128);
        float expected_font = i == CUI_SETTINGS_XSMALL ? 12 : i == CUI_SETTINGS_LARGE ? 16 : 14;
        bool text_font = false;
        for (size_t j = 0; j < drawing.count; j++)
            if (drawing.commands[j].type == CUI_DRAW_TEXT && drawing.commands[j].text.length == 7 &&
                memcmp(drawing.commands[j].text.data, "default", 7) == 0)
                text_font = drawing.commands[j].font_size == expected_font;
        assert(text_font);
        cui_rect track = cui_bounds(ui, cui_switch_track_id(cui_settings_part_id(11, 0, 0, 2, 24)));
        assert(track.h == (i == CUI_SETTINGS_XSMALL || i == CUI_SETTINGS_SMALL ? 16
                           : i == CUI_SETTINGS_LARGE                           ? 24
                                                                               : 20));
    }
    cui_settings_options bad = options;
    bad.sidebar_maximum = NAN;
    assert(frame(ui, &page, 1, &state, bad).invalid);
    cui_edit_destroy(edit);
    cui_edit_destroy(number_edit);
    cui_destroy(ui);
}
static void click_settings(cui_context *ui, cui_id id) {
    cui_rect bounds = cui_bounds(ui, id);
    assert(bounds.w > 0 && bounds.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2}));
}
static void test_dropdown_menu_and_reset_tooltip(void) {
    cui_context *ui = cui_create((cui_config){.width = 1000, .height = 600});
    assert(ui);
    cui_id selected_choice = 9101;
    cui_choice_item choices[] = {
        {9101, {"First", 5}, false}, {9102, {"Disabled", 8}, true}, {9103, {"Last", 4}, false}};
    cui_menu_entry entries[3];
    bool checks[3];
    cui_settings_dropdown_state dropdown_state;
    assert(!cui_settings_dropdown_init(NULL, 9900, entries, checks, 3));
    assert(!cui_settings_dropdown_init(&dropdown_state, 0, entries, checks, 3));
    assert(cui_settings_dropdown_init(&dropdown_state, 9900, entries, checks, 3));
    cui_setting_item item = {.title = {"Mode", 4},
                             .kind = CUI_SETTING_DROPDOWN,
                             .choices = choices,
                             .choice_count = 3,
                             .choice = &selected_choice,
                             .dropdown_state = &dropdown_state,
                             .has_default = true,
                             .default_choice = 9101,
                             .on_change = changed};
    cui_setting_group group = {.items = &item, .count = 1};
    cui_setting_page page = {.title = {"General", 7}, .groups = &group, .count = 1};
    cui_settings_state state;
    cui_settings_page_state workspace[1];
    assert(cui_settings_state_init(&state, workspace, 1, NULL));
    cui_settings_options options = cui_settings_default();
    options.width = 1000;
    options.height = 500;
    dropdown_state.capacity = 2;
    assert(frame(ui, &page, 1, &state, options).invalid);
    dropdown_state.capacity = 3;
    dropdown_state.checked = NULL;
    assert(frame(ui, &page, 1, &state, options).invalid);
    dropdown_state.checked = checks;
    cui_id saved_popup = dropdown_state.menu.overlay.id;
    dropdown_state.menu.overlay.id = 0;
    assert(frame(ui, &page, 1, &state, options).invalid);
    dropdown_state.menu.overlay.id = saved_popup;
    assert(!frame(ui, &page, 1, &state, options).invalid);
    cui_id trigger = cui_settings_part_id(11, 0, 0, 0, 24);
    cui_rect button = cui_bounds(ui, trigger);
    assert(button.h == 32 && button.w < 128); /* Source intrinsic outline trigger. */
    bool outline = false, caret = false;
    cui_theme theme = cui_theme_light();
    cui_color caret_color = theme.buttons[CUI_THEME_BUTTON_DEFAULT].outline_foreground;
    caret_color.a = (uint8_t)lroundf(caret_color.a * .75f);
    for (size_t i = 0; i < drawing.count; i++) {
        cui_draw_command command = drawing.commands[i];
        outline |= command.type == CUI_DRAW_BORDER && fabsf(command.bounds.x - button.x) < .01f &&
                   fabsf(command.bounds.w - button.w) < .01f;
        caret |= command.type == CUI_DRAW_IMAGE && command.bitmap.custom_tint &&
                 memcmp(&command.bitmap.tint, &caret_color, sizeof caret_color) == 0;
    }
    assert(outline && caret);
    click_settings(ui, trigger);
    frame(ui, &page, 1, &state, options);
    assert(dropdown_state.menu.overlay.open && cui_focused(ui) == 9101 && checks[0]);
    cui_rect popup = cui_bounds(ui, 9900);
    assert(fabsf(popup.x + popup.w - button.x - button.w) < .01f);
    assert(cui_bounds(ui, 9101).h == 26);
    size_t semantic_count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
    bool checked_semantic = false, disabled_semantic = false, expanded = false;
    for (size_t i = 0; i < semantic_count; i++) {
        if (semantics[i].id == 9101)
            checked_semantic =
                semantics[i].role == CUI_ROLE_MENUITEMCHECKBOX && semantics[i].checked;
        if (semantics[i].id == 9102)
            disabled_semantic = semantics[i].disabled;
        if (semantics[i].id == trigger)
            expanded = semantics[i].details.expanded;
    }
    assert(checked_semantic && disabled_semantic && expanded);
    unsigned before = changes;
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    frame(ui, &page, 1, &state, options);
    assert(selected_choice == 9103 && !dropdown_state.menu.overlay.open && changes == before + 1);
    assert(cui_focused(ui) == trigger && checks[2] && !checks[0]);
    /* Controlled external selection updates checked rows and visible label. */
    selected_choice = 9101;
    frame(ui, &page, 1, &state, options);
    cui_focus(ui, trigger);
    key(ui, CUI_KEY_DOWN);
    frame(ui, &page, 1, &state, options);
    click_settings(ui, 9102);
    frame(ui, &page, 1, &state, options);
    assert(selected_choice == 9101 && dropdown_state.menu.overlay.open);
    click_settings(ui, 9103);
    frame(ui, &page, 1, &state, options);
    assert(selected_choice == 9103 && !dropdown_state.menu.overlay.open);
    cui_id reset_id = cui_settings_part_id(11, 0, 0, 0, 34);
    cui_focus(ui, reset_id);
    for (unsigned i = 0; i < 35; i++)
        frame(ui, &page, 1, &state, options);
    assert(state.pages[0].reset_tooltip.open);
    assert(cui_bounds(ui, cui_settings_part_id(11, 0, 0, 0, 49)).w > 0);
    key(ui, CUI_KEY_ESCAPE);
    frame(ui, &page, 1, &state, options);
    assert(!state.pages[0].reset_tooltip.open);
    click_settings(ui, reset_id);
    assert(frame(ui, &page, 1, &state, options).reset && selected_choice == 9101);
    assert(!state.pages[0].reset_tooltip.open);
    item.disabled = true;
    frame(ui, &page, 1, &state, options);
    click_settings(ui, trigger);
    frame(ui, &page, 1, &state, options);
    assert(!dropdown_state.menu.overlay.open && selected_choice == 9101);
    /* Existing initialized Select descriptors remain usable. */
    cui_choice_popup legacy;
    cui_choice_popup_init(&legacy, 9910);
    item.disabled = false;
    item.dropdown_state = NULL;
    item.popup = &legacy;
    assert(!frame(ui, &page, 1, &state, options).invalid);
    click_settings(ui, trigger);
    frame(ui, &page, 1, &state, options);
    assert(legacy.overlay.open);
    cui_destroy(ui);
}
int main(void) {
    test_input_number_and_sizes();
    test_search_reset_resize_scroll();
    test_filter();
    test_navigation_and_fields();
    test_dropdown_menu_and_reset_tooltip();
    return 0;
}
