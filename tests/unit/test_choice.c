/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
#include <assert.h>
static cui_choice_item items[] = {
    {10, {"First", 5}, false}, {20, {"Disabled", 8}, true}, {30, {"Last", 4}, false}};
static cui_id selected = 10;
static size_t visible;
static void panel(cui_context *ui, size_t index, void *user) {
    (void)user;
    visible = index;
    cui_button(ui, 100 + index, &(cui_theme){.control_height = 32, .font_size = 14},
               cui_string("Content"), false);
}
static bool scene(cui_context *ui, cui_tabs_options options, const cui_theme *theme) {
    visible = 99;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_tabs(ui, 1, theme, items, 3, &selected, options, panel, NULL);
    assert(cui_end_frame(ui).count > 0 && cui_last_error(ui) == CUI_OK);
    return changed;
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void click(cui_context *ui, cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 2, .y = b.y + 2}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 2, .y = b.y + 2}));
}
static bool toggles(cui_context *ui, bool *values, cui_toggle_group_options options) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_toggle_group(ui, 2, &theme, items, 3, values, options);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}

static bool list_scene(cui_context *ui, const cui_choice_item *choices, size_t count, cui_id *value,
                       cui_list_state *state, cui_list_options options) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_begin_box(ui, 90, (cui_box){.width = 240, .height = 400});
    bool changed = cui_list(ui, 91, &theme, choices, count, value, state, options);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static bool popup_scene(cui_context *ui, cui_choice_popup *state, cui_id *value,
                        const cui_choice_item *choices, size_t count, bool menu) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 320, 240, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 80, .height = 50, .clip = true});
    cui_choice_popup_options options = {.viewport = {0, 0, 320, 240}, .width = 220, .dt = .016f};
    bool changed =
        menu ? cui_menu(ui, 2, &theme, cui_string("Actions"), choices, count, value, state, options)
             : cui_select(ui, 2, &theme, cui_string("Account"), cui_string("Choose"), choices,
                          count, value, state, options);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void test_lists_and_popups(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_id value = 0;
    cui_list_state list = {0};
    cui_list_options options = {0};
    list_scene(ui, items, 3, &value, &list, options);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_DOWN);
    assert(!list_scene(ui, items, 3, &value, &list, options));
    assert(list.active == 30 && !value);
    key(ui, CUI_KEY_ENTER);
    assert(list_scene(ui, items, 3, &value, &list, options));
    assert(value == 30);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "f"}));
    assert(!list_scene(ui, items, 3, &value, &list, options));
    assert(list.active == 10 && value == 30);
    /* Repeated single-letter search cycles matching labels without selecting. */
    cui_choice_item many[40];
    for (size_t i = 0; i < 40; i++)
        many[i] = (cui_choice_item){1000 + i, {"Alpha", 5}, i == 39};
    list = (cui_list_state){0};
    options.visible_count = 4;
    list_scene(ui, many, 40, &value, &list, options);
    cui_focus(ui, 1000);
    key(ui, CUI_KEY_END);
    list_scene(ui, many, 40, &value, &list, options);
    assert(list.active == 1038 && list.first_visible == 35 && cui_focused(ui) == 1038);
    size_t n = 0;
    const cui_semantic *semantics = cui_semantics(ui, &n);
    assert(n == 5 && semantics[0].id == 1035 && semantics[4].role == CUI_ROLE_LISTBOX);
    key(ui, CUI_KEY_DOWN);
    list_scene(ui, many, 40, &value, &list, options);
    assert(list.active == 1000 && list.first_visible == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "a"}));
    list_scene(ui, many, 40, &value, &list, options);
    assert(list.active == 1001);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "a"}));
    list_scene(ui, many, 40, &value, &list, options);
    assert(list.active == 1002);
    list = (cui_list_state){.active = 1000};
    options = (cui_list_options){0};
    list_scene(ui, items, 3, &value, &list, options);
    cui_focus(ui, 10);
    list.active = 10;
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(list_scene(ui, items, 3, &value, &list, options));
    assert(value == 30 && list.active == 30);
    cui_choice_popup popup;
    cui_choice_popup_init(&popup, 500);
    value = 10;
    popup_scene(ui, &popup, &value, items, 3, false);
    click(ui, 2);
    assert(!popup_scene(ui, &popup, &value, items, 3, false));
    assert(popup.overlay.open && cui_focused(ui) == 10);
    cui_rect bounds = cui_bounds(ui, 30);
    assert(bounds.w > 0 && bounds.y > 50); /* portal escapes clipped parent */
    key(ui, CUI_KEY_DOWN);
    popup_scene(ui, &popup, &value, items, 3, false);
    assert(cui_focused(ui) == 30 && value == 10);
    key(ui, CUI_KEY_ENTER);
    assert(popup_scene(ui, &popup, &value, items, 3, false));
    assert(value == 30 && !popup.overlay.open && cui_focused(ui) == 2);
    semantics = cui_semantics(ui, &n);
    assert(n == 1 && !semantics[0].details.expanded);
    click(ui, 2);
    popup_scene(ui, &popup, &value, items, 3, false);
    assert(popup.overlay.open);
    key(ui, CUI_KEY_ESCAPE);
    assert(!popup_scene(ui, &popup, &value, items, 3, false));
    assert(!popup.overlay.open && cui_focused(ui) == 2 && value == 30);
    key(ui, CUI_KEY_UP);
    popup_scene(ui, &popup, &value, items, 3, false);
    assert(popup.overlay.open);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 310, .y = 230}));
    popup_scene(ui, &popup, &value, items, 3, false);
    assert(!popup.overlay.open && cui_focused(ui) == 2);
    /* Menu reactivation dispatches the same action again, unlike a select change. */
    popup_scene(ui, &popup, &value, items, 3, true);
    click(ui, 2);
    popup_scene(ui, &popup, &value, items, 3, true);
    key(ui, CUI_KEY_ENTER);
    assert(popup_scene(ui, &popup, &value, items, 3, true));
    assert(value == 30);
    /* A large popup pages keyboard focus into a visible, reachable window. */
    value = 1000;
    popup_scene(ui, &popup, &value, many, 40, false);
    click(ui, 2);
    popup_scene(ui, &popup, &value, many, 40, false);
    key(ui, CUI_KEY_END);
    popup_scene(ui, &popup, &value, many, 40, false);
    assert(popup.list.active == 1038 && cui_focused(ui) == 1038 && cui_bounds(ui, 1038).h > 0);
    key(ui, CUI_KEY_ENTER);
    assert(popup_scene(ui, &popup, &value, many, 40, false));
    assert(value == 1038 && cui_focused(ui) == 2);
    value = 0;
    popup_scene(ui, &popup, &value, NULL, 0, false);
    click(ui, 2);
    popup_scene(ui, &popup, &value, NULL, 0, false);
    assert(popup.overlay.open);
    key(ui, CUI_KEY_ESCAPE);
    popup_scene(ui, &popup, &value, NULL, 0, false);
    assert(!popup.overlay.open && cui_focused(ui) == 2);
    cui_destroy(ui);
}

static cui_combobox_result combo_scene(cui_context *ui, cui_edit *query, cui_choice_popup *popup,
                                       cui_id *value, cui_choice_item *scratch, size_t capacity,
                                       bool disabled) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_combobox_result result = cui_combobox(
        ui, 2, &theme, cui_string("Find"), query, items, 3, scratch, capacity, value, popup,
        (cui_choice_popup_options){.disabled = disabled, .viewport = {0, 0, 640, 480}});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_combobox(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    char text[64] = "";
    cui_edit *query = cui_edit_create((cui_edit_config){
        .buffer = text, .capacity = sizeof(text), .history_bytes = 1024, .max_history_steps = 16});
    assert(query);
    cui_choice_popup popup;
    cui_choice_popup_init(&popup, 700);
    cui_choice_item scratch[3];
    cui_id value = 10;
    cui_combobox_result result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(result.error == CUI_EDIT_OK && result.matching_count == 3 && !popup.overlay.open);
    click(ui, 2);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(popup.overlay.open && cui_focused(ui) == 2);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(result.changed && value == 30 && !popup.overlay.open && cui_focused(ui) == 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "LA"}));
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(result.query_changed && result.matching_count == 1 && scratch[0].id == 30 &&
           popup.overlay.open && cui_focused(ui) == 2);
    key(ui, CUI_KEY_ESCAPE);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(!popup.overlay.open && value == 30 && cui_focused(ui) == 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Z"}));
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(result.matching_count == 0 && popup.overlay.open && value == 30);
    key(ui, CUI_KEY_ENTER);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(!result.changed && value == 30);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, true);
    assert(!popup.overlay.open && !result.query_changed);
    result = combo_scene(ui, query, &popup, &value, scratch, 2, false);
    assert(result.error == CUI_EDIT_CAPACITY);
    assert(cui_edit_select(query, 0, cui_edit_get_state(query).length) == CUI_EDIT_OK);
    assert(cui_edit_insert(query, cui_string("Fir")) == CUI_EDIT_OK);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    click(ui, 2);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(result.matching_count == 1 && scratch[0].id == 10 && popup.overlay.open);
    click(ui, 10);
    result = combo_scene(ui, query, &popup, &value, scratch, 3, false);
    assert(result.changed && value == 10 && !popup.overlay.open && cui_focused(ui) == 2);
    cui_edit_destroy(query);
    cui_destroy(ui);
}
int main(void) {
    test_lists_and_popups();
    test_combobox();
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_tabs_options options = {0};
    for (size_t i = 0; i < 4; i++)
        assert(!scene(ui, options, &themes[i]));
    assert(visible == 0);
    for (int variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++) {
        options.variant = (cui_tab_variant)variant;
        for (size_t i = 0; i < 4; i++)
            assert(!scene(ui, options, &themes[i]));
    }
    options.variant = CUI_TAB_DEFAULT;
    cui_focus(ui, 10);
    key(ui, CUI_KEY_RIGHT);
    assert(scene(ui, options, &themes[0]));
    assert(selected == 30 && cui_focused(ui) == 30 && visible == 2);
    size_t count = 0;
    const cui_semantic *s = cui_semantics(ui, &count);
    assert(count == 6 && s[0].role == CUI_ROLE_TAB && !s[0].checked && s[2].checked &&
           s[2].details.selected && s[0].tab_index < 0 && s[2].tab_index == 0);
    key(ui, CUI_KEY_RIGHT);
    assert(scene(ui, options, &themes[0]));
    assert(selected == 10);
    key(ui, CUI_KEY_END);
    assert(scene(ui, options, &themes[0]));
    assert(selected == 30);
    options.manual_activation = true;
    key(ui, CUI_KEY_HOME);
    assert(!scene(ui, options, &themes[0]));
    assert(cui_focused(ui) == 10 && selected == 30);
    key(ui, CUI_KEY_ENTER);
    assert(scene(ui, options, &themes[0]));
    assert(selected == 10);
    options.vertical = true;
    key(ui, CUI_KEY_DOWN);
    assert(!scene(ui, options, &themes[0]));
    assert(cui_focused(ui) == 30);
    click(ui, 20);
    assert(!scene(ui, options, &themes[0]));
    assert(selected == 10);
    options.disabled = true;
    click(ui, 30);
    assert(!scene(ui, options, &themes[0]));
    assert(selected == 10);
    options.disabled = false;
    selected = 123;
    scene(ui, options, &themes[0]);
    assert(visible == 99);
    selected = 20;
    scene(ui, options, &themes[0]);
    assert(visible == 99);
    /* IDs keep selection stable through reorder. */
    selected = 30;
    cui_choice_item swap = items[0];
    items[0] = items[2];
    items[2] = swap;
    scene(ui, options, &themes[0]);
    assert(selected == 30 && visible == 0);
    swap = items[0];
    items[0] = items[2];
    items[2] = swap;
    bool values[3] = {true, false, false};
    cui_toggle_group_options group = {0};
    toggles(ui, values, group);
    click(ui, 10);
    assert(!toggles(ui, values, group));
    assert(values[0]);
    click(ui, 30);
    assert(toggles(ui, values, group));
    assert(!values[0] && values[2]);
    group.multiple = true;
    group.allow_empty = true;
    click(ui, 10);
    assert(toggles(ui, values, group));
    assert(values[0] && values[2]);
    click(ui, 30);
    assert(toggles(ui, values, group));
    assert(values[0] && !values[2]);
    click(ui, 10);
    assert(toggles(ui, values, group));
    assert(!values[0]);
    click(ui, 20);
    assert(!toggles(ui, values, group));
    cui_focus(ui, 10);
    key(ui, CUI_KEY_RIGHT);
    assert(!toggles(ui, values, group));
    assert(cui_focused(ui) == 30);
    selected = 10;
    options = (cui_tabs_options){.manual_activation = true};
    scene(ui, options, &themes[0]);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_RIGHT);
    key(ui, CUI_KEY_ENTER);
    assert(scene(ui, options, &themes[0]));
    assert(selected == 30 && cui_focused(ui) == 30);
    values[0] = true;
    values[2] = false;
    group = (cui_toggle_group_options){0};
    toggles(ui, values, group);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_RIGHT);
    key(ui, CUI_KEY_ENTER);
    assert(toggles(ui, values, group));
    assert(!values[0] && values[2]);
    cui_destroy(ui);
    return 0;
}
