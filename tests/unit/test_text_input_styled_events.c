/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_edit *edit;
    char bytes[128], copies[16][128];
    cui_input_state state;
    cui_input_options options;
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
    assert(!(event->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    if (event->has_text) {
        assert(cui_component_event_copy_utf8(event->text, 127, f->copies[i], 128,
                                             &f->events[i].text) == CUI_COMPONENT_EVENT_COPY_OK);
    } else
        assert(!event->text.data && !event->text.length);
    f->omit = true; /* Application flag only; omission is deferred to a later frame. */
}
static cui_input_event_result frame(fixture *f, bool facade, bool observer, bool omit) {
    assert(cui_begin_frame(f->ui, 400, 100, .016f));
    cui_theme theme = cui_theme_light();
    cui_input_event_result r = {0};
    cui_begin_box(f->ui, 1, (cui_box){.width = -1, .height = -1});
    if (!omit) {
        if (facade)
            r = cui_input_events(f->ui, 10, &theme, cui_string("Name"), f->edit, &f->state,
                                 f->options, (cui_component_text_handler){observer ? observe : NULL, f});
        else
            r.control = cui_input_ex(f->ui, 10, &theme, cui_string("Name"), f->edit,
                                      &f->state, f->options);
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    f->ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->bytes, .capacity = 128,
                                              .history_bytes = 4096, .max_history_steps = 16});
    assert(f->ui && f->edit);
    frame(f, false, false, false);
    cui_focus(f->ui, 10);
}
static void finish(fixture *f) { cui_destroy(f->ui); cui_edit_destroy(f->edit); }
static void queue(fixture *f, cui_event_type type, const char *text) {
    assert(cui_queue_event(f->ui, (cui_event){.type = type, .key = CUI_KEY_ENTER,
                          .text_data = text, .text_length = text ? strlen(text) : 0}));
}
static void equal(fixture *a, fixture *b, cui_input_result x, cui_input_result y) {
    assert(x.changed == y.changed && x.submitted == y.submitted && x.error == y.error &&
           x.rejected == y.rejected && !strcmp(a->bytes, b->bytes));
    cui_edit_state sa = cui_edit_get_state(a->edit), sb = cui_edit_get_state(b->edit);
    assert(sa.revision == sb.revision && sa.cursor == sb.cursor && sa.anchor == sb.anchor);
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(!memcmp(&p->color, &q->color, sizeof p->color));
        assert(p->text.length == q->text.length);
        if (p->text.length) assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
}
static void *fail_allocate(void *user, size_t bytes) { (void)user; (void)bytes; return NULL; }
static void fail_release(void *user, void *storage) { (void)user; (void)storage; assert(false); }
int main(void) {
    fixture a, b; init(&a); init(&b);
    for (unsigned mode = 0; mode < 3; ++mode) {
        a.options.masked = b.options.masked = mode == 1;
        a.options.content_type = b.options.content_type = mode == 2 ?
            CUI_TEXT_INPUT_PURPOSE_PASSWORD : CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED;
        unsigned start = b.count;
        for (unsigned i = 0; i < 2; ++i) {
            queue(&a, CUI_EVENT_TEXT, "a"); queue(&b, CUI_EVENT_TEXT, "a");
            queue(&a, CUI_EVENT_KEY_DOWN, NULL); queue(&b, CUI_EVENT_KEY_DOWN, NULL);
        }
        cui_input_event_result x = frame(&a, false, false, false);
        cui_input_event_result y = frame(&b, true, true, false);
        equal(&a, &b, x.control, y.control);
        assert(y.change_count == 2 && y.submit_count == 2 && b.count == start + 4);
        for (unsigned i = 0; i < 4; ++i) {
            assert(b.events[start+i].event.event_index == i);
            assert(b.events[start+i].has_text == (mode == 0));
        }
        if (!mode) assert(!strcmp(b.copies[0], "a") && !strcmp(b.copies[2], "aa"));
    }
    a.options = b.options = (cui_input_options){.masked = true};
    queue(&a, CUI_EVENT_COMPOSITION_START, NULL); queue(&b, CUI_EVENT_COMPOSITION_START, NULL);
    queue(&a, CUI_EVENT_COMPOSITION_UPDATE, "hi"); queue(&b, CUI_EVENT_COMPOSITION_UPDATE, "hi");
    cui_input_event_result x = frame(&a, false, false, false), y = frame(&b, true, true, false);
    equal(&a, &b, x.control, y.control); assert(!y.change_count);
    queue(&a, CUI_EVENT_COMPOSITION_COMMIT, "hi"); queue(&b, CUI_EVENT_COMPOSITION_COMMIT, "hi");
    x = frame(&a, false, false, false); y = frame(&b, true, true, false);
    equal(&a, &b, x.control, y.control); assert(y.change_count == 1 && !b.events[b.count-1].has_text);
    queue(&a, CUI_EVENT_TEXT, "z"); queue(&b, CUI_EVENT_TEXT, "z");
    x = frame(&a, false, false, false); y = frame(&b, true, false, false);
    equal(&a, &b, x.control, y.control); assert(y.control.changed && !y.change_count);
    assert(b.omit && b.draw.count); /* Callback did not remove current submission. */
    frame(&b, true, true, b.omit);
    size_t n; const cui_semantic *items = cui_semantics(b.ui, &n);
    for (size_t i = 0; i < n; ++i) assert(items[i].id != 10);
    finish(&a); finish(&b);
    init(&a); init(&b);
    a.options = b.options = (cui_input_options){.masked = true,
        .mask_allocator = {.allocate = fail_allocate, .release = fail_release}};
    queue(&a, CUI_EVENT_TEXT, "secret"); queue(&b, CUI_EVENT_TEXT, "secret");
    x = frame(&a, false, false, false); y = frame(&b, true, true, false);
    equal(&a, &b, x.control, y.control);
    assert(y.control.error != CUI_EDIT_OK && !y.change_count && !y.submit_count && !b.count);
    assert(!a.bytes[0] && !b.bytes[0]);
    finish(&a); finish(&b);
    puts("PASS Styled Input ordered observers, legacy paint equivalence, IME and secure lifetime");
    return 0;
}
