/* SPDX-License-Identifier: MIT */
#include "cui/cui_editor_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_edit *edit;
    char bytes[128], copies[16][128];
    cui_editor_state state;
    cui_edit_selection selections[4];
    cui_edit_selection_snapshot history[17];
    cui_editor_cursor_anchor anchors[4];
    cui_textarea_line lines[32];
    cui_editor_options options;
    cui_component_text_event events[16];
    unsigned count;
    bool omit;
    cui_draw_list draw;
} fixture;
static void observe(void *user, const cui_component_text_event *event) {
    fixture *f = user;
    unsigned i = f->count++;
    assert(i < 16);
    f->events[i] = *event;
    assert(event->after.revision == cui_edit_get_state(f->edit).revision);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(event->event.frame_sequence == cui_frame_sequence(f->ui));
    assert(event->event.kind == CUI_COMPONENT_EVENT_CHANGE && event->event.target == 10);
    if (event->text.length) {
        char too_small[1] = {'Q'};
        cui_str untouched = {"sentinel", 8};
        assert(cui_component_event_copy_utf8(event->text, 127, too_small, 1, &untouched) ==
               CUI_COMPONENT_EVENT_COPY_CAPACITY);
        assert(too_small[0] == 'Q' && untouched.length == 8);
    }
    if (event->has_text) {
        assert(cui_component_event_copy_utf8(event->text, 127, f->copies[i], 128,
                                             &f->events[i].text) == CUI_COMPONENT_EVENT_COPY_OK);
    } else
        assert(!event->text.data && !event->text.length);
    f->omit = true; /* Application flag only; omission is deferred to a later frame. */
}
static cui_multiline_event_result frame(fixture *f, bool facade, bool observer, bool omit) {
    assert(cui_begin_frame(f->ui, 400, 100, .016f));
    cui_theme theme = cui_theme_light();
    cui_multiline_event_result r = {0};
    cui_begin_box(f->ui, 1, (cui_box){.width = -1, .height = -1});
    if (!omit) {
        if (facade)
            r = cui_editor_events(f->ui, 10, &theme, cui_string("Name"), &f->state,
                                 f->options, (cui_component_text_handler){observer ? observe : NULL, f});
        else
            r.control = cui_editor(f->ui, 10, &theme, cui_string("Name"), &f->state, f->options);
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    f->ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->bytes, .capacity = 128, .multiline = true,
                                              .history_bytes = 4096, .max_history_steps = 16});
    assert(f->ui && f->edit);
    assert(cui_editor_state_init(&f->state, f->edit, f->lines, 32));
    f->options = cui_editor_default();
    f->options.textarea.width = 220; f->options.textarea.rows = 3;
    frame(f, false, false, false);
    cui_focus(f->ui, 10);
}
static void finish(fixture *f) { cui_destroy(f->ui); cui_edit_destroy(f->edit); }
static void queue(fixture *f, cui_event_type type, const char *text) {
    assert(cui_queue_event(f->ui, (cui_event){.type = type, .key = type == CUI_EVENT_KEY_DOWN ? CUI_KEY_ENTER : 0,
                          .text_data = text, .text_length = text ? strlen(text) : 0}));
}
static void equal(fixture *a, fixture *b, cui_textarea_result x, cui_textarea_result y) {
    assert(x.changed == y.changed && x.pressed_enter == y.pressed_enter && x.error == y.error &&
           x.edit_error == y.edit_error && x.selection_changed == y.selection_changed &&
           x.scrolled == y.scrolled && x.enter_shift == y.enter_shift &&
           x.enter_secondary == y.enter_secondary && x.visual_error == y.visual_error &&
           !strcmp(a->bytes, b->bytes));
    cui_edit_state sa = cui_edit_get_state(a->edit), sb = cui_edit_get_state(b->edit);
    assert(sa.revision == sb.revision && sa.cursor == sb.cursor && sa.anchor == sb.anchor);
    assert(sa.can_undo == sb.can_undo && sa.can_redo == sb.can_redo);
    size_t na, nb;
    const cui_semantic *ma = cui_semantics(a->ui, &na), *mb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i) {
        assert(ma[i].id == mb[i].id && ma[i].role == mb[i].role && ma[i].value == mb[i].value);
    }
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(!memcmp(&p->color, &q->color, sizeof p->color));
        assert(p->text.length == q->text.length);
        if (p->text.length) assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
}
static void pair_event(fixture *a, fixture *b, cui_event_type type, const char *text) {
    queue(a, type, text); queue(b, type, text);
}
static cui_multiline_event_result pair_frame(fixture *a, fixture *b, bool observer) {
    cui_multiline_event_result x = frame(a, false, false, false);
    cui_multiline_event_result y = frame(b, true, observer, false);
    equal(a, b, x.control, y.control);
    return y;
}
int main(void) {
    fixture a, b; init(&a); init(&b);
    pair_event(&a, &b, CUI_EVENT_TEXT, "a");
    pair_event(&a, &b, CUI_EVENT_KEY_DOWN, NULL);
    pair_event(&a, &b, CUI_EVENT_TEXT, "b");
    cui_multiline_event_result y = pair_frame(&a, &b, true);
    assert(y.change_count == 1 && b.count == 1 && !strcmp(b.copies[0], "a\nb"));
    assert(b.events[0].before.length == 0 && b.events[0].after.length == 3);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_START, NULL);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_UPDATE, "hi");
    y = pair_frame(&a, &b, true); assert(!y.change_count);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_COMMIT, "hi");
    y = pair_frame(&a, &b, true); assert(y.change_count == 1);
    assert(!strcmp(b.copies[0], "a\nb")); /* Owned copy survives later edits. */
    pair_event(&a, &b, CUI_EVENT_TEXT, "z");
    y = pair_frame(&a, &b, false); assert(y.control.changed && !y.change_count);
    y = pair_frame(&a, &b, true); assert(!y.change_count);
    assert(cui_edit_select(a.edit, 0, 1) == CUI_EDIT_OK);
    assert(cui_edit_select(b.edit, 0, 1) == CUI_EDIT_OK);
    y = pair_frame(&a, &b, true); assert(!y.change_count); /* External/selection-only. */
    assert(cui_queue_event(a.ui, (cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_Z,.modifiers=CUI_MOD_CTRL}));
    assert(cui_queue_event(b.ui, (cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_Z,.modifiers=CUI_MOD_CTRL}));
    y = pair_frame(&a, &b, true); assert(y.change_count == 1);
    a.options.textarea.readonly = b.options.textarea.readonly = true;
    pair_event(&a, &b, CUI_EVENT_TEXT, "ignored");
    y = pair_frame(&a, &b, true); assert(!y.change_count);
    a.options.textarea.readonly = b.options.textarea.readonly = false;
    a.options.textarea.submit_on_enter = b.options.textarea.submit_on_enter = true;
    pair_event(&a, &b, CUI_EVENT_KEY_DOWN, NULL);
    y = pair_frame(&a, &b, true); assert(y.control.pressed_enter && !y.change_count);
    assert(b.omit && b.draw.count);
    frame(&b, true, true, b.omit);
    size_t n; const cui_semantic *items = cui_semantics(b.ui, &n);
    for (size_t i = 0; i < n; ++i) assert(items[i].id != 10);
    finish(&a); finish(&b);
    /* A committed newline can exceed row storage: still notify despite layout error. */
    init(&a); init(&b);
    a.state.view.capacity = b.state.view.capacity = 1;
    pair_event(&a, &b, CUI_EVENT_TEXT, "x\ny");
    y = pair_frame(&a, &b, true);
    assert(y.control.error == CUI_TEXTAREA_CAPACITY && y.change_count == 1 &&
           !strcmp(b.copies[0], "x\ny"));
    /* Existing invalid layout before reduction must not apply another queued edit. */
    pair_event(&a, &b, CUI_EVENT_TEXT, "ignored");
    y = pair_frame(&a, &b, true); assert(!y.change_count && !strcmp(b.bytes,"x\ny"));
    finish(&a); finish(&b);
    /* Failed IME commit exceeds document capacity, without committed CHANGE. */
    init(&a); init(&b);
    char long_text[200]; memset(long_text, 'x', sizeof long_text - 1); long_text[199] = 0;
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_START, NULL);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_UPDATE, long_text);
    y = pair_frame(&a, &b, true); assert(!y.change_count);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_COMMIT, long_text);
    y = pair_frame(&a, &b, true);
    assert(!y.change_count && y.control.edit_error != CUI_EDIT_OK && !b.bytes[0]);
    a.options.textarea.disabled = b.options.textarea.disabled = true;
    pair_event(&a, &b, CUI_EVENT_TEXT, "ignored");
    y = pair_frame(&a, &b, true); assert(!y.change_count);
    finish(&a); finish(&b);
    /* Public Base facade: aggregation is one revision notification even when
     * text followed by Undo returns to the original bytes within one batch. */
    init(&b);
    queue(&b, CUI_EVENT_TEXT, "one");
    assert(cui_queue_event(b.ui, (cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_Z,
                                           .modifiers=CUI_MOD_CTRL}));
    assert(cui_begin_frame(b.ui, 400, 100, .016f));
    cui_textarea_metrics metrics;
    assert(cui_textarea_measure(b.options.textarea, 16, &metrics));
    y = cui_base_editor_events(b.ui, 10, cui_string("Name"), &b.state, b.options,
                                  metrics, (cui_component_text_handler){observe, &b});
    b.draw = cui_end_frame(b.ui);
    assert(y.change_count == 1 && y.control.changed && !b.bytes[0]);
    assert(b.events[0].before.revision != b.events[0].after.revision);
    assert(!b.events[0].before.length && !b.events[0].after.length);
    finish(&b);
    /* Completion acceptance occurs in the outer reducer, before its nested
     * Textarea pass. Pointer acceptance occurs later, after Styled main paint. */
    init(&a); init(&b);
    assert(cui_edit_insert(a.edit, cui_string("pri")) == CUI_EDIT_OK);
    assert(cui_edit_insert(b.edit, cui_string("pri")) == CUI_EDIT_OK);
    pair_frame(&a, &b, true);
    cui_editor_completion_item item = {cui_string("print"), {0}, cui_string("print()")};
    assert(cui_editor_show_completions(&a.state, &item, 1, 0, 3) == CUI_EDIT_OK);
    assert(cui_editor_show_completions(&b.state, &item, 1, 0, 3) == CUI_EDIT_OK);
    pair_event(&a, &b, CUI_EVENT_KEY_DOWN, NULL);
    y = pair_frame(&a, &b, true);
    assert(y.change_count == 1 && y.control.changed && !strcmp(b.bytes, "print()"));
    assert(!a.state.completion.open && !b.state.completion.open);
    assert(cui_edit_undo(a.edit) == CUI_EDIT_OK && cui_edit_undo(b.edit) == CUI_EDIT_OK);
    assert(cui_editor_show_completions(&a.state, &item, 1, 0, 3) == CUI_EDIT_OK);
    assert(cui_editor_show_completions(&b.state, &item, 1, 0, 3) == CUI_EDIT_OK);
    pair_frame(&a, &b, true);
    cui_rect popup = cui_bounds(b.ui, cui_editor_completion_item_id(10, 0));
    assert(popup.w > 0 && popup.h > 0);
    for (unsigned up = 0; up < 2; ++up) {
        cui_event event = {.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                          .x = popup.x + 4, .y = popup.y + 4,
                          .pointer_button = CUI_POINTER_PRIMARY};
        assert(cui_queue_event(a.ui, event) && cui_queue_event(b.ui, event));
        y = pair_frame(&a, &b, true);
        assert(y.change_count == (up ? 1u : 0u));
    }
    assert(y.control.changed && !strcmp(b.bytes, "print()") && !b.state.completion.open);
    finish(&a); finish(&b);
    /* One frame with two attached cursors replaces both but emits only once. */
    init(&a); init(&b);
    fixture *fs[2] = {&a, &b};
    for (unsigned i = 0; i < 2; ++i) {
        fixture *f = fs[i];
        assert(cui_edit_insert(f->edit, cui_string("ab\ncd")) == CUI_EDIT_OK);
        assert(cui_edit_attach_selections(f->edit, f->selections, 4, f->history, 17) == CUI_EDIT_OK);
        assert(cui_editor_multicursor_init(&f->state, f->anchors, 4));
        cui_edit_selection ranges[2] = {{1, 1, 1}, {2, 4, 4}};
        assert(cui_edit_select_many(f->edit, ranges, 2) == CUI_EDIT_OK);
    }
    pair_frame(&a, &b, true);
    pair_event(&a, &b, CUI_EVENT_TEXT, "!");
    y = pair_frame(&a, &b, true);
    assert(y.change_count == 1 && !strcmp(b.bytes, "a!b\nc!d"));
    assert(b.count == 1);
    size_t count; const cui_edit_selection *live = cui_edit_get_selections(b.edit, &count);
    assert(count == 2 && live[0].id == 1 && live[1].id == 2);
    finish(&a); finish(&b);
    puts("PASS aggregate Editor CHANGE, legacy equivalence, IME and post-mutation errors");
    return 0;
}
