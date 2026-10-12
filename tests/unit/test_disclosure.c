/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_disclosure.h"
#include <assert.h>
static cui_disclosure_item items[] = {
    {10, {"One", 3}, false}, {20, {"Two", 3}, true}, {30, {"Three", 5}, false}};
static bool opened[3];
static unsigned panels;
static void panel(cui_context *ui, size_t index, void *user) {
    (void)user;
    panels |= 1u << index;
    cui_button(ui, 100 + index, &(cui_theme){.font_size = 14, .control_height = 32},
               cui_string("Inside"), false);
}
static bool scene(cui_context *ui, cui_accordion_options options, const cui_theme *theme) {
    panels = 0;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    bool changed = cui_accordion(ui, 1, theme, items, 3, opened, options, panel, NULL);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count > 0 && cui_last_error(ui) == CUI_OK);
    return changed;
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void click(cui_context *ui, cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    assert(b.w > 0 && b.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 2, .y = b.y + 2}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 2, .y = b.y + 2}));
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_accordion_options options = {.collapsible = true, .bordered = true};
    for (size_t i = 0; i < 4; i++)
        assert(!scene(ui, options, &themes[i]));
    assert(!panels);
    click(ui, 10);
    assert(scene(ui, options, &themes[0]));
    assert(opened[0] && panels == 1);
    size_t count = 0;
    const cui_semantic *s = cui_semantics(ui, &count);
    assert(count == 6 && s[0].checked && s[0].role == CUI_ROLE_BUTTON && s[1].disabled &&
           s[0].details.expanded);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_DOWN);
    assert(!scene(ui, options, &themes[0]));
    assert(cui_focused(ui) == 30);
    key(ui, CUI_KEY_ENTER);
    assert(scene(ui, options, &themes[0]));
    assert(!opened[0] && opened[2] && panels == 4);
    options.collapsible = false;
    key(ui, CUI_KEY_ENTER);
    assert(!scene(ui, options, &themes[0]));
    assert(opened[2]);
    key(ui, CUI_KEY_HOME);
    scene(ui, options, &themes[0]);
    assert(cui_focused(ui) == 10);
    key(ui, CUI_KEY_END);
    scene(ui, options, &themes[0]);
    assert(cui_focused(ui) == 30);
    options.multiple = true;
    options.collapsible = true;
    click(ui, 10);
    assert(scene(ui, options, &themes[0]));
    assert(opened[0] && opened[2] && panels == 5);
    click(ui, 20);
    assert(!scene(ui, options, &themes[0]));
    assert(!opened[1]);
    options.disabled = true;
    click(ui, 10);
    assert(!scene(ui, options, &themes[0]));
    assert(opened[0]);
    options.disabled = false;
    cui_focus(ui, 100);
    opened[0] = false;
    scene(ui, options, &themes[0]);
    assert(cui_focused(ui) == 0); /* hidden content cannot remain focused */
    bool open = false;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_collapsible(ui, 200, &themes[0], cui_string("More"), &open, false, panel, NULL);
    cui_end_frame(ui);
    cui_focus(ui, 200);
    key(ui, CUI_KEY_SPACE);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_collapsible(ui, 200, &themes[0], cui_string("More"), &open, false, panel, NULL));
    cui_end_frame(ui);
    assert(open);
    cui_destroy(ui);
    return 0;
}
