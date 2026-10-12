/* SPDX-License-Identifier: MIT */
#include "cui/cui_sidebar.h"
#ifndef CUI_SIDEBAR_BASE_ONLY
#include "cui/cui_choice.h"
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
static void noop(cui_context *ui, const cui_theme *theme, bool collapsed, void *user) {
    (void)ui;
    (void)theme;
    (void)collapsed;
    (void)user;
}
static void closef(float a, float b) {
    assert(fabsf(a - b) < .02f);
}
static void test_model(void) {
    cui_sidebar_state s;
    cui_sidebar_cache cache[4];
    assert(cui_sidebar_state_init(&s, cache, 4));
    cui_sidebar_entry e[] = {{.id = 1, .render = noop, .estimated_height = 32},
                             {.id = 2, .render = noop, .estimated_height = 64}};
    cui_sidebar_options o = cui_sidebar_default();
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(s.width, 255);
    assert(s.render_child);
    cache[0].bounds.h = 50;
    cache[1].bounds.h = 70;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(cache[0].height, 50);
    cui_sidebar_entry reorder[] = {e[1], {.id = 3, .render = noop, .estimated_height = 30}, e[0]};
    assert(cui_sidebar_prepare(&s, reorder, 3, o, 400));
    assert(cache[0].id == 2 && cache[1].id == 3 && cache[2].id == 1);
    closef(cache[0].height, 70);
    closef(cache[2].height, 50);
    o.collapsed = true;
    o.now_seconds = 1;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    assert(s.icon_collapsed && s.animating);
    closef(s.width, 255);
    o.now_seconds = 1.1;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(s.width, 151.5f);
    o.now_seconds = 1.2;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(s.width, 48);
    assert(!s.animating);
    o.collapse = CUI_SIDEBAR_OFFCANVAS;
    o.now_seconds = 2;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    assert(s.render_child && s.animating);
    o.now_seconds = 2.1;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(s.width, 24);
    o.collapsed = false;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    assert(s.render_child);
    closef(s.width, 24);
    o.now_seconds = 2.3;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(s.width, 255);
    o.collapsed = true;
    o.reduced_motion = true;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    assert(!s.render_child);
    closef(s.width, 0);
    o.collapse = CUI_SIDEBAR_NONE;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    assert(s.render_child && !s.icon_collapsed);
    closef(s.width, 255);
    o.collapse = CUI_SIDEBAR_ICON;
    o.width_percent = true;
    o.width = .5f;
    o.collapsed = false;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(s.width, 200);
    assert(!s.animating);
    o.collapsed = true;
    assert(cui_sidebar_prepare(&s, e, 2, o, 400));
    closef(s.width, 48);
    cui_sidebar_state before = s;
    o.width = NAN;
    assert(!cui_sidebar_prepare(&s, e, 2, o, 400));
    assert(!memcmp(&s, &before, sizeof(s)));
    o = cui_sidebar_default();
    e[1].id = 1;
    assert(!cui_sidebar_prepare(&s, e, 2, o, 400));
    assert(!cui_sidebar_prepare(&s, NULL, 1, o, 400));
}
static unsigned clicks, expands;
static bool requested;
static void activate(void *user) {
    assert(user == &clicks);
    clicks++;
}
static void expand(bool value, void *user) {
    assert(user == &clicks);
    expands++;
    requested = value;
}
static void base_frame(cui_context *ui, cui_sidebar_menu_item *item, bool collapsed) {
    cui_begin_frame(ui, 200, 100, .016f);
    cui_base_sidebar_item(ui, item, collapsed, 1);
    cui_begin_box(ui, item->id, (cui_box){.width = 160, .height = 28});
    cui_end_box(ui);
    if (item->child_count && !collapsed) {
        cui_begin_box(ui, cui_sidebar_part_id(item->id, 0), (cui_box){.width = 20, .height = 20});
        cui_end_box(ui);
    }
    cui_end_frame(ui);
}
static void key(cui_context *ui, cui_key k) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void test_base(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_sidebar_item_state state = {0};
    cui_sidebar_menu_item child = {.id = 11, .label = cui_string("Child")};
    cui_sidebar_menu_item item = {.id = 10,
                                  .label = cui_string("Projects"),
                                  .state = &state,
                                  .children = &child,
                                  .child_count = 1,
                                  .default_open = true,
                                  .on_activate = activate,
                                  .on_expand = expand,
                                  .user = &clicks};
    clicks = expands = 0;
    base_frame(ui, &item, false);
    assert(state.open);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(clicks == 1 && expands == 0 && state.open);
    item.click_to_toggle = true;
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(clicks == 2 && expands == 1 && !state.open);
    item.click_to_open = true;
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(state.open && requested);
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(state.open && expands == 2);
    item.disabled = true;
    cui_focus(ui, cui_sidebar_part_id(10, 0));
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(!state.open && clicks == 4 && expands == 3);
    item.disable_caret = true;
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(expands == 3);
    item.disable_caret = false;
    item.disabled = false;
    bool controlled = false;
    item.expanded = &controlled;
    cui_focus(ui, 10);
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(requested && !controlled);
    size_t n = 0;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == 10) {
            assert(!sem[i].details.expanded);
            found = true;
        }
    assert(found);
    item.expanded = NULL;
    item.click_to_open = false;
    unsigned before_clicks = clicks, before_expands = expands;
    bool before_open = state.open;
    cui_focus(ui, 10);
    key(ui, CUI_KEY_ENTER);
    key(ui, CUI_KEY_ENTER);
    base_frame(ui, &item, false);
    assert(clicks == before_clicks + 2 && expands == before_expands + 2 &&
           state.open == before_open);
    base_frame(ui, &item, true);
    sem = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        assert(sem[i].id != cui_sidebar_part_id(10, 0));
    cui_destroy(ui);
}
#ifndef CUI_SIDEBAR_BASE_ONLY
typedef struct {
    unsigned calls;
    bool collapsed;
    float height;
    cui_id control;
} content_state;
static void content(cui_context *ui, const cui_theme *theme, bool collapsed, void *user) {
    content_state *s = user;
    s->calls++;
    s->collapsed = collapsed;
    if (s->control)
        cui_interact(ui, s->control, CUI_ROLE_BUTTON, cui_string("Entry"), false, false, 0);
    cui_begin_box(ui, s->control,
                  (cui_box){.width = -1, .height = s->height, .background = theme->primary});
    cui_end_box(ui);
}
static void render_frame(cui_context *ui, const cui_theme *theme, cui_sidebar_state *s,
                         cui_sidebar_entry *entries, size_t count, cui_sidebar_options o) {
    cui_begin_frame(ui, 600, 300, .016f);
    cui_begin_box(ui, 900, (cui_box){.width = 600, .height = 300, .row = true});
    assert(cui_sidebar(ui, 100, theme, s, entries, count, o));
    cui_begin_box(ui, 901, (cui_box){.width = -1, .height = -1});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
}
static void test_layout(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t t = 0; t < 4; t++) {
        cui_context *ui = cui_create((cui_config){.max_elements = 256});
        assert(ui);
        cui_sidebar_state s;
        cui_sidebar_cache cache[4];
        assert(cui_sidebar_state_init(&s, cache, 4));
        content_state h = {.height = 20}, f = {.height = 20}, c = {.height = 500, .control = 111};
        cui_sidebar_entry e = {.id = 110, .render = content, .user = &c, .estimated_height = 500};
        cui_sidebar_options o = cui_sidebar_default();
        o.height = 300;
        o.header = content;
        o.header_user = &h;
        o.footer = content;
        o.footer_user = &f;
        render_frame(ui, &themes[t], &s, &e, 1, o);
        render_frame(ui, &themes[t], &s, &e, 1, o);
        closef(s.bounds.w, 255);
        assert(s.viewport.h < 260 && s.viewport.h > 220);
        assert(h.calls == 2 && f.calls == 2 && c.calls == 2);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = s.viewport.x + 30,
                                               .y = s.viewport.y + 30,
                                               .dy = -2}));
        render_frame(ui, &themes[t], &s, &e, 1, o);
        assert(s.scroll.y > 0);
        o.collapsed = true;
        o.reduced_motion = true;
        render_frame(ui, &themes[t], &s, &e, 1, o);
        closef(s.bounds.w, 48);
        closef(s.viewport.h, 216);
        assert(c.collapsed);
        o.collapse = CUI_SIDEBAR_OFFCANVAS;
        render_frame(ui, &themes[t], &s, &e, 1, o);
        closef(s.bounds.w, 0);
        assert(!s.render_child);
        size_t n = 0;
        const cui_semantic *sem = cui_semantics(ui, &n);
        for (size_t i = 0; i < n; i++)
            assert(sem[i].id != 111);
        o.collapse = CUI_SIDEBAR_NONE;
        render_frame(ui, &themes[t], &s, &e, 1, o);
        closef(s.bounds.w, 255);
        assert(!c.collapsed);
        cui_destroy(ui);
    }
}
static void test_virtual_and_animation(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_theme t = cui_theme_light();
    cui_sidebar_state s;
    cui_sidebar_cache cache[32];
    assert(cui_sidebar_state_init(&s, cache, 32));
    content_state c[32] = {0};
    cui_sidebar_entry e[32] = {0};
    for (size_t i = 0; i < 32; i++) {
        c[i].height = 40;
        c[i].control = 1000 + i;
        e[i] = (cui_sidebar_entry){
            .id = 2000 + i, .render = content, .user = &c[i], .estimated_height = 40};
    }
    cui_sidebar_options o = cui_sidebar_default();
    o.height = 100;
    o.viewport_height = 300;
    render_frame(ui, &t, &s, e, 32, o);
    render_frame(ui, &t, &s, e, 32, o);
    unsigned rendered = 0;
    for (size_t i = 0; i < 32; i++)
        if (c[i].calls)
            rendered++;
    assert(rendered < 10 && rendered > 3 && c[31].calls == 0);
    s.scroll.y = 800;
    render_frame(ui, &t, &s, e, 32, o);
    assert(c[20].calls && c[0].calls == 2);
    assert(cui_last_error(ui) == CUI_OK);
    cui_rect bar = cui_bounds(ui, cui_scrollbar_id(cui_sidebar_part_id(100, 1), true));
    closef(bar.x, s.viewport.x + s.viewport.w - 8);
    o.width_percent = true;
    o.width = .5f;
    render_frame(ui, &t, &s, e, 32, o);
    closef(s.bounds.w, 300);
    bar = cui_bounds(ui, cui_scrollbar_id(cui_sidebar_part_id(100, 1), true));
    closef(bar.x, 292);
    o.width_percent = false;
    o.width = 0;
    s.scroll.y = 0;
    render_frame(ui, &t, &s, e, 32, o);
    cui_focus(ui, 1000);
    assert(cui_focused(ui) == 1000);
    o.collapse = CUI_SIDEBAR_OFFCANVAS;
    o.collapsed = true;
    o.now_seconds = 1;
    render_frame(ui, &t, &s, e, 32, o);
    assert(s.render_child && cui_focused(ui) == 1000);
    o.now_seconds = 1.1;
    render_frame(ui, &t, &s, e, 32, o);
    closef(s.width, 127.5f);
    closef(cui_bounds(ui, cui_sidebar_part_id(100, 0)).x, -127.5f);
    assert(cui_focused(ui) == 1000);
    o.side = CUI_SIDEBAR_RIGHT;
    render_frame(ui, &t, &s, e, 32, o);
    closef(cui_bounds(ui, cui_sidebar_part_id(100, 0)).x, 0);
    o.now_seconds = 1.2;
    render_frame(ui, &t, &s, e, 32, o);
    assert(!s.render_child && cui_focused(ui) == 0);
    render_frame(ui, &t, &s, e, 32, o);
    assert(!cui_needs_redraw(ui));
    cui_destroy(ui);
}
static void test_toggle(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_theme t = cui_theme_light();
    for (int side = 0; side < 2; side++)
        for (int collapsed = 0; collapsed < 2; collapsed++) {
            cui_sidebar_toggle_options o = {.side = (cui_sidebar_side)side,
                                            .collapsed = collapsed,
                                            .accessibility_label = cui_string("Toggle sidebar")};
            cui_begin_frame(ui, 100, 100, .016f);
            cui_sidebar_toggle_button(ui, 300, &t, o);
            cui_draw_list list = cui_end_frame(ui);
            unsigned images = 0;
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_IMAGE)
                    images++;
            assert(images == 1);
            closef(cui_bounds(ui, 300).w, 24);
        }
    cui_destroy(ui);
}
typedef struct {
    cui_menu_tree_state state;
    cui_id action;
} context_state;
static void context(cui_context *ui, const cui_theme *theme, bool collapsed, void *user) {
    (void)collapsed;
    context_state *s = user;
    cui_menu_entry entry = {.id = 420, .label = cui_string("Open")};
    cui_context_menu(ui, 410, theme, cui_string("Actions"), &entry, 1, &s->action, &s->state,
                     (cui_menu_tree_options){.popup = {.viewport = {0, 0, 300, 200}}});
}
static void suffix(cui_context *ui, const cui_theme *theme, bool collapsed, void *user) {
    (void)collapsed;
    (void)theme;
    unsigned *click = user;
    cui_interaction v =
        cui_interact(ui, 412, CUI_ROLE_BUTTON, cui_string("Suffix"), false, false, 0);
    if (v.activated)
        (*click)++;
    cui_begin_box(ui, 412, (cui_box){.width = 20, .height = 20});
    cui_end_box(ui);
}
static void composed_frame(cui_context *ui, const cui_theme *theme, cui_sidebar_menu_item *item,
                           bool collapsed) {
    cui_begin_frame(ui, 300, 200, .016f);
    cui_begin_box(ui, 400, (cui_box){.width = 255, .height = 200});
    assert(cui_sidebar_menu(ui, 401, theme, item, 1, collapsed));
    cui_end_box(ui);
    cui_end_frame(ui);
}
static void test_composition(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_theme t = cui_theme_light();
    context_state context_value = {.state = {.overlay = {.id = 430}}};
    unsigned suffix_clicks = 0;
    cui_sidebar_item_state item_state = {0};
    cui_sidebar_menu_item child = {.id = 411, .label = cui_string("Child")};
    cui_sidebar_menu_item item = {
        .id = 410,
        .label = cui_string("Parent"),
        .icon = cui_string("folder"),
        .state = &item_state,
        .children = &child,
        .child_count = 1,
        .disabled = true,
        .suffix = suffix,
        .suffix_user = &suffix_clicks,
        .context = context,
        .context_user = &context_value,
        .on_activate = activate,
        .user = &clicks,
        .tooltip_options = {.viewport = {0, 0, 300, 200}, .dt = .1f, .open_delay = .1f}};
    clicks = 0;
    composed_frame(ui, &t, &item, false);
    cui_rect caret = cui_bounds(ui, cui_sidebar_part_id(410, 0));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = caret.x + 10, .y = caret.y + 10}));
    composed_frame(ui, &t, &item, false);
    assert(!item_state.open && cui_pressed(ui, cui_sidebar_part_id(410, 0)));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 280, .y = 180}));
    composed_frame(ui, &t, &item, false);
    assert(cui_pressed(ui, cui_sidebar_part_id(410, 0)));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = caret.x + 10, .y = caret.y + 10}));
    composed_frame(ui, &t, &item, false);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = caret.x + 10, .y = caret.y + 10}));
    composed_frame(ui, &t, &item, false);
    assert(item_state.open && clicks == 0);
    cui_focus(ui, 412);
    key(ui, CUI_KEY_ENTER);
    composed_frame(ui, &t, &item, false);
    assert(suffix_clicks == 1 && clicks == 0);
    cui_rect row = cui_bounds(ui, 410);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_SECONDARY,
                                           .x = row.x + 4,
                                           .y = row.y + 4}));
    composed_frame(ui, &t, &item, false);
    assert(context_value.state.overlay.open);
    key(ui, CUI_KEY_ESCAPE);
    composed_frame(ui, &t, &item, false);
    assert(!context_value.state.overlay.open);
    item.disabled = false;
    composed_frame(ui, &t, &item, true);
    row = cui_bounds(ui, 410);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = row.x + 10, .y = row.y + 10}));
    composed_frame(ui, &t, &item, true);
    composed_frame(ui, &t, &item, true);
    assert(item_state.tooltip.open);
    cui_destroy(ui);
}
static void section_text(cui_context *ui, const cui_theme *theme, bool collapsed, void *user) {
    (void)collapsed;
    (void)user;
    cui_text(ui, cui_string("Section"), 14, theme->text);
}
static void section_frame(cui_context *ui, const cui_theme *theme, bool selected) {
    cui_begin_frame(ui, 300, 200, .016f);
    cui_begin_box(ui, 600, (cui_box){.width = 200, .height = 100});
    assert(cui_sidebar_header(ui, 610, theme,
                              (cui_sidebar_section_options){.content = section_text,
                                                            .selected = selected,
                                                            .on_activate = activate,
                                                            .activate_user = &clicks}));
    cui_end_box(ui);
    cui_end_frame(ui);
}
static void test_section_and_text(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t t = 0; t < 4; t++) {
        cui_context *ui = cui_create((cui_config){.max_elements = 128});
        assert(ui);
        clicks = 0;
        section_frame(ui, &themes[t], false);
        cui_rect section = cui_bounds(ui, 610);
        assert(cui_queue_event(
            ui,
            (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = section.x + 4, .y = section.y + 4}));
        section_frame(ui, &themes[t], false);
        assert(cui_pressed(ui, 610) && clicks == 0 && cui_focused(ui) == 0);
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 280, .y = 180}));
        section_frame(ui, &themes[t], false);
        assert(cui_pressed(ui, 610));
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = section.x + 4, .y = section.y + 4}));
        section_frame(ui, &themes[t], false);
        assert(clicks == 1 && cui_focused(ui) == 0);
        cui_sidebar_menu_item item = {.id = 620, .label = cui_string("Centered"), .active = true};
        cui_begin_frame(ui, 255, 100, .016f);
        cui_begin_box(ui, 600, (cui_box){.width = 255, .height = 100});
        assert(cui_sidebar_menu(ui, 621, &themes[t], &item, 1, false));
        cui_end_box(ui);
        cui_draw_list draw = cui_end_frame(ui);
        cui_rect row = cui_bounds(ui, 620);
        bool found = false;
        for (size_t i = 0; i < draw.count; i++)
            if (draw.commands[i].type == CUI_DRAW_TEXT) {
                const cui_draw_command *d = &draw.commands[i];
                closef(d->bounds.y + d->bounds.h * .5f, row.y + row.h * .5f);
                assert(d->bounds.y >= row.y && d->bounds.y + d->bounds.h <= row.y + row.h + .02f);
                assert(!memcmp(&d->color, &themes[t].sidebar_accent_foreground, sizeof(cui_color)));
                found = true;
            }
        assert(found);
        cui_destroy(ui);
    }
}
static void test_menu(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_sidebar_item_state state = {0};
    cui_sidebar_menu_item child = {.id = 211, .label = cui_string("Child")};
    cui_sidebar_menu_item item = {.id = 210,
                                  .label = cui_string("Parent"),
                                  .icon = cui_string("folder"),
                                  .state = &state,
                                  .default_open = true,
                                  .children = &child,
                                  .child_count = 1};
    cui_begin_frame(ui, 255, 300, .016f);
    cui_begin_box(ui, 200, (cui_box){.width = 255, .height = 300});
    assert(cui_sidebar_menu(ui, 201, &theme, &item, 1, false));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    closef(cui_bounds(ui, 210).h, 28);
    assert(cui_bounds(ui, 211).x >= 24);
    bool image = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_IMAGE)
            image = true;
    assert(image);
    cui_focus(ui, cui_sidebar_part_id(210, 0));
    key(ui, CUI_KEY_ENTER);
    cui_begin_frame(ui, 255, 300, .016f);
    cui_begin_box(ui, 200, (cui_box){.width = 255, .height = 300});
    assert(cui_sidebar_menu(ui, 201, &theme, &item, 1, false));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(!state.open);
    cui_begin_frame(ui, 48, 300, .016f);
    cui_begin_box(ui, 200, (cui_box){.width = 48, .height = 300});
    assert(cui_sidebar_menu(ui, 201, &theme, &item, 1, true));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_bounds(ui, 210).h >= 32);
    size_t n = 0;
    const cui_semantic *sem = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        assert(sem[i].id != 211 && sem[i].id != cui_sidebar_part_id(210, 0));
    cui_destroy(ui);
}
#endif
int main(void) {
    test_model();
    test_base();
#ifndef CUI_SIDEBAR_BASE_ONLY
    test_layout();
    test_menu();
    test_virtual_and_animation();
    test_toggle();
    test_composition();
    test_section_and_text();
#endif
    return 0;
}
