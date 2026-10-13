/* SPDX-License-Identifier: MIT */
#include "cui/cui_radio_group.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_radio_group_item items[] = {
    {11, {"Standard", 8}, false}, {12, {"Priority", 8}, false}, {13, {"Express", 7}, true}};
static cui_context *ui;
static cui_theme theme;
static cui_id selected;
static cui_radio_group_state state;
static unsigned calls;
static bool accept;
static cui_id requested;
static cui_draw_list drawing;
static void change(void *user, cui_id id) {
    assert(user == &selected);
    calls++;
    requested = id;
    if (accept)
        selected = id;
}
static cui_radio_group_result frame(size_t count, cui_radio_group_options o) {
    assert(cui_begin_frame(ui, 2000, 1000, .016f));
    cui_radio_group_result r = cui_radio_group(ui, 10, &theme, items, count, &selected, &state, o);
    cui_draw_list d = cui_end_frame(ui);
    drawing = d;
    assert(cui_last_error(ui) == CUI_OK);
    if (r.valid) {
        size_t n;
        const cui_semantic *s = cui_semantics(ui, &n);
        assert(n == count + 1 && s[0].role == CUI_ROLE_RADIOGROUP);
        unsigned checked = 0, marks = 0, tab_stops = 0;
        for (size_t i = 0; i < count; i++) {
            assert(s[i + 1].role == CUI_ROLE_RADIO &&
                   s[i + 1].checked == (selected == items[i].id));
            assert(s[i + 1].details.parent == 10 && s[i + 1].details.pos_in_set == i + 1 &&
                   s[i + 1].details.set_size == count);
            checked += s[i + 1].checked;
            tab_stops += s[i + 1].tab_index == 0;
            assert((s[i + 1].tab_index == 0) == (items[i].id == r.tab_stop));
        }
        for (size_t i = 0; i < d.count; i++)
            if (d.commands[i].type == CUI_DRAW_IMAGE)
                marks++;
        assert(checked <= 1 && marks == checked &&
               tab_stops == (r.tab_stop != 0)); /* Same-frame selected paint and snapshots. */
    }
    return r;
}
static void key(cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void touch(cui_event_type type, float x, float y) {
    assert(cui_queue_event(
        ui,
        (cui_event){
            .type = type, .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 42, .x = x, .y = y}));
}
int main(void) {
    ui = cui_create((cui_config){.width = 2000, .height = 1000});
    assert(ui);
    theme = cui_theme_light();
    selected = 11;
    cui_radio_group_options o = {.name = {"Delivery", 8}, .vertical = true};
    frame(3, o);
    key(CUI_KEY_TAB);
    frame(3, o);
    assert(cui_focused(ui) == 11);
    key(CUI_KEY_DOWN);
    assert(frame(3, o).changed && selected == 12 && cui_focused(ui) == 12);
    key(CUI_KEY_DOWN);
    frame(3, o);
    assert(selected == 11); /* Skip disabled and wrap. */
    key(CUI_KEY_END);
    frame(3, o);
    assert(selected == 12);
    key(CUI_KEY_HOME);
    frame(3, o);
    assert(selected == 11);
    key(CUI_KEY_UP);
    frame(3, o);
    assert(selected == 12);
    key(CUI_KEY_LEFT);
    frame(3, o);
    assert(selected == 11);
    key(CUI_KEY_RIGHT);
    frame(3, o);
    assert(selected == 12);
    key(CUI_KEY_DOWN);
    key(CUI_KEY_DOWN);
    frame(3, o);
    assert(selected == 12); /* Both queued arrows advance in reducer order. */
    /* One tab stop: Tab leaves this only group rather than visiting its sibling. */
    key(CUI_KEY_TAB);
    frame(3, o);
    assert(cui_focused(ui) == 12);
    o.on_change = change;
    o.change_user = &selected;
    accept = false;
    calls = 0;
    key(CUI_KEY_HOME);
    cui_radio_group_result r = frame(3, o);
    assert(r.requested && !r.changed && requested == 11 && calls == 1 && selected == 12 &&
           cui_focused(ui) == 11);
    key(CUI_KEY_ENTER);
    frame(3, o);
    assert(calls == 2 && selected == 12);
    accept = true;
    key(CUI_KEY_ENTER);
    r = frame(3, o);
    assert(r.changed && calls == 3 && selected == 11);
    key(CUI_KEY_ENTER);
    frame(3, o);
    assert(calls == 3); /* No duplicate selected callback. */
    cui_rect b = cui_bounds(ui, 12);
    touch(CUI_EVENT_POINTER_DOWN, b.x + 5, b.y + 5);
    touch(CUI_EVENT_POINTER_UP, b.x + 5, b.y + 5);
    frame(3, o);
    assert(calls == 4 && selected == 12);
    b = cui_bounds(ui, 11);
    touch(CUI_EVENT_POINTER_DOWN, b.x + 5, b.y + 5);
    touch(CUI_EVENT_POINTER_MOVE, b.x + 25, b.y + 5);
    touch(CUI_EVENT_POINTER_UP, b.x + 5, b.y + 5);
    frame(3, o);
    assert(calls == 4 && selected == 12);
    touch(CUI_EVENT_POINTER_DOWN, b.x + 5, b.y + 5);
    touch(CUI_EVENT_CANCEL, b.x + 5, b.y + 5);
    touch(CUI_EVENT_POINTER_UP, b.x + 5, b.y + 5);
    frame(3, o);
    assert(calls == 4);
    o.disabled = true;
    key(CUI_KEY_HOME);
    frame(3, o);
    assert(calls == 4 && cui_focused(ui) == 0);
    o.disabled = false;
    frame(3, o);
    cui_focus(ui, 12);
    frame(3, o);
    items[1].disabled = true;
    frame(3, o);
    assert(cui_focused(ui) == 11 && selected == 12 && calls == 4);
    items[1].disabled = false;
    cui_focus(ui, 12);
    frame(3, o);
    frame(1, o);
    assert(cui_focused(ui) == 11 && selected == 12 &&
           calls == 4); /* Removed model remains caller-owned. */
    frame(0, o);
    assert(!cui_focused(ui));
    o = (cui_radio_group_options){0};
    selected = 11;
    for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; size++)
        for (unsigned density = 0; density < 2; density++) {
            theme = cui_theme_light();
            theme.font_size = density ? 24 : 16;
            o.design_size = (cui_design_size)size;
            assert(frame(3, o).valid);
            cui_rect a = cui_bounds(ui, 11), b2 = cui_bounds(ui, 12);
            assert(a.w > 0 && a.h > 0 && b2.x >= a.x + a.w);
            cui_design_metrics expected;
            assert(cui_design_measure(o.design_size, theme.font_size / 16, &expected));
            bool indicator = false, label = false;
            for (size_t n = 0; n < drawing.count; n++) {
                const cui_draw_command *d = &drawing.commands[n];
                if (d->type == CUI_DRAW_IMAGE) {
                    assert(fabsf(d->bounds.w -
                                 (expected.indicator_size - 4 * theme.font_size / 16)) < .01f);
                    indicator = true;
                }
                if (d->type == CUI_DRAW_TEXT) {
                    assert(d->font_size == expected.font_size);
                    label = true;
                }
            }
            assert(indicator && label);
        }
    cui_focus(ui, 12);
    frame(3, o);
    key(CUI_KEY_SPACE);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    assert(frame(3, o).changed && selected == 12);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB, .modifiers = CUI_MOD_SHIFT}));
    frame(3, o);
    assert(cui_focused(ui) == 12);
    o.gap = NAN;
    assert(!frame(3, o).valid);
    o.gap = 0;
    cui_id saved = items[1].id;
    items[1].id = 11;
    assert(!frame(3, o).valid);
    items[1].id = saved;
    cui_destroy(ui);
    return 0;
}
