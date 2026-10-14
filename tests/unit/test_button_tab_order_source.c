/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>
static unsigned clicks, events;
static cui_button_click_event copied;
static cui_button_options options;
static void legacy(void *user) {
    assert(user == &options);
    ++clicks;
    /* By-value descriptor protects order and event callback from caller edits. */
    options.tab_index = -999;
    options.skip_tab = true;
    options.on_click_event = NULL;
}
static void clicked(void *user, const cui_button_click_event *event) {
    assert(user == &options && clicks == events + 1);
    copied = *event;
    ++events;
}
static void item(cui_context *ui, const cui_theme *t, cui_id id, int index, bool skip,
                 bool disabled) {
    cui_button_options o = {
        .custom_tab_index = true, .tab_index = index, .skip_tab = skip, .disabled = disabled};
    if (id == 3)
        o = options;
    cui_button_ex(ui, id, t, cui_string("Button"), o);
}
static void frame(cui_context *ui, bool modal, bool removed) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 600, 500, .016f));
    cui_begin_box(ui, 90, (cui_box){.width = 500, .height = 400});
    item(ui, &t, 1, 4, false, false);
    assert(cui_begin_tab_group(ui, -2));
    item(ui, &t, 2, 99, false, false);
    assert(cui_begin_tab_group(ui, -1));
    if (!removed)
        item(ui, &t, 3, -9, false, false);
    item(ui, &t, 4, -9, false, false);
    cui_end_tab_group(ui);
    item(ui, &t, 5, -1, true, false);
    item(ui, &t, 6, -100, false, true);
    item(ui, &t, 9, -200, false, false);
    cui_set_semantic_details(ui, 9, (cui_semantic_details){.no_focus = true});
    cui_end_tab_group(ui);
    if (modal) {
        assert(cui_begin_focus_scope(ui, 100, true));
        item(ui, &t, 7, -3, false, false);
        item(ui, &t, 8, -4, false, false);
        cui_end_focus_scope(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
}
static void tab(cui_context *ui, bool reverse, cui_id expected, bool modal, bool removed) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                           .key = CUI_KEY_TAB,
                                           .modifiers = reverse ? CUI_MOD_SHIFT : 0}));
    frame(ui, modal, removed);
    assert(cui_focused(ui) == expected);
}
int main(void) {
    options = (cui_button_options){.custom_tab_index = true,
                                   .tab_index = -9,
                                   .on_click = legacy,
                                   .on_click_event = clicked,
                                   .action_user = &options};
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    frame(ui, false, false);
    tab(ui, false, 3, false, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame(ui, false, false);
    assert(clicks == 1 && events == 1 && copied.target == 3 &&
           copied.origin == CUI_BUTTON_CLICK_KEYBOARD && copied.key == CUI_KEY_ENTER);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    bool checked = false;
    for (size_t i = 0; i < count; ++i)
        if (sem[i].id == 3) {
            assert(sem[i].tab_index == -9 && sem[i].tab_ordered && !sem[i].tab_stop_disabled);
            checked = true;
        }
    assert(checked);
    options.tab_index = -9;
    options.skip_tab = false;
    options.on_click_event = clicked;
    tab(ui, false, 4, false, false);
    tab(ui, false, 2, false, false);
    tab(ui, false, 1, false, false);
    tab(ui, false, 3, false, false);
    tab(ui, true, 1, false, false);
    cui_focus(ui, 5);
    assert(cui_focused(ui) == 5); /* skip_tab preserves programmatic eligibility. */
    tab(ui, false, 3, false, false);
    cui_focus(ui, 9);
    frame(ui, false, false);
    assert(cui_focused(ui) != 9);
    cui_focus(ui, 6);
    frame(ui, false, false);
    assert(cui_focused(ui) != 6);
    cui_button_click_event saved = copied;
    frame(ui, true, false);
    tab(ui, false, 8, true, false);
    tab(ui, true, 7, true, false);
    frame(ui, false, true);
    cui_focus(ui, 0);
    tab(ui, false, 4, false, true);
    assert(!memcmp(&saved, &copied, sizeof saved));
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 600, 500, .016f));
    cui_button_ex(ui, 10, &t, cui_string("Default"), (cui_button_options){0});
    item(ui, &t, 11, -1, false, false);
    cui_end_frame(ui);
    sem = cui_semantics(ui, &count);
    assert(count == 2 && sem[0].tab_index == 0 && !sem[0].tab_ordered &&
           !sem[0].tab_stop_disabled && sem[1].tab_index == -1 && sem[1].tab_ordered &&
           !sem[1].tab_stop_disabled);
    cui_focus(ui, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    assert(cui_begin_frame(ui, 600, 500, .016f));
    cui_button_ex(ui, 10, &t, cui_string("Default"), (cui_button_options){0});
    item(ui, &t, 11, -1, false, false);
    assert(cui_focused(ui) == 11);
    cui_end_frame(ui);
    cui_destroy(ui);
}
