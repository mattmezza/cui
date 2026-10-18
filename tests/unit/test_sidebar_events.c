/* SPDX-License-Identifier: MIT */
#include "cui/cui_sidebar_events.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_sidebar_activation copies[32];
static size_t calls, legacy_calls, expands, trace_count;
static unsigned trace[64];
static bool consume_later, cancel_later, redirect;
static bool *controlled_owner;
static bool accept_old, accept_new, cancel_current_expansion;
static cui_sidebar_expansion_fn request_observer, change_observer;
static cui_sidebar_expansion_event expansion_copies[16];
static size_t expansion_calls;
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
    if (controlled_owner && accept_old)
        *controlled_owner = value;
    if (cancel_current_expansion) {
        size_t n;
        (void)cui_frame_events(ui, &n);
        for (size_t e = 0; e < n; e++)
            if (cui_event_target(ui, e) == 10)
                cui_cancel_event_activation(ui, e);
    }
}
static void activated(void *user, const cui_sidebar_activation *event) {
    assert(user == &calls && calls < 32);
    copies[calls++] = *event;
    trace[trace_count++] = 3;
    assert(event->event.kind == CUI_COMPONENT_EVENT_ACTIVATE);
    if (event->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION)
        assert(event->event.activation.target == event->item);
}
static void expansion(void *user, const cui_sidebar_expansion_event *event) {
    assert(user == &calls && expansion_calls < 16);
    expansion_copies[expansion_calls++] = *event;
    bool request = event->event.kind == CUI_COMPONENT_EVENT_REQUEST;
    assert(request || event->event.kind == CUI_COMPONENT_EVENT_CHANGE);
    trace[trace_count++] = request ? 4 : 5;
    if (request && accept_new) {
        assert(controlled_owner);
        *controlled_owner = event->value;
    }
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
            (cui_sidebar_item_handler){.on_activate = observer ? activated : NULL,
                                       .user = &calls,
                                       .on_expand_request = observer ? request_observer : NULL,
                                       .on_expand_change = observer ? change_observer : NULL});
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
    controlled_owner = NULL;
    accept_old = accept_new = cancel_current_expansion = false;
    request_observer = change_observer = NULL;
    expansion_calls = 0;
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
            (cui_sidebar_item_handler){.on_activate = observer ? activated : NULL,
                                       .user = &calls,
                                       .on_expand_request = observer ? request_observer : NULL,
                                       .on_expand_change = observer ? change_observer : NULL});
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
static void test_expansion(void) {
    reset();
    ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    assert(ui);
    request_observer = change_observer = expansion;
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
    base_frame(&item, true, false, false, 0);
    assert(expansion_calls == 0);
    click(10, false);
    click(10, false);
    cui_sidebar_item_event_result r = base_frame(&item, true, false, false, 0);
    assert(r.change_count == 2 && r.request_count == 0 && r.activation_count == 2 &&
           !r.control.open);
    unsigned expected[] = {1, 5, 2, 3, 1, 5, 2, 3};
    assert(trace_count == 8 && !memcmp(trace, expected, sizeof(expected)));
    assert(!expansion_copies[0].previous && expansion_copies[0].value);
    assert(expansion_copies[1].previous && !expansion_copies[1].value);
    assert(expansion_copies[0].event.event_index == copies[0].event.event_index);
    assert(expansion_copies[0].event.activation.target == copies[0].event.activation.target);
    cancel_current_expansion = true;
    click(10, false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.change_count == 1 && r.activation_count == 1 && r.control.open);
    assert(expansion_copies[2].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(copies[calls - 1].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    cancel_current_expansion = false;
    state.open = false;
    redirect = true;
    click(10, false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.control.expansion_changed && r.change_count == 0 && !r.control.open);
    redirect = false;
    bool owner = false;
    controlled_owner = &owner;
    item.expanded = &owner;
    expansion_calls = trace_count = 0;
    click(10, false);
    click(10, false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.request_count == 2 && r.change_count == 0 && !owner && !r.control.open);
    assert(!expansion_copies[0].previous && expansion_copies[0].value &&
           !expansion_copies[1].previous && expansion_copies[1].value);
    accept_new = true;
    expansion_calls = 0;
    click(10, false);
    click(10, false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.request_count == 2 && !owner && expansion_copies[1].previous &&
           !expansion_copies[1].value);
    accept_new = false;
    accept_old = true;
    expansion_calls = 0;
    click(10, false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.request_count == 1 && r.change_count == 0 && owner && r.control.open);
    assert(!expansion_copies[0].previous && expansion_copies[0].value);
    item.click_to_open = true;
    click(10, false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.request_count == 0 && owner);
    item.click_to_open = false;
    /* Caret is a Button: real accessibility ACTIVATE requests expansion only. */
    assert(
        cui_queue_accessibility_action(ui, cui_sidebar_part_id(10, 0), CUI_ACCESSIBILITY_ACTIVATE));
    r = base_frame(&item, true, false, false, 0);
    assert(r.request_count == 1 && r.activation_count == 0 && !owner);
    cui_sidebar_expansion_event saved = expansion_copies[expansion_calls - 1];
    assert(saved.event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(saved.event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(saved.event.target == 10 && saved.event.activation.target == cui_sidebar_part_id(10, 0));
    item.disable_caret = true;
    base_frame(&item, true, false, false, 0);
    click(cui_sidebar_part_id(10, 0), false);
    r = base_frame(&item, true, false, false, 0);
    assert(r.request_count == 0);
    item.disable_caret = false;
    click(10, false);
    r = base_frame(&item, false, false, false, 0);
    assert(r.control.expansion_changed && owner && r.request_count == 0 && r.change_count == 0);
    for (unsigned passive = 1; passive <= 2; passive++) {
        base_frame(&item, true, false, false, 0);
        click(10, false);
        r = base_frame(&item, true, false, false, passive);
        assert(r.request_count == 0 && r.change_count == 0);
    }
    cui_destroy(ui);
    assert(saved.item == 10 && saved.event.activation.target == cui_sidebar_part_id(10, 0));
    /* Styled forwarding preserves the compound target and original caret cause. */
    reset();
    ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    assert(ui);
    request_observer = change_observer = expansion;
    item.expanded = NULL;
    item.disable_caret = false;
    menu_frame(&item, 1, false, true, false);
    click(cui_sidebar_part_id(10, 0), true);
    cui_sidebar_menu_event_result menu = menu_frame(&item, 1, false, true, false);
    assert(menu.change_count == 1 && menu.request_count == 0 && menu.activation_count == 0);
    assert(expansion_copies[0].event.target == 2 &&
           expansion_copies[0].event.activation.target == cui_sidebar_part_id(10, 0));
    owner = false;
    controlled_owner = &owner;
    accept_new = true;
    item.expanded = &owner;
    click(10, false);
    menu = menu_frame(&item, 1, false, true, false);
    assert(menu.request_count == 1 && menu.change_count == 0 && owner);
    assert(expansion_copies[1].event.kind == CUI_COMPONENT_EVENT_REQUEST &&
           expansion_copies[1].event.target == 2 &&
           expansion_copies[1].event.activation.target == 10);
    cui_destroy(ui);
    reset();
    ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    assert(ui);
    request_observer = change_observer = expansion;
    cui_sidebar_item_state states[2] = {{0}};
    cui_sidebar_menu_item children[2] = {{.id = 11, .label = {"A child", 7}},
                                         {.id = 21, .label = {"B child", 7}}};
    cui_sidebar_menu_item parents[2] = {{.id = 10,
                                         .label = {"A", 1},
                                         .state = &states[0],
                                         .children = &children[0],
                                         .child_count = 1},
                                        {.id = 20,
                                         .label = {"B", 1},
                                         .state = &states[1],
                                         .children = &children[1],
                                         .child_count = 1}};
    menu_frame(parents, 2, false, true, false);
    click(cui_sidebar_part_id(20, 0), false);
    click(cui_sidebar_part_id(10, 0), false);
    menu = menu_frame(parents, 2, false, true, false);
    assert(menu.change_count == 2 && menu.activation_count == 0);
    assert(expansion_copies[0].item == 10 && expansion_copies[1].item == 20);
    assert(expansion_copies[0].event.event_index > expansion_copies[1].event.event_index);
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
            ui, 2, &t, &parent, 1, frame == 2,
            (cui_sidebar_item_handler){.on_activate = activated, .user = &calls});
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(r.valid && r.submitted);
        assert(r.activation_count == (frame == 1 ? 1u : 0u));
    }
    assert(calls == 1 && copies[0].item == 31 && copies[0].level == 2);
    assert(copies[0].event.target == 2 && copies[0].event.activation.target == 31);
    cui_destroy(ui);
}
static cui_sidebar_collapse_request collapse_copies[16];
static size_t collapse_calls, old_collapse_calls;
static bool old_collapse_values[16], collapse_owner, toggle_compare, collapse_consume_a11y,
    collapse_cancel_claimed;
static size_t claimed_cancel_attempts;
static void old_collapse(bool value, void *user) {
    assert(user == &collapse_owner && old_collapse_calls < 16);
    old_collapse_values[old_collapse_calls++] = value;
    trace[trace_count++] = 6;
    collapse_owner = true; /* Deliberately differs from some requests. */
    if (collapse_consume_a11y) {
        size_t n;
        (void)cui_frame_events(ui, &n);
        for (size_t i = 0; i < n; i++)
            if (cui_event_activation(ui, i) == 60)
                assert(cui_consume_event(ui, i));
    }
    if (collapse_cancel_claimed) {
        size_t n;
        (void)cui_frame_events(ui, &n);
        for (size_t i = 0; i < n; i++)
            if (cui_event_activation(ui, i) == 60) {
                assert(!cui_cancel_event_activation(ui, i));
                claimed_cancel_attempts++;
            }
    }
}
static void collapse_requested(void *user, const cui_sidebar_collapse_request *request) {
    assert(user == &collapse_owner && collapse_calls < 16);
    collapse_copies[collapse_calls++] = *request;
    trace[trace_count++] = 7;
    assert(request->event.kind == CUI_COMPONENT_EVENT_REQUEST && request->event.target == 60);
    assert(request->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    collapse_owner = false; /* Allowed owner assignment; local flips do not reread it. */
}
static cui_sidebar_toggle_event_result toggle_frame(bool styled, bool legacy, bool observer,
                                                    unsigned passive, bool disabled, bool entry) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 250, .inert = passive == 1, .layout_only = passive == 2});
    cui_sidebar_toggle_options options = {.collapsed = entry,
                                          .disabled = disabled,
                                          .accessibility_label = {"Toggle", 6},
                                          .on_change = old_collapse,
                                          .user = &collapse_owner};
    cui_sidebar_collapse_handler handler = {.on_request = observer ? collapse_requested : NULL,
                                            .user = &collapse_owner};
    cui_sidebar_toggle_event_result result;
    memset(&result, 0, sizeof(result));
    cui_theme theme = cui_theme_light();
    if (styled) {
        if (legacy)
            result.activated = cui_sidebar_toggle_button(ui, 60, &theme, options);
        else
            result = cui_sidebar_toggle_button_events(ui, 60, &theme, options, handler);
    } else {
        if (legacy)
            result.activated = cui_base_sidebar_toggle(ui, 60, options);
        else
            result = cui_base_sidebar_toggle_events(ui, 60, options, handler);
        cui_begin_box(ui, 60, (cui_box){.width = 24, .height = 24});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    if (styled) {
        assert(draw.count < 128);
        if (passive != 2)
            assert(draw.count > 0);
        if (legacy) {
            old_count = draw.count;
            memcpy(old_commands, draw.commands, draw.count * sizeof(*draw.commands));
        }
        if (toggle_compare) {
            assert(old_count == draw.count);
            for (size_t i = 0; i < draw.count; i++) {
                assert(old_commands[i].type == draw.commands[i].type);
                assert(
                    !memcmp(&old_commands[i].bounds, &draw.commands[i].bounds, sizeof(cui_rect)));
                assert(!memcmp(&old_commands[i].color, &draw.commands[i].color, sizeof(cui_color)));
            }
        }
    }
    return result;
}
static void test_collapse(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        bool legacy_values[3] = {false};
        for (unsigned pass = 0; pass < 2; pass++) {
            reset();
            collapse_calls = old_collapse_calls = 0;
            collapse_owner = false;
            ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
            assert(ui);
            toggle_frame(styled != 0, pass == 0, true, 0, false, false);
            assert(collapse_calls == 0 && old_collapse_calls == 0);
            click(60, false);
            click(60, false);
            click(60, false);
            toggle_compare = pass != 0;
            cui_sidebar_toggle_event_result result =
                toggle_frame(styled != 0, pass == 0, true, 0, false, false);
            toggle_compare = false;
            assert(result.activated && old_collapse_calls == 3);
            assert(old_collapse_values[0] && !old_collapse_values[1] && old_collapse_values[2]);
            if (!pass)
                memcpy(legacy_values, old_collapse_values, sizeof(legacy_values));
            else {
                assert(result.valid && result.request_count == 3 && collapse_calls == 3);
                assert(!memcmp(legacy_values, old_collapse_values, sizeof(legacy_values)));
                assert(!collapse_copies[0].previous && collapse_copies[0].value &&
                       collapse_copies[1].previous && !collapse_copies[1].value &&
                       !collapse_copies[2].previous && collapse_copies[2].value);
                unsigned expected[] = {6, 7, 6, 7, 6, 7};
                assert(trace_count == 6 && !memcmp(trace, expected, sizeof(expected)));
                assert(!collapse_owner);
            }
            size_t n;
            const cui_semantic *sem = cui_semantics(ui, &n);
            bool found = false;
            for (size_t i = 0; i < n; i++)
                if (sem[i].id == 60) {
                    assert(sem[i].details.expanded);
                    found = true;
                }
            assert(found);
            cui_destroy(ui);
        }
        reset();
        collapse_calls = old_collapse_calls = 0;
        ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
        assert(ui);
        toggle_frame(styled != 0, false, true, 0, false, true);
        click(60, true);
        cui_sidebar_toggle_event_result result =
            toggle_frame(styled != 0, false, true, 0, false, true);
        assert(result.request_count == 1 && collapse_copies[0].previous &&
               !collapse_copies[0].value);
        assert(collapse_copies[0].event.activation.release.type == CUI_POINTER_TOUCH);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        for (size_t i = 0; i < n; i++)
            if (sem[i].id == 60)
                assert(!sem[i].details.expanded);
        cui_focus(ui, 60);
        key(CUI_KEY_ENTER);
        result = toggle_frame(styled != 0, false, true, 0, false, false);
        assert(result.request_count == 1 &&
               collapse_copies[1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
        assert(cui_queue_accessibility_action(ui, 60, CUI_ACCESSIBILITY_ACTIVATE));
        result = toggle_frame(styled != 0, false, true, 0, false, false);
        assert(result.request_count == 1 &&
               collapse_copies[2].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
        cui_sidebar_collapse_request saved = collapse_copies[2];
        click(60, false);
        result = toggle_frame(styled != 0, false, false, 0, false, false);
        assert(result.activated && result.request_count == 0);
        toggle_frame(styled != 0, false, true, 0, true, false);
        click(60, false);
        result = toggle_frame(styled != 0, false, true, 0, true, false);
        assert(!result.activated && result.request_count == 0);
        for (unsigned passive = 1; passive <= 2; passive++) {
            toggle_frame(styled != 0, false, true, 0, false, false);
            click(60, false);
            result = toggle_frame(styled != 0, false, true, passive, false, false);
            assert(!result.activated && result.request_count == 0);
        }
        toggle_frame(styled != 0, false, true, 0, false, false);
        collapse_cancel_claimed = true;
        claimed_cancel_attempts = 0;
        click(60, false);
        click(60, false);
        click(60, false);
        result = toggle_frame(styled != 0, false, true, 0, false, false);
        assert(result.request_count == 3 && claimed_cancel_attempts >= 3);
        collapse_cancel_claimed = false;
        collapse_consume_a11y = true;
        for (unsigned i = 0; i < 3; i++)
            assert(cui_queue_accessibility_action(ui, 60, CUI_ACCESSIBILITY_ACTIVATE));
        result = toggle_frame(styled != 0, false, true, 0, false, false);
        assert(result.activated && result.request_count == 1);
        assert(collapse_copies[collapse_calls - 1].event.flags &
               CUI_COMPONENT_EVENT_HAS_ACTIVATION);
        collapse_consume_a11y = false;
        assert(!cui_base_sidebar_toggle_events(ui, 0, (cui_sidebar_toggle_options){0},
                                               (cui_sidebar_collapse_handler){0})
                    .valid);
        assert(!cui_sidebar_toggle_button_events(ui, 60, NULL, (cui_sidebar_toggle_options){0},
                                                 (cui_sidebar_collapse_handler){0})
                    .valid);
        cui_destroy(ui);
        assert(saved.event.activation.target == 60 &&
               saved.event.kind == CUI_COMPONENT_EVENT_REQUEST);
    }
}
int main(void) {
    test_base();
    test_legacy_equivalence();
    test_menu();
    test_nested();
    test_expansion();
    test_collapse();
    return 0;
}
