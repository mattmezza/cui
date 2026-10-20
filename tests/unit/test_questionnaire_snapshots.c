/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_questionnaire_snapshot_math.h"
#include "cui/cui_questionnaire_snapshots.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_questionnaire_state s;
    cui_questionnaire_item items[3];
    cui_questionnaire_choice choices[3];
    cui_questionnaire_item_runtime runtime[3];
    cui_questionnaire_choice_runtime selected[3];
    char model_text[96], edit_text[64];
    cui_edit *edit;
    unsigned events, validations;
} model;
typedef struct {
    cui_questionnaire_item_snapshot items[3];
    cui_str choices[3];
    char text[256];
} arena;
static void event(const cui_questionnaire_state *s, cui_questionnaire_event e, void *user) {
    (void)s;
    (void)e;
    ((model *)user)->events++;
}
static cui_str validate(const cui_questionnaire_state *s, size_t item, void *user) {
    (void)s;
    (void)item;
    ((model *)user)->validations++;
    return (cui_str){0};
}
static void init_policy(model *m, bool input_disabled) {
    memset(m, 0, sizeof *m);
    strcpy(m->edit_text, "draft");
    m->edit =
        cui_edit_create((cui_edit_config){.buffer = m->edit_text, .capacity = sizeof m->edit_text});
    assert(m->edit);
    m->choices[0].value = cui_string("");
    m->choices[1].value = cui_string("é");
    m->choices[2].value = (cui_str){"abc", 3};
    m->items[0] = (cui_questionnaire_item){.name = cui_string("mixed"),
                                           .required = true,
                                           .multiple = true,
                                           .choices = m->choices,
                                           .choice_count = 3,
                                           .input = m->edit,
                                           .input_disabled = input_disabled,
                                           .validate = validate,
                                           .validate_user = m};
    m->items[1].name = cui_string("");
    m->items[2] = (cui_questionnaire_item){.name = cui_string("disabled"), .disabled = true};
    assert(
        cui_questionnaire_init(&m->s, m->items, 3,
                               (cui_questionnaire_workspace){.items = m->runtime,
                                                             .item_capacity = 3,
                                                             .choices = m->selected,
                                                             .choice_capacity = 3,
                                                             .text = m->model_text,
                                                             .text_capacity = sizeof m->model_text,
                                                             .max_answer_bytes = 31},
                               event, m) == CUI_QUESTIONNAIRE_OK);
    size_t selected[] = {2, 0, 1, 1};
    assert(cui_questionnaire_set_answer(&m->s, 0, selected, 4, cui_string(" γ ")) ==
           CUI_QUESTIONNAIRE_OK);
}
static void init(model *m) {
    init_policy(m, false);
}
static cui_questionnaire_snapshot_workspace workspace(arena *a) {
    return (cui_questionnaire_snapshot_workspace){.items = a->items,
                                                  .item_capacity = 3,
                                                  .choices = a->choices,
                                                  .choice_capacity = 3,
                                                  .text = a->text,
                                                  .text_capacity = sizeof a->text};
}
static void equal(cui_str s, const char *bytes, size_t length) {
    assert(s.length == length && s.data && !memcmp(s.data, bytes, length));
    assert(s.data[length] == 0);
}
static void data_and_retention(void) {
    model m;
    init(&m);
    assert(cui_questionnaire_set_current(&m.s, 1, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_skip(&m.s));
    cui_questionnaire_query_snapshot skipped;
    assert(cui_questionnaire_query_copy(&m.s, &skipped) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(skipped.complete && skipped.confirmable && skipped.submit_visible);
    assert(cui_questionnaire_choose(&m.s, 0, 0, 10, false) == CUI_QUESTIONNAIRE_OK);
    /* choose multiple changes the answer; restore all three silently. */
    size_t selected[] = {2, 1, 0};
    assert(cui_questionnaire_set_answer(&m.s, 0, selected, 3, cui_string(" γ ")) ==
           CUI_QUESTIONNAIRE_OK);
    unsigned events = m.events, validations = m.validations;
    cui_questionnaire_state before = m.s;
    cui_questionnaire_item_runtime runtime[3];
    memcpy(runtime, m.runtime, sizeof runtime);
    char text[96];
    memcpy(text, m.model_text, sizeof text);
    arena a;
    memset(&a, 0xa5, sizeof a);
    cui_questionnaire_snapshot_requirements n, measured;
    cui_questionnaire_submission_snapshot out;
    assert(cui_questionnaire_submission_measure(&m.s, &measured) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(measured.items == 2 && measured.choices == 3 && measured.text_bytes == 20);
    assert(cui_questionnaire_submission_copy(&m.s, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(!memcmp(&n, &measured, sizeof n) && out.items == a.items && out.count == 2);
    const cui_questionnaire_item_snapshot *first = &out.items[0];
    assert(first->item == 0 && !first->disabled && first->status == CUI_QUESTIONNAIRE_ANSWERED);
    assert(first->name.data == a.text);
    assert(first->answer.choices[0].data == a.text + 6);
    assert(first->answer.choices[1].data == a.text + 7);
    assert(first->answer.choices[2].data == a.text + 10);
    assert(first->answer.freeform.data == a.text + 14);
    assert(out.items[1].name.data == a.text + 19);
    equal(first->name, "mixed", 5);
    assert(first->answer.choice_count == 3 && first->answer.choices == a.choices);
    equal(first->answer.choices[0], "", 0);
    equal(first->answer.choices[1], "é", 2);
    equal(first->answer.choices[2], "abc", 3);
    assert(first->answer.has_freeform);
    equal(first->answer.freeform, " γ ", 4);
    assert(out.items[1].item == 1 && out.items[1].status == CUI_QUESTIONNAIRE_SKIPPED);
    equal(out.items[1].name, "", 0);
    assert(!out.items[1].answer.choices && !out.items[1].answer.has_freeform &&
           !out.items[1].answer.freeform.data);
    assert(m.events == events && m.validations == validations);
    assert(!memcmp(&before, &m.s, sizeof before) && !memcmp(runtime, m.runtime, sizeof runtime) &&
           !memcmp(text, m.model_text, sizeof text));
    assert(cui_questionnaire_set_input(&m.s, 0, cui_string("changed")) == CUI_QUESTIONNAIRE_OK);
    cui_questionnaire_reset(&m.s);
    assert(cui_questionnaire_disable_item(&m.s, 0, true) == CUI_QUESTIONNAIRE_OK);
    cui_edit_destroy(m.edit);
    memset(&m, 0, sizeof m);
    equal(first->name, "mixed", 5);
    equal(first->answer.freeform, " γ ", 4);
    equal(first->answer.choices[2], "abc", 3);
}
static void capacity_and_invalid(void) {
    model m;
    init(&m);
    arena a;
    memset(&a, 0x5a, sizeof a);
    cui_questionnaire_snapshot_requirements n;
    cui_questionnaire_submission_snapshot good, failed;
    assert(cui_questionnaire_submission_copy(&m.s, workspace(&a), &n, &good) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    arena before = a;
    for (unsigned dimension = 0; dimension < 3; ++dimension) {
        cui_questionnaire_snapshot_workspace w = workspace(&a);
        if (dimension == 0)
            w.item_capacity = n.items - 1;
        if (dimension == 1)
            w.choice_capacity = n.choices - 1;
        if (dimension == 2)
            w.text_capacity = n.text_bytes - 1;
        cui_questionnaire_snapshot_requirements required = {0};
        failed = good;
        assert(cui_questionnaire_submission_copy(&m.s, w, &required, &failed) ==
               CUI_QUESTIONNAIRE_SNAPSHOT_CAPACITY);
        assert(!failed.items && !failed.count && !memcmp(&required, &n, sizeof n));
        assert(!memcmp(&a, &before, sizeof a));
        equal(good.items[0].name, "mixed", 5);
    }
    cui_questionnaire_snapshot_workspace w = workspace(&a);
    w.text = NULL;
    assert(cui_questionnaire_submission_copy(&m.s, w, &n, &failed) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_INVALID);
    assert(!n.items && !n.choices && !n.text_bytes && !memcmp(&a, &before, sizeof a));
    assert(cui_questionnaire_submission_copy(&m.s, workspace(&a), NULL, &failed) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_INVALID);
    assert(!failed.items && !failed.count);
    assert(cui_questionnaire_submission_copy(&m.s, workspace(&a), &n, NULL) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_INVALID);
    cui_questionnaire_item_snapshot one;
    assert(cui_questionnaire_answer_copy(&m.s, SIZE_MAX, workspace(&a), &n, &one) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_UNKNOWN_ITEM);
    assert(!one.name.data && !one.answer.choices && !n.items);
    assert(cui_questionnaire_answer_measure(NULL, 0, &n) == CUI_QUESTIONNAIRE_SNAPSHOT_INVALID);
    assert(cui_questionnaire_submission_measure(&m.s, NULL) == CUI_QUESTIONNAIRE_SNAPSHOT_INVALID);
    /* Stale measurement: answer grows; copy remeasures before writing. */
    assert(cui_questionnaire_submission_measure(&m.s, &n) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    w = workspace(&a);
    w.text_capacity = n.text_bytes;
    assert(cui_questionnaire_set_input(&m.s, 0, cui_string("substantially longer")) ==
           CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_submission_copy(&m.s, w, &n, &failed) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_CAPACITY);
    assert(n.text_bytes > w.text_capacity && !memcmp(&a, &before, sizeof a));
    cui_edit_destroy(m.edit);
}
static void filters_and_queries(void) {
    model m;
    init(&m);
    arena a;
    cui_questionnaire_item_snapshot out;
    cui_questionnaire_snapshot_requirements n;
    assert(cui_questionnaire_disable_choice(&m.s, 0, 1, true) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_answer_copy(&m.s, 0, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(out.answer.choice_count == 2);
    equal(out.answer.choices[1], "abc", 3);
    assert(cui_questionnaire_answer_copy(&m.s, 2, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(out.disabled && !out.answer.choice_count && !out.answer.has_freeform);
    cui_questionnaire_query_snapshot query;
    unsigned events = m.events, validators = m.validations;
    assert(cui_questionnaire_query_copy(&m.s, &query) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(query.current_item == 0 && query.position == 0 && query.current == 1 &&
           query.total == 2);
    assert(!query.previous_visible && query.next_visible && !query.skip_visible &&
           !query.submit_visible && query.confirmable);
    cui_str error = cui_string("rejected");
    assert(cui_questionnaire_external_error(&m.s, 0, &error) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_query_copy(&m.s, &query) == CUI_QUESTIONNAIRE_SNAPSHOT_OK &&
           query.confirmable);
    assert(m.events == events && m.validations == validators);
    assert(cui_questionnaire_set_current(&m.s, 1, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_query_copy(&m.s, &query) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(query.previous_visible && !query.next_visible && query.skip_visible &&
           query.submit_visible && !query.confirmable);
    assert(cui_questionnaire_disable_item(&m.s, 0, true) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_disable_item(&m.s, 1, true) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_query_copy(&m.s, &query) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(query.current_item == SIZE_MAX && query.position == SIZE_MAX && !query.current &&
           !query.total && !query.confirmable);
    cui_edit_destroy(m.edit);
}
static void editor_and_disabled_input(void) {
    model m;
    init(&m);
    assert(cui_edit_replace_range(m.edit, 0, cui_edit_get_text(m.edit).length,
                                  cui_string("unsynchronized")) == CUI_EDIT_OK);
    cui_str draft = cui_edit_get_text(m.edit);
    char retained_draft[64];
    assert(draft.length < sizeof retained_draft);
    memcpy(retained_draft, draft.data, draft.length);
    arena a;
    cui_questionnaire_item_snapshot out;
    cui_questionnaire_snapshot_requirements n;
    assert(cui_questionnaire_answer_copy(&m.s, 0, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    equal(out.answer.freeform, " γ ", 4);
    cui_str after = cui_edit_get_text(m.edit);
    assert(after.length == draft.length && !memcmp(after.data, retained_draft, after.length));
    cui_edit_destroy(m.edit);
    init_policy(&m, true);
    assert(cui_questionnaire_answer_copy(&m.s, 0, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(!out.answer.has_freeform && !out.answer.freeform.data && !out.answer.freeform.length);
    assert(out.answer.choice_count == 3);
    assert(cui_questionnaire_disable_choice(&m.s, 0, 1, true) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_disable_choice(&m.s, 0, 1, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_answer_copy(&m.s, 0, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(out.answer.choice_count == 3);
    assert(cui_questionnaire_set_current(&m.s, 1, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_skip(&m.s));
    assert(cui_questionnaire_disable_item(&m.s, 1, true) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_answer_copy(&m.s, 1, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(out.status == CUI_QUESTIONNAIRE_SKIPPED && out.disabled && !out.answer.choice_count);
    cui_edit_destroy(m.edit);
    /* Reject embedded NUL through the actual legacy model contract. */
    cui_questionnaire_state invalid = {0};
    cui_questionnaire_item item = {.name = (cui_str){"a\0b", 3}};
    cui_questionnaire_item_runtime runtime;
    assert(
        cui_questionnaire_init(&invalid, &item, 1,
                               (cui_questionnaire_workspace){.items = &runtime, .item_capacity = 1},
                               NULL, NULL) == CUI_QUESTIONNAIRE_INVALID);
}
static void pending_and_required_last(void) {
    cui_questionnaire_state s = {0};
    cui_questionnaire_choice choice = {.value = cui_string("yes")};
    cui_questionnaire_item item = {
        .name = cui_string("one"), .required = true, .choices = &choice, .choice_count = 1};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected;
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = &selected,
                                                                .choice_capacity = 1},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    cui_questionnaire_query_snapshot q;
    assert(cui_questionnaire_query_copy(&s, &q) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(q.submit_visible && !q.skip_visible && !q.confirmable && !q.previous_visible &&
           !q.next_visible);
    assert(cui_questionnaire_choose(&s, 0, 0, 10, false) == CUI_QUESTIONNAIRE_OK);
    assert(s.pending_confirm);
    cui_questionnaire_state before = s;
    arena a;
    cui_questionnaire_item_snapshot out;
    cui_questionnaire_snapshot_requirements n;
    assert(cui_questionnaire_query_copy(&s, &q) == CUI_QUESTIONNAIRE_SNAPSHOT_OK && q.confirmable);
    assert(cui_questionnaire_answer_copy(&s, 0, workspace(&a), &n, &out) ==
           CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(!memcmp(&before, &s, sizeof s));
}
static void empty_and_math(void) {
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(&s, NULL, 0, (cui_questionnaire_workspace){0}, NULL, NULL) ==
           CUI_QUESTIONNAIRE_OK);
    cui_questionnaire_submission_snapshot out;
    cui_questionnaire_snapshot_requirements n;
    assert(cui_questionnaire_submission_copy(&s, (cui_questionnaire_snapshot_workspace){0}, &n,
                                             &out) == CUI_QUESTIONNAIRE_SNAPSHOT_OK);
    assert(!n.items && !n.choices && !n.text_bytes && !out.count && !out.items);
    cui_questionnaire_query_snapshot q;
    assert(cui_questionnaire_query_copy(NULL, &q) == CUI_QUESTIONNAIRE_SNAPSHOT_INVALID);
    assert(q.current_item == SIZE_MAX && q.position == SIZE_MAX && !q.complete);
    assert(cui_questionnaire_query_copy(&s, NULL) == CUI_QUESTIONNAIRE_SNAPSHOT_INVALID);
    /* Private arithmetic unit probe, NOT a forged-model public API test. */
    size_t sum = SIZE_MAX - 1;
    assert(cui_questionnaire_snapshot_add(&sum, 1) && sum == SIZE_MAX);
    assert(!cui_questionnaire_snapshot_add(&sum, 1) && sum == SIZE_MAX);
    assert(cui_questionnaire_snapshot_extent(SIZE_MAX / sizeof(cui_str), sizeof(cui_str)));
    assert(!cui_questionnaire_snapshot_extent(SIZE_MAX / sizeof(cui_str) + 1, sizeof(cui_str)));
}
int main(void) {
    data_and_retention();
    capacity_and_invalid();
    filters_and_queries();
    editor_and_disabled_input();
    pending_and_required_last();
    empty_and_math();
    puts("Questionnaire snapshots passed");
    return 0;
}
