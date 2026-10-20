/* SPDX-License-Identifier: MIT */
#include "cui/cui_calendar_presentation.h"
#include "cui/cui_software.h"
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
        f->result = cui_calendar(f->ui, 10, &f->theme, &f->state, &f->selection, f->options);
    else if (mode == 1)
        f->result = cui_calendar_ex(f->ui, 10, &f->theme, &f->state, &f->selection, f->options,
                                    f->presentation);
    else {
        if (mode == 2)
            f->result = cui_base_calendar_ex(f->ui, 10, &f->state, &f->selection, f->options,
                                             f->presentation);
        else {
            f->result = cui_calendar_reduce_ex(f->ui, 10, &f->state, &f->selection, f->options,
                                               f->presentation);
            cui_calendar_submit_ex(f->ui, 10, &f->state, f->selection, f->options, f->presentation);
        }
        size_t n;
        const cui_semantic *sem = cui_semantics(f->ui, &n);
        for (size_t i = 0; i < n; i++) {
            cui_begin_box(f->ui, sem[i].id, (cui_box){.width = 40, .height = 10});
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
    } else
        CHECK(cui_queue_accessibility_action(f->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
}
static void navigation(void) {
    for (unsigned mode = 1; mode <= 3; mode++)
        for (unsigned kind = 0; kind < 3; kind++) {
            fixture f = create(false);
            frame(&f, mode);
            cui_id header = cui_calendar_month_toggle_id(10);
            const cui_semantic *sem = semantic(&f, header);
            CHECK(sem && !sem->details.expanded && !sem->details.selected);
            CHECK(sem->name.length == strlen("Scegli mese") &&
                  !memcmp(sem->name.data, "Scegli mese", sem->name.length));
            activate(&f, header, kind);
            frame(&f, mode);
            CHECK(f.state.view == CUI_CALENDAR_MONTHS && f.result.navigated);
            CHECK(!f.result.changed && !f.result.activated && f.selection.start.month == 2);
            CHECK(cui_focused(f.ui) == cui_calendar_month_id(10, 2));
            sem = semantic(&f, header);
            CHECK(sem && sem->details.expanded && sem->details.selected);
            /* Ordered keys use the updated grid cursor before activation. */
            key(&f, CUI_KEY_RIGHT);
            key(&f, CUI_KEY_RIGHT);
            key(&f, CUI_KEY_ENTER);
            frame(&f, mode);
            CHECK(f.state.view == CUI_CALENDAR_DAYS && f.state.month.month == 4);
            CHECK(!f.result.changed && !f.result.activated);
            CHECK(cui_focused(f.ui) == cui_calendar_day_id(10, (cui_date){2024, 4, 1}));
            activate(&f, header, kind);
            frame(&f, mode);
            key(&f, CUI_KEY_ESCAPE);
            frame(&f, mode);
            CHECK(f.state.view == CUI_CALENDAR_DAYS && cui_focused(f.ui) == header);
            activate(&f, header, kind);
            frame(&f, mode);
            activate(&f, header, kind);
            frame(&f, mode);
            CHECK(f.state.view == CUI_CALENDAR_DAYS && cui_focused(f.ui) == header);
            cui_id year = cui_calendar_navigation_id(10, 5);
            activate(&f, year, kind);
            frame(&f, mode);
            CHECK(f.state.view == CUI_CALENDAR_YEARS);
            CHECK(semantic(&f, year)->details.expanded);
            activate(&f, year, kind);
            frame(&f, mode);
            CHECK(f.state.view == CUI_CALENDAR_DAYS);
            /* Independent headers remain ordered in one submitted event batch. */
            if (kind == 1) {
                activate(&f, header, kind);
                frame(&f, mode);
                activate(&f, year, kind);
            } else {
                activate(&f, header, kind);
                activate(&f, year, kind);
            }
            frame(&f, mode);
            CHECK(f.state.view == CUI_CALENDAR_YEARS);
            CHECK(!semantic(&f, header)->details.expanded && semantic(&f, year)->details.expanded);
            cui_destroy(f.ui);
        }
}
static bool policy(void *user, cui_date date) {
    unsigned blocked = *(unsigned *)user;
    return blocked == 99 || (unsigned)date.month == blocked;
}
static void policies(void) {
    fixture f = create(false);
    unsigned blocked = 0;
    f.options.is_disabled = policy;
    f.options.disabled_user = &blocked;
    frame(&f, 1);
    activate(&f, cui_calendar_month_toggle_id(10), 2);
    frame(&f, 1);
    /* Eligibility changes after prior semantics and before reduction. */
    activate(&f, cui_calendar_month_id(10, 3), 2);
    blocked = 3;
    frame(&f, 1);
    CHECK(f.state.view == CUI_CALENDAR_MONTHS && f.state.month.month == 2);
    CHECK(!f.result.navigated && !f.result.changed);
    CHECK(semantic(&f, cui_calendar_month_id(10, 3))->disabled);
    blocked = 0;
    frame(&f, 1);
    activate(&f, cui_calendar_month_id(10, 3), 2);
    frame(&f, 1);
    CHECK(f.state.view == CUI_CALENDAR_DAYS && f.state.month.month == 3);
    activate(&f, cui_calendar_month_toggle_id(10), 2);
    f.options.disabled = true;
    frame(&f, 1);
    CHECK(f.state.view == CUI_CALENDAR_DAYS && !f.result.navigated);
    CHECK(semantic(&f, cui_calendar_month_toggle_id(10))->disabled);
    f.options.disabled = false;
    blocked = 99;
    frame(&f, 1);
    activate(&f, cui_calendar_month_toggle_id(10), 2);
    frame(&f, 1);
    CHECK(f.state.view == CUI_CALENDAR_MONTHS);
    CHECK(cui_focused(f.ui) == cui_calendar_month_toggle_id(10));
    key(&f, CUI_KEY_ESCAPE);
    frame(&f, 1);
    CHECK(f.state.view == CUI_CALENDAR_DAYS);
    cui_destroy(f.ui);
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
static void today(void) {
    unsigned char *image = malloc(800 * 500 * 4);
    CHECK(image);
    for (unsigned dark = 0; dark < 2; dark++) {
        fixture f = create(dark != 0);
        f.presentation.today = (cui_date){2024, 2, 20};
        for (unsigned state = 0; state < 4; state++) {
            unsigned blocked = state == 1 || state == 3 ? 2 : 0;
            f.options.is_disabled = policy;
            f.options.disabled_user = &blocked;
            if (state >= 2)
                f.selection.start = f.presentation.today;
            cui_draw_list draw = frame(&f, 1);
            cui_rect b = cui_bounds(f.ui, cui_calendar_day_id(10, f.presentation.today));
            pixels(draw, image);
            unsigned x = (unsigned)(b.x + b.w / 2), y = (unsigned)(b.y + b.h / 2);
            cui_color expected = state >= 2 ? f.theme.primary : f.theme.accent;
            CHECK(!memcmp(image + (y * 800 + x) * 4, &expected, 4));
            bool text = false;
            for (size_t i = 0; i < draw.count; i++) {
                const cui_draw_command *c = &draw.commands[i];
                if (c->type == CUI_DRAW_TEXT && c->text.length == 2 &&
                    !memcmp(c->text.data, "20", 2)) {
                    CHECK(color_equal(c->color, state == 1 || state == 3 ? f.theme.muted
                                                : state == 2 ? f.theme.on_primary
                                                             : f.theme.accent_foreground));
                    text = true;
                }
            }
            CHECK(text && !f.result.changed && !f.result.activated);
        }
        cui_destroy(f.ui);
    }
    free(image);
}
static void equivalence(void) {
    unsigned char *a = malloc(800 * 500 * 4), *b = malloc(800 * 500 * 4);
    CHECK(a && b);
    for (unsigned dark = 0; dark < 2; dark++)
        for (unsigned view = 0; view < 3; view++)
            for (unsigned months = 1; months <= 3; months++) {
                fixture old = create(dark != 0), ex = create(dark != 0);
                old.state.view = ex.state.view = (cui_calendar_view)view;
                old.options.months = ex.options.months = months;
                ex.presentation = (cui_calendar_presentation_options){0};
                cui_draw_list x = frame(&old, 0), y = frame(&ex, 1);
                CHECK(x.count == y.count);
                for (size_t i = 0; i < x.count; i++) {
                    const cui_draw_command *u = &x.commands[i], *v = &y.commands[i];
                    CHECK(u->type == v->type && color_equal(u->color, v->color));
                    CHECK(!memcmp(&u->bounds, &v->bounds, sizeof u->bounds));
                    CHECK(u->font_size == v->font_size && u->radius == v->radius &&
                          u->stroke == v->stroke);
                    CHECK(u->text.length == v->text.length &&
                          (!u->text.length || !memcmp(u->text.data, v->text.data, u->text.length)));
                }
                pixels(x, a);
                pixels(y, b);
                CHECK(!memcmp(a, b, 800 * 500 * 4));
                size_t n, m;
                const cui_semantic *u = cui_semantics(old.ui, &n), *v = cui_semantics(ex.ui, &m);
                CHECK(n == m);
                for (size_t i = 0; i < n; i++) {
                    CHECK(u[i].id == v[i].id && u[i].role == v[i].role &&
                          u[i].disabled == v[i].disabled);
                    CHECK(u[i].checked == v[i].checked &&
                          u[i].tab_stop_disabled == v[i].tab_stop_disabled);
                    CHECK(u[i].details.selected == v[i].details.selected &&
                          u[i].details.expanded == v[i].details.expanded);
                    CHECK(u[i].name.length == v[i].name.length &&
                          !memcmp(u[i].name.data, v[i].name.data, u[i].name.length));
                }
                /* Invalid date is omitted, and multimonth ignores the toggle. */
                ex.presentation = (cui_calendar_presentation_options){.month_toggle = months > 1,
                                                                      .today = {2024, 2, 30}};
                y = frame(&ex, 1);
                pixels(y, b);
                CHECK(!memcmp(a, b, 800 * 500 * 4));
                CHECK(!semantic(&ex, cui_calendar_month_toggle_id(10)));
                cui_destroy(old.ui);
                cui_destroy(ex.ui);
            }
    free(a);
    free(b);
}
static void invalid_and_omission(void) {
    fixture f = create(false);
    f.presentation.neighboring_days = true;
    CHECK(cui_begin_frame(f.ui, 800, 500, .016f));
    cui_calendar_result r =
        cui_calendar_ex(f.ui, 10, NULL, &f.state, &f.selection, f.options, f.presentation);
    CHECK(!r.changed && !r.navigated);
    r = cui_base_calendar_ex(f.ui, 10, NULL, &f.selection, f.options, f.presentation);
    CHECK(!r.changed && !r.navigated);
    r = cui_calendar_reduce_ex(NULL, 10, &f.state, &f.selection, f.options, f.presentation);
    CHECK(!r.changed && !r.navigated);
    cui_calendar_submit_ex(f.ui, 10, NULL, f.selection, f.options, f.presentation);
    cui_end_frame(f.ui);
    frame(&f, 1);
    CHECK(cui_begin_frame(f.ui, 800, 500, .016f));
    cui_end_frame(f.ui);
    CHECK(!semantic(&f, cui_calendar_month_toggle_id(10)));
    frame(&f, 1);
    CHECK(f.state.month.year == 2024 && f.state.month.month == 2);
    cui_destroy(f.ui);
}
static cui_calendar_cell_info cell_for(fixture *f, unsigned panel, cui_date date) {
    for (unsigned cell = 0; cell < 42; cell++) {
        cui_calendar_cell_info info;
        if (cui_calendar_cell_resolve(10, f->state.month, cui_calendar_first_weekday(f->options),
                                      panel, cell, &info) &&
            !cui_date_compare(info.date, date))
            return info;
    }
    CHECK(false);
    return (cui_calendar_cell_info){0};
}
static void cell_queries(void) {
    cui_calendar_cell_info info = {.id = 42};
    CHECK(!cui_calendar_cell_resolve(0, (cui_date){2024, 2, 1}, 1, 0, 0, &info) && !info.id);
    CHECK(!cui_calendar_cell_resolve(10, (cui_date){2024, 2, 30}, 1, 0, 0, &info) && !info.id);
    CHECK(!cui_calendar_cell_resolve(10, (cui_date){2024, 2, 1}, 1, 3, 0, &info) && !info.id);
    CHECK(!cui_calendar_cell_resolve(10, (cui_date){2024, 2, 1}, 1, 0, 42, &info) && !info.id);
    CHECK(!cui_calendar_cell_resolve(10, (cui_date){2024, 2, 1}, 1, 0, 0, NULL));
    CHECK(!cui_calendar_cell_resolve(10, (cui_date){9999, 12, 1}, 1, 1, 0, &info) && !info.id);
    CHECK(!cui_calendar_cell_resolve(10, (cui_date){1, 1, 1}, 0, 0, 0, &info) && !info.id);
    const cui_date starts[] = {{2024, 2, 1}, {2023, 12, 1}, {1, 1, 1}, {9999, 11, 1}};
    for (unsigned start = 0; start < sizeof starts / sizeof starts[0]; start++)
        for (unsigned first = 0; first < 7; first++) {
            cui_id ids[126];
            size_t count = 0;
            for (unsigned panel = 0; panel < 3; panel++)
                for (unsigned cell = 0; cell < 42; cell++) {
                    if (!cui_calendar_cell_resolve(10, starts[start], first, panel, cell, &info))
                        continue;
                    for (size_t i = 0; i < count; i++)
                        CHECK(ids[i] != info.id);
                    ids[count++] = info.id;
                    if (!info.neighboring)
                        CHECK(info.id == cui_calendar_day_id(10, info.date));
                    else
                        CHECK(info.id != cui_calendar_day_id(10, info.date));
                    cui_calendar_cell_info same;
                    CHECK(cui_calendar_cell_resolve(10, starts[start], first + 7, panel, cell,
                                                    &same));
                    CHECK(same.id == info.id && !cui_date_compare(same.date, info.date));
                    /* Construct a root whose XOR-derived occurrence would be zero. */
                    cui_id zero_root = info.id ^ 10;
                    CHECK(!cui_calendar_cell_resolve(zero_root, starts[start], first, panel, cell,
                                                     &same));
                    CHECK(!same.id && !cui_date_valid(same.date));
                    zero_root = cui_calendar_day_id(10, info.date) ^ 10;
                    CHECK(!cui_calendar_cell_resolve(zero_root, starts[start], first, panel, cell,
                                                     &same));
                    CHECK(!same.id && !cui_date_valid(same.date));
                }
        }
}
static void neighbor_geometry(void) {
    for (unsigned months = 1; months <= 3; months++) {
        fixture f = create(false);
        f.options.months = months;
        f.presentation.neighboring_days = true;
        frame(&f, 1);
        unsigned tab_days = 0;
        for (unsigned panel = 0; panel < months; panel++)
            for (unsigned cell = 0; cell < 42; cell++) {
                cui_calendar_cell_info info;
                CHECK(cui_calendar_cell_resolve(
                    10, f.state.month, cui_calendar_first_weekday(f.options), panel, cell, &info));
                const cui_semantic *sem = semantic(&f, info.id);
                CHECK(sem && sem->role == CUI_ROLE_BUTTON && sem->details.parent == 10);
                CHECK(sem->bounds.w > 0 && sem->bounds.h > 0 && sem->name.length > 0);
                if (!sem->tab_stop_disabled)
                    tab_days++;
                if (info.neighboring)
                    CHECK(sem->tab_stop_disabled);
            }
        CHECK(tab_days == 1);
        cui_focus(f.ui, cui_calendar_day_id(10, f.state.cursor));
        for (unsigned tab = 0; tab < 8; tab++) {
            key(&f, CUI_KEY_TAB);
            frame(&f, 1);
            for (unsigned panel = 0; panel < months; panel++)
                for (unsigned cell = 0; cell < 42; cell++) {
                    cui_calendar_cell_info info;
                    CHECK(cui_calendar_cell_resolve(10, f.state.month,
                                                    cui_calendar_first_weekday(f.options), panel,
                                                    cell, &info));
                    if (info.neighboring)
                        CHECK(cui_focused(f.ui) != info.id);
                }
        }

        if (months > 1) {
            cui_calendar_cell_info neighbor = cell_for(&f, 0, (cui_date){2024, 3, 1});
            cui_calendar_cell_info canonical = cell_for(&f, 1, (cui_date){2024, 3, 1});
            CHECK(neighbor.neighboring && !canonical.neighboring && neighbor.id != canonical.id);
            activate(&f, neighbor.id, 2);
            frame(&f, 1);
            CHECK(f.state.month.month == 2 && f.selection.start.month == 3);
            CHECK(cui_focused(f.ui) == canonical.id);
        }
        cui_destroy(f.ui);
    }
}
static void neighbor_routing(void) {
    for (unsigned kind = 0; kind < 3; kind++) {
        fixture f = create(false);
        f.presentation.neighboring_days = true;
        frame(&f, 1);
        cui_calendar_cell_info before = cell_for(&f, 0, (cui_date){2024, 1, 29});
        activate(&f, before.id, kind);
        frame(&f, 1);
        CHECK(f.result.activated && f.result.changed && f.result.navigated);
        CHECK(f.selection.start.month == 1 && f.selection.start.day == 29);
        CHECK(f.state.month.month == 1 &&
              cui_focused(f.ui) == cui_calendar_day_id(10, before.date));
        cui_destroy(f.ui);
        /* Two activations must complete a range, not collapse into one selection. */
        f = create(false);
        f.options.range = true;
        f.presentation.neighboring_days = true;
        frame(&f, 1);
        before = cell_for(&f, 0, (cui_date){2024, 1, 29});
        cui_calendar_cell_info after = cell_for(&f, 0, (cui_date){2024, 3, 10});
        if (kind == 1) {
            cui_focus(f.ui, before.id);
            key(&f, CUI_KEY_ENTER);
            key(&f, CUI_KEY_ENTER);
        } else {
            activate(&f, before.id, kind);
            activate(&f, after.id, kind);
        }
        frame(&f, 1);
        CHECK(f.result.activated && !f.state.selecting_end && cui_date_valid(f.selection.end));
        CHECK(!cui_date_compare(f.selection.start, before.date));
        CHECK(!cui_date_compare(f.selection.end, kind == 1 ? before.date : after.date));
        cui_destroy(f.ui);
        /* A preceding month navigation does not remap the old cell to a new date. */
        if (kind != 1) {
            f = create(false);
            f.presentation.neighboring_days = true;
            frame(&f, 1);
            before = cell_for(&f, 0, (cui_date){2024, 1, 29});
            activate(&f, cui_calendar_navigation_id(10, 3), kind);
            activate(&f, before.id, kind);
            frame(&f, 1);
            CHECK(!cui_date_compare(f.selection.start, before.date) && f.state.month.month == 1);
            cui_destroy(f.ui);
            f = create(false);
            f.presentation.neighboring_days = true;
            frame(&f, 1);
            before = cell_for(&f, 0, (cui_date){2024, 1, 29});
            activate(&f, before.id, kind);
            activate(&f, cui_calendar_month_toggle_id(10), kind);
            frame(&f, 1);
            CHECK(f.state.view == CUI_CALENDAR_MONTHS && f.state.month.month == 1);
            CHECK(!cui_date_compare(f.selection.start, before.date));
            cui_destroy(f.ui);
        }
    }
    fixture f = create(false);
    f.presentation.neighboring_days = true;
    unsigned blocked = 0;
    f.options.is_disabled = policy;
    f.options.disabled_user = &blocked;
    frame(&f, 1);
    cui_calendar_cell_info before = cell_for(&f, 0, (cui_date){2024, 1, 29});
    activate(&f, before.id, 2);
    blocked = 1;
    frame(&f, 1);
    CHECK(!f.result.activated && !f.result.changed && f.state.month.month == 2);
    CHECK(semantic(&f, before.id)->disabled);
    blocked = 0;
    frame(&f, 1);
    activate(&f, before.id, 2);
    frame(&f, 1);
    CHECK(f.result.activated && f.selection.start.month == 1);
    cui_destroy(f.ui);
}
static void neighbor_edges_and_space(void) {
    fixture range = create(false);
    range.presentation.neighboring_days = true;
    range.options.range = true;
    range.state.selecting_end = true;
    frame(&range, 1);
    cui_calendar_cell_info earlier = cell_for(&range, 0, (cui_date){2024, 1, 29});
    activate(&range, earlier.id, 2);
    frame(&range, 1);
    CHECK(!cui_date_compare(range.selection.start, earlier.date));
    CHECK(!cui_date_compare(range.selection.end, (cui_date){2024, 2, 15}));
    CHECK(!range.state.selecting_end && range.state.month.month == 1);
    cui_destroy(range.ui);

    fixture f = create(false);
    f.presentation.neighboring_days = true;
    frame(&f, 1);
    cui_calendar_cell_info before = cell_for(&f, 0, (cui_date){2024, 1, 29});
    cui_focus(f.ui, before.id);
    key(&f, CUI_KEY_SPACE);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    frame(&f, 1);
    CHECK(f.result.activated && !cui_date_compare(f.selection.start, before.date));
    cui_destroy(f.ui);
    f = create(false);
    f.presentation.neighboring_days = true;
    frame(&f, 1);
    before = cell_for(&f, 0, (cui_date){2024, 1, 29});
    cui_calendar_cell_info after = cell_for(&f, 0, (cui_date){2024, 3, 10});
    activate(&f, before.id, 2);
    activate(&f, after.id, 2);
    f.options.minimum = (cui_date){2024, 2, 10};
    f.options.maximum = (cui_date){2024, 2, 20};
    frame(&f, 1);
    CHECK(!f.result.activated && !f.result.changed);
    CHECK(semantic(&f, before.id)->disabled && semantic(&f, after.id)->disabled);
    cui_destroy(f.ui);
    const cui_date edges[] = {{1, 1, 1}, {9999, 12, 1}};
    for (unsigned edge = 0; edge < 2; edge++) {
        f = create(false);
        f.presentation.neighboring_days = true;
        f.options.months = 3;
        cui_calendar_init(&f.state, edges[edge]);
        f.selection.start = edges[edge];
        frame(&f, 1);
        unsigned valid = 0;
        for (unsigned panel = 0; panel < 3; panel++)
            for (unsigned cell = 0; cell < 42; cell++) {
                cui_calendar_cell_info info;
                if (cui_calendar_cell_resolve(10, edges[edge],
                                              cui_calendar_first_weekday(f.options), panel, cell,
                                              &info)) {
                    CHECK(semantic(&f, info.id));
                    valid++;
                }
            }
        CHECK(valid > 0 && valid <= 126);
        CHECK(cui_date_valid(f.state.cursor));
        cui_destroy(f.ui);
    }
}
static void neighbor_paint(void) {
    unsigned char *image = malloc(800 * 500 * 4);
    CHECK(image);
    for (unsigned dark = 0; dark < 2; dark++)
        for (unsigned state = 0; state < 5; state++) {
            fixture f = create(dark != 0);
            f.presentation.neighboring_days = true;
            cui_date date = {2024, 1, 29};
            unsigned blocked = state == 3 ? 1 : 0;
            f.options.is_disabled = policy;
            f.options.disabled_user = &blocked;
            if (state == 1 || state == 2 || state == 3)
                f.selection.start = date;
            if (state == 2) {
                f.options.range = true;
                f.selection.end = (cui_date){2024, 2, 10};
            }
            if (state == 4)
                f.presentation.today = date;
            cui_draw_list draw = frame(&f, 1);
            cui_calendar_cell_info info = cell_for(&f, 0, date);
            cui_rect bounds = cui_bounds(f.ui, info.id);
            cui_color fg = state == 0 || state == 3 ? f.theme.muted
                           : state == 4             ? f.theme.accent_foreground
                                                    : f.theme.on_primary;
            cui_color bg = state == 0   ? f.theme.surface
                           : state == 4 ? f.theme.accent
                                        : f.theme.primary;
            pixels(draw, image);
            unsigned x = (unsigned)(bounds.x + bounds.w / 2),
                     y = (unsigned)(bounds.y + bounds.h / 2);
            CHECK(!memcmp(image + (y * 800 + x) * 4, &bg, 4));
            bool found = false;
            for (size_t i = 0; i < draw.count; i++) {
                const cui_draw_command *c = &draw.commands[i];
                if (c->type == CUI_DRAW_TEXT && c->bounds.x >= bounds.x &&
                    c->bounds.x < bounds.x + bounds.w && c->bounds.y >= bounds.y &&
                    c->bounds.y < bounds.y + bounds.h) {
                    CHECK(color_equal(c->color, fg));
                    found = true;
                }
            }
            CHECK(found && !f.result.changed);
            cui_destroy(f.ui);
        }
    free(image);
}
int main(void) {
    cell_queries();
    neighbor_geometry();
    neighbor_routing();
    neighbor_paint();
    neighbor_edges_and_space();
    navigation();
    policies();
    today();
    equivalence();
    invalid_and_omission();
    puts("Calendar presentation checks passed");
    return 0;
}
