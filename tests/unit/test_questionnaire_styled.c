/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire.h"
#include <assert.h>
#include <stdio.h>
static cui_questionnaire_input_state input_view;
static void frame(cui_context *ui, cui_questionnaire_state *s, cui_questionnaire_options o) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_theme t = cui_theme_light();
    cui_id answer_id = 12;
    assert(cui_questionnaire_begin(ui, 10, &t, s, o));
    (void)cui_questionnaire_answer_keys(ui, 10, s, &answer_id, 1, 19);
    (void)cui_questionnaire_shortcut_keys(ui, 10, s, &answer_id, 1, 19, o.numeric_shortcuts);
    cui_questionnaire_progress(ui, 11, &t, s, o);
    cui_questionnaire_heading(ui, 13, &t, s, 0, o);
    (void)cui_questionnaire_choice_control(ui, 12, &t, s, 0, 0, o);
    if (s->items[0].input)
        assert(cui_questionnaire_input_control(ui, 19, &t, s, 0, &input_view, NULL, o) ==
               CUI_QUESTIONNAIRE_OK);
    cui_questionnaire_error_control(ui, 14, &t, s, 0, o);
    cui_begin_box(ui, 15, (cui_box){.row = true, .gap = 8});
    (void)cui_questionnaire_action_control(ui, 16, &t, s, CUI_QUESTIONNAIRE_PREVIOUS, (cui_str){0},
                                           o);
    (void)cui_questionnaire_action_control(ui, 17, &t, s, CUI_QUESTIONNAIRE_SKIP, (cui_str){0}, o);
    (void)cui_questionnaire_action_control(ui, 18, &t, s, CUI_QUESTIONNAIRE_FINISH, (cui_str){0},
                                           o);
    cui_end_box(ui);
    cui_questionnaire_end(ui);
    cui_draw_list draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && draws.count > 0);
}
static void shortcut_frame(cui_context *ui, cui_questionnaire_state *s, bool numbers) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_theme t = cui_theme_light();
    cui_id ids[] = {22, 23, 24};
    (void)cui_questionnaire_radio_keys(ui, s, ids, 3);
    assert(cui_questionnaire_begin(ui, 20, &t, s, (cui_questionnaire_options){.width = 300}));
    (void)cui_questionnaire_answer_keys(ui, 20, s, ids, 3, 0);
    (void)cui_questionnaire_confirm_keys(ui, 20, s, ids, 3, 0, CUI_MOD_CTRL);
    (void)cui_questionnaire_shortcut_keys(ui, 20, s, ids, 3, 0, numbers);
    for (size_t i = 0; i < 3; i++)
        (void)cui_questionnaire_choice_control(ui, ids[i], &t, s, 0, i,
                                               (cui_questionnaire_options){0});
    cui_begin_box(ui, 26, (cui_box){.width = 60, .height = 20});
    cui_interact(ui, 26, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    cui_end_box(ui);
    cui_questionnaire_end(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void radio_frame(cui_context *ui, cui_questionnaire_state *s) {
    shortcut_frame(ui, s, false);
}
static void radio_keyboard(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_questionnaire_choice choices[] = {{.value = cui_string("a")},
                                          {.value = cui_string("b"), .disabled = true},
                                          {.value = cui_string("c")}};
    cui_questionnaire_item item = {
        .name = cui_string("radio"), .choices = choices, .choice_count = 3};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected[3];
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = selected,
                                                                .choice_capacity = 3},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    radio_frame(ui, &s);
    cui_focus(ui, 22);
    const cui_event ignored[] = {
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .repeat = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .modifiers = CUI_MOD_CTRL},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .ime_composing = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .prefer_character_input = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT, .ime_composing = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .ime_composing = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_C}};
    for (size_t i = 0; i < sizeof ignored / sizeof ignored[0]; i++) {
        assert(cui_queue_event(ui, ignored[i]));
        radio_frame(ui, &s);
        assert(!cui_questionnaire_choice_selected(&s, 0, 0));
        assert(!cui_questionnaire_choice_selected(&s, 0, 2));
        assert(!s.pending_confirm && !s.complete && !cui_event_consumed(ui, 0));
    }
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B}));
    radio_frame(ui, &s);
    assert(cui_questionnaire_choice_selected(&s, 0, 2));
    assert(cui_focused(ui) == 24 && s.pending_confirm && cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B}));
    radio_frame(ui, &s);
    assert(s.complete && cui_event_consumed(ui, 0));
    assert(cui_questionnaire_reset(&s));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_2}));
    shortcut_frame(ui, &s, true);
    assert(cui_questionnaire_choice_selected(&s, 0, 2) && cui_focused(ui) == 24);
    assert(cui_questionnaire_reset(&s));
    cui_focus(ui, 22);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 24 && cui_questionnaire_choice_selected(&s, 0, 2));
    assert(!s.pending_confirm && !s.complete && cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 22 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT, .repeat = true}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 22 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 24 && cui_questionnaire_choice_selected(&s, 0, 2));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    radio_frame(ui, &s);
    assert(s.complete && cui_event_consumed(ui, 0));
    assert(cui_questionnaire_reset(&s));
    cui_focus(ui, 22);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .modifiers = CUI_MOD_CTRL}));
    radio_frame(ui, &s);
    assert(!s.complete && cui_questionnaire_item_error(&s, 0).kind == CUI_QUESTIONNAIRE_UNFILLED);
    assert(cui_questionnaire_activate(&s, 0, 2) == CUI_QUESTIONNAIRE_OK);
    cui_focus(ui, 26);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 24 && cui_questionnaire_choice_selected(&s, 0, 2));
    assert(!cui_questionnaire_choice_selected(&s, 0, 0) && !s.pending_confirm);
    assert(cui_questionnaire_activate(&s, 0, 0) == CUI_QUESTIONNAIRE_OK);
    cui_focus(ui, 26);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 22 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(!cui_questionnaire_choice_selected(&s, 0, 2) && !s.pending_confirm);

    cui_destroy(ui);
}
static void progress_semantics(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_questionnaire_item items[3] = {
        {.name = cui_string("one")}, {.name = cui_string("two")}, {.name = cui_string("three")}};
    cui_questionnaire_item_runtime runtime[3];
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(
               &s, items, 3, (cui_questionnaire_workspace){.items = runtime, .item_capacity = 3},
               NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    cui_theme theme = cui_theme_light();
    for (size_t item = 0; item < 3; item++) {
        assert(cui_questionnaire_set_current(&s, item, false) == CUI_QUESTIONNAIRE_OK);
        assert(cui_begin_frame(ui, 400, 300, .016f));
        cui_questionnaire_progress(ui, 31, &theme, &s, (cui_questionnaire_options){0});
        cui_end_frame(ui);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 1 && sem[0].role == CUI_ROLE_PROGRESSBAR);
        assert(sem[0].value == (float)(item + 1));
        assert(sem[0].details.minimum == 0 && sem[0].details.maximum == 3);
    }
    cui_destroy(ui);
}
int main(void) {
    progress_semantics();
    radio_keyboard();
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_questionnaire_choice choice = {.value = cui_string("yes"),
                                       .label = cui_string("Yes"),
                                       .description = cui_string("A detailed answer")};
    cui_questionnaire_item item = {
        .name = cui_string("question"), .multiple = true, .choices = &choice, .choice_count = 1};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected;
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = &selected,
                                                                .choice_capacity = 1},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    for (int size = 0; size < 4; size++) {
        cui_questionnaire_options o = {.size = (cui_questionnaire_size)size, .width = 300};
        cui_str description = choice.description;
        choice.description = (cui_str){0};
        frame(ui, &s, o);
        const float minimum[] = {44, 36, 52, 28};
        assert(cui_bounds(ui, 12).h == minimum[size]);
        choice.description = description;
        frame(ui, &s, o);
        cui_rect bounds = cui_bounds(ui, 12);
        assert(bounds.w == 300 && bounds.h > 28);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                               .x = bounds.x + 20,
                                               .y = bounds.y + bounds.h / 2}));
        frame(ui, &s, o);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                               .x = bounds.x + 20,
                                               .y = bounds.y + bounds.h / 2}));
        frame(ui, &s, o);
        assert(cui_questionnaire_choice_selected(&s, 0, 0));
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        bool found = false;
        for (size_t i = 0; i < count; i++)
            if (sem[i].id == 12) {
                assert(sem[i].role == CUI_ROLE_CHECKBOX && sem[i].checked);
                found = true;
            }
        assert(found);
        assert(cui_questionnaire_reset(&s));
    }
    cui_questionnaire_options o = {.width = 300};
    assert(!cui_questionnaire_submit(&s));
    frame(ui, &s, o);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    bool error = false, skip = false, submit = false;
    for (size_t i = 0; i < count; i++) {
        assert(sem[i].id != 16); /* Previous hidden at the first item. */
        error |= sem[i].id == 14 && sem[i].role == CUI_ROLE_ALERT;
        skip |= sem[i].id == 17;
        submit |= sem[i].id == 18;
    }
    assert(error && skip && submit);
    cui_rect bounds = cui_bounds(ui, 17);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 4, .y = bounds.y + 4}));
    frame(ui, &s, o);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = bounds.x + 4, .y = bounds.y + 4}));
    frame(ui, &s, o);
    assert(s.complete && cui_questionnaire_item_status(&s, 0) == CUI_QUESTIONNAIRE_SKIPPED);
    char buffer[64] = {0}, storage[3 * 64];
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    assert(edit);
    item.input = edit;
    item.multiple = false;
    s = (cui_questionnaire_state){0};
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = &selected,
                                                                .choice_capacity = 1,
                                                                .text = storage,
                                                                .text_capacity = sizeof storage,
                                                                .max_answer_bytes = 63},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_activate(&s, 0, 0) == CUI_QUESTIONNAIRE_OK);
    frame(ui, &s, o);
    cui_focus(ui, 19);
    frame(ui, &s, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "custom"}));
    frame(ui, &s, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_A}));
    frame(ui, &s, o);
    assert(!cui_event_consumed(ui, 0) && cui_focused(ui) == 19);
    assert(cui_questionnaire_freeform(&s, 0).length == 6);
    assert(!cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    frame(ui, &s, o);
    assert(!s.complete);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame(ui, &s, o);
    assert(s.complete && cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    frame(ui, &s, o);
    assert(cui_focused(ui) == 19); /* Draft text retains cursor keys. */
    assert(cui_questionnaire_set_input(&s, 0, (cui_str){0}) == CUI_QUESTIONNAIRE_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    frame(ui, &s, o);
    assert(cui_focused(ui) == 12 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(!s.pending_confirm);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    frame(ui, &s, o);
    assert(cui_focused(ui) == 19 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_questionnaire_set_input(&s, 0, cui_string("custom")) == CUI_QUESTIONNAIRE_OK);
    o.disabled = true;
    frame(ui, &s, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "ignored"}));
    frame(ui, &s, o);
    assert(cui_questionnaire_freeform(&s, 0).length == 6);
    cui_edit_destroy(edit);
    cui_destroy(ui);
    puts("questionnaire styled tests passed");
    return 0;
}
