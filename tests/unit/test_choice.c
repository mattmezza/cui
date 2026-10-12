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
int main(void) {
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
    assert(count == 4 && s[0].role == CUI_ROLE_TAB && !s[0].checked && s[2].checked);
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
    cui_destroy(ui);
    return 0;
}
