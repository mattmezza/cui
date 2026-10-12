#include "cui/cui.h"
#include <assert.h>
#include <math.h>
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
static cui_size styled_measure(void *user, cui_str text, float size, cui_text_style style) {
    float scale = *(float *)user;
    return (cui_size){text.length *
                          (10 + 3 * style.weight + 2 * style.italic + 7 * style.monospace) * scale,
                      size + style.weight};
}
static void test_styled_text_pipeline(void) {
    float scale = 1;
    cui_context *ui = cui_create((cui_config){
        .width = 300, .height = 200, .measure_styled = styled_measure, .measure_user = &scale});
    assert(ui);
    cui_text_style bold = {.weight = CUI_TEXT_WEIGHT_BOLD};
    cui_text_style mono = {.monospace = true, .italic = true};
    assert(cui_measure_text_ex(ui, cui_string("abc"), 14, bold).width == 57);
    assert(cui_measure_text_ex(ui, cui_string("abc"), 14, mono).width == 57);
    assert(cui_measure_text(ui, cui_string("abc"), 14).width == 30);
    assert(cui_measure_text_ex(ui, cui_string("abc"), 14,
                               (cui_text_style){.weight = (cui_text_weight)99})
               .width == 0);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_text_ex(ui, cui_string("abc"), 14, (cui_color){1, 2, 3, 255}, 30, bold);
    cui_text_ex(ui, cui_string("abc"), 14, (cui_color){1, 2, 3, 255}, 0, mono);
    cui_text(ui, cui_string("abc"), 14, (cui_color){1, 2, 3, 255});
    cui_draw_list draw = cui_end_frame(ui);
    size_t seen = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_draw_command c = draw.commands[i];
            if (seen == 0)
                assert(c.text_style.weight == CUI_TEXT_WEIGHT_BOLD && c.bounds.w == 57 &&
                       c.bounds.y > 0);
            else if (seen == 1)
                assert(c.text_style.monospace && c.text_style.italic && c.bounds.w == 57);
            else
                assert(c.text_style.weight == CUI_TEXT_WEIGHT_NORMAL && !c.text_style.italic &&
                       !c.text_style.monospace && c.bounds.w == 30);
            seen++;
        }
    assert(seen == 3 && cui_last_error(ui) == CUI_OK);
    scale = 2;
    cui_text_metrics_changed(ui);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_text_ex(ui, cui_string("abc"), 14, (cui_color){1, 2, 3, 255}, 0, bold);
    draw = cui_end_frame(ui);
    bool doubled = false;
    for (size_t i = 0; i < draw.count; i++)
        doubled |= draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].bounds.w == 114;
    assert(doubled && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void queue(cui_context *ui, cui_event_type type, float x, float y, cui_key key,
                  unsigned mods) {
    assert(cui_queue_event(
        ui, (cui_event){.type = type, .x = x, .y = y, .key = key, .modifiers = mods}));
}
static void scope_event_scene(cui_context *ui, bool check) {
    assert(cui_begin_focus_scope(ui, 900, false));
    assert(cui_begin_focus_scope(ui, 901, false));
    if (check) {
        assert(cui_event_within_scope(ui, 0, 900));
        assert(cui_event_within_scope(ui, 0, 901));
        assert(cui_event_hit_within_scope(ui, 0, 901));
        assert(!cui_event_within_scope(ui, 0, 999));
        assert(!cui_event_within_scope(ui, 1, 900));
        assert(!cui_event_consumed(ui, 0));
    }
    control(ui, 1, false);
    cui_end_focus_scope(ui);
    cui_end_focus_scope(ui);
    assert(cui_begin_focus_scope(ui, 902, false));
    if (check)
        assert(!cui_event_within_scope(ui, 0, 902));
    control(ui, 2, false);
    cui_end_focus_scope(ui);
}
static void test_event_scope(void) {
    cui_context *ui = make();
    begin(ui);
    scope_event_scene(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    begin(ui);
    assert(!cui_event_within_scope(ui, 0, 900)); /* Not yet submitted. */
    scope_event_scene(ui, true);
    cui_end_frame(ui);
    assert(!cui_event_within_scope(ui, 0, 900));
    cui_destroy(ui);
}
static void test_scroll_gesture_metadata(void) {
    cui_context *ui = make();
    for (unsigned phase = CUI_SCROLL_PHASE_STARTED; phase <= CUI_SCROLL_PHASE_CANCELLED; ++phase)
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .scroll_precise = true,
                                               .scroll_phase = (cui_scroll_phase)phase}));
    begin(ui);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    assert(count == 4);
    for (size_t i = 0; i < count; ++i)
        assert(events[i].scroll_precise && events[i].scroll_phase == (cui_scroll_phase)(i + 1) &&
               events[i].dx == 0 && events[i].dy == 0);
    cui_begin_box(ui, 1, (cui_box){.width = 100, .height = 40});
    cui_interact(ui, 1, CUI_ROLE_GROUP, cui_string("Gallery"), false, false, 0);
    assert(cui_set_semantic_role(ui, 1, CUI_ROLE_REGION));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_semantics(ui, &count)[0].role == CUI_ROLE_REGION && count == 1);
    cui_destroy(ui);
}
static void test_activation_cancellation(void) {
    cui_context *ui = make();
    begin(ui);
    control(ui, 1, false);
    cui_end_frame(ui);
    /* Cancel the second of two clicks without erasing the first. */
    for (unsigned i = 0; i < 2; ++i) {
        queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
        queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    }
    begin(ui);
    assert(!cui_cancel_event_activation(ui, 0));
    assert(!cui_cancel_event_activation(ui, 4));
    assert(cui_cancel_event_activation(ui, 3));
    assert(!cui_cancel_event_activation(ui, 3));
    assert(cui_event_target(ui, 3) == 1 && !cui_event_activation(ui, 3));
    assert(cui_event_activation(ui, 1) == 1);
    assert(control(ui, 1, false).activated);
    assert(!cui_cancel_event_activation(ui, 1));
    cui_end_frame(ui);
    assert(!cui_pressed(ui, 1));
    assert(!cui_cancel_event_activation(ui, 1));
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    begin(ui);
    assert(cui_cancel_event_activation(ui, 1));
    assert(!control(ui, 1, false).activated);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    assert(cui_cancel_event_activation(ui, 0));
    assert(!control(ui, 1, false).activated);
    cui_end_frame(ui);
    assert(cui_focused(ui) == 1 && !cui_pressed(ui, 1));
    cui_destroy(ui);
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
    assert(cui_begin_focus_scope(ui, 920, false));
    cui_begin_box(ui, 20, (cui_box){.width = 100, .height = 80, .clip = true});
    cui_interact(ui, 20, CUI_ROLE_GROUP, cui_string("Inner"), false, false, 0);
    cui_set_semantic_details(
        ui, 20,
        (cui_semantic_details){.scrollable = true, .scroll_y = inner_scroll, .scroll_max_y = 100});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
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
    assert(cui_event_hit_within_scope(ui, 0, 920));
    assert(!cui_event_within_scope(ui, 0, 920));
    assert(!cui_event_hit_within_scope(ui, 2, 920));
    assert(!cui_event_hit_within_scope(ui, 3, 920));
    assert(!cui_defer_scroll_event(ui, 1, 999));
    assert(cui_defer_scroll_event(ui, 1, 20));
    assert(cui_event_target(ui, 1) == 10);
    assert(cui_event_hit_within_scope(ui, 1, 920));
    assert(!cui_defer_scroll_event(ui, 1, 20));
    assert(!cui_defer_scroll_event(ui, 1, 10));
    cui_end_frame(ui);
    wheel(ui, 20, 20, -1); /* Inner is at its end. */
    wheel(ui, 20, 20, 1);
    begin(ui);
    assert(cui_event_target(ui, 0) == 10 && cui_event_target(ui, 1) == 20);
    scroll_regions(ui, 50, true);
    assert(cui_consume_event(ui, 1));
    assert(!cui_defer_scroll_event(ui, 1, 20));
    assert(cui_event_target(ui, 1) == 20);
    cui_end_frame(ui);
    wheel(ui, 20, 20, -1); /* An interactive higher portal blocks background scrolling. */
    begin(ui);
    assert(!cui_event_target(ui, 0));
    scroll_regions(ui, 50, false);
    assert(!cui_event_hit_within_scope(ui, 0, 920));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void focusable_log(cui_context *ui, bool enabled, bool disabled, bool tabstop) {
    cui_begin_box(ui, 40, (cui_box){.width = 200, .height = 150});
    cui_interact(ui, 40, CUI_ROLE_LOG, cui_string("Messages"), disabled, false, 0);
    cui_set_semantic_details(ui, 40, (cui_semantic_details){.focusable = enabled});
    cui_set_tab_stop(ui, 40, tabstop);
    control(ui, 41, disabled);
    cui_end_box(ui);
}
static void test_focusable_container(void) {
    cui_context *ui = make();
    begin(ui);
    focusable_log(ui, false, false, true);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 41); /* Passive log is skipped. */
    focusable_log(ui, true, false, true);
    cui_focus(ui, 0);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 40);
    focusable_log(ui, true, false, true);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    begin(ui);
    assert(cui_focused(ui) == 41); /* Child takes precedence over container. */
    focusable_log(ui, true, false, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_DOWN, 150, 100, 0, 0);
    begin(ui);
    assert(cui_focused(ui) == 40); /* Blank region remains pointer focusable. */
    focusable_log(ui, true, false, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_END, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 40);
    focusable_log(ui, true, true, true);
    cui_end_frame(ui);
    assert(!cui_focused(ui));
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static bool pointer_card(cui_context *ui, bool disabled) {
    cui_begin_box(ui, 70, (cui_box){.width = 200, .height = 150});
    bool activated =
        cui_interact(ui, 70, CUI_ROLE_GROUP, cui_string("Card"), disabled, false, 0).activated;
    cui_set_semantic_details(ui, 70, (cui_semantic_details){.pointer_activatable = true});
    control(ui, 71, false);
    cui_end_box(ui);
    return activated;
}

static bool proxy_button_scene(cui_context *ui) {
    cui_begin_box(ui, 70, (cui_box){.width = 100, .height = 30});
    bool activated =
        cui_interact(ui, 70, CUI_ROLE_BUTTON, cui_string("Increment"), false, false, 0).activated;
    cui_set_semantic_details(
        ui, 70,
        (cui_semantic_details){.no_focus = true, .pointer_activatable = true, .focus_target = 71});
    cui_end_box(ui);
    control(ui, 71, false);
    return activated;
}
static void test_nonfocusable_proxy_button(void) {
    cui_context *ui = make();
    begin(ui);
    proxy_button_scene(ui);
    cui_end_frame(ui);
    cui_focus(ui, 70);
    assert(!cui_focused(ui));
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 71);
    assert(!proxy_button_scene(ui));
    cui_focus(ui, 70);
    assert(cui_focused(ui) == 71);
    cui_end_frame(ui);
    cui_focus(ui, 0);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    begin(ui);
    assert(cui_focused(ui) == 71 && cui_event_target(ui, 0) == 70);
    assert(!proxy_button_scene(ui));
    cui_end_frame(ui);
    assert(cui_pressed(ui, 70));
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    begin(ui);
    assert(proxy_button_scene(ui));
    cui_end_frame(ui);
    assert(!cui_pressed(ui, 70) && cui_focused(ui) == 71);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    assert(!proxy_button_scene(ui));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_pointer_only_container(void) {
    cui_context *ui = make();
    begin(ui);
    assert(!pointer_card(ui, false));
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 71);
    pointer_card(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 150, 100, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 150, 100, 0, 0);
    begin(ui);
    assert(cui_focused(ui) == 71 && cui_event_target(ui, 0) == 70);
    assert(pointer_card(ui, false));
    cui_end_frame(ui);
    /* Passive pointer containers retain capture between event-loop frames. */
    queue(ui, CUI_EVENT_POINTER_DOWN, 150, 100, 0, 0);
    begin(ui);
    assert(!pointer_card(ui, false));
    cui_end_frame(ui);
    assert(cui_pressed(ui, 70));
    queue(ui, CUI_EVENT_POINTER_MOVE, 160, 100, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 70);
    pointer_card(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_UP, 160, 100, 0, 0);
    begin(ui);
    assert(pointer_card(ui, false));
    cui_end_frame(ui);
    assert(!cui_pressed(ui, 70));
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_ENTER, 0);
    begin(ui);
    assert(!pointer_card(ui, false));
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 71 && !pointer_card(ui, false));
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 150, 100, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 250, 200, 0, 0);
    begin(ui);
    assert(!pointer_card(ui, false));
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 150, 100, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 150, 100, 0, 0);
    begin(ui);
    assert(!pointer_card(ui, true));
    cui_end_frame(ui);
    assert(cui_focused(ui) == 71 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_underlay_order_and_layout(void) {
    cui_context *ui = make();
    assert(!cui_set_underlay(ui, (cui_rect){0, 0, 20, 10}, (cui_color){0}, (cui_corner_radii){0}));
    begin(ui);
    cui_begin_box(
        ui, 80, (cui_box){.width = 100, .height = 40, .clip = true, .background = {1, 2, 3, 255}});
    assert(cui_set_underlay(ui, (cui_rect){-10, 5, 130, 20}, (cui_color){4, 5, 6, 255},
                            (cui_corner_radii){1, 2, 3, 4}));
    assert(!cui_set_underlay(ui, (cui_rect){0, 0, -1, 20}, (cui_color){0}, (cui_corner_radii){0}));
    cui_text(ui, cui_string("Above indicator"), 12, (cui_color){7, 8, 9, 255});
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    size_t background = SIZE_MAX, underlay = SIZE_MAX, label = SIZE_MAX;
    int clips = 0;
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type == CUI_DRAW_RECT && c->color.r == 1)
            background = i;
        if (c->type == CUI_DRAW_RECT && c->color.r == 4) {
            underlay = i;
            assert(clips > 0 && c->bounds.x == -10 && c->bounds.y == 5 && c->bounds.w == 130);
            assert(c->custom_corners && c->corner_radii.bottom_right == 4);
        }
        if (c->type == CUI_DRAW_TEXT)
            label = i;
        if (c->type == CUI_DRAW_CLIP_PUSH)
            clips++;
        if (c->type == CUI_DRAW_CLIP_POP)
            clips--;
        assert(clips >= 0);
    }
    assert(!clips && background < underlay && underlay < label);
    cui_rect box = cui_bounds(ui, 80);
    assert(box.w == 100 && box.h == 40 && cui_last_error(ui) == CUI_OK);
    size_t semantics;
    cui_semantics(ui, &semantics);
    assert(!semantics);
    cui_destroy(ui);
}

static void test_underlay_capacity_and_hidden(void) {
    cui_context *ui = cui_create((cui_config){.width = 100, .height = 100, .max_commands = 2});
    assert(ui && cui_begin_frame(ui, 100, 100, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 40, .height = 40});
    assert(cui_set_underlay(ui, (cui_rect){0, 0, 20, 20}, (cui_color){255, 0, 0, 255},
                            (cui_corner_radii){0}));
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
    ui = make();
    begin(ui);
    cui_begin_box(ui, 1, (cui_box){.width = 40, .height = 40, .layout_only = true});
    assert(cui_set_underlay(ui, (cui_rect){0, 0, 20, 20}, (cui_color){255, 0, 0, 255},
                            (cui_corner_radii){0}));
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_perceptual_color_mix(void) {
    cui_color black = {0, 0, 0, 255}, white = {255, 255, 255, 255};
    cui_color middle = cui_color_mix_oklab(black, white, .5f);
    /* Half Oklab lightness encodes approximately sRGB 0.389, not RGB 0.5. */
    assert(middle.r >= 98 && middle.r <= 100);
    assert(middle.r == middle.g && middle.g == middle.b && middle.a == 255);
    cui_color red = {255, 0, 0, 255}, transparent_white = {255, 255, 255, 0};
    cui_color faded = cui_color_mix_oklab(red, transparent_white, .75f);
    assert(faded.r == 255 && faded.g == 0 && faded.b == 0 && faded.a == 64);
    cui_color purple = cui_color_mix_oklab(red, (cui_color){0, 0, 255, 255}, .5f);
    assert(purple.r >= 139 && purple.r <= 141 && purple.g >= 82 && purple.g <= 84 &&
           purple.b >= 161 && purple.b <= 163);
    assert(cui_color_mix_oklab(red, white, -1).r == 255);
    assert(cui_color_mix_oklab(red, white, NAN).g == 0);
    assert(cui_color_mix_oklab(red, white, 2).g == 255);
    assert(cui_color_mix_oklab(transparent_white, (cui_color){255, 0, 0, 0}, .5f).a == 0);
}

static void test_dominant_axis_scroll(void) {
    cui_context *ui = make();
    for (int horizontal = 0; horizontal < 2; horizontal++) {
        begin(ui);
        scroll_regions(ui, 50, false);
        assert(cui_set_semantic_role(ui, 20, CUI_ROLE_LOG));
        cui_set_semantic_details(ui, 10,
                                 (cui_semantic_details){.scrollable = true,
                                                        .scroll_x = 50,
                                                        .scroll_y = 50,
                                                        .scroll_max_x = 100,
                                                        .scroll_max_y = 100});
        cui_set_semantic_details(
            ui, 20,
            (cui_semantic_details){.scrollable = true,
                                   .scroll_axis_lock = true,
                                   .orientation = horizontal ? CUI_ORIENTATION_HORIZONTAL
                                                             : CUI_ORIENTATION_VERTICAL,
                                   .scroll_x = 50,
                                   .scroll_y = 50,
                                   .scroll_max_x = 100,
                                   .scroll_max_y = 100});
        cui_end_frame(ui);
        const float dx[] = {-4, -1, -2, 0};
        const float dy[] = {-1, -4, -2, 0};
        for (size_t i = 0; i < 4; i++)
            assert(cui_queue_event(
                ui,
                (cui_event){.type = CUI_EVENT_SCROLL, .x = 20, .y = 20, .dx = dx[i], .dy = dy[i]}));
        begin(ui);
        assert(cui_event_target(ui, 0) == (horizontal ? 20 : 10));
        assert(cui_event_target(ui, 1) == (horizontal ? 10 : 20));
        assert(cui_event_target(ui, 2) == (horizontal ? 10 : 20));
        assert(!cui_event_target(ui, 3));
        scroll_regions(ui, 0, false);
        assert(cui_set_semantic_role(ui, 20, CUI_ROLE_LISTITEM));
        cui_set_semantic_details(ui, 20,
                                 (cui_semantic_details){.scrollable = true,
                                                        .scroll_axis_lock = true,
                                                        .orientation = CUI_ORIENTATION_VERTICAL,
                                                        .scroll_max_y = 100});
        cui_end_frame(ui);
        wheel(ui, 20, 20, 1);
        begin(ui);
        assert(cui_event_target(ui, 0) == 10); /* Axis lock preserves edge chaining. */
        cui_end_frame(ui);
    }
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void gesture_regions(cui_context *ui, bool reorder, bool portal, bool enabled) {
    if (reorder)
        cui_interact(ui, 99, CUI_ROLE_GROUP, cui_string("Unrelated"), false, false, 0);
    scroll_regions(ui, 50, portal);
    cui_set_semantic_details(ui, 20,
                             (cui_semantic_details){.scrollable = true,
                                                    .scroll_axis_lock = true,
                                                    .scroll_gesture_lock = enabled,
                                                    .scroll_trap_horizontal = true,
                                                    .orientation = CUI_ORIENTATION_HORIZONTAL,
                                                    .scroll_x = 50,
                                                    .scroll_max_x = 100});
}
static void gesture_event(cui_context *ui, float x, float dx, float dy, cui_scroll_phase phase,
                          bool precise) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = x,
                                           .y = 20,
                                           .dx = dx,
                                           .dy = dy,
                                           .scroll_phase = phase,
                                           .scroll_precise = precise}));
}
static void test_persistent_scroll_routing(void) {
    cui_context *ui = make();
    begin(ui);
    gesture_regions(ui, false, false, true);
    cui_end_frame(ui);
    gesture_event(ui, 20, -40, -10, CUI_SCROLL_PHASE_STARTED, true);
    begin(ui);
    assert(cui_event_target(ui, 0) == 20);
    assert(cui_frame_events(ui, NULL)[0].dy == 0);
    gesture_regions(ui, true, false, true);
    cui_end_frame(ui);
    /* Inserting an unrelated semantic must not transfer the gesture to another ID. */
    gesture_event(ui, 20, -10, -15, CUI_SCROLL_PHASE_MOVED, true);
    begin(ui);
    assert(cui_event_target(ui, 0) == 20);
    assert(cui_frame_events(ui, NULL)[0].dy == 0);
    gesture_regions(ui, false, false, true);
    cui_end_frame(ui);
    gesture_event(ui, 20, -10, -25, CUI_SCROLL_PHASE_MOVED, true);
    begin(ui);
    assert(cui_event_target(ui, 0) == 10);
    assert(cui_frame_events(ui, NULL)[0].dy == -25);
    gesture_regions(ui, false, false, true);
    cui_end_frame(ui);
    /* Ending the gesture permits a fresh horizontal lock in the same batch. */
    gesture_event(ui, 20, 0, 0, CUI_SCROLL_PHASE_ENDED, true);
    gesture_event(ui, 20, -40, -10, CUI_SCROLL_PHASE_MOVED, true);
    gesture_event(ui, 20, -10, -15, CUI_SCROLL_PHASE_MOVED, true);
    begin(ui);
    assert(cui_event_target(ui, 2) == 20);
    gesture_regions(ui, false, true, true);
    cui_end_frame(ui);
    gesture_event(ui, 20, -10, -15, CUI_SCROLL_PHASE_MOVED, true);
    begin(ui);
    assert(!cui_event_target(ui, 0));
    assert(cui_frame_events(ui, NULL)[0].dy == -15); /* Occluded owner cannot filter. */
    gesture_regions(ui, false, false, true);
    cui_end_frame(ui);
    gesture_event(ui, 120, -40, -10, CUI_SCROLL_PHASE_STARTED, true);
    begin(ui);
    assert(cui_event_target(ui, 0) == 10);
    assert(cui_frame_events(ui, NULL)[0].dy == -10); /* Outside child's clip. */
    gesture_regions(ui, false, false, false);
    cui_end_frame(ui);
    gesture_event(ui, 20, -10, -15, CUI_SCROLL_PHASE_MOVED, true);
    begin(ui);
    assert(cui_event_target(ui, 0) == 10);
    gesture_regions(ui, false, false, true);
    cui_end_frame(ui);
    gesture_event(ui, 20, -10, -15, CUI_SCROLL_PHASE_MOVED, true);
    begin(ui);
    assert(cui_event_target(ui, 0) == 10); /* Re-enabled owner starts fresh vertically. */
    assert(cui_frame_events(ui, NULL)[0].dx == 0);
    gesture_regions(ui, false, false, true);
    cui_end_frame(ui);
    gesture_event(ui, 20, -40, -10, CUI_SCROLL_PHASE_NONE, false);
    begin(ui);
    assert(cui_frame_events(ui, NULL)[0].dy == -10); /* Line wheels remain unfiltered. */
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

static void proxy_scene(cui_context *ui, cui_id target, bool disabled) {
    control(ui, 1, false);
    assert(cui_set_semantic_role(ui, 1, CUI_ROLE_GROUP));
    cui_set_semantic_details(ui, 1, (cui_semantic_details){.focus_target = target});
    control(ui, 2, disabled);
}
static void test_pointer_focus_proxy(void) {
    cui_context *ui = make();
    begin(ui);
    proxy_scene(ui, 2, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, CUI_KEY_NONE, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 10, 10, CUI_KEY_NONE, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "abc"}));
    begin(ui);
    assert(cui_focused(ui) == 2 && cui_event_target(ui, 2) == 2);
    assert(cui_event_target(ui, 0) == 1 && !cui_pressed(ui, 1) && !cui_event_activation(ui, 1));
    proxy_scene(ui, 2, false);
    cui_end_frame(ui);
    begin(ui);
    proxy_scene(ui, 2, true);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, CUI_KEY_NONE, 0);
    begin(ui);
    assert(!cui_focused(ui));
    proxy_scene(ui, 999, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 10, 10, CUI_KEY_NONE, 0);
    begin(ui);
    assert(!cui_focused(ui));
    proxy_scene(ui, 999, false);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_focus_within_scopes(void) {
    cui_context *ui = make();
    begin(ui);
    assert(cui_begin_focus_scope(ui, 100, false));
    control(ui, 1, false);
    assert(cui_begin_focus_scope(ui, 200, false));
    control(ui, 2, false);
    cui_focus(ui, 2);
    assert(cui_focus_within(ui, 100) && cui_focus_within(ui, 200));
    cui_end_focus_scope(ui);
    cui_end_focus_scope(ui);
    control(ui, 3, false);
    cui_end_frame(ui);
    assert(cui_focus_within(ui, 100) && cui_focus_within(ui, 200) && !cui_focus_within(ui, 999));
    begin(ui);
    assert(cui_focus_within(ui, 200));
    assert(cui_begin_focus_scope(ui, 100, false));
    control(ui, 1, false);
    assert(cui_begin_focus_scope(ui, 200, false));
    control(ui, 2, false);
    cui_begin_box(ui, 4, (cui_box){.inert = true});
    assert(!cui_focus_within(ui, 200));
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    assert(cui_dismiss_focus_scope(ui, 200, 1));
    assert(cui_focus_within(ui, 100) && !cui_focus_within(ui, 200));
    cui_end_focus_scope(ui);
    control(ui, 3, false);
    cui_focus(ui, 3);
    assert(!cui_focus_within(ui, 100));
    cui_end_frame(ui);
    assert(!cui_focus_within(ui, 100) && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_box_maximum_axes(void) {
    cui_context *ui = make();
    for (int mode = 0; mode < 3; mode++) {
        cui_rect r = {0};
        begin(ui);
        cui_track_bounds(ui, 1, &r);
        cui_begin_box(ui, 1,
                      (cui_box){.width = mode == 0   ? 0
                                         : mode == 1 ? -1
                                                     : 100,
                                .max_width = 40,
                                .max_height = 20,
                                .clip = true});
        cui_begin_box(ui, 2, (cui_box){.width = 80, .height = 60});
        cui_end_box(ui);
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(r.w == 40 && r.h == 20 && cui_last_error(ui) == CUI_OK);
    }
    cui_rect short_box = {0}, zero = {0};
    begin(ui);
    cui_track_bounds(ui, 1, &short_box);
    cui_track_bounds(ui, 3, &zero);
    cui_begin_box(ui, 1, (cui_box){.max_width = 100});
    cui_begin_box(ui, 2, (cui_box){.width = 20, .height = 10});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, 3, (cui_box){.fixed_width = true, .max_width = 40, .height = 10});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(short_box.w == 20 && zero.w == 0);
    cui_destroy(ui);
}

static void hover_scope_frame(cui_context *ui, bool overlay) {
    assert(cui_begin_focus_scope(ui, 700, false));
    control(ui, 701, false);
    cui_begin_portal(ui, 702, (cui_box){0}, (cui_rect){120, 0, 30, 30}, 2);
    cui_interact(ui, 702, CUI_ROLE_BUTTON, cui_string("remove"), false, false, 0);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    if (overlay) {
        cui_begin_portal(ui, 703, (cui_box){0}, (cui_rect){120, 0, 30, 30}, 3);
        cui_interact(ui, 703, CUI_ROLE_BUTTON, cui_string("other"), false, false, 0);
        cui_end_box(ui);
    }
}
static void test_hover_scope(void) {
    cui_context *ui = make();
    begin(ui);
    hover_scope_frame(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_MOVE, 125, 5, 0, 0);
    begin(ui);
    assert(cui_hover_within(ui, 700));
    hover_scope_frame(ui, true);
    cui_end_frame(ui);
    begin(ui);
    assert(!cui_hover_within(ui, 700));
    hover_scope_frame(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_MOVE, 250, 150, 0, 0);
    begin(ui);
    assert(!cui_hover_within(ui, 700));
    cui_end_frame(ui);
    cui_destroy(ui);
}
static void test_anchored_underlay(void) {
    cui_context *ui = make();
    for (int width = 80; width <= 140; width += 60) {
        begin(ui);
        cui_begin_box(ui, 80, (cui_box){.width = (float)width, .height = 30});
        assert(cui_set_underlay_ex(ui,
                                   (cui_underlay_layout){.bounds = {0, 0, 0, 1},
                                                         .grow_width = true,
                                                         .vertical = CUI_ALIGN_END},
                                   (cui_color){17, 18, 19, 255}, (cui_corner_radii){0}));
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(list.count == 3 && list.commands[1].type == CUI_DRAW_RECT);
        assert(list.commands[1].bounds.w == width && list.commands[1].bounds.y == 29 &&
               list.commands[1].bounds.h == 1);
    }
    cui_destroy(ui);
}
static void test_explicit_text_line_height(void) {
    cui_context *ui = make();
    begin(ui);
    cui_begin_box(ui, 55, (cui_box){.width = 100});
    cui_interact(ui, 55, CUI_ROLE_TOOLBAR, cui_string("Tools"), false, false, 0);
    cui_text_with_line_height(ui, cui_string("one\ntwo"), 10, (cui_color){1, 2, 3, 255}, 24);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    cui_rect bounds;
    bounds = cui_bounds(ui, 55);
    assert(bounds.h == 48);
    assert(list.count == 2 && list.commands[1].bounds.y - list.commands[0].bounds.y == 24);
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_TAB, 0);
    begin(ui);
    assert(cui_focused(ui) == 0);
    cui_begin_box(ui, 55, (cui_box){.width = 100});
    cui_text_with_line_height(ui, cui_string("one\ntwo"), 10, (cui_color){1, 2, 3, 255}, 0);
    cui_end_box(ui);
    list = cui_end_frame(ui);
    bounds = cui_bounds(ui, 55);
    assert(bounds.h == 26);
    assert(list.count == 2 && list.commands[1].bounds.y - list.commands[0].bounds.y == 13);
    cui_destroy(ui);
}
static void test_bitmap_leaf(void) {
    unsigned char pixels[24] = {255};
    cui_context *ui = make();
    cui_bitmap_paint paint = {.bitmap = {pixels, 2, 2, 12, 7}, .opacity = .5f};
    assert(cui_bitmap_valid(paint.bitmap));
    assert(!cui_bitmap_valid((cui_bitmap){pixels, 2, 2, 7, 0}));
    begin(ui);
    cui_bitmap_paint bad = paint;
    bad.source = (cui_rect){1, 0, 2, 2};
    assert(!cui_paint_bitmap(ui, 11, bad, (cui_box){0}));
    assert(cui_paint_bitmap(ui, 11, paint, (cui_box){.radius = 1}));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].type == CUI_DRAW_IMAGE);
    const cui_draw_command *d = &list.commands[0];
    assert(d->bitmap.bitmap.pixels == pixels && d->bitmap.bitmap.revision == 7);
    assert(d->bitmap.source.w == 2 && d->bitmap.source.h == 2);
    assert(d->bounds.w == 2 && d->bounds.h == 2 && d->corner_radii.top_left == 1);
    begin(ui);
    assert(cui_paint_bitmap(ui, 11, paint, (cui_box){.layout_only = true}));
    assert(cui_end_frame(ui).count == 0);
    begin(ui);
    paint.source = (cui_rect){1, 0, 1, 2};
    assert(cui_paint_bitmap(ui, 11, paint, (cui_box){.width = 20, .height = 30}));
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].bounds.w == 20 && list.commands[0].bounds.h == 30);
    cui_destroy(ui);
}
static void absolute_scene(cui_context *ui, bool overlay) {
    cui_begin_box(ui, 800, (cui_box){.width = 100, .height = 80, .clip = true});
    cui_begin_box(ui, 801, (cui_box){.width = 100, .height = 40});
    cui_end_box(ui);
    cui_begin_absolute_box(ui, 802, (cui_box){.background = {255, 0, 0, 255}},
                           (cui_rect){95, 10, 9, 40});
    cui_interact(ui, 802, CUI_ROLE_SEPARATOR, cui_string("Resize"), false, false, 50);
    cui_set_semantic_details(
        ui, 802, (cui_semantic_details){.focusable = true, .cursor = CUI_CURSOR_RESIZE_HORIZONTAL});
    cui_end_box(ui);
    cui_begin_box(ui, 803, (cui_box){.width = 100, .height = 40});
    cui_end_box(ui);
    cui_end_box(ui);
    if (overlay) {
        cui_begin_portal(ui, 804, (cui_box){.background = {0, 0, 255, 255}},
                         (cui_rect){90, 0, 20, 60}, 2);
        cui_interact(ui, 804, CUI_ROLE_BUTTON, cui_string("Overlay"), false, false, 0);
        cui_end_box(ui);
    }
}
static void test_absolute_boxes(void) {
    cui_context *ui = make();
    begin(ui);
    absolute_scene(ui, false);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_bounds(ui, 803).y == 40); /* Absolute child consumes no flow space. */
    cui_rect bounds = cui_bounds(ui, 802);
    assert(bounds.x == 95 && bounds.y == 10 && bounds.w == 9 && bounds.h == 40);
    bool clipped = false, painted = false;
    for (size_t i = 0; i < list.count; i++) {
        if (list.commands[i].type == CUI_DRAW_CLIP_PUSH)
            clipped = list.commands[i].bounds.w == 100;
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].color.r == 255) {
            assert(clipped);
            painted = true;
        }
    }
    assert(painted);
    queue(ui, CUI_EVENT_POINTER_DOWN, 102, 20, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 102, 20, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 0); /* Clipped tail is not interactive. */
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_DEFAULT);
    absolute_scene(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 97, 20, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 97, 20, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 802 && cui_focused(ui) == 802);
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_RESIZE_HORIZONTAL);
    absolute_scene(ui, true);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 97, 20, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 97, 20, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 804); /* Parent layer stays below portals. */
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_DEFAULT);
    absolute_scene(ui, true);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    begin(ui);
    cui_begin_box(ui, 901, (cui_box){.width = 160, .height = 120, .padding = 20});
    cui_begin_box(ui, 902, (cui_box){.width = -1, .height = -1, .clip = true});
    cui_begin_absolute_box(ui, 903, (cui_box){.background = {1, 2, 3, 255}},
                           (cui_rect){4, 0, 9, -1});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    bounds = cui_bounds(ui, 903);
    assert(bounds.x == 24 && bounds.y == 20 && bounds.w == 9 && bounds.h == 80);
    assert(cui_last_error(ui) == CUI_OK);
    begin(ui);
    cui_begin_absolute_box(ui, 900, (cui_box){0}, (cui_rect){0, 0, 10, 10});
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0);
    assert(cui_last_error(ui) == CUI_ERROR_LAYOUT);
    cui_destroy(ui);
}
static void test_inherited_opacity(void) {
    cui_context *ui = make();
    unsigned char rgba[] = {255, 255, 255, 255};
    begin(ui);
    cui_begin_box(ui, 910,
                  (cui_box){.width = 200,
                            .height = 150,
                            .background = {1, 0, 0, 200},
                            .opacity = .5f,
                            .custom_opacity = true});
    cui_begin_box(ui, 911,
                  (cui_box){.width = 100,
                            .height = 70,
                            .background = {2, 0, 0, 200},
                            .border = {3, 0, 0, 200},
                            .border_width = 1,
                            .opacity = .5f,
                            .custom_opacity = true});
    assert(cui_set_underlay(ui, (cui_rect){0, 0, 5, 5}, (cui_color){4, 0, 0, 200},
                            (cui_corner_radii){0}));
    cui_text(ui, cui_string("Opacity"), 10, (cui_color){5, 0, 0, 200});
    assert(cui_paint_bitmap(ui, 912,
                            (cui_bitmap_paint){.bitmap = {rgba, 1, 1, 4, 1}, .opacity = .8f},
                            (cui_box){.width = 5, .height = 5}));
    cui_end_box(ui);
    cui_begin_portal(ui, 913, (cui_box){.background = {6, 0, 0, 200}}, (cui_rect){220, 10, 20, 20},
                     1);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    unsigned found = 0;
    for (size_t i = 0; i < list.count; i++) {
        cui_draw_command d = list.commands[i];
        if (d.type == CUI_DRAW_IMAGE) {
            assert(fabsf(d.bitmap.opacity - .2f) < .0001f);
            found |= 1u << 7;
        } else if (d.type == CUI_DRAW_RECT || d.type == CUI_DRAW_BORDER ||
                   d.type == CUI_DRAW_TEXT) {
            assert(d.color.r >= 1 && d.color.r <= 6);
            assert(d.color.a == (d.color.r == 1 || d.color.r == 6 ? 100 : 50));
            found |= 1u << d.color.r;
        }
    }
    assert(found == 254);
    begin(ui);
    cui_begin_box(
        ui, 920,
        (cui_box){.width = 20, .height = 20, .background = {1, 2, 3, 255}, .custom_opacity = true});
    cui_interact(ui, 920, CUI_ROLE_BUTTON, cui_string("Invisible"), false, false, 0);
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].color.a == 0);
    size_t count;
    assert(cui_semantics(ui, &count) && count == 1); /* Opacity does not imply inert. */
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_fractional_text_metrics(void) {
    cui_context *ui = make();
    cui_size measured = cui_measure_text(ui, cui_string("one"), 10.5f);
    assert(fabsf(measured.width - 18.9f) < .001f);
    for (unsigned frame = 0; frame < 140; frame++) {
        float height = frame % 3 == 0 ? 22.652476f : 20.25f + frame * .001f;
        begin(ui);
        cui_begin_box(ui, 930, (cui_box){.width = 100});
        cui_text_with_line_height(ui, cui_string("one\ntwo"), 10.5f, (cui_color){1, 2, 3, 255},
                                  height);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        assert(fabsf(cui_bounds(ui, 930).h - 2 * height) < .001f);
        assert(list.count == 2 && list.commands[0].font_size == 10.5f);
        assert(fabsf(list.commands[1].bounds.y - list.commands[0].bounds.y - height) < .001f);
        assert(fabsf(list.commands[0].bounds.y - (height - measured.height) * .5f) < .001f);
    }
    begin(ui);
    cui_begin_box(ui, 931, (cui_box){.width = 25});
    cui_text_with_line_height(ui, cui_string("one two"), 10.5f, (cui_color){1, 2, 3, 255}, 21.75f);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 2 && fabsf(cui_bounds(ui, 931).h - 43.5f) < .001f);
    assert(fabsf(list.commands[1].bounds.y - list.commands[0].bounds.y - 21.75f) < .001f);
    cui_destroy(ui);
}
static void ordered_absolute_scene(cui_context *ui, bool hidden) {
    cui_begin_box(ui, 940, (cui_box){.width = 100, .height = 60, .clip = true});
    /* Register handle behavior before children, but paint it as a floating child. */
    cui_interact(ui, 941, CUI_ROLE_GROUP, cui_string("Handle"), false, false, 0);
    cui_set_semantic_role(ui, 941, CUI_ROLE_SEPARATOR);
    cui_set_semantic_details(
        ui, 941, (cui_semantic_details){.focusable = true, .cursor = CUI_CURSOR_RESIZE_HORIZONTAL});
    cui_begin_absolute_box(ui, 941, (cui_box){.background = {1, 2, 3, 255}, .inert = hidden},
                           (cui_rect){95, 0, 9, -1});
    cui_end_box(ui);
    cui_begin_box(ui, 942, (cui_box){.width = 100, .height = 60});
    cui_interact(ui, 942, CUI_ROLE_BUTTON, cui_string("Pane child"), false, false, 0);
    cui_end_box(ui);
    cui_end_box(ui);
}
static void test_layout_hit_order(void) {
    cui_context *ui = make();
    begin(ui);
    ordered_absolute_scene(ui, false);
    cui_end_frame(ui);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 2 && sem[0].role == CUI_ROLE_SEPARATOR);
    queue(ui, CUI_EVENT_POINTER_DOWN, 97, 20, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 941 && cui_pressed(ui, 941));
    assert(!cui_hovered(ui, 942));
    ordered_absolute_scene(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_MOVE, 200, 20, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 941);
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_RESIZE_HORIZONTAL);
    ordered_absolute_scene(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_UP, 200, 20, 0, 0);
    begin(ui);
    ordered_absolute_scene(ui, true);
    cui_end_frame(ui);
    sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].id == 942); /* Paint-time inert applies to early behavior. */
    queue(ui, CUI_EVENT_POINTER_DOWN, 97, 20, 0, 0);
    queue(ui, CUI_EVENT_POINTER_UP, 97, 20, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 942);
    ordered_absolute_scene(ui, false);
    cui_end_frame(ui);
    queue(ui, CUI_EVENT_POINTER_DOWN, 102, 20, 0, 0);
    begin(ui);
    assert(cui_event_target(ui, 0) == 0);
    ordered_absolute_scene(ui, false);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_percentage_dimensions(void) {
    cui_context *ui = make();
    for (unsigned frame = 0; frame < 2; frame++) {
        float width = frame ? 300 : 200;
        begin(ui);
        cui_begin_box(ui, 960,
                      (cui_box){.width = width, .height = 100, .padding = 10, .row = true});
        cui_begin_box(
            ui, 961,
            (cui_box){.width = .25f, .height = .5f, .width_percent = true, .height_percent = true});
        cui_end_box(ui);
        cui_begin_box(
            ui, 962,
            (cui_box){.width = .75f, .height = 1, .width_percent = true, .height_percent = true});
        cui_end_box(ui);
        cui_end_box(ui);
        cui_end_frame(ui);
        cui_rect a = cui_bounds(ui, 961), b = cui_bounds(ui, 962);
        assert(a.x == 10 && a.y == 10 && a.w == (width - 20) * .25f && a.h == 40);
        assert(b.x == 10 + a.w && b.w == (width - 20) * .75f && b.h == 80);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}
static void test_scoped_focus_and_consumption(void) {
    cui_context *ui = make();
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_RIGHT, 0);
    begin(ui);
    assert(!cui_event_consumed(ui, 0));
    assert(cui_consume_event(ui, 0) && cui_event_consumed(ui, 0));
    assert(cui_consume_event(ui, 0) && !cui_consume_event(ui, 1));
    control(ui, 1000, false);
    assert(cui_begin_focus_scope(ui, 1100, false));
    control(ui, 1001, false);
    control(ui, 1002, true);
    control(ui, 1003, false);
    cui_set_tab_stop(ui, 1003, false);
    assert(cui_begin_focus_scope(ui, 1101, false));
    control(ui, 1004, false);
    cui_end_focus_scope(ui);
    cui_interact(ui, 1005, CUI_ROLE_BUTTON, cui_string("inert after behavior"), false, false, 0);
    cui_begin_box(ui, 1005, (cui_box){.inert = true});
    cui_end_box(ui);
    cui_focus(ui, 1001);
    assert(cui_focus_step_within(ui, 1100, true) && cui_focused(ui) == 1004);
    assert(cui_focus_step_within(ui, 1100, true) && cui_focused(ui) == 1001);
    assert(cui_focus_step_within(ui, 1100, false) && cui_focused(ui) == 1004);
    assert(cui_focus_step_within(ui, 1101, true) && cui_focused(ui) == 1004);
    cui_focus(ui, 1000);
    assert(!cui_focus_step_within(ui, 1100, true) && cui_focused(ui) == 1000);
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    assert(!cui_consume_event(ui, 0) && !cui_focus_step_within(ui, 1100, true));
    queue(ui, CUI_EVENT_KEY_DOWN, 0, 0, CUI_KEY_LEFT, 0);
    begin(ui);
    assert(!cui_event_consumed(ui, 0));
    assert(cui_begin_focus_scope(ui, 1100, false));
    control(ui, 1001, false);
    cui_focus(ui, 1001);
    assert(cui_begin_focus_scope(ui, 1102, true));
    control(ui, 1006, false);
    cui_end_focus_scope(ui);
    assert(cui_focus_step_within(ui, 1100, true) && cui_focused(ui) == 1006);
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    cui_destroy(ui);
}
static unsigned resource_collects, resource_destroys;
static void resource_collect(void *payload) {
    unsigned *value = payload;
    assert(*value == 42);
    resource_collects++;
}
static void resource_destroy(void *payload) {
    unsigned *value = payload;
    assert(*value == 42);
    resource_destroys++;
}
static void test_context_resources(void) {
    cui_context *ui = cui_create(
        (cui_config){.width = 100, .height = 100, .max_resources = 2, .resource_bytes = 8});
    assert(ui);
    assert(!cui_resource_get(ui, 1, 1, 4, resource_collect, resource_destroy));
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 100, 100, .25f));
    assert(cui_frame_time(ui) == .25 && cui_frame_delta(ui) == .25f);
    assert(!cui_reduced_motion(ui));
    cui_set_reduced_motion(ui, true);
    assert(cui_reduced_motion(ui) && cui_needs_redraw(ui));
    unsigned *a = cui_resource_get(ui, 1, 1, sizeof(*a), resource_collect, resource_destroy);
    unsigned *b = cui_resource_get(ui, 1, 2, sizeof(*b), resource_collect, resource_destroy);
    assert(a && b && a != b && !*a && !*b);
    *a = *b = 42;
    assert(cui_resource_get(ui, 1, 1, sizeof(*a), resource_collect, resource_destroy) == a);
    assert(!resource_collects && !resource_destroys);
    assert(!cui_resource_get(ui, 2, 1, 1, NULL, NULL));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    assert(!cui_resource_get(ui, 1, 1, 3, resource_collect, resource_destroy));
    assert(cui_last_error(ui) == CUI_ERROR_LIFECYCLE);
    cui_clear_error(ui);
    cui_end_frame(ui);
    /* Failed begin does not reclaim payloads or advance time. */
    assert(!cui_begin_frame(ui, 100, 100, NAN));
    assert(cui_frame_time(ui) == .25 && !resource_collects);
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 100, 100, .5f));
    assert(resource_collects == 2 && resource_destroys == 0 && cui_frame_time(ui) == .75);
    assert(cui_resource_get(ui, 1, 1, 4, resource_collect, resource_destroy) == a);
    cui_end_frame(ui);
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(resource_collects == 3 && resource_destroys == 1);
    /* Untouched previous-frame resource has released both slot and bytes. */
    unsigned *c = cui_resource_get(ui, 2, 1, 4, resource_collect, resource_destroy);
    assert(c && !*c);
    *c = 42;
    assert(cui_resource_get(ui, 1, 1, 4, resource_collect, resource_destroy) == a);
    cui_end_frame(ui);
    cui_destroy(ui);
    assert(resource_destroys == 3);
    assert(!cui_create((cui_config){.max_resources = SIZE_MAX}));
}
static void test_bitmap_underlay(void) {
    cui_context *ui = make();
    unsigned char pixels[] = {255, 0, 0, 255, 0, 0, 255, 128};
    cui_bitmap_paint paint = {.bitmap = {pixels, 1, 2, 4, 7}, .opacity = .8f};
    for (unsigned pass = 0; pass < 2; ++pass) {
        float width = pass ? 160 : 80;
        begin(ui);
        cui_begin_box(ui, 1200,
                      (cui_box){.width = width,
                                .height = 40,
                                .background = {10, 20, 30, 255},
                                .opacity = .5f,
                                .custom_opacity = true});
        assert(cui_set_bitmap_underlay(ui,
                                       (cui_underlay_layout){.bounds = {2, 3, -4, -6},
                                                             .grow_width = true,
                                                             .grow_height = true},
                                       paint, (cui_corner_radii){2, 3, 4, 5}));
        cui_bitmap_paint invalid = paint;
        invalid.source.w = 2;
        invalid.source.h = 2;
        assert(
            !cui_set_bitmap_underlay(ui, (cui_underlay_layout){0}, invalid, (cui_corner_radii){0}));
        cui_text(ui, cui_string("foreground"), 12, (cui_color){255, 255, 255, 255});
        cui_end_box(ui);
        cui_draw_list draw = cui_end_frame(ui);
        size_t image = SIZE_MAX, text = SIZE_MAX;
        for (size_t i = 0; i < draw.count; ++i) {
            if (draw.commands[i].type == CUI_DRAW_IMAGE)
                image = i;
            if (draw.commands[i].type == CUI_DRAW_TEXT)
                text = i;
        }
        assert(image != SIZE_MAX && image < text && image > 0);
        assert(draw.commands[image - 1].type == CUI_DRAW_CLIP_PUSH &&
               draw.commands[image + 1].type == CUI_DRAW_CLIP_POP);
        const cui_draw_command *d = &draw.commands[image];
        assert(d->bounds.x == 2 && d->bounds.y == 3 && d->bounds.w == width - 4 &&
               d->bounds.h == 34);
        assert(d->bitmap.bitmap.pixels == pixels && d->bitmap.source.h == 2 &&
               d->bitmap.bitmap.revision == 7 && fabsf(d->bitmap.opacity - .4f) < .001f);
        assert(d->custom_corners && d->corner_radii.bottom_right == 5);
    }
    begin(ui);
    cui_begin_box(ui, 1200, (cui_box){.width = 80, .height = 40});
    assert(cui_set_bitmap_underlay(ui, (cui_underlay_layout){.bounds = {0, 0, 20, 20}}, paint,
                                   (cui_corner_radii){0}));
    assert(cui_set_underlay(ui, (cui_rect){0, 0, 10, 10}, (cui_color){1, 2, 3, 255},
                            (cui_corner_radii){0}));
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    for (size_t i = 0; i < draw.count; ++i)
        assert(draw.commands[i].type != CUI_DRAW_IMAGE);
    cui_destroy(ui);
}
int main(void) {
    test_styled_text_pipeline();
    test_bitmap_underlay();
    test_context_resources();
    test_scoped_focus_and_consumption();
    test_percentage_dimensions();
    test_layout_hit_order();
    test_fractional_text_metrics();
    test_inherited_opacity();
    test_absolute_boxes();
    test_bitmap_leaf();
    test_explicit_text_line_height();
    test_anchored_underlay();
    test_hover_scope();
    test_layout_only_and_zero_size();
    test_inert_painted_subtree();
    test_semantic_role_refinement();
    test_pressed_query();
    test_pointer_focus_proxy();
    test_focus_within_scopes();
    test_box_maximum_axes();
    test_custom_shape_commands();
    test_transient_clip_lifecycle();
    test_many_simultaneous_clips();
    test_bounds_tracking();
    test_menu_check_radio_roles();
    test_secondary_pointer();
    test_clipped_child_translation();
    test_pointer_scope_guard();
    test_nested_wheel_routing();
    test_dominant_axis_scroll();
    test_persistent_scroll_routing();
    test_perceptual_color_mix();
    test_focusable_container();
    test_nonfocusable_proxy_button();
    test_pointer_only_container();
    test_underlay_order_and_layout();
    test_underlay_capacity_and_hidden();
    test_table_semantic_roles();
    test_scroll_gesture_metadata();
    test_event_scope();
    test_activation_cancellation();
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
