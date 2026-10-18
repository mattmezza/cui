/* SPDX-License-Identifier: MIT */
#include "cui/cui_text_input_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_component_text_event events[32];
    char copies[32][128];
    unsigned count, app_changes;
    unsigned *completed;
} log_t;
static void notify(void *user, const cui_component_text_event *event) {
    log_t *log = user;
    unsigned i = log->count++;
    assert(i < 32);
    ++log->app_changes;
    if (log->completed)
        assert(*log->completed > 0); /* Existing completion precedes observer. */
    log->events[i] = *event;
    assert(!(event->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    if (event->has_text) {
        cui_str copy = {0};
        assert(cui_component_event_copy_utf8(event->text, 127, log->copies[i], 128, &copy) ==
               CUI_COMPONENT_EVENT_COPY_OK);
        log->events[i].text = copy;
        char small[1] = {'Q'};
        cui_str unchanged = {"sentinel", 8};
        if (event->text.length) {
            assert(cui_component_event_copy_utf8(event->text, 127, small, sizeof small,
                                                 &unchanged) == CUI_COMPONENT_EVENT_COPY_CAPACITY);
            assert(small[0] == 'Q' && unchanged.length == 8);
        }
    } else
        assert(!event->text.data && !event->text.length);
}
typedef struct {
    cui_context *ui;
    cui_edit *edit;
    char buffer[128];
    cui_input_behavior_state state;
    cui_base_input_options options;
    log_t log;
} fixture;
static cui_input_event_result frame(fixture *f, bool observer) {
    assert(cui_begin_frame(f->ui, 400, 100, .016f));
    cui_begin_box(f->ui, 10, (cui_box){.width = 220, .height = 40});
    cui_input_event_result result =
        cui_base_input_events(f->ui, 10, cui_string("Input"), f->edit, &f->state, f->options,
                              (cui_component_text_handler){observer ? notify : NULL, &f->log});
    cui_end_box(f->ui);
    cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return result;
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    f->ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->buffer,
                                                .capacity = sizeof f->buffer,
                                                .history_bytes = 4096,
                                                .max_history_steps = 16});
    assert(f->ui && f->edit);
    frame(f, true);
    cui_focus(f->ui, 10);
}
static void finish(fixture *f) {
    cui_destroy(f->ui);
    cui_edit_destroy(f->edit);
}
static void key(fixture *f, cui_key k, unsigned modifiers, bool repeat) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                              .key = k,
                                              .modifiers = modifiers,
                                              .repeat = repeat}));
}
static void text(fixture *f, const char *value) {
    assert(cui_queue_event(
        f->ui,
        (cui_event){.type = CUI_EVENT_TEXT, .text_data = value, .text_length = strlen(value)}));
}
static void evidence(const cui_component_text_event *e, cui_component_event_kind kind, size_t index,
                     cui_component_event_source source) {
    assert(e->event.kind == kind && e->event.target == 10 && e->event.event_index == index);
    assert(e->event.source == source && e->event.frame_sequence > 0);
    assert((e->event.flags & (CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX)) ==
           (CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX));
}
static void batches(void) {
    fixture f;
    init(&f);
    text(&f, "a");
    key(&f, CUI_KEY_ENTER, 0, false);
    text(&f, "b");
    key(&f, CUI_KEY_ENTER, 0, false);
    key(&f, CUI_KEY_ENTER, 0, true);
    cui_input_event_result result = frame(&f, true);
    assert(result.control.changed && result.control.submitted && result.change_count == 2 &&
           result.submit_count == 2);
    assert(f.log.count == 4 && f.log.app_changes == 4 && !strcmp(f.buffer, "ab"));
    evidence(&f.log.events[0], CUI_COMPONENT_EVENT_CHANGE, 0, CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    evidence(&f.log.events[1], CUI_COMPONENT_EVENT_SUBMIT, 1, CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    evidence(&f.log.events[2], CUI_COMPONENT_EVENT_CHANGE, 2, CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    evidence(&f.log.events[3], CUI_COMPONENT_EVENT_SUBMIT, 3, CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(!strcmp(f.log.copies[0], "a") && !strcmp(f.log.copies[1], "a") &&
           !strcmp(f.log.copies[3], "ab"));
    assert(f.log.events[0].after.revision != f.log.events[0].before.revision);
    assert(f.log.events[1].after.revision == f.log.events[1].before.revision);
    result = frame(&f, true);
    assert(!result.change_count && !result.submit_count && f.log.count == 4);
    key(&f, CUI_KEY_LEFT, 0, false);
    result = frame(&f, true);
    assert(!result.change_count && f.log.count == 4);
    /* Missing observer does not disable editing; no callback counts are invented. */
    text(&f, "X");
    result = frame(&f, false);
    assert(result.control.changed && !result.change_count && f.log.count == 4);
    key(&f, CUI_KEY_Z, CUI_MOD_CTRL, false);
    result = frame(&f, true);
    assert(result.change_count == 1);
    key(&f, CUI_KEY_Z, CUI_MOD_CTRL | CUI_MOD_SHIFT, false);
    result = frame(&f, true);
    assert(result.change_count == 1);
    assert(!strcmp(f.log.copies[0], "a")); /* Bounded owned copy survived model mutations. */
    f.options.readonly = true;
    text(&f, "forbidden");
    result = frame(&f, true);
    assert(!result.control.changed && !result.change_count);
    key(&f, CUI_KEY_ENTER, 0, false);
    result = frame(&f, true);
    assert(result.submit_count == 1 && !result.change_count); /* Legacy readonly Enter. */
    f.options.disabled = true;
    key(&f, CUI_KEY_ENTER, 0, false);
    result = frame(&f, true);
    assert(!result.submit_count);
    finish(&f);
}
static bool accept(void *user, cui_str candidate) {
    (void)user;
    return !memchr(candidate.data, 'X', candidate.length);
}
static void composition(fixture *f, cui_event_type type, const char *value) {
    assert(cui_queue_event(
        f->ui,
        (cui_event){.type = type, .text_data = value, .text_length = value ? strlen(value) : 0}));
}
static void ime_and_secure(void) {
    fixture f;
    init(&f);
    f.options.validate = accept;
    composition(&f, CUI_EVENT_COMPOSITION_START, NULL);
    composition(&f, CUI_EVENT_COMPOSITION_UPDATE, "hi");
    cui_input_event_result result = frame(&f, true);
    assert(!result.change_count && !f.buffer[0] && f.log.count == 0);
    composition(&f, CUI_EVENT_COMPOSITION_COMMIT, "hi");
    result = frame(&f, true);
    assert(result.change_count == 1 && !strcmp(f.buffer, "hi"));
    evidence(&f.log.events[0], CUI_COMPONENT_EVENT_CHANGE, 0, CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    composition(&f, CUI_EVENT_COMPOSITION_START, NULL);
    composition(&f, CUI_EVENT_COMPOSITION_UPDATE, "X");
    result = frame(&f, true);
    assert(result.control.rejected && !result.change_count);
    composition(&f, CUI_EVENT_COMPOSITION_COMMIT, "X");
    result = frame(&f, true);
    assert(result.control.rejected && !result.change_count && !strcmp(f.buffer, "hi") &&
           !f.state.composition->active);
    text(&f, "X");
    result = frame(&f, true);
    assert(result.control.rejected && !result.change_count);
    key(&f, CUI_KEY_ENTER, 0, false);
    result = frame(&f, true);
    assert(result.submit_count == 1); /* Not form validation. */
    f.options.masked = true;
    text(&f, "secret");
    result = frame(&f, true);
    assert(result.change_count == 1 && !f.log.events[f.log.count - 1].has_text);
    f.options.masked = false;
    f.options.content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD;
    key(&f, CUI_KEY_ENTER, 0, false);
    result = frame(&f, true);
    assert(result.submit_count == 1 && !f.log.events[f.log.count - 1].has_text);
    finish(&f);
}
static cui_str paste(void *user) {
    return cui_string(user);
}
typedef struct {
    unsigned prepared, completed;
} timing;
static cui_edit_result prepare(void *user, cui_str candidate, bool provisional) {
    timing *t = user;
    (void)candidate;
    (void)provisional;
    ++t->prepared;
    return CUI_EDIT_OK;
}
static void complete(void *user, bool prepared, bool applied) {
    timing *t = user;
    assert(prepared && applied);
    ++t->completed;
}
static void paste_and_single(void) {
    fixture f;
    init(&f);
    timing t = {0};
    cui_platform platform = {
        .capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_get = paste, .user = (void *)"pasted"};
    f.options.clipboard = &platform;
    f.options.presentation =
        (cui_edit_presentation){.prepare = prepare, .user = &t, .user_bytes = sizeof t};
    f.options.presentation_complete = complete;
    f.options.presentation_user = &t;
    f.log.completed = &t.completed;
    key(&f, CUI_KEY_V, CUI_MOD_CTRL, false);
    cui_input_event_result result = frame(&f, true);
    assert(result.change_count == 1 && !strcmp(f.buffer, "pasted") && t.completed == 1 &&
           t.prepared == 1);
    evidence(&f.log.events[0], CUI_COMPONENT_EVENT_CHANGE, 0, CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    f.options.presentation = (cui_edit_presentation){0};
    f.options.presentation_complete = NULL;
    f.log.completed = NULL;
    f.options.clipboard = NULL;
    key(&f, CUI_KEY_V, CUI_MOD_CTRL, false);
    result = frame(&f, true);
    assert(result.control.error == CUI_EDIT_CLIPBOARD_UNAVAILABLE && !result.change_count);
    assert(!strcmp(f.buffer, "pasted"));
    f.options.clipboard = &platform;
    /* Composites can observe exactly one ordered event without whole-frame replay. */
    text(&f, "!");
    key(&f, CUI_KEY_ENTER, 0, false);
    assert(cui_begin_frame(f.ui, 400, 100, 0));
    cui_begin_box(f.ui, 10, (cui_box){.width = 220, .height = 40});
    cui_interact(f.ui, 10, CUI_ROLE_TEXTBOX, cui_string("Input"), false, false, 0);
    result = cui_base_input_reduce_event_events(f.ui, 10, f.edit, &f.state, f.options, 0,
                                                (cui_component_text_handler){notify, &f.log});
    assert(result.change_count == 1 && !result.submit_count);
    result = cui_base_input_reduce_event_events(f.ui, 10, f.edit, &f.state, f.options, 1,
                                                (cui_component_text_handler){notify, &f.log});
    assert(result.submit_count == 1 && !result.change_count && !strcmp(f.buffer, "pasted!"));
    unsigned count = f.log.count;
    /* Already-consumed events must not replay a submission or notification. */
    assert(cui_consume_event(f.ui, 1));
    result = cui_base_input_reduce_event_events(f.ui, 10, f.edit, &f.state, f.options, 1,
                                                (cui_component_text_handler){notify, &f.log});
    assert(!result.submit_count && !result.change_count && f.log.count == count);
    cui_edit_state before = cui_edit_get_state(f.edit);
    result = cui_base_input_reduce_event_events(f.ui, 10, f.edit, &f.state, f.options, SIZE_MAX,
                                                (cui_component_text_handler){notify, &f.log});
    assert(result.control.error == CUI_EDIT_INVALID_ARGUMENT && !result.change_count &&
           f.log.count == count);
    assert(cui_edit_get_state(f.edit).revision == before.revision);
    cui_end_box(f.ui);
    cui_end_frame(f.ui);
    finish(&f);
}
int main(void) {
    batches();
    ime_and_secure();
    paste_and_single();
    puts("PASS input post-mutation CHANGE/SUBMIT, ordered evidence, IME, secure redaction and "
         "borrowed copies");
    return 0;
}
