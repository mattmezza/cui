/* SPDX-License-Identifier: MIT */
#include "cui/cui_calendar_behavior.h"
#include "cui/cui_software.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
typedef struct {
    cui_context *ui;
    cui_theme theme;
    cui_calendar_state state;
    cui_date_selection selection;
    cui_calendar_options options;
    cui_calendar_presentation_options presentation;
    cui_calendar_result result;
    cui_calendar_behavior_options behavior;
} fixture;
static fixture create(bool dark) {
    fixture f = {.ui = cui_create((cui_config){.width = 800, .height = 500}),
                 .theme = dark ? cui_theme_dark() : cui_theme_light(),
                 .selection = {.start = {2024, 2, 15}},
                 .options = {.locale = cui_date_locale_builtin(CUI_DATE_LOCALE_IT)},
                 .presentation = {.month_toggle = true}};
    CHECK(f.ui);
    cui_set_reduced_motion(f.ui, true);
    cui_calendar_init(&f.state, f.selection.start);
    return f;
}
static cui_draw_list frame(fixture *f, unsigned mode) {
    CHECK(cui_begin_frame(f->ui, 800, 500, .016f));
    if (mode == 0)
        f->result = cui_calendar_ex(f->ui, 10, &f->theme, &f->state, &f->selection, f->options,
                                    f->presentation);
    else if (mode == 1)
        f->result = cui_calendar_with_behavior(f->ui, 10, &f->theme, &f->state, &f->selection,
                                               f->options, f->presentation, f->behavior);
    else {
        if (mode == 2)
            f->result = cui_base_calendar_with_behavior(f->ui, 10, &f->state, &f->selection,
                                                        f->options, f->presentation, f->behavior);
        else {
            f->result = cui_calendar_reduce_with_behavior(f->ui, 10, &f->state, &f->selection,
                                                          f->options, f->presentation, f->behavior);
            cui_calendar_submit_ex(f->ui, 10, &f->state, f->selection, f->options, f->presentation);
        }
        size_t n;
        const cui_semantic *sem = cui_semantics(f->ui, &n);
        for (size_t i = 0; i < n; i++) {
            cui_begin_box(f->ui, sem[i].id, (cui_box){.width = 40, .height = 3});
            cui_end_box(f->ui);
        }
    }
    cui_draw_list draw = cui_end_frame(f->ui);
    CHECK(cui_last_error(f->ui) == CUI_OK);
    return draw;
}
static const cui_semantic *semantic(fixture *f, cui_id id) {
    size_t n;
    const cui_semantic *sem = cui_semantics(f->ui, &n);
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == id)
            return &sem[i];
    return NULL;
}
static void key(fixture *f, cui_key code) {
    CHECK(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code}));
}
static void activate(fixture *f, cui_id id, unsigned kind) {
    if (kind == 0) {
        cui_rect b = cui_bounds(f->ui, id);
        CHECK(b.w > 0 && b.h > 0);
        CHECK(cui_queue_event(
            f->ui,
            (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w / 2, .y = b.y + b.h / 2}));
        CHECK(cui_queue_event(
            f->ui,
            (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + b.w / 2, .y = b.y + b.h / 2}));
    } else if (kind == 1) {
        cui_focus(f->ui, id);
        key(f, CUI_KEY_ENTER);
    } else if (kind == 3) {
        cui_focus(f->ui, id);
        key(f, CUI_KEY_SPACE);
        CHECK(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    } else
        CHECK(cui_queue_accessibility_action(f->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
}
static bool color_equal(cui_color a, cui_color b) {
    return !memcmp(&a, &b, sizeof a);
}
static void pixels(cui_draw_list draw, unsigned char *out) {
    cui_draw_command commands[1024];
    CHECK(draw.count <= 1024);
    cui_draw_list copy = {.commands = commands, .width = draw.width, .height = draw.height};
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[copy.count++] = draw.commands[i];
    cui_software *sw = cui_software_create(800, 500, NULL);
    CHECK(sw && cui_software_render(sw, &copy, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *data = cui_software_pixels(sw, &w, &h);
    CHECK(w == 800 && h == 500);
    memcpy(out, data, 800 * 500 * 4);
    cui_software_destroy(sw);
}
static void pending(fixture *f) {
    f->options.range = true;
    f->selection = (cui_date_selection){.start = {2024, 2, 20}};
    f->state.selecting_end = true;
}
static void range_transitions(void) {
    for (unsigned mode = 1; mode <= 3; mode++)
        for (unsigned kind = 0; kind < 4; kind++)
            for (unsigned restart = 0; restart < 2; restart++) {
                fixture f = create(false);
                pending(&f);
                f.behavior.range_transition = (cui_calendar_range_transition)restart;
                frame(&f, mode);
                activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 10}), kind);
                frame(&f, mode);
                CHECK(f.result.activated && f.result.changed && f.selection.start.day == 10);
                CHECK(f.state.selecting_end == (restart != 0));
                CHECK(restart ? !cui_date_valid(f.selection.end) : f.selection.end.day == 20);
                if (restart) {
                    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 15}), kind);
                    frame(&f, mode);
                    CHECK(f.selection.start.day == 10 && f.selection.end.day == 15 &&
                          !f.state.selecting_end);
                    CHECK(semantic(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 12}))
                              ->details.selected);
                }
                cui_destroy(f.ui);
            }
    fixture f = create(false);
    pending(&f);
    f.behavior.range_transition = CUI_CALENDAR_RANGE_RESTART_EARLIER;
    frame(&f, 1);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 10}), 2);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 15}), 2);
    frame(&f, 1);
    CHECK(f.selection.start.day == 10 && f.selection.end.day == 15 && !f.state.selecting_end);
    cui_destroy(f.ui);
    /* Shape inference is an opt-in, not a passive rewrite or caller flag requirement. */
    for (unsigned complete = 0; complete < 2; complete++) {
        f = create(false);
        pending(&f);
        f.state.selecting_end = complete != 0;
        if (complete)
            f.selection.end = (cui_date){2024, 2, 22};
        f.behavior.range_transition = CUI_CALENDAR_RANGE_RESTART_EARLIER;
        frame(&f, 1);
        CHECK(f.state.selecting_end == (complete != 0));
        activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 21}), 2);
        frame(&f, 1);
        CHECK(f.state.selecting_end == (complete != 0));
        CHECK(complete ? f.selection.start.day == 21 && !cui_date_valid(f.selection.end)
                       : f.selection.start.day == 20 && f.selection.end.day == 21);
        cui_destroy(f.ui);
    }
}
typedef struct {
    unsigned day, month;
    int year;
} blocked_dates;
static bool disabled(void *user, cui_date date) {
    const blocked_dates *b = user;
    return (b->day && (unsigned)date.day == b->day) ||
           (b->month && (unsigned)date.month == b->month) || (b->year && date.year == b->year);
}
static void validation(void) {
    for (unsigned transition = 0; transition < 2; transition++)
        for (unsigned validate = 0; validate < 2; validate++)
            for (unsigned kind = 0; kind < 4; kind++) {
                fixture f = create(false);
                pending(&f);
                blocked_dates block = {0};
                f.options.is_disabled = disabled;
                f.options.disabled_user = &block;
                f.behavior.range_transition = (cui_calendar_range_transition)transition;
                f.behavior.range_validation = (cui_calendar_range_validation)validate;
                frame(&f, 1);
                activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 22}), kind);
                block.day = 20;
                frame(&f, 1);
                CHECK(f.result.activated == !validate && f.result.changed == !validate);
                CHECK(validate ? f.state.selecting_end && !cui_date_valid(f.selection.end)
                               : !f.state.selecting_end && f.selection.end.day == 22);
                if (validate) {
                    block.day = 21;
                    frame(&f, 1); /* Disabled interior is permitted. */
                    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 22}), kind);
                    frame(&f, 1);
                    CHECK(f.result.activated && f.selection.end.day == 22);
                }
                cui_destroy(f.ui);
            }
}
static void year_destination(void) {
    for (unsigned mode = 1; mode <= 3; mode++)
        for (unsigned kind = 0; kind < 4; kind++)
            for (unsigned month_disabled = 0; month_disabled < 2; month_disabled++) {
                fixture f = create(false);
                f.options.months = 3;
                blocked_dates block = {0};
                f.options.is_disabled = disabled;
                f.options.disabled_user = &block;
                cui_calendar_init(&f.state, (cui_date){2024, 2, 29});
                f.behavior.year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS;
                frame(&f, mode);
                activate(&f, cui_calendar_navigation_id(10, 5), kind);
                frame(&f, mode);
                activate(&f, cui_calendar_year_id(10, 2025), kind);
                if (month_disabled)
                    block.month = 2;
                frame(&f, mode);
                CHECK(f.state.view == CUI_CALENDAR_DAYS && f.state.month.year == 2025 &&
                      f.state.month.month == 2);
                CHECK(f.state.cursor.year == 2025 && f.state.cursor.month == 2 &&
                      f.state.cursor.day == 28);
                CHECK(f.selection.start.year == 2024 && !f.result.changed && !f.result.activated &&
                      f.result.navigated);
                CHECK(cui_focused(f.ui) ==
                      (month_disabled ? cui_calendar_navigation_id(10, 5)
                                      : cui_calendar_day_id(10, (cui_date){2025, 2, 28})));
                cui_destroy(f.ui);
            }
    const int previous[] = {2, 9998}, next[] = {1, 9999};
    for (unsigned edge = 0; edge < 2; edge++) {
        fixture f = create(false);
        cui_calendar_init(&f.state, (cui_date){previous[edge], 2, 28});
        f.behavior.year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS;
        frame(&f, 1);
        activate(&f, cui_calendar_navigation_id(10, 5), 2);
        frame(&f, 1);
        activate(&f, cui_calendar_year_id(10, (unsigned)next[edge]), 2);
        frame(&f, 1);
        CHECK(f.state.month.year == next[edge] && f.state.view == CUI_CALENDAR_DAYS &&
              cui_date_valid(f.state.cursor));
        cui_destroy(f.ui);
    }
    fixture f = create(false);
    blocked_dates block = {0};
    f.options.is_disabled = disabled;
    f.options.disabled_user = &block;
    f.behavior.year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS;
    frame(&f, 1);
    activate(&f, cui_calendar_navigation_id(10, 5), 2);
    frame(&f, 1);
    activate(&f, cui_calendar_year_id(10, 2025), 2);
    block.year = 2025;
    frame(&f, 1);
    CHECK(f.state.view == CUI_CALENDAR_YEARS && f.state.month.year == 2024 && !f.result.navigated);
    cui_destroy(f.ui);
}
static void recenter(void) {
    for (unsigned policy = 0; policy < 3; policy++)
        for (unsigned kind = 0; kind < 4; kind++) {
            fixture f = create(false);
            pending(&f);
            f.options.months = 2;
            f.presentation.neighboring_days = true;
            f.behavior.recenter = (cui_calendar_recenter_policy)policy;
            frame(&f, 1);
            activate(&f, cui_calendar_day_id(10, (cui_date){2024, 3, 10}), kind);
            frame(&f, 1);
            CHECK(f.selection.end.month == 3 && f.result.activated && f.result.changed);
            CHECK(f.state.month.month == (policy == 1 ? 3 : 2));
            cui_date expected = policy == 2 ? (cui_date){2024, 2, 20} : (cui_date){2024, 3, 10};
            CHECK(!cui_date_compare(f.state.cursor, expected));
            CHECK(cui_focused(f.ui) == cui_calendar_day_id(10, expected));
            cui_destroy(f.ui);
        }
    for (unsigned all = 0; all < 2; all++) {
        fixture f = create(false);
        pending(&f);
        f.presentation.neighboring_days = true;
        blocked_dates block = {0};
        f.options.is_disabled = disabled;
        f.options.disabled_user = &block;
        f.behavior.recenter = CUI_CALENDAR_RECENTER_SELECTION_START;
        frame(&f, 1);
        cui_calendar_cell_info neighbor = {0};
        for (unsigned cell = 0; cell < 42; cell++) {
            cui_calendar_cell_info info;
            if (cui_calendar_cell_resolve(10, f.state.month, 1, 0, cell, &info) &&
                !cui_date_compare(info.date, (cui_date){2024, 3, 10}))
                neighbor = info;
        }
        CHECK(neighbor.id);
        activate(&f, neighbor.id, 2);
        if (all)
            block.month = 2;
        else
            block.day = 20;
        frame(&f, 1);
        CHECK(f.state.month.month == 2 && f.selection.end.month == 3);
        CHECK(cui_focused(f.ui) == (all ? cui_calendar_navigation_id(10, 5)
                                        : cui_calendar_day_id(10, (cui_date){2024, 2, 1})));
        CHECK(f.state.cursor.month == 2 && f.state.cursor.day == (all ? 20 : 1));
        cui_destroy(f.ui);
    }
}
static void equivalence_and_invalid(void) {
    unsigned char *a = malloc(800 * 500 * 4), *b = malloc(800 * 500 * 4);
    CHECK(a && b);
    for (unsigned dark = 0; dark < 2; dark++)
        for (unsigned view = 0; view < 3; view++)
            for (unsigned months = 1; months <= 3; months++) {
                fixture old = create(dark != 0), new = create(dark != 0);
                old.state.view = new.state.view = (cui_calendar_view)view;
                old.options.months = new.options.months = months;
                old.presentation.neighboring_days = new.presentation.neighboring_days = true;
                cui_draw_list x = frame(&old, 0), y = frame(&new, 1);
                CHECK(x.count == y.count);
                pixels(x, a);
                pixels(y, b);
                CHECK(!memcmp(a, b, 800 * 500 * 4));
                for (size_t i = 0; i < x.count; i++) {
                    CHECK(x.commands[i].type == y.commands[i].type &&
                          color_equal(x.commands[i].color, y.commands[i].color));
                    CHECK(x.commands[i].text.length == y.commands[i].text.length);
                    CHECK(!x.commands[i].text.length ||
                          !memcmp(x.commands[i].text.data, y.commands[i].text.data,
                                  x.commands[i].text.length));
                }
                new.behavior = (cui_calendar_behavior_options){
                    (cui_calendar_range_transition)-1, (cui_calendar_year_destination)INT_MAX,
                    (cui_calendar_recenter_policy)-1, (cui_calendar_range_validation)INT_MAX};
                y = frame(&new, 1);
                pixels(y, b);
                CHECK(!memcmp(a, b, 800 * 500 * 4));
                cui_destroy(old.ui);
                cui_destroy(new.ui);
            }
    free(a);
    free(b);
    fixture f = create(false);
    pending(&f);
    f.behavior = (cui_calendar_behavior_options){
        CUI_CALENDAR_RANGE_RESTART_EARLIER, (cui_calendar_year_destination)-1,
        (cui_calendar_recenter_policy)INT_MAX, (cui_calendar_range_validation)-1};
    frame(&f, 1);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 10}), 2);
    frame(&f, 1);
    CHECK(f.state.selecting_end && f.selection.start.day == 10 && !cui_date_valid(f.selection.end));
    cui_destroy(f.ui);
}
static void combinations_and_limits(void) {
    for (unsigned transition = 0; transition < 2; transition++)
        for (unsigned validate = 0; validate < 2; validate++)
            for (unsigned year = 0; year < 2; year++)
                for (unsigned center = 0; center < 3; center++) {
                    fixture f = create(false);
                    pending(&f);
                    f.behavior = (cui_calendar_behavior_options){
                        (cui_calendar_range_transition)transition,
                        (cui_calendar_year_destination)year, (cui_calendar_recenter_policy)center,
                        (cui_calendar_range_validation)validate};
                    frame(&f, 1);
                    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 10}), 2);
                    frame(&f, 1);
                    CHECK(f.selection.start.day == 10 &&
                          f.state.selecting_end == (transition != 0));
                    CHECK(transition ? !cui_date_valid(f.selection.end)
                                     : f.selection.end.day == 20);
                    cui_date_selection selection = f.selection;
                    activate(&f, cui_calendar_navigation_id(10, 5), 2);
                    frame(&f, 1);
                    activate(&f, cui_calendar_year_id(10, 2025), 2);
                    frame(&f, 1);
                    CHECK(f.state.view == (year ? CUI_CALENDAR_DAYS : CUI_CALENDAR_MONTHS));
                    CHECK(!cui_date_compare(selection.start, f.selection.start) &&
                          !cui_date_compare(selection.end, f.selection.end) && !f.result.changed);
                    cui_destroy(f.ui);
                }
    fixture f = create(false);
    blocked_dates block = {.day = 20};
    f.options.is_disabled = disabled;
    f.options.disabled_user = &block;
    f.behavior = (cui_calendar_behavior_options){
        CUI_CALENDAR_RANGE_RESTART_EARLIER, CUI_CALENDAR_YEAR_OPEN_DAYS,
        CUI_CALENDAR_RECENTER_SELECTION_START, CUI_CALENDAR_VALIDATE_RANGE_ENDPOINTS};
    frame(&f, 1);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 21}), 2);
    frame(&f, 1);
    CHECK(f.result.activated && f.selection.start.day == 21 && !cui_date_valid(f.selection.end) &&
          !f.state.selecting_end);
    f.options.disabled = true;
    frame(&f, 1);
    CHECK(!f.result.activated && !f.result.changed);
    CHECK(cui_begin_frame(f.ui, 800, 500, .016f));
    cui_end_frame(f.ui);
    frame(&f, 1);
    CHECK(f.selection.start.day == 21);
    CHECK(!cui_calendar_reduce_with_behavior(NULL, 10, &f.state, &f.selection, f.options,
                                             f.presentation, f.behavior)
               .changed);
    cui_destroy(f.ui);
    /* Invalid fields fall back independently; valid direct-year behavior survives. */
    f = create(false);
    f.behavior = (cui_calendar_behavior_options){
        (cui_calendar_range_transition)-1, CUI_CALENDAR_YEAR_OPEN_DAYS,
        (cui_calendar_recenter_policy)INT_MAX, (cui_calendar_range_validation)-1};
    frame(&f, 1);
    activate(&f, cui_calendar_navigation_id(10, 5), 2);
    frame(&f, 1);
    activate(&f, cui_calendar_year_id(10, 2025), 2);
    frame(&f, 1);
    CHECK(f.state.view == CUI_CALENDAR_DAYS);
    cui_destroy(f.ui);
}
static void range_pixels(void) {
    unsigned char *image = malloc(800 * 500 * 4);
    CHECK(image);
    fixture f = create(false);
    pending(&f);
    f.behavior.range_transition = CUI_CALENDAR_RANGE_RESTART_EARLIER;
    frame(&f, 1);
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 10}), 2);
    cui_draw_list draw = frame(&f, 1);
    pixels(draw, image);
    cui_rect selected = cui_bounds(f.ui, cui_calendar_day_id(10, (cui_date){2024, 2, 10}));
    cui_rect not_selected = cui_bounds(f.ui, cui_calendar_day_id(10, (cui_date){2024, 2, 15}));
    unsigned x = (unsigned)(selected.x + selected.w / 2),
             y = (unsigned)(selected.y + selected.h / 2);
    CHECK(!memcmp(image + (y * 800 + x) * 4, &f.theme.primary, 4));
    x = (unsigned)(not_selected.x + not_selected.w / 2);
    y = (unsigned)(not_selected.y + not_selected.h / 2);
    CHECK(!memcmp(image + (y * 800 + x) * 4, &f.theme.surface, 4));
    activate(&f, cui_calendar_day_id(10, (cui_date){2024, 2, 15}), 2);
    draw = frame(&f, 1);
    pixels(draw, image);
    not_selected = cui_bounds(f.ui, cui_calendar_day_id(10, (cui_date){2024, 2, 12}));
    x = (unsigned)(not_selected.x + not_selected.w / 2);
    y = (unsigned)(not_selected.y + not_selected.h / 2);
    CHECK(!memcmp(image + (y * 800 + x) * 4, &f.theme.primary, 4));
    cui_destroy(f.ui);
    free(image);
}
int main(void) {
    combinations_and_limits();
    range_pixels();
    range_transitions();
    validation();
    year_destination();
    recenter();
    equivalence_and_invalid();
    puts("Calendar behavior checks passed");
    return 0;
}
