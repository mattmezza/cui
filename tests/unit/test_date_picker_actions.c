/* SPDX-License-Identifier: MIT */
#include "cui/cui_date_picker_actions.h"
#include "cui/cui_software.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
static unsigned checks;
typedef struct {
    cui_context *ui;
    bool styled, omit_presets;
    cui_theme theme;
    cui_date_picker_actions_state state;
    cui_date_picker_value value;
    cui_date_picker_actions_options options;
    cui_date_picker_preset presets[2];
    cui_date_picker_actions_result result;
    cui_draw_list draw;
    cui_id immediate_focus;
    unsigned predicate_calls;
    int blocked;
} fixture;
static bool disabled(void *u, cui_date d) {
    fixture *f = u;
    f->predicate_calls++;
    return d.day == f->blocked;
}
static cui_id root(fixture *f) {
    return f->styled ? cui_date_picker_calendar_id(31) : 10;
}
static void create(fixture *f, bool styled) {
    *f = (fixture){.styled = styled,
                   .theme = cui_theme_light(),
                   .value = {.dates = {.start = {2024, 2, 15}}},
                   .options = {.picker = {.viewport = {0, 0, 1000, 800}},
                               .clear_id = 33,
                               .clear_label = {"Clear", 5}}};
    f->ui = cui_create(
        (cui_config){.width = 1000, .height = 800, .max_elements = 1024, .max_commands = 2048});
    CHECK(f->ui);
    f->presets[0] = (cui_date_picker_preset){
        .id = 34, .label = {"March", 5}, .value = {.dates = {.start = {2025, 3, 20}}}};
    f->presets[1] = (cui_date_picker_preset){
        .id = 35,
        .label = {"Range", 5},
        .value = {.shape = CUI_DATE_PICKER_RANGE,
                  .dates = {.start = {2026, 4, 12}, .end = {2026, 4, 18}}}};
    f->options.presets = f->presets;
    f->options.preset_count = 2;
    f->options.picker.calendar.is_disabled = disabled;
    f->options.picker.calendar.disabled_user = f;
    cui_date_picker_actions_init(&f->state, 32, (cui_date){2024, 2, 15});
}
static const cui_semantic *semantic(fixture *f, cui_id id) {
    size_t count;
    const cui_semantic *s = cui_semantics(f->ui, &count);
    for (size_t i = 0; i < count; i++)
        if (s[i].id == id)
            return s + i;
    return NULL;
}
static void frame(fixture *f) {
    CHECK(cui_begin_frame(f->ui, 1000, 800, .016f));
    if (f->styled)
        f->result = cui_date_picker_actions(f->ui, 31, &f->theme, cui_string("Date"), &f->state,
                                            &f->value, f->options);
    else {
        f->result = cui_base_date_picker_actions(f->ui, 31, 10, cui_string("Date"), &f->state,
                                                 &f->value, f->options);
        if (f->result.valid) {
            cui_begin_box(f->ui, 31, (cui_box){.width = 180, .height = 32});
            cui_end_box(f->ui);
            if (f->options.cleanable && cui_date_valid(f->value.dates.start)) {
                cui_begin_box(f->ui, 33, (cui_box){.width = 100, .height = 32});
                cui_end_box(f->ui);
            }
            if (f->state.picker.overlay.open) {
                cui_rect bounds = {0, 80, 900, 650};
                cui_overlay_set_bounds(&f->state.picker.overlay, bounds);
                CHECK(cui_overlay_begin(f->ui, &f->state.picker.overlay));
                cui_begin_portal(f->ui, 32, (cui_box){0}, bounds, 100);
                cui_calendar_options o = f->options.picker.calendar;
                o.range = f->state.calendar_value.shape == CUI_DATE_PICKER_RANGE;
                o.disabled = o.disabled || f->options.picker.disabled;
                cui_calendar_submit_ex(f->ui, 10, &f->state.picker.calendar,
                                       f->state.calendar_value.dates, o,
                                       f->options.calendar.presentation);
                if (!f->omit_presets)
                    cui_date_picker_actions_submit_presets(f->ui, &f->state, f->options);
                size_t n;
                const cui_semantic *sem = cui_semantics(f->ui, &n);
                for (size_t i = 0; i < n; i++)
                    if (sem[i].id != 31 && sem[i].id != 33) {
                        cui_begin_box(f->ui, sem[i].id, (cui_box){.width = 80, .height = 3});
                        cui_end_box(f->ui);
                    }
                cui_end_box(f->ui);
                cui_overlay_end(f->ui);
            }
        }
    }
    f->immediate_focus = cui_focused(f->ui);
    f->draw = cui_end_frame(f->ui);
    if (cui_last_error(f->ui) != CUI_OK)
        fprintf(stderr, "FRAME ERR %d styled%d open%d date%d-%d-%d\n", cui_last_error(f->ui),
                f->styled, f->state.picker.overlay.open, f->value.dates.start.year,
                f->value.dates.start.month, f->value.dates.start.day);
    CHECK(cui_last_error(f->ui) == CUI_OK);
}
static void activate(fixture *f, cui_id id, unsigned kind) {
    if (kind == 0)
        CHECK(cui_queue_accessibility_action(f->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
    else if (kind == 1) {
        cui_focus(f->ui, id);
        CHECK(
            cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    } else {
        cui_rect r = cui_bounds(f->ui, id);
        CHECK(r.w > 0 && r.h > 0);
        CHECK(cui_queue_event(
            f->ui,
            (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
        CHECK(cui_queue_event(
            f->ui,
            (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
    }
}
static void open_picker(fixture *f) {
    frame(f);
    CHECK(f->result.valid);
    activate(f, 31, 0);
    frame(f);
    CHECK(f->state.picker.overlay.open);
}
static void source_vectors(bool styled, unsigned kind) {
    fixture f;
    create(&f, styled);
    open_picker(&f);
    CHECK(semantic(&f, 34)->tab_stop_disabled);
    activate(&f, 34, kind);
    frame(&f);
    CHECK(f.result.action_activated && f.result.changed && !f.state.picker.overlay.open);
    CHECK(f.value.dates.start.year == 2025 && f.state.calendar_value.dates.start.year == 2025);
    CHECK(f.state.picker.calendar.month.year == 2025 && f.state.picker.calendar.month.month == 3);
    CHECK(f.immediate_focus == 31);
    open_picker(&f);
    f.blocked = 12;
    f.state.picker.calendar.view = CUI_CALENDAR_YEARS;
    cui_date prior = f.state.picker.calendar.month;
    activate(&f, 35, kind);
    frame(&f);
    CHECK(f.result.action_activated && f.value.shape == CUI_DATE_PICKER_RANGE &&
          f.value.dates.start.year == 2026);
    CHECK(f.state.calendar_value.shape == CUI_DATE_PICKER_SINGLE &&
          f.state.calendar_value.dates.start.year == 2025);
    CHECK(!cui_date_compare(prior, f.state.picker.calendar.month));
    CHECK(f.state.picker.calendar.view == CUI_CALENDAR_YEARS);
    CHECK(f.immediate_focus == 31);
    f.blocked = 0;
    frame(&f);
    CHECK(f.state.calendar_value.shape == CUI_DATE_PICKER_SINGLE);
    open_picker(&f);
    CHECK(f.state.calendar_value.shape == CUI_DATE_PICKER_SINGLE);
    /* Source Selected is emitted for accepted complete child values, even unchanged. */
    f.state.picker.calendar.view = CUI_CALENDAR_DAYS;
    frame(&f);
    activate(&f, cui_calendar_day_id(root(&f), (cui_date){2025, 3, 20}), 0);
    frame(&f);
    CHECK(f.result.calendar.activated && !f.result.calendar.changed && f.result.changed &&
          f.value.shape == CUI_DATE_PICKER_SINGLE && f.value.dates.start.day == 20);
    f.blocked = 12;
    open_picker(&f);
    activate(&f, 35, 0);
    frame(&f);
    CHECK(f.value.shape == CUI_DATE_PICKER_RANGE &&
          f.state.calendar_value.shape == CUI_DATE_PICKER_SINGLE);
    f.blocked = 0;
    open_picker(&f);
    /* A genuine day change publishes the child's retained single shape. */
    activate(&f, cui_calendar_day_id(root(&f), (cui_date){2025, 3, 21}), 0);
    frame(&f);
    CHECK(f.result.calendar.changed && f.value.shape == CUI_DATE_PICKER_SINGLE &&
          f.value.dates.start.day == 21);
    CHECK(!cui_date_compare(f.value.dates.start, f.state.calendar_value.dates.start));
    cui_destroy(f.ui);
}
static void policies(bool styled) {
    fixture f;
    create(&f, styled);
    f.options.preset_policy = CUI_DATE_PICKER_PRESET_MATCH_SHAPE;
    open_picker(&f);
    activate(&f, 35, 0);
    frame(&f);
    CHECK(!f.result.action_activated && !f.result.changed && f.state.picker.overlay.open);
    f.options.preset_policy = CUI_DATE_PICKER_PRESET_MATCH_SHAPE_ENABLED;
    f.blocked = 20;
    activate(&f, 34, 0);
    frame(&f);
    CHECK(!f.result.action_activated && f.state.picker.overlay.open);
    f.options.picker.calendar.disabled = true;
    f.blocked = 0;
    activate(&f, 34, 0);
    frame(&f);
    CHECK(f.result.action_activated && f.value.dates.start.year == 2025);
    open_picker(&f);
    f.options.picker.disabled = true;
    activate(&f, 34, 0);
    frame(&f);
    CHECK(!f.result.action_activated && !f.state.picker.overlay.open);
    cui_destroy(f.ui);
}
static void clear_delete(bool styled, unsigned kind) {
    fixture f;
    create(&f, styled);
    f.options.cleanable = true;
    frame(&f);
    CHECK(semantic(&f, 33));
    activate(&f, 33, kind);
    frame(&f);
    CHECK(f.result.action_activated && f.result.changed && !cui_date_valid(f.value.dates.start));
    CHECK(!semantic(&f, 33));
    f.options.delete_policy = CUI_DATE_PICKER_DELETE_TRIGGER;
    cui_focus(f.ui, 31);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE}));
    frame(&f);
    CHECK(f.result.action_activated && !f.result.changed);
    f.value =
        (cui_date_picker_value){.shape = CUI_DATE_PICKER_RANGE, .dates = {.start = {2024, 2, 18}}};
    frame(&f);
    cui_focus(f.ui, 31);
    CHECK(cui_queue_event(
        f.ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE, .modifiers = CUI_MOD_CTRL}));
    frame(&f);
    CHECK(!f.result.action_activated && f.value.dates.start.day == 18);
    cui_focus(f.ui, 31);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_BACKSPACE}));
    frame(&f);
    CHECK(!f.result.action_activated);
    open_picker(&f);
    f.options.delete_policy = CUI_DATE_PICKER_DELETE_SCOPE;
    cui_date month = f.state.picker.calendar.month;
    f.state.picker.calendar.view = CUI_CALENDAR_YEARS;
    frame(&f);
    cui_focus(f.ui, 34);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE}));
    frame(&f);
    CHECK(f.result.action_activated && f.value.shape == CUI_DATE_PICKER_RANGE &&
          !cui_date_valid(f.value.dates.start));
    CHECK(!cui_date_compare(month, f.state.picker.calendar.month));
    CHECK(f.state.picker.calendar.view == CUI_CALENDAR_YEARS);
    CHECK(f.immediate_focus == 34); /* No explicit focus restoration during clear. */
    CHECK(!cui_date_valid(f.state.calendar_value.dates.start) &&
          !f.state.picker.calendar.selecting_end);
    cui_destroy(f.ui);
}

static void endpoint_and_capacity(bool styled) {
    fixture f;
    create(&f, styled);
    f.value = (cui_date_picker_value){.shape = CUI_DATE_PICKER_RANGE,
                                      .dates = {.start = {2024, 2, 15}, .end = {2024, 2, 16}}};
    f.options.preset_policy = CUI_DATE_PICKER_PRESET_MATCH_SHAPE_ENABLED;
    open_picker(&f);
    f.blocked = 18;
    activate(&f, 35, 0);
    frame(&f);
    CHECK(!f.result.action_activated && f.value.dates.start.year == 2024);
    f.options.preset_policy = CUI_DATE_PICKER_PRESET_SOURCE;
    activate(&f, 35, 0);
    frame(&f);
    CHECK(f.result.action_activated && f.value.dates.start.year == 2026);
    CHECK(f.state.calendar_value.dates.start.year == 2024);
    cui_destroy(f.ui);
    create(&f, styled);
    cui_date_picker_preset presets[CUI_DATE_PICKER_PRESET_LIMIT];
    char label[CUI_DATE_PICKER_LABEL_LIMIT];
    memset(label, 'L', sizeof label);
    for (unsigned i = 0; i < CUI_DATE_PICKER_PRESET_LIMIT; i++)
        presets[i] = (cui_date_picker_preset){.id = 40 + i,
                                              .label = {label, sizeof label},
                                              .value = {.dates = {.start = {2024, 3, (int)i + 1}}}};
    f.options.presets = presets;
    f.options.preset_count = CUI_DATE_PICKER_PRESET_LIMIT;
    open_picker(&f);
    for (unsigned i = 0; i < CUI_DATE_PICKER_PRESET_LIMIT; i++)
        CHECK(semantic(&f, 40 + i));
    if (styled) {
        cui_rect r = cui_bounds(f.ui, 47);
        CHECK(r.w > 0 && r.h > 0 && r.y + r.h <= 800);
    }
    cui_destroy(f.ui);
}
static void scope_targets(bool styled) {
    for (unsigned part = 0; part < 5; part++) {
        fixture f;
        create(&f, styled);
        f.options.delete_policy = CUI_DATE_PICKER_DELETE_SCOPE;
        f.options.calendar.presentation.month_toggle = true;
        open_picker(&f);
        cui_id id = 0;
        if (part == 0)
            id = cui_calendar_day_id(root(&f), (cui_date){2024, 2, 16});
        if (part == 1)
            id = cui_calendar_month_toggle_id(root(&f));
        if (part == 2)
            id = cui_calendar_navigation_id(root(&f), 5);
        if (part == 3) {
            f.state.picker.calendar.view = CUI_CALENDAR_YEARS;
            id = cui_calendar_year_id(root(&f), 2024);
        }
        if (part == 4) {
            f.state.picker.calendar.view = CUI_CALENDAR_MONTHS;
            id = cui_calendar_month_id(root(&f), 2);
        }
        frame(&f);
        CHECK(semantic(&f, id));
        cui_focus(f.ui, id);
        CHECK(
            cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE}));
        frame(&f);
        CHECK(f.result.action_activated && !cui_date_valid(f.value.dates.start) &&
              !f.state.picker.overlay.open);
        CHECK(f.immediate_focus == id);
        cui_destroy(f.ui);
    }
}

static void off_view_targets(bool styled) {
    fixture f;
    create(&f, styled);
    f.options.delete_policy = CUI_DATE_PICKER_DELETE_SCOPE;
    open_picker(&f);
    cui_id absent_ids[] = {cui_calendar_year_id(root(&f), 2050),
                           cui_calendar_day_id(root(&f), (cui_date){2030, 9, 12})};
    for (unsigned i = 0; i < 2; i++) {
        CHECK(!semantic(&f, absent_ids[i]));
        for (size_t n = 0; n < f.state.submitted_target_count; n++)
            CHECK(f.state.submitted_targets[n] != absent_ids[i]);
    }
    f.state.picker.calendar.view = CUI_CALENDAR_YEARS;
    frame(&f);
    cui_id year = cui_calendar_year_id(root(&f), 2024);
    CHECK(semantic(&f, year));
    cui_focus(f.ui, year);
    f.state.picker.calendar.view = CUI_CALENDAR_DAYS;
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE}));
    frame(&f);
    CHECK(!f.result.action_activated && f.state.picker.overlay.open);
    cui_id day = cui_calendar_day_id(root(&f), (cui_date){2024, 2, 16});
    CHECK(semantic(&f, day));
    cui_focus(f.ui, day);
    f.state.picker.calendar.month = (cui_date){2024, 3, 1};
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE}));
    frame(&f);
    CHECK(!f.result.action_activated && f.state.picker.overlay.open);
    /* A target newly valid in this view still needs an actual prior binding. */
    f.state.picker.calendar.view = CUI_CALENDAR_YEARS;
    f.state.picker.calendar.year_page = 2041;
    cui_id newly_valid = cui_calendar_year_id(root(&f), 2050);
    CHECK(!semantic(&f, newly_valid));
    for (size_t n = 0; n < f.state.submitted_target_count; n++)
        CHECK(f.state.submitted_targets[n] != newly_valid);
    frame(&f);
    CHECK(!f.result.action_activated && f.state.picker.overlay.open);
    cui_destroy(f.ui);
}
static void invalid(bool styled) {
    fixture f;
    create(&f, styled);
    frame(&f);
    for (unsigned i = 0; i < 10; i++) {
        cui_date_picker_actions_options save = f.options;
        cui_date_picker_preset p = f.presets[1];
        switch (i) {
        case 0:
            f.options.preset_count = 9;
            break;
        case 1:
            f.options.presets = NULL;
            break;
        case 2:
            f.presets[1].label = (cui_str){"\xc0\x80", 2};
            break;
        case 3:
            f.presets[1].id = 34;
            break;
        case 4:
            f.presets[1].value.dates.end = (cui_date){2024, 2, 30};
            break;
        case 5:
            f.options.picker.viewport.w = NAN;
            break;
        case 6:
            f.options.preset_policy = (cui_date_picker_preset_policy)99;
            break;
        case 7:
            f.options.cleanable = true;
            f.options.clear_id = 31;
            break;
        case 8:
            f.presets[1].id = cui_calendar_day_id(root(&f), (cui_date){2024, 2, 16});
            break;
        case 9:
            f.options.delete_policy = (cui_date_picker_delete_policy)-1;
            break;
        }
        cui_date_picker_actions_state state = f.state;
        f.predicate_calls = 0;
        frame(&f);
        CHECK(!f.result.valid && !f.result.changed && !memcmp(&state, &f.state, sizeof state));
        CHECK(!f.predicate_calls && !semantic(&f, 31));
        f.options = save;
        f.presets[1] = p;
    }
    cui_destroy(f.ui);
}
static void ordered(bool styled) {
    fixture f;
    create(&f, styled);
    open_picker(&f);
    activate(&f, 34, 0);
    activate(&f, 35, 0);
    frame(&f);
    CHECK(f.value.dates.start.year == 2025 && f.result.action_activated);
    CHECK(!cui_queue_accessibility_action(f.ui, 35, CUI_ACCESSIBILITY_ACTIVATE));
    cui_clear_error(f.ui);
    frame(&f);
    CHECK(!f.result.action_activated && f.value.dates.start.year == 2025);
    open_picker(&f);
    /* Actions run after Calendar even if queued earlier. */
    activate(&f, 35, 0);
    activate(&f, cui_calendar_day_id(root(&f), (cui_date){2025, 3, 21}), 0);
    frame(&f);
    CHECK(f.result.calendar.changed && f.result.action_activated &&
          f.value.dates.start.year == 2026);
    CHECK(f.state.calendar_value.shape == CUI_DATE_PICKER_RANGE);
    cui_destroy(f.ui);
}
static void external(bool styled) {
    fixture f;
    create(&f, styled);
    frame(&f);
    f.blocked = 20;
    f.value = f.presets[0].value;
    frame(&f);
    CHECK(f.state.calendar_value.dates.start.year == 2024);
    f.blocked = 0;
    frame(&f);
    CHECK(f.state.calendar_value.dates.start.year == 2024);
    f.value.dates.start.day = 21;
    frame(&f);
    CHECK(f.state.calendar_value.dates.start.year == 2025);
    cui_destroy(f.ui);
}

static void identity(bool styled) {
    fixture f;
    create(&f, styled);
    open_picker(&f);
    activate(&f, 34, 0);
    f.presets[0].id = 36;
    frame(&f);
    CHECK(!f.result.action_activated && f.state.picker.overlay.open);
    activate(&f, 36, 0);
    frame(&f);
    CHECK(f.result.action_activated);
    open_picker(&f);
    cui_focus(f.ui, 34);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    activate(&f, 35, 0);
    frame(&f);
    CHECK(!f.result.action_activated && !f.state.picker.overlay.open);
    open_picker(&f);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 999, .y = 799}));
    activate(&f, 35, 0);
    frame(&f);
    CHECK(!f.result.action_activated && !f.state.picker.overlay.open);
    /* Whole component omission ages records without a retained host pointer. */
    CHECK(cui_begin_frame(f.ui, 1000, 800, .016f));
    cui_end_frame(f.ui);
    frame(&f);
    CHECK(!f.result.action_activated);
    CHECK(!f.state.submitted_frame);
    cui_date_picker_actions_state prior = f.state;
    CHECK(!cui_base_date_picker_actions(f.ui, 31, 10, cui_string("Date"), &f.state, &f.value,
                                        f.options)
               .valid);
    CHECK(!memcmp(&prior, &f.state, sizeof prior));
    cui_destroy(f.ui);
}

static void submission_failure(void) {
    fixture f;
    create(&f, false);
    open_picker(&f);
    f.omit_presets = true;
    frame(&f);
    CHECK(!f.state.submitted_frame);
    CHECK(!cui_queue_accessibility_action(f.ui, 34, CUI_ACCESSIBILITY_ACTIVATE));
    cui_clear_error(f.ui);
    f.omit_presets = false;
    CHECK(cui_begin_frame(f.ui, 1000, 800, .016f));
    cui_date_picker_actions_result r = cui_base_date_picker_actions(
        f.ui, 31, 10, cui_string("Date"), &f.state, &f.value, f.options);
    CHECK(r.valid);
    cui_date_picker_actions_state once = f.state;
    CHECK(!cui_base_date_picker_actions(f.ui, 31, 10, cui_string("Date"), &f.state, &f.value,
                                        f.options)
               .valid);
    CHECK(!memcmp(&once, &f.state, sizeof once));
    CHECK(cui_overlay_begin(f.ui, &f.state.picker.overlay));
    cui_date_picker_actions_submit_presets(f.ui, &f.state, f.options);
    CHECK(f.state.submitted_frame == cui_frame_sequence(f.ui));
    cui_date_picker_value before = f.value;
    cui_date_picker_actions_submit_presets(f.ui, &f.state, f.options);
    CHECK(!f.state.submitted_frame && cui_last_error(f.ui) == CUI_ERROR_DUPLICATE_ID);
    CHECK(!memcmp(&before, &f.value, sizeof before));
    cui_overlay_end(f.ui);
    cui_end_frame(f.ui);
    cui_clear_error(f.ui);
    frame(&f);
    CHECK(!f.result.action_activated);
    cui_destroy(f.ui);
}
/* Compare semantic values, never C struct padding or borrowed string pointers.
 * Independent equivalent submissions need not initialize padding identically. */
/* Keep the field table flat: adjacent X-macro calls are not nested expressions. */
// clang-format off
#define DATE_DETAILS_SCALARS(X) \
    X(expanded) X(selected) X(readonly) X(multiline) X(required) X(invalid) \
    X(multiselectable) X(busy) X(scrollable) X(orientation) X(controls) X(labelled_by) \
    X(parent) X(focus_target) X(minimum) X(maximum) X(level) X(pos_in_set) X(set_size) \
    X(row_index) X(column_index) X(row_count) X(column_count) X(scroll_x) X(scroll_y) \
    X(scroll_max_x) X(scroll_max_y) X(scroll_axis_lock) X(scroll_trap_horizontal) \
    X(scroll_gesture_lock) X(focusable) X(pointer_activatable) X(handles_tab) X(cursor) \
    X(no_focus) X(gesture_pan) X(gesture_zoom) X(touch_drag_priority) X(touch_scroll_disabled) \
    X(no_pointer_focus) X(blocks_pointer_activation) X(secret) X(password) \
    X(text_input_configured) X(text_input_config.type) X(text_input_config.purpose) \
    X(release_key_activation)
// clang-format on
static bool details_equal(const cui_semantic_details *a, const cui_semantic_details *b) {
#define SAME(field)                                                                                \
    if (a->field != b->field)                                                                      \
        return false;
    DATE_DETAILS_SCALARS(SAME)
#undef SAME
    return a->description.length == b->description.length &&
           (!a->description.length ||
            !memcmp(a->description.data, b->description.data, a->description.length));
}
static void details_values(void) {
    cui_semantic_details a, b;
    memset(&a, 0xa5, sizeof a);
    memset(&b, 0x5a, sizeof b);
#define ZERO(field) a.field = b.field = 0;
    DATE_DETAILS_SCALARS(ZERO)
#undef ZERO
    char left[] = "Same description", right[] = "Same description";
    a.description = cui_string(left);
    b.description = cui_string(right);
    CHECK(details_equal(&a, &b));
#define DIFFERENT(field)                                                                           \
    b.field = 1;                                                                                   \
    CHECK(!details_equal(&a, &b));                                                                 \
    b.field = 0;
    DATE_DETAILS_SCALARS(DIFFERENT)
#undef DIFFERENT
    right[0] = 'X';
    CHECK(!details_equal(&a, &b));
    b.description.length--;
    CHECK(!details_equal(&a, &b));
}
#undef DATE_DETAILS_SCALARS
static void zero_pixels(void) {
    fixture a, b;
    create(&a, true);
    create(&b, true);
    a.options.preset_count = b.options.preset_count = 0;
    for (unsigned stage = 0; stage < 2; stage++) {
        if (stage) {
            activate(&a, 31, 0);
            activate(&b, 31, 0);
        }
        frame(&a);
        CHECK(cui_begin_frame(b.ui, 1000, 800, .016f));
        bool changed = cui_date_picker_ex(b.ui, 31, &b.theme, cui_string("Date"), &b.state.picker,
                                          &b.value.dates, b.options.picker, b.options.calendar);
        b.draw = cui_end_frame(b.ui);
        CHECK(cui_last_error(b.ui) == CUI_OK);
        CHECK(a.result.changed == changed && a.draw.count == b.draw.count);
        CHECK(!memcmp(&a.state.picker, &b.state.picker, sizeof a.state.picker));
        size_t na, nb;
        const cui_semantic *sa = cui_semantics(a.ui, &na), *sb = cui_semantics(b.ui, &nb);
        CHECK(na == nb);
        for (size_t i = 0; i < na; i++) {
            CHECK(sa[i].id == sb[i].id && sa[i].role == sb[i].role &&
                  sa[i].disabled == sb[i].disabled);
            CHECK(sa[i].name.length == sb[i].name.length &&
                  (!sa[i].name.length ||
                   !memcmp(sa[i].name.data, sb[i].name.data, sa[i].name.length)));
            CHECK(details_equal(&sa[i].details, &sb[i].details));
        }
        unsigned char *pixels[2] = {malloc(1000 * 800 * 4), malloc(1000 * 800 * 4)};
        CHECK(pixels[0] && pixels[1]);
        for (unsigned which = 0; which < 2; which++) {
            cui_draw_list original = which ? b.draw : a.draw;
            cui_draw_command commands[2048];
            cui_draw_list filtered = {.commands = commands, .width = 1000, .height = 800};
            for (size_t i = 0; i < original.count; i++)
                if (original.commands[i].type != CUI_DRAW_TEXT)
                    commands[filtered.count++] = original.commands[i];
            cui_software *sw = cui_software_create(1000, 800, NULL);
            CHECK(sw);
            CHECK(cui_software_render(sw, &filtered, (cui_color){0, 0, 0, 255}));
            unsigned w, h;
            const unsigned char *data = cui_software_pixels(sw, &w, &h);
            CHECK(w == 1000 && h == 800);
            memcpy(pixels[which], data, 1000 * 800 * 4);
            cui_software_destroy(sw);
        }
        CHECK(!memcmp(pixels[0], pixels[1], 1000 * 800 * 4));
        free(pixels[0]);
        free(pixels[1]);
    }
    cui_destroy(a.ui);
    cui_destroy(b.ui);
}

void example_date_actions(cui_context *, cui_date_picker_actions_state *, cui_date_picker_value *,
                          cui_rect);
static void guide_example(void) {
    cui_context *ui = cui_create((cui_config){.width = 1000, .height = 800});
    CHECK(ui);
    cui_date_picker_actions_state state;
    cui_date_picker_actions_init(&state, 702, (cui_date){2024, 2, 1});
    cui_date_picker_value value = {.dates = {.start = {2024, 2, 15}}};
    for (unsigned stage = 0; stage < 3; stage++) {
        if (stage)
            CHECK(cui_queue_accessibility_action(ui, stage == 1 ? 701 : 704,
                                                 CUI_ACCESSIBILITY_ACTIVATE));
        CHECK(cui_begin_frame(ui, 1000, 800, .016f));
        example_date_actions(ui, &state, &value, (cui_rect){0, 0, 1000, 800});
        if (stage == 1)
            CHECK(state.submitted_frame == cui_frame_sequence(ui));
        cui_end_frame(ui);
        CHECK(cui_last_error(ui) == CUI_OK);
    }
    CHECK(value.dates.start.month == 3 && !state.picker.overlay.open);
    cui_destroy(ui);
}
int main(void) {
    cui_date_picker_actions_init(NULL, 0, (cui_date){0});
    CHECK(!cui_base_date_picker_actions(NULL, 0, 0, (cui_str){0}, NULL, NULL,
                                        (cui_date_picker_actions_options){0})
               .valid);
    for (unsigned styled = 0; styled < 2; styled++) {
        for (unsigned kind = 0; kind < 3; kind++) {
            source_vectors(styled, kind);
            clear_delete(styled, kind);
        }
        endpoint_and_capacity(styled);
        scope_targets(styled);
        off_view_targets(styled);
        policies(styled);
        invalid(styled);
        ordered(styled);
        external(styled);
        identity(styled);
    }
    guide_example();
    details_values();
    zero_pixels();
    submission_failure();
    printf("PASS: %u DatePicker action assertions\n", checks);
    return 0;
}
