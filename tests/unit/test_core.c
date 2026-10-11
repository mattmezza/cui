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
int main(void) {
    test_input();
    test_strings_and_draw();
    test_contexts_errors();
    test_clipped_hit_and_arena();
    test_numeric_id_text_domains();
    test_creation_overflow();
    test_focus_scopes();
    puts("core tests passed");
    return 0;
}
