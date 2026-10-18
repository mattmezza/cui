/* SPDX-License-Identifier: MIT */
#include "cui/cui_sidebar_events.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_sidebar_activation copies[32];
static size_t calls, legacy_calls, expands, trace_count;
static unsigned trace[64];
static bool consume_later, cancel_later, redirect;
static cui_sidebar_item_state state;
static void old_activate(void *user) {
    assert(user == &legacy_calls);
    legacy_calls++;
    trace[trace_count++] = 2;
    if (consume_later) {
        size_t n;
        (void)cui_frame_events(ui, &n);
        for (size_t e = 0; e < n; e++)
            if (cui_event_target(ui, e) == 20) {
                cui_consume_event(ui, e);
                if (cancel_later)
                    cui_cancel_event_activation(ui, e);
            }
    }
}
static void old_expand(bool value, void *user) {
    assert(user == &legacy_calls);
    expands++;
    trace[trace_count++] = 1;
    if (redirect)
        state.open = !value;
}
static void activated(void *user, const cui_sidebar_activation *event) {
    assert(user == &calls && calls < 32);
    copies[calls++] = *event;
    trace[trace_count++] = 3;
    assert(event->event.kind == CUI_COMPONENT_EVENT_ACTIVATE);
    if (event->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION)
        assert(event->event.activation.target == event->item);
}
static cui_sidebar_item_event_result base_frame(cui_sidebar_menu_item *item, bool observer,
                                                bool legacy, bool collapsed, unsigned passive) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 250, .inert = passive == 1, .layout_only = passive == 2});
    cui_sidebar_item_event_result r;
    memset(&r, 0, sizeof(r));
    if (legacy)
        r.control = cui_base_sidebar_item(ui, item, collapsed, 1);
    else
        r = cui_base_sidebar_item_events(
            ui, item, collapsed, 1,
            (cui_sidebar_item_handler){observer ? activated : NULL, &calls});
    cui_begin_box(ui, item->id, (cui_box){.width = 180, .height = 28});
    cui_end_box(ui);
    if (item->child_count && !collapsed) {
        cui_begin_box(ui, cui_sidebar_part_id(item->id, 0), (cui_box){.width = 20, .height = 20});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    return r;
}
static void click(cui_id id, bool touch) {
    cui_rect b = cui_bounds(ui, id);
    assert(b.w > 0 && b.h > 0);
    for (unsigned up = 0; up < 2; up++)
        assert(cui_queue_event(
            ui, (cui_event){.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                            .x = b.x + 2,
                            .y = b.y + 2,
                            .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                            .pointer_id = touch ? 42 : 0}));
}
static void key(cui_key k) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void reset(void) {
    calls = legacy_calls = expands = trace_count = 0;
    consume_later = cancel_later = redirect = false;
    memset(&state, 0, sizeof(state));
}
static void test_base(void) {
    reset();
    ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_sidebar_menu_item child = {.id = 11, .label = {"Child", 5}};
    cui_sidebar_menu_item item = {.id = 10,
                                  .label = {"Parent", 6},
                                  .state = &state,
                                  .children = &child,
                                  .child_count = 1,
                                  .click_to_toggle = true,
                                  .on_activate = old_activate,
                                  .on_expand = old_expand,
                                  .user = &legacy_calls};
    assert(base_frame(&item, true, false, false, 0).valid);
    assert(calls == 0);
    click(10, false);
    click(10, false);
    cui_sidebar_item_event_result r = base_frame(&item, true, false, false, 0);
    assert(r.control.activated && r.control.expansion_changed && !r.control.open &&
           r.activation_count == 2);
    assert(calls == 2 && legacy_calls == 2 && expands == 2);
    unsigned expected[] = {1, 2, 3, 1, 2, 3};
    assert(trace_count == 6 && !memcmp(trace, expected, sizeof(expected)));
    assert(copies[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(copies[0].event.target == 10 && copies[0].level == 1 &&
           copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(copies[0].event.event_index == 1 && copies[1].event.event_index == 3);
    cui_sidebar_activation saved = copies[0];
    click(cui_sidebar_part_id(10, 0), false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.control.expansion_changed && !r.control.activated && r.activation_count == 0 &&
           state.open);
    cui_focus(ui, 10);
    key(CUI_KEY_ENTER);
    r = base_frame(&item, true, false, false, 0);
    assert(r.activation_count == 1 &&
           copies[calls - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    click(10, true);
    r = base_frame(&item, true, false, false, 0);
    assert(r.activation_count == 1 &&
           copies[calls - 1].event.activation.release.type == CUI_POINTER_TOUCH);
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    assert(!cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    assert(base_frame(&item, true, false, false, 0).activation_count == 0);
    assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_FOCUS));
    base_frame(&item, true, false, false, 0);
    assert(cui_focused(ui) == 10);
    key(CUI_KEY_ENTER);
    assert(base_frame(&item, true, false, false, 0).activation_count == 1);
    assert(copies[calls - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(copies[calls - 1].event.flags & CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(!(copies[calls - 1].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    size_t before = calls;
    click(10, false);
    r = base_frame(&item, false, false, false, 0);
    assert(r.control.activated && r.activation_count == 0 && calls == before);
    item.disabled = true;
    base_frame(&item, true, false, false, 0);
    click(10, false);
    assert(!base_frame(&item, true, false, false, 0).control.activated);
    item.disabled = false;
    for (unsigned passive = 1; passive <= 2; passive++) {
        base_frame(&item, true, false, false, 0);
        click(10, false);
        assert(base_frame(&item, true, false, false, passive).activation_count == 0);
    }
    base_frame(&item, true, false, false, 0);
    redirect = true;
    state.open = false;
    click(10, false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.control.expansion_changed && !r.control.open && r.activation_count == 1);
    assert(!cui_base_sidebar_item_events(ui, NULL, false, 1, (cui_sidebar_item_handler){0}).valid);
    item.on_activate = NULL;
    item.on_expand = NULL;
    size_t old_calls = legacy_calls;
    click(10, false);
    assert(base_frame(&item, true, false, false, 0).activation_count == 1);
    assert(legacy_calls == old_calls);
    cui_destroy(ui);
    assert(saved.item == 10 && saved.event.activation.target == 10);
}
static void test_legacy_equivalence(void) {
    cui_sidebar_item_result old;
    size_t old_activations = 0, old_expansions = 0;
    memset(&old, 0, sizeof(old));
    for (unsigned pass = 0; pass < 2; pass++) {
        reset();
        ui = cui_create((cui_config){.max_elements = 128});
        assert(ui);
        cui_sidebar_menu_item child = {.id = 11, .label = {"Child", 5}};
        cui_sidebar_menu_item item = {.id = 10,
                                      .label = {"Parent", 6},
                                      .state = &state,
                                      .children = &child,
                                      .child_count = 1,
                                      .click_to_toggle = true,
                                      .on_activate = old_activate,
                                      .on_expand = old_expand,
                                      .user = &legacy_calls};
        base_frame(&item, pass != 0, pass == 0, false, 0);
        click(10, false);
        click(10, false);
        cui_sidebar_item_event_result r = base_frame(&item, pass != 0, pass == 0, false, 0);
        if (!pass) {
            old = r.control;
            old_activations = legacy_calls;
            old_expansions = expands;
        } else {
            assert(old.activated == r.control.activated && old.open == r.control.open &&
                   old.expansion_changed == r.control.expansion_changed);
            assert(legacy_calls == old_activations && expands == old_expansions);
            assert(r.activation_count == 2 && calls == 2);
        }
        cui_destroy(ui);
    }
}
static cui_draw_command old_commands[128];
static size_t old_count;
static cui_sidebar_menu_event_result menu_frame(cui_sidebar_menu_item *items, size_t count,
                                                bool legacy, bool observer, bool compare) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 300, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 250, .height = 280});
    cui_sidebar_menu_event_result r;
    memset(&r, 0, sizeof(r));
    if (legacy)
        r.submitted = cui_sidebar_menu(ui, 2, &t, items, count, false);
    else
        r = cui_sidebar_menu_events(
            ui, 2, &t, items, count, false,
            (cui_sidebar_item_handler){observer ? activated : NULL, &calls});
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(draw.count < 128);
    if (legacy) {
        old_count = draw.count;
        memcpy(old_commands, draw.commands, draw.count * sizeof(*draw.commands));
    }
    if (compare) {
        assert(draw.count == old_count);
        for (size_t i = 0; i < draw.count; i++) {
            assert(draw.commands[i].type == old_commands[i].type);
            assert(!memcmp(&draw.commands[i].bounds, &old_commands[i].bounds, sizeof(cui_rect)));
            assert(!memcmp(&draw.commands[i].color, &old_commands[i].color, sizeof(cui_color)));
        }
    }
    return r;
}
static void test_menu(void) {
    reset();
    ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    assert(ui);
    cui_sidebar_menu_item items[] = {
        {.id = 10, .label = {"A", 1}, .on_activate = old_activate, .user = &legacy_calls},
        {.id = 20, .label = {"B", 1}, .on_activate = old_activate, .user = &legacy_calls}};
    menu_frame(items, 2, true, false, false);
    cui_rect bounds = cui_bounds(ui, 10);
    cui_sidebar_menu_event_result r = menu_frame(items, 2, false, true, true);
    assert(r.valid && r.submitted && r.activation_count == 0);
    assert(!memcmp(&bounds,
                   &(cui_rect){cui_bounds(ui, 10).x, cui_bounds(ui, 10).y, cui_bounds(ui, 10).w,
                               cui_bounds(ui, 10).h},
                   sizeof(bounds)));
    click(20, false);
    click(10, false);
    r = menu_frame(items, 2, false, true, false);
    assert(r.activation_count == 2 && copies[0].item == 10 && copies[1].item == 20);
    assert(copies[0].event.target == 2 && copies[0].event.activation.target == 10);
    assert(copies[0].event.event_index > copies[1].event.event_index);
    calls = legacy_calls = trace_count = 0;
    click(10, false);
    click(10, false);
    click(20, false);
    r = menu_frame(items, 2, false, true, false);
    assert(r.activation_count == 3 && calls == 3 && legacy_calls == 3);
    calls = legacy_calls = trace_count = 0;
    consume_later = true;
    click(10, false);
    click(20, false);
    r = menu_frame(items, 2, false, true, false);
    assert(r.activation_count == 2 && calls == 2 && legacy_calls == 2);
    calls = legacy_calls = trace_count = 0;
    cancel_later = true;
    click(10, false);
    click(20, false);
    r = menu_frame(items, 2, false, true, false);
    assert(r.activation_count == 1 && calls == 1 && legacy_calls == 1);
    consume_later = cancel_later = false;
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool row = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == 10) {
            assert(sem[i].role == CUI_ROLE_TREEITEM);
            row = true;
        }
    assert(row);
    assert(!cui_sidebar_menu_events(ui, 0, NULL, items, 2, false, (cui_sidebar_item_handler){0})
                .valid);
    cui_destroy(ui);
}
static void test_nested(void) {
    reset();
    ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    assert(ui);
    cui_theme t = cui_theme_light();
    cui_sidebar_menu_item child = {.id = 31, .label = {"Child", 5}};
    cui_sidebar_menu_item parent = {.id = 30,
                                    .label = {"Parent", 6},
                                    .state = &state,
                                    .children = &child,
                                    .child_count = 1,
                                    .default_open = true};
    for (unsigned frame = 0; frame < 3; frame++) {
        if (frame)
            click(31, false);
        assert(cui_begin_frame(ui, 300, 300, .016f));
        cui_begin_box(ui, 1, (cui_box){.width = 250, .height = 280});
        cui_sidebar_menu_event_result r = cui_sidebar_menu_events(
            ui, 2, &t, &parent, 1, frame == 2, (cui_sidebar_item_handler){activated, &calls});
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(r.valid && r.submitted);
        assert(r.activation_count == (frame == 1 ? 1u : 0u));
    }
    assert(calls == 1 && copies[0].item == 31 && copies[0].level == 2);
    assert(copies[0].event.target == 2 && copies[0].event.activation.target == 31);
    cui_destroy(ui);
}
int main(void) {
    test_base();
    test_legacy_equivalence();
    test_menu();
    test_nested();
    return 0;
}
