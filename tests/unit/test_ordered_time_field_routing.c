/* SPDX-License-Identifier: MIT */
/* Same-TU prototype. Consume Core's real route and the actual V1 digit reducer.
 * Private functions here exercise the admission design without exporting APIs. */
#include "../../src/base/cui_time_field_v1.c"
#include "../../src/core/cui.c"
#include <assert.h>
#include <stdio.h>

enum { BEFORE = 10, FIELD = 20, AFTER = 30, MODAL = 40 };

typedef struct {
    cui_time_field_options_v1 options;
    cui_time_field_state_v1 state;
    cui_time value;
    size_t edits, rejected;
    bool filter_text, observe_internal_tab;
} working_time;

static cui_core_ordered_result step(void *user, const cui_event *event, cui_id target) {
    working_time *work = user;
    cui_time_field_state_v1 *state = &work->state;
    if (event->type == CUI_EVENT_POINTER_DOWN) {
        for (unsigned i = 0; i < 4; ++i)
            if (cui_tfw_visible(&work->options, i) && target == cui_time_segment_id(FIELD, i)) {
                state->segment = i;
                cui_tfw_reset(state);
                return (cui_core_ordered_result){.consume = true};
            }
    } else if (event->type == CUI_EVENT_TEXT) {
        if (!work->filter_text) {
            work->edits++;
            if (!cui_tfw_text_edit(state, &work->value, &work->options, cui_tfw_text(event)))
                work->rejected++;
        }
        return (cui_core_ordered_result){.consume = true};
    } else if (event->key == CUI_KEY_TAB) {
        unsigned next =
            cui_tfw_next(&work->options, state->segment, !(event->modifiers & CUI_MOD_SHIFT));
        if (next != state->segment) {
            state->segment = next;
            cui_tfw_reset(state);
            return (cui_core_ordered_result){.consume = !work->observe_internal_tab,
                                             .tab_moved = true};
        }
    }
    return (cui_core_ordered_result){0};
}

static cui_context *create(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 180});
    assert(ui);
    return ui;
}

static working_time time_work(void) {
    working_time work = {.value = {9, 30, 0}};
    assert(cui_time_field_options_init_v1(&work.options) == CUI_TIME_TEXT_OK);
    assert(cui_time_field_state_init_v1(&work.state, work.value, &work.options) ==
           CUI_TIME_TEXT_OK);
    return work;
}

static cui_core_ordered_input admission(working_time *work) {
    return (cui_core_ordered_input){.root = FIELD, .step = step, .user = work};
}

static bool begin_probe(cui_context *ui, const cui_core_ordered_input *input) {
    return cui_core_ordered_ready(ui, input->root) &&
           cui_core_begin_ordered_frame(ui, 640, 180, 0, input, 1);
}

static cui_interaction button(cui_context *ui, cui_id id) {
    cui_begin_box(ui, id, (cui_box){.width = 52, .height = 28});
    cui_interaction result =
        cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("Button"), false, false, 0);
    cui_end_box(ui);
    return result;
}

static void field(cui_context *ui, const working_time *work, bool disabled) {
    cui_begin_box(ui, FIELD, (cui_box){.row = true, .gap = 4});
    cui_interact(ui, FIELD, CUI_ROLE_GROUP, cui_string("Time"), disabled, false, 0);
    cui_set_semantic_details(ui, FIELD, (cui_semantic_details){.focusable = true});
    cui_set_tab_stop(ui, FIELD, true);
    for (unsigned i = 0; i < 4; ++i) {
        if (!cui_tfw_visible(&work->options, i))
            continue;
        cui_id child = cui_time_segment_id(FIELD, i);
        cui_begin_box(ui, child, (cui_box){.width = 40, .height = 28});
        cui_interact(ui, child, CUI_ROLE_TEXTBOX, cui_time_field_v1_name(&work->options, i),
                     disabled, false, (float)cui_tfw_value(work->value, &work->options, i));
        cui_set_semantic_details(
            ui, child,
            (cui_semantic_details){.no_focus = true, .focus_target = FIELD, .parent = FIELD});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}

static void end(cui_context *ui) {
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && cui_accessibility_numeric_snapshot_ready(ui));
}

static void prime(cui_context *ui, const working_time *work, bool disabled) {
    assert(cui_begin_frame(ui, 640, 180, 0));
    button(ui, BEFORE);
    field(ui, work, disabled);
    button(ui, AFTER);
    end(ui);
}

static void key(cui_context *ui, cui_key value, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = value, .modifiers = modifiers}));
}

static void digit(cui_context *ui, const char *value) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = value, .text_length = strlen(value)}));
}

static void mouse(cui_context *ui, cui_id target) {
    cui_rect r = cui_bounds(ui, target);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
}

static void ordered_batch(bool filter_text, bool reject_value) {
    working_time owner = time_work();
    if (reject_value) {
        owner.options.text.flags = CUI_TIME_TEXT_HAS_MINIMUM_V1;
        owner.options.text.minimum = (cui_time){9, 0, 0};
    }
    cui_context *ui = create();
    prime(ui, &owner, false);
    cui_focus(ui, FIELD);
    cui_rect button_bounds = cui_bounds(ui, AFTER);
    working_time work = owner;
    work.filter_text = filter_text;
    cui_core_ordered_input input = admission(&work);
    char token[] = "3";
    digit(ui, token);
    token[0] = '9'; /* Core owns an exact copy of the queued digit. */
    key(ui, CUI_KEY_TAB, 0);
    key(ui, CUI_KEY_ENTER, 0);
    assert(begin_probe(ui, &input));
    assert(cui_frame_input_snapshot_ready(ui));
    assert(cui_event_target(ui, 0) == FIELD && cui_event_target(ui, 1) == FIELD);
    assert(cui_event_consumed(ui, 0) && cui_event_consumed(ui, 1) == filter_text);
    assert(cui_event_target(ui, 2) == (filter_text ? FIELD : AFTER));
    assert(cui_focused(ui) == (filter_text ? FIELD : AFTER));
    assert(work.edits == (filter_text ? 0u : 1u) && work.rejected == (reject_value ? 1u : 0u));
    assert(work.value.hour == (filter_text || reject_value ? 9u : 3u));
    assert(work.state.segment == CUI_TIME_FIELD_MINUTE_V1);
    assert(owner.value.hour == 9 && owner.state.segment == CUI_TIME_FIELD_HOUR_V1);
    assert(cui_event_accepted_activation(ui, 2) == 0);

    button(ui, BEFORE);
    field(ui, &work, false);
    cui_interaction after = button(ui, AFTER);
    assert(after.activated == !filter_text);
    if (!filter_text) {
        cui_activation_snapshot receipt;
        assert(cui_event_activation_snapshot(ui, 2, &receipt));
        assert(receipt.target == AFTER && receipt.event_index == 2 &&
               receipt.event_type == CUI_EVENT_KEY_DOWN && receipt.key == CUI_KEY_ENTER &&
               receipt.has_key_bounds && receipt.key_bounds.x == button_bounds.x &&
               receipt.key_bounds.w == button_bounds.w && !receipt.has_press &&
               !receipt.has_release);
    }
    end(ui);
    assert(begin_probe(ui, &input));
    assert(work.edits == (filter_text ? 0u : 1u) && !button(ui, AFTER).activated);
    end(ui);
    cui_destroy(ui);
}

static void retain_selection(void) {
    working_time work = time_work();
    work.observe_internal_tab = true;
    work.options.text.precision = CUI_TIME_TEXT_SECOND;
    cui_context *ui = create();
    prime(ui, &work, false);
    cui_core_ordered_input input = admission(&work);
    cui_id minute = cui_time_segment_id(FIELD, CUI_TIME_FIELD_MINUTE_V1);
    mouse(ui, minute);
    key(ui, CUI_KEY_TAB, CUI_MOD_SHIFT); /* Minute to Hour, logically still in field. */
    key(ui, CUI_KEY_TAB, CUI_MOD_SHIFT); /* Hour to previous button. */
    key(ui, CUI_KEY_ENTER, 0);
    key(ui, CUI_KEY_TAB, 0); /* Re-enter on Hour. */
    key(ui, CUI_KEY_TAB, 0); /* Hour to Minute. */
    assert(begin_probe(ui, &input));
    assert(cui_event_target(ui, 0) == minute && cui_event_consumed(ui, 0));
    assert(cui_event_target(ui, 1) == FIELD && !cui_event_consumed(ui, 1));
    assert(cui_event_target(ui, 2) == FIELD && !cui_event_consumed(ui, 2));
    assert(cui_event_target(ui, 3) == BEFORE && cui_event_target(ui, 4) == BEFORE);
    assert(cui_event_target(ui, 5) == FIELD && work.state.segment == CUI_TIME_FIELD_MINUTE_V1);
    assert(cui_focused(ui) == FIELD && button(ui, BEFORE).activated);
    field(ui, &work, false);
    button(ui, AFTER);
    end(ui);

    cui_id second = cui_time_segment_id(FIELD, CUI_TIME_FIELD_SECOND_V1);
    mouse(ui, second);
    key(ui, CUI_KEY_TAB, 0); /* Exit from Second. */
    key(ui, CUI_KEY_ENTER, 0);
    assert(cui_queue_accessibility_action(ui, FIELD, CUI_ACCESSIBILITY_FOCUS));
    assert(begin_probe(ui, &input));
    assert(cui_event_target(ui, 0) == second && cui_event_target(ui, 2) == AFTER &&
           cui_event_target(ui, 3) == FIELD);
    assert(cui_focused(ui) == FIELD && work.state.segment == CUI_TIME_FIELD_SECOND_V1);
    field(ui, &work, false);
    assert(button(ui, AFTER).activated);
    end(ui);
    cui_destroy(ui);
}

static void invalid_admissions(void) {
    working_time work = time_work();
    cui_context *ui = create();
    cui_core_ordered_input input = admission(&work);
    digit(ui, "3");
    assert(!begin_probe(ui, &input)); /* No layout yet. */
    assert(ui->queued_count == 1 && !work.edits && cui_focused(ui) == 0);
    prime(ui, &work, true);
    digit(ui, "3");
    assert(!begin_probe(ui, &input)); /* Disabled prior root. */
    assert(ui->queued_count == 1 && !work.edits);
    prime(ui, &work, false);
    assert(cui_begin_frame(ui, 640, 180, 0));
    field(ui, &work, false);
    assert(cui_begin_focus_scope(ui, MODAL, true));
    button(ui, AFTER);
    cui_end_focus_scope(ui);
    end(ui);
    digit(ui, "3");
    assert(!begin_probe(ui, &input)); /* Root outside modal. */
    assert(ui->queued_count == 1 && !work.edits);
    assert(cui_begin_frame(ui, 640, 180, 0));
    assert(cui_event_target(ui, 0) == AFTER);
    end(ui); /* Omit the field. */
    digit(ui, "3");
    assert(!begin_probe(ui, &input));
    assert(ui->queued_count == 1 && !work.edits);
    cui_destroy(ui);
}

static void changed_mode_and_composition_guard(void) {
    cui_context *ui = create();
    working_time work = time_work();
    work.options.text.precision = CUI_TIME_TEXT_SECOND;
    prime(ui, &work, false);
    cui_id old_second = cui_time_segment_id(FIELD, CUI_TIME_FIELD_SECOND_V1);
    mouse(ui, old_second);
    work.options.text.precision = CUI_TIME_TEXT_MINUTE;
    assert(cui_time_field_state_init_v1(&work.state, work.value, &work.options) ==
           CUI_TIME_TEXT_OK);
    cui_core_ordered_input input = admission(&work);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = FIELD}));
    digit(ui, "3");
    key(ui, CUI_KEY_TAB, 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL, .composition_target = FIELD}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB, .repeat = true}));
    key(ui, CUI_KEY_TAB, 0);
    assert(begin_probe(ui, &input));
    assert(cui_event_target(ui, 0) == old_second && !cui_event_consumed(ui, 0));
    assert(cui_event_target(ui, 2) == FIELD && !cui_event_consumed(ui, 2));
    assert(cui_event_target(ui, 3) == FIELD && !cui_event_consumed(ui, 3));
    assert(cui_event_target(ui, 5) == FIELD && !cui_event_consumed(ui, 5));
    assert(cui_event_target(ui, 6) == FIELD && cui_event_consumed(ui, 6));
    assert(work.edits == 0 && work.state.segment == CUI_TIME_FIELD_MINUTE_V1);
    assert(cui_focused(ui) == FIELD && work.value.hour == 9);
    field(ui, &work, false);
    button(ui, AFTER);
    end(ui);
    assert(cui_bounds(ui, old_second).w == 0);
    work.state.segment = CUI_TIME_FIELD_HOUR_V1;
    key(ui, CUI_KEY_TAB, CUI_MOD_CTRL);
    assert(begin_probe(ui, &input));
    assert(cui_focused(ui) == AFTER && work.state.segment == CUI_TIME_FIELD_HOUR_V1 &&
           !cui_event_consumed(ui, 0));
    end(ui);
    cui_destroy(ui);
}

static void failed_frame_keeps_owner(void) {
    const working_time owner = time_work();
    cui_context *ui = create();
    prime(ui, &owner, false);
    cui_focus(ui, FIELD);
    working_time work = owner;
    cui_core_ordered_input input = admission(&work);
    digit(ui, "3");
    assert(begin_probe(ui, &input));
    assert(work.value.hour == 3 && owner.value.hour == 9);
    cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_LAYOUT});
    assert(!cui_frame_input_snapshot_ready(ui));
    (void)cui_end_frame(ui);
    cui_clear_error(ui);
    assert(!begin_probe(ui, &input));

    work = owner;
    prime(ui, &owner, false);
    assert(begin_probe(ui, &input));
    assert(work.value.hour == 9 && work.edits == 0);
    end(ui);
    cui_destroy(ui);
}

static void host_tab_boundary(void) {
    working_time work = time_work();
    cui_context *ui = create();
    prime(ui, &work, false);
    cui_core_ordered_input input = admission(&work);
    cui_focus(ui, FIELD);
    assert(cui_core_ordered_queue_tab(ui, false));
    assert(!cui_core_ordered_queue_tab(ui, false));
    assert(begin_probe(ui, &input));
    assert(cui_focused(ui) == FIELD && cui_event_consumed(ui, 0));
    assert(cui_core_ordered_tab_result(ui) == CUI_ORDERED_TAB_UNRESOLVED);
    button(ui, BEFORE);
    field(ui, &work, false);
    button(ui, AFTER);
    end(ui);
    assert(cui_core_ordered_tab_result(ui) == CUI_ORDERED_TAB_INSIDE);

    work.state.segment = CUI_TIME_FIELD_HOUR_V1;
    digit(ui, "3");
    assert(cui_core_ordered_queue_tab(ui, false));
    assert(begin_probe(ui, &input));
    assert(cui_focused(ui) == AFTER && cui_event_target(ui, 1) == FIELD &&
           !cui_event_consumed(ui, 1));
    button(ui, BEFORE);
    field(ui, &work, false);
    button(ui, AFTER);
    end(ui);
    assert(work.value.hour == 3 && cui_core_ordered_tab_result(ui) == CUI_ORDERED_TAB_INSIDE);

    assert(cui_core_ordered_queue_tab(ui, false));
    key(ui, CUI_KEY_ENTER, 0);
    assert(begin_probe(ui, &input));
    assert(!cui_focused(ui) && !cui_event_target(ui, 1) && !cui_event_activation(ui, 1) &&
           !cui_event_consumed(ui, 0));
    button(ui, BEFORE);
    field(ui, &work, false);
    button(ui, AFTER);
    end(ui);
    assert(cui_core_ordered_tab_result(ui) == CUI_ORDERED_TAB_LEAVE);

    cui_focus(ui, BEFORE);
    assert(cui_core_ordered_queue_tab(ui, true));
    assert(begin_probe(ui, &input));
    assert(!cui_focused(ui) && !cui_event_consumed(ui, 0));
    button(ui, BEFORE);
    field(ui, &work, false);
    button(ui, AFTER);
    end(ui);
    assert(cui_core_ordered_tab_result(ui) == CUI_ORDERED_TAB_LEAVE);

    /* Unmarked traversal retains the ordinary desktop wrap and clears receipts. */
    cui_focus(ui, BEFORE);
    key(ui, CUI_KEY_TAB, CUI_MOD_SHIFT);
    assert(begin_probe(ui, &input));
    assert(cui_focused(ui) == AFTER);
    end(ui);
    assert(cui_core_ordered_tab_result(ui) == CUI_ORDERED_TAB_UNRESOLVED);
    cui_destroy(ui);
}

static void host_tab_modal_wraps(void) {
    working_time work = time_work();
    cui_context *ui = create();
    assert(cui_begin_frame(ui, 640, 180, 0));
    assert(cui_begin_focus_scope(ui, MODAL, true));
    button(ui, BEFORE);
    field(ui, &work, false);
    button(ui, AFTER);
    cui_end_focus_scope(ui);
    end(ui);

    cui_focus(ui, AFTER);
    cui_core_ordered_input input = admission(&work);
    assert(cui_core_ordered_queue_tab(ui, false));
    assert(begin_probe(ui, &input));
    assert(cui_focused(ui) == BEFORE && !cui_event_consumed(ui, 0));
    assert(cui_begin_focus_scope(ui, MODAL, true));
    button(ui, BEFORE);
    field(ui, &work, false);
    button(ui, AFTER);
    cui_end_focus_scope(ui);
    end(ui);
    assert(cui_core_ordered_tab_result(ui) == CUI_ORDERED_TAB_INSIDE);
    cui_destroy(ui);
}

int main(void) {
    ordered_batch(false, false);
    ordered_batch(true, false);
    ordered_batch(false, true);
    retain_selection();
    invalid_admissions();
    changed_mode_and_composition_guard();
    failed_frame_keeps_owner();
    host_tab_boundary();
    host_tab_modal_wraps();
    puts("ordered TimeField dispatch experiment: PASS");
    return 0;
}
