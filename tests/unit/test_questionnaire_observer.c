/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_questionnaire_observer_internal.h"
#include "cui/cui_questionnaire_observer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_questionnaire_event_kind kind;
    cui_questionnaire_payload_status status;
    size_t count;
    cui_questionnaire_status answer_status;
    char answer;
    bool previous, current;
    char freeform[32];
    char previous_name[16], current_name[16];
} saved;
typedef struct {
    cui_questionnaire_state state;
    cui_questionnaire_choice choices[2];
    cui_questionnaire_item items[2];
    cui_questionnaire_item_runtime runtime[2];
    cui_questionnaire_choice_runtime selected[4];
    cui_questionnaire_observer observer;
    cui_questionnaire_observer_frame frames[3];
    cui_questionnaire_item_snapshot records[3][2];
    cui_str strings[3][4];
    char text[3][128];
    saved typed[32];
    cui_questionnaire_event_kind legacy[32];
    size_t typed_count, legacy_count, replacement_count;
    unsigned mutation;
    cui_edit *edit;
    char edit_buffer[64], model_text[3 * 64];
    bool inside, detached, busy, pinned, other_busy;
} fixture;
static void record_typed(void *user, const cui_questionnaire_observation *e) {
    fixture *f = user;
    assert(f->typed_count < 32);
    saved *s = &f->typed[f->typed_count++];
    s->kind = e->kind;
    s->status = e->payload_status;
    assert(e->event.target == 900 && e->event.flags == 0 &&
           e->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN && !e->event.frame_sequence &&
           !e->event.event_index);
    assert(e->event.kind == (e->kind == CUI_QUESTIONNAIRE_COMPLETED ? CUI_COMPONENT_EVENT_COMMIT
                             : e->kind == CUI_QUESTIONNAIRE_SUBMIT  ? CUI_COMPONENT_EVENT_SUBMIT
                                                                    : CUI_COMPONENT_EVENT_CHANGE));
    if (e->payload_status != CUI_QUESTIONNAIRE_PAYLOAD_READY) {
        assert(!e->has_current_name && !e->has_previous_name && !e->current_name.data &&
               !e->previous_name.data && !e->answer.name.data && !e->submission.items &&
               !e->submission.count);
        if (e->payload_status == CUI_QUESTIONNAIRE_PAYLOAD_NO_FRAME)
            assert(!e->required.items && !e->required.choices && !e->required.text_bytes);
        if (e->payload_status == CUI_QUESTIONNAIRE_PAYLOAD_CAPACITY)
            assert(e->required.text_bytes > 0);
        return;
    }
    s->previous = e->has_previous_name;
    s->current = e->has_current_name;
    if (s->previous) {
        assert(e->previous_name.length < sizeof s->previous_name);
        memcpy(s->previous_name, e->previous_name.data, e->previous_name.length);
    }
    if (s->current) {
        assert(e->current_name.length < sizeof s->current_name);
        memcpy(s->current_name, e->current_name.data, e->current_name.length);
    }
    const cui_questionnaire_answer_snapshot *a = NULL;
    if (e->kind == CUI_QUESTIONNAIRE_ANSWER_CHANGED) {
        a = &e->answer.answer;
        s->answer_status = e->answer.status;
        assert(e->answer.item == e->item);
    }
    if (e->submission.count) {
        s->count = e->submission.count;
        a = &e->submission.items[0].answer;
    }
    if (a && a->choice_count)
        s->answer = a->choices[0].data[0];
    if (a && a->has_freeform) {
        assert(a->freeform.length < sizeof s->freeform);
        memcpy(s->freeform, a->freeform.data, a->freeform.length);
    }
}
static void replacement(const cui_questionnaire_state *state, cui_questionnaire_event e,
                        void *user) {
    (void)state;
    assert(e.kind == CUI_QUESTIONNAIRE_SUBMIT);
    ++*(size_t *)user;
}
static void legacy(const cui_questionnaire_state *state, cui_questionnaire_event e, void *user) {
    fixture *f = user;
    assert(state == &f->state && f->legacy_count < 32);
    f->legacy[f->legacy_count++] = e.kind;
    if (f->mutation == 1 && e.kind == CUI_QUESTIONNAIRE_COMPLETED && !f->inside) {
        f->inside = true;
        size_t second = 1;
        assert(cui_questionnaire_set_answer(&f->state, 0, &second, 1, (cui_str){0}) ==
               CUI_QUESTIONNAIRE_OK);
        assert(cui_questionnaire_submit(&f->state));
        f->inside = false;
    } else if (f->mutation == 2 && e.kind == CUI_QUESTIONNAIRE_COMPLETED) {
        f->state.event = replacement;
        f->state.event_user = &f->replacement_count;
    } else if (f->mutation == 3 && e.kind == CUI_QUESTIONNAIRE_ANSWER_CHANGED) {
        assert(cui_questionnaire_disable_item(&f->state, 0, true) == CUI_QUESTIONNAIRE_OK);
    } else if (f->mutation == 4 && e.kind == CUI_QUESTIONNAIRE_ANSWER_CHANGED) {
        assert(cui_questionnaire_set_current(&f->state, 0, false) == CUI_QUESTIONNAIRE_OK);
    } else if (f->mutation == 6 && e.kind == CUI_QUESTIONNAIRE_COMPLETED) {
        assert(cui_questionnaire_disable_item(&f->state, 1, true) == CUI_QUESTIONNAIRE_OK);
        assert(cui_questionnaire_reset(&f->state));
    } else if (f->mutation == 5 && e.kind == CUI_QUESTIONNAIRE_COMPLETED) {
        cui_questionnaire_observer_frame frame = {0};
        f->detached =
            cui_questionnaire_observer_detach(&f->observer) == CUI_QUESTIONNAIRE_OBSERVER_BUSY;
        f->pinned = f->state.observer == &f->observer && f->observer.private_used == 1;
        f->busy = cui_questionnaire_observer_attach(
                      &f->state, &f->observer, 900, &frame, 1,
                      (cui_questionnaire_observation_handler){record_typed, f}) ==
                  CUI_QUESTIONNAIRE_OBSERVER_BUSY;
        cui_questionnaire_observer other = {0};
        f->other_busy =
            cui_questionnaire_observer_attach(&f->state, &other, 901, NULL, 0,
                                              (cui_questionnaire_observation_handler){0}) ==
            CUI_QUESTIONNAIRE_OBSERVER_BUSY;
        assert(cui_questionnaire_submit(&f->state)); /* Inactive attachment emits no typed event. */
    }
}
static void init_model(fixture *f, bool multiple, bool input) {
    memset(f, 0, sizeof *f);
    f->choices[0].value = cui_string("A");
    f->choices[1].value = cui_string("B");
    f->items[0] = (cui_questionnaire_item){.name = cui_string("first"),
                                           .multiple = multiple,
                                           .choices = f->choices,
                                           .choice_count = 2};
    if (input) {
        f->edit = cui_edit_create(
            (cui_edit_config){.buffer = f->edit_buffer, .capacity = sizeof f->edit_buffer});
        assert(f->edit);
        f->items[0].input = f->edit;
    }
    f->items[1] = (cui_questionnaire_item){
        .name = cui_string(""), .multiple = multiple, .choices = f->choices, .choice_count = 2};
    assert(
        cui_questionnaire_init(&f->state, f->items, 2,
                               (cui_questionnaire_workspace){.items = f->runtime,
                                                             .item_capacity = 2,
                                                             .choices = f->selected,
                                                             .choice_capacity = 4,
                                                             .text = f->model_text,
                                                             .text_capacity = sizeof f->model_text,
                                                             .max_answer_bytes = 63},
                               legacy, f) == CUI_QUESTIONNAIRE_OK);
    assert(!f->state.observer);
    for (size_t i = 0; i < 3; ++i)
        f->frames[i].workspace =
            (cui_questionnaire_snapshot_workspace){.items = f->records[i],
                                                   .item_capacity = 2,
                                                   .choices = f->strings[i],
                                                   .choice_capacity = 4,
                                                   .text = f->text[i],
                                                   .text_capacity = sizeof f->text[i]};
}
static void init(fixture *f, bool multiple) {
    init_model(f, multiple, false);
}
static void attach(fixture *f, size_t count) {
    assert(cui_questionnaire_observer_attach(
               &f->state, &f->observer, 900, f->frames, count,
               (cui_questionnaire_observation_handler){record_typed, f}) ==
           CUI_QUESTIONNAIRE_OBSERVER_OK);
}
static void answer_a(fixture *f) {
    size_t first = 0;
    assert(cui_questionnaire_set_answer(&f->state, 0, &first, 1, (cui_str){0}) ==
           CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_set_answer(&f->state, 1, &first, 1, (cui_str){0}) ==
           CUI_QUESTIONNAIRE_OK);
}
static void frozen_nested_pair(void) {
    fixture f;
    init(&f, false);
    answer_a(&f);
    attach(&f, 3);
    f.mutation = 1;
    assert(cui_questionnaire_submit(&f.state));
    assert(f.typed_count == 4 && f.legacy_count == 4);
    assert(f.typed[0].kind == CUI_QUESTIONNAIRE_COMPLETED && f.typed[0].answer == 'A');
    assert(f.typed[1].kind == CUI_QUESTIONNAIRE_COMPLETED && f.typed[1].answer == 'B');
    assert(f.typed[2].kind == CUI_QUESTIONNAIRE_SUBMIT && f.typed[2].answer == 'B');
    assert(f.typed[3].kind == CUI_QUESTIONNAIRE_SUBMIT && f.typed[3].answer == 'A');
    for (size_t i = 0; i < 4; ++i)
        assert(f.typed[i].kind == f.legacy[i] && f.typed[i].count == 2);
    assert(!f.observer.private_used && !f.observer.private_inflight);
    assert(cui_questionnaire_submit(&f.state));
    assert(f.typed_count == 5 && f.typed[4].kind == CUI_QUESTIONNAIRE_SUBMIT &&
           f.typed[4].answer == 'B');
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
}
static void compatibility(void) {
    for (unsigned mutation = 1; mutation < 7; ++mutation) {
        if (mutation == 5)
            continue;
        fixture old, observed;
        init(&old, false);
        init(&observed, false);
        old.mutation = observed.mutation = mutation;
        attach(&observed, 3);
        if (mutation <= 2 || mutation == 6) {
            answer_a(&old);
            answer_a(&observed);
            assert(cui_questionnaire_submit(&old.state) ==
                   cui_questionnaire_submit(&observed.state));
        } else if (mutation == 3) {
            assert(cui_questionnaire_choose(&old.state, 0, 0, 10, false) ==
                   cui_questionnaire_choose(&observed.state, 0, 0, 10, false));
            assert(!old.state.pending_confirm && !observed.state.pending_confirm);
        } else {
            assert(cui_questionnaire_set_current(&old.state, 1, false) == CUI_QUESTIONNAIRE_OK);
            assert(cui_questionnaire_set_current(&observed.state, 1, false) ==
                   CUI_QUESTIONNAIRE_OK);
            assert(cui_questionnaire_skip(&old.state) == cui_questionnaire_skip(&observed.state));
        }
        assert(old.legacy_count == observed.legacy_count &&
               old.replacement_count == observed.replacement_count);
        assert(!memcmp(old.legacy, observed.legacy, old.legacy_count * sizeof old.legacy[0]));
        assert(old.state.current == observed.state.current &&
               old.state.complete == observed.state.complete &&
               old.state.pending_confirm == observed.state.pending_confirm);
        assert(!memcmp(old.selected, observed.selected, sizeof old.selected));
        if (mutation == 2)
            assert(observed.replacement_count == 1 && observed.typed_count == 2);
        if (mutation == 4)
            assert(observed.typed[0].answer_status == CUI_QUESTIONNAIRE_SKIPPED);
        if (mutation == 6)
            assert(observed.typed_count == 2 && observed.typed[0].count == 2 &&
                   observed.typed[1].count == 2 && !observed.state.complete);
        assert(cui_questionnaire_observer_detach(&observed.observer) ==
               CUI_QUESTIONNAIRE_OBSERVER_OK);
    }
}
static void capacity_depth_detach(void) {
    fixture f;
    init(&f, false);
    answer_a(&f);
    memset(f.text, 0xa5, sizeof f.text);
    f.frames[0].workspace.text_capacity = 1;
    attach(&f, 1);
    assert(cui_questionnaire_submit(&f.state));
    assert(f.typed_count == 2 && f.typed[0].status == CUI_QUESTIONNAIRE_PAYLOAD_CAPACITY &&
           f.typed[1].status == CUI_QUESTIONNAIRE_PAYLOAD_CAPACITY);
    for (size_t i = 0; i < sizeof f.text; ++i)
        assert(((unsigned char *)f.text)[i] == 0xa5);
    cui_questionnaire_observer_statistics stats =
        cui_questionnaire_observer_get_statistics(&f.observer);
    assert(stats.attempted == 2 && stats.delivered == 2 && stats.payload_failures == 2);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    init(&f, false);
    answer_a(&f);
    attach(&f, 1);
    f.mutation = 1;
    assert(cui_questionnaire_submit(&f.state));
    assert(f.typed_count == 4 && f.typed[0].status == CUI_QUESTIONNAIRE_PAYLOAD_READY &&
           f.typed[1].status == CUI_QUESTIONNAIRE_PAYLOAD_NO_FRAME &&
           f.typed[2].status == CUI_QUESTIONNAIRE_PAYLOAD_NO_FRAME &&
           f.typed[3].status == CUI_QUESTIONNAIRE_PAYLOAD_READY && f.typed[3].answer == 'A');
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    init(&f, false);
    answer_a(&f);
    attach(&f, 3);
    f.mutation = 5;
    assert(cui_questionnaire_submit(&f.state));
    assert(f.detached && f.busy && f.other_busy && f.pinned && !f.state.observer &&
           !f.observer.private_state);
    assert(f.typed_count == 1 && f.legacy_count == 3); /* Outer C, nested S, outer S. */
    assert(cui_questionnaire_observer_attach(
               &f.state, &f.observer, 900, f.frames, 3,
               (cui_questionnaire_observation_handler){record_typed, &f}) ==
           CUI_QUESTIONNAIRE_OBSERVER_OK);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
}
static void independent_capacity_failures(void) {
    for (unsigned dimension = 0; dimension < 3; ++dimension) {
        fixture f;
        init(&f, false);
        answer_a(&f);
        memset(f.records, 0xa5, sizeof f.records);
        memset(f.strings, 0x5a, sizeof f.strings);
        memset(f.text, 0x3c, sizeof f.text);
        if (dimension == 0)
            f.frames[0].workspace.item_capacity = 1;
        if (dimension == 1)
            f.frames[0].workspace.choice_capacity = 1;
        if (dimension == 2)
            f.frames[0].workspace.text_capacity = 1;
        attach(&f, 1);
        f.mutation = 6;
        assert(cui_questionnaire_submit(&f.state));
        assert(f.typed_count == 2 && f.legacy_count == 2);
        assert(f.typed[0].status == CUI_QUESTIONNAIRE_PAYLOAD_CAPACITY &&
               f.typed[1].status == CUI_QUESTIONNAIRE_PAYLOAD_CAPACITY);
        for (size_t j = 0; j < sizeof f.records; ++j)
            assert(((unsigned char *)f.records)[j] == 0xa5);
        for (size_t j = 0; j < sizeof f.strings; ++j)
            assert(((unsigned char *)f.strings)[j] == 0x5a);
        for (size_t j = 0; j < sizeof f.text; ++j)
            assert(((unsigned char *)f.text)[j] == 0x3c);
        assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    }
}
static void names_and_silence(void) {
    fixture f;
    init(&f, false);
    attach(&f, 3);
    answer_a(&f);
    assert(!f.typed_count && !f.legacy_count);
    assert(cui_questionnaire_next(&f.state));
    assert(f.typed_count == 1 && f.typed[0].previous && f.typed[0].current);
    assert(!strcmp(f.typed[0].previous_name, "first") && !f.typed[0].current_name[0]);
    assert(cui_questionnaire_previous(&f.state));
    assert(f.typed[1].previous && !f.typed[1].previous_name[0] &&
           !strcmp(f.typed[1].current_name, "first"));
    size_t count = f.typed_count;
    assert(cui_questionnaire_set_current(&f.state, 1, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_disable_choice(&f.state, 0, 0, true) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_external_error(&f.state, 1, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_reset(&f.state));
    assert(f.state.observer == &f.observer && f.typed_count == count);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
}
static void ui_frame(cui_context *ui, fixture *f, cui_questionnaire_input_state *input) {
    assert(cui_begin_frame(ui, 360, 300, .001f));
    cui_theme theme = cui_theme_light();
    cui_questionnaire_options o = {.width = 340};
    assert(cui_questionnaire_begin(ui, 10, &theme, &f->state, o));
    for (size_t c = 0; c < 2; ++c)
        (void)cui_questionnaire_choice_control(ui, 12 + c, &theme, &f->state, 0, c, o);
    if (input && f->state.current == 0)
        assert(cui_questionnaire_input_control(ui, 14, &theme, &f->state, 0, input, NULL, o) ==
               CUI_QUESTIONNAIRE_OK);
    cui_questionnaire_end(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void routed_and_timer(void) {
    fixture f;
    init(&f, true);
    attach(&f, 3);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    ui_frame(ui, &f, NULL);
    cui_focus(ui, 12);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_A}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B}));
    ui_frame(ui, &f, NULL);
    assert(f.typed_count == 2 && f.legacy_count == 2);
    assert(cui_queue_accessibility_action(ui, 12, CUI_ACCESSIBILITY_ACTIVATE));
    ui_frame(ui, &f, NULL);
    assert(f.typed_count == 3 && f.legacy_count == 3);
    cui_rect r = cui_bounds(ui, 12);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 5, .y = r.y + 5}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 5, .y = r.y + 5}));
    ui_frame(ui, &f, NULL);
    assert(f.typed_count == 4 && f.legacy_count == 4);
    cui_destroy(ui);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    init(&f, false);
    attach(&f, 3);
    assert(cui_questionnaire_choose(&f.state, 0, 0, 10, false) == CUI_QUESTIONNAIRE_OK);
    assert(f.typed_count == 1 && f.state.pending_confirm);
    assert(!cui_questionnaire_tick(&f.state, 10.1));
    assert(cui_questionnaire_tick(&f.state, 10.2));
    assert(f.typed_count == 2 && f.typed[1].kind == CUI_QUESTIONNAIRE_CURRENT_CHANGED);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    init_model(&f, false, true);
    attach(&f, 3);
    ui = cui_create((cui_config){0});
    assert(ui);
    cui_questionnaire_input_state input = {0};
    ui_frame(ui, &f, &input);
    cui_focus(ui, 14);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "é"}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"}));
    ui_frame(ui, &f, &input);
    assert(f.typed_count == 2 && !strcmp(f.typed[0].freeform, "é") &&
           !strcmp(f.typed[1].freeform, "éx"));
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    cui_destroy(ui);
    cui_edit_destroy(f.edit);
}
static void preflight_and_missing_handler(void) {
    fixture f;
    init(&f, false);
    assert(cui_questionnaire_observer_attach(&f.state, &f.observer, 0, NULL, 0,
                                             (cui_questionnaire_observation_handler){0}) ==
           CUI_QUESTIONNAIRE_OBSERVER_INVALID);
    f.frames[0].workspace.items = NULL;
    assert(cui_questionnaire_observer_attach(&f.state, &f.observer, 900, f.frames, 1,
                                             (cui_questionnaire_observation_handler){0}) ==
           CUI_QUESTIONNAIRE_OBSERVER_INVALID);
    init(&f, false);
    assert(cui_questionnaire_observer_attach(&f.state, &f.observer, 900, NULL, 0,
                                             (cui_questionnaire_observation_handler){0}) ==
           CUI_QUESTIONNAIRE_OBSERVER_OK);
    assert(cui_questionnaire_activate(&f.state, 0, 0) == CUI_QUESTIONNAIRE_OK);
    cui_questionnaire_observer_statistics stats =
        cui_questionnaire_observer_get_statistics(&f.observer);
    assert(stats.attempted == 1 && !stats.delivered && !stats.payload_failures &&
           f.legacy_count == 1);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
    init(&f, false);
    attach(&f, 0);
    assert(cui_questionnaire_activate(&f.state, 0, 0) == CUI_QUESTIONNAIRE_OK);
    assert(f.typed_count == 1 && f.typed[0].status == CUI_QUESTIONNAIRE_PAYLOAD_NO_FRAME);
    assert(cui_questionnaire_observer_detach(&f.observer) == CUI_QUESTIONNAIRE_OBSERVER_OK);
}
int main(void) {
    frozen_nested_pair();
    compatibility();
    capacity_depth_detach();
    names_and_silence();
    routed_and_timer();
    preflight_and_missing_handler();
    independent_capacity_failures();
    puts("Questionnaire observer passed");
    return 0;
}
