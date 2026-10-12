/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>
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

static cui_combobox_result combo_order_scene(cui_context *ui, bool multi, cui_edit *query,
                                             cui_choice_popup *popup, cui_id *value, bool *chosen) {
    cui_theme theme = cui_theme_light();
    cui_choice_item filtered[3];
    bool filtered_selected[3];
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_combobox_result result =
        multi
            ? cui_combobox_multi(ui, 2, &theme, cui_string("Ordered query"), query, items, 3,
                                 filtered, filtered_selected, 3, chosen, popup,
                                 (cui_choice_popup_options){.viewport = {0, 0, 640, 480}})
            : cui_combobox(ui, 2, &theme, cui_string("Ordered query"), query, items, 3, filtered, 3,
                           value, popup, (cui_choice_popup_options){.viewport = {0, 0, 640, 480}});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}

static void test_combobox_text_enter_order(void) {
    for (unsigned multi = 0; multi < 2; multi++) {
        for (unsigned enter_first = 0; enter_first < 2; enter_first++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
            assert(ui);
            char text[32] = "";
            cui_edit *query = cui_edit_create((cui_edit_config){.buffer = text,
                                                                .capacity = sizeof(text),
                                                                .history_bytes = 512,
                                                                .max_history_steps = 8});
            assert(query);
            cui_choice_popup popup;
            cui_choice_popup_init(&popup, 700);
            cui_id value = 0;
            bool chosen[3] = {false};
            combo_order_scene(ui, multi != 0, query, &popup, &value, chosen);
            click(ui, 2);
            combo_order_scene(ui, multi != 0, query, &popup, &value, chosen);
            assert(popup.overlay.open && popup.list.active == 10);
            if (enter_first)
                key(ui, CUI_KEY_ENTER);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Las"}));
            if (!enter_first)
                key(ui, CUI_KEY_ENTER);
            cui_combobox_result result =
                combo_order_scene(ui, multi != 0, query, &popup, &value, chosen);
            assert(result.changed && result.query_changed && result.matching_count == 1);
            assert(cui_edit_get_text(query).length == 3 && memcmp(text, "Las", 3) == 0);
            assert((multi ? chosen[enter_first ? 0 : 2] : value == (enter_first ? 10 : 30)));
            assert(popup.overlay.open == (multi || enter_first));
            if (popup.overlay.open)
                assert(popup.list.active == 30);
            cui_edit_destroy(query);
            cui_destroy(ui);
        }
    }
}

static size_t action_count;
static void record_action(cui_id id, void *user) {
    (void)user;
    assert(id == 10 || id == 30);
    action_count++;
}
static bool multi_scene(cui_context *ui, bool *chosen, cui_choice_popup *popup, bool standalone) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed;
    if (standalone)
        changed = cui_list_multi(ui, 90, &theme, items, 3, chosen, &popup->list,
                                 (cui_list_options){.on_activate = record_action});
    else
        changed = cui_select_multi(ui, 2, &theme, cui_string("Accounts"),
                                   cui_string("Choose accounts"), items, 3, chosen, popup,
                                   (cui_choice_popup_options){.viewport = {0, 0, 640, 480}});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void assert_multi_group(cui_context *ui, cui_id id, cui_id labelled_by) {
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++) {
        if (semantics[i].id != id)
            continue;
        assert(semantics[i].role == CUI_ROLE_LISTBOX);
        assert(semantics[i].details.multiselectable);
        assert(semantics[i].details.orientation == CUI_ORIENTATION_VERTICAL);
        assert(semantics[i].details.labelled_by == labelled_by);
        return;
    }
    assert(!"Multi selection group is missing");
}
static void test_multi_select(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    bool chosen[3] = {false, false, false};
    cui_choice_popup popup;
    cui_choice_popup_init(&popup, 800);
    multi_scene(ui, chosen, &popup, true);
    assert_multi_group(ui, 90, 0);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_ENTER);
    key(ui, CUI_KEY_ENTER);
    assert(multi_scene(ui, chosen, &popup, true));
    assert(!chosen[0] && action_count == 2);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(multi_scene(ui, chosen, &popup, true));
    assert(chosen[2] && action_count == 3);
    chosen[2] = false;
    multi_scene(ui, chosen, &popup, false);
    click(ui, 2);
    multi_scene(ui, chosen, &popup, false);
    assert(popup.overlay.open && cui_focused(ui) == 10);
    assert_multi_group(ui, 800, 2);
    assert_multi_group(ui, 800 ^ UINT64_C(0xd1bc905ee4216f11), 0);
    key(ui, CUI_KEY_ENTER);
    assert(multi_scene(ui, chosen, &popup, false));
    assert(chosen[0] && popup.overlay.open);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(multi_scene(ui, chosen, &popup, false));
    assert(chosen[0] && chosen[2] && !chosen[1] && popup.overlay.open);
    size_t n = 0;
    const cui_semantic *s = cui_semantics(ui, &n);
    bool first = false, last = false;
    for (size_t i = 0; i < n; i++) {
        if (s[i].id == 10)
            first = s[i].details.selected && s[i].checked;
        if (s[i].id == 30)
            last = s[i].details.selected && s[i].checked;
    }
    assert(first && last);
    key(ui, CUI_KEY_ESCAPE);
    assert(!multi_scene(ui, chosen, &popup, false));
    assert(!popup.overlay.open && cui_focused(ui) == 2);
    cui_id chip = 10 ^ UINT64_C(0x45f2b82e6c9bf447);
    click(ui, chip);
    assert(multi_scene(ui, chosen, &popup, false));
    assert(!chosen[0] && chosen[2] && cui_focused(ui) == 2);
    cui_destroy(ui);
}

static cui_combobox_result combo_multi_scene(cui_context *ui, cui_edit *query,
                                             cui_choice_popup *popup, bool *chosen,
                                             cui_choice_item *scratch, bool *scratch_selected) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_combobox_result result = cui_combobox_multi(
        ui, 2, &theme, cui_string("Search accounts"), query, items, 3, scratch, scratch_selected, 3,
        chosen, popup, (cui_choice_popup_options){.viewport = {0, 0, 640, 480}});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_multi_combobox(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    char text[64] = "";
    cui_edit *query = cui_edit_create((cui_edit_config){
        .buffer = text, .capacity = sizeof(text), .history_bytes = 1024, .max_history_steps = 16});
    assert(query);
    bool chosen[3] = {false, false, false}, scratch_selected[3];
    cui_choice_item scratch[3];
    cui_choice_popup popup;
    cui_choice_popup_init(&popup, 900);
    combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    click(ui, 2);
    combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    assert(popup.overlay.open && cui_focused(ui) == 2);
    assert_multi_group(ui, 900, 2);
    assert_multi_group(ui, 900 ^ UINT64_C(0xd1bc905ee4216f11), 0);
    key(ui, CUI_KEY_ENTER);
    key(ui, CUI_KEY_ENTER);
    cui_combobox_result result =
        combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    assert(result.changed && !chosen[0] && popup.overlay.open && cui_focused(ui) == 2);
    key(ui, CUI_KEY_ENTER);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    result = combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    assert(result.changed && chosen[0] && chosen[2] && !chosen[1] && popup.overlay.open);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "fir"}));
    result = combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    assert(result.matching_count == 1 && scratch[0].id == 10 && scratch_selected[0]);
    /* Selecting one filtered item preserves selections filtered out of view. */
    click(ui, 10);
    result = combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    assert(result.changed && !chosen[0] && chosen[2] && popup.overlay.open);
    key(ui, CUI_KEY_ESCAPE);
    result = combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    assert(!popup.overlay.open && chosen[2]);
    cui_id chip = 30 ^ UINT64_C(0x45f2b82e6c9bf447);
    click(ui, chip);
    result = combo_multi_scene(ui, query, &popup, chosen, scratch, scratch_selected);
    assert(result.changed && !chosen[2] && cui_focused(ui) == 2);
    cui_edit_destroy(query);
    cui_destroy(ui);
}

static bool menu_checked;
static int menu_radio;
static cui_menu_entry deep_entries[] = {
    {.id = 72, .label = {"Deep action", 11}, .kind = CUI_MENU_ACTION}};
static cui_menu_entry child_entries[] = {
    {.id = 61, .label = {"Export formats", 14}, .kind = CUI_MENU_LABEL},
    {.id = 62, .label = {"Export", 6}, .kind = CUI_MENU_ACTION},
    {.id = 63,
     .label = {"Advanced", 8},
     .kind = CUI_MENU_SUBMENU,
     .children = deep_entries,
     .child_count = 1}};
static cui_menu_entry tree_entries[] = {
    {.id = 41, .label = {"File", 4}, .kind = CUI_MENU_LABEL},
    {.id = 42,
     .label = {"Open", 4},
     .icon = {"+", 1},
     .shortcut = {"Ctrl+O", 6},
     .kind = CUI_MENU_ACTION},
    {.id = 43, .kind = CUI_MENU_SEPARATOR},
    {.id = 44, .label = {"Disabled", 8}, .disabled = true, .kind = CUI_MENU_ACTION},
    {.id = 45, .label = {"Sidebar", 7}, .kind = CUI_MENU_CHECK, .checked = &menu_checked},
    {.id = 46,
     .label = {"Compact", 7},
     .kind = CUI_MENU_RADIO,
     .radio_value = &menu_radio,
     .radio_option = 1},
    {.id = 47,
     .label = {"Comfortable", 11},
     .kind = CUI_MENU_RADIO,
     .radio_value = &menu_radio,
     .radio_option = 2},
    {.id = 48,
     .label = {"Formats", 7},
     .kind = CUI_MENU_SUBMENU,
     .children = child_entries,
     .child_count = 3}};
static bool tree_scene(cui_context *ui, cui_menu_tree_state *state, cui_id *action, bool disabled) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 540, .016f));
    bool activated =
        cui_menu_tree(ui, 2, &theme, cui_string("File menu"), tree_entries, 8, action, state,
                      (cui_menu_tree_options){.popup = {.disabled = disabled,
                                                        .viewport = {0, 0, 640, 540},
                                                        .width = 220,
                                                        .dt = .016f},
                                              .keep_open_checks = true});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return activated;
}
static void test_menu_tree(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
    assert(ui);
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 1100);
    cui_id action = 0;
    tree_scene(ui, &state, &action, false);
    click(ui, 2);
    tree_scene(ui, &state, &action, false);
    assert(state.overlay.open && state.depth == 1 && cui_focused(ui) == 42);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    key(ui, CUI_KEY_ENTER);
    assert(tree_scene(ui, &state, &action, false));
    assert(!menu_checked && state.overlay.open && action == 45);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(tree_scene(ui, &state, &action, false));
    assert(menu_radio == 1);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(tree_scene(ui, &state, &action, false));
    assert(menu_radio == 2);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_RIGHT);
    assert(!tree_scene(ui, &state, &action, false));
    assert(state.depth == 2 && cui_focused(ui) == 62);
    cui_rect parent = cui_bounds(ui, 48), child = cui_bounds(ui, 62);
    assert(child.x > parent.x && child.w > 0);
    key(ui, CUI_KEY_END);
    key(ui, CUI_KEY_RIGHT);
    tree_scene(ui, &state, &action, false);
    assert(state.depth == 3 && cui_focused(ui) == 72);
    key(ui, CUI_KEY_ESCAPE);
    tree_scene(ui, &state, &action, false);
    assert(state.depth == 2 && cui_focused(ui) == 63);
    key(ui, CUI_KEY_LEFT);
    tree_scene(ui, &state, &action, false);
    assert(state.depth == 1 && cui_focused(ui) == 48);
    key(ui, CUI_KEY_RIGHT);
    key(ui, CUI_KEY_ENTER);
    assert(tree_scene(ui, &state, &action, false));
    assert(action == 62 && !state.overlay.open && cui_focused(ui) == 2);
    size_t n = 0;
    cui_semantics(ui, &n);
    assert(n == 1);
    /* Hover opens a submenu, and parent items remain clickable with it open. */
    click(ui, 2);
    tree_scene(ui, &state, &action, false);
    parent = cui_bounds(ui, 48);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = parent.x + 3, .y = parent.y + 3}));
    tree_scene(ui, &state, &action, false);
    assert(state.depth == 2 && cui_focused(ui) == 62);
    click(ui, 45);
    assert(tree_scene(ui, &state, &action, false));
    assert(menu_checked && state.depth == 1 && state.overlay.open && cui_focused(ui) == 45);
    const cui_semantic *semantics = cui_semantics(ui, &n);
    bool check_role = false, radio_role = false;
    for (size_t i = 0; i < n; i++) {
        if (semantics[i].id == 45)
            check_role = semantics[i].role == CUI_ROLE_MENUITEMCHECKBOX && semantics[i].checked;
        if (semantics[i].id == 47)
            radio_role = semantics[i].role == CUI_ROLE_MENUITEMRADIO && semantics[i].checked;
    }
    assert(check_role && radio_role);
    key(ui, CUI_KEY_ESCAPE);
    tree_scene(ui, &state, &action, false);
    assert(!state.overlay.open && cui_focused(ui) == 2);
    key(ui, CUI_KEY_UP);
    tree_scene(ui, &state, &action, false);
    assert(state.overlay.open && cui_focused(ui) == 48);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "c"}));
    tree_scene(ui, &state, &action, false);
    assert(cui_focused(ui) == 46);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "c"}));
    tree_scene(ui, &state, &action, false);
    assert(cui_focused(ui) == 47);
    tree_scene(ui, &state, &action, true);
    assert(!state.overlay.open && state.depth == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 639, .y = 539}));
    tree_scene(ui, &state, &action, false);
    assert(!state.overlay.open);
    cui_destroy(ui);
}

static void test_menu_depth_bound(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
    assert(ui);
    cui_menu_entry chain[CUI_MENU_DEPTH_MAX + 1];
    for (size_t i = 0; i < CUI_MENU_DEPTH_MAX + 1; i++)
        chain[i] =
            (cui_menu_entry){.id = 4000 + i,
                             .label = {"Level", 5},
                             .kind = i < CUI_MENU_DEPTH_MAX ? CUI_MENU_SUBMENU : CUI_MENU_ACTION,
                             .children = i < CUI_MENU_DEPTH_MAX ? &chain[i + 1] : NULL,
                             .child_count = i < CUI_MENU_DEPTH_MAX ? 1 : 0};
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 3900);
    cui_id action = 0;
    cui_theme theme = cui_theme_light();
    for (size_t frame = 0; frame < CUI_MENU_DEPTH_MAX + 2; frame++) {
        if (frame == 1)
            click(ui, 2);
        if (frame >= 2)
            key(ui, CUI_KEY_RIGHT);
        assert(cui_begin_frame(ui, 640, 540, .016f));
        cui_menu_tree(ui, 2, &theme, cui_string("Depth"), chain, 1, &action, &state,
                      (cui_menu_tree_options){.popup = {.viewport = {0, 0, 640, 540}}});
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        assert(state.depth <= CUI_MENU_DEPTH_MAX);
    }
    assert(state.depth == CUI_MENU_DEPTH_MAX && state.depth_limit && !action);
    cui_destroy(ui);
}

static bool context_button_clicked;
static bool context_scene(cui_context *ui, cui_menu_tree_state *state, cui_id *action) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 540, .016f));
    cui_begin_box(ui, 9000, (cui_box){.width = 360, .height = 300, .padding = 12, .clip = true});
    context_button_clicked = cui_button(ui, 2, &theme, cui_string("Receiving area"), false);
    cui_context_menu(
        ui, 9000, &theme, cui_string("Context file menu"), tree_entries, 8, action, state,
        (cui_menu_tree_options){.popup = {.viewport = {0, 0, 640, 540}, .width = 220}});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return context_button_clicked;
}
static void secondary(cui_context *ui, cui_event_type type, float x, float y) {
    assert(cui_queue_event(
        ui, (cui_event){.type = type, .x = x, .y = y, .pointer_button = CUI_POINTER_SECONDARY}));
}
static void test_context_menu(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
    assert(ui);
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 1200);
    cui_id action = 0;
    context_scene(ui, &state, &action);
    click(ui, 2);
    assert(context_scene(ui, &state, &action));
    assert(!state.overlay.open && cui_focused(ui) == 2);
    cui_rect receiver = cui_bounds(ui, 2);
    float x = receiver.x + 4, y = receiver.y + 4;
    secondary(ui, CUI_EVENT_POINTER_DOWN, x, y);
    assert(!context_scene(ui, &state, &action));
    assert(state.overlay.open && state.anchor.x == x && state.anchor.y == y &&
           cui_focused(ui) == 42);
    assert(state.panels[0].x == x && state.panels[0].y == y);
    secondary(ui, CUI_EVENT_POINTER_UP, x, y);
    assert(!context_scene(ui, &state, &action));
    assert(!action);
    size_t n = 0;
    const cui_semantic *semantics = cui_semantics(ui, &n);
    bool named = false;
    for (size_t i = 0; i < n; i++)
        if (semantics[i].id == 1200)
            named = semantics[i].role == CUI_ROLE_MENU && semantics[i].name.length == 17;
    assert(named);
    key(ui, CUI_KEY_END);
    key(ui, CUI_KEY_RIGHT);
    key(ui, CUI_KEY_ENTER);
    context_scene(ui, &state, &action);
    assert(action == 62 && !state.overlay.open && cui_focused(ui) == 2);
    secondary(ui, CUI_EVENT_POINTER_DOWN, 300, 280);
    context_scene(ui, &state, &action);
    assert(state.overlay.open && state.panels[0].x == 300 && state.panels[0].y <= 280);
    secondary(ui, CUI_EVENT_POINTER_DOWN, 40, 40);
    context_scene(ui, &state, &action);
    assert(state.overlay.open && state.anchor.x == 40 && state.panels[0].x == 40);
    key(ui, CUI_KEY_ESCAPE);
    context_scene(ui, &state, &action);
    assert(!state.overlay.open && cui_focused(ui) == 2);
    secondary(ui, CUI_EVENT_POINTER_DOWN, 600, 400);
    context_scene(ui, &state, &action);
    assert(!state.overlay.open);
    cui_destroy(ui);
}

static void guarded_context_scene(cui_context *ui, cui_menu_tree_state *state, bool nested) {
    cui_theme theme = cui_theme_light();
    cui_id action = 0;
    assert(cui_begin_frame(ui, 640, 540, .016f));
    assert(cui_begin_focus_scope(ui, 5000, true));
    cui_button(ui, 5001, &theme, cui_string("Modal focus"), false);
    if (!nested)
        cui_end_focus_scope(ui);
    cui_begin_box(ui, 9000, (cui_box){.width = 360, .height = 300});
    cui_context_menu(ui, 9000, &theme, cui_string("Scoped context"), tree_entries, 8, &action,
                     state, (cui_menu_tree_options){.popup.viewport = {0, 0, 640, 540}});
    cui_end_box(ui);
    if (nested)
        cui_end_focus_scope(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_context_modal_guard(void) {
    for (unsigned nested = 0; nested < 2; nested++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
        assert(ui);
        cui_menu_tree_state state;
        cui_menu_tree_init(&state, 1200);
        guarded_context_scene(ui, &state, nested != 0);
        cui_focus(ui, 5001);
        cui_rect receiver = cui_bounds(ui, 9000);
        secondary(ui, CUI_EVENT_POINTER_DOWN, receiver.x + 10, receiver.y + 10);
        guarded_context_scene(ui, &state, nested != 0);
        assert(state.overlay.open == (nested != 0));
        if (!nested) {
            assert(cui_focused(ui) == 5001);
            size_t count = 0;
            const cui_semantic *semantics = cui_semantics(ui, &count);
            for (size_t i = 0; i < count; i++)
                assert(semantics[i].id != 1200 && semantics[i].id != 42);
        } else {
            key(ui, CUI_KEY_ESCAPE);
            guarded_context_scene(ui, &state, true);
            assert(!state.overlay.open && cui_focused(ui) == 5001);
        }
        cui_destroy(ui);
    }
}

static size_t choice_rendered;
static cui_id choice_rendered_ids[3];
static size_t choice_rendered_indices[3];
static void custom_choice_row(cui_context *ui, const cui_choice_item *item, size_t index,
                              void *user) {
    assert(user == &choice_rendered);
    assert(index < 3 && choice_rendered < 3);
    choice_rendered_indices[choice_rendered] = index;
    choice_rendered_ids[choice_rendered++] = item->id;
    cui_text(ui, cui_string("Custom option body"), 13, (cui_color){20, 30, 40, 255});
}
static bool custom_choices_scene(cui_context *ui, unsigned mode, cui_edit *query,
                                 cui_choice_popup *state, cui_id *value, bool *chosen) {
    cui_theme theme = cui_theme_light();
    cui_choice_item filtered[3];
    bool filtered_selected[3];
    choice_rendered = 0;
    assert(cui_begin_frame(ui, 640, 540, .016f));
    cui_choice_popup_options options = {
        .viewport = {0, 0, 640, 540}, .render = custom_choice_row, .render_user = &choice_rendered};
    bool changed;
    if (mode == 0)
        changed = cui_list(
            ui, 90, &theme, items, 3, value, &state->list,
            (cui_list_options){.render = custom_choice_row, .render_user = &choice_rendered});
    else if (mode == 1)
        changed = cui_select(ui, 2, &theme, cui_string("Custom choices"), cui_string("Choose"),
                             items, 3, value, state, options);
    else if (mode == 2)
        changed = cui_select_multi(ui, 2, &theme, cui_string("Custom choices"),
                                   cui_string("Choose"), items, 3, chosen, state, options);
    else if (mode == 3)
        changed = cui_combobox(ui, 2, &theme, cui_string("Custom choices"), query, items, 3,
                               filtered, 3, value, state, options)
                      .changed;
    else
        changed = cui_combobox_multi(ui, 2, &theme, cui_string("Custom choices"), query, items, 3,
                                     filtered, filtered_selected, 3, chosen, state, options)
                      .changed;
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void test_custom_choice_rows(void) {
    for (unsigned mode = 0; mode < 5; mode++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
        assert(ui);
        char text[32] = "";
        cui_edit *query = cui_edit_create((cui_edit_config){.buffer = text,
                                                            .capacity = sizeof(text),
                                                            .history_bytes = 512,
                                                            .max_history_steps = 8});
        assert(query);
        cui_choice_popup state;
        cui_choice_popup_init(&state, 800);
        cui_id value = 0;
        bool chosen[3] = {false, false, false};
        custom_choices_scene(ui, mode, query, &state, &value, chosen);
        if (mode) {
            click(ui, 2);
            custom_choices_scene(ui, mode, query, &state, &value, chosen);
        }
        assert(choice_rendered == 3 && choice_rendered_ids[0] == 10 &&
               choice_rendered_ids[1] == 20 && choice_rendered_ids[2] == 30);
        size_t count = 0;
        const cui_semantic *semantics = cui_semantics(ui, &count);
        bool labelled = false, disabled = false;
        for (size_t i = 0; i < count; i++) {
            if (semantics[i].id == 10)
                labelled = semantics[i].role == CUI_ROLE_OPTION && semantics[i].name.length == 5 &&
                           memcmp(semantics[i].name.data, "First", 5) == 0;
            if (semantics[i].id == 20)
                disabled = semantics[i].disabled;
        }
        assert(labelled && disabled);
        if (mode >= 3) {
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "las"}));
            custom_choices_scene(ui, mode, query, &state, &value, chosen);
            assert(choice_rendered == 1 && choice_rendered_ids[0] == 30 &&
                   choice_rendered_indices[0] == 0);
        }
        click(ui, 30);
        assert(custom_choices_scene(ui, mode, query, &state, &value, chosen));
        if (mode == 2 || mode == 4)
            assert(chosen[2] && state.overlay.open);
        else
            assert(value == 30 && (!mode || !state.overlay.open));
        cui_edit_destroy(query);
        cui_destroy(ui);
    }
}

static cui_choice_item scroll_items[12];
static bool scroll_choices_scene(cui_context *ui, unsigned mode, cui_edit *query,
                                 cui_choice_popup *state, cui_id *value, bool *chosen,
                                 bool disabled) {
    cui_theme theme = cui_theme_light();
    cui_choice_item filtered[12];
    bool filtered_selected[12];
    assert(cui_begin_frame(ui, 640, 540, .016f));
    cui_choice_popup_options options = {.disabled = disabled, .viewport = {0, 0, 640, 160}};
    cui_list_options list_options = {
        .disabled = disabled, .visible_begin = state->list.first_visible, .visible_count = 4};
    bool changed;
    if (mode == 0)
        changed = cui_list(ui, 90, &theme, scroll_items, 12, value, &state->list, list_options);
    else if (mode == 1)
        changed =
            cui_list_multi(ui, 90, &theme, scroll_items, 12, chosen, &state->list, list_options);
    else if (mode == 2)
        changed = cui_select(ui, 2, &theme, cui_string("Scroll"), cui_string("Choose"),
                             scroll_items, 12, value, state, options);
    else if (mode == 3)
        changed = cui_select_multi(ui, 2, &theme, cui_string("Scroll"), cui_string("Choose"),
                                   scroll_items, 12, chosen, state, options);
    else if (mode == 4)
        changed = cui_combobox(ui, 2, &theme, cui_string("Scroll"), query, scroll_items, 12,
                               filtered, 12, value, state, options)
                      .changed;
    else
        changed = cui_combobox_multi(ui, 2, &theme, cui_string("Scroll"), query, scroll_items, 12,
                                     filtered, filtered_selected, 12, chosen, state, options)
                      .changed;
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void wheel(cui_context *ui, float x, float y, float dy) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = x, .y = y, .dy = dy}));
}
static void assert_scroll_metadata(cui_context *ui, cui_id id, size_t first) {
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    cui_theme theme = cui_theme_light();
    for (size_t i = 0; i < count; i++)
        if (semantics[i].id == id) {
            assert(semantics[i].details.scrollable);
            assert(semantics[i].details.scroll_y == first * theme.control_height);
            assert(semantics[i].details.scroll_max_y >= semantics[i].details.scroll_y);
            assert(semantics[i].details.scroll_max_y > 0);
            return;
        }
    assert(!"Scroll semantics missing");
}
static void test_choice_scrolling(void) {
    for (size_t i = 0; i < 12; i++)
        scroll_items[i] = (cui_choice_item){1000 + i, cui_string("Scroll option"), i == 2};
    for (unsigned mode = 0; mode < 6; mode++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
        assert(ui);
        char text[32] = "";
        cui_edit *query = cui_edit_create((cui_edit_config){.buffer = text,
                                                            .capacity = sizeof(text),
                                                            .history_bytes = 512,
                                                            .max_history_steps = 8});
        assert(query);
        cui_choice_popup state;
        cui_choice_popup_init(&state, 800);
        cui_id value = 1000;
        bool chosen[12] = {false};
        scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
        if (mode >= 2) {
            click(ui, 2);
            scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
        } else
            cui_focus(ui, 1000);
        cui_rect bounds = cui_bounds(ui, mode >= 2 ? 800 : 90);
        float x = bounds.x + 10, y = bounds.y + 12;
        wheel(ui, x, y, -.25f);
        assert(!scroll_choices_scene(ui, mode, query, &state, &value, chosen, false));
        assert(state.list.first_visible == 0 && state.list.scroll_remainder == .25f);
        wheel(ui, x, y, -1.75f);
        assert(!scroll_choices_scene(ui, mode, query, &state, &value, chosen, false));
        assert(state.list.first_visible == 2 && state.list.active == 1003);
        assert_scroll_metadata(ui, mode >= 2 ? 800 : 90, 2);
        if (mode >= 2)
            assert_scroll_metadata(ui, 800 ^ UINT64_C(0xd1bc905ee4216f11), 2);
        assert(value == 1000 && !chosen[3]);
        assert(cui_focused(ui) == (mode >= 4 ? 2 : 1003));
        /* Idle frames retain the wheel window instead of snapping to old selection. */
        scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
        assert(state.list.first_visible == 2);
        wheel(ui, 630, 530, -10);
        scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
        assert(state.list.first_visible == 2);
        wheel(ui, x, y, NAN);
        scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
        assert(state.list.first_visible == 2);
        wheel(ui, x, y, -10000);
        scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
        assert(state.list.first_visible > 2 && state.list.first_visible < 12);
        wheel(ui, x, y, 10000);
        scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
        assert(state.list.first_visible == 0 && value == 1000 && !chosen[11]);
        if (mode < 2) {
            wheel(ui, x, y, -3);
            scroll_choices_scene(ui, mode, query, &state, &value, chosen, true);
            assert(state.list.first_visible == 0);
            scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
            cui_focus(ui, 1000);
        }
        key(ui, CUI_KEY_END);
        key(ui, CUI_KEY_ENTER);
        assert(scroll_choices_scene(ui, mode, query, &state, &value, chosen, false));
        if (mode == 1 || mode == 3 || mode == 5)
            assert(chosen[11] && !chosen[3]);
        else
            assert(value == 1011);
        cui_edit_destroy(query);
        cui_destroy(ui);
    }
}

static void test_list_scroll_modal_guard(void) {
    for (unsigned nested = 0; nested < 2; nested++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
        assert(ui);
        cui_theme theme = cui_theme_light();
        cui_list_state state = {0};
        cui_id selected = 1000;
        for (unsigned frame = 0; frame < 2; frame++) {
            if (frame) {
                cui_rect bounds = cui_bounds(ui, 90);
                wheel(ui, bounds.x + 10, bounds.y + 10, -3);
            }
            assert(cui_begin_frame(ui, 640, 540, .016f));
            assert(cui_begin_focus_scope(ui, 5000, true));
            cui_button(ui, 5001, &theme, cui_string("Modal"), false);
            if (!nested)
                cui_end_focus_scope(ui);
            assert(!cui_list(
                ui, 90, &theme, scroll_items, 12, &selected, &state,
                (cui_list_options){.visible_count = 4, .visible_begin = state.first_visible}));
            if (nested)
                cui_end_focus_scope(ui);
            cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
        }
        assert(state.first_visible == (nested ? 3 : 0) && selected == 1000);
        cui_destroy(ui);
    }
}

static void test_scroll_activation_order(void) {
    for (unsigned mode = 0; mode < 6; mode++) {
        for (unsigned order = 0; order < 3; order++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
            assert(ui);
            char text[32] = "";
            cui_edit *query = cui_edit_create((cui_edit_config){.buffer = text,
                                                                .capacity = sizeof(text),
                                                                .history_bytes = 512,
                                                                .max_history_steps = 8});
            assert(query);
            cui_choice_popup state;
            cui_choice_popup_init(&state, 800);
            cui_id value = 0;
            bool chosen[12] = {false};
            scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
            if (mode >= 2) {
                click(ui, 2);
                scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
            } else
                cui_focus(ui, 1000);
            cui_rect bounds = cui_bounds(ui, mode >= 2 ? 800 : 90);
            if (order == 1)
                key(ui, CUI_KEY_ENTER);
            wheel(ui, bounds.x + 10, bounds.y + 12, -3);
            if (order == 0)
                key(ui, CUI_KEY_ENTER);
            else if (order == 2)
                click(ui, 1000); /* previous-frame row hit must not activate after scrolling */
            bool changed = scroll_choices_scene(ui, mode, query, &state, &value, chosen, false);
            assert(changed == (order != 2));
            if (order == 2)
                assert(value == 0 && !chosen[0] && !chosen[3]);
            else if (mode == 1 || mode == 3 || mode == 5)
                assert(chosen[order == 0 ? 3 : 0] && !chosen[order == 0 ? 0 : 3]);
            else
                assert(value == (order == 0 ? 1003 : 1000));
            cui_edit_destroy(query);
            cui_destroy(ui);
        }
    }
}

static unsigned tab_prefix_count, tab_suffix_count;
static void tab_prefix(cui_context *ui, void *user) {
    assert(user == &tab_prefix_count);
    tab_prefix_count++;
    cui_theme theme = cui_theme_light();
    cui_button(ui, 700, &theme, cui_string("Before tabs"), false);
}
static void tab_suffix(cui_context *ui, void *user) {
    assert(user == &tab_prefix_count);
    tab_suffix_count++;
    cui_theme theme = cui_theme_light();
    cui_button(ui, 701, &theme, cui_string("After tabs"), false);
}
static bool custom_tabs_scene(cui_context *ui, cui_id *value, cui_tab_variant variant,
                              const cui_theme *theme) {
    choice_rendered = 0;
    assert(cui_begin_frame(ui, 640, 540, .016f));
    bool changed = cui_tabs(ui, 1, theme, items, 3, value,
                            (cui_tabs_options){.variant = variant,
                                               .render = custom_choice_row,
                                               .render_user = &choice_rendered,
                                               .prefix = tab_prefix,
                                               .suffix = tab_suffix,
                                               .content_user = &tab_prefix_count},
                            panel, NULL);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && choice_rendered == 3);
    return changed;
}
static void test_custom_tabs(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t t = 0; t < 4; t++) {
        for (cui_tab_variant variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
            assert(ui);
            cui_id value = 10;
            tab_prefix_count = tab_suffix_count = 0;
            custom_tabs_scene(ui, &value, variant, &themes[t]);
            assert(tab_prefix_count == 1 && tab_suffix_count == 1 && visible == 0);
            cui_rect prefix = cui_bounds(ui, 700), first = cui_bounds(ui, 10),
                     last = cui_bounds(ui, 30), suffix = cui_bounds(ui, 701);
            assert(prefix.x + prefix.w <= first.x + .01f && last.x + last.w <= suffix.x + .01f);
            cui_rect bar = cui_bounds(ui, 1 ^ UINT64_C(0xcda0312561f50cab));
            assert(bar.h == prefix.h && fabsf(first.y - bar.y - (bar.h - first.h) * .5f) < .01f);
            size_t count = 0;
            const cui_semantic *semantics = cui_semantics(ui, &count);
            bool named = false, disabled = false;
            for (size_t i = 0; i < count; i++) {
                if (semantics[i].id == 10)
                    named = semantics[i].role == CUI_ROLE_TAB && semantics[i].name.length == 5 &&
                            semantics[i].details.selected;
                if (semantics[i].id == 20)
                    disabled = semantics[i].disabled;
            }
            assert(named && disabled);
            click(ui, 700);
            assert(!custom_tabs_scene(ui, &value, variant, &themes[t]) && value == 10);
            click(ui, 701);
            assert(!custom_tabs_scene(ui, &value, variant, &themes[t]) && value == 10);
            cui_focus(ui, 10);
            key(ui, CUI_KEY_RIGHT);
            key(ui, CUI_KEY_ENTER);
            assert(custom_tabs_scene(ui, &value, variant, &themes[t]) && value == 30 &&
                   visible == 2);
            cui_destroy(ui);
        }
    }
}

static void nested_lists_scene(cui_context *ui, cui_list_state *parent, cui_list_state *child) {
    cui_theme theme = cui_theme_light();
    cui_id parent_selected = 0, child_selected = 0;
    assert(cui_begin_frame(ui, 640, 540, .016f));
    assert(!cui_base_list(ui, scroll_items, 12, &parent_selected, parent,
                          (cui_list_options){.visible_count = 4,
                                             .visible_begin = parent->first_visible,
                                             .scroll_target = 100,
                                             .scroll_bounds = cui_bounds(ui, 100)}));
    cui_interact(ui, 100, CUI_ROLE_LISTBOX, cui_string("Parent list"), false, false, 0);
    cui_set_semantic_details(ui, 100,
                             (cui_semantic_details){.scrollable = true,
                                                    .scroll_y = parent->first_visible * 36,
                                                    .scroll_max_y = 8 * 36});
    cui_begin_box(ui, 100, (cui_box){.width = 400, .height = 250, .clip = true});
    cui_choice_item child_items[12];
    for (size_t i = 0; i < 12; i++)
        child_items[i] = (cui_choice_item){2000 + i, cui_string("Nested child"), false};
    assert(
        !cui_list(ui, 90, &theme, child_items, 12, &child_selected, child,
                  (cui_list_options){.visible_count = 4, .visible_begin = child->first_visible}));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_nested_list_wheel_routing(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
    assert(ui);
    cui_list_state parent = {0}, child = {0};
    nested_lists_scene(ui, &parent, &child);
    cui_rect bounds = cui_bounds(ui, 90);
    wheel(ui, bounds.x + 10, bounds.y + 10, -3);
    nested_lists_scene(ui, &parent, &child);
    assert(child.first_visible == 3 && parent.first_visible == 0);
    wheel(ui, bounds.x + 10, bounds.y + 10, -100);
    nested_lists_scene(ui, &parent, &child);
    assert(child.first_visible == 8 && parent.first_visible == 0);
    /* At the inner boundary, the core routes a subsequent wheel to its ancestor. */
    wheel(ui, bounds.x + 10, bounds.y + 10, -1);
    nested_lists_scene(ui, &parent, &child);
    assert(child.first_visible == 8 && parent.first_visible == 1);
    cui_destroy(ui);
}

static cui_draw_list combo_edit_scene(cui_context *ui, unsigned multi, cui_edit *query,
                                      cui_choice_popup *state, const cui_theme *theme) {
    cui_choice_item filtered[3];
    bool filtered_selected[3], chosen[3] = {false};
    cui_id selected = 0;
    assert(cui_begin_frame(ui, 640, 540, .016f));
    cui_choice_popup_options options = {.viewport = {0, 0, 640, 540}};
    cui_combobox_result result;
    if (multi)
        result = cui_combobox_multi(ui, 2, theme, cui_string("Editor combo"), query, items, 3,
                                    filtered, filtered_selected, 3, chosen, state, options);
    else
        result = cui_combobox(ui, 2, theme, cui_string("Editor combo"), query, items, 3, filtered,
                              3, &selected, state, options);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && result.error == CUI_EDIT_OK);
    return draw;
}
static bool same_color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void test_combo_editor_paint_and_hit(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (unsigned multi = 0; multi < 2; multi++) {
        for (size_t t = 0; t < 4; t++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
            assert(ui);
            char text[256] = "";
            cui_edit *query = cui_edit_create((cui_edit_config){.buffer = text,
                                                                .capacity = sizeof(text),
                                                                .history_bytes = 2048,
                                                                .max_history_steps = 16});
            assert(query);
            cui_choice_popup state;
            cui_choice_popup_init(&state, 800);
            combo_edit_scene(ui, multi, query, &state, &themes[t]);
            click(ui, 2);
            combo_edit_scene(ui, multi, query, &state, &themes[t]);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "abécd"}));
            cui_draw_list draw = combo_edit_scene(ui, multi, query, &state, &themes[t]);
            assert(cui_edit_get_text(query).length == 6 && memcmp(text, "abécd", 6) == 0);
            bool caret = false;
            for (size_t i = 0; i < draw.count; i++)
                caret |= draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.w == 1 &&
                         same_color(draw.commands[i].color, themes[t].text);
            assert(caret);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = CUI_KEY_A,
                                                   .modifiers = CUI_MOD_CTRL}));
            draw = combo_edit_scene(ui, multi, query, &state, &themes[t]);
            cui_edit_state selection = cui_edit_get_state(query);
            assert(selection.anchor == 0 && selection.cursor == 6);
            bool highlighted = false;
            for (size_t i = 0; i < draw.count; i++)
                highlighted |= draw.commands[i].type == CUI_DRAW_TEXT &&
                               draw.commands[i].text.length == 6 &&
                               same_color(draw.commands[i].color, themes[t].on_primary);
            assert(highlighted);
            cui_rect viewport = cui_bounds(ui, cui_input_viewport_id(2));
            float start_x =
                viewport.x + cui_measure_text(ui, cui_string("a"), themes[t].font_size).width;
            float end_x =
                viewport.x + cui_measure_text(ui, cui_string("abé"), themes[t].font_size).width;
            float y = viewport.y + viewport.h * .5f;
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = start_x, .y = y}));
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = end_x, .y = y}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = end_x, .y = y}));
            combo_edit_scene(ui, multi, query, &state, &themes[t]);
            selection = cui_edit_get_state(query);
            assert(selection.anchor == 1 && selection.cursor == 4 &&
                   !state.query_behavior.dragging);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "X"}));
            combo_edit_scene(ui, multi, query, &state, &themes[t]);
            assert(cui_edit_get_text(query).length == 4 && memcmp(text, "aXcd", 4) == 0);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = CUI_KEY_Z,
                                                   .modifiers = CUI_MOD_CTRL}));
            combo_edit_scene(ui, multi, query, &state, &themes[t]);
            assert(cui_edit_get_text(query).length == 6 && memcmp(text, "abécd", 6) == 0);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = CUI_KEY_A,
                                                   .modifiers = CUI_MOD_CTRL}));
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_TEXT,
                                .text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"}));
            combo_edit_scene(ui, multi, query, &state, &themes[t]);
            assert(cui_edit_get_text(query).length == 52 && state.query_scroll_x > 0);
            viewport = cui_bounds(ui, cui_input_viewport_id(2));
            cui_input_hit_geometry geometry = {ui, viewport, themes[t].font_size,
                                               state.query_scroll_x};
            size_t expected = cui_base_input_hit_test(&geometry, cui_edit_get_text(query),
                                                      viewport.x + 1, viewport.y + viewport.h / 2);
            assert(expected > 0 && expected < 52);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                   .x = viewport.x + 1,
                                                   .y = viewport.y + viewport.h / 2}));
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                                   .x = viewport.x + 1,
                                                   .y = viewport.y + viewport.h / 2}));
            combo_edit_scene(ui, multi, query, &state, &themes[t]);
            selection = cui_edit_get_state(query);
            assert(selection.anchor == expected && selection.cursor == expected);
            cui_edit_destroy(query);
            cui_destroy(ui);
        }
    }
}

typedef struct {
    cui_choice_item entries[3];
    size_t count;
    cui_id selected, requested;
    cui_theme theme;
} close_tabs_model;
static cui_id close_tab_id(cui_id tab) {
    return tab ^ UINT64_C(0x8242bd476918bf60);
}
static void close_tab_mark(cui_context *ui, void *user) {
    cui_theme *theme = user;
    cui_text(ui, cui_string("×"), theme->font_size, theme->text);
}
static void close_tab_body(cui_context *ui, const cui_choice_item *item, size_t index, void *user) {
    close_tabs_model *model = user;
    assert(index < model->count);
    cui_begin_box(ui, 0, (cui_box){.row = true, .gap = 3});
    cui_text(ui, item->label, model->theme.font_size,
             item->disabled ? model->theme.muted : model->theme.text);
    cui_theme close_theme = model->theme;
    close_theme.control_height = 14;
    close_theme.font_size = 10;
    close_theme.padding = 1;
    cui_id close = close_tab_id(item->id);
    if (cui_button_ex(ui, close, &close_theme, item->label,
                      (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                           .disabled = item->disabled,
                                           .content = close_tab_mark,
                                           .content_user = &close_theme}))
        model->requested = item->id;
    cui_set_semantic_details(
        ui, close, (cui_semantic_details){.description = {"Close tab", 9}, .parent = item->id});
    cui_end_box(ui);
}
static bool closable_tabs_scene(cui_context *ui, close_tabs_model *model, cui_tab_variant variant) {
    assert(cui_begin_frame(ui, 640, 540, .016f));
    visible = 99;
    bool changed = cui_tabs(
        ui, 1, &model->theme, model->entries, model->count, &model->selected,
        (cui_tabs_options){.variant = variant, .render = close_tab_body, .render_user = model},
        panel, NULL);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void apply_tab_close(cui_context *ui, close_tabs_model *model) {
    assert(model->requested);
    for (size_t i = 0; i < model->count; i++) {
        if (model->entries[i].id != model->requested)
            continue;
        for (size_t j = i + 1; j < model->count; j++)
            model->entries[j - 1] = model->entries[j];
        model->count--;
        if (model->selected == model->requested) {
            model->selected = 0;
            for (size_t j = 0; j < model->count; j++)
                if (!model->entries[j].disabled) {
                    model->selected = model->entries[j].id;
                    break;
                }
        }
        cui_focus(ui, model->selected);
        model->requested = 0;
        return;
    }
    assert(!"Closing request must refer to a current tab");
}
static void test_closable_tab_composition(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t t = 0; t < 4; t++) {
        for (cui_tab_variant variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
            assert(ui);
            close_tabs_model model = {.count = 3, .selected = 10, .theme = themes[t]};
            memcpy(model.entries, items, sizeof(items));
            closable_tabs_scene(ui, &model, variant);
            click(ui, close_tab_id(30));
            assert(!closable_tabs_scene(ui, &model, variant));
            assert(model.requested == 30 && model.selected == 10 && visible == 0);
            size_t count = 0;
            const cui_semantic *semantics = cui_semantics(ui, &count);
            bool labelled = false;
            for (size_t i = 0; i < count; i++)
                if (semantics[i].id == close_tab_id(30))
                    labelled = semantics[i].role == CUI_ROLE_BUTTON &&
                               semantics[i].name.length == 4 &&
                               semantics[i].details.description.length == 9 &&
                               semantics[i].details.parent == 30;
            assert(labelled);
            apply_tab_close(ui, &model); /* mutation is after borrowing ends */
            closable_tabs_scene(ui, &model, variant);
            assert(model.count == 2 && model.selected == 10 && cui_focused(ui) == 10);
            semantics = cui_semantics(ui, &count);
            for (size_t i = 0; i < count; i++)
                assert(semantics[i].id != 30 && semantics[i].id != close_tab_id(30));
            click(ui, close_tab_id(20));
            assert(!closable_tabs_scene(ui, &model, variant) && !model.requested);
            cui_focus(ui, close_tab_id(10));
            key(ui, CUI_KEY_ENTER);
            assert(!closable_tabs_scene(ui, &model, variant) && model.requested == 10);
            apply_tab_close(ui, &model);
            closable_tabs_scene(ui, &model, variant);
            assert(model.count == 1 && !model.selected && !cui_focused(ui) && visible == 99);
            cui_destroy(ui);
        }
    }
}

static unsigned custom_rows;
static void custom_menu_row(cui_context *ui, void *user) {
    assert(user == &custom_rows);
    custom_rows++;
    cui_text(ui, cui_string("Custom body"), 14, (cui_color){20, 20, 20, 255});
}
static cui_draw_list menu_config_scene(cui_context *ui, cui_menu_tree_state *state,
                                       cui_menu_entry *entries, cui_id *action, bool right) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 540, .016f));
    cui_menu_tree(
        ui, 2, &theme, cui_string("Configured"), entries, 2, action, state,
        (cui_menu_tree_options){
            .popup = {.viewport = {0, 0, 640, 540}, .width = 220, .side = CUI_POPUP_RIGHT},
            .checks_on_right = right});
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static void marker_positions(cui_draw_list list, float *marker, float *label) {
    *marker = *label = -1;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_TEXT) {
            cui_draw_command command = list.commands[i];
            if (command.text.length == 3 && !memcmp(command.text.data, "[x]", 3))
                *marker = command.bounds.x;
            if (command.text.length == 6 && !memcmp(command.text.data, "Toggle", 6))
                *label = command.bounds.x;
        }
    assert(*marker >= 0 && *label >= 0);
}
static void test_menu_configuration(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 540});
    assert(ui);
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 1300);
    cui_id action = 0;
    bool checked = true;
    cui_menu_entry entries[] = {
        {.id = 51, .label = {"Toggle", 6}, .kind = CUI_MENU_CHECK, .checked = &checked},
        {.id = 52,
         .label = {"Accessible item", 15},
         .kind = CUI_MENU_ACTION,
         .render = custom_menu_row,
         .render_user = &custom_rows}};
    menu_config_scene(ui, &state, entries, &action, false);
    cui_rect trigger = cui_bounds(ui, 2);
    click(ui, 2);
    cui_draw_list list = menu_config_scene(ui, &state, entries, &action, false);
    assert(state.overlay.open && state.panels[0].x >= trigger.x + trigger.w);
    float marker, label;
    marker_positions(list, &marker, &label);
    assert(marker < label && custom_rows == 1);
    list = menu_config_scene(ui, &state, entries, &action, true);
    marker_positions(list, &marker, &label);
    assert(marker > label && custom_rows == 2);
    size_t n = 0;
    const cui_semantic *s = cui_semantics(ui, &n);
    bool named = false;
    for (size_t i = 0; i < n; i++)
        if (s[i].id == 52)
            named = s[i].name.length == 15 && s[i].role == CUI_ROLE_MENUITEM;
    assert(named);
    click(ui, 52);
    menu_config_scene(ui, &state, entries, &action, true);
    assert(action == 52 && !state.overlay.open && cui_focused(ui) == 2);
    cui_destroy(ui);
}

static cui_draw_list dropdown_draw;
static unsigned dropdown_custom_calls;
static void dropdown_custom_body(cui_context *ui, void *user) {
    assert(user == &dropdown_custom_calls);
    dropdown_custom_calls++;
    cui_text(ui, cui_string("Custom save"), 11, (cui_color){1, 2, 3, 255});
}
static cui_dropdown_button_result dropdown_scene(cui_context *ui, cui_menu_tree_state *state,
                                                 cui_id *action, const cui_theme *theme,
                                                 cui_dropdown_button_options options) {
    static const cui_menu_entry entries[] = {
        {.id = 2101, .label = {"Save as", 7}, .kind = CUI_MENU_ACTION},
        {.id = 2102, .label = {"Export", 6}, .kind = CUI_MENU_ACTION}};
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_dropdown_button_result result =
        cui_dropdown_button(ui, 2000, 2001, 2002, theme, cui_string("Save"),
                            cui_string("More save actions"), entries, 2, action, state, options);
    dropdown_draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_dropdown_button(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t t = 0; t < 4; t++)
        for (int variant = CUI_DROPDOWN_BUTTON_PRIMARY; variant <= CUI_DROPDOWN_BUTTON_CUSTOM;
             variant++)
            for (int size = CUI_DROPDOWN_BUTTON_MEDIUM; size <= CUI_DROPDOWN_BUTTON_XSMALL;
                 size++) {
                cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
                assert(ui);
                cui_menu_tree_state state;
                cui_menu_tree_init(&state, 2200);
                cui_id action = 0;
                dropdown_custom_calls = 0;
                cui_dropdown_button_options options = {
                    .selected = true,
                    .width = 120,
                    .variant = (cui_dropdown_button_variant)variant,
                    .custom = {{20, 30, 40, 255},
                               {200, 210, 220, 255},
                               {50, 60, 70, 255},
                               {80, 90, 100, 255}},
                    .size = (cui_dropdown_button_size)size,
                    .leading = {"[", 1},
                    .trailing = {"]", 1},
                    .content = dropdown_custom_body,
                    .content_user = &dropdown_custom_calls,
                    .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
                cui_dropdown_button_result result =
                    dropdown_scene(ui, &state, &action, &themes[t], options);
                assert(!result.action_activated && !result.menu_activated &&
                       dropdown_custom_calls == 1);
                cui_rect left = cui_bounds(ui, 2001), right = cui_bounds(ui, 2002);
                float scale = size == CUI_DROPDOWN_BUTTON_XSMALL  ? .625f
                              : size == CUI_DROPDOWN_BUTTON_SMALL ? .8f
                              : size == CUI_DROPDOWN_BUTTON_LARGE ? 1.2f
                                                                  : 1;
                assert(left.w == 120 && left.h == themes[t].control_height * scale &&
                       right.h == left.h && left.x + left.w == right.x);
                bool left_shape = false, right_shape = false, left_border = false,
                     right_border = false;
                for (size_t d = 0; d < dropdown_draw.count; d++) {
                    cui_draw_command command = dropdown_draw.commands[d];
                    bool is_left = command.bounds.x == left.x && command.bounds.w == left.w &&
                                   command.bounds.y == left.y && command.bounds.h == left.h;
                    bool is_right = command.bounds.x == right.x && command.bounds.w == right.w &&
                                    command.bounds.y == right.y && command.bounds.h == right.h;
                    if (command.type == CUI_DRAW_RECT && is_left)
                        left_shape = command.custom_corners &&
                                     command.corner_radii.top_left == themes[t].radius &&
                                     command.corner_radii.bottom_left == themes[t].radius &&
                                     command.corner_radii.top_right == 0 &&
                                     command.corner_radii.bottom_right == 0;
                    if (command.type == CUI_DRAW_RECT && is_right)
                        right_shape = command.custom_corners &&
                                      command.corner_radii.top_right == themes[t].radius &&
                                      command.corner_radii.bottom_right == themes[t].radius &&
                                      command.corner_radii.top_left == 0 &&
                                      command.corner_radii.bottom_left == 0;
                    if (command.type == CUI_DRAW_BORDER && is_left)
                        left_border =
                            command.custom_border_edges && command.border_edges.left == 1 &&
                            command.border_edges.right == 1 && command.border_edges.top == 1 &&
                            command.border_edges.bottom == 1;
                    if (command.type == CUI_DRAW_BORDER && is_right)
                        right_border =
                            command.custom_border_edges && command.border_edges.left == 0 &&
                            command.border_edges.right == 1 && command.border_edges.top == 1 &&
                            command.border_edges.bottom == 1;
                }
                if (variant != CUI_DROPDOWN_BUTTON_LINK && variant != CUI_DROPDOWN_BUTTON_TEXT)
                    assert(left_shape && right_shape);
                if (variant == CUI_DROPDOWN_BUTTON_OUTLINE ||
                    variant == CUI_DROPDOWN_BUTTON_DEFAULT)
                    assert(left_border && right_border);
                size_t count = 0;
                const cui_semantic *semantics = cui_semantics(ui, &count);
                cui_role expected_role =
                    variant == CUI_DROPDOWN_BUTTON_LINK ? CUI_ROLE_LINK : CUI_ROLE_BUTTON;
                bool action_named = false, trigger_named = false;
                for (size_t i = 0; i < count; i++) {
                    if (semantics[i].id == 2001)
                        action_named = semantics[i].role == expected_role &&
                                       semantics[i].name.length == 4 && !semantics[i].checked;
                    if (semantics[i].id == 2002)
                        trigger_named = semantics[i].role == expected_role &&
                                        semantics[i].name.length == 17 && !semantics[i].checked;
                }
                assert(action_named && trigger_named);
                cui_destroy(ui);
            }

    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 2200);
    cui_id action = 0;
    cui_dropdown_button_options options = {
        .variant = CUI_DROPDOWN_BUTTON_OUTLINE,
        .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
    dropdown_scene(ui, &state, &action, &theme, options);
    click(ui, 2001);
    cui_dropdown_button_result result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(result.action_activated && !result.menu_activated && !state.overlay.open && !action);
    click(ui, 2002);
    result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(!result.action_activated && !result.menu_activated && state.overlay.open &&
           cui_focused(ui) == 2101);
    key(ui, CUI_KEY_ENTER);
    result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(!result.action_activated && result.menu_activated && action == 2101 &&
           !state.overlay.open && cui_focused(ui) == 2002);
    options.action_loading = true;
    dropdown_scene(ui, &state, &action, &theme, options);
    click(ui, 2001);
    result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(!result.action_activated);
    click(ui, 2002);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(state.overlay.open);
    options.disabled = true;
    result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(!result.action_activated && !result.menu_activated && !state.overlay.open);
    cui_destroy(ui);
}
static cui_color dropdown_surface(cui_context *ui, cui_id id) {
    cui_rect bounds = cui_bounds(ui, id);
    for (size_t i = 0; i < dropdown_draw.count; i++) {
        cui_draw_command command = dropdown_draw.commands[i];
        if (command.type == CUI_DRAW_RECT && command.bounds.x == bounds.x &&
            command.bounds.y == bounds.y && command.bounds.w == bounds.w &&
            command.bounds.h == bounds.h)
            return command.color;
    }
    return (cui_color){0};
}
static void test_dropdown_inheritance_and_ghost_pair(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 2200);
    cui_id action = 0;
    cui_dropdown_button_options options = {
        .inherit_action_variant = true,
        .inherit_action_size = true,
        .action_variant = CUI_DROPDOWN_BUTTON_DANGER,
        .action_size = CUI_DROPDOWN_BUTTON_SMALL,
        .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(cui_bounds(ui, 2001).h == theme.control_height * .8f);
    assert(cui_bounds(ui, 2002).h == cui_bounds(ui, 2001).h);
    assert(same_color(dropdown_surface(ui, 2001), theme.danger));
    assert(same_color(dropdown_surface(ui, 2002), theme.danger));
    options.inherit_action_variant = options.inherit_action_size = false;
    options.variant = CUI_DROPDOWN_BUTTON_GHOST;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(cui_bounds(ui, 2001).h == theme.control_height);
    assert(dropdown_surface(ui, 2001).a == 0 && dropdown_surface(ui, 2002).a == 0);
    for (cui_id id = 2001; id <= 2002; id++) {
        cui_rect bounds = cui_bounds(ui, id);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 2, .y = bounds.y + 2}));
        dropdown_scene(ui, &state, &action, &theme, options);
        assert(dropdown_surface(ui, 2001).a && dropdown_surface(ui, 2002).a);
        cui_color idle = theme.accent;
        idle.a /= 2;
        assert(same_color(dropdown_surface(ui, id == 2001 ? 2002 : 2001), idle));
    }
    click(ui, 2002);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(state.overlay.open);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
    dropdown_scene(ui, &state, &action, &theme, options);
    cui_color held = theme.accent;
    held.a /= 2;
    assert(same_color(dropdown_surface(ui, 2001), held));
    assert(same_color(dropdown_surface(ui, 2002), held));
    key(ui, CUI_KEY_ESCAPE);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(!state.overlay.open && dropdown_surface(ui, 2001).a == 0 &&
           dropdown_surface(ui, 2002).a == 0);
    options.disabled = true;
    cui_rect bounds = cui_bounds(ui, 2001);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 2, .y = bounds.y + 2}));
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(dropdown_surface(ui, 2001).a == 0 && dropdown_surface(ui, 2002).a == 0);
    cui_destroy(ui);
}
static void test_dropdown_optional_halves(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 2200);
    cui_id action = 0;
    cui_dropdown_button_options options = {
        .hide_action = true, .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
    dropdown_scene(ui, &state, &action, &theme, options);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        assert(semantics[i].id != 2001);
    assert(cui_bounds(ui, 2002).x == cui_bounds(ui, 2000).x);
    click(ui, 2002);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(state.overlay.open);
    key(ui, CUI_KEY_ENTER);
    cui_dropdown_button_result result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(result.menu_activated && !result.action_activated && action == 2101);
    click(ui, 2002);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(state.overlay.open);
    options.hide_action = false;
    options.hide_menu = true;
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(!state.overlay.open && state.depth == 0);
    semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        assert(semantics[i].id != 2002);
    click(ui, 2001);
    result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(result.action_activated && !result.menu_activated);
    cui_focus(ui, 2001);
    key(ui, CUI_KEY_ENTER);
    result = dropdown_scene(ui, &state, &action, &theme, options);
    assert(result.action_activated && !result.menu_activated);
    cui_destroy(ui);
}
static void test_dropdown_anchor_corners(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 2200);
    cui_id action = 0;
    const cui_menu_entry entries[] = {{.id = 2101, .label = {"Save as", 7}}};
    for (int corner = CUI_DROPDOWN_ANCHOR_TOP_RIGHT; corner <= CUI_DROPDOWN_ANCHOR_BOTTOM_LEFT;
         corner++) {
        for (int frame = 0; frame < 2; frame++) {
            assert(cui_begin_frame(ui, 640, 480, .016f));
            cui_begin_box(ui, 1999, (cui_box){.width = 640, .height = 480, .padding = 100});
            cui_dropdown_button(
                ui, 2000, 2001, 2002, &theme, cui_string("Save"), cui_string("Actions"), entries, 1,
                &action, &state,
                (cui_dropdown_button_options){
                    .width = 100,
                    .anchor = (cui_dropdown_button_anchor)corner,
                    .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 160}}});
            cui_end_box(ui);
            cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            if (!frame)
                click(ui, 2002);
        }
        assert(state.overlay.open);
        cui_rect anchor = cui_bounds(ui, 2002), panel = state.panels[0];
        bool right =
            corner == CUI_DROPDOWN_ANCHOR_TOP_RIGHT || corner == CUI_DROPDOWN_ANCHOR_BOTTOM_RIGHT;
        bool above =
            corner == CUI_DROPDOWN_ANCHOR_BOTTOM_RIGHT || corner == CUI_DROPDOWN_ANCHOR_BOTTOM_LEFT;
        assert(panel.x == (right ? anchor.x + anchor.w - panel.w : anchor.x));
        assert(panel.y == (above ? anchor.y - panel.h - 4 : anchor.y + anchor.h + 4));
        cui_overlay_close(ui, &state.overlay);
    }
    cui_destroy(ui);
}
static void test_dropdown_action_tooltip(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 2200);
    cui_hover_state tooltip = {0};
    cui_id action = 0;
    cui_dropdown_button_options options = {
        .tooltip_id = 2300,
        .tooltip = {"Save the document", 17},
        .tooltip_state = &tooltip,
        .tooltip_options = {.viewport = {0, 0, 640, 480},
                            .width = 180,
                            .height = 40,
                            .dt = .016f,
                            .open_delay = .032f},
        .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
    dropdown_scene(ui, &state, &action, &theme, options);
    cui_rect bounds = cui_bounds(ui, 2001);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 2, .y = bounds.y + 2}));
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(!tooltip.open && tooltip.pending);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(tooltip.open);
    bool text = false;
    for (size_t i = 0; i < dropdown_draw.count; i++) {
        cui_draw_command command = dropdown_draw.commands[i];
        text |= command.type == CUI_DRAW_TEXT && command.text.length == options.tooltip.length &&
                memcmp(command.text.data, options.tooltip.data, options.tooltip.length) == 0;
    }
    assert(text);
    key(ui, CUI_KEY_ESCAPE);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(!tooltip.open);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
    dropdown_scene(ui, &state, &action, &theme, options);
    cui_focus(ui, 2001);
    dropdown_scene(ui, &state, &action, &theme, options);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(tooltip.open);
    options.action_loading = true;
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(!tooltip.open);
    options.action_loading = false;
    click(ui, 2002);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(state.overlay.open && !tooltip.open);
    cui_destroy(ui);
}
static void test_tab_max_width(void) {
    cui_theme theme = cui_theme_light();
    const cui_choice_item entries[] = {
        {10, {"A", 1}, false}, {20, {"A long \xc3\xa9\xc3\xa9\xc3\xa9 tab label", 23}, false}};
    for (int variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_id value = 10;
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_tabs(ui, 1, &theme, entries, 2, &value,
                 (cui_tabs_options){.variant = (cui_tab_variant)variant, .max_width = 100}, NULL,
                 NULL);
        cui_draw_list draw = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        assert(cui_bounds(ui, 10).w < 100 && cui_bounds(ui, 20).w == 100);
        bool marker = false, full = false;
        for (size_t i = 0; i < draw.count; i++) {
            cui_draw_command command = draw.commands[i];
            if (command.type != CUI_DRAW_TEXT)
                continue;
            marker |= command.text.length == 3 && memcmp(command.text.data, "\xe2\x80\xa6", 3) == 0;
            full |= command.text.length == entries[1].label.length;
            if (command.text.length > 1 && command.text.data[0] == 'A')
                assert(command.text.data[command.text.length - 1] != (char)0xc3);
        }
        assert(marker && !full);
        size_t count;
        const cui_semantic *semantics = cui_semantics(ui, &count);
        bool named = false;
        for (size_t i = 0; i < count; i++)
            named |= semantics[i].id == 20 && semantics[i].name.length == entries[1].label.length;
        assert(named);
        click(ui, 20);
        assert(cui_begin_frame(ui, 640, 480, .016f));
        assert(cui_tabs(ui, 1, &theme, entries, 2, &value,
                        (cui_tabs_options){.variant = (cui_tab_variant)variant, .max_width = 100},
                        NULL, NULL));
        cui_end_frame(ui);
        assert(value == 20);
        cui_destroy(ui);
    }
}
static void tab_scroll_prefix(cui_context *ui, void *user) {
    (void)user;
    cui_begin_box(ui, 3000, (cui_box){.width = 20, .height = 20});
    cui_end_box(ui);
}
static void tab_scroll_suffix(cui_context *ui, void *user) {
    (void)user;
    cui_begin_box(ui, 3001, (cui_box){.width = 20, .height = 20});
    cui_end_box(ui);
}
static bool tab_scroll_scene(cui_context *ui, cui_tab_scroll_state *scroll, cui_id *value,
                             bool vertical, bool disabled, size_t count) {
    cui_theme theme = cui_theme_light();
    const cui_choice_item entries[] = {{10, {"First long tab", 14}, false},
                                       {20, {"Second long tab", 15}, false},
                                       {30, {"Third long tab", 14}, false}};
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_tabs(ui, 1, &theme, entries, count, value,
                            (cui_tabs_options){.vertical = vertical,
                                               .disabled = disabled,
                                               .max_width = 100,
                                               .scroll = scroll,
                                               .bar_extent = vertical ? 48 : 110,
                                               .prefix = tab_scroll_prefix,
                                               .suffix = tab_scroll_suffix},
                            NULL, NULL);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void test_tab_scrolling(void) {
    for (int vertical = 0; vertical < 2; vertical++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_tab_scroll_state scroll = {0};
        cui_id value = 10;
        tab_scroll_scene(ui, &scroll, &value, vertical, false, 3);
        tab_scroll_scene(ui, &scroll, &value, vertical, false, 3);
        assert(scroll.maximum > 0 && scroll.offset == 0);
        cui_rect prefix = cui_bounds(ui, 3000), suffix = cui_bounds(ui, 3001);
        cui_rect viewport = scroll.viewport, first = cui_bounds(ui, 10);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = viewport.x + 2,
                                               .y = viewport.y + 2,
                                               .dx = vertical ? 0 : -1,
                                               .dy = vertical ? -1 : 0}));
        assert(!tab_scroll_scene(ui, &scroll, &value, vertical, false, 3));
        assert(scroll.changed && scroll.offset == 32 && value == 10);
        cui_rect moved = cui_bounds(ui, 10);
        assert((vertical ? moved.y : moved.x) == (vertical ? first.y : first.x) - 32);
        assert(cui_bounds(ui, 3000).x == prefix.x && cui_bounds(ui, 3000).y == prefix.y);
        assert(cui_bounds(ui, 3001).x == suffix.x && cui_bounds(ui, 3001).y == suffix.y);
        if (!vertical) {
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                                   .x = viewport.x + 2,
                                                   .y = viewport.y + 2,
                                                   .dx = -1}));
            click(ui, 20);
            assert(!tab_scroll_scene(ui, &scroll, &value, vertical, false, 3));
            assert(scroll.offset == 64 && value == 10);
            scroll.offset = 32;
        }
        value = 30;
        tab_scroll_scene(ui, &scroll, &value, vertical, false, 3);
        assert(scroll.offset == 32);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = viewport.x + 2,
                                               .y = viewport.y + 2,
                                               .dx = vertical ? 0 : -1000,
                                               .dy = vertical ? -1000 : 0}));
        tab_scroll_scene(ui, &scroll, &value, vertical, false, 3);
        assert(scroll.offset == scroll.maximum);
        float end = scroll.offset;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = viewport.x + 2,
                                               .y = viewport.y + 2,
                                               .dx = vertical ? 0 : 1000,
                                               .dy = vertical ? 1000 : 0}));
        tab_scroll_scene(ui, &scroll, &value, vertical, true, 3);
        assert(scroll.offset == end && !scroll.changed);
        assert(cui_queue_event(ui,
                               (cui_event){.type = CUI_EVENT_SCROLL, .x = 600, .y = 450, .dy = 1}));
        tab_scroll_scene(ui, &scroll, &value, vertical, false, 3);
        assert(scroll.offset == end && !scroll.changed);
        tab_scroll_scene(ui, &scroll, &value, vertical, false, 1);
        assert(scroll.maximum == (vertical ? 0 : 2) && scroll.offset == scroll.maximum);
        cui_destroy(ui);
    }
}
static bool tab_overflow_scene(cui_context *ui, cui_tab_scroll_state *scroll,
                               cui_tab_overflow *overflow, cui_id *value, float extent,
                               size_t count) {
    cui_theme theme = cui_theme_light();
    const cui_choice_item entries[] = {{10, {"First long tab", 14}, false},
                                       {20, {"Disabled long tab", 17}, true},
                                       {30, {"Third long tab", 14}, false}};
    visible = 99;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_tabs(
        ui, 1, &theme, entries, count, value,
        (cui_tabs_options){
            .max_width = 100, .scroll = scroll, .bar_extent = extent, .overflow = overflow},
        panel, NULL);
    dropdown_draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void test_tab_overflow_menu(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_tab_scroll_state scroll = {0};
    cui_tab_overflow overflow;
    cui_menu_entry scratch[3];
    cui_tab_overflow_init(&overflow, 3100, 3101, scratch, 3);
    overflow.options.popup.viewport = (cui_rect){0, 0, 640, 480};
    overflow.options.popup.width = 220;
    cui_id value = 10;
    tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 3);
    assert(overflow.visible && !overflow.menu.overlay.open);
    click(ui, 3101);
    tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 3);
    assert(overflow.menu.overlay.open);
    bool full = false;
    for (size_t i = 0; i < dropdown_draw.count; i++) {
        cui_draw_command command = dropdown_draw.commands[i];
        full |= command.type == CUI_DRAW_TEXT && command.text.length == 17 &&
                memcmp(command.text.data, "Disabled long tab", 17) == 0;
    }
    assert(full);
    key(ui, CUI_KEY_DOWN);
    key(ui, CUI_KEY_ENTER);
    assert(tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 3));
    assert(value == 30 && visible == 2 && !overflow.menu.overlay.open && cui_focused(ui) == 3101);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    bool selected = false;
    for (size_t i = 0; i < count; i++)
        selected |= semantics[i].id == 30 && semantics[i].details.selected;
    assert(selected);
    click(ui, 3101);
    tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 3);
    assert(overflow.menu.overlay.open);
    tab_overflow_scene(ui, &scroll, &overflow, &value, 400, 3);
    assert(!overflow.visible && !overflow.menu.overlay.open && overflow.menu.depth == 0);
    semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        assert(semantics[i].id != 3101);
    tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 3);
    click(ui, 3101);
    tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 3);
    assert(overflow.menu.overlay.open);
    tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 0);
    assert(!overflow.visible && !overflow.menu.overlay.open);
    overflow.capacity = 1;
    tab_overflow_scene(ui, &scroll, &overflow, &value, 110, 3);
    assert(!overflow.visible);
    cui_destroy(ui);
}
static bool tab_drag_scene(cui_context *ui, cui_choice_item *entries, size_t count, cui_id *value,
                           cui_tab_drag_state *drag, bool vertical, bool disabled) {
    cui_theme theme = cui_theme_light();
    visible = 99;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_tabs(
        ui, 1, &theme, entries, count, value,
        (cui_tabs_options){.vertical = vertical, .disabled = disabled, .drag = drag}, panel, NULL);
    dropdown_draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void test_tab_drag_reorder(void) {
    for (int vertical = 0; vertical < 2; vertical++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_choice_item entries[] = {
            {10, {"First", 5}, false}, {20, {"Second", 6}, false}, {30, {"Third", 5}, false}};
        cui_id value = 20;
        cui_tab_drag_state drag = {0};
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        cui_rect first = cui_bounds(ui, 10), last = cui_bounds(ui, 30);
        float x = vertical ? last.x + 2 : last.x + last.w - 2;
        float y = vertical ? last.y + last.h - 2 : last.y + 2;
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = first.x + 2, .y = first.y + 2}));
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(drag.candidate == 10 && !drag.active);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x, .y = y}));
        assert(!tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false));
        assert(drag.active && drag.before == 0 && !drag.requested && value == 20);
        bool marker = false;
        for (size_t i = 0; i < dropdown_draw.count; i++) {
            cui_draw_command command = dropdown_draw.commands[i];
            marker |= command.type == CUI_DRAW_RECT &&
                      (vertical ? command.bounds.h == 2 : command.bounds.w == 2) &&
                      same_color(command.color, cui_theme_light().primary);
        }
        assert(marker);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
        assert(!tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false));
        assert(!drag.active && drag.requested && drag.moved == 10 && drag.before == 0 &&
               value == 20);
        cui_choice_item moved = entries[0];
        entries[0] = entries[1];
        entries[1] = entries[2];
        entries[2] = moved;
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(value == 20 && visible == 0 && !drag.requested);
        first = cui_bounds(ui, 20);
        last = cui_bounds(ui, 10);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = first.x + 2, .y = first.y + 2}));
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = last.x + 2, .y = last.y + 2}));
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(drag.active && drag.before == 10);
        key(ui, CUI_KEY_ESCAPE);
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(!drag.active && !drag.candidate && !drag.requested);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = last.x + 2, .y = last.y + 2}));
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(!drag.requested && value == 20);
        entries[2].disabled = true;
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        click(ui, 10);
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(!drag.candidate && !drag.requested && value == 20);
        entries[2].disabled = false;
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        click(ui, 10);
        assert(tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false));
        assert(value == 10 && !drag.requested);
        first = cui_bounds(ui, 10);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = first.x + 2, .y = first.y + 2}));
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(drag.active && !drag.drop_valid);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 600, .y = 450}));
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(!drag.requested && !drag.active);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = first.x + 2, .y = first.y + 2}));
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = first.x + 10, .y = first.y + 10}));
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(drag.active);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
        tab_drag_scene(ui, entries, 3, &value, &drag, vertical, false);
        assert(!drag.active && !drag.candidate && !drag.requested);
        cui_destroy(ui);
    }
}
static cui_size counting_tab_measure(void *user, cui_str text, float font_size) {
    unsigned *calls = user;
    (*calls)++;
    return (cui_size){.width = (float)text.length * 10, .height = font_size};
}
static void test_long_tab_label_measurement(void) {
    unsigned calls = 0;
    cui_context *ui = cui_create((cui_config){
        .width = 640, .height = 480, .measure = counting_tab_measure, .measure_user = &calls});
    assert(ui);
    char label[4096];
    memset(label, 'a', sizeof(label));
    cui_choice_item entry = {10, {label, sizeof(label)}, false};
    cui_id value = 10;
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_tabs(ui, 1, &theme, &entry, 1, &value, (cui_tabs_options){.max_width = 100}, NULL, NULL);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && cui_bounds(ui, 10).w == 100);
    assert(calls < 64); /* Long labels do not remeasure each shorter prefix. */
    bool marker = false;
    for (size_t i = 0; i < draw.count; i++)
        marker |= draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 3 &&
                  memcmp(draw.commands[i].text.data, "\xe2\x80\xa6", 3) == 0;
    assert(marker);
    cui_destroy(ui);
}
static void test_dropdown_colored_outline(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    const cui_dropdown_button_variant variants[] = {
        CUI_DROPDOWN_BUTTON_PRIMARY, CUI_DROPDOWN_BUTTON_SECONDARY, CUI_DROPDOWN_BUTTON_DANGER,
        CUI_DROPDOWN_BUTTON_WARNING, CUI_DROPDOWN_BUTTON_SUCCESS,   CUI_DROPDOWN_BUTTON_INFO};
    for (size_t t = 0; t < 4; t++)
        for (size_t v = 0; v < sizeof(variants) / sizeof(variants[0]); v++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
            assert(ui);
            cui_menu_tree_state state;
            cui_menu_tree_init(&state, 2200);
            cui_id action = 0;
            cui_dropdown_button_options options = {
                .outline = true,
                .variant = variants[v],
                .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
            dropdown_scene(ui, &state, &action, &themes[t], options);
            assert(dropdown_surface(ui, 2001).a == 25 && dropdown_surface(ui, 2002).a == 25);
            cui_color expected = variants[v] == CUI_DROPDOWN_BUTTON_PRIMARY   ? themes[t].primary
                                 : variants[v] == CUI_DROPDOWN_BUTTON_DANGER  ? themes[t].danger
                                 : variants[v] == CUI_DROPDOWN_BUTTON_WARNING ? themes[t].warning
                                 : variants[v] == CUI_DROPDOWN_BUTTON_SUCCESS ? themes[t].success
                                 : variants[v] == CUI_DROPDOWN_BUTTON_INFO    ? themes[t].info
                                                                              : themes[t].text;
            bool foreground = false;
            for (size_t i = 0; i < dropdown_draw.count; i++) {
                cui_draw_command command = dropdown_draw.commands[i];
                foreground |= command.type == CUI_DRAW_TEXT && command.text.length == 4 &&
                              same_color(command.color, expected);
            }
            assert(foreground);
            if (variants[v] >= CUI_DROPDOWN_BUTTON_DANGER) {
                options.outline = false;
                dropdown_scene(ui, &state, &action, &themes[t], options);
                assert(same_color(dropdown_surface(ui, 2001), expected));
                cui_color on = variants[v] == CUI_DROPDOWN_BUTTON_DANGER    ? themes[t].on_danger
                               : variants[v] == CUI_DROPDOWN_BUTTON_WARNING ? themes[t].on_warning
                               : variants[v] == CUI_DROPDOWN_BUTTON_SUCCESS ? themes[t].on_success
                                                                            : themes[t].on_info;
                bool filled_label = false;
                for (size_t i = 0; i < dropdown_draw.count; i++) {
                    cui_draw_command command = dropdown_draw.commands[i];
                    filled_label |= command.type == CUI_DRAW_TEXT && command.text.length == 4 &&
                                    same_color(command.color, on);
                }
                assert(filled_label);
                options.outline = true;
                dropdown_scene(ui, &state, &action, &themes[t], options);
            }
            cui_rect bounds = cui_bounds(ui, 2001);
            assert(cui_queue_event(
                ui,
                (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 2, .y = bounds.y + 2}));
            dropdown_scene(ui, &state, &action, &themes[t], options);
            assert(dropdown_surface(ui, 2001).a == 51 && dropdown_surface(ui, 2002).a == 25);
            options.selected = true;
            dropdown_scene(ui, &state, &action, &themes[t], options);
            assert(dropdown_surface(ui, 2001).a == 102 && dropdown_surface(ui, 2002).a == 102);
            click(ui, 2001);
            assert(dropdown_scene(ui, &state, &action, &themes[t], options).action_activated);
            options.disabled = true;
            dropdown_scene(ui, &state, &action, &themes[t], options);
            assert(dropdown_surface(ui, 2001).a == 12 && dropdown_surface(ui, 2002).a == 12);
            cui_destroy(ui);
        }
}
static void test_dropdown_inner_button_presentation(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 2200);
    cui_id action = 0;
    cui_dropdown_button_options options = {
        .action_selected = true,
        .variant = CUI_DROPDOWN_BUTTON_GHOST,
        .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(
        same_color(dropdown_surface(ui, 2001), theme.buttons[CUI_THEME_BUTTON_SECONDARY].active));
    assert(
        same_color(dropdown_surface(ui, 2002), theme.buttons[CUI_THEME_BUTTON_SECONDARY].active));
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (semantics[i].id == 2001 || semantics[i].id == 2002)
            assert(!semantics[i].checked && !semantics[i].details.selected &&
                   semantics[i].value == 0);
    options.action_selected = false;
    options.variant = CUI_DROPDOWN_BUTTON_DANGER;
    options.action_outline = true;
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(dropdown_surface(ui, 2001).a == 25);
    assert(same_color(dropdown_surface(ui, 2002), theme.danger));
    bool action_color = false, trigger_color = false;
    for (size_t i = 0; i < dropdown_draw.count; i++) {
        cui_draw_command command = dropdown_draw.commands[i];
        if (command.type != CUI_DRAW_TEXT)
            continue;
        action_color |= command.text.length == 4 && same_color(command.color, theme.danger);
        trigger_color |= command.text.length == 1 && same_color(command.color, theme.on_danger);
    }
    assert(action_color && trigger_color);
    options.outline = true;
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(dropdown_surface(ui, 2001).a == 25 && dropdown_surface(ui, 2002).a == 25);
    click(ui, 2002);
    dropdown_scene(ui, &state, &action, &theme, options);
    assert(state.overlay.open);
    semantics = cui_semantics(ui, &count);
    bool expanded = false;
    for (size_t i = 0; i < count; i++)
        expanded |=
            semantics[i].id == 2002 && semantics[i].details.expanded && !semantics[i].checked;
    assert(expanded);
    cui_destroy(ui);
}
static bool reveal_tabs_scene(cui_context *ui, cui_tab_scroll_state *scroll, cui_id *value,
                              bool automatic) {
    cui_theme theme = cui_theme_light();
    const cui_choice_item entries[] = {{10, {"First long tab", 14}, false},
                                       {20, {"Second long tab", 15}, false},
                                       {30, {"Third long tab", 14}, false}};
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_tabs(
        ui, 1, &theme, entries, 3, value,
        (cui_tabs_options){
            .max_width = 100, .scroll = scroll, .bar_extent = 110, .reveal_selected = automatic},
        NULL, NULL);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void test_tab_reveal_requests(void) {
    cui_tab_scroll_state geometry = {.offset = 10, .maximum = 300};
    assert(cui_tab_scroll_reveal(&geometry, 180, 50, 100) && geometry.offset == 130);
    assert(!cui_tab_scroll_reveal(&geometry, 140, 20, 100) && geometry.offset == 130);
    assert(cui_tab_scroll_reveal(&geometry, 20, 200, 100) && geometry.offset == 20);
    assert(!cui_tab_scroll_reveal(&geometry, NAN, 20, 100) && geometry.offset == 20);
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_tab_scroll_state scroll = {0};
    cui_id value = 10;
    reveal_tabs_scene(ui, &scroll, &value, false);
    scroll.reveal_id = 30;
    reveal_tabs_scene(ui, &scroll, &value, false);
    assert(!scroll.reveal_id && scroll.offset == scroll.maximum && value == 10);
    cui_rect last = cui_bounds(ui, 30), viewport = cui_bounds(ui, scroll.receiving_id);
    assert(last.x >= viewport.x && last.x + last.w <= viewport.x + viewport.w);
    scroll.reveal_id = 10;
    reveal_tabs_scene(ui, &scroll, &value, false);
    assert(scroll.offset == 0);
    value = 30;
    reveal_tabs_scene(ui, &scroll, &value, false);
    assert(scroll.offset == 0); /* Selection alone preserves manual position by default. */
    value = 10;
    reveal_tabs_scene(ui, &scroll, &value, true);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_END);
    assert(reveal_tabs_scene(ui, &scroll, &value, true));
    assert(value == 30 && scroll.offset == scroll.maximum);
    last = cui_bounds(ui, 30);
    viewport = cui_bounds(ui, scroll.receiving_id);
    assert(last.x >= viewport.x && last.x + last.w <= viewport.x + viewport.w);
    scroll.offset = 32;
    reveal_tabs_scene(ui, &scroll, &value, true);
    assert(scroll.offset == 32); /* No new selection means no automatic reveal. */
    scroll.reveal_id = 9999;
    reveal_tabs_scene(ui, &scroll, &value, false);
    assert(!scroll.reveal_id && scroll.offset == 32);
    cui_destroy(ui);
}
static void scrollbar_tabs_scene(cui_context *ui, cui_tab_scroll_state *scroll, bool vertical,
                                 bool disabled, size_t count) {
    cui_theme theme = cui_theme_light();
    const cui_choice_item entries[] = {{10, {"First long tab", 14}, false},
                                       {20, {"Second long tab", 15}, false},
                                       {30, {"Third long tab", 14}, false}};
    cui_id value = 10;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_tabs(ui, 1, &theme, entries, count, &value,
             (cui_tabs_options){.vertical = vertical,
                                .disabled = disabled,
                                .max_width = 100,
                                .scroll = scroll,
                                .bar_extent = vertical ? 48 : 110,
                                .scrollbar = true},
             NULL, NULL);
    dropdown_draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_tab_scrollbars(void) {
    for (int vertical = 0; vertical < 2; vertical++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_tab_scroll_state scroll = {0};
        for (int frame = 0; frame < 3; frame++)
            scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.scrollbar_id && scroll.maximum > 0);
        cui_id id = scroll.scrollbar_id;
        cui_rect track = cui_bounds(ui, id);
        assert((vertical ? track.w : track.h) == 6);
        bool thumb = false;
        for (size_t i = 0; i < dropdown_draw.count; i++) {
            cui_draw_command command = dropdown_draw.commands[i];
            thumb |= command.type == CUI_DRAW_RECT &&
                     (vertical ? command.bounds.w == 6 : command.bounds.h == 6) &&
                     same_color(command.color, cui_theme_light().primary);
        }
        assert(thumb);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = track.x + 2, .y = track.y + 2}));
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.scrollbar_dragging && scroll.offset == 0);
        float x = vertical ? track.x + 2 : track.x + track.w - 2;
        float y = vertical ? track.y + track.h - 2 : track.y + 2;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x, .y = y}));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(!scroll.scrollbar_dragging && scroll.offset == scroll.maximum && scroll.changed);
        size_t count;
        const cui_semantic *semantics = cui_semantics(ui, &count);
        bool range = false;
        for (size_t i = 0; i < count; i++)
            range |= semantics[i].id == id && semantics[i].role == CUI_ROLE_SCROLLBAR &&
                     semantics[i].details.maximum == scroll.maximum &&
                     semantics[i].value == scroll.offset &&
                     semantics[i].details.orientation ==
                         (vertical ? CUI_ORIENTATION_VERTICAL : CUI_ORIENTATION_HORIZONTAL);
        assert(range);
        cui_focus(ui, id);
        key(ui, CUI_KEY_HOME);
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.offset == 0);
        key(ui, vertical ? CUI_KEY_DOWN : CUI_KEY_RIGHT);
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.offset == 32);
        key(ui, CUI_KEY_PAGEDOWN);
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.offset == fminf(scroll.maximum, vertical ? 80 : 142));
        key(ui, CUI_KEY_END);
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.offset == scroll.maximum);
        float end = scroll.offset;
        key(ui, CUI_KEY_HOME);
        scrollbar_tabs_scene(ui, &scroll, vertical, true, 3);
        assert(scroll.offset == end && !scroll.changed);
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = track.x + 2,
                                               .y = track.y + 2,
                                               .dx = vertical ? 0 : 1,
                                               .dy = vertical ? 1 : 0}));
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.offset == end - 32);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = track.x + 2, .y = track.y + 2}));
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(scroll.scrollbar_dragging);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 3);
        assert(!scroll.scrollbar_dragging);
        scrollbar_tabs_scene(ui, &scroll, vertical, false, 1);
        assert(!scroll.scrollbar_id && scroll.offset == 0);
        semantics = cui_semantics(ui, &count);
        for (size_t i = 0; i < count; i++)
            assert(semantics[i].id != id);
        cui_destroy(ui);
    }
}
static void test_dropdown_presentation_variants(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t t = 0; t < 4; t++)
        for (int v = CUI_DROPDOWN_BUTTON_DEFAULT; v <= CUI_DROPDOWN_BUTTON_CUSTOM; v++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
            assert(ui);
            cui_menu_tree_state state;
            cui_menu_tree_init(&state, 2200);
            cui_id action = 0;
            cui_dropdown_button_options options = {
                .variant = (cui_dropdown_button_variant)v,
                .custom = {{20, 30, 40, 255},
                           {200, 210, 220, 255},
                           {50, 60, 70, 255},
                           {80, 90, 100, 255}},
                .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
            dropdown_scene(ui, &state, &action, &themes[t], options);
            bool transparent = v == CUI_DROPDOWN_BUTTON_LINK || v == CUI_DROPDOWN_BUTTON_TEXT;
            cui_color expected = v == CUI_DROPDOWN_BUTTON_CUSTOM    ? options.custom.color
                                 : v == CUI_DROPDOWN_BUTTON_DEFAULT ? themes[t].surface
                                                                    : (cui_color){0};
            if (v == CUI_DROPDOWN_BUTTON_CUSTOM)
                expected.a = (uint8_t)(expected.a * .2f);
            if (transparent)
                assert(dropdown_surface(ui, 2001).a == 0);
            else
                assert(same_color(dropdown_surface(ui, 2001), expected));
            if (v == CUI_DROPDOWN_BUTTON_LINK) {
                size_t semantic_count;
                const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
                unsigned links = 0;
                for (size_t i = 0; i < semantic_count; i++)
                    if (semantics[i].id == 2001 || semantics[i].id == 2002) {
                        assert(semantics[i].role == CUI_ROLE_LINK);
                        links++;
                    }
                assert(links == 2);
                cui_rect line = cui_bounds(ui, 2001 ^ UINT64_C(0xc8134e39119b751d));
                assert(line.h == 1 && line.w > 0);
            }
            cui_rect bounds = cui_bounds(ui, 2001);
            assert(cui_queue_event(
                ui,
                (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 2, .y = bounds.y + 2}));
            dropdown_scene(ui, &state, &action, &themes[t], options);
            if (transparent)
                assert(dropdown_surface(ui, 2001).a == 0);
            if (v == CUI_DROPDOWN_BUTTON_CUSTOM)
                assert(same_color(dropdown_surface(ui, 2001), options.custom.hover));
            if (v == CUI_DROPDOWN_BUTTON_CUSTOM) {
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                       .x = bounds.x + 2,
                                                       .y = bounds.y + 2}));
                dropdown_scene(ui, &state, &action, &themes[t], options);
                assert(same_color(dropdown_surface(ui, 2001), options.custom.active));
                dropdown_scene(ui, &state, &action, &themes[t], options);
                assert(same_color(dropdown_surface(ui, 2001), options.custom.active));
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
                dropdown_scene(ui, &state, &action, &themes[t], options);
                assert(same_color(dropdown_surface(ui, 2001), options.custom.hover));
                cui_focus(ui, 2001);
                key(ui, CUI_KEY_SPACE);
                dropdown_scene(ui, &state, &action, &themes[t], options);
                assert(same_color(dropdown_surface(ui, 2001), options.custom.active));
                assert(cui_queue_event(
                    ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
                assert(dropdown_scene(ui, &state, &action, &themes[t], options).action_activated);
                assert(same_color(dropdown_surface(ui, 2001), options.custom.hover));
            }
            options.selected = true;
            dropdown_scene(ui, &state, &action, &themes[t], options);
            if (v == CUI_DROPDOWN_BUTTON_CUSTOM)
                assert(same_color(dropdown_surface(ui, 2001), options.custom.active));
            cui_focus(ui, 2001);
            key(ui, CUI_KEY_ENTER);
            assert(dropdown_scene(ui, &state, &action, &themes[t], options).action_activated);
            options.disabled = true;
            key(ui, CUI_KEY_ENTER);
            assert(!dropdown_scene(ui, &state, &action, &themes[t], options).action_activated);
            if (transparent)
                assert(dropdown_surface(ui, 2001).a == 0);
            options.disabled = false;
            options.outline = true;
            dropdown_scene(ui, &state, &action, &themes[t], options);
            if (v == CUI_DROPDOWN_BUTTON_CUSTOM)
                assert(dropdown_surface(ui, 2001).a == options.custom.active.a);
            cui_destroy(ui);
        }
}
static cui_menu_entry scrolling_menu[12], scrolling_child[12];
static bool menu_drag_enabled;
static bool scrolling_menu_scene(cui_context *ui, cui_menu_tree_state *state, cui_id *action,
                                 bool disabled) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool result = cui_menu_tree(
        ui, 3000, &theme, cui_string("Scrollable"), scrolling_menu, 12, action, state,
        (cui_menu_tree_options){
            .drag_scroll = menu_drag_enabled,
            .popup = {
                .viewport = {0, 0, 640, 200}, .width = 180, .dt = .016f, .disabled = disabled}});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void menu_wheel(cui_context *ui, cui_rect panel, float dy) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = panel.x + 5, .y = panel.y + 15, .dy = dy}));
}
static void test_hierarchical_menu_scroll(void) {
    for (size_t i = 0; i < 12; i++) {
        scrolling_menu[i] = (cui_menu_entry){.id = 3100 + i, .label = {"Root", 4}};
        scrolling_child[i] = (cui_menu_entry){.id = 3200 + i, .label = {"Child", 5}};
    }
    scrolling_menu[0].kind = CUI_MENU_SUBMENU;
    scrolling_menu[0].children = scrolling_child;
    scrolling_menu[0].child_count = 12;
    scrolling_menu[2].disabled = true;
    scrolling_child[2].kind = CUI_MENU_SEPARATOR;
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 3300);
    cui_id action = 0;
    scrolling_menu_scene(ui, &state, &action, false);
    click(ui, 3000);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.depth == 1 && state.visible_rows[0] == 4);
    key(ui, CUI_KEY_RIGHT);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.depth == 2);
    menu_wheel(ui, state.panels[1], -.5f);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.levels[1].first_visible == 0);
    menu_wheel(ui, state.panels[1], -1.5f);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.levels[1].first_visible == 2 && state.levels[0].first_visible == 0);
    assert(state.levels[1].active == 3203 && cui_focused(ui) == 3203);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    bool range = false;
    for (size_t i = 0; i < count; i++)
        if (semantics[i].id == (3100 ^ UINT64_C(0x4b64d23e91a738c1)))
            range = semantics[i].details.scrollable && semantics[i].details.scroll_y == 84 &&
                    semantics[i].details.scroll_max_y == 336;
    assert(range);
    cui_rect parent = cui_bounds(ui, 3100);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = parent.x + 3, .y = parent.y + 3}));
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.depth == 2 && state.levels[1].first_visible == 2 &&
           state.levels[1].active == 3203);
    menu_wheel(ui, state.panels[0], -2);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.depth == 1 && state.levels[0].first_visible == 2);
    assert(state.levels[0].active == 3103 && cui_focused(ui) == 3103);
    key(ui, CUI_KEY_END);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.levels[0].first_visible == 8 && cui_focused(ui) == 3111);
    key(ui, CUI_KEY_HOME);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.levels[0].first_visible == 0 && cui_focused(ui) == 3100);
    menu_wheel(ui, state.panels[0], -1000);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.levels[0].first_visible == 8);
    menu_wheel(ui, state.panels[0], 1000);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.levels[0].first_visible == 0);
    cui_rect row = cui_bounds(ui, 3101);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = row.x + 3, .y = row.y + 3}));
    menu_wheel(ui, state.panels[0], -3);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = row.x + 3, .y = row.y + 3}));
    assert(!scrolling_menu_scene(ui, &state, &action, false) && action == 0);
    assert(state.overlay.open && state.levels[0].first_visible == 3);
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 600, .y = 450, .dy = -2}));
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.levels[0].first_visible == 3);
    menu_wheel(ui, state.panels[0], -2);
    scrolling_menu_scene(ui, &state, &action, true);
    assert(!state.overlay.open && !state.depth);
    cui_destroy(ui);
}
static void test_menu_pointer_drag_scroll(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 3300);
    cui_id action = 0;
    menu_drag_enabled = true;
    scrolling_menu_scene(ui, &state, &action, false);
    click(ui, 3000);
    scrolling_menu_scene(ui, &state, &action, false);
    cui_rect row = cui_bounds(ui, 3103);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = row.x + 3, .y = row.y + 3}));
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.scroll_pending && !state.scroll_dragging);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = row.x + 3, .y = row.y + 3 - 84}));
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.scroll_dragging && state.levels[0].first_visible == 2);
    cui_rect moved = cui_bounds(ui, 3103);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = moved.x + 3, .y = moved.y + 3}));
    assert(!scrolling_menu_scene(ui, &state, &action, false));
    assert(!action && !state.scroll_dragging && !state.scroll_pending && state.overlay.open);
    menu_wheel(ui, state.panels[0], 100);
    scrolling_menu_scene(ui, &state, &action, false);
    row = cui_bounds(ui, 3101);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = row.x + 3, .y = row.y + 3}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = row.x + 3, .y = row.y - 5}));
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.scroll_dragging && state.levels[0].first_visible == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    scrolling_menu_scene(ui, &state, &action, false);
    assert(!state.scroll_pending && !state.scroll_dragging && !action);
    click(ui, 3101);
    assert(scrolling_menu_scene(ui, &state, &action, false));
    assert(action == 3101 && !state.overlay.open);
    action = 0;
    click(ui, 3000);
    scrolling_menu_scene(ui, &state, &action, false);
    key(ui, CUI_KEY_RIGHT);
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.depth == 2);
    row = cui_bounds(ui, 3203);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = row.x + 3, .y = row.y + 3}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = row.x + 3, .y = row.y + 3 - 84}));
    scrolling_menu_scene(ui, &state, &action, false);
    assert(state.scroll_dragging && state.scroll_level == 1 && state.levels[1].first_visible == 2 &&
           state.levels[0].first_visible == 0);
    scrolling_menu_scene(ui, &state, &action, true);
    assert(!state.scroll_dragging && !state.scroll_pending && !state.overlay.open);
    menu_drag_enabled = false;
    cui_destroy(ui);
}
static void test_dropdown_configurable_palettes(void) {
    const cui_dropdown_button_variant variants[] = {
        CUI_DROPDOWN_BUTTON_DEFAULT, CUI_DROPDOWN_BUTTON_PRIMARY, CUI_DROPDOWN_BUTTON_SECONDARY,
        CUI_DROPDOWN_BUTTON_DANGER,  CUI_DROPDOWN_BUTTON_WARNING, CUI_DROPDOWN_BUTTON_SUCCESS,
        CUI_DROPDOWN_BUTTON_INFO};
    for (size_t kind = 0; kind < CUI_THEME_BUTTON_COUNT; kind++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_theme theme = cui_theme_light();
        cui_theme_button_colors colors = {
            {11, 12, 13, 255}, {21, 22, 23, 255},   {31, 32, 33, 255}, {111, 112, 113, 255},
            {41, 42, 43, 111}, {51, 52, 53, 122},   {61, 62, 63, 133}, {71, 72, 73, 255},
            {81, 82, 83, 255}, {121, 122, 123, 255}};
        theme.buttons[kind] = colors;
        cui_menu_tree_state state;
        cui_menu_tree_init(&state, 2200);
        cui_id action = 0;
        cui_dropdown_button_options options = {
            .variant = variants[kind],
            .menu = {.popup = {.viewport = {0, 0, 640, 480}, .width = 220}}};
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
        dropdown_scene(ui, &state, &action, &theme, options);
        assert(same_color(dropdown_surface(ui, 2001), colors.normal));
        cui_rect bounds = cui_bounds(ui, 2001);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 2, .y = bounds.y + 2}));
        dropdown_scene(ui, &state, &action, &theme, options);
        assert(same_color(dropdown_surface(ui, 2001), colors.hover));
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 2, .y = bounds.y + 2}));
        dropdown_scene(ui, &state, &action, &theme, options);
        assert(same_color(dropdown_surface(ui, 2001), colors.active));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 600, .y = 450}));
        options.outline = true;
        dropdown_scene(ui, &state, &action, &theme, options);
        assert(same_color(dropdown_surface(ui, 2001), colors.outline_normal));
        bool label = false;
        for (size_t i = 0; i < dropdown_draw.count; i++)
            label |= dropdown_draw.commands[i].type == CUI_DRAW_TEXT &&
                     dropdown_draw.commands[i].text.length == 4 &&
                     same_color(dropdown_draw.commands[i].color, colors.outline_foreground);
        assert(label);
        options.selected = true;
        dropdown_scene(ui, &state, &action, &theme, options);
        assert(same_color(dropdown_surface(ui, 2001), colors.outline_active));
        options.disabled = true;
        dropdown_scene(ui, &state, &action, &theme, options);
        colors.outline_normal.a /= 2;
        assert(same_color(dropdown_surface(ui, 2001), colors.outline_normal));
        cui_destroy(ui);
    }
}
static void capped_custom_tab(cui_context *ui, const cui_choice_item *item, size_t index,
                              void *user) {
    (void)item;
    (void)user;
    cui_begin_box(ui, 9000 + index,
                  (cui_box){.width = index ? 300 : 20, .height = 20, .background = {5, 6, 7, 255}});
    cui_end_box(ui);
}
static void test_custom_tab_width_caps(void) {
    const cui_choice_item entries[] = {{10, {"Short", 5}, false}, {20, {"Long", 4}, false}};
    cui_theme theme = cui_theme_light();
    for (int variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_id selected = 10;
        cui_tab_scroll_state scroll = {0};
        cui_tabs_options options = {.variant = (cui_tab_variant)variant,
                                    .render = capped_custom_tab,
                                    .max_width = 100,
                                    .scroll = &scroll,
                                    .bar_extent = 110};
        for (int frame = 0; frame < 2; frame++) {
            assert(cui_begin_frame(ui, 640, 480, .016f));
            cui_tabs(ui, 1, &theme, entries, 2, &selected, options, NULL, NULL);
            cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            assert(cui_bounds(ui, 10).w < 100 && cui_bounds(ui, 20).w == 100);
        }
        assert(scroll.maximum ==
               cui_bounds(ui, 10).w + 100 +
                   cui_tab_size_metrics((cui_tab_variant)variant, CUI_TAB_MEDIUM, 1).gap - 110);
        click(ui, 20);
        assert(cui_begin_frame(ui, 640, 480, .016f));
        assert(cui_tabs(ui, 1, &theme, entries, 2, &selected, options, NULL, NULL));
        cui_end_frame(ui);
        assert(selected == 20);
        cui_destroy(ui);
    }
}
static bool tab_slot_action, tab_slot_disabled;
static void tab_part_prefix(cui_context *ui, const cui_choice_item *item, size_t index,
                            void *user) {
    (void)user;
    tab_slot_disabled = item->disabled;
    cui_begin_box(ui, 9100 + index,
                  (cui_box){.width = 12, .height = 12, .background = {7, 8, 9, 255}});
    cui_end_box(ui);
}
static void tab_part_suffix(cui_context *ui, const cui_choice_item *item, size_t index,
                            void *user) {
    (void)user;
    cui_theme theme = cui_theme_light();
    theme.control_height = 14;
    theme.padding = 2;
    theme.font_size = 10;
    tab_slot_action |= cui_button(ui, 9200 + index, &theme, cui_string("x"), item->disabled);
}
static void tab_part_icon(cui_context *ui, const cui_choice_item *item, size_t index, void *user) {
    (void)item;
    (void)user;
    cui_begin_box(ui, 9300 + index,
                  (cui_box){.width = 20, .height = 20, .background = {7, 8, 9, 255}});
    cui_end_box(ui);
}
static void test_structured_tab_parts(void) {
    const cui_choice_item entries[] = {{10, {"An exceptionally long tab label", 31}, false},
                                       {20, {"A", 1}, false}};
    cui_tab_parts parts[] = {{.prefix = tab_part_prefix,
                              .suffix = tab_part_suffix,
                              .prefix_width = 12,
                              .suffix_width = 16},
                             {0}};
    cui_theme theme = cui_theme_light();
    for (int variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_id selected = 10;
        cui_tabs_options options = {.variant = (cui_tab_variant)variant,
                                    .max_width = 140,
                                    .parts = parts,
                                    .parts_count = 1};
        tab_slot_action = false;
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_tabs(ui, 1, &theme, entries, 2, &selected, options, NULL, NULL);
        cui_draw_list draw = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        cui_rect tab = cui_bounds(ui, 10), prefix = cui_bounds(ui, 9100),
                 suffix = cui_bounds(ui, 9200);
        assert(tab.w == 140 && prefix.w == 12 && suffix.w <= 16);
        assert(suffix.x > tab.x + tab.w / 2 && suffix.x + suffix.w <= tab.x + tab.w);
        assert(cui_bounds(ui, 20).w < 140);
        bool ellipsis = false;
        for (size_t i = 0; i < draw.count; i++)
            ellipsis |= draw.commands[i].type == CUI_DRAW_TEXT &&
                        draw.commands[i].text.length == 3 &&
                        memcmp(draw.commands[i].text.data, "\xe2\x80\xa6", 3) == 0;
        assert(ellipsis);
        click(ui, 9200);
        assert(cui_begin_frame(ui, 640, 480, .016f));
        assert(!cui_tabs(ui, 1, &theme, entries, 2, &selected, options, NULL, NULL));
        cui_end_frame(ui);
        assert(tab_slot_action && selected == 10);
        options.disabled = true;
        tab_slot_action = false;
        click(ui, 9200);
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_tabs(ui, 1, &theme, entries, 2, &selected, options, NULL, NULL);
        cui_end_frame(ui);
        assert(tab_slot_disabled && !tab_slot_action);
        options.disabled = false;
        options.render = capped_custom_tab;
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_tabs(ui, 1, &theme, entries, 2, &selected, options, NULL, NULL);
        cui_end_frame(ui);
        assert(cui_bounds(ui, 10).w < 140 && cui_bounds(ui, 20).w == 140);
        options.render = NULL;
        parts[0].icon = tab_part_icon;
        options.max_width = 20;
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_tabs(ui, 1, &theme, entries, 1, &selected, options, NULL, NULL);
        cui_end_frame(ui);
        assert(cui_bounds(ui, 10).w > 20 && cui_bounds(ui, 9300).w == 20);
        parts[0].icon = NULL;
        cui_destroy(ui);
    }
}
static unsigned list_confirms, list_secondary_confirms, list_cancels, list_legacy_confirms;
static void list_confirm(cui_id item, bool secondary, void *user) {
    assert(user == &list_confirms && item == 10);
    list_confirms++;
    list_secondary_confirms += secondary;
    assert(list_legacy_confirms == list_confirms);
}
static void list_cancel(void *user) {
    assert(user == &list_confirms);
    list_cancels++;
}
static void list_legacy_confirm(cui_id item, void *user) {
    assert(item == 10 && user == &list_confirms);
    list_legacy_confirms++;
}
static void test_list_confirmation_contract(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    const cui_choice_item entries[] = {{10, {"Enabled", 7}, false}, {20, {"Disabled", 8}, true}};
    cui_list_state state = {.active = 10};
    cui_id selected = 0;
    cui_list_options options = {.on_activate = list_legacy_confirm,
                                .on_confirm = list_confirm,
                                .on_cancel = list_cancel,
                                .activation_user = &list_confirms,
                                .track_secondary = true};
    list_confirms = list_secondary_confirms = list_cancels = list_legacy_confirms = 0;
    list_scene(ui, entries, 2, &selected, &state, options);
    cui_rect row = cui_bounds(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_SECONDARY,
                                           .x = row.x + 2,
                                           .y = row.y + 2}));
    assert(!list_scene(ui, entries, 2, &selected, &state, options));
    assert(state.secondary_active == 10 && !selected && !list_confirms);
    cui_focus(ui, 10);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .modifiers = CUI_MOD_CTRL}));
    assert(list_scene(ui, entries, 2, &selected, &state, options));
    assert(selected == 10 && list_confirms == 1 && list_secondary_confirms == 1 &&
           !state.secondary_active);
    key(ui, CUI_KEY_ENTER);
    assert(list_scene(ui, entries, 2, &selected, &state, options));
    assert(list_confirms == 2 && list_secondary_confirms == 1);
    options.secondary_modifiers = CUI_MOD_ALT;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = row.x + 2, .y = row.y + 2}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = row.x + 2,
                                           .y = row.y + 2,
                                           .modifiers = CUI_MOD_ALT}));
    assert(list_scene(ui, entries, 2, &selected, &state, options));
    assert(list_confirms == 3 && list_secondary_confirms == 2);
    key(ui, CUI_KEY_ESCAPE);
    assert(!list_scene(ui, entries, 2, &selected, &state, options));
    assert(list_cancels == 1 && selected == 10);
    options.disabled = true;
    key(ui, CUI_KEY_ENTER);
    key(ui, CUI_KEY_ESCAPE);
    assert(!list_scene(ui, entries, 2, &selected, &state, options));
    assert(list_confirms == 3 && list_cancels == 1);
    options.disabled = false;
    list_scene(ui, NULL, 0, &selected, &state, options);
    cui_focus(ui, 91);
    key(ui, CUI_KEY_ESCAPE);
    assert(!list_scene(ui, NULL, 0, &selected, &state, options));
    assert(list_cancels == 2);
    cui_destroy(ui);
}
static void sized_tab_icon(cui_context *ui, const cui_choice_item *item, size_t index, void *user) {
    (void)item;
    (void)index;
    const cui_tab_metrics *metrics = user;
    cui_begin_box(ui, 9400,
                  (cui_box){.width = metrics->icon_size,
                            .height = metrics->icon_size,
                            .background = {1, 2, 3, 255}});
    cui_end_box(ui);
}
static void test_tab_reference_size_metrics(void) {
    const cui_choice_item entry = {10, {"Accessible icon", 15}, false};
    const float outer[] = {32, 24, 36, 20}, underline_outer[] = {36, 30, 44, 26};
    const float tab_inner[] = {30, 22, 36, 18}, rounded_inner[] = {26, 22, 36, 18};
    const float segmented_inner[] = {24, 18, 28, 16}, underline_inner[] = {26, 22, 32, 20};
    const float icons[] = {16, 14, 16, 10};
    for (int variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++)
        for (int size = CUI_TAB_MEDIUM; size <= CUI_TAB_XSMALL; size++) {
            cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
            assert(ui);
            cui_theme theme = cui_theme_light();
            cui_tab_metrics metrics =
                cui_tab_size_metrics((cui_tab_variant)variant, (cui_tab_size)size, 1);
            float inner = variant == CUI_TAB_UNDERLINE   ? underline_inner[size]
                          : variant == CUI_TAB_SEGMENTED ? segmented_inner[size]
                          : variant == CUI_TAB_OUTLINE || variant == CUI_TAB_PILL
                              ? rounded_inner[size]
                              : tab_inner[size];
            assert(metrics.height ==
                   (variant == CUI_TAB_UNDERLINE ? underline_outer[size] : outer[size]));
            assert(metrics.inner_height == inner && metrics.icon_size == icons[size]);
            cui_tab_parts parts = {.icon = sized_tab_icon, .user = &metrics};
            cui_tab_scroll_state scroll = {0};
            cui_tabs_options options = {.variant = (cui_tab_variant)variant,
                                        .size = (cui_tab_size)size,
                                        .max_width = 5,
                                        .parts = &parts,
                                        .parts_count = 1,
                                        .scroll = &scroll,
                                        .bar_extent = 10};
            cui_id selected = 10;
            assert(cui_begin_frame(ui, 640, 480, .016f));
            cui_tabs(ui, 1, &theme, &entry, 1, &selected, options, NULL, NULL);
            cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            assert(cui_bounds(ui, 10).h == metrics.height);
            assert(cui_bounds(ui, 9400).w == metrics.icon_size);
            assert(cui_bounds(ui, 10 ^ UINT64_C(0xacf7823ab98e17f0)).w == inner * 1.25f);
            assert(scroll.maximum == inner * 1.25f + 2 * metrics.padding_x - 10);
            cui_rect tab = cui_bounds(ui, 10);
            cui_rect row = cui_bounds(ui, 10 ^ UINT64_C(0xacf7823ab98e17d0));
            if (variant == CUI_TAB_UNDERLINE) {
                float expected =
                    tab.y +
                    (metrics.height - inner - metrics.margin_top - metrics.margin_bottom) * .5f +
                    metrics.margin_top;
                assert(fabsf(row.y - expected) < .01f && row.h == inner);
            }
            cui_rect wrapper = cui_bounds(ui, 1 ^ UINT64_C(0xa7eb5ea7941c2230));
            cui_rect viewport = cui_bounds(ui, 1 ^ UINT64_C(0x739b1b53a6cef421));
            assert(fabsf(viewport.x - wrapper.x - metrics.bar_padding_x) < .01f);
            assert(fabsf(wrapper.w - 10 - 2 * metrics.bar_padding_x) < .01f);
            options.density = 2;
            options.height = 80;
            metrics = cui_tab_size_metrics((cui_tab_variant)variant, (cui_tab_size)size, 2);
            assert(cui_begin_frame(ui, 640, 480, .016f));
            cui_tabs(ui, 1, &theme, &entry, 1, &selected, options, NULL, NULL);
            cui_end_frame(ui);
            assert(cui_bounds(ui, 10).h == 80 && cui_bounds(ui, 9400).w == metrics.icon_size);
            cui_destroy(ui);
        }
}
static void test_tab_independent_palette(void) {
    for (cui_tab_variant variant = CUI_TAB_DEFAULT; variant <= CUI_TAB_SEGMENTED; variant++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_theme theme = cui_theme_light();
        theme.tab_foreground = (cui_color){11, 22, 33, 255};
        theme.tab_active_foreground = (cui_color){44, 55, 66, 255};
        theme.tab_active = (cui_color){77, 88, 99, 255};
        theme.tab_bar = (cui_color){100, 110, 120, 255};
        theme.tab_bar_segmented = (cui_color){130, 140, 150, 255};
        theme.secondary_hover = (cui_color){160, 170, 180, 255};
        const cui_choice_item entry = {10, {"Palette", 7}, false};
        cui_id selected = 0;
        for (int phase = 0; phase < 4; phase++) {
            if (phase == 1) {
                cui_rect bounds = cui_bounds(ui, 10);
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                                       .x = bounds.x + 2,
                                                       .y = bounds.y + 2}));
            }
            if (phase >= 2)
                selected = 10;
            assert(cui_begin_frame(ui, 640, 480, .016f));
            cui_tabs(ui, 1, &theme, &entry, 1, &selected,
                     (cui_tabs_options){.variant = variant, .disabled = phase == 3}, NULL, NULL);
            cui_draw_list draw = cui_end_frame(ui);
            cui_color expected =
                phase == 3                ? theme.muted
                : variant == CUI_TAB_PILL ? (phase == 2   ? theme.on_primary
                                             : phase == 1 ? theme.secondary_foreground
                                                          : theme.text)
                : variant == CUI_TAB_OUTLINE && phase == 2 ? theme.primary
                : variant == CUI_TAB_OUTLINE && phase == 1 ? theme.secondary_foreground
                : phase == 0                               ? theme.tab_foreground
                                                           : theme.tab_active_foreground;
            if (variant == CUI_TAB_PILL && phase == 3) {
                expected = theme.on_primary;
                expected.a *= .5f;
            }
            bool label = false, bar = false;
            for (size_t i = 0; i < draw.count; i++) {
                cui_draw_command command = draw.commands[i];
                if (command.type == CUI_DRAW_TEXT && command.text.length == 7)
                    label |= same_color(command.color, expected);
                if (command.type == CUI_DRAW_RECT)
                    bar |= same_color(command.color, variant == CUI_TAB_DEFAULT
                                                         ? theme.tab_bar
                                                         : theme.tab_bar_segmented);
            }
            assert(label);
            if (variant == CUI_TAB_DEFAULT || variant == CUI_TAB_SEGMENTED)
                assert(bar);
        }
        cui_destroy(ui);
    }
}
static void test_tab_retained_spring(void) {
    cui_spring_settings settings = cui_theme_light().spring_move;
    for (int regime = 0; regime < 3; regime++) {
        settings.damping = regime == 0 ? .85f : regime == 1 ? 1 : 1.5f;
        settings.epsilon = .001f;
        cui_tab_motion motion = {0};
        cui_tab_motion_step(&motion, 10, 40, 0, settings, false);
        assert(motion.initialized && motion.left == 10 && motion.width == 40);
        cui_tab_motion whole = motion;
        cui_tab_motion_step(&whole, 200, 80, .04f, settings, false);
        cui_tab_motion_step(&motion, 200, 80, .02f, settings, false);
        cui_tab_motion_step(&motion, 200, 80, .02f, settings, false);
        assert(fabsf(motion.left - whole.left) < .001f &&
               fabsf(motion.width - whole.width) < .001f &&
               fabsf(motion.left_velocity - whole.left_velocity) < .01f);
        assert(motion.left > 10 && motion.left < 200 && motion.left_velocity > 0);
        float left = motion.left, velocity = motion.left_velocity;
        cui_tab_motion_step(&motion, -20, 25, 0, settings, false);
        assert(motion.left == left && motion.left_velocity == velocity);
        cui_tab_motion_step(&motion, -20, 25, .001f, settings, false);
        assert(motion.left > left); /* in-flight velocity survives reverse retarget */
        for (int i = 0; i < 1000; i++)
            cui_tab_motion_step(&motion, -20, 25, .016f, settings, false);
        assert(motion.left == -20 && motion.width == 25 && motion.left_velocity == 0 &&
               motion.width_velocity == 0);
        cui_tab_motion_step(&motion, 300, 90, .016f, settings, true);
        assert(motion.left == 300 && motion.width == 90 && motion.left_velocity == 0);
        settings.travel = false;
        cui_tab_motion_step(&motion, 100, 10, .016f, settings, false);
        assert(motion.left == 100 && motion.width == 10);
        settings.travel = true;
        settings.response = 0;
        cui_tab_motion_step(&motion, 0, 5, .016f, settings, false);
        assert(motion.left == 0 && motion.width == 5);
        settings = cui_theme_light().spring_move;
        cui_tab_motion_step(&motion, NAN, 20, .016f, settings, false);
        cui_tab_motion_step(&motion, 20, -1, .016f, settings, false);
        assert(motion.left == 0 && motion.width == 5);
    }
}
static cui_draw_list animated_tabs_scene(cui_context *ui, cui_id *selected, const cui_theme *theme,
                                         cui_tab_animation *animation, cui_tab_variant variant,
                                         float dt, bool reduced, bool disabled,
                                         cui_tab_scroll_state *scroll) {
    const cui_choice_item entries[] = {{10, {"One", 3}, false}, {20, {"Long second", 11}, false}};
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_tabs(ui, 1, theme, entries, 2, selected,
             (cui_tabs_options){.variant = variant,
                                .animation = animation,
                                .dt = dt,
                                .reduced_motion = reduced,
                                .disabled = disabled,
                                .scroll = scroll,
                                .bar_extent = scroll ? 70 : 0},
             NULL, NULL);
    return cui_end_frame(ui);
}
static cui_color tab_label_color(cui_draw_list draw, size_t length) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == length)
            return draw.commands[i].color;
    assert(false);
    return (cui_color){0};
}
static void test_tab_shared_indicator(void) {
    const cui_tab_variant variants[] = {CUI_TAB_PILL, CUI_TAB_UNDERLINE, CUI_TAB_SEGMENTED};
    for (size_t v = 0; v < 3; v++) {
        cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
        assert(ui);
        cui_theme theme = cui_theme_light();
        theme.text = (cui_color){0, 0, 0, 255};
        theme.on_primary = (cui_color){255, 255, 255, 255};
        cui_tab_animation animation = {0};
        cui_id selected = 10;
        animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], 0, false, false, NULL);
        assert(!animation.ready && animation.target_bounds.w > 0 && animation.strip_bounds.w > 0);
        cui_draw_list draw = animated_tabs_scene(ui, &selected, &theme, &animation, variants[v],
                                                 .016f, false, false, NULL);
        assert(animation.ready && animation.spring.initialized && !animation.transitioning);
        cui_color surface = variants[v] == CUI_TAB_SEGMENTED ? theme.background : theme.primary;
        size_t indicators = 0, indicator_index = SIZE_MAX, label_index = SIZE_MAX;
        for (size_t i = 0; i < draw.count; i++) {
            if (draw.commands[i].type == CUI_DRAW_RECT &&
                same_color(draw.commands[i].color, surface)) {
                indicators++;
                indicator_index = i;
            }
            if (draw.commands[i].type == CUI_DRAW_TEXT && label_index == SIZE_MAX)
                label_index = i;
        }
        assert(indicators == 1 && indicator_index < label_index);
        float initial_left = animation.spring.left, initial_width = animation.spring.width;
        click(ui, 20);
        draw = animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], 0, false, false,
                                   NULL);
        assert(selected == 20 && animation.transitioning && animation.spring.left == initial_left &&
               animation.spring.width == initial_width);
        if (variants[v] == CUI_TAB_PILL)
            assert(same_color(tab_label_color(draw, 11), theme.text));
        draw = animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], .1f, false,
                                   false, NULL);
        assert(animation.spring.left > initial_left && animation.spring.left_velocity > 0);
        if (variants[v] == CUI_TAB_PILL)
            assert(same_color(tab_label_color(draw, 11), (cui_color){128, 128, 128, 255}));
        float moving_left = animation.spring.left, moving_velocity = animation.spring.left_velocity;
        click(ui, 10);
        animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], 0, false, false, NULL);
        assert(selected == 10 && animation.spring.left == moving_left &&
               animation.spring.left_velocity == moving_velocity);
        draw = animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], .016f, true,
                                   false, NULL);
        assert(animation.spring.left == 0 && animation.spring.left_velocity == 0);
        if (variants[v] == CUI_TAB_PILL)
            assert(same_color(tab_label_color(draw, 3), theme.on_primary));
        animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], .016f, false, true,
                            NULL);
        assert(!animation.ready);
        cui_tab_scroll_state scroll = {0};
        selected = 20;
        animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], .016f, true, false,
                            &scroll);
        animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], .016f, true, false,
                            &scroll);
        float unscrolled_left = animation.spring.left;
        scroll.offset = 20;
        animated_tabs_scene(ui, &selected, &theme, &animation, variants[v], .016f, true, false,
                            &scroll);
        assert(fabsf(animation.spring.left - unscrolled_left + 20) < .01f);
        cui_destroy(ui);
    }
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    theme.text = (cui_color){255, 0, 0, 255};
    theme.on_primary = (cui_color){0, 0, 255, 255};
    cui_tab_animation animation = {0};
    cui_id selected = 10;
    animated_tabs_scene(ui, &selected, &theme, &animation, CUI_TAB_PILL, 0, false, false, NULL);
    animated_tabs_scene(ui, &selected, &theme, &animation, CUI_TAB_PILL, .016f, false, false, NULL);
    click(ui, 20);
    animated_tabs_scene(ui, &selected, &theme, &animation, CUI_TAB_PILL, 0, false, false, NULL);
    cui_draw_list draw = animated_tabs_scene(ui, &selected, &theme, &animation, CUI_TAB_PILL, .1f,
                                             false, false, NULL);
    /* Source interpolates hue channels: red -> blue crosses green at halfway. */
    assert(same_color(tab_label_color(draw, 11), (cui_color){0, 255, 0, 255}));
    draw = animated_tabs_scene(ui, &selected, &theme, &animation, CUI_TAB_PILL, .1f, false, false,
                               NULL);
    assert(same_color(tab_label_color(draw, 11), theme.on_primary) && !animation.transitioning);
    cui_destroy(ui);
}
static unsigned tab_tail_calls;
static bool tab_tail_activated;
static void custom_tab_tail(cui_context *ui, void *user) {
    assert(user == &tab_tail_calls);
    tab_tail_calls++;
    cui_theme theme = cui_theme_light();
    theme.control_height = 20;
    theme.font_size = 10;
    theme.padding = 2;
    tab_tail_activated |= cui_button(ui, 9501, &theme, cui_string("Tail"), false);
}
static void tab_tail_scene(cui_context *ui, cui_tab_scroll_state *scroll,
                           cui_tabs_options options) {
    cui_theme theme = cui_theme_light();
    const cui_choice_item entry = {10, {"One", 3}, false};
    cui_id selected = 10;
    options.scroll = scroll;
    options.bar_extent = 70;
    options.variant = CUI_TAB_PILL;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_tabs(ui, 1, &theme, &entry, 1, &selected, options, NULL, NULL);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_tab_scroll_tail(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_tab_scroll_state scroll = {0};
    cui_tabs_options options = {.last_empty_space = custom_tab_tail,
                                .last_empty_space_user = &tab_tail_calls};
    tab_tail_calls = 0;
    tab_tail_activated = false;
    tab_tail_scene(ui, &scroll, options);
    assert(tab_tail_calls == 0 && scroll.maximum == 0);
    options.suffix = tab_scroll_suffix;
    options.last_empty_space = NULL;
    tab_tail_scene(ui, &scroll, options);
    cui_rect first = cui_bounds(ui, 10);
    cui_rect tail = cui_bounds(ui, 1 ^ UINT64_C(0x4c2c9f0875e4b4c0));
    assert(tail.w == 12 && fabsf(tail.x - first.x - first.w - 4) < .01f);
    assert(scroll.maximum == fmaxf(0, first.w + 4 + 12 - 70));
    options.last_empty_space = custom_tab_tail;
    options.last_empty_space_width = 32;
    tab_tail_scene(ui, &scroll, options);
    assert(tab_tail_calls == 1 && cui_bounds(ui, 1 ^ UINT64_C(0x4c2c9f0875e4b4c0)).w == 32);
    assert(fabsf(scroll.maximum - first.w - 4 - 32 + 70) < .01f);
    cui_rect viewport = cui_bounds(ui, 1 ^ UINT64_C(0x739b1b53a6cef421));
    assert(cui_queue_event(
        ui, (cui_event){
                .type = CUI_EVENT_SCROLL, .x = viewport.x + 2, .y = viewport.y + 2, .dx = -1000}));
    tab_tail_scene(ui, &scroll, options);
    assert(scroll.offset == scroll.maximum);
    click(ui, 9501);
    tab_tail_scene(ui, &scroll, options);
    assert(tab_tail_activated);
    options.last_empty_space_width = 0;
    options.custom_last_empty_space_width = true;
    tab_tail_scene(ui, &scroll, options);
    assert(cui_bounds(ui, 1 ^ UINT64_C(0x4c2c9f0875e4b4c0)).w == 0 && scroll.maximum == 0);
    tab_tail_activated = false;
    click(ui, 9501);
    tab_tail_scene(ui, &scroll, options);
    assert(!tab_tail_activated);
    options = (cui_tabs_options){.density = 2, .suffix = tab_scroll_suffix};
    tab_tail_scene(ui, &scroll, options);
    assert(cui_bounds(ui, 1 ^ UINT64_C(0x4c2c9f0875e4b4c0)).w == 24);
    cui_tab_overflow overflow;
    cui_menu_entry scratch[1];
    cui_tab_overflow_init(&overflow, 9600, 9601, scratch, 1);
    options = (cui_tabs_options){.overflow = &overflow};
    tab_tail_scene(ui, &scroll, options);
    assert(!overflow.visible && cui_bounds(ui, 1 ^ UINT64_C(0x4c2c9f0875e4b4c0)).w == 12);
    cui_destroy(ui);
}
static void test_first_tab_border_policy(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme theme = cui_theme_light();
    const cui_choice_item entries[] = {{10, {"First", 5}, false}, {20, {"Second", 6}, false}};
    cui_tab_parts parts[] = {{0}, {.omit_leading_border = true}};
    cui_id selected = 10;
    for (int phase = 0; phase < 4; phase++) {
        parts[0].omit_leading_border = phase >= 1;
        if (phase == 3)
            selected = 20;
        assert(cui_begin_frame(ui, 640, 480, .016f));
        cui_tabs(ui, 1, &theme, entries, 2, &selected,
                 (cui_tabs_options){.parts = parts, .parts_count = 2, .disabled = phase == 2}, NULL,
                 NULL);
        cui_draw_list draw = cui_end_frame(ui);
        cui_rect bounds = cui_bounds(ui, selected);
        bool border = false;
        for (size_t i = 0; i < draw.count; i++) {
            cui_draw_command command = draw.commands[i];
            if (command.type == CUI_DRAW_BORDER && command.bounds.x == bounds.x &&
                command.bounds.y == bounds.y && command.bounds.w == bounds.w &&
                command.bounds.h == bounds.h) {
                assert(command.custom_border_edges && command.border_edges.right == 1 &&
                       command.border_edges.top == 0 && command.border_edges.bottom == 0);
                assert(command.border_edges.left == (phase == 1 || phase == 2 ? 0 : 1));
                border = true;
            }
        }
        assert(border);
    }
    cui_destroy(ui);
}
int main(void) {
    test_first_tab_border_policy();
    test_tab_scroll_tail();
    test_tab_shared_indicator();
    test_tab_retained_spring();
    test_tab_independent_palette();
    test_tab_reference_size_metrics();
    test_list_confirmation_contract();
    test_structured_tab_parts();
    test_custom_tab_width_caps();
    test_dropdown_configurable_palettes();
    test_hierarchical_menu_scroll();
    test_menu_pointer_drag_scroll();
    test_dropdown_presentation_variants();
    test_lists_and_popups();
    test_combobox();
    test_combobox_text_enter_order();
    test_multi_select();
    test_multi_combobox();
    test_menu_tree();
    test_menu_depth_bound();
    test_context_menu();
    test_context_modal_guard();
    test_custom_choice_rows();
    test_choice_scrolling();
    test_list_scroll_modal_guard();
    test_scroll_activation_order();
    test_custom_tabs();
    test_nested_list_wheel_routing();
    test_combo_editor_paint_and_hit();
    test_closable_tab_composition();
    test_menu_configuration();
    test_dropdown_button();
    test_dropdown_inheritance_and_ghost_pair();
    test_dropdown_optional_halves();
    test_dropdown_anchor_corners();
    test_dropdown_action_tooltip();
    test_tab_max_width();
    test_tab_scrolling();
    test_tab_overflow_menu();
    test_tab_drag_reorder();
    test_long_tab_label_measurement();
    test_dropdown_colored_outline();
    test_dropdown_inner_button_presentation();
    test_tab_reveal_requests();
    test_tab_scrollbars();
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
