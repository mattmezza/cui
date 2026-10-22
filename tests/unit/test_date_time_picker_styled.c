/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_picker_composition_internal.h"
#include "../../src/components/cui_picker_paint_internal.h"
#include "cui/cui_components.h"
#include "cui/cui_date_time_picker.h"
#include "cui/cui_software.h"
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
    cui_theme theme;
    cui_date_time_picker_state s;
    cui_date_time_value v;
    cui_date_time_picker_options o;
    cui_date_time_preset p;
    cui_date_time_picker_result r;
    cui_draw_list draw;
} fixture;
static void create(fixture *f) {
    *f = (fixture){.v = {{2024, 2, 15}, {9, 30, 45}},
                   .o = {.picker = {.viewport = {0, 0, 1000, 800}},
                         .precision = CUI_DATE_TIME_SECOND,
                         .default_time = {8, 15, 0},
                         .cleanable = true,
                         .clear_id = 33,
                         .clear_label = {"Clear", 5}}};
    f->ui = cui_create(
        (cui_config){.width = 1000, .height = 800, .max_elements = 2048, .max_commands = 4096});
    CHECK(f->ui);
    f->theme = cui_theme_light();
    f->p = (cui_date_time_preset){.id = 34, .label = {"Tomorrow", 8}, .date = {2024, 2, 16}};
    f->o.presets = &f->p;
    f->o.preset_count = 1;
    cui_date_time_picker_init(&f->s, 32, (cui_date){2024, 2, 1});
}
static void frame(fixture *f) {
    CHECK(cui_begin_frame(f->ui, 1000, 800, .016f));
    f->r = cui_date_time_picker(f->ui, 31, &f->theme, cui_string("Date/time"), &f->s, &f->v, f->o);
    f->draw = cui_end_frame(f->ui);
    CHECK(cui_last_error(f->ui) == CUI_OK);
}
static void activate(fixture *f, cui_id id) {
    CHECK(cui_queue_accessibility_action(f->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
    frame(f);
}
static bool semantic(fixture *f, cui_id id) {
    size_t n;
    const cui_semantic *s = cui_semantics(f->ui, &n);
    for (size_t i = 0; i < n; i++)
        if (s[i].id == id)
            return true;
    return false;
}
static void inside(fixture *f, cui_id id) {
    cui_rect a = cui_bounds(f->ui, id), b = f->s.actions.picker.overlay.bounds;
    CHECK(a.w > 0 && a.h > 0 && isfinite(a.x) && isfinite(a.y));
    CHECK(a.x >= b.x - .01f && a.y >= b.y - .01f && a.x + a.w <= b.x + b.w + .01f &&
          a.y + a.h <= b.y + b.h + .01f);
}
static unsigned char *raster(cui_draw_list draw) {
    cui_draw_command *commands = malloc(draw.count * sizeof *commands);
    CHECK(commands);
    cui_draw_list filtered = draw;
    filtered.commands = commands;
    filtered.count = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[filtered.count++] = draw.commands[i];
    cui_software *sw = cui_software_create(1000, 800, NULL);
    CHECK(sw);
    CHECK(cui_software_render(sw, &filtered, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *data = cui_software_pixels(sw, &w, &h);
    CHECK(w == 1000 && h == 800);
    unsigned char *copy = malloc(w * h * 4);
    CHECK(copy);
    memcpy(copy, data, w * h * 4);
    cui_software_destroy(sw);
    free(commands);
    return copy;
}
static void geometry(void) {
    for (unsigned months = 1; months <= 3; months++)
        for (unsigned mode = 0; mode < 4; mode++) {
            fixture f;
            create(&f);
            f.o.picker.calendar.months = months;
            f.o.precision = mode & 1 ? CUI_DATE_TIME_SECOND : CUI_DATE_TIME_MINUTE;
            f.o.twelve_hour = (mode & 2) != 0;
            frame(&f);
            CHECK(f.r.valid);
            activate(&f, 31);
            frame(&f);
            CHECK(f.s.actions.picker.overlay.open && f.r.valid);
            cui_id time = cui_date_time_picker_time_id(31);
            inside(&f, time);
            inside(&f, 34);
            for (unsigned i = 0; i < 4; i++) {
                bool shown = (i != 2 || (mode & 1)) && (i != 3 || (mode & 2));
                CHECK(semantic(&f, cui_time_segment_id(time, i)) == shown);
                if (shown)
                    inside(&f, cui_time_segment_id(time, i));
            }
            unsigned char *pixels = raster(f.draw);
            CHECK(pixels[3] == 255);
            free(pixels);
            cui_destroy(f.ui);
        }
    fixture f;
    create(&f);
    f.o.picker.viewport.w = 180;
    cui_date_time_picker_state before = f.s;
    cui_date_time_value value = f.v;
    frame(&f);
    CHECK(!f.r.valid && !memcmp(&before, &f.s, sizeof before) &&
          !memcmp(&value, &f.v, sizeof value));
    CHECK(!semantic(&f, 31));
    cui_destroy(f.ui);
}
static void supported_metrics(void) {
    const float cells[] = {20, 32, 64, 128};
    for (unsigned theme = 0; theme < 3; theme++)
        for (unsigned cell = 0; cell < 4; cell++) {
            fixture f;
            create(&f);
            f.theme = theme == 0   ? cui_theme_light()
                      : theme == 1 ? cui_theme_dark()
                                   : cui_theme_high_contrast();
            f.o.picker.calendar.cell_size = cells[cell];
            f.o.picker.calendar.months = 2;
            frame(&f);
            if (cell >= 3) {
                CHECK(!f.r.valid && !semantic(&f, 31));
            } else {
                CHECK(f.r.valid);
                activate(&f, 31);
                frame(&f);
                inside(&f, cui_date_time_picker_time_id(31));
                inside(&f, 34);
                inside(&f, cui_calendar_day_id(cui_date_picker_calendar_id(31), f.v.date));
                unsigned char *pixels = raster(f.draw);
                free(pixels);
            }
            cui_destroy(f.ui);
        }
    fixture f;
    create(&f);
    cui_date_time_preset presets[8];
    for (unsigned i = 0; i < 8; i++) {
        presets[i] = f.p;
        presets[i].id = 100 + i;
    }
    f.o.presets = presets;
    f.o.preset_count = 8;
    frame(&f);
    CHECK(f.r.valid);
    activate(&f, 31);
    frame(&f);
    for (unsigned i = 0; i < 8; i++)
        inside(&f, presets[i].id);
    cui_destroy(f.ui);
}
static void interactions(void) {
    fixture f;
    create(&f);
    frame(&f);
    activate(&f, 31);
    frame(&f);
    cui_id time = cui_date_time_picker_time_id(31), hour = cui_time_segment_id(time, 0);
    cui_focus(f.ui, hour);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = {'1'}}));
    frame(&f);
    CHECK(f.s.time_field.digit_count == 1 && f.v.time.hour == 1);
    frame(&f);
    CHECK(f.s.time_field.digit_count == 1);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = {'2'}}));
    frame(&f);
    CHECK(f.v.time.hour == 12 && f.s.time_field.digit_count == 0);
    CHECK(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    frame(&f);
    CHECK(cui_focused(f.ui) != 31 && cui_focused(f.ui) != 33);
    activate(&f, 34);
    CHECK(f.v.date.day == 16 && f.v.time.hour == 12 && !f.s.actions.picker.overlay.open);
    CHECK(cui_focused(f.ui) == 31);
    activate(&f, 33);
    CHECK(!cui_date_valid(f.v.date) && f.v.time.hour == 8 && !semantic(&f, 33));
    f.o.picker.disabled = true;
    frame(&f);
    CHECK(!f.s.actions.picker.overlay.open);
    cui_destroy(f.ui);
}
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
static void legacy_time_equivalence(void) {
    fixture a, b;
    create(&a);
    create(&b);
    cui_time_state states[2] = {{0}, {0}};
    cui_time times[2] = {{9, 30, 45}, {9, 30, 45}};
    for (unsigned stage = 0; stage < 2; stage++) {
        if (stage)
            for (unsigned i = 0; i < 2; i++) {
                cui_context *ui = i ? b.ui : a.ui;
                cui_focus(ui, cui_time_segment_id(51, 1));
                CHECK(cui_queue_event(ui,
                                      (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
            }
        for (unsigned i = 0; i < 2; i++) {
            fixture *f = i ? &b : &a;
            CHECK(cui_begin_frame(f->ui, 1000, 800, .016f));
            cui_time_options o = {.seconds = true, .twelve_hour = true};
            if (i) {
                (void)cui_picker_time_reduce(f->ui, 51, cui_string("Time"), &states[i], &times[i],
                                             o);
                cui_picker_time_submit(f->ui, 51, cui_string("Time"), &states[i], &times[i], o);
                cui_picker_time_paint(f->ui, 51, &f->theme, &times[i], o);
            } else
                (void)cui_time_field(f->ui, 51, &f->theme, cui_string("Time"), &states[i],
                                     &times[i], o);
            f->draw = cui_end_frame(f->ui);
            CHECK(cui_last_error(f->ui) == CUI_OK);
        }
        CHECK(times[0].hour == times[1].hour && times[0].minute == times[1].minute &&
              times[0].second == times[1].second);
        size_t na, nb;
        const cui_semantic *sa = cui_semantics(a.ui, &na), *sb = cui_semantics(b.ui, &nb);
        CHECK(na == nb);
        for (size_t j = 0; j < na; j++) {
            CHECK(sa[j].id == sb[j].id && sa[j].role == sb[j].role &&
                  sa[j].disabled == sb[j].disabled);
            CHECK(sa[j].name.length == sb[j].name.length &&
                  !memcmp(sa[j].name.data, sb[j].name.data, sa[j].name.length));
            CHECK(sa[j].value == sb[j].value && sa[j].focused == sb[j].focused &&
                  sa[j].checked == sb[j].checked &&
                  sa[j].tab_stop_disabled == sb[j].tab_stop_disabled);
            CHECK(details_equal(&sa[j].details, &sb[j].details));
        }
        unsigned char *pa = raster(a.draw), *pb = raster(b.draw);
        CHECK(!memcmp(pa, pb, 1000 * 800 * 4));
        free(pa);
        free(pb);
    }
    cui_destroy(a.ui);
    cui_destroy(b.ui);
}
int main(void) {
    geometry();
    supported_metrics();
    interactions();
    legacy_time_equivalence();
    printf("PASS: %u Styled DateTime assertions\n", checks);
    return 0;
}
