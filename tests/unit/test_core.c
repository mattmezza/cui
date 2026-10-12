#include "cui/cui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static cui_context *make(void) {
    cui_context *ui = cui_create((cui_config){
        .width = 300, .height = 200, .max_elements = 64, .max_commands = 128, .frame_bytes = 2048});
    assert(ui);
    return ui;
}
static cui_interaction control(cui_context *ui, cui_id id, bool disabled) {
    cui_begin_box(ui, id, (cui_box){.width = 100, .height = 30, .background = {20, 30, 40, 255}});
    cui_interaction s =
        cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("button"), disabled, false, 0);
    cui_end_box(ui);
    return s;
}
static void begin(cui_context *ui) {
    assert(cui_begin_frame(ui, 300, 200, 0.016f));
}
static void queue(cui_context *ui, cui_event_type type, float x, float y, cui_key key,
                  unsigned mods) {
    assert(cui_queue_event(
        ui, (cui_event){.type = type, .x = x, .y = y, .key = key, .modifiers = mods}));
}
static void test_input(void) {
    cui_context *ui = make();
    cui_id a = cui_id_from(ui, "a"), b = cui_id_from(ui, "b");
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    begin(ui);
    assert(!control(ui, a, false).activated);
    control(ui, b, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_MOVE, 50, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 50, 10, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == a);
    assert(cui_event_target(ui, 1) == a);
    assert(cui_event_target(ui, 2) == a);
    assert(cui_event_activation(ui, 0) == 0);
    assert(cui_event_activation(ui, 1) == 0);
    assert(cui_event_activation(ui, 2) == a);
    assert(cui_event_activation(ui, 3) == 0);
    assert(!control(ui, b, false).activated);
    assert(control(ui, a, false).activated);
    cui_end_frame(ui);
    assert(cui_focused(ui) == a);
    /* Event order uses previous regions, even after controls were reordered. */
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_CANCEL, 0, 0, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    begin(ui);
    assert(!control(ui, b, false).activated);
    assert(!control(ui, a, false).activated);
    cui_end_frame(ui);
    cui_focus(ui, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    assert(control(ui, b, false).activated);
    control(ui, a, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == b);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, CUI_MOD_SHIFT);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_SPACE, 0);
    queue(ui, CUI_EVENT_KEY_UP, 0, 0, CUI_KEY_SPACE, 0);
    begin(ui);
    control(ui, b, false);
    assert(control(ui, a, false).activated);
    assert(cui_event_activation(ui, 2) == a); /* Released Space, after Tab. */
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    control(ui, b, false);
    assert(!control(ui, a, true).activated);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 0);
    cui_focus(ui, b);
    begin(ui);
    control(ui, a, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 0);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    begin(ui);
    assert(control(ui, a, false).pressed);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_UP, 250, 180, 0, 0);
    begin(ui);
    assert(!control(ui, a, false).activated);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_strings_and_draw(void) {
    cui_context *ui = make();
    begin(ui);
    char text[] = "copied text", name[] = "copied name";
    cui_id id = cui_id_from(ui, "clip");
    cui_begin_box(ui, id,
                  (cui_box){.width = 100,
                            .height = 50,
                            .clip = true,
                            .background = {1, 2, 3, 255},
                            .border = {4, 5, 6, 255},
                            .border_width = 2,
                            .radius = 3});
    cui_text(ui, cui_string(text), 12, (cui_color){255, 255, 255, 255});
    cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string(name), false, false, 0);
    cui_update_semantic(ui, id, true, 42);
    memset(text, 'x', sizeof(text) - 1);
    memset(name, 'y', sizeof(name) - 1);
    cui_end_box(ui);
    cui_draw_list dl = cui_end_frame(ui);
    bool txt = false, border = false, push = false, pop = false;
    for (size_t i = 0; i < dl.count; i++) {
        const cui_draw_command *d = &dl.commands[i];
        if (d->type == CUI_DRAW_TEXT) {
            assert(d->text.length == 11);
            assert(!memcmp(d->text.data, "copied text", 11));
            txt = true;
        }
        if (d->type == CUI_DRAW_BORDER) {
            border = true;
            assert(d->stroke == 2);
        }
        push |= d->type == CUI_DRAW_CLIP_PUSH;
        pop |= d->type == CUI_DRAW_CLIP_POP;
    }
    assert(txt && border && push && pop);
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    assert(n == 1);
    assert(!memcmp(s[0].name.data, "copied name", 11));
    assert(s[0].checked && s[0].value == 42);
    assert(s[0].bounds.w == 100);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_contexts_errors(void) {
    cui_context *a = make(), *b = make();
    begin(a);
    cui_id id = cui_id_from(a, "same");
    control(a, id, false);
    begin(b);
    control(b, id, false);
    cui_end_frame(a);
    cui_end_frame(b);
    assert(cui_bounds(a, id).w == 100 && cui_bounds(b, id).w == 100);
    begin(a);
    assert(!cui_begin_frame(a, 300, 200, 0));
    assert(cui_last_error(a) == CUI_ERROR_LIFECYCLE);
    cui_clear_error(a);
    control(a, id, false);
    control(a, id, false);
    cui_end_frame(a);
    assert(cui_last_error(a) == CUI_ERROR_DUPLICATE_ID);
    cui_clear_error(a);
    begin(a);
    cui_id plain = cui_id_from(a, "x");
    assert(cui_push_id(a, "scope"));
    assert(cui_id_from(a, "x") != plain);
    cui_pop_id(a);
    assert(cui_id_from(a, "x") == plain);
    cui_end_frame(a);
    assert(cui_last_error(a) == CUI_OK);
    cui_end_frame(a);
    assert(cui_last_error(a) == CUI_ERROR_LIFECYCLE);
    cui_destroy(a);
    cui_destroy(b);
    cui_context *small =
        cui_create((cui_config){.max_elements = 1, .max_commands = 1, .frame_bytes = 4});
    assert(small);
    begin(small);
    cui_begin_box(small, 1, (cui_box){.width = 50, .height = 30});
    cui_text(small, cui_string("too long"), 12, (cui_color){0});
    cui_end_box(small);
    cui_end_frame(small);
    assert(cui_last_error(small) == CUI_ERROR_CAPACITY);
    cui_destroy(small);
    small = make();
    for (size_t i = 0; i < 256; i++)
        assert(cui_queue_event(small, (cui_event){0}));
    assert(!cui_queue_event(small, (cui_event){0}));
    assert(cui_last_error(small) == CUI_ERROR_CAPACITY);
    cui_destroy(small);
}

static void test_clipped_hit_and_arena(void) {
    cui_context *ui = make();
    cui_id outer = cui_id_from(ui, "outer"), child = cui_id_from(ui, "child");
    begin(ui);
    cui_begin_box(ui, outer, (cui_box){.width = 50, .height = 20, .clip = true});
    control(ui, child, false);
    cui_end_box(ui);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 75, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 75, 10, 0, 0);
    begin(ui);
    cui_begin_box(ui, outer, (cui_box){.width = 50, .height = 20, .clip = true});
    assert(!control(ui, child, false).activated);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_event_target(ui, 0) == 0);
    cui_destroy(ui);
    ui = cui_create((cui_config){.max_elements = 10, .max_commands = 20, .frame_bytes = 4});
    assert(ui);
    begin(ui);
    cui_text(ui, cui_string("1234"), 10, (cui_color){0});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
    ui = cui_create((cui_config){.max_elements = 10, .max_commands = 1});
    assert(ui);
    begin(ui);
    control(ui, 1, false);
    control(ui, 2, false);
    assert(cui_end_frame(ui).count == 0);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
    ui = make();
    begin(ui);
    for (size_t i = 0; i < 258; i++)
        cui_begin_box(ui, (cui_id)(i + 1), (cui_box){.width = 10, .height = 10});
    for (size_t i = 0; i < 258; i++)
        cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    begin(ui);
    control(ui, 1, false);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_creation_overflow(void) {
    assert(!cui_create((cui_config){.max_elements = 1000001}));
    assert(!cui_create((cui_config){.max_elements = SIZE_MAX}));
    assert(!cui_create((cui_config){.max_commands = SIZE_MAX}));
    assert(!cui_create((cui_config){.frame_bytes = SIZE_MAX}));
    /* Rejected configurations must leave an existing Clay context usable. */
    cui_context *ui = make();
    begin(ui);
    assert(!cui_create((cui_config){.max_elements = SIZE_MAX}));
    control(ui, 1, false);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_numeric_id_text_domains(void) {
    cui_context *ui = make();
    for (size_t frame = 0; frame < 3; frame++) {
        begin(ui);
        for (cui_id id = 1; id <= 8; id++) {
            cui_begin_box(ui, id, (cui_box){.width = 100, .height = 20});
            if ((id % 2) == 0) {
                cui_begin_box(ui, 100 + id, (cui_box){.width = 10, .height = 10});
                cui_end_box(ui);
            }
            cui_text(ui, cui_string("OFF"), 10, (cui_color){0, 0, 0, 255});
            cui_interact(ui, id, CUI_ROLE_SWITCH, cui_string("switch"), false, false, 0);
            cui_end_box(ui);
        }
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}

static void modal_frame(cui_context *ui, bool nested) {
    control(ui, 1, false);
    assert(cui_begin_focus_scope(ui, 100, true));
    control(ui, 2, false);
    control(ui, 3, false);
    if (nested) {
        assert(cui_begin_focus_scope(ui, 200, true));
        control(ui, 4, false);
        cui_end_focus_scope(ui);
    }
    cui_end_focus_scope(ui);
    control(ui, 5, false);
}
static void test_focus_scopes(void) {
    cui_context *ui = make();
    begin(ui);
    modal_frame(ui, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 2);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, CUI_MOD_SHIFT);
    begin(ui);
    modal_frame(ui, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 3);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ESCAPE, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 100);
    assert(cui_event_target(ui, 1) == 0);
    assert(cui_event_target(ui, 2) == 100);
    assert(!control(ui, 1, false).activated);
    assert(cui_begin_focus_scope(ui, 100, true));
    control(ui, 2, false);
    control(ui, 3, false);
    cui_end_focus_scope(ui);
    control(ui, 5, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 3);
    begin(ui);
    modal_frame(ui, true);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 4);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ESCAPE, 0);
    begin(ui);
    assert(cui_event_target(ui, 1) == 200);
    modal_frame(ui, true);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 4);
    begin(ui);
    assert(cui_focus_restore(ui, 1));
    control(ui, 1, false);
    control(ui, 5, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 1);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    control(ui, 1, false);
    control(ui, 5, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 5);
    begin(ui);
    assert(cui_focus_restore(ui, 1));
    control(ui, 1, true);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 0);
    begin(ui);
    assert(!cui_focus_restore(ui, 1));
    control(ui, 1, false);
    assert(cui_begin_focus_scope(ui, 300, false));
    control(ui, 2, false);
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    cui_focus(ui, 2);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ESCAPE, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 300);
    control(ui, 1, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 0);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void portal_frame(cui_context *ui, bool *high, bool *low, bool *back) {
    cui_begin_box(ui, 90, (cui_box){.width = 40, .height = 40, .clip = true});
    /* Submit highest first: hit testing must follow paint layer, not source order. */
    cui_begin_portal(ui, 91, (cui_box){.background = {255, 0, 0, 255}}, (cui_rect){80, 70, 100, 30},
                     20);
    *high = cui_interact(ui, 91, CUI_ROLE_BUTTON, cui_string("high"), false, false, 0).activated;
    cui_end_box(ui);
    cui_begin_portal(ui, 92, (cui_box){.background = {0, 255, 0, 255}}, (cui_rect){80, 70, 100, 30},
                     10);
    *low = cui_interact(ui, 92, CUI_ROLE_BUTTON, cui_string("low"), false, false, 0).activated;
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_portal(ui, 93, (cui_box){.background = {0, 0, 255, 255}}, (cui_rect){80, 70, 100, 30},
                     1);
    *back = cui_interact(ui, 93, CUI_ROLE_BUTTON, cui_string("back"), false, false, 0).activated;
    cui_end_box(ui);
}
static void test_portals(void) {
    cui_context *ui = make();
    bool high, low, back;
    begin(ui);
    portal_frame(ui, &high, &low, &back);
    cui_draw_list list = cui_end_frame(ui);
    int red = -1, green = -1, blue = -1;
    for (size_t i = 0; i < list.count; i++) {
        cui_draw_command c = list.commands[i];
        if (c.type != CUI_DRAW_RECT || c.bounds.w != 100)
            continue;
        assert(c.bounds.x == 80 && c.bounds.y == 70 && c.bounds.h == 30);
        if (c.color.r == 255)
            red = (int)i;
        if (c.color.g == 255)
            green = (int)i;
        if (c.color.b == 255)
            blue = (int)i;
    }
    assert(blue >= 0 && green > blue && red > green);
    queue(ui, CUI_EVENT_POINTER_DOWN, 90, 80, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 90, 80, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 91);
    assert(cui_hovered(ui, 91));
    assert(!cui_hovered(ui, 92));
    assert(!cui_hovered(ui, 93));
    portal_frame(ui, &high, &low, &back);
    cui_end_frame(ui);
    assert(high && !low && !back);
    assert(cui_last_error(ui) == CUI_OK);
    begin(ui);
    cui_begin_portal(ui, 100, (cui_box){0}, (cui_rect){0, 0, -1, 10}, 1);
    cui_text(ui, cui_string("suppressed"), 12, (cui_color){0});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_LAYOUT);
    cui_destroy(ui);
}
static void test_semantic_details_and_roving(void) {
    cui_context *ui = make();
    begin(ui);
    cui_begin_box(ui, 90, (cui_box){.width = 200, .height = 200});
    cui_interact(ui, 90, CUI_ROLE_GROUP, cui_string("group"), false, false, 0);
    control(ui, 1, false);
    control(ui, 2, false);
    cui_set_tab_stop(ui, 1, false);
    char description[] = "description";
    cui_set_semantic_details(ui, 2,
                             (cui_semantic_details){.expanded = true,
                                                    .selected = true,
                                                    .multiselectable = true,
                                                    .orientation = CUI_ORIENTATION_VERTICAL,
                                                    .controls = 20,
                                                    .parent = 90,
                                                    .description = cui_string(description)});
    memset(description, 'X', sizeof(description) - 1);
    cui_end_box(ui);
    cui_end_frame(ui);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(count == 3 && nodes[1].tab_index == -1);
    assert(nodes[2].details.expanded && nodes[2].details.selected);
    assert(nodes[2].details.multiselectable &&
           nodes[2].details.orientation == CUI_ORIENTATION_VERTICAL);
    assert(nodes[2].details.controls == 20 && nodes[2].details.parent == 90);
    assert(nodes[2].details.description.length == 11);
    assert(memcmp(nodes[2].details.description.data, "description", 11) == 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 2);
    control(ui, 1, false);
    cui_set_tab_stop(ui, 1, false);
    control(ui, 2, false);
    cui_end_frame(ui);
    /* A roving item remains pointer focusable. */
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    begin(ui);
    assert(cui_focused(ui) == 1);
    control(ui, 1, false);
    cui_set_tab_stop(ui, 1, false);
    control(ui, 2, false);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_scope_dismissal(void) {
    cui_context *ui = make();
    begin(ui);
    modal_frame(ui, true);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 4);
    begin(ui);
    control(ui, 1, false);
    assert(cui_begin_focus_scope(ui, 100, true));
    control(ui, 2, false);
    assert(cui_begin_focus_scope(ui, 200, true));
    /* Register behavior, then dismiss before painting the closed popup. */
    cui_interact(ui, 4, CUI_ROLE_BUTTON, cui_string("popup item"), false, false, 0);
    assert(cui_dismiss_focus_scope(ui, 200, 2));
    assert(!cui_dismiss_focus_scope(ui, 200, 2));
    cui_interact(ui, 40, CUI_ROLE_BUTTON, cui_string("suppressed"), false, false, 0);
    cui_end_focus_scope(ui);
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 2);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(count == 2 && nodes[0].id == 1 && nodes[1].id == 2);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ESCAPE, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 100);
    control(ui, 1, false);
    assert(cui_begin_focus_scope(ui, 100, true));
    cui_interact(ui, 2, CUI_ROLE_BUTTON, cui_string("popup item"), false, false, 0);
    assert(cui_dismiss_focus_scope(ui, 100, 1));
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 1);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_invalid_portal_at_depth_limit(void) {
    cui_context *ui = cui_create(
        (cui_config){.width = 200, .height = 200, .max_elements = 512, .max_commands = 1024});
    assert(ui && cui_begin_frame(ui, 200, 200, .016f));
    for (cui_id id = 1; id <= 256; id++)
        cui_begin_box(ui, id, (cui_box){.width = 200, .height = 200});
    cui_begin_portal(ui, 300, (cui_box){0}, (cui_rect){0, 0, -1, 10}, 1);
    cui_end_box(ui);
    cui_text(ui, cui_string("retained parent"), 12, (cui_color){255, 255, 255, 255});
    for (size_t i = 0; i < 256; i++)
        cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    bool retained = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].text.length == 15)
            retained = true;
    assert(retained && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
}

static void test_child_alignment(void) {
    cui_context *ui = make();
    begin(ui);
    cui_begin_box(ui, 10, (cui_box){.width = 300, .height = 200, .row = true});
    for (unsigned i = 0; i < 3; i++) {
        cui_begin_box(
            ui, 20 + i,
            (cui_box){
                .width = 100, .height = 40, .align_x = (cui_align)i, .align_y = (cui_align)i});
        cui_begin_box(ui, 30 + i, (cui_box){.width = 20, .height = 10});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    cui_rect start = cui_bounds(ui, 30), center = cui_bounds(ui, 31), end = cui_bounds(ui, 32);
    assert(start.x == 0 && start.y == 0);
    assert(center.x == 140 && center.y == 15);
    assert(end.x == 280 && end.y == 30);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_menu_check_radio_roles(void) {
    cui_context *ui = make();
    begin(ui);
    cui_interact(ui, 10, CUI_ROLE_MENU, cui_string("menu"), false, false, 0);
    cui_interact(ui, 11, CUI_ROLE_MENUITEMCHECKBOX, cui_string("check"), false, true, 0);
    cui_interact(ui, 12, CUI_ROLE_MENUITEMRADIO, cui_string("radio"), false, true, 0);
    cui_interact(ui, 13, CUI_ROLE_MENUITEMCHECKBOX, cui_string("disabled"), true, false, 0);
    cui_end_frame(ui);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(count == 4 && nodes[1].checked && nodes[2].checked && nodes[3].disabled);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 11);
    cui_interact(ui, 11, CUI_ROLE_MENUITEMCHECKBOX, cui_string("check"), false, true, 0);
    cui_interact(ui, 12, CUI_ROLE_MENUITEMRADIO, cui_string("radio"), false, true, 0);
    cui_interact(ui, 13, CUI_ROLE_MENUITEMCHECKBOX, cui_string("disabled"), true, false, 0);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 12);
    cui_end_frame(ui);
    cui_destroy(ui);
}

static void test_secondary_pointer(void) {
    cui_context *ui = make();
    begin(ui);
    control(ui, 1, false);
    control(ui, 2, false);
    cui_end_frame(ui);
    cui_focus(ui, 2);
    for (unsigned button = CUI_POINTER_SECONDARY; button <= CUI_POINTER_MIDDLE; ++button) {
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                               .x = 10,
                                               .y = 10,
                                               .pointer_button = (cui_pointer_button)button}));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                               .x = 10,
                                               .y = 10,
                                               .pointer_button = (cui_pointer_button)button}));
        begin(ui);
        assert(cui_focused(ui) == 2);
        assert(cui_event_target(ui, 0) == 1 && cui_event_target(ui, 1) == 1);
        assert(!cui_event_activation(ui, 0) && !cui_event_activation(ui, 1));
        cui_interaction first = control(ui, 1, false);
        assert(!first.activated && !first.pressed);
        control(ui, 2, false);
        cui_end_frame(ui);
    }
    cui_rect second_bounds = cui_bounds(ui, 2);
    /* Secondary release cannot terminate an in-progress primary drag. */
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = second_bounds.x + 10,
                                           .y = second_bounds.y + 10,
                                           .pointer_button = CUI_POINTER_SECONDARY}));
    queue(ui, CUI_EVENT_POINTER_MOVE, 20, 10, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 1) == 2 && cui_event_target(ui, 2) == 1);
    assert(control(ui, 1, false).pressed);
    control(ui, 2, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_UP, 20, 10, 0, 0);
    begin(ui);
    assert(control(ui, 1, false).activated);
    control(ui, 2, false);
    cui_end_frame(ui);
    cui_destroy(ui);
}

static void offset_scene(cui_context *ui) {
    cui_begin_box(
        ui, 10,
        (cui_box){.width = 80, .height = 50, .clip = true, .offset_x = -10, .offset_y = -15});
    control(ui, 1, false);
    control(ui, 2, false);
    cui_end_box(ui);
}
static void test_clipped_child_translation(void) {
    cui_context *ui = make();
    begin(ui);
    offset_scene(ui);
    cui_draw_list list = cui_end_frame(ui);
    cui_rect first = cui_bounds(ui, 1), second = cui_bounds(ui, 2);
    assert(first.x == -10 && first.y == -15 && second.x == -10 && second.y == 15);
    bool clip = false, painted = false;
    for (size_t i = 0; i < list.count; ++i) {
        if (list.commands[i].type == CUI_DRAW_CLIP_PUSH) {
            clip = true;
            assert(list.commands[i].bounds.w == 80 && list.commands[i].bounds.h == 50);
        }
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].bounds.y == -15)
            painted = true;
    }
    assert(clip && painted);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 25, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 2);
    offset_scene(ui);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_CANCEL, 0, 0, 0, 0);
    queue(ui, CUI_EVENT_POINTER_DOWN, 85, 25, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 1) ==
           0); /* Shifted child exists beyond viewport, hit is clipped. */
    offset_scene(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_pointer_scope_guard(void) {
    cui_context *ui = make();
    assert(!cui_pointer_scope_allowed(ui, 0));
    begin(ui);
    assert(cui_pointer_scope_allowed(ui, 500));
    assert(cui_begin_focus_scope(ui, 100, true));
    assert(cui_pointer_scope_allowed(ui, 500));
    assert(cui_begin_focus_scope(ui, 200, false));
    assert(cui_pointer_scope_allowed(ui, 500));
    cui_end_focus_scope(ui);
    cui_end_focus_scope(ui);
    assert(!cui_pointer_scope_allowed(ui, 500));
    cui_end_frame(ui);
    begin(ui);
    assert(!cui_pointer_scope_allowed(ui, 500));
    assert(cui_pointer_scope_allowed(ui, 100));
    assert(cui_begin_focus_scope(ui, 100, true));
    assert(cui_pointer_scope_allowed(ui, 500));
    assert(cui_begin_focus_scope(ui, 300, true));
    cui_end_focus_scope(ui);
    assert(!cui_pointer_scope_allowed(ui, 500));
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    begin(ui);
    assert(!cui_pointer_scope_allowed(ui, 100));
    assert(cui_pointer_scope_allowed(ui, 300));
    assert(cui_begin_focus_scope(ui, 300, true));
    assert(cui_pointer_scope_allowed(ui, 500));
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    cui_destroy(ui);
}

static void scroll_regions(cui_context *ui, float inner_scroll, bool portal) {
    cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 160, .clip = true});
    cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Outer"), false, false, 0);
    cui_set_semantic_details(
        ui, 10, (cui_semantic_details){.scrollable = true, .scroll_y = 50, .scroll_max_y = 300});
    cui_begin_box(ui, 20, (cui_box){.width = 100, .height = 80, .clip = true});
    cui_interact(ui, 20, CUI_ROLE_GROUP, cui_string("Inner"), false, false, 0);
    cui_set_semantic_details(
        ui, 20,
        (cui_semantic_details){.scrollable = true, .scroll_y = inner_scroll, .scroll_max_y = 100});
    cui_end_box(ui);
    cui_end_box(ui);
    if (portal) {
        cui_begin_portal(ui, 30, (cui_box){0}, (cui_rect){0, 0, 100, 80}, 4);
        cui_interact(ui, 30, CUI_ROLE_BUTTON, cui_string("Popup"), false, false, 0);
        cui_end_box(ui);
    }
}
static void wheel(cui_context *ui, float x, float y, float dy) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = x, .y = y, .dy = dy}));
}
static void test_nested_wheel_routing(void) {
    cui_context *ui = make();
    begin(ui);
    scroll_regions(ui, 0, false);
    cui_end_frame(ui);
    wheel(ui, 20, 20, 1); /* Inner is already at its start: bubble upward. */
    wheel(ui, 20, 20, -1);
    wheel(ui, 120, 100, -1);
    wheel(ui, 250, 100, -1);
    begin(ui);
    assert(cui_event_target(ui, 0) == 10 && cui_event_target(ui, 1) == 20);
    assert(cui_event_target(ui, 2) == 10 && !cui_event_target(ui, 3));
    scroll_regions(ui, 100, false);
    cui_end_frame(ui);
    wheel(ui, 20, 20, -1); /* Inner is at its end. */
    wheel(ui, 20, 20, 1);
    begin(ui);
    assert(cui_event_target(ui, 0) == 10 && cui_event_target(ui, 1) == 20);
    scroll_regions(ui, 50, true);
    cui_end_frame(ui);
    wheel(ui, 20, 20, -1); /* An interactive higher portal blocks background scrolling. */
    begin(ui);
    assert(!cui_event_target(ui, 0));
    scroll_regions(ui, 50, false);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_table_semantic_roles(void) {
    cui_context *ui = make();
    begin(ui);
    cui_interact(ui, 1, CUI_ROLE_ROWGROUP, cui_string("Body"), false, false, 0);
    cui_interact(ui, 2, CUI_ROLE_ROWHEADER, cui_string("Row 2"), false, false, 0);
    cui_interact(ui, 3, CUI_ROLE_COLUMNHEADER, cui_string("Name"), false, false, 0);
    cui_interact(ui, 4, CUI_ROLE_GRIDCELL, cui_string("Alice"), false, false, 0);
    cui_interact(ui, 5, CUI_ROLE_ALERTDIALOG, cui_string("Confirm"), false, false, 0);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 2);
    cui_interact(ui, 2, CUI_ROLE_ROWHEADER, cui_string("Row 2"), false, false, 0);
    cui_interact(ui, 3, CUI_ROLE_COLUMNHEADER, cui_string("Name"), false, false, 0);
    cui_interact(ui, 4, CUI_ROLE_GRIDCELL, cui_string("Alice"), false, false, 0);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 3);
    cui_end_frame(ui);
    cui_destroy(ui);
}

static void test_custom_shape_commands(void) {
    cui_context *ui = make();
    begin(ui);
    cui_begin_box(ui, 99,
                  (cui_box){.width = 80,
                            .height = 40,
                            .radius = 9,
                            .background = {10, 20, 30, 255},
                            .border = {255, 0, 0, 255},
                            .border_width = 3,
                            .custom_corners = true,
                            .corner_radii = {7, 0, 0, 11},
                            .custom_border_edges = true,
                            .border_edges = {0, 4, 2, 6}});
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    bool fill = false, border = false;
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type != CUI_DRAW_RECT && c->type != CUI_DRAW_BORDER)
            continue;
        assert(c->custom_corners && c->corner_radii.top_left == 7 &&
               c->corner_radii.top_right == 0 && c->corner_radii.bottom_left == 0 &&
               c->corner_radii.bottom_right == 11);
        if (c->type == CUI_DRAW_RECT)
            fill = true;
        else {
            border = true;
            assert(c->custom_border_edges && c->border_edges.left == 0 &&
                   c->border_edges.right == 4 && c->border_edges.top == 2 &&
                   c->border_edges.bottom == 6);
        }
    }
    assert(fill && border);
    cui_destroy(ui);
}

static void test_transient_clip_lifecycle(void) {
    cui_context *ui = make();
    for (unsigned frame = 0; frame < 240; frame++) {
        begin(ui);
        for (unsigned clip = 0; clip < 4; clip++) {
            cui_id parent = 1000 + frame * 8 + clip * 2;
            cui_begin_box(ui, parent,
                          (cui_box){.width = 80, .height = 30, .clip = true, .offset_y = -7});
            cui_begin_box(ui, parent + 1,
                          (cui_box){.width = 40, .height = 60, .background = {255, 0, 0, 255}});
            cui_end_box(ui);
            cui_end_box(ui);
        }
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && list.count > 0);
        cui_rect parent = cui_bounds(ui, 1000 + frame * 8);
        cui_rect child = cui_bounds(ui, 1001 + frame * 8);
        assert(child.y == parent.y - 7 && parent.h == 30 && child.h == 60);
    }
    cui_destroy(ui);
}

static void test_many_simultaneous_clips(void) {
    cui_context *ui = cui_create(
        (cui_config){.width = 300, .height = 200, .max_elements = 512, .max_commands = 1024});
    assert(ui);
    for (unsigned frame = 0; frame < 4; frame++) {
        assert(cui_begin_frame(ui, 300, 200, .016f));
        for (unsigned clip = 0; clip < 160; clip++) {
            cui_id id = 5000 + frame * 400 + clip * 2;
            cui_begin_box(ui, id,
                          (cui_box){.width = 80, .height = 1, .clip = true, .offset_y = -2});
            cui_begin_box(ui, id + 1,
                          (cui_box){.width = 40, .height = 8, .background = {255, 0, 0, 255}});
            cui_end_box(ui);
            cui_end_box(ui);
        }
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && list.count > 0);
        cui_rect parent = cui_bounds(ui, 5000 + frame * 400);
        cui_rect child = cui_bounds(ui, 5001 + frame * 400);
        assert(child.y == parent.y - 2);
    }
    cui_destroy(ui);
}

static void tracked_frame(cui_context *ui, cui_rect *bounds, float height, bool present) {
    begin(ui);
    assert(cui_track_bounds(ui, 77, bounds));
    assert(cui_track_bounds(ui, 77, bounds));
    if (present) {
        cui_begin_box(ui, 77, (cui_box){.width = 80, .height = height});
        cui_end_box(ui);
    }
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_bounds_tracking(void) {
    cui_context *ui = make();
    cui_rect bounds = {0}, other = {0};
    assert(!cui_track_bounds(NULL, 77, &bounds));
    assert(!cui_track_bounds(ui, 77, &bounds) && cui_last_error(ui) == CUI_ERROR_LIFECYCLE);
    cui_clear_error(ui);
    tracked_frame(ui, &bounds, 40, true);
    assert(bounds.w == 80 && bounds.h == 40 && cui_needs_redraw(ui));
    tracked_frame(ui, &bounds, 40, true);
    assert(!cui_needs_redraw(ui));
    tracked_frame(ui, &bounds, 90, true);
    assert(bounds.h == 90 && cui_needs_redraw(ui));
    tracked_frame(ui, &bounds, 90, true);
    assert(!cui_needs_redraw(ui));
    tracked_frame(ui, &bounds, 20, true);
    assert(bounds.h == 20 && cui_needs_redraw(ui));
    tracked_frame(ui, &bounds, 20, false);
    assert(bounds.w == 0 && bounds.h == 0 && cui_needs_redraw(ui));
    tracked_frame(ui, &bounds, 20, false);
    assert(!cui_needs_redraw(ui));
    begin(ui);
    assert(!cui_track_bounds(ui, 0, &bounds) && !cui_track_bounds(ui, 77, NULL));
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_track_bounds(ui, 77, &bounds));
    assert(!cui_track_bounds(ui, 77, &other) && cui_last_error(ui) == CUI_ERROR_DUPLICATE_ID);
    cui_clear_error(ui);
    assert(!cui_track_bounds(ui, 78, &bounds) && cui_last_error(ui) == CUI_ERROR_DUPLICATE_ID);
    cui_clear_error(ui);
    cui_end_frame(ui);
    cui_destroy(ui);
    ui = cui_create((cui_config){.width = 300, .height = 200, .max_elements = 2});
    assert(ui);
    cui_rect slots[3] = {{0}};
    begin(ui);
    assert(cui_track_bounds(ui, 1, &slots[0]) && cui_track_bounds(ui, 2, &slots[1]));
    assert(!cui_track_bounds(ui, 3, &slots[2]) && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    cui_end_frame(ui);
    assert(!cui_needs_redraw(ui));
    cui_destroy(ui);
}

static void test_layout_only_and_zero_size(void) {
    cui_context *ui = make();
    cui_rect body = {0}, zero = {0}, child = {0};
    begin(ui);
    control(ui, 31, false);
    cui_end_frame(ui);
    cui_focus(ui, 31);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    cui_track_bounds(ui, 30, &body);
    cui_track_bounds(ui, 32, &zero);
    cui_track_bounds(ui, 33, &child);
    cui_begin_box(
        ui, 30,
        (cui_box){.width = 100, .layout_only = true, .clip = true, .background = {255, 0, 0, 255}});
    assert(!cui_focused(ui));
    assert(!cui_event_target(ui, 0) && !cui_event_activation(ui, 0));
    cui_focus(ui, 99);
    assert(cui_begin_focus_scope(ui, 70, true));
    assert(!control(ui, 31, false).activated);
    cui_text(ui, cui_string("Measured, not painted"), 16, (cui_color){255, 255, 255, 255});
    cui_begin_portal(ui, 40, (cui_box){.background = {0, 255, 0, 255}}, (cui_rect){10, 10, 80, 30},
                     100);
    control(ui, 41, false);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    cui_end_box(ui);
    cui_begin_box(ui, 32, (cui_box){.width = 100, .fixed_height = true, .clip = true});
    cui_begin_box(ui, 33, (cui_box){.width = 80, .height = 60});
    cui_end_box(ui);
    cui_end_box(ui);
    control(ui, 50, false);
    cui_draw_list dl = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(body.w == 100 && body.h > 30 && zero.h == 0 && child.h == 60);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].id == 50 && cui_focused(ui) == 0);
    int clips = 0;
    size_t paints = 0;
    for (size_t i = 0; i < dl.count; i++) {
        if (dl.commands[i].type == CUI_DRAW_CLIP_PUSH)
            clips++;
        else if (dl.commands[i].type == CUI_DRAW_CLIP_POP)
            clips--;
        else {
            paints++;
            assert(dl.commands[i].type == CUI_DRAW_RECT);
        }
        assert(clips >= 0);
    }
    assert(!clips && paints == 1);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    control(ui, 50, false);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 50);
    cui_destroy(ui);
}

static void test_inert_painted_subtree(void) {
    cui_context *ui = make();
    begin(ui);
    control(ui, 1, false);
    cui_end_frame(ui);
    cui_focus(ui, 1);
    queue(ui, CUI_EVENT_POINTER_MOVE, 10, 10, CUI_KEY_NONE, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    assert(cui_begin_focus_scope(ui, 100, true));
    cui_begin_box(ui, 20, (cui_box){.inert = true, .width = 100});
    assert(!cui_hovered(ui, 1) && !cui_focused(ui) && !cui_event_target(ui, 1));
    assert(!cui_dismiss_focus_scope(ui, 100, 1));
    assert(!control(ui, 1, false).activated);
    cui_begin_portal(ui, 30, (cui_box){0}, (cui_rect){120, 0, 100, 30}, 10);
    assert(!control(ui, 2, false).activated);
    cui_end_box(ui);
    cui_end_box(ui);
    control(ui, 3, false);
    cui_end_focus_scope(ui);
    cui_draw_list dl = cui_end_frame(ui);
    size_t count, paints = 0;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].id == 3 && cui_focused(ui) == 3);
    for (size_t i = 0; i < dl.count; i++)
        paints += dl.commands[i].type == CUI_DRAW_RECT;
    assert(paints == 3 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_semantic_role_refinement(void) {
    cui_context *ui = make();
    begin(ui);
    control(ui, 1, false);
    cui_set_semantic_details(ui, 1,
                             (cui_semantic_details){.description = cui_string("destination")});
    assert(cui_set_semantic_role(ui, 1, CUI_ROLE_LINK));
    assert(!cui_set_semantic_role(ui, 2, CUI_ROLE_LINK));
    assert(!cui_set_semantic_role(ui, 1, (cui_role)999));
    cui_end_frame(ui);
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    assert(n == 1 && s[0].role == CUI_ROLE_LINK && s[0].details.description.length == 11);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    assert(control(ui, 1, false).activated);
    assert(cui_set_semantic_role(ui, 1, CUI_ROLE_LINK));
    cui_end_frame(ui);
    assert(cui_focused(ui) == 1);
    begin(ui);
    control(ui, 1, true);
    assert(cui_set_semantic_role(ui, 1, CUI_ROLE_LINK));
    cui_end_frame(ui);
    assert(!cui_focused(ui));
    cui_destroy(ui);
}

static void test_pressed_query(void) {
    cui_context *ui = make();
    begin(ui);
    control(ui, 1, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, CUI_KEY_NONE, 0);
    begin(ui);
    assert(control(ui, 1, false).pressed && cui_pressed(ui, 1));
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_MOVE, 250, 180, CUI_KEY_NONE, 0);
    begin(ui);
    assert(control(ui, 1, false).pressed && cui_pressed(ui, 1) && !cui_hovered(ui, 1));
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_CANCEL, 0, 0, CUI_KEY_NONE, 0);
    begin(ui);
    control(ui, 1, false);
    assert(!cui_pressed(ui, 1));
    cui_end_frame(ui);
    cui_focus(ui, 1);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_SPACE, 0);
    begin(ui);
    assert(control(ui, 1, false).pressed && cui_pressed(ui, 1));
    cui_begin_box(ui, 2, (cui_box){.inert = true});
    assert(!cui_pressed(ui, 1));
    cui_end_box(ui);
    cui_end_frame(ui);
    begin(ui);
    control(ui, 1, true);
    assert(!cui_pressed(ui, 1));
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_UP, 0, 0, CUI_KEY_SPACE, 0);
    begin(ui);
    assert(!control(ui, 1, false).activated && !cui_pressed(ui, 1));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

int main(void) {
    test_layout_only_and_zero_size();
    test_inert_painted_subtree();
    test_semantic_role_refinement();
    test_pressed_query();
    test_custom_shape_commands();
    test_transient_clip_lifecycle();
    test_many_simultaneous_clips();
    test_bounds_tracking();
    test_menu_check_radio_roles();
    test_secondary_pointer();
    test_clipped_child_translation();
    test_pointer_scope_guard();
    test_nested_wheel_routing();
    test_table_semantic_roles();
    test_input();
    test_strings_and_draw();
    test_contexts_errors();
    test_clipped_hit_and_arena();
    test_numeric_id_text_domains();
    test_creation_overflow();
    test_focus_scopes();
    test_portals();
    test_child_alignment();
    test_invalid_portal_at_depth_limit();
    test_scope_dismissal();
    test_semantic_details_and_roving();
    puts("core tests passed");
    return 0;
}
