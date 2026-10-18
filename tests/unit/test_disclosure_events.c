/* SPDX-License-Identifier: MIT */
#include "cui/cui_disclosure_events.h"
#include <assert.h>
#include <string.h>
static cui_draw_list draw[2];
static void equal_output(cui_context *a, cui_context *b) {
    assert(cui_focused(a) == cui_focused(b));
    assert(draw[0].count == draw[1].count);
    for (size_t i = 0; i < draw[0].count; ++i) {
        const cui_draw_command *x = draw[0].commands + i, *y = draw[1].commands + i;
        assert(x->type == y->type && !memcmp(&x->bounds, &y->bounds, sizeof x->bounds));
        assert(!memcmp(&x->color, &y->color, sizeof x->color));
        assert(x->text.length == y->text.length);
        if (x->text.length)
            assert(!memcmp(x->text.data, y->text.data, x->text.length));
    }
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a, &na), *sb = cui_semantics(b, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i)
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role &&
               sa[i].details.expanded == sb[i].details.expanded &&
               sa[i].details.controls == sb[i].details.controls);
}
typedef struct {
    cui_context *ui;
    bool *open;
    cui_disclosure_change changes[8];
    size_t count;
    bool semantic_before;
} observer;
static void observe(void *user, const cui_disclosure_change *change) {
    observer *o = user;
    assert(o->count < 8 && *o->open == change->expanded);
    assert(change->previous != change->expanded);
    assert(change->item == 10 && change->item_index == 0);
    assert(change->event.target == 10);
    assert(change->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(change->event.activation.target == 10);
    assert(change->event.activation.event_index == change->event.event_index);
    size_t count;
    const cui_semantic *sem = cui_semantics(o->ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (sem[i].id == 10)
            assert(sem[i].details.expanded == o->semantic_before);
    o->changes[o->count++] = *change;
}
static cui_disclosure_event_result scene(cui_context *ui, bool *open, observer *o, bool legacy,
                                         bool disabled, bool passive) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.layout_only = passive});
    cui_theme theme = cui_theme_light();
    cui_disclosure_event_result result = {0};
    if (legacy)
        result.changed =
            cui_collapsible_ex(ui, 10, &theme, cui_string("Title"), open,
                               (cui_collapsible_options){.disabled = disabled}, NULL, NULL);
    else
        result = cui_collapsible_events(ui, 10, &theme, cui_string("Title"), open,
                                        (cui_collapsible_options){.disabled = disabled}, NULL, NULL,
                                        (cui_disclosure_handler){o ? observe : NULL, o});
    cui_end_box(ui);
    draw[legacy ? 1 : 0] = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void click(cui_context *ui) {
    cui_rect r = cui_bounds(ui, 10);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 2, .y = r.y + 2}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 2, .y = r.y + 2}));
}
typedef struct {
    cui_context *ui;
    bool *open;
    cui_disclosure_change changes[12];
    bool states[12][3];
    size_t count;
} accordion_observer;
static const cui_disclosure_item group[] = {
    {20, {"A", 1}, false}, {30, {"B", 1}, true}, {40, {"C", 1}, false}};
static void accordion_change(void *user, const cui_disclosure_change *change) {
    accordion_observer *o = user;
    assert(o->count < 12 && change->event.target == 99);
    assert(change->item == group[change->item_index].id);
    assert(o->open[change->item_index] == change->expanded);
    assert(change->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    if (o->count < 3) {
        size_t count;
        const cui_semantic *sem = cui_semantics(o->ui, &count);
        for (size_t i = 0; i < count; ++i)
            for (size_t j = 0; j < 3; ++j)
                if (sem[i].id == group[j].id)
                    assert(sem[i].checked == (j < 2) && !sem[i].details.expanded);
    }
    o->changes[o->count] = *change;
    memcpy(o->states[o->count], o->open, sizeof(o->states[0]));
    ++o->count;
}
static void panel_content(cui_context *ui, size_t index, void *user) {
    (void)user;
    cui_begin_box(ui, 200 + index, (cui_box){.width = 30, .height = 20});
    cui_end_box(ui);
}
static cui_disclosure_event_result accordion_scene(cui_context *ui, bool *open,
                                                   accordion_observer *o, bool legacy,
                                                   cui_accordion_options options) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_theme theme = cui_theme_light();
    cui_disclosure_event_result result = {0};
    if (legacy)
        result.changed =
            cui_accordion(ui, 99, &theme, group, 3, open, options, panel_content, NULL);
    else
        result = cui_accordion_events(ui, 99, &theme, group, 3, open, options, panel_content, NULL,
                                      (cui_disclosure_handler){o ? accordion_change : NULL, o});
    draw[legacy ? 1 : 0] = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void group_click(cui_context *ui, cui_id id) {
    cui_rect r = cui_bounds(ui, id);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 2, .y = r.y + 2}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 2, .y = r.y + 2}));
}
static cui_disclosure_event_result base_scene(cui_context *ui, bool *open, accordion_observer *o,
                                              bool passive, bool inert) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 99, (cui_box){.layout_only = passive, .inert = inert});
    cui_disclosure_event_result result = cui_base_accordion_events(
        ui, 99, group, 3, open, (cui_accordion_options){.multiple = true, .collapsible = true},
        (cui_disclosure_handler){accordion_change, o});
    for (size_t i = 0; i < 3; ++i) {
        cui_begin_box(ui, group[i].id, (cui_box){.width = 100, .height = 30});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    return result;
}
static void test_base_routing(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    bool open[3] = {false, false, false};
    accordion_observer o = {.ui = ui, .open = open, .count = 3};
    base_scene(ui, open, &o, false, false);
    cui_focus(ui, 20);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    cui_disclosure_event_result result = base_scene(ui, open, &o, false, false);
    assert(result.change_count == 1 && o.count == 4 && open[2]);
    assert(o.changes[3].item == 40 && o.changes[3].event.event_index == 1);
    /* Routing was frozen before the reducer's cursor/focus navigation. */
    assert(o.changes[3].event.activation.target == 20);
    group_click(ui, 40);
    result = base_scene(ui, open, &o, false, true);
    assert(!result.changed && result.change_count == 0 && open[2]);
    base_scene(ui, open, &o, false, false);
    group_click(ui, 40);
    result = base_scene(ui, open, &o, true, false);
    assert(result.change_count == 0 && o.count == 4);
    cui_destroy(ui);
}
static void test_accordion(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    cui_context *old = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    assert(ui && old);
    bool open[3] = {true, true, false}, prior[3] = {true, true, false};
    accordion_observer o = {.ui = ui, .open = open};
    cui_disclosure_motion motion[3] = {0}, old_motion[3] = {0};
    cui_accordion_options options = {
        .collapsible = true, .motion = motion, .motion_count = 3, .dt = .016f};
    cui_accordion_options old_options = options;
    old_options.motion = old_motion;
    accordion_scene(ui, open, &o, false, options);
    accordion_scene(old, prior, NULL, true, old_options);
    group_click(ui, 40);
    group_click(old, 40);
    cui_disclosure_event_result result = accordion_scene(ui, open, &o, false, options);
    cui_disclosure_event_result legacy = accordion_scene(old, prior, NULL, true, old_options);
    assert(result.changed == legacy.changed && result.changed && result.change_count == 3);
    assert(memcmp(open, prior, sizeof open) == 0);
    equal_output(ui, old);
    assert(!memcmp(motion, old_motion, sizeof motion));
    options.motion = NULL;
    options.motion_count = 0;
    assert(o.changes[0].item_index == 0 && o.changes[0].previous && !o.changes[0].expanded);
    assert(o.changes[1].item_index == 1 && o.changes[1].previous && !o.changes[1].expanded);
    assert(o.changes[2].item_index == 2 && !o.changes[2].previous && o.changes[2].expanded);
    assert(!o.states[0][0] && o.states[0][1] && !o.states[0][2]);
    assert(!o.states[1][0] && !o.states[1][1] && !o.states[1][2]);
    for (size_t i = 0; i < 3; ++i) {
        assert(o.changes[i].event.activation.target == 40);
        assert(o.changes[i].event.event_index == o.changes[0].event.event_index);
    }
    /* Repeated same-frame toggles retain both transitions and input indices. */
    group_click(ui, 40);
    group_click(ui, 40);
    result = accordion_scene(ui, open, &o, false, options);
    assert(result.change_count == 2 && o.count == 5 && open[2]);
    assert(o.changes[3].event.event_index < o.changes[4].event.event_index);
    options.collapsible = false;
    group_click(ui, 40);
    result = accordion_scene(ui, open, &o, false, options);
    assert(!result.changed && result.change_count == 0 && o.count == 5);
    options.multiple = true;
    cui_focus(ui, 20);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    result = accordion_scene(ui, open, &o, false, options);
    assert(result.change_count == 1 && open[0] && open[2]);
    assert(o.changes[5].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 40,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    options.collapsible = true;
    result = accordion_scene(ui, open, &o, false, options);
    assert(result.change_count == 1 && open[0] && !open[2]);
    assert(o.changes[6].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    options.disabled = true;
    group_click(ui, 20);
    result = accordion_scene(ui, open, &o, false, options);
    assert(!result.changed && result.change_count == 0);
    options.disabled = false;
    accordion_scene(ui, open, NULL, false, options);
    group_click(ui, 20);
    result = accordion_scene(ui, open, NULL, false, options);
    assert(result.changed && !open[0] && result.change_count == 0);
    cui_destroy(ui);
    cui_destroy(old);
    assert(o.changes[1].event.activation.target == 40);
}
int main(void) {
    test_accordion();
    test_base_routing();
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    cui_context *old = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    assert(ui && old);
    bool open = false, previous = false;
    observer o = {.ui = ui, .open = &open};
    scene(ui, &open, &o, false, false, false);
    scene(old, &previous, NULL, true, false, false);
    click(ui);
    click(ui);
    click(old);
    click(old);
    cui_disclosure_event_result result = scene(ui, &open, &o, false, false, false);
    cui_disclosure_event_result legacy = scene(old, &previous, NULL, true, false, false);
    assert(result.changed == legacy.changed && result.changed && !open && open == previous);
    equal_output(ui, old);
    assert(result.change_count == 2 && o.count == 2 && !result.count_overflow);
    assert(!o.changes[0].previous && o.changes[0].expanded);
    assert(o.changes[1].previous && !o.changes[1].expanded);
    assert(o.changes[0].event.event_index < o.changes[1].event.event_index);
    assert(o.changes[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    click(ui);
    result = scene(ui, &open, NULL, false, false, false);
    assert(result.changed && open && result.change_count == 0);
    o.semantic_before = true;
    click(ui);
    result = scene(ui, &open, &o, false, true, false);
    assert(!result.changed && open && o.count == 2);
    scene(ui, &open, &o, false, false, true);
    assert(o.count == 2);
    /* Keyboard and accessibility are genuine accepted evidence, not inferred clicks. */
    scene(ui, &open, NULL, false, false, false);
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    result = scene(ui, &open, &o, false, false, false);
    assert(result.change_count == 1 && !open && o.count == 3);
    assert(o.changes[2].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    o.semantic_before = false;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 10,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    result = scene(ui, &open, &o, false, false, false);
    assert(result.change_count == 1 && open && o.count == 4);
    assert(o.changes[3].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(cui_base_collapsible_events(ui, 0, cui_string(""), &open, false,
                                       (cui_disclosure_handler){observe, &o})
               .change_count == 0);
    /* Omit on the next frame; copied pointer-free evidence survives teardown. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_end_frame(ui);
    cui_destroy(ui);
    cui_destroy(old);
    assert(o.changes[0].event.activation.target == 10);
    return 0;
}
