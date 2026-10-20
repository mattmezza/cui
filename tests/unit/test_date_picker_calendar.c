/* SPDX-License-Identifier: MIT */
#include "cui/cui_date_picker_calendar.h"
#include "cui/cui_software.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); abort(); } } while (0)
enum { W = 1000, H = 650 };
typedef struct {
    cui_context *ui;
    cui_theme theme;
    cui_date_picker_state state;
    cui_date_selection selection;
    cui_date_picker_options options;
    cui_date_picker_calendar_options extension;
    cui_calendar_result result;
    cui_draw_list draw;
    bool base, omit, inert, layout, changed;
    int blocked_day, blocked_month;
} fixture;
static bool disabled(void *user, cui_date d) {
    const fixture *f = user;
    return (f->blocked_day && d.day == f->blocked_day) ||
           (f->blocked_month && d.month == f->blocked_month);
}
static fixture create(bool base, bool dark) {
    fixture f = {.ui = cui_create((cui_config){.width = W, .height = H,
                                               .max_elements = 1024, .max_commands = 2048}),
        .theme = dark ? cui_theme_dark() : cui_theme_light(), .base = base,
        .selection = {.start = {2024, 2, 15}},
        .options = {.viewport = {0, 0, W, H}}};
    CHECK(f.ui);
    cui_set_reduced_motion(f.ui, true);
    cui_date_picker_init(&f.state, 32, f.selection.start);
    return f;
}
static cui_id root(const fixture *f) {
    return f->base ? 10 : cui_date_picker_calendar_id(31);
}
static const cui_semantic *semantic(fixture *f, cui_id id) {
    size_t n;
    const cui_semantic *s = cui_semantics(f->ui, &n);
    for (size_t i = 0; i < n; i++) if (s[i].id == id) return s + i;
    return NULL;
}
static void frame(fixture *f, bool legacy) {
    CHECK(cui_begin_frame(f->ui, W, H, .016f));
    cui_begin_box(f->ui, 1000, (cui_box){.inert = f->inert, .layout_only = f->layout});
    f->result = (cui_calendar_result){0};
    f->changed = false;
    if (!f->omit) {
        if (!f->base) {
            f->changed = legacy
                ? cui_date_picker(f->ui, 31, &f->theme, cui_string("Date"), &f->state,
                                  &f->selection, f->options)
                : cui_date_picker_ex(f->ui, 31, &f->theme, cui_string("Date"), &f->state,
                                     &f->selection, f->options, f->extension);
        } else {
            f->result = legacy
                ? cui_base_date_picker(f->ui, 31, 10, cui_string("Date"), &f->state,
                                       &f->selection, f->options)
                : cui_base_date_picker_ex(f->ui, 31, 10, cui_string("Date"), &f->state,
                                          &f->selection, f->options, f->extension);
            f->changed = f->result.changed;
            cui_begin_box(f->ui, 31, (cui_box){.width = 180, .height = 32});
            cui_end_box(f->ui);
            if (f->state.overlay.open) {
                cui_rect bounds = {0, 40, 900, 600};
                cui_overlay_set_bounds(&f->state.overlay, bounds);
                if (cui_overlay_begin(f->ui, &f->state.overlay)) {
                    cui_begin_portal(f->ui, 32, (cui_box){0}, bounds, 100);
                    cui_calendar_options o = f->options.calendar;
                    o.disabled = o.disabled || f->options.disabled;
                    cui_calendar_submit_ex(f->ui, 10, &f->state.calendar, f->selection, o,
                        legacy ? (cui_calendar_presentation_options){0} : f->extension.presentation);
                    size_t n;
                    const cui_semantic *s = cui_semantics(f->ui, &n);
                    for (size_t i = 0; i < n; i++) if (s[i].id != 31) {
                        cui_begin_box(f->ui, s[i].id, (cui_box){.width = 100, .height = 3});
                        cui_end_box(f->ui);
                    }
                    cui_end_box(f->ui);
                    cui_overlay_end(f->ui);
                }
            }
        }
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    CHECK(cui_last_error(f->ui) == CUI_OK);
}
static void key(fixture *f, cui_key k) {
    CHECK(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void activate(fixture *f, cui_id id, unsigned kind) {
    if (!kind) {
        cui_rect b = cui_bounds(f->ui, id);
        CHECK(b.w > 0 && b.h > 0);
        CHECK(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
            .x = b.x + b.w / 2, .y = b.y + b.h / 2}));
        CHECK(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
            .x = b.x + b.w / 2, .y = b.y + b.h / 2}));
    } else if (kind == 2) {
        CHECK(cui_queue_accessibility_action(f->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
    } else {
        cui_focus(f->ui, id);
        key(f, kind == 1 ? CUI_KEY_ENTER : CUI_KEY_SPACE);
        if (kind == 3)
            CHECK(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    }
}
static void open_picker(fixture *f, unsigned kind) {
    frame(f, false);
    activate(f, 31, kind);
    frame(f, false);
    CHECK(f->state.overlay.open && !f->changed);
    const cui_semantic *s = semantic(f, 31);
    CHECK(s && s->details.expanded && s->details.controls == 32);
}
static cui_id day(fixture *f, unsigned d) {
    return cui_calendar_day_id(root(f), (cui_date){2024, 2, d});
}
static void ranges(void) {
    for (unsigned base = 0; base < 2; base++)
        for (unsigned kind = 0; kind < 4; kind++)
            for (unsigned restart = 0; restart < 2; restart++) {
                fixture f = create(base != 0, false);
                f.options.calendar.range = true;
                f.selection.start.day = 20;
                f.state.calendar.selecting_end = true;
                f.extension.behavior.range_transition = (cui_calendar_range_transition)restart;
                open_picker(&f, kind);
                activate(&f, day(&f, 10), kind);
                frame(&f, false);
                CHECK(f.changed && f.selection.start.day == 10);
                CHECK(f.state.overlay.open == (restart != 0));
                CHECK(f.state.calendar.selecting_end == (restart != 0));
                if (restart) {
                    CHECK(!cui_date_valid(f.selection.end));
                    activate(&f, day(&f, 15), kind);
                    frame(&f, false);
                    CHECK(f.changed && f.selection.end.day == 15 && !f.state.overlay.open);
                } else CHECK(f.selection.end.day == 20);
                CHECK(cui_focused(f.ui) == 31 && !semantic(&f, 31)->details.expanded);
                cui_destroy(f.ui);
            }
    for (unsigned base = 0; base < 2; base++) {
        fixture f = create(base != 0, false);
        f.extension.behavior.range_transition = CUI_CALENDAR_RANGE_RESTART_EARLIER;
        f.options.calendar.range = true;
        f.selection.start.day = 20; /* Endpoint shape, not stale phase flag. */
        open_picker(&f, 2);
        CHECK(!f.state.calendar.selecting_end);
        activate(&f, day(&f, 22), 2);
        frame(&f, false);
        CHECK(!f.state.overlay.open && f.selection.end.day == 22);
        cui_destroy(f.ui);
    }
}
static void validation_and_queues(void) {
    for (unsigned base = 0; base < 2; base++)
        for (unsigned validate = 0; validate < 2; validate++) {
            fixture f = create(base != 0, false);
            f.options.calendar.range = true;
            f.options.calendar.is_disabled = disabled;
            f.options.calendar.disabled_user = &f;
            f.extension.behavior.range_validation = (cui_calendar_range_validation)validate;
            f.state.calendar.selecting_end = true;
            open_picker(&f, 2);
            activate(&f, day(&f, 22), 2);
            f.blocked_day = 15; /* Eligibility changes after previous semantics. */
            frame(&f, false);
            CHECK(f.state.overlay.open == (validate != 0));
            CHECK(f.state.calendar.selecting_end == (validate != 0));
            CHECK(validate ? !f.changed && !cui_date_valid(f.selection.end)
                           : f.changed && f.selection.end.day == 22);
            if (validate) {
                f.blocked_day = 18; /* Disabled interior is allowed. */
                activate(&f, day(&f, 22), 2);
                frame(&f, false);
                CHECK(f.changed && !f.state.overlay.open && f.selection.end.day == 22);
            }
            cui_destroy(f.ui);
        }
    for (unsigned base = 0; base < 2; base++) {
        fixture f = create(base != 0, false);
        f.options.calendar.range = true;
        f.state.calendar.selecting_end = true;
        f.extension.presentation.neighboring_days = true;
        f.extension.behavior.range_transition = CUI_CALENDAR_RANGE_RESTART_EARLIER;
        f.extension.behavior.recenter = CUI_CALENDAR_RECENTER_ACTIVATED_DATE;
        open_picker(&f, 2);
        cui_calendar_cell_info neighbor = {0};
        for (unsigned i = 0; i < 42; i++) {
            cui_calendar_cell_info info;
            if (cui_calendar_cell_resolve(root(&f), f.state.calendar.month, 0, 0, i, &info) &&
                !cui_date_compare(info.date, (cui_date){2024, 3, 1})) neighbor = info;
        }
        CHECK(neighbor.id && neighbor.neighboring);
        activate(&f, neighbor.id, 2); /* Completes, then same original-grid date starts again. */
        activate(&f, day(&f, 20), 2);
        frame(&f, false);
        CHECK(f.state.overlay.open && f.state.calendar.selecting_end);
        CHECK(f.selection.start.month == 2 && f.selection.start.day == 20 &&
              !cui_date_valid(f.selection.end) && f.state.calendar.month.month == 2);
        cui_destroy(f.ui);
        f = create(base != 0, false);
        open_picker(&f, 2);
        activate(&f, day(&f, 20), 2);
        activate(&f, day(&f, 15), 2);
        frame(&f, false);
        CHECK(!f.changed && f.selection.start.day == 15 && !f.state.overlay.open);
        cui_destroy(f.ui);
    }
}
static void headers_and_neighbors(void) {
    for (unsigned base = 0; base < 2; base++)
        for (unsigned kind = 0; kind < 4; kind++) {
            fixture f = create(base != 0, false);
            cui_date_locale locale = *cui_date_locale_builtin(CUI_DATE_LOCALE_IT);
            locale.choose_month = "Mese locale";
            f.options.calendar.locale = &locale;
            f.extension.presentation = (cui_calendar_presentation_options){.month_toggle = true,
                .today = {2024, 2, 16}, .neighboring_days = true};
            f.extension.behavior.year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS;
            open_picker(&f, kind);
            CHECK(semantic(&f, cui_calendar_month_toggle_id(root(&f))));
            const cui_semantic *month_header = semantic(&f, cui_calendar_month_toggle_id(root(&f)));
            CHECK(month_header->name.length == strlen("Mese locale") &&
                  !memcmp(month_header->name.data, "Mese locale", month_header->name.length));
            CHECK(semantic(&f, 31)->name.length > 0);
            activate(&f, cui_calendar_month_toggle_id(root(&f)), kind);
            frame(&f, false);
            CHECK(f.state.overlay.open && !f.changed && f.state.calendar.view == CUI_CALENDAR_MONTHS);
            CHECK(semantic(&f, cui_calendar_month_toggle_id(root(&f)))->details.expanded);
            activate(&f, cui_calendar_month_id(root(&f), 2), kind);
            frame(&f, false);
            CHECK(f.state.calendar.view == CUI_CALENDAR_DAYS);
            activate(&f, cui_calendar_navigation_id(root(&f), 5), kind);
            frame(&f, false);
            activate(&f, cui_calendar_year_id(root(&f), 2025), kind);
            frame(&f, false);
            CHECK(f.state.calendar.view == CUI_CALENDAR_DAYS && f.state.calendar.month.year == 2025);
            CHECK(!f.changed && f.selection.start.year == 2024 && f.state.overlay.open);
            cui_destroy(f.ui);
            f = create(base != 0, false);
            f.extension.presentation.neighboring_days = true;
            f.extension.behavior.recenter = CUI_CALENDAR_RECENTER_ACTIVATED_DATE;
            open_picker(&f, kind);
            cui_calendar_cell_info neighbor = {0};
            for (unsigned i = 0; i < 42; i++) {
                cui_calendar_cell_info info;
                if (cui_calendar_cell_resolve(root(&f), f.state.calendar.month, 0, 0, i, &info) &&
                    !cui_date_compare(info.date, (cui_date){2024, 3, 1})) neighbor = info;
            }
            CHECK(neighbor.id && semantic(&f, neighbor.id));
            activate(&f, neighbor.id, kind);
            frame(&f, false);
            CHECK(f.changed && f.selection.start.month == 3 && f.selection.start.day == 1 &&
                  f.state.calendar.month.month == 3 && !f.state.overlay.open);
            CHECK(cui_focused(f.ui) == 31);
            cui_destroy(f.ui);
        }
}
static void pixels(cui_draw_list draw, unsigned char *out) {
    cui_draw_command commands[2048];
    CHECK(draw.count <= 2048);
    cui_draw_list copy = {.commands = commands, .width = draw.width, .height = draw.height};
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT) commands[copy.count++] = draw.commands[i];
    cui_software *sw = cui_software_create(W, H, NULL);
    CHECK(sw && cui_software_render(sw, &copy, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *data = cui_software_pixels(sw, &w, &h);
    CHECK(w == W && h == H);
    memcpy(out, data, W * H * 4);
    cui_software_destroy(sw);
}
static void equal(fixture *a, fixture *b, unsigned char *pa, unsigned char *pb) {
    CHECK(a->changed == b->changed && a->state.overlay.open == b->state.overlay.open);
    CHECK(!cui_date_compare(a->selection.start, b->selection.start) &&
          !cui_date_compare(a->selection.end, b->selection.end));
    CHECK(!cui_date_compare(a->state.calendar.month, b->state.calendar.month) &&
          !cui_date_compare(a->state.calendar.cursor, b->state.calendar.cursor));
    CHECK(a->state.calendar.view == b->state.calendar.view &&
          a->state.calendar.selecting_end == b->state.calendar.selecting_end);
    CHECK(cui_focused(a->ui) == cui_focused(b->ui));
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    CHECK(na == nb);
    for (size_t i = 0; i < na; i++) {
        CHECK(sa[i].id == sb[i].id && sa[i].role == sb[i].role &&
              sa[i].disabled == sb[i].disabled && sa[i].name.length == sb[i].name.length);
        CHECK(!memcmp(&sa[i].details, &sb[i].details, sizeof sa[i].details));
        CHECK(!sa[i].name.length || !memcmp(sa[i].name.data, sb[i].name.data, sa[i].name.length));
    }
    CHECK(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; i++) {
        const cui_draw_command *x = a->draw.commands + i, *y = b->draw.commands + i;
        CHECK(x->type == y->type && !memcmp(&x->bounds, &y->bounds, sizeof x->bounds) &&
              !memcmp(&x->color, &y->color, sizeof x->color) && x->text.length == y->text.length);
        CHECK(!x->text.length || !memcmp(x->text.data, y->text.data, x->text.length));
    }
    pixels(a->draw, pa); pixels(b->draw, pb);
    CHECK(!memcmp(pa, pb, W * H * 4));
}
static void zero_and_invalid(void) {
    unsigned char *pa = malloc(W * H * 4), *pb = malloc(W * H * 4);
    CHECK(pa && pb);
    for (unsigned base = 0; base < 2; base++)
        for (unsigned dark = 0; dark < 2; dark++)
            for (unsigned view = 0; view < 3; view++)
                for (unsigned months = 1; months <= 3; months++) {
                    fixture a = create(base != 0, dark != 0), b = create(base != 0, dark != 0);
                    a.options.calendar.months = b.options.calendar.months = months;
                    a.state.calendar.view = b.state.calendar.view = (cui_calendar_view)view;
                    frame(&a, true); frame(&b, false); equal(&a, &b, pa, pb);
                    activate(&a, 31, 2); activate(&b, 31, 2);
                    frame(&a, true); frame(&b, false); equal(&a, &b, pa, pb);
                    b.extension.behavior = (cui_calendar_behavior_options){
                        (cui_calendar_range_transition)-1, (cui_calendar_year_destination)INT_MAX,
                        (cui_calendar_recenter_policy)99, (cui_calendar_range_validation)-5};
                    b.extension.presentation.today = (cui_date){2024, 2, 31};
                    frame(&a, true); frame(&b, false); equal(&a, &b, pa, pb);
                    cui_destroy(a.ui); cui_destroy(b.ui);
                }
    free(pa); free(pb);
}
static void paint_and_boundaries(void) {
    unsigned char *a = malloc(W * H * 4), *b = malloc(W * H * 4);
    CHECK(a && b);
    for (unsigned dark = 0; dark < 2; dark++) {
        fixture f = create(false, dark != 0);
        f.extension.presentation.neighboring_days = true;
        open_picker(&f, 2);
        pixels(f.draw, a);
        f.extension.presentation.today = (cui_date){2024, 2, 16};
        frame(&f, false); pixels(f.draw, b);
        CHECK(memcmp(a, b, W * H * 4)); /* Actual today background. */
        f.options.calendar.is_disabled = disabled;
        f.options.calendar.disabled_user = &f;
        f.blocked_day = 16;
        frame(&f, false);
        bool label = false;
        for (size_t i = 0; i < f.draw.count; i++) {
            const cui_draw_command *c = f.draw.commands + i;
            if (c->type == CUI_DRAW_TEXT && c->text.length == 2 &&
                !memcmp(c->text.data, "16", 2)) {
                CHECK(!memcmp(&c->color, &f.theme.muted, sizeof c->color));
                label = true;
            }
        }
        CHECK(label && semantic(&f, day(&f, 16))->disabled);
        f.blocked_day = 0;
        f.extension.presentation.today = f.selection.start;
        frame(&f, false); pixels(f.draw, a);
        f.extension.presentation.today = (cui_date){0};
        frame(&f, false); pixels(f.draw, b);
        CHECK(!memcmp(a, b, W * H * 4)); /* Selected overrides today. */
        cui_rect bounds = f.state.overlay.bounds;
        CHECK(bounds.x >= 0 && bounds.y >= 0 && bounds.x + bounds.w <= W &&
              bounds.y + bounds.h <= H);
        cui_destroy(f.ui);
    }
    free(a); free(b);
    for (unsigned base = 0; base < 2; base++) {
        for (unsigned months = 2; months <= 3; months++) {
            fixture f = create(base != 0, false);
            f.options.calendar.months = months;
            f.extension.presentation.month_toggle = true;
            open_picker(&f, 2);
            CHECK(!semantic(&f, cui_calendar_month_toggle_id(root(&f))));
            cui_destroy(f.ui);
        }
        for (unsigned edge = 0; edge < 2; edge++) {
            fixture f = create(base != 0, false);
            cui_calendar_init(&f.state.calendar, (cui_date){edge ? 9998 : 2, 2, 28});
            f.extension.behavior.year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS;
            f.extension.behavior.range_transition = (cui_calendar_range_transition)99;
            f.extension.presentation.neighboring_days = true;
            open_picker(&f, 2);
            activate(&f, cui_calendar_navigation_id(root(&f), 5), 2); frame(&f, false);
            activate(&f, cui_calendar_year_id(root(&f), edge ? 9999 : 1), 2); frame(&f, false);
            CHECK(f.state.calendar.month.year == (edge ? 9999 : 1) &&
                  f.state.calendar.view == CUI_CALENDAR_DAYS && cui_date_valid(f.state.calendar.cursor));
            CHECK(f.state.overlay.open && !f.changed);
            cui_destroy(f.ui);
        }
        fixture f = create(base != 0, false);
        cui_calendar_init(&f.state.calendar, (cui_date){2024, 2, 29});
        f.options.calendar.months = 2;
        f.options.calendar.is_disabled = disabled;
        f.options.calendar.disabled_user = &f;
        f.extension.behavior.year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS;
        open_picker(&f, 2);
        activate(&f, cui_calendar_navigation_id(root(&f), 5), 2); frame(&f, false);
        activate(&f, cui_calendar_year_id(root(&f), 2025), 2);
        f.blocked_month = 2;
        frame(&f, false);
        CHECK(f.state.calendar.cursor.day == 28 && f.state.calendar.cursor.month == 2);
        CHECK(cui_focused(f.ui) == cui_calendar_navigation_id(root(&f), 5));
        CHECK(f.state.overlay.open && !f.changed);
        cui_destroy(f.ui);
        for (unsigned recenter = 1; recenter <= 2; recenter++) {
            f = create(base != 0, false);
            f.options.calendar.range = true;
            f.options.calendar.months = 2;
            f.state.calendar.selecting_end = true;
            f.extension.behavior.recenter = (cui_calendar_recenter_policy)recenter;
            open_picker(&f, 2);
            activate(&f, cui_calendar_day_id(root(&f), (cui_date){2024, 3, 20}), 2);
            frame(&f, false);
            CHECK(f.state.calendar.month.month == (recenter == 1 ? 3 : 2));
            CHECK(f.selection.end.month == 3 && !f.state.overlay.open && cui_focused(f.ui) == 31);
            cui_destroy(f.ui);
        }
    }
}
static void combinations(void) {
    for (unsigned base = 0; base < 2; base++)
        for (unsigned restart = 0; restart < 2; restart++)
            for (unsigned validate = 0; validate < 2; validate++)
                for (unsigned years = 0; years < 2; years++)
                    for (unsigned recenter = 0; recenter < 3; recenter++) {
                        fixture f = create(base != 0, false);
                        f.options.calendar.range = true;
                        f.selection.start = (cui_date){2025, 2, 20};
                        f.state.calendar.selecting_end = true;
                        f.extension.presentation.month_toggle = true;
                        f.extension.presentation.neighboring_days = true;
                        f.extension.behavior = (cui_calendar_behavior_options){
                            (cui_calendar_range_transition)restart,
                            (cui_calendar_year_destination)years,
                            (cui_calendar_recenter_policy)recenter,
                            (cui_calendar_range_validation)validate};
                        open_picker(&f, 2);
                        activate(&f, cui_calendar_navigation_id(root(&f), 5), 2); frame(&f, false);
                        activate(&f, cui_calendar_year_id(root(&f), 2025), 2); frame(&f, false);
                        CHECK(f.state.calendar.view == (years ? CUI_CALENDAR_DAYS : CUI_CALENDAR_MONTHS));
                        CHECK(!f.changed && f.selection.start.day == 20 && f.state.overlay.open);
                        if (!years) {
                            activate(&f, cui_calendar_month_id(root(&f), 2), 2); frame(&f, false);
                        }
                        activate(&f, cui_calendar_day_id(root(&f), (cui_date){2025, 2, 10}), 2);
                        frame(&f, false);
                        CHECK(f.changed && f.selection.start.year == 2025 && f.selection.start.day == 10);
                        CHECK(f.state.overlay.open == (restart != 0) &&
                              f.state.calendar.selecting_end == (restart != 0));
                        CHECK(restart ? !cui_date_valid(f.selection.end) : f.selection.end.day == 20);
                        cui_destroy(f.ui);
                    }
    fixture f = create(false, false);
    f.extension.presentation.neighboring_days = true;
    open_picker(&f, 1);
    cui_focus(f.ui, day(&f, 15)); key(&f, CUI_KEY_RIGHT); key(&f, CUI_KEY_ENTER);
    frame(&f, false);
    CHECK(f.changed && f.selection.start.day == 16 && !f.state.overlay.open && cui_focused(f.ui) == 31);
    cui_destroy(f.ui);
}
static void lifecycle_and_errors(void) {
    for (unsigned base = 0; base < 2; base++) {
        fixture f = create(base != 0, false);
        f.extension.presentation.month_toggle = true;
        open_picker(&f, 2);
        activate(&f, cui_calendar_navigation_id(root(&f), 5), 2);
        frame(&f, false);
        CHECK(f.state.calendar.view == CUI_CALENDAR_YEARS);
        key(&f, CUI_KEY_ESCAPE); key(&f, CUI_KEY_ESCAPE); frame(&f, false);
        CHECK(f.state.overlay.open && f.state.calendar.view == CUI_CALENDAR_DAYS);
        key(&f, CUI_KEY_ESCAPE); frame(&f, false);
        CHECK(!f.state.overlay.open && cui_focused(f.ui) == 31);
        activate(&f, 31, 2); activate(&f, 31, 2); frame(&f, false);
        CHECK(!f.state.overlay.open && !f.changed);
        activate(&f, 31, 2); frame(&f, false);
        CHECK(f.state.overlay.open);
        CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 990, .y = 640}));
        CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 990, .y = 640}));
        frame(&f, false);
        CHECK(!f.state.overlay.open && cui_focused(f.ui) == 31);
        activate(&f, 31, 2); frame(&f, false);
        CHECK(f.state.overlay.open);
        f.options.calendar.disabled = true;
        activate(&f, day(&f, 20), 2); frame(&f, false);
        CHECK(f.state.overlay.open && !f.changed && f.selection.start.day == 15);
        f.options.disabled = true; frame(&f, false);
        CHECK(!f.state.overlay.open);
        f.options.disabled = f.options.calendar.disabled = false;
        frame(&f, false); /* Publish enabled trigger before requesting an AT action. */
        activate(&f, 31, 2); frame(&f, false);
        CHECK(f.state.overlay.open);
        f.omit = true; activate(&f, day(&f, 20), 2); frame(&f, false);
        CHECK(!f.changed && f.state.overlay.open && !semantic(&f, 31));
        f.omit = false; frame(&f, false);
        f.inert = true; activate(&f, day(&f, 20), 2); frame(&f, false);
        CHECK(!f.changed && f.selection.start.day == 15);
        f.inert = false; f.layout = true; frame(&f, false);
        CHECK(!f.changed);
        cui_destroy(f.ui);
    }
    fixture f = create(false, false);
    f.options.calendar.range = true;
    open_picker(&f, 2);
    activate(&f, day(&f, 20), 2);
    f.options.viewport.w = NAN;
    frame(&f, false);
    CHECK(!f.changed && f.selection.start.day == 20 && f.state.calendar.selecting_end &&
          !f.state.overlay.open && cui_focused(f.ui) == 31);
    /* A subsequent ordinary control is outside the removed modal scope. */
    f.options.viewport = (cui_rect){0, 0, W, H};
    frame(&f, false);
    activate(&f, 31, 2); frame(&f, false); CHECK(f.state.overlay.open);
    cui_destroy(f.ui);
    f = create(false, false);
    CHECK(cui_begin_frame(f.ui, W, H, .016f));
    cui_date_selection saved = f.selection;
    CHECK(!cui_base_date_picker_ex(f.ui, 31, 10, cui_string("Date"), NULL, &f.selection,
        f.options, f.extension).activated);
    CHECK(!cui_date_picker_ex(f.ui, 31, NULL, cui_string("Date"), &f.state, &f.selection,
        f.options, f.extension));
    CHECK(!cui_base_date_picker_ex(f.ui, 31, 10, cui_string("Date"), &f.state, NULL,
        f.options, f.extension).changed);
    CHECK(!cui_date_compare(saved.start, f.selection.start) && !f.state.overlay.open);
    cui_end_frame(f.ui);
    cui_destroy(f.ui);
    CHECK(!cui_date_picker_calendar_id(0));
    cui_id zero_root = UINT64_C(0x713d9902fd4b5a97) + UINT64_C(0x9e3779b97f4a7c15) * 900;
    CHECK(!cui_date_picker_calendar_id(zero_root));
    CHECK(!cui_base_date_picker_ex(NULL, 31, 10, cui_string("Date"), NULL, NULL,
        (cui_date_picker_options){0}, (cui_date_picker_calendar_options){0}).changed);
    CHECK(!cui_date_picker_ex(NULL, 31, NULL, cui_string("Date"), NULL, NULL,
        (cui_date_picker_options){0}, (cui_date_picker_calendar_options){0}));
}
int main(void) {
    fputs("ranges\n", stderr); ranges();
    fputs("validation/queues\n", stderr); validation_and_queues();
    fputs("headers/neighbors\n", stderr); headers_and_neighbors();
    fputs("zero/invalid\n", stderr); zero_and_invalid();
    fputs("paint/boundaries\n", stderr); paint_and_boundaries();
    fputs("combinations\n", stderr); combinations();
    fputs("lifecycle/errors\n", stderr); lifecycle_and_errors();
    puts("DatePicker Calendar extension checks passed");
    return 0;
}
