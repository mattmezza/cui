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
static cui_radio_group_event_result motion_frame(cui_radio_group_motion *motion,
                                                 const cui_radio_group_item *list, size_t count,
                                                 cui_radio_group_options options,
                                                 const cui_selection_indicator_parts *parts) {
    assert(cui_begin_frame(ui, 2000, 1000, .016f));
    cui_radio_group_event_result result = cui_radio_group_present(
        ui, 10, &theme, list, count, &selected, &state, options,
        (cui_radio_group_presentation){
            .motion = motion, .parts = parts, .part_count = parts ? count : 0},
        (cui_selection_handler){0});
    drawing = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_motion(void) {
    cui_radio_group_motion motion = {0};
    cui_selection_indicator_parts parts[] = {{101}, {102}, {103}};
    cui_radio_group_options options = {.vertical = true};
    selected = 11;
    assert(motion_frame(&motion, items, 3, options, parts).legacy.valid);
    assert(motion.items[0].progress == 1 && motion.items[1].progress == 0 &&
           motion.next_wake_seconds == 0);
    cui_rect before = cui_bounds(ui, 101);
    selected = 12;
    motion_frame(&motion, items, 3, options, parts);
    assert(motion.items[0].progress > 0 && motion.items[0].progress < 1);
    assert(motion.items[1].progress > 0 && motion.items[1].progress < 1);
    assert(motion.next_wake_seconds > cui_frame_time(ui));
    cui_rect after = cui_bounds(ui, 101);
    assert(before.w == after.w && before.h == after.h);
    unsigned marks = 0;
    for (size_t i = 0; i < drawing.count; i++)
        if (drawing.commands[i].type == CUI_DRAW_IMAGE) {
            assert(drawing.commands[i].bounds.w == 12);
            marks++;
        }
    assert(marks == 2);
    float previous = motion.items[0].progress;
    selected = 11;
    for (unsigned i = 0; i < 8; i++)
        motion_frame(&motion, items, 3, options, parts);
    assert(motion.items[0].progress > previous);
    cui_radio_group_item reordered[] = {items[1], items[0]};
    float retained = motion.items[0].progress;
    motion_frame(&motion, reordered, 2, options, NULL);
    assert(motion.items[1].id == 11 && motion.items[1].progress >= retained);
    assert(motion.items[2].id == 0 && !motion.items[2].initialized);
    cui_set_reduced_motion(ui, true);
    selected = 12;
    motion_frame(&motion, reordered, 2, options, NULL);
    assert(motion.items[0].progress == 1 && motion.items[1].progress == 0 &&
           motion.next_wake_seconds == 0);
    cui_set_reduced_motion(ui, false);
    selected = 11;
    for (unsigned i = 0; i < 200; i++)
        motion_frame(&motion, reordered, 2, options, NULL);
    assert(motion.items[1].progress == 1 && motion.items[0].progress == 0 &&
           motion.next_wake_seconds == 0);
    options.on_change = change;
    options.change_user = &selected;
    accept = false;
    calls = 0;
    cui_focus(ui, 12);
    key(CUI_KEY_ENTER);
    cui_radio_group_event_result rejected = motion_frame(&motion, items, 3, options, parts);
    assert(rejected.legacy.requested && !rejected.legacy.changed && calls == 1 && selected == 11);
    assert(motion.items[0].progress == 1 && motion.items[1].progress == 0);
    accept = true;
    key(CUI_KEY_ENTER);
    cui_radio_group_event_result accepted = motion_frame(&motion, items, 3, options, parts);
    assert(accepted.legacy.changed && calls == 2 && selected == 12);
    assert(motion.items[0].progress < 1 && motion.items[1].progress > 0);
    motion.items[0].progress = NAN;
    motion.items[0].velocity = NAN;
    motion_frame(&motion, items, 3, options, parts);
    assert(isfinite(motion.items[0].progress) && isfinite(motion.items[0].velocity));
    motion.id = 999; /* Storage reused for another group snaps instead of inheriting travel. */
    motion_frame(&motion, items, 3, options, parts);
    assert(motion.id == 10 && motion.items[0].progress == 0 && motion.items[1].progress == 1 &&
           motion.next_wake_seconds == 0);
    options.disabled = true;
    motion_frame(&motion, items, 3, options, parts);
    bool disabled_mark = false;
    for (size_t i = 0; i < drawing.count; i++)
        if (drawing.commands[i].type == CUI_DRAW_IMAGE) {
            assert(drawing.commands[i].bitmap.tint.a == theme.on_primary.a / 2);
            disabled_mark = true;
        }
    assert(disabled_mark);
    motion_frame(&motion, items, 0, options, NULL);
    assert(motion.items[0].id == 0 && motion.next_wake_seconds == 0);
}
static cui_radio_group_content rich_content[3];
static unsigned content_calls;
static void decoration(cui_context *ctx, void *user) {
    assert(ctx == ui && user == &content_calls);
    content_calls++;
    assert(!cui_button(ctx, 301, &theme, cui_string("Tracked"), false));
}
static void change_and_replace_content(void *user, cui_id id) {
    change(user, id);
    rich_content[1] = (cui_radio_group_content){0};
}
static cui_radio_group_result rich_frame(cui_radio_group_options options, size_t content_count) {
    assert(cui_begin_frame(ui, 450, 280, .016f));
    cui_radio_group_result result =
        cui_radio_group_present_with_content(ui, 10, &theme, items, 3, &selected, &state, options,
                                             (cui_radio_group_presentation){0}, rich_content,
                                             content_count, (cui_selection_handler){0})
            .legacy;
    drawing = cui_end_frame(ui);
    return result;
}
static void check_description(cui_str expected) {
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 4 && sem[2].id == 12 && sem[2].role == CUI_ROLE_RADIO &&
           sem[2].name.length == 8 && !memcmp(sem[2].name.data, "Priority", 8));
    assert(sem[2].details.parent == 10 && sem[2].details.pos_in_set == 2 &&
           sem[2].details.set_size == 3 && sem[2].details.required &&
           sem[2].checked == (selected == 12));
    assert(sem[2].details.description.length == expected.length);
    if (expected.length)
        assert(!memcmp(sem[2].details.description.data, expected.data, expected.length));
}
static void test_rich_content(void) {
    char description[] = "One business day";
    rich_content[1] = (cui_radio_group_content){.description = cui_string(description),
                                                .content = decoration,
                                                .content_user = &content_calls,
                                                .max_width = 135};
    selected = 11;
    cui_radio_group_options options = {.vertical = true, .required = true};
    assert(rich_frame(options, 3).valid && cui_last_error(ui) == CUI_OK);
    description[0] = 'X';
    check_description(cui_string("One business day"));
    description[0] = 'O';
    cui_rect row = cui_bounds(ui, 12), child = cui_bounds(ui, 301);
    assert(row.w <= 16 + 8 + 135 && row.h > cui_bounds(ui, 11).h);
    assert(child.w > 0 && child.h > 0 && child.y > row.y && child.y + child.h <= row.y + row.h);
    assert(content_calls == 1);

    calls = 0;
    accept = true;
    options.on_change = change_and_replace_content;
    options.change_user = &selected;
    float x = child.x + child.w / 2, y = child.y + child.h / 2;
    touch(CUI_EVENT_POINTER_DOWN, x, y);
    touch(CUI_EVENT_POINTER_UP, x, y);
    cui_radio_group_result result = rich_frame(options, 3);
    assert(cui_last_error(ui) == CUI_OK && result.valid && result.changed && selected == 12 &&
           calls == 1 && content_calls == 2);
    check_description(cui_string("One business day"));
    assert(rich_frame(options, 3).valid && content_calls == 2);
    check_description((cui_str){0});
    assert(cui_bounds(ui, 12).h < row.h);

    rich_content[1].max_width = NAN;
    assert(!rich_frame(options, 3).valid && calls == 1 && content_calls == 2);
    rich_content[1].max_width = 0;
    assert(!rich_frame(options, 2).valid && cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(ui);
    assert(rich_frame(options, 3).valid && cui_last_error(ui) == CUI_OK);
}

static cui_radio_group_result hint_frame(cui_radio_group_content *content, bool inert) {
    assert(cui_begin_frame(ui, 260, 140, .016f));
    if (inert)
        cui_begin_box(ui, 0, (cui_box){.inert = true});
    cui_radio_group_result result =
        cui_radio_group_present_with_content(ui, 10, &theme, items, 3, &selected, &state,
                                             (cui_radio_group_options){.vertical = true},
                                             (cui_radio_group_presentation){0}, content,
                                             content ? 3 : 0, (cui_selection_handler){0})
            .legacy;
    if (inert)
        cui_end_box(ui);
    drawing = cui_end_frame(ui);
    return result;
}

static void test_radio_group_tooltip(void) {
    cui_radio_tooltip_state hint_state = {0};
    cui_radio_tooltip hint = {.id = 401,
                              .state = &hint_state,
                              .text = cui_string("Track online"),
                              .options = {.viewport = {0, 0, 260, 140},
                                          .side = CUI_POPUP_RIGHT,
                                          .width = 110,
                                          .open_delay = .04f}};
    cui_radio_group_content content[3] = {
        [1] = {.description = cui_string("Arrives Wednesday"), .tooltip = &hint}};
    cui_focus(ui, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_LEAVE}));
    selected = 11;
    assert(hint_frame(content, false).valid && !hint_state.hover.open);
    cui_rect row = cui_bounds(ui, 12);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = row.x + row.w / 2,
                                           .y = row.y + row.h / 2}));
    assert(hint_frame(content, false).valid && hint_state.hover.pending && !hint_state.hover.open &&
           cui_needs_redraw(ui));
    assert(hint_frame(content, false).valid && !hint_state.hover.open);
    assert(hint_frame(content, false).valid && hint_state.hover.open);
    cui_rect panel = cui_bounds(ui, hint.id);
    assert(panel.w > 0 && panel.x >= row.x + row.w && panel.x + panel.w <= 260 && panel.y >= 0 &&
           panel.y + panel.h <= 140);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 5 && sem[2].id == 12 && !sem[2].checked && sem[2].tab_index < 0);
    assert(sem[2].details.description.length == 17 &&
           !memcmp(sem[2].details.description.data, "Arrives Wednesday", 17));
    assert(sem[4].id == hint.id && sem[4].role == CUI_ROLE_LABEL && sem[4].details.parent == 12 &&
           sem[4].tab_index < 0);
    assert(selected == 11);
    key(CUI_KEY_ESCAPE);
    assert(hint_frame(content, false).valid && hint_state.hover.suppressed &&
           !hint_state.hover.open);
    assert(hint_frame(content, false).valid && !hint_state.hover.open);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_LEAVE}));
    assert(hint_frame(content, false).valid && !hint_state.hover.suppressed);
    cui_focus(ui, 12);
    assert(hint_frame(content, false).valid && hint_state.hover.pending);
    assert(hint_frame(content, false).valid && !hint_state.hover.open);
    assert(hint_frame(content, false).valid && hint_state.hover.open);
    assert(hint_frame(content, true).valid && cui_bounds(ui, hint.id).w == 0);
    assert(hint_frame(content, false).valid && !hint_state.hover.open);
    assert(hint_frame(NULL, false).valid && cui_bounds(ui, hint.id).w == 0);

    cui_focus(ui, 12);
    key(CUI_KEY_ENTER);
    hint.id = 11;
    assert(!hint_frame(content, false).valid && selected == 11 &&
           cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(ui);
    hint.id = 401;
    content[0].tooltip = &hint;
    assert(!hint_frame(content, false).valid && selected == 11);
    cui_clear_error(ui);
    content[0].tooltip = NULL;
    hint.options.open_delay = 0;
    assert(hint_frame(content, false).valid);
    cui_focus(ui, 12);
    key(CUI_KEY_ENTER);
    assert(hint_frame(content, false).changed && selected == 12 && hint_state.hover.open);
    items[1].disabled = true;
    assert(hint_frame(content, false).valid && !hint_state.hover.open &&
           cui_bounds(ui, hint.id).w == 0 && selected == 12);
    items[1].disabled = false;
    hint.options.viewport.w = 0;
    cui_focus(ui, 12);
    for (unsigned i = 0; i < 3; i++)
        assert(hint_frame(content, false).valid && !hint_state.hover.open &&
               !hint_state.hover.pending);
    assert(!cui_needs_redraw(ui));
}
static const cui_semantic *tab_semantic(cui_id id) {
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (semantics[i].id == id)
            return &semantics[i];
    assert(!"missing tab order semantic");
    return NULL;
}

static cui_radio_group_result tab_order_frame(cui_radio_presentation standalone,
                                              cui_radio_options standalone_options,
                                              cui_radio_group_presentation presentation,
                                              cui_radio_group_options options) {
    int other = 0;
    assert(cui_begin_frame(ui, 500, 300, .016f));
    cui_radio_present(ui, 201, &theme, cui_string("Account default"), &other, 1, standalone_options,
                      standalone);
    cui_radio_ex(ui, 202, &theme, cui_string("Legacy option"), &other, 2,
                 (cui_radio_options){.tab_index = -99});
    cui_radio_group_event_result result =
        cui_radio_group_present(ui, 10, &theme, items, 3, &selected, &state, options, presentation,
                                (cui_selection_handler){0});
    cui_end_frame(ui);
    assert(result.legacy.valid && cui_last_error(ui) == CUI_OK);
    return result.legacy;
}

static void test_tab_order(void) {
    cui_radio_presentation standalone = {.custom_tab_index = true, .tab_index = 3};
    cui_radio_options standalone_options = {0};
    cui_radio_group_presentation presentation = {.custom_tab_index = true, .tab_index = -2};
    cui_radio_group_options options = {
        .vertical = true, .on_change = change, .change_user = &selected};
    selected = 11;
    state = (cui_radio_group_state){0};
    calls = 0;
    accept = false;
    cui_focus(ui, 0);
    assert(tab_order_frame(standalone, standalone_options, presentation, options).tab_stop == 11);
    assert(!tab_semantic(202)->tab_ordered && tab_semantic(202)->tab_index == 0);
    assert(!tab_semantic(11)->tab_stop_disabled && tab_semantic(11)->tab_ordered &&
           tab_semantic(11)->tab_index == -2);
    assert(tab_semantic(12)->tab_stop_disabled && tab_semantic(12)->tab_index == -2);
    key(CUI_KEY_TAB);
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 11 && calls == 0);
    key(CUI_KEY_DOWN);
    cui_radio_group_result result =
        tab_order_frame(standalone, standalone_options, presentation, options);
    assert(result.tab_stop == 12 && result.requested && !result.changed && selected == 11);
    assert(calls == 1 && cui_focused(ui) == 12 && tab_semantic(11)->tab_stop_disabled);
    assert(!tab_semantic(12)->tab_stop_disabled && tab_semantic(12)->tab_index == -2);
    key(CUI_KEY_TAB);
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 202);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB, .modifiers = CUI_MOD_SHIFT}));
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 11 && calls == 1);

    options.skip_tab = true;
    assert(!tab_order_frame(standalone, standalone_options, presentation, options).tab_stop);
    assert(tab_semantic(11)->tab_stop_disabled && tab_semantic(11)->tab_index == -2);
    key(CUI_KEY_TAB);
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 202);
    key(CUI_KEY_TAB);
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 201);

    standalone.tab_index = -4;
    options.skip_tab = false;
    cui_focus(ui, 0);
    tab_order_frame(standalone, standalone_options, presentation, options);
    key(CUI_KEY_TAB);
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 201 && tab_semantic(201)->tab_index == -4);
    standalone_options.skip_tab = true;
    options.disabled = true;
    cui_focus(ui, 0);
    assert(!tab_order_frame(standalone, standalone_options, presentation, options).tab_stop);
    key(CUI_KEY_TAB);
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 202 && tab_semantic(201)->tab_stop_disabled);

    standalone.custom_tab_index = false;
    standalone_options.skip_tab = false;
    presentation.tab_index = 0;
    options.disabled = false;
    cui_focus(ui, 0);
    tab_order_frame(standalone, standalone_options, presentation, options);
    key(CUI_KEY_TAB);
    tab_order_frame(standalone, standalone_options, presentation, options);
    assert(cui_focused(ui) == 201); /* Equal indices follow submission order. */
    assert(calls == 1);
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
    theme = cui_theme_light();
    test_motion();
    test_rich_content();
    test_radio_group_tooltip();
    test_tab_order();
    cui_destroy(ui);
    return 0;
}
