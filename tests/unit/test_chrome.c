/* SPDX-License-Identifier: MIT */
#include "cui/cui_chrome.h"
#include <assert.h>
#include <math.h>
static void near(float a, float b) {
    assert(fabsf(a - b) < .02f);
}
static unsigned calls;
static void control(cui_context *ui, const cui_theme *theme, cui_id id, void *user) {
    assert(user == &calls);
    near(theme->font_size, 12);
    calls++;
    cui_begin_box(ui, id ^ 999, (cui_box){.width = 30, .height = 25});
    cui_interact(ui, id ^ 999, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    cui_end_box(ui);
}
typedef struct {
    cui_window_info info;
    unsigned actions[7], closed;
} test_window;
static cui_window_info window_query(void *user) {
    return ((test_window *)user)->info;
}
static bool window_perform(void *user, cui_window_action action, float x, float y) {
    (void)x;
    (void)y;
    test_window *window = user;
    window->actions[action]++;
    if (action == CUI_WINDOW_MAXIMIZE)
        window->info.maximized = true;
    if (action == CUI_WINDOW_RESTORE)
        window->info.maximized = false;
    return true;
}
static void window_close(void *user) {
    ((test_window *)user)->closed++;
}
static cui_draw_list title_frame(cui_context *ui, cui_title_bar_state *state,
                                 cui_title_bar_options o, const cui_theme *theme) {
    assert(cui_begin_frame(ui, 600, 200, .05f));
    cui_chrome_item title = {.id = 101, .text = {"Document", 8}};
    cui_title_bar(ui, 100, theme, state, &title, 1, o);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void title_event(cui_context *ui, cui_event_type type, float x, float y,
                        cui_pointer_button button) {
    assert(
        cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y, .pointer_button = button}));
}
static void title_click(cui_context *ui, float x, float y) {
    title_event(ui, CUI_EVENT_POINTER_DOWN, x, y, CUI_POINTER_PRIMARY);
    title_event(ui, CUI_EVENT_POINTER_UP, x, y, CUI_POINTER_PRIMARY);
}
static void title_bar(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 200});
    assert(ui);
    test_window window = {.info = {.decoration = CUI_WINDOW_CLIENT_DECORATED, .actions = 127}};
    cui_window_host host = {.user = &window, .query = window_query, .perform = window_perform};
    cui_title_bar_options o = {.width = 400, .host = &host, .name = {"Window", 6}};
    cui_title_bar_state state = {0};
    cui_theme theme = cui_theme_light();
    theme.title_bar = (cui_color){0, 0, 0, 255};
    theme.background = (cui_color){200, 100, 40, 255};
    cui_draw_list first = title_frame(ui, &state, o, &theme);
    unsigned icons = 0;
    for (size_t i = 0; i < first.count; ++i)
        if (first.commands[i].type == CUI_DRAW_IMAGE &&
            first.commands[i].bitmap.bitmap.height != 34) {
            near(first.commands[i].bounds.w, 14);
            near(first.commands[i].bounds.h, 14);
            icons++;
        }
    assert(icons == 3);
    near(cui_bounds(ui, 100).h, 34);
    near(cui_bounds(ui, 101).x, 12);
    cui_rect close = cui_bounds(ui, cui_title_bar_control_id(100, 2));
    near(close.x, 366);
    near(close.w, 34);
    near(close.h, 34);
    title_click(ui, close.x + 17, 17);
    title_frame(ui, &state, o, &theme);
    assert(window.actions[CUI_WINDOW_CLOSE] == 1 && !window.actions[CUI_WINDOW_MOVE]);
    o.on_close = window_close;
    o.close_user = &window;
    title_click(ui, close.x + 17, 17);
    title_frame(ui, &state, o, &theme);
    assert(window.closed == 1 && window.actions[CUI_WINDOW_CLOSE] == 1);
    cui_focus(ui, cui_title_bar_control_id(100, 1));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    title_frame(ui, &state, o, &theme);
    assert(window.actions[CUI_WINDOW_MAXIMIZE] == 1 && window.info.maximized);
    title_frame(ui, &state, o, &theme);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    title_frame(ui, &state, o, &theme);
    assert(window.actions[CUI_WINDOW_RESTORE] == 1 && !window.info.maximized);
    /* Empty title area owns drag; window controls do not bubble into it. */
    title_event(ui, CUI_EVENT_POINTER_DOWN, 200, 17, CUI_POINTER_PRIMARY);
    title_frame(ui, &state, o, &theme);
    assert(state.move_armed && cui_pressed(ui, 100));
    title_event(ui, CUI_EVENT_POINTER_MOVE, 210, 17, CUI_POINTER_PRIMARY);
    title_frame(ui, &state, o, &theme);
    title_event(ui, CUI_EVENT_POINTER_MOVE, 220, 17, CUI_POINTER_PRIMARY);
    title_frame(ui, &state, o, &theme);
    assert(window.actions[CUI_WINDOW_MOVE] == 1 && !state.move_armed);
    title_event(ui, CUI_EVENT_POINTER_UP, 220, 17, CUI_POINTER_PRIMARY);
    title_frame(ui, &state, o, &theme);
    title_click(ui, 200, 17);
    title_frame(ui, &state, o, &theme);
    title_click(ui, 201, 17);
    title_frame(ui, &state, o, &theme);
    assert(window.actions[CUI_WINDOW_TITLEBAR_DOUBLE_CLICK] == 1);
    title_event(ui, CUI_EVENT_POINTER_DOWN, 200, 17, CUI_POINTER_SECONDARY);
    title_frame(ui, &state, o, &theme);
    assert(window.actions[CUI_WINDOW_MENU] == 1);
    for (unsigned decoration = 0; decoration < 4; ++decoration) {
        window.info.decoration = (cui_window_decoration)decoration;
        window.info.fullscreen = true;
        title_frame(ui, &state, o, &theme);
        near(cui_bounds(ui, 101).x, decoration == CUI_WINDOW_NATIVE_TRAFFIC_LIGHTS ? 92
                                    : decoration == CUI_WINDOW_BROWSER             ? 12
                                                                                   : 24);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        assert(n == (decoration == CUI_WINDOW_CLIENT_DECORATED ? 5u : 2u));
        assert(sem[0].tab_index == -1 && sem[0].details.pointer_activatable);
    }
    assert(cui_begin_frame(ui, 600, 200, .05f));
    cui_title_bar(ui, 100, &theme, &state, NULL, 0, o);
    cui_draw_list draw = cui_end_frame(ui);
    const cui_draw_command *gradient = NULL;
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type == CUI_DRAW_IMAGE && draw.commands[i].bitmap.bitmap.height == 34)
            gradient = &draw.commands[i];
    assert(gradient);
    assert(gradient->bitmap.bitmap.pixels[0] == 90 && gradient->bitmap.bitmap.pixels[1] == 45 &&
           gradient->bitmap.bitmap.pixels[2] == 18 && gradient->bitmap.bitmap.pixels[33 * 4] == 0);
    cui_destroy(ui);
}
int main(void) {
    title_bar();
    assert(cui_status_bar_center_alignment(false, false) == CUI_ALIGN_START);
    assert(cui_status_bar_center_alignment(false, true) == CUI_ALIGN_START);
    assert(cui_status_bar_center_alignment(true, false) == CUI_ALIGN_END);
    assert(cui_status_bar_center_alignment(true, true) == CUI_ALIGN_CENTER);
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 200});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_chrome_item left = {.id = 10, .text = {"Left", 4}},
                    center = {.id = 20, .text = {"Center", 6}},
                    right = {.id = 30, .text = {"Right", 5}};
    for (unsigned t = 0; t < 4; ++t)
        for (unsigned mask = 0; mask < 4; ++mask)
            for (unsigned resize = 0; resize < 2; ++resize) {
                float width = resize ? 500 : 400, density = themes[t].font_size / 16;
                bool l = (mask & 1) != 0, r = (mask & 2) != 0;
                assert(cui_begin_frame(ui, 600, 200, .016f));
                assert(cui_status_bar(ui, 1, &themes[t], (cui_chrome_region){&left, l},
                                      (cui_chrome_region){&center, 1},
                                      (cui_chrome_region){&right, r},
                                      (cui_status_bar_options){.width = width}));
                cui_end_frame(ui);
                cui_rect a = cui_bounds(ui, 10), b = cui_bounds(ui, 20), c = cui_bounds(ui, 30);
                float start = l ? a.x + a.w + 8 * density : 8 * density;
                float end = r ? c.x - 8 * density : width - 8 * density;
                if (l && r)
                    near(b.x + b.w / 2, (start + end) / 2);
                else if (l)
                    near(b.x + b.w, end);
                else
                    near(b.x, start);
                if (l)
                    near(a.x, 8 * density);
                if (r)
                    near(c.x + c.w, width - 8 * density);
                near(cui_bounds(ui, 1).h, 8 * density + 1 + 12 * density * 1.618034f);
                size_t n;
                const cui_semantic *s = cui_semantics(ui, &n);
                assert(n == 2u + l + r);
                for (size_t i = 0; i < n; ++i)
                    assert(s[i].tab_index == -1);
            }
    center.render = control;
    center.user = &calls;
    assert(cui_begin_frame(ui, 600, 200, .016f));
    assert(cui_status_bar(ui, 1, themes, (cui_chrome_region){0}, (cui_chrome_region){&center, 1},
                          (cui_chrome_region){0}, (cui_status_bar_options){.width = 300}));
    cui_end_frame(ui);
    assert(calls == 1);
    near(cui_bounds(ui, 1).h, 34);
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    assert(n == 2 && s[1].role == CUI_ROLE_BUTTON && s[1].tab_index == 0);
    assert(cui_begin_frame(ui, 600, 200, .016f));
    right.id = left.id;
    assert(!cui_status_bar(ui, 1, themes, (cui_chrome_region){&left, 1}, (cui_chrome_region){0},
                           (cui_chrome_region){&right, 1}, (cui_status_bar_options){0}));
    cui_end_frame(ui);
    cui_destroy(ui);
    return 0;
}
