/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static cui_str str(const char *s) {
    size_t n = 0;
    while (s[n])
        n++;
    return (cui_str){s, n};
}
static void test_disabled_retained_answer_preserves_pending(void) {
    cui_questionnaire_choice choices[2] = {{.value = str("a")}, {.value = str("b")}};
    cui_questionnaire_item items[2] = {
        {.name = str("hidden"), .disabled = true, .choices = choices, .choice_count = 2},
        {.name = str("active"), .required = true, .choices = choices, .choice_count = 2}};
    cui_questionnaire_item_runtime runtime[2];
    cui_questionnaire_choice_runtime selected[4];
    cui_questionnaire_state s = {0};
    cui_questionnaire_workspace w = {
        .items = runtime, .item_capacity = 2, .choices = selected, .choice_capacity = 4};
    assert(cui_questionnaire_init(&s, items, 2, w, NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(s.current == 1);
    assert(cui_questionnaire_choose(&s, 1, 0, 10, false) == CUI_QUESTIONNAIRE_OK);
    assert(s.pending_confirm);
    size_t answer = 1;
    assert(cui_questionnaire_set_answer(&s, 0, &answer, 1, (cui_str){0}) == CUI_QUESTIONNAIRE_OK);
    assert(s.pending_confirm);
    assert(!cui_questionnaire_choice_selected(&s, 0, 1));
    assert(!cui_questionnaire_tick(&s, 10.1));
    assert(cui_questionnaire_tick(&s, 10.2));
    assert(s.complete);
    answer = 0;
    assert(cui_questionnaire_set_answer(&s, 0, &answer, 1, (cui_str){0}) == CUI_QUESTIONNAIRE_OK);
    assert(s.complete);
    assert(cui_questionnaire_disable_item(&s, 0, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_choice_selected(&s, 0, 0));
    assert(!s.complete);
}
static cui_questionnaire_event_kind events[32];
static size_t event_count;
static void record(const cui_questionnaire_state *s, cui_questionnaire_event e, void *user) {
    (void)s;
    (void)user;
    assert(event_count < 32);
    events[event_count++] = e.kind;
}
static void test_validation_skip_completion_order(void) {
    cui_questionnaire_choice choice = {.value = str("yes")};
    cui_questionnaire_item items[2] = {
        {.name = str("required"), .required = true, .choices = &choice, .choice_count = 1},
        {.name = str("optional"), .choices = &choice, .choice_count = 1}};
    cui_questionnaire_item_runtime runtime[2];
    cui_questionnaire_choice_runtime selected[2];
    cui_questionnaire_state s = {0};
    cui_questionnaire_workspace w = {
        .items = runtime, .item_capacity = 2, .choices = selected, .choice_capacity = 2};
    event_count = 0;
    assert(cui_questionnaire_init(&s, items, 2, w, record, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(!cui_questionnaire_submit(&s));
    assert(s.current == 0);
    assert(cui_questionnaire_item_error(&s, 0).kind == CUI_QUESTIONNAIRE_REQUIRED);
    assert(cui_questionnaire_item_error(&s, 1).kind == CUI_QUESTIONNAIRE_UNFILLED);
    assert(!cui_questionnaire_skip(&s));
    assert(cui_questionnaire_choose(&s, 0, 0, 2, true) == CUI_QUESTIONNAIRE_OK);
    assert(s.current == 0 && s.pending_confirm);
    assert(cui_questionnaire_tick(&s, 2));
    assert(s.current == 1);
    assert(cui_questionnaire_skip(&s));
    assert(s.complete && cui_questionnaire_item_status(&s, 1) == CUI_QUESTIONNAIRE_SKIPPED);
    assert(events[event_count - 2] == CUI_QUESTIONNAIRE_COMPLETED);
    assert(events[event_count - 1] == CUI_QUESTIONNAIRE_SUBMIT);
    size_t before = event_count;
    assert(cui_questionnaire_submit(&s));
    assert(event_count == before + 1 && events[before] == CUI_QUESTIONNAIRE_SUBMIT);
    assert(cui_questionnaire_activate(&s, 1, 0) == CUI_QUESTIONNAIRE_OK);
    assert(!s.complete);
    assert(cui_questionnaire_submit(&s));
    assert(events[event_count - 2] == CUI_QUESTIONNAIRE_COMPLETED);
    assert(events[event_count - 1] == CUI_QUESTIONNAIRE_SUBMIT);
}
static void test_input_draft_reset_and_capacity(void) {
    char buffer[32] = "initial";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    assert(edit);
    cui_questionnaire_choice choice = {.value = str("yes"), .default_selected = true};
    cui_questionnaire_item item = {
        .name = str("answer"), .choices = &choice, .choice_count = 1, .input = edit};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected;
    char text[3 * 9];
    cui_questionnaire_workspace w = {.items = &runtime,
                                     .item_capacity = 1,
                                     .choices = &selected,
                                     .choice_capacity = 1,
                                     .text = text,
                                     .text_capacity = sizeof text,
                                     .max_answer_bytes = 8};
    cui_questionnaire_state s = {0};
    event_count = 0;
    assert(cui_questionnaire_init(&s, &item, 1, w, record, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_freeform(&s, 0).length == 7);
    assert(!cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_questionnaire_activate(&s, 0, 0) == CUI_QUESTIONNAIRE_OK);
    assert(!cui_questionnaire_freeform(&s, 0).length);
    assert(!strcmp(buffer, "initial"));
    assert(event_count == 1);
    assert(cui_questionnaire_set_input(&s, 0, str("draft")) == CUI_QUESTIONNAIRE_OK);
    assert(event_count == 1 && !cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_questionnaire_set_input(&s, 0, str("too long!")) == CUI_QUESTIONNAIRE_CAPACITY);
    assert(!strcmp(buffer, "draft") && cui_questionnaire_freeform(&s, 0).length == 5);
    assert(cui_questionnaire_set_input(&s, 0, (cui_str){"\xff", 1}) == CUI_QUESTIONNAIRE_INVALID);
    assert(!strcmp(buffer, "draft"));
    size_t chosen = 0;
    assert(cui_questionnaire_set_answer(&s, 0, &chosen, 1, (cui_str){0}) == CUI_QUESTIONNAIRE_OK);
    assert(!strcmp(buffer, "draft") && cui_questionnaire_choice_selected(&s, 0, 0));
    cui_str external = str("Server rejected");
    assert(cui_questionnaire_external_error(&s, 0, &external) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_disable_choice(&s, 0, 0, true) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_reset(&s));
    assert(!strcmp(buffer, "initial") && cui_questionnaire_freeform(&s, 0).length == 7);
    assert(selected.disabled && runtime.has_external_error);
    assert(cui_questionnaire_item_error(&s, 0).kind == CUI_QUESTIONNAIRE_MESSAGE);
    assert(event_count == 1);
    cui_edit_destroy(edit);
}
static void test_schema_and_shortcuts(void) {
    cui_questionnaire_choice choices[3] = {
        {.value = str("a"), .disabled = true}, {.value = str("b")}, {.value = str("c")}};
    cui_questionnaire_item items[2] = {{.name = str("one"), .choices = choices, .choice_count = 3},
                                       {.name = str("one"), .choices = choices, .choice_count = 3}};
    cui_questionnaire_item_runtime runtime[2];
    cui_questionnaire_choice_runtime selected[6];
    cui_questionnaire_workspace w = {
        .items = runtime, .item_capacity = 2, .choices = selected, .choice_capacity = 6};
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(&s, items, 2, w, NULL, NULL) == CUI_QUESTIONNAIRE_DUPLICATE_ITEM);
    assert(!s.initialized);
    items[1].name = str("two");
    choices[2].value = str("b");
    assert(cui_questionnaire_init(&s, items, 2, w, NULL, NULL) ==
           CUI_QUESTIONNAIRE_DUPLICATE_CHOICE);
    choices[2].value = str("c");
    choices[0].default_selected = choices[1].default_selected = true;
    assert(cui_questionnaire_init(&s, items, 2, w, NULL, NULL) ==
           CUI_QUESTIONNAIRE_MULTIPLE_DEFAULTS);
    choices[0].default_selected = choices[1].default_selected = false;
    assert(cui_questionnaire_init(&s, items, 2, w, NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_shortcut(&s, 0, 0, false) == 0);
    assert(cui_questionnaire_shortcut(&s, 0, 1, false) == 'A');
    assert(cui_questionnaire_shortcut(&s, 0, 2, true) == '2');
    assert(cui_questionnaire_activate_shortcut(&s, 'b', false, 1, false));
    assert(cui_questionnaire_choice_selected(&s, 0, 2) && s.pending_confirm);
    assert(cui_questionnaire_activate(&s, 0, 1) == CUI_QUESTIONNAIRE_OK);
    assert(!s.pending_confirm); /* Radio arrow selection never auto-confirms. */
    assert(!cui_questionnaire_tick(&s, 2) && s.current == 0);
    assert(cui_questionnaire_disable_choice(&s, 0, 1, true) == CUI_QUESTIONNAIRE_OK);
    assert(!cui_questionnaire_choice_selected(&s, 0, 1));
    assert(cui_questionnaire_disable_choice(&s, 0, 1, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_choice_selected(&s, 0, 1));
}
static void test_choice_positions(void) {
    cui_questionnaire_choice choices[] = {
        {.value = str("a"), .disabled = true}, {.value = str("b")}, {.value = str("c")}};
    cui_questionnaire_item item = {.name = str("item"), .choices = choices, .choice_count = 3};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected[3];
    cui_questionnaire_state state = {0};
    size_t position = 91, total = 92;
    assert(!cui_questionnaire_choice_position(NULL, 0, 0, &position, &total));
    assert(!cui_questionnaire_choice_position(&state, 0, 0, &position, &total));
    assert(position == 91 && total == 92);
    assert(cui_questionnaire_init(&state, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = selected,
                                                                .choice_capacity = 3},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(!cui_questionnaire_choice_position(&state, 0, 0, &position, &total));
    assert(!cui_questionnaire_choice_position(&state, 1, 1, &position, &total));
    assert(!cui_questionnaire_choice_position(&state, 0, 3, &position, &total));
    assert(!cui_questionnaire_choice_position(&state, 0, 1, NULL, &total));
    assert(!cui_questionnaire_choice_position(&state, 0, 1, &position, NULL));
    assert(position == 91 && total == 92);
    assert(cui_questionnaire_choice_position(&state, 0, 1, &position, &total));
    assert(position == 1 && total == 2);
    assert(cui_questionnaire_choice_position(&state, 0, 2, &position, &total));
    assert(position == 2 && total == 2);
    assert(cui_questionnaire_disable_choice(&state, 0, 0, false) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_choice_position(&state, 0, 2, &position, &total));
    assert(position == 3 && total == 3);
    assert(cui_questionnaire_disable_item(&state, 0, true) == CUI_QUESTIONNAIRE_OK);
    assert(!cui_questionnaire_choice_position(&state, 0, 2, &position, &total));
    assert(position == 3 && total == 3);
}
int main(void) {
    test_disabled_retained_answer_preserves_pending();
    test_validation_skip_completion_order();
    test_input_draft_reset_and_capacity();
    test_schema_and_shortcuts();
    test_choice_positions();
    puts("questionnaire tests passed");
    return 0;
}
