/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "cui/cui_virtual_scroll.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    size_t calls, indices[32];
    uint64_t keys[32];
    cui_id ids[32];
    cui_size sizes[32];
    bool child, defer;
} row_probe;
static void row(cui_context *ui, const cui_theme *t, void *user, size_t index, uint64_t key,
                cui_id item, cui_size size) {
    row_probe *p = user;
    assert(t && p->calls < 32);
    p->indices[p->calls] = index;
    p->keys[p->calls] = key;
    p->ids[p->calls] = item;
    p->sizes[p->calls] = size;
    ++p->calls;
    assert(cui_set_underlay(ui, (cui_rect){0, 0, size.width, size.height},
                            (cui_color){(uint8_t)(30 + index), 100, 150, 255},
                            (cui_corner_radii){0}));
    if (p->child && index == 0) {
        cui_begin_box(ui, 30, (cui_box){.width = 50, .height = 20});
        cui_interact(ui, 30, CUI_ROLE_GROUP, cui_string("Child"), false, false, 0);
        cui_set_semantic_details(
            ui, 30,
            (cui_semantic_details){.parent = item, .scrollable = true, .scroll_max_y = 100});
        cui_end_box(ui);
        if (p->defer)
            assert(cui_defer_scroll_event(ui, 0, 30));
    }
}
static cui_virtual_widget_result frame(cui_context *ui, const cui_theme *theme, cui_virtual_list *v,
                                       cui_virtual_widget_state *s, cui_virtual_widget_options o,
                                       row_probe *p, cui_draw_list *draw) {
    p->calls = 0;
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_virtual_widget_result r = cui_virtual_list_draw(ui, 10, theme, v, s, o, row, p);
    cui_draw_list d = cui_end_frame(ui);
    if (draw)
        *draw = d;
    assert(cui_last_error(ui) == CUI_OK);
    return r;
}
static cui_virtual_list *model(void) {
    uint64_t keys[] = {0, 101, 102, 103, 104, 105, 106, 107};
    double heights[] = {20, 30, 10, 40, 25, 15, 50, 10};
    cui_virtual_list *v = cui_virtual_create((cui_virtual_config){16, 20});
    assert(v && cui_virtual_rebuild(v, keys, heights, 8) == CUI_VIRTUAL_OK);
    return v;
}
static void key(cui_context *ui, cui_key code) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code}));
}
static void layout_and_wheel(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; ++theme)
        for (unsigned horizontal = 0; horizontal < 2; ++horizontal) {
            cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
            cui_virtual_list *v = model();
            cui_virtual_widget_state s = {0};
            row_probe p = {0};
            cui_virtual_widget_options o = {
                .scroll = {.width = 100, .height = 60, .hide_scrollbars = true},
                .horizontal = horizontal != 0};
            cui_draw_list d;
            cui_virtual_widget_result r = frame(ui, &themes[theme], v, &s, o, &p, &d);
            assert(r.error == CUI_VIRTUAL_OK && p.calls == (horizontal ? 4U : 3U) &&
                   r.view.begin == 0 && r.view.end == p.calls);
            assert(p.keys[0] == 0 && p.ids[0] == cui_virtual_item_id(10, 0));
            assert(p.sizes[1].width == (horizontal ? 30 : 100) &&
                   p.sizes[1].height == (horizontal ? 60 : 30));
            cui_rect first = cui_bounds(ui, p.ids[0]), second = cui_bounds(ui, p.ids[1]);
            assert(first.x == 0 && first.y == 0);
            assert((horizontal ? second.x : second.y) == 20);
            cui_software *renderer = cui_software_create(400, 300, NULL);
            assert(renderer);
            assert(cui_software_render(renderer, &d, (cui_color){0, 0, 0, 255}));
            const unsigned char *pixels = cui_software_pixels(renderer, NULL, NULL);
            assert(pixels[(size_t)5 * 400 * 4 + 5 * 4] == 30);
            assert(pixels[(size_t)70 * 400 * 4 + 5 * 4] == 0 &&
                   pixels[(size_t)5 * 400 * 4 + 110 * 4] == 0);
            cui_software_destroy(renderer);
            size_t n;
            const cui_semantic *sem = cui_semantics(ui, &n);
            assert(n == p.calls + 1 && sem[0].details.focusable && sem[0].details.scrollable);
            assert(sem[1].role == CUI_ROLE_LISTITEM && sem[1].details.parent == 10 &&
                   sem[1].details.pos_in_set == 1 && sem[1].details.set_size == 8);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                                   .x = 5,
                                                   .y = 5,
                                                   .dx = horizontal ? -.5f : 0,
                                                   .dy = horizontal ? 0 : -.5f,
                                                   .scroll_precise = true}));
            r = frame(ui, &themes[theme], v, &s, o, &p, NULL);
            assert(r.scroll_changed && (horizontal ? s.scroll.x : s.scroll.y) == .5f);
            first = cui_bounds(ui, cui_virtual_item_id(10, 0));
            assert(fabsf((horizontal ? first.x : first.y) + .5f) < .001f);
            o.overscan = 1;
            s.scroll.x = horizontal ? 60 : 0;
            s.scroll.y = horizontal ? 0 : 60;
            r = frame(ui, &themes[theme], v, &s, o, &p, NULL);
            assert(r.view.begin == 2 && p.indices[0] == 2 && p.calls < 8);
            for (size_t i = 0; i < p.calls; ++i)
                assert(p.ids[i] == cui_virtual_item_id(10, p.keys[i]));
            uint64_t reorder[] = {0, 102, 101, 103, 104, 105, 106, 107};
            assert(cui_virtual_rebuild(v, reorder, NULL, 8) == CUI_VIRTUAL_OK);
            s.scroll.x = s.scroll.y = 0;
            o.overscan = 0;
            frame(ui, &themes[theme], v, &s, o, &p, NULL);
            assert(p.ids[1] == cui_virtual_item_id(10, 102) &&
                   p.ids[2] == cui_virtual_item_id(10, 101));
            assert(cui_virtual_height(v, 1) == 10 && cui_virtual_height(v, 2) == 30);
            cui_destroy(ui);
            cui_virtual_destroy(v);
        }
}
static void keyboard_and_scrollbars(void) {
    cui_theme t = cui_theme_light();
    for (unsigned horizontal = 0; horizontal < 2; ++horizontal) {
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
        cui_virtual_list *v = model();
        cui_virtual_widget_state s = {0};
        row_probe p = {0};
        cui_virtual_widget_options o = {.scroll = {.width = 100, .height = 60, .wheel_step = 7},
                                        .horizontal = horizontal != 0};
        frame(ui, &t, v, &s, o, &p, NULL);
        cui_focus(ui, 10);
        key(ui, CUI_KEY_END);
        frame(ui, &t, v, &s, o, &p, NULL);
        float maximum = horizontal ? 100 : 140;
        assert((horizontal ? s.scroll.x : s.scroll.y) == maximum);
        assert(p.indices[p.calls - 1] == 7);
        key(ui, CUI_KEY_HOME);
        frame(ui, &t, v, &s, o, &p, NULL);
        assert((horizontal ? s.scroll.x : s.scroll.y) == 0);
        key(ui, horizontal ? CUI_KEY_RIGHT : CUI_KEY_DOWN);
        frame(ui, &t, v, &s, o, &p, NULL);
        assert((horizontal ? s.scroll.x : s.scroll.y) == 7);
        key(ui, CUI_KEY_PAGEDOWN);
        frame(ui, &t, v, &s, o, &p, NULL);
        assert((horizontal ? s.scroll.x : s.scroll.y) == (horizontal ? 100 : 67));
        cui_focus(ui, cui_scrollbar_id(10, !horizontal));
        key(ui, CUI_KEY_END);
        frame(ui, &t, v, &s, o, &p, NULL);
        assert((horizontal ? s.scroll.x : s.scroll.y) == maximum);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        bool found = false;
        for (size_t i = 0; i < count; ++i)
            if (sem[i].role == CUI_ROLE_SCROLLBAR) {
                assert(sem[i].value == maximum);
                found = true;
            }
        assert(found);
        o.scroll.disabled = true;
        key(ui, CUI_KEY_HOME);
        frame(ui, &t, v, &s, o, &p, NULL);
        assert((horizontal ? s.scroll.x : s.scroll.y) == maximum);
        cui_virtual_widget_state saved = s;
        o.scroll.width = NAN;
        cui_virtual_widget_result r = frame(ui, &t, v, &s, o, &p, NULL);
        assert(r.error == CUI_VIRTUAL_INVALID && p.calls == 0 && !memcmp(&saved, &s, sizeof(s)));
        cui_destroy(ui);
        cui_virtual_destroy(v);
    }
}
static void deferred_and_empty(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_virtual_list *v = model();
    cui_theme t = cui_theme_light();
    cui_virtual_widget_state s = {0};
    row_probe p = {.child = true};
    cui_virtual_widget_options o = {
        .scroll = {.width = 100, .height = 60, .hide_scrollbars = true}};
    frame(ui, &t, v, &s, o, &p, NULL);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_SCROLL, .x = 5, .y = 5, .dy = -.5f, .scroll_precise = true}));
    p.defer = true;
    cui_virtual_widget_result r = frame(ui, &t, v, &s, o, &p, NULL);
    assert(r.window_pending && r.scroll_changed && s.scroll.y == .5f && cui_needs_redraw(ui));
    cui_rect first = cui_bounds(ui, cui_virtual_item_id(10, 0));
    assert(fabsf(first.y + .5f) < .001f);
    p.defer = false;
    r = frame(ui, &t, v, &s, o, &p, NULL);
    assert(!r.window_pending && s.scroll.y == .5f);
    assert(cui_virtual_rebuild(v, NULL, NULL, 0) == CUI_VIRTUAL_OK);
    r = frame(ui, &t, v, &s, o, &p, NULL);
    assert(r.error == CUI_VIRTUAL_OK && p.calls == 0 && s.scroll.y == 0 && r.view.begin == 0 &&
           r.view.end == 0);
    cui_destroy(ui);
    cui_virtual_destroy(v);
}
static void cross_axis_and_large_offsets(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_virtual_list *v = model();
    cui_theme t = cui_theme_light();
    cui_virtual_widget_state state = {0};
    row_probe p = {0};
    cui_virtual_widget_options o = {
        .scroll = {.width = 100, .height = 60, .horizontal = true, .content_width = 250}};
    cui_virtual_widget_result r = frame(ui, &t, v, &state, o, &p, NULL);
    assert(r.error == CUI_VIRTUAL_OK && p.calls == 2 && p.sizes[0].width == 250);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_SCROLL, .x = 5, .y = 5, .dx = -.5f, .scroll_precise = true}));
    frame(ui, &t, v, &state, o, &p, NULL);
    assert(state.scroll.x == .5f && state.scroll.y == 0);
    assert(cui_bounds(ui, cui_virtual_item_id(10, 0)).x == -.5f);
    cui_scroll_options resolved = {.width = 777};
    o.scroll.gap = 1;
    assert(!cui_virtual_widget_scroll_options(v, o, &resolved) && resolved.width == 777);
    cui_virtual_widget_state saved = state;
    r = frame(ui, &t, v, &state, o, &p, NULL);
    assert(r.error == CUI_VIRTUAL_INVALID && p.calls == 0 &&
           !memcmp(&saved, &state, sizeof(state)));
    o.scroll.gap = 0;
    o.no_item_semantics = true;
    frame(ui, &t, v, &state, o, &p, NULL);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; ++i)
        assert(sem[i].role != CUI_ROLE_LISTITEM);
    uint64_t keys[] = {1, 2, 3, 4, 5, 6};
    double heights[] = {1000000000, 24, 24, 24, 24, 24};
    assert(cui_virtual_rebuild(v, keys, heights, 6) == CUI_VIRTUAL_OK);
    o = (cui_virtual_widget_options){
        .scroll = {.width = 100, .height = 60, .hide_scrollbars = true}};
    state = (cui_virtual_widget_state){.scroll = {.y = 1000000000}};
    r = frame(ui, &t, v, &state, o, &p, NULL);
    assert(r.error == CUI_VIRTUAL_OK && r.view.begin == 1 && r.view.end == 4 && p.calls == 3);
    assert(cui_bounds(ui, p.ids[0]).y == 0 && cui_bounds(ui, p.ids[1]).y == 24 &&
           cui_bounds(ui, p.ids[2]).y == 48);
    /* Renderer-local positions preserve these small row differences even though
     * a float global offset cannot represent every pixel near one billion. */
    cui_destroy(ui);
    cui_virtual_destroy(v);
}
typedef struct {
    size_t calls, index;
    float available;
    bool fail;
} measure_probe;
static bool measure(void *user, size_t index, uint64_t key, float available, cui_size *out) {
    measure_probe *p = user;
    ++p->calls;
    p->index = index;
    p->available = available;
    assert(key == 107);
    *out = (cui_size){80, 35};
    return !p->fail;
}
static void intrinsic_sizing(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_theme t = cui_theme_light();
    cui_virtual_list *v = model();
    cui_virtual_widget_state state = {0};
    row_probe rows = {0};
    measure_probe p = {0};
    cui_virtual_widget_options o = {.scroll = {.hide_scrollbars = true},
                                    .sizing = CUI_VIRTUAL_SIZE_INFER,
                                    .measure = measure,
                                    .measure_user = &p,
                                    .measure_index = 999};
    assert(frame(ui, &t, v, &state, o, &rows, NULL).error == CUI_VIRTUAL_OK);
    cui_rect bounds = cui_bounds(ui, 10);
    assert(bounds.w == 80 && bounds.h == 200 && rows.calls == 8);
    assert(p.calls == 1 && p.index == 7 && p.available == 0);
    o.available = (cui_size){100, 60};
    assert(frame(ui, &t, v, &state, o, &rows, NULL).error == CUI_VIRTUAL_OK);
    bounds = cui_bounds(ui, 10);
    assert(bounds.w == 100 && bounds.h == 60 && p.available == 80);
    o.scroll.width = 120;
    o.scroll.height = 40;
    frame(ui, &t, v, &state, o, &rows, NULL);
    bounds = cui_bounds(ui, 10);
    assert(bounds.w == 120 && bounds.h == 40 && p.available == 100);
    o.horizontal = true;
    o.scroll.width = o.scroll.height = 0;
    o.available = (cui_size){0};
    frame(ui, &t, v, &state, o, &rows, NULL);
    bounds = cui_bounds(ui, 10);
    assert(bounds.w == 200 && bounds.h == 35 && p.available == 0);
    cui_virtual_widget_state saved = state;
    p.fail = true;
    assert(frame(ui, &t, v, &state, o, &rows, NULL).error == CUI_VIRTUAL_INVALID);
    assert(!memcmp(&state, &saved, sizeof(state)) && rows.calls == 0);
    p.fail = false;
    o.available.width = NAN;
    size_t calls = p.calls;
    assert(frame(ui, &t, v, &state, o, &rows, NULL).error == CUI_VIRTUAL_INVALID);
    assert(p.calls == calls && !memcmp(&state, &saved, sizeof(state)));
    o.available.width = 0;
    assert(cui_virtual_rebuild(v, NULL, NULL, 0) == CUI_VIRTUAL_OK);
    assert(frame(ui, &t, v, &state, o, &rows, NULL).error == CUI_VIRTUAL_OK);
    assert(p.calls == calls && rows.calls == 0);
    cui_destroy(ui);
    cui_virtual_destroy(v);
}
int main(void) {
    intrinsic_sizing();
    cross_axis_and_large_offsets();
    layout_and_wheel();
    keyboard_and_scrollbars();
    deferred_and_empty();
    puts("styled virtual list passed");
    return 0;
}
