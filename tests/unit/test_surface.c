/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static bool allow_accept;
static unsigned decisions;
static bool decide(void *user, cui_surface_result result) {
    (void)user;
    decisions++;
    return result != CUI_SURFACE_ACCEPTED || allow_accept;
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static bool button(cui_context *ui, cui_id id, const cui_theme *theme, cui_str name,
                   bool disabled) {
#ifdef CUI_SURFACE_BASE_ONLY
    (void)theme;
    cui_begin_box(ui, id, (cui_box){.width = 100, .height = 28});
    bool activated = cui_interact(ui, id, CUI_ROLE_BUTTON, name, disabled, false, 0).activated;
    cui_end_box(ui);
    return activated;
#else
    return cui_button(ui, id, theme, name, disabled);
#endif
}
static bool begin(cui_context *ui, const cui_theme *theme, cui_surface_state *s,
                  cui_surface_options options) {
#ifdef CUI_SURFACE_BASE_ONLY
    if (!cui_base_surface_begin(ui, s, cui_string("Surface title"), cui_string("Description"),
                                options))
        return false;
    cui_rect bounds = cui_surface_current_geometry(s, options);
    cui_begin_portal(ui, s->overlay.id, (cui_box){0}, bounds,
                     options.layer ? (int16_t)(options.layer + 1) : 101);
    cui_rect handle = cui_surface_resize_bounds(s, options);
    if (handle.w > 0 && handle.h > 0) {
        cui_begin_portal(ui, cui_surface_part_id(s->overlay.id, 5), (cui_box){0}, handle,
                         options.layer ? (int16_t)(options.layer + 2) : 102);
        cui_base_surface_submit_resize(ui, s, options);
        cui_end_box(ui);
    }
    cui_begin_box(ui, cui_surface_part_id(s->overlay.id, 4), (cui_box){.width = 100, .height = 20});
    cui_end_box(ui);
    if (!options.hide_close && (s->kind != CUI_SURFACE_ALERT || options.show_alert_close))
        button(ui, cui_surface_part_id(s->overlay.id, 1), theme, cui_string("Close"), false);
    return true;
#else
    return cui_surface_begin(ui, theme, s, cui_string("Surface title"), cui_string("Description"),
                             options);
#endif
}
static void end(cui_context *ui) {
#ifdef CUI_SURFACE_BASE_ONLY
    cui_end_box(ui);
    cui_base_surface_end(ui);
#else
    cui_surface_end(ui);
#endif
}
static cui_draw_list scene(cui_context *ui, const cui_theme *theme, cui_surface_state *s,
                           cui_surface_options options, bool body_closes) {
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_begin_box(ui, 900, (cui_box){.width = 600, .height = 400});
    button(ui, 1, theme, cui_string("Open"), false);
    button(ui, 2, theme, cui_string("Background"), false);
    cui_begin_box(ui, 901, (cui_box){.width = 40, .height = 40, .clip = true});
    if (begin(ui, theme, s, options)) {
        if (s->kind == CUI_SURFACE_ALERT) {
            button(ui, cui_surface_part_id(s->overlay.id, 3), theme, cui_string("Cancel"), false);
            button(ui, cui_surface_part_id(s->overlay.id, 2), theme, cui_string("Confirm"), false);
        } else {
            if (button(ui, 20, theme, cui_string("First action"), false) && body_closes)
                cui_surface_close(ui, s, CUI_SURFACE_CLOSED);
            button(ui, 21, theme, cui_string("Disabled"), true);
            button(ui, 22, theme, cui_string("Last action"), false);
        }
        end(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
#ifdef CUI_SURFACE_BASE_ONLY
    for (size_t i = 0; i < list.count; ++i)
        assert(list.commands[i].type == CUI_DRAW_CLIP_PUSH ||
               list.commands[i].type == CUI_DRAW_CLIP_POP);
#endif
    return list;
}
static void pointer(cui_context *ui, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
}
static void dialog_events(void) {
#ifdef CUI_SURFACE_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (unsigned theme = 0; theme < 4; ++theme) {
        cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
        cui_surface_state s;
        cui_surface_init(&s, 100, CUI_SURFACE_DIALOG);
        cui_surface_options options = {.viewport = {0, 0, 600, 400}, .width = 300, .height = 260};
        scene(ui, &themes[theme], &s, options, false);
        cui_focus(ui, 1);
        cui_surface_open(ui, &s, 1);
        scene(ui, &themes[theme], &s, options, false);
        cui_rect bounds = cui_bounds(ui, 100);
        assert(bounds.x == 150 && bounds.y == 70 && bounds.w == 300 && bounds.h == 260);
        assert(cui_bounds(ui, 20).x >= bounds.x &&
               cui_bounds(ui, 20).x > 40); /* Portal escapes parent clip. */
        size_t count;
        const cui_semantic *nodes = cui_semantics(ui, &count);
        bool dialog = false;
        for (size_t i = 0; i < count; ++i)
            if (nodes[i].id == 100) {
                dialog = true;
                assert(nodes[i].role == CUI_ROLE_DIALOG &&
                       nodes[i].details.description.length == 11 &&
                       nodes[i].details.labelled_by == cui_surface_part_id(100, 4));
            }
        assert(dialog);
        for (unsigned i = 0; i < 23; ++i)
            key(ui, CUI_KEY_TAB);
        scene(ui, &themes[theme], &s, options, false);
        assert(cui_focused(ui) == 20 || cui_focused(ui) == 22 ||
               cui_focused(ui) == cui_surface_part_id(100, 1));
        key(ui, CUI_KEY_ESCAPE);
        scene(ui, &themes[theme], &s, options, false);
        assert(!s.overlay.open && cui_focused(ui) == 1 &&
               cui_surface_take_result(&s) == CUI_SURFACE_CANCELLED);
        assert(cui_surface_take_result(&s) == CUI_SURFACE_NONE);
        cui_surface_open(ui, &s, 1);
        scene(ui, &themes[theme], &s, options, false);
        pointer(ui, 5, 5);
        scene(ui, &themes[theme], &s, options, false);
        assert(!s.overlay.open && cui_focused(ui) == 1);
        cui_surface_open(ui, &s, 1);
        scene(ui, &themes[theme], &s, options, false);
        cui_focus(ui, 20);
        key(ui, CUI_KEY_ENTER);
        scene(ui, &themes[theme], &s, options, true);
        assert(!s.overlay.open && cui_focused(ui) == 1 &&
               cui_surface_take_result(&s) == CUI_SURFACE_CLOSED);
        nodes = cui_semantics(ui, &count);
        for (size_t i = 0; i < count; ++i)
            assert(nodes[i].id != 100 && nodes[i].id != 20);
        cui_destroy(ui);
    }
}
static void confirmation_and_popover(void) {
#ifdef CUI_SURFACE_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_surface_state s;
    cui_surface_init(&s, 100, CUI_SURFACE_ALERT);
    cui_surface_options options = {
        .viewport = {0, 0, 600, 400}, .width = 300, .height = 260, .decide = decide};
    scene(ui, &theme, &s, options, false);
    cui_surface_open(ui, &s, 1);
    scene(ui, &theme, &s, options, false);
    pointer(ui, 5, 5);
    scene(ui, &theme, &s, options, false);
    assert(s.overlay.open); /* Alert dialogs do not dismiss on outside presses by default. */
    allow_accept = false;
    decisions = 0;
    cui_focus(ui, cui_surface_part_id(100, 2));
    key(ui, CUI_KEY_ENTER);
    scene(ui, &theme, &s, options, false);
    assert(s.overlay.open && decisions == 1 && s.result == CUI_SURFACE_NONE);
    allow_accept = true;
    key(ui, CUI_KEY_ENTER);
    scene(ui, &theme, &s, options, false);
    assert(!s.overlay.open && decisions == 2 && cui_focused(ui) == 1 &&
           cui_surface_take_result(&s) == CUI_SURFACE_ACCEPTED);
    cui_surface_init(&s, 100, CUI_SURFACE_POPOVER);
    options.anchor = (cui_rect){500, 350, 50, 30};
    options.side = CUI_POPUP_BELOW;
    options.no_escape_dismiss = options.no_outside_dismiss = true;
    cui_surface_open(ui, &s, 1);
    scene(ui, &theme, &s, options, false);
    cui_rect panel = cui_bounds(ui, 100);
    assert(panel.x + panel.w <= 600 && panel.y + panel.h <= 400 && panel.y < options.anchor.y);
    cui_focus(ui, 1);
    key(ui, CUI_KEY_TAB);
    scene(ui, &theme, &s, options, false);
    assert(cui_focused(ui) ==
           2); /* Nonmodal popovers leave background keyboard navigation available. */
    key(ui, CUI_KEY_ESCAPE);
    pointer(ui, 5, 5);
    scene(ui, &theme, &s, options, false);
    assert(s.overlay.open);
    cui_destroy(ui);
}
static void sheet_events(void) {
#ifdef CUI_SURFACE_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (unsigned theme = 0; theme < 4; ++theme)
        for (unsigned edge = 0; edge < 4; ++edge) {
            cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
            cui_surface_state s;
            cui_surface_init(&s, 100, CUI_SURFACE_SHEET);
            cui_surface_options o = {.viewport = {0, 0, 600, 400},
                                     .width = 300,
                                     .height = 300,
                                     .edge = (cui_sheet_edge)edge};
            scene(ui, &themes[theme], &s, o, false);
            cui_focus(ui, 1);
            cui_surface_open(ui, &s, 1);
            scene(ui, &themes[theme], &s, o, false);
            cui_rect actual = cui_bounds(ui, 100), expected = cui_surface_geometry(s.kind, o);
            assert(actual.x == expected.x && actual.y == expected.y && actual.w == expected.w &&
                   actual.h == expected.h);
            cui_focus(ui, cui_surface_part_id(100, 1));
            key(ui, CUI_KEY_ENTER);
            scene(ui, &themes[theme], &s, o, false);
            assert(!s.overlay.open && cui_focused(ui) == 1 &&
                   cui_surface_take_result(&s) == CUI_SURFACE_CLOSED);
            cui_surface_open(ui, &s, 1);
            scene(ui, &themes[theme], &s, o, false);
            key(ui, CUI_KEY_ESCAPE);
            scene(ui, &themes[theme], &s, o, false);
            assert(!s.overlay.open && cui_focused(ui) == 1 &&
                   cui_surface_take_result(&s) == CUI_SURFACE_CANCELLED);
            cui_destroy(ui);
        }
}
#ifndef CUI_SURFACE_BASE_ONLY
static void alert_scene(cui_context *ui, const cui_theme *theme, cui_surface_state *s,
                        cui_surface_options o) {
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_begin_box(ui, 900, (cui_box){.width = 600, .height = 400});
    button(ui, 1, theme, cui_string("Open"), false);
    cui_alert_dialog(ui, theme, s, cui_string("Delete record?"),
                     cui_string("This change cannot be undone."), cui_string("Delete"),
                     cui_string("Keep"), o);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void styled_confirmation(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (unsigned theme = 0; theme < 4; ++theme) {
        cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
        cui_surface_state s;
        cui_surface_init(&s, 100, CUI_SURFACE_ALERT);
        cui_surface_options o = {
            .viewport = {0, 0, 600, 400}, .width = 380, .height = 240, .decide = decide};
        alert_scene(ui, &themes[theme], &s, o);
        cui_surface_open(ui, &s, 1);
        alert_scene(ui, &themes[theme], &s, o);
        size_t count;
        const cui_semantic *nodes = cui_semantics(ui, &count);
        bool alert = false;
        for (size_t i = 0; i < count; ++i)
            if (nodes[i].id == 100) {
                alert = true;
                assert(nodes[i].role == CUI_ROLE_ALERTDIALOG);
            }
        assert(alert && cui_bounds(ui, cui_surface_part_id(100, 2)).w > 0 &&
               cui_bounds(ui, cui_surface_part_id(100, 3)).w > 0);
        allow_accept = false;
        decisions = 0;
        cui_focus(ui, cui_surface_part_id(100, 2));
        key(ui, CUI_KEY_ENTER);
        alert_scene(ui, &themes[theme], &s, o);
        assert(s.overlay.open && decisions == 1);
        cui_rect cancel = cui_bounds(ui, cui_surface_part_id(100, 3));
        pointer(ui, cancel.x + cancel.w / 2, cancel.y + cancel.h / 2);
        alert_scene(ui, &themes[theme], &s, o);
        assert(!s.overlay.open && decisions == 2 && cui_focused(ui) == 1 &&
               cui_surface_take_result(&s) == CUI_SURFACE_CANCELLED);
        cui_surface_open(ui, &s, 1);
        alert_scene(ui, &themes[theme], &s, o);
        allow_accept = true;
        cui_focus(ui, cui_surface_part_id(100, 2));
        key(ui, CUI_KEY_ENTER);
        alert_scene(ui, &themes[theme], &s, o);
        assert(!s.overlay.open && cui_surface_take_result(&s) == CUI_SURFACE_ACCEPTED);
        cui_destroy(ui);
    }
}
#endif
static void nested_scene(cui_context *ui, const cui_theme *theme, cui_surface_state *parent,
                         cui_surface_state *child) {
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_begin_box(ui, 900, (cui_box){.width = 600, .height = 400});
    button(ui, 1, theme, cui_string("Open"), false);
    cui_surface_options o = {.viewport = {0, 0, 600, 400}, .width = 400, .height = 300};
    if (begin(ui, theme, parent, o)) {
        button(ui, 20, theme, cui_string("Open child"), false);
        o.width = 300;
        o.height = 260;
        o.layer = 104;
        if (begin(ui, theme, child, o)) {
            button(ui, 30, theme, cui_string("Child action"), false);
            end(ui);
        }
        end(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void nested_surfaces(void) {
#ifdef CUI_SURFACE_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_surface_state parent, child;
    cui_surface_init(&parent, 100, CUI_SURFACE_DIALOG);
    cui_surface_init(&child, 200, CUI_SURFACE_DIALOG);
    nested_scene(ui, &theme, &parent, &child);
    cui_surface_open(ui, &parent, 1);
    nested_scene(ui, &theme, &parent, &child);
    cui_focus(ui, 20);
    cui_surface_open(ui, &child, 20);
    nested_scene(ui, &theme, &parent, &child);
    cui_focus(ui, 30);
    for (unsigned i = 0; i < 9; ++i)
        key(ui, CUI_KEY_TAB);
    nested_scene(ui, &theme, &parent, &child);
    assert(cui_focused(ui) == 30 || cui_focused(ui) == cui_surface_part_id(200, 1));
    key(ui, CUI_KEY_ESCAPE);
    nested_scene(ui, &theme, &parent, &child);
    assert(parent.overlay.open && !child.overlay.open && cui_focused(ui) == 20);
    key(ui, CUI_KEY_ESCAPE);
    nested_scene(ui, &theme, &parent, &child);
    assert(!parent.overlay.open && cui_focused(ui) == 1);
    cui_destroy(ui);
}
static void resize_sheets(void) {
#ifdef CUI_SURFACE_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (unsigned theme = 0; theme < 4; ++theme)
        for (unsigned edge = 0; edge < 4; ++edge) {
            cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
            cui_surface_state s;
            cui_surface_init(&s, 100, CUI_SURFACE_SHEET);
            cui_surface_options o = {.viewport = {0, 0, 600, 400},
                                     .width = 300,
                                     .height = 300,
                                     .sheet_minimum = 160,
                                     .sheet_maximum = 360,
                                     .edge = (cui_sheet_edge)edge};
            bool vertical = edge == CUI_SHEET_TOP || edge == CUI_SHEET_BOTTOM;
            float direction = edge == CUI_SHEET_RIGHT || edge == CUI_SHEET_BOTTOM ? -1 : 1;
            scene(ui, &themes[theme], &s, o, false);
            cui_surface_open(ui, &s, 1);
            scene(ui, &themes[theme], &s, o, false);
            cui_id resize = cui_surface_part_id(100, 5);
            cui_rect handle = cui_bounds(ui, resize);
            assert(handle.w > 0 && handle.h > 0);
            float x = handle.x + handle.w / 2, y = handle.y + handle.h / 2;
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
            scene(ui, &themes[theme], &s, o, false);
            assert(s.resizing && cui_focused(ui) == resize);
            x += vertical ? 0 : direction * 40;
            y += vertical ? direction * 40 : 0;
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x, .y = y}));
            scene(ui, &themes[theme], &s, o, false);
            cui_rect bounds = cui_bounds(ui, 100);
            assert(s.sheet_size == 340 && (vertical ? bounds.h : bounds.w) == 340);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
            scene(ui, &themes[theme], &s, o, false);
            assert(!s.resizing);
            key(ui, CUI_KEY_HOME);
            scene(ui, &themes[theme], &s, o, false);
            assert(s.sheet_size == 160);
            key(ui, vertical ? CUI_KEY_DOWN : CUI_KEY_RIGHT);
            scene(ui, &themes[theme], &s, o, false);
            assert(s.sheet_size == (direction > 0 ? 170 : 160));
            key(ui, CUI_KEY_END);
            scene(ui, &themes[theme], &s, o, false);
            assert(s.sheet_size == 360);
            size_t count;
            const cui_semantic *nodes = cui_semantics(ui, &count);
            bool found = false;
            for (size_t i = 0; i < count; ++i)
                if (nodes[i].id == resize) {
                    found = true;
                    assert(nodes[i].value == 360 && nodes[i].details.minimum == 160 &&
                           nodes[i].details.maximum == 360 && nodes[i].details.controls == 100);
                }
            assert(found);
            handle = cui_bounds(ui, resize);
            x = handle.x + handle.w / 2;
            y = handle.y + handle.h / 2;
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                   .x = x,
                                                   .y = y,
                                                   .pointer_button = CUI_POINTER_SECONDARY}));
            scene(ui, &themes[theme], &s, o, false);
            assert(!s.resizing && s.sheet_size == 360);
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
            scene(ui, &themes[theme], &s, o, false);
            assert(s.resizing);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
            scene(ui, &themes[theme], &s, o, false);
            assert(!s.resizing && s.sheet_size == 360);
            o.no_sheet_resize = true;
            key(ui, CUI_KEY_HOME);
            scene(ui, &themes[theme], &s, o, false);
            assert(s.sheet_size == 360 && !s.resizing);
            nodes = cui_semantics(ui, &count);
            for (size_t i = 0; i < count; ++i)
                assert(nodes[i].id != resize);
            cui_destroy(ui);
        }
}
static void geometry(void) {
    cui_surface_options o = {.viewport = {10, 20, 600, 400}, .width = 200, .height = 150};
    const cui_rect expected[] = {
        {10, 20, 200, 400}, {410, 20, 200, 400}, {10, 20, 600, 150}, {10, 270, 600, 150}};
    for (unsigned edge = 0; edge < 4; ++edge) {
        o.edge = (cui_sheet_edge)edge;
        cui_rect actual = cui_surface_geometry(CUI_SURFACE_SHEET, o);
        assert(!memcmp(&actual, &expected[edge], sizeof actual));
    }
    o.width = o.height = 10000;
    o.margin = 10;
    cui_rect actual = cui_surface_geometry(CUI_SURFACE_DIALOG, o);
    assert(actual.x == 20 && actual.y == 30 && actual.w == 580 && actual.h == 380);
    o.viewport.w = NAN;
    assert(cui_surface_geometry(CUI_SURFACE_DIALOG, o).w == 0);
}
int main(void) {
    geometry();
    dialog_events();
    confirmation_and_popover();
    sheet_events();
    resize_sheets();
    nested_surfaces();
#ifndef CUI_SURFACE_BASE_ONLY
    styled_confirmation();
#endif
    return 0;
}
