/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include "cui/cui_toolbar.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void test_metrics(void) {
    cui_toolbar_metrics m;
    assert(cui_toolbar_measure(CUI_TOOLBAR_SMALL, 0, &m));
    assert(m.height == 32 && m.padding == 4 && m.gap == 4 && m.control_height == 24 &&
           m.font_size == 14);
    assert(cui_toolbar_measure(CUI_TOOLBAR_XSMALL, 0, &m));
    assert(m.height == 28 && m.control_height == 20 && m.font_size == 12);
    assert(cui_toolbar_measure(CUI_TOOLBAR_LARGE, 0, &m));
    assert(m.size == CUI_TOOLBAR_MEDIUM && m.height == 48 && m.control_height == 32);
    assert(cui_toolbar_measure(CUI_TOOLBAR_CUSTOM, 37, &m));
    assert(m.custom_size == 37 && m.height == 48 && m.control_height == 24);
    assert(!cui_toolbar_measure(CUI_TOOLBAR_CUSTOM, NAN, &m));
    assert(!cui_toolbar_measure((cui_toolbar_size)99, 0, &m));
    assert(!cui_toolbar_measure(CUI_TOOLBAR_SMALL, 0, NULL));
}
static void control(cui_context *ui, cui_id id, bool disabled, cui_role role) {
    cui_interact(ui, id, role, cui_string("Command"), disabled, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = 24, .height = 24});
    cui_end_box(ui);
}
static void base_bar(cui_context *ui, bool disabled) {
    assert(cui_base_toolbar_begin(ui, 100, cui_string("Commands"), (cui_box){.row = true}));
    control(ui, 101, false, CUI_ROLE_BUTTON);
    assert(cui_base_toolbar_group_begin(ui, 110, cui_string("History"), (cui_box){.row = true}));
    control(ui, 102, true, CUI_ROLE_BUTTON);
    control(ui, 103, false, CUI_ROLE_BUTTON);
    cui_base_toolbar_group_end(ui);
    cui_base_toolbar_end(ui, 100, disabled);
    control(ui, 104, false, CUI_ROLE_BUTTON);
}
static void frame(cui_context *ui, bool disabled) {
    cui_begin_frame(ui, 400, 100, .016f);
    base_bar(ui, disabled);
    cui_end_frame(ui);
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void test_navigation(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    frame(ui, false);
    cui_focus(ui, 101);
    key(ui, CUI_KEY_RIGHT);
    frame(ui, false);
    assert(cui_focused(ui) == 103);
    key(ui, CUI_KEY_RIGHT);
    frame(ui, false);
    assert(cui_focused(ui) == 101);
    key(ui, CUI_KEY_LEFT);
    frame(ui, false);
    assert(cui_focused(ui) == 103);
    key(ui, CUI_KEY_LEFT);
    frame(ui, true);
    assert(cui_focused(ui) == 103);
    key(ui, CUI_KEY_TAB);
    frame(ui, false);
    assert(cui_focused(ui) == 104);
    key(ui, CUI_KEY_LEFT);
    frame(ui, false);
    assert(cui_focused(ui) == 104);
    size_t count = 0;
    const cui_semantic *sem = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (sem[i].id == 100) {
            assert(sem[i].role == CUI_ROLE_TOOLBAR && !sem[i].disabled && sem[i].tab_index == -1);
            assert(sem[i].details.orientation == CUI_ORIENTATION_HORIZONTAL);
        }
    cui_destroy(ui);
}
static void nested_frame(cui_context *ui) {
    cui_begin_frame(ui, 400, 100, .016f);
    assert(cui_base_toolbar_begin(ui, 300, cui_string("Outer"), (cui_box){.row = true}));
    control(ui, 301, false, CUI_ROLE_BUTTON);
    assert(cui_base_toolbar_begin(ui, 310, cui_string("Inner"), (cui_box){.row = true}));
    control(ui, 311, false, CUI_ROLE_BUTTON);
    control(ui, 312, false, CUI_ROLE_BUTTON);
    cui_base_toolbar_end(ui, 310, false);
    control(ui, 313, false, CUI_ROLE_TEXTBOX);
    if (cui_focused(ui) == 313) {
        size_t n = 0;
        const cui_event *events = cui_frame_events(ui, &n);
        for (size_t i = 0; i < n; i++)
            if (events[i].type == CUI_EVENT_KEY_DOWN &&
                (events[i].key == CUI_KEY_LEFT || events[i].key == CUI_KEY_RIGHT))
                assert(cui_consume_event(ui, i));
    }
    cui_base_toolbar_end(ui, 300, false);
    cui_end_frame(ui);
}
static void test_nested(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    nested_frame(ui);
    cui_focus(ui, 311);
    key(ui, CUI_KEY_RIGHT);
    nested_frame(ui);
    assert(cui_focused(ui) == 312);
    key(ui, CUI_KEY_RIGHT);
    nested_frame(ui);
    assert(cui_focused(ui) == 311);
    cui_focus(ui, 313);
    key(ui, CUI_KEY_LEFT);
    nested_frame(ui);
    assert(cui_focused(ui) == 313);
    cui_destroy(ui);
}
static void input_frame(cui_context *ui, cui_edit *edit) {
    cui_begin_frame(ui, 400, 100, .016f);
    assert(cui_base_toolbar_begin(ui, 400, cui_string("Editing"), (cui_box){.row = true}));
    control(ui, 401, false, CUI_ROLE_BUTTON);
    cui_base_input(ui, 402, cui_string("Query"), edit, false);
    cui_begin_box(ui, 402, (cui_box){.width = 100, .height = 24});
    cui_end_box(ui);
    cui_base_toolbar_end(ui, 400, false);
    cui_end_frame(ui);
}
static void test_input(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    char text[32] = "abc";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = text, .capacity = 32});
    assert(edit);
    input_frame(ui, edit);
    cui_focus(ui, 402);
    cui_edit_select(edit, 0, 0);
    key(ui, CUI_KEY_LEFT);
    input_frame(ui, edit);
    assert(cui_focused(ui) == 402);
    assert(cui_edit_get_state(edit).cursor == 0);
    key(ui, CUI_KEY_RIGHT);
    input_frame(ui, edit);
    assert(cui_focused(ui) == 402);
    assert(cui_edit_get_state(edit).cursor == 1);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
#ifndef CUI_TOOLBAR_BASE_ONLY
typedef struct {
    unsigned called;
    bool prepared;
    float height;
    cui_id control;
} probe;
static void item(cui_context *ui, const cui_theme *theme, const cui_toolbar_presentation *p,
                 void *user) {
    probe *v = user;
    v->called++;
    v->prepared = p->prepared;
    v->height = p->metrics.control_height;
    cui_interact(ui, v->control, CUI_ROLE_BUTTON, cui_string("Item"), false, false, 0);
    cui_begin_box(ui, v->control,
                  (cui_box){.width = 20,
                            .height = p->prepared ? p->metrics.control_height : 10,
                            .background = theme->primary});
    cui_end_box(ui);
}
static unsigned activated;
static void command_activate(void *user) {
    assert(user == &activated);
    activated++;
}
static void command_frame(cui_context *ui, const cui_theme *theme, cui_toolbar_command *command,
                          cui_toolbar_options options) {
    cui_toolbar_item i = {
        .id = 501, .sized = true, .render = cui_toolbar_command_render, .user = command};
    cui_begin_frame(ui, 400, 100, .016f);
    assert(cui_toolbar(ui, 500, theme, &i, 1, options));
    cui_end_frame(ui);
}
static void test_command(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_toolbar_command c = {.id = 502,
                             .label = cui_string("Save"),
                             .accessible_name = cui_string("Save document"),
                             .on_activate = command_activate,
                             .activate_user = &activated};
    cui_toolbar_options o = cui_toolbar_default();
    activated = 0;
    command_frame(ui, &theme, &c, o);
    cui_rect b = cui_bounds(ui, 502);
    assert(b.h == 24 && b.w >= 24);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 2, .y = b.y + 2}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 2, .y = b.y + 2}));
    command_frame(ui, &theme, &c, o);
    assert(activated == 1);
    key(ui, CUI_KEY_ENTER);
    command_frame(ui, &theme, &c, o);
    assert(activated == 2);
    c.disabled = true;
    key(ui, CUI_KEY_ENTER);
    command_frame(ui, &theme, &c, o);
    assert(activated == 2);
    size_t n = 0;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == 502) {
            assert(sem[i].disabled && sem[i].role == CUI_ROLE_BUTTON);
            assert(sem[i].name.length == 13 && !memcmp(sem[i].name.data, "Save document", 13));
            found = true;
        }
    assert(found);
    c.disabled = false;
    c.label = (cui_str){0};
    o.size = CUI_TOOLBAR_CUSTOM;
    o.custom_size = 37;
    command_frame(ui, &theme, &c, o);
    b = cui_bounds(ui, 502);
    assert(b.w == 37 && b.h == 37);
    c.accessible_name = (cui_str){0};
    command_frame(ui, &theme, &c, o);
    sem = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        assert(sem[i].id != 502);
    cui_destroy(ui);
}
static void test_layout(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t t = 0; t < 4; t++)
        for (int size = 0; size < 5; size++) {
            probe a = {.control = 201}, b = {.control = 202};
            cui_toolbar_item items[] = {{.id = 211, .sized = true, .render = item, .user = &a},
                                        {.render = item, .user = &b}};
            cui_toolbar_options o = cui_toolbar_default();
            o.size = (cui_toolbar_size)size;
            o.custom_size = 37;
            cui_begin_frame(ui, 400, 100, .016f);
            assert(cui_toolbar(ui, 200, &themes[t], items, 2, o));
            cui_end_frame(ui);
            cui_toolbar_metrics m;
            assert(cui_toolbar_measure(o.size, 37, &m));
            cui_rect bar = cui_bounds(ui, 200), wrapper = cui_bounds(ui, 211),
                     content = cui_bounds(ui, 202);
            assert(bar.h == m.height && wrapper.h == m.control_height && content.h == 10);
            assert(wrapper.x == m.padding && content.x == m.padding + 20 + m.gap);
            assert(a.called == 1 && a.prepared && b.called == 1 && !b.prepared);
            assert(a.height == m.control_height);
            cui_begin_frame(ui, 400, 100, .016f);
            items[0].id = 200;
            assert(!cui_toolbar(ui, 200, &themes[t], items, 2, o));
            size_t n = 99;
            cui_semantics(ui, &n);
            assert(n == 0);
            cui_end_frame(ui);
        }
    cui_destroy(ui);
}
#endif
int main(void) {
    test_metrics();
    test_navigation();
    test_nested();
    test_input();
#ifndef CUI_TOOLBAR_BASE_ONLY
    test_layout();
    test_command();
#endif
    return 0;
}
