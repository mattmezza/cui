/* SPDX-License-Identifier: MIT */
#include "cui/cui_date_time_picker.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, #x);                                 \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
typedef struct {
    cui_context *ui;
    cui_date_time_picker_state s;
    cui_date_time_value v;
    cui_date_time_picker_options o;
    cui_date_time_preset p[2];
    cui_date_time_picker_result r;
    cui_id resolved_target;
    bool omit_time, omit_presets, fail_time, legacy;
    unsigned predicate_calls;
    int blocked;
} fixture;
static bool blocked(void *user, cui_date d) {
    fixture *f = user;
    f->predicate_calls++;
    return d.day == f->blocked;
}
static void create(fixture *f) {
    *f = (fixture){.v = {{2024, 2, 15}, {9, 30, 45}},
                   .o = {.picker = {.viewport = {0, 0, 1000, 800}},
                         .precision = CUI_DATE_TIME_SECOND,
                         .default_time = {8, 15, 37},
                         .clear_id = 33,
                         .clear_label = {"Clear", 5}}};
    f->ui = cui_create(
        (cui_config){.width = 1000, .height = 800, .max_elements = 1024, .max_commands = 2048});
    CHECK(f->ui);
    f->p[0] = (cui_date_time_preset){.id = 34, .label = {"Day", 3}, .date = {2025, 3, 20}};
    f->p[1] = (cui_date_time_preset){.id = 35,
                                     .label = {"Timed", 5},
                                     .date = {2026, 4, 18},
                                     .has_time = true,
                                     .time = {23, 59, 59}};
    f->o.presets = f->p;
    f->o.preset_count = 2;
    f->o.picker.calendar.is_disabled = blocked;
    f->o.picker.calendar.disabled_user = f;
    f->o.calendar.presentation.month_toggle = true;
    cui_date_time_picker_init(&f->s, 32, (cui_date){2024, 2, 1});
}
static void frame(fixture *f) {
    CHECK(cui_begin_frame(f->ui, 1000, 800, .016f));
    size_t event_count;
    (void)cui_frame_events(f->ui, &event_count);
    f->resolved_target = event_count ? cui_event_target(f->ui, event_count - 1) : 0;
    if (!f->legacy)
        f->r = cui_base_date_time_picker(f->ui, 31, 10, 51, cui_string("Date/time"), &f->s, &f->v,
                                         f->o);
    if (f->legacy) {
        f->r = (cui_date_time_picker_result){.valid = true};
        f->r.calendar =
            cui_base_date_picker_ex(f->ui, 31, 10, cui_string("Legacy"), &f->s.actions.picker,
                                    &f->s.actions.calendar_value.dates, f->o.picker, f->o.calendar);
        f->v.date = f->s.actions.calendar_value.dates.start;
    }
    if (f->r.valid) {
        cui_begin_box(f->ui, 31, (cui_box){.width = 100, .height = 30});
        cui_end_box(f->ui);
        if (f->o.cleanable && cui_date_valid(f->v.date)) {
            cui_begin_box(f->ui, 33, (cui_box){.width = 100, .height = 30});
            cui_end_box(f->ui);
        }
        if (f->s.actions.picker.overlay.open) {
            cui_rect bounds = {0, 80, 850, 650};
            cui_overlay_set_bounds(&f->s.actions.picker.overlay, bounds);
            CHECK(cui_overlay_begin(f->ui, &f->s.actions.picker.overlay));
            cui_begin_portal(f->ui, 32, (cui_box){0}, bounds, 100);
            cui_calendar_options o = f->o.picker.calendar;
            o.disabled |= f->o.picker.disabled;
            cui_calendar_submit_ex(f->ui, 10, &f->s.actions.picker.calendar,
                                   f->s.actions.calendar_value.dates, o,
                                   f->o.calendar.presentation);
            if (!f->omit_time)
                cui_date_time_picker_submit_time(f->ui, 51, &f->s, &f->v, f->o);
            if (f->fail_time) {
                cui_date_time_picker_submit_time(f->ui, 51, &f->s, &f->v, f->o);
                CHECK(cui_last_error(f->ui) == CUI_ERROR_DUPLICATE_ID &&
                      !f->s.time_submitted_frame);
                cui_clear_error(f->ui);
            }
            if (!f->omit_presets)
                cui_date_time_picker_submit_presets(f->ui, &f->s, f->o);
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
    (void)cui_end_frame(f->ui);
    CHECK(cui_last_error(f->ui) == CUI_OK);
}
static void activate(fixture *f, cui_id id) {
    bool queued = cui_queue_accessibility_action(f->ui, id, CUI_ACCESSIBILITY_ACTIVATE);
    if (!queued)
        fprintf(stderr, "Activation %llu open%d date%d-%d-%d error%d\n", (unsigned long long)id,
                f->s.actions.picker.overlay.open, f->v.date.year, f->v.date.month, f->v.date.day,
                cui_last_error(f->ui));
    CHECK(queued);
    frame(f);
}
static void open_picker(fixture *f) {
    frame(f);
    activate(f, 31);
    CHECK(f->s.actions.picker.overlay.open);
    frame(f);
}
static void key(fixture *f, cui_id target, cui_key k) {
    cui_focus(f->ui, target);
    cui_event event = {.type = CUI_EVENT_KEY_DOWN, .key = k};
    if (k >= CUI_KEY_0 && k <= CUI_KEY_9) {
        event.type = CUI_EVENT_TEXT;
        event.text[0] = (char)('0' + k - CUI_KEY_0);
    }
    CHECK(cui_queue_event(f->ui, event));
    frame(f);
}
static void days_and_time(void) {
    fixture f;
    create(&f);
    open_picker(&f);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 16}));
    CHECK(f.r.calendar.activated && f.r.source_change && f.r.value_changed &&
          f.s.actions.picker.overlay.open);
    CHECK(f.v.date.day == 16 && f.v.time.second == 45);
    key(&f, cui_time_segment_id(51, 1), CUI_KEY_UP);
    CHECK(f.r.time_edited && f.r.source_change && f.v.time.minute == 31 && f.v.date.day == 16);
    CHECK(f.s.actions.picker.overlay.open);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 16}));
    CHECK(f.r.confirmed && !f.r.source_change && !f.r.value_changed &&
          !f.s.actions.picker.overlay.open);
    CHECK(cui_focused(f.ui) == 31);
    cui_destroy(f.ui);
}
static void queue_day(fixture *f, int day) {
    CHECK(cui_queue_accessibility_action(f->ui, cui_calendar_day_id(10, (cui_date){2024, 2, day}),
                                         CUI_ACCESSIBILITY_ACTIVATE));
}
static void ordered_days(void) {
    const int sequences[][3] = {{16, 15, 0}, {16, 17, 0}, {16, 16, 17}, {15, 16, 0}};
    const int final[] = {15, 17, 16, 15};
    for (unsigned i = 0; i < 4; i++) {
        fixture f;
        create(&f);
        open_picker(&f);
        cui_focus(f.ui, cui_time_segment_id(51, 1));
        for (unsigned j = 0; j < 3 && sequences[i][j]; j++)
            queue_day(&f, sequences[i][j]);
        if (i >= 2)
            CHECK(
                cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
        frame(&f);
        CHECK(f.v.date.day == final[i]);
        CHECK(f.r.confirmed == (i >= 2));
        CHECK(f.s.actions.picker.overlay.open == (i < 2));
        CHECK(f.r.source_change == (i != 3));
        CHECK(!f.r.time_edited && f.v.time.minute == 30);
        CHECK(f.r.value_changed == (i == 1 || i == 2));
        cui_destroy(f.ui);
    }
    fixture f;
    create(&f);
    open_picker(&f);
    f.blocked = 17; /* Prior enabled semantic, current matcher rejection. */
    queue_day(&f, 16);
    queue_day(&f, 17);
    queue_day(&f, 15);
    frame(&f);
    CHECK(f.v.date.day == 15 && f.r.source_change && !f.r.value_changed && !f.r.confirmed);
    CHECK(f.s.actions.picker.overlay.open);
    f.o.picker.calendar.disabled = true;
    queue_day(&f, 16);
    queue_day(&f, 15);
    frame(&f);
    CHECK(!f.r.calendar.activated && !f.r.source_change && !f.r.confirmed);
    cui_destroy(f.ui);

    create(&f);
    open_picker(&f);
    CHECK(cui_queue_accessibility_action(f.ui, cui_calendar_navigation_id(10, 3),
                                         CUI_ACCESSIBILITY_ACTIVATE));
    queue_day(&f, 16);
    queue_day(&f, 15);
    frame(&f);
    CHECK(f.r.calendar.navigated && f.r.source_change && !f.r.confirmed && f.v.date.day == 15);
    cui_destroy(f.ui);

    create(&f);
    open_picker(&f);
    f.v.date = (cui_date){2025, 3, 20};
    f.blocked = 20;
    frame(&f); /* External outer accepted, child rejects. */
    CHECK(f.s.actions.calendar_value.dates.start.year == 2024);
    activate(&f, 31);
    frame(&f);
    queue_day(&f, 16);
    queue_day(&f, 15);
    frame(&f);
    CHECK(f.v.date.year == 2024 && f.v.date.day == 15 && f.r.source_change && !f.r.confirmed);
    cui_destroy(f.ui);

    /* Legacy NULL visitor processes all accepted events, closes once on any activation. */
    create(&f);
    f.legacy = true;
    f.s.actions.calendar_value.dates.start = f.v.date;
    open_picker(&f);
    queue_day(&f, 15);
    queue_day(&f, 16);
    queue_day(&f, 17);
    frame(&f);
    CHECK(f.r.calendar.activated && f.r.calendar.changed && f.v.date.day == 17);
    CHECK(!f.s.actions.picker.overlay.open);
    cui_destroy(f.ui);
}
static void drafts_sync(void) {
    fixture f;
    create(&f);
    open_picker(&f);
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_1);
    CHECK(f.s.time_field.digit_count == 1 && f.v.time.hour == 1);
    frame(&f);
    CHECK(f.s.time_field.digit_count == 1);
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_2);
    CHECK(f.v.time.hour == 12 && !f.s.time_field.digit_count);
    key(&f, cui_time_segment_id(51, 1), CUI_KEY_4);
    CHECK(f.s.time_field.digit_count == 1);
    key(&f, cui_time_segment_id(51, 1), CUI_KEY_ESCAPE);
    CHECK(!f.s.actions.picker.overlay.open && f.s.time_field.digit_count == 1);
    activate(&f, 31);
    frame(&f);
    CHECK(f.s.time_field.digit_count == 1);
    cui_date_time_value same = f.v;
    CHECK(cui_date_time_picker_assign(&f.s, &f.v, same));
    frame(&f);
    CHECK(!f.s.actions.picker.overlay.open && !f.s.time_field.digit_count && !f.r.source_change);
    CHECK(!f.s.assignment_pending);
    f.v = (cui_date_time_value){{2028, 2, 29}, {22, 59, 58}};
    frame(&f);
    CHECK(f.s.actions.calendar_value.dates.start.year == 2028 && !f.r.source_change);
    f.o.precision = CUI_DATE_TIME_MINUTE;
    frame(&f);
    CHECK(!f.v.time.second && f.r.value_changed && !f.r.source_change);
    f.o.default_time = (cui_time){7, 12, 41};
    frame(&f);
    CHECK(f.v.time.hour == 7 && f.v.time.minute == 12 && !f.v.time.second);
    f.o.twelve_hour = true;
    frame(&f);
    CHECK(f.v.time.hour == 7);
    f.o.default_time = (cui_time){5, 0, 0};
    f.v.time = (cui_time){20, 22, 0};
    frame(&f);
    CHECK(f.v.time.hour == 20 && f.v.time.minute == 22 && !f.r.source_change);
    activate(&f, 31);
    frame(&f);
    key(&f, cui_time_segment_id(51, 3), CUI_KEY_UP);
    CHECK(f.v.time.hour == 8 && f.r.source_change && f.r.time_edited);
    cui_destroy(f.ui);
}
static void presets_clear(void) {
    fixture f;
    create(&f);
    f.o.cleanable = true;
    f.o.delete_policy = CUI_DATE_PICKER_DELETE_SCOPE;
    open_picker(&f);
    activate(&f, 34);
    CHECK(f.v.date.year == 2025 && f.v.time.hour == 9 && f.v.time.second == 45);
    CHECK(f.r.source_change && f.r.action_activated && !f.s.actions.picker.overlay.open &&
          cui_focused(f.ui) == 31);
    activate(&f, 31);
    frame(&f);
    activate(&f, 35);
    CHECK(f.v.time.hour == 23 && f.v.time.second == 59 && f.v.date.year == 2026);
    cui_focus(f.ui, 33);
    activate(&f, 33);
    CHECK(!f.v.date.year && f.v.time.hour == 8 && f.v.time.second == 37 && f.r.source_change);
    CHECK(cui_focused(f.ui) != 31);
    key(&f, 31, CUI_KEY_DELETE);
    CHECK(f.r.source_change && !f.r.value_changed && f.r.action_activated);
    activate(&f, 31);
    frame(&f);
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_UP);
    CHECK(!f.v.date.year && f.v.time.hour == 9 && f.r.time_edited && f.r.value_changed &&
          !f.r.source_change);
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_DELETE);
    CHECK(f.r.action_activated && !f.s.actions.picker.overlay.open && f.v.time.hour == 8);
    cui_destroy(f.ui);
}
static void repeated_and_phase_order(void) {
    fixture f;
    create(&f);
    f.o.precision = CUI_DATE_TIME_MINUTE;
    open_picker(&f);
    activate(&f, 35);
    CHECK(f.v.time.hour == 23 && f.v.time.minute == 59 && !f.v.time.second);
    activate(&f, 31);
    frame(&f);
    activate(&f, 35);
    CHECK(f.r.source_change && f.r.action_activated && !f.r.value_changed);
    activate(&f, 31);
    frame(&f);
    cui_focus(f.ui, cui_time_segment_id(51, 1));
    CHECK(cui_queue_accessibility_action(f.ui, 35, CUI_ACCESSIBILITY_ACTIVATE));
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    frame(&f);
    CHECK(f.r.time_edited && f.r.action_activated && f.r.source_change && !f.r.value_changed);
    CHECK(f.v.time.minute == 59 && !f.s.actions.picker.overlay.open);
    cui_destroy(f.ui);
}
static void failed_time_binding(void) {
    fixture f;
    create(&f);
    open_picker(&f);
    f.fail_time = true;
    frame(&f);
    f.fail_time = false;
    CHECK(!f.s.time_submitted_frame);
    /* Core still has the genuine first successful textbox binding this frame;
       the failed second submission must revoke the host's admission stamp. */
    unsigned hour = f.v.time.hour;
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_UP);
    CHECK(f.v.time.hour == hour && !f.r.time_edited);
    CHECK(f.resolved_target == cui_time_segment_id(51, 0));
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_UP);
    CHECK(f.v.time.hour == hour + 1 && f.r.time_edited);
    cui_destroy(f.ui);
}
static void divergence(void) {
    fixture f;
    create(&f);
    open_picker(&f);
    f.blocked = 20;
    activate(&f, 34);
    CHECK(f.v.date.year == 2025 && f.s.actions.calendar_value.dates.start.year == 2024);
    f.blocked = 0;
    activate(&f, 31);
    frame(&f);
    CHECK(f.s.actions.calendar_value.dates.start.year == 2024);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 15}));
    CHECK(f.v.date.year == 2024 && f.r.source_change && !f.r.confirmed &&
          f.s.actions.picker.overlay.open);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 15}));
    CHECK(f.r.confirmed && !f.s.actions.picker.overlay.open);
    f.o.preset_policy = CUI_DATE_PICKER_PRESET_MATCH_SHAPE_ENABLED;
    f.blocked = 20;
    activate(&f, 31);
    frame(&f);
    activate(&f, 34);
    CHECK(!f.r.action_activated && f.v.date.year == 2024 && f.s.actions.picker.overlay.open);
    cui_destroy(f.ui);
}
static void invalid(void) {
    fixture f;
    create(&f);
    frame(&f);
    for (unsigned i = 0; i < 15; i++) {
        cui_date_time_picker_options options = f.o;
        cui_date_time_value value = f.v;
        cui_date_time_preset preset = f.p[0];
        switch (i) {
        case 0:
            f.o.precision = (cui_date_time_precision)9;
            break;
        case 1:
            f.o.default_time.hour = 24;
            break;
        case 2:
            f.o.picker.calendar.range = true;
            break;
        case 3:
            f.o.picker.viewport.w = NAN;
            break;
        case 4:
            f.o.picker.calendar.minimum = (cui_date){2024, 2, 30};
            break;
        case 5:
            f.o.picker.calendar.minimum = (cui_date){2025, 1, 1};
            f.o.picker.calendar.maximum = (cui_date){2024, 1, 1};
            break;
        case 6:
            f.o.preset_count = 9;
            break;
        case 7:
            f.o.presets = NULL;
            break;
        case 8:
            f.p[0].id = cui_time_segment_id(51, 3);
            break;
        case 9:
            f.p[0].label = (cui_str){"\xc0\x80", 2};
            break;
        case 10:
            f.p[0].has_time = true;
            f.p[0].time.hour = 99;
            break;
        case 11:
            f.v.date = (cui_date){2024, 0, 0};
            break;
        case 12:
            f.v.time.minute = 60;
            break;
        case 13:
            f.o.time_label = (cui_str){NULL, 1};
            break;
        case 14:
            f.o.calendar.behavior.recenter = (cui_calendar_recenter_policy)99;
            break;
        }
        cui_date_time_picker_state before = f.s;
        cui_date_time_value submitted = f.v;
        unsigned calls = f.predicate_calls;
        frame(&f);
        CHECK(!f.r.valid && !memcmp(&before, &f.s, sizeof before));
        CHECK(!memcmp(&submitted, &f.v, sizeof submitted) && calls == f.predicate_calls);
        f.o = options;
        f.v = value;
        f.p[0] = preset;
    }
    cui_date_time_picker_state zero = {0};
    CHECK(!cui_date_time_picker_assign(&zero, &f.v, f.v));
    CHECK(!cui_date_time_picker_assign(NULL, &f.v, f.v));
    CHECK(!cui_date_time_picker_assign(&f.s, NULL, f.v));
    cui_date_time_value bad = f.v;
    bad.time.second = 60;
    CHECK(!cui_date_time_picker_assign(&f.s, &f.v, bad));
    CHECK(!f.s.assignment_pending);
    CHECK(cui_date_time_picker_assign(&f.s, &f.v, f.v));
    f.o.picker.viewport.w = 0;
    frame(&f);
    CHECK(!f.r.valid && f.s.assignment_pending);
    f.o.picker.viewport.w = 1000;
    frame(&f);
    CHECK(f.r.valid && !f.s.assignment_pending && !f.r.source_change);
    CHECK(cui_begin_frame(f.ui, 1000, 800, .016f));
    CHECK(
        !cui_base_date_time_picker(f.ui, 31, 10, 51, cui_string("Empty"), &zero, &f.v, f.o).valid);
    CHECK(!cui_base_date_time_picker(f.ui, 31, 10, 51, cui_string("Empty"), NULL, &f.v, f.o).valid);
    (void)cui_end_frame(f.ui);
    cui_date_time_picker_init(NULL, 1, (cui_date){2024, 1, 1});
    cui_destroy(f.ui);
}
static void stale_and_disabled(void) {
    fixture f;
    create(&f);
    open_picker(&f);
    f.omit_time = true;
    frame(&f);
    CHECK(!f.s.time_submitted_frame);
    unsigned hour = f.v.time.hour;
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_UP);
    CHECK(f.v.time.hour == hour && !f.r.time_edited);
    f.omit_time = false;
    frame(&f);
    f.omit_presets = true;
    frame(&f);
    CHECK(!f.s.actions.submitted_frame);
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_UP);
    CHECK(f.v.time.hour == hour && !f.r.time_edited);
    f.omit_presets = false;
    frame(&f);
    f.o.picker.disabled = true;
    frame(&f);
    CHECK(!f.s.actions.picker.overlay.open);
    key(&f, 31, CUI_KEY_DELETE);
    CHECK(!f.r.action_activated);
    f.o.picker.disabled = false;
    f.o.picker.calendar.disabled = true;
    frame(&f);
    activate(&f, 31);
    frame(&f);
    key(&f, cui_time_segment_id(51, 0), CUI_KEY_UP);
    CHECK(f.r.time_edited && f.v.time.hour == hour + 1);
    CHECK(cui_begin_frame(f.ui, 1000, 800, .016f));
    cui_date_time_picker_result r =
        cui_base_date_time_picker(f.ui, 31, 10, 51, cui_string("Date/time"), &f.s, &f.v, f.o);
    CHECK(r.valid);
    cui_date_time_picker_state once = f.s;
    CHECK(!cui_base_date_time_picker(f.ui, 31, 10, 51, cui_string("Date/time"), &f.s, &f.v, f.o)
               .valid);
    CHECK(!memcmp(&once, &f.s, sizeof once));
    (void)cui_end_frame(f.ui);
    cui_destroy(f.ui);
}
int main(void) {
    days_and_time();
    ordered_days();
    drafts_sync();
    presets_clear();
    repeated_and_phase_order();
    failed_time_binding();
    divergence();
    invalid();
    stale_and_disabled();
    printf("PASS: %u DateTime Base assertions\n", checks);
    return 0;
}
