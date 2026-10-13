/* SPDX-License-Identifier: MIT */
#include "cui/cui_picker.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static void key(cui_context *ui, cui_key code, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code, .modifiers = modifiers}));
}
static void click(cui_context *ui, cui_rect bounds) {
    assert(bounds.w > 0 && bounds.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2}));
}
#ifndef CUI_PICKER_BASE_ONLY
static cui_id named(cui_context *ui, const char *name) {
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (s[i].name.length == strlen(name) && !memcmp(s[i].name.data, name, strlen(name)))
            return s[i].id;
    assert(false);
    return 0;
}
#endif
static bool weekends(void *user, cui_date date) {
    (void)user;
    int day = cui_date_weekday(date);
    return day == 0 || day == 6;
}
static void models(void) {
    cui_date grid[42];
    cui_calendar_grid((cui_date){2024, 2, 1}, 1, grid);
    assert(!cui_date_compare(grid[0], (cui_date){2024, 1, 29}));
    assert(!cui_date_compare(grid[31], (cui_date){2024, 2, 29}));
    assert(!cui_date_compare(grid[41], (cui_date){2024, 3, 10}));
    cui_calendar_grid((cui_date){1, 1, 1}, 0, grid);
    assert(!cui_date_valid(grid[0]) && !cui_date_compare(grid[1], (cui_date){1, 1, 1}));
    for (int year = 1990; year < 2030; ++year)
        for (int month = 1; month <= 12; ++month)
            for (unsigned first = 0; first < 7; ++first) {
                cui_calendar_grid((cui_date){year, month, 1}, first, grid);
                assert((unsigned)cui_date_weekday(grid[0]) == first);
                unsigned inside = 0;
                for (unsigned i = 0; i < 42; ++i) {
                    if (i)
                        assert(!cui_date_compare(grid[i], cui_date_add_days(grid[i - 1], 1)));
                    if (grid[i].month == month)
                        ++inside;
                }
                assert(inside == (unsigned)cui_date_days_in_month(year, month));
            }
    cui_calendar_options options = {
        .minimum = {2024, 2, 5}, .maximum = {2024, 2, 20}, .is_disabled = weekends};
    assert(cui_calendar_date_enabled((cui_date){2024, 2, 5}, options));
    assert(!cui_calendar_date_enabled((cui_date){2024, 2, 3}, options));
    assert(!cui_calendar_date_enabled((cui_date){2024, 2, 21}, options));
    cui_time time = {9, 8, 7}, copy = time;
    assert(cui_time_parse(cui_string("23:59:59"), &time) && time.hour == 23 && time.second == 59);
    const char *invalid[] = {"24:00", "23:60", "9:00", "09:00 ", "09.00", "99:99:99", "09:00:60"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(!cui_time_parse(cui_string(invalid[i]), &copy) && copy.hour == 9);
    char text[9];
    assert(cui_time_format(time, true, text, sizeof(text)) == 8 && !strcmp(text, "23:59:59"));
    assert(cui_time_format(time, false, text, 4) == 5 && !strcmp(text, "23:"));
}
static cui_calendar_result calendar_scene(cui_context *ui, const cui_theme *theme,
                                          cui_calendar_state *state, cui_date_selection *selection,
                                          cui_calendar_options options) {
    assert(cui_begin_frame(ui, 1000, 600, .016f));
#ifdef CUI_PICKER_BASE_ONLY
    (void)theme;
    cui_calendar_result result = cui_base_calendar(ui, 10, state, selection, options);
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        cui_begin_box(ui, s[i].id, (cui_box){.width = 32, .height = 24});
        cui_end_box(ui);
    }
    assert(cui_end_frame(ui).count == 0);
#else
    cui_calendar_result result = cui_calendar(ui, 10, theme, state, selection, options);
    cui_end_frame(ui);
#endif
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void calendar_events(void) {
#ifdef CUI_PICKER_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (size_t i = 0; i < 4; ++i) {
        cui_context *ui = cui_create((cui_config){.width = 1000, .height = 600});
        assert(ui);
        cui_calendar_state state;
        cui_calendar_init(&state, (cui_date){2024, 2, 28});
        cui_date_selection selected = {.start = {2024, 2, 28}};
        cui_calendar_options options = {.first_weekday = 1, .is_disabled = weekends};
        calendar_scene(ui, &themes[i], &state, &selected, options);
        cui_focus(ui, cui_calendar_day_id(10, state.cursor));
        key(ui, CUI_KEY_RIGHT, 0);
        key(ui, CUI_KEY_RIGHT, 0);
        assert(calendar_scene(ui, &themes[i], &state, &selected, options).navigated);
        assert(!cui_date_compare(state.cursor, (cui_date){2024, 3, 1}) && state.month.month == 3);
        key(ui, CUI_KEY_ENTER, 0);
        assert(calendar_scene(ui, &themes[i], &state, &selected, options).changed);
        assert(!cui_date_compare(selected.start, (cui_date){2024, 3, 1}));
        key(ui, CUI_KEY_RIGHT, 0);
        key(ui, CUI_KEY_ENTER, 0);
        cui_calendar_result ordered = calendar_scene(ui, &themes[i], &state, &selected, options);
        assert(ordered.changed && ordered.navigated && ordered.activated);
        assert(!cui_date_compare(state.cursor, (cui_date){2024, 3, 4}));
        assert(!cui_date_compare(selected.start, (cui_date){2024, 3, 4}));
        key(ui, CUI_KEY_RIGHT, 0);
        key(ui, CUI_KEY_RIGHT, 0);
        key(ui, CUI_KEY_SPACE, 0);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
        calendar_scene(ui, &themes[i], &state, &selected, options);
        assert(!cui_date_compare(selected.start, (cui_date){2024, 3, 6}));
        click(ui, cui_bounds(ui, cui_calendar_navigation_id(10, 1)));
        calendar_scene(ui, &themes[i], &state, &selected, options);
        assert(state.month.year == 2023 && state.month.month == 3);
        options.range = true;
        options.months = 2;
        options.is_disabled = NULL;
        calendar_scene(ui, &themes[i], &state, &selected, options);
        click(ui, cui_bounds(ui, cui_calendar_day_id(10, (cui_date){2023, 3, 20})));
        assert(calendar_scene(ui, &themes[i], &state, &selected, options).changed &&
               state.selecting_end);
        click(ui, cui_bounds(ui, cui_calendar_day_id(10, (cui_date){2023, 3, 10})));
        assert(calendar_scene(ui, &themes[i], &state, &selected, options).changed &&
               !state.selecting_end);
        assert(selected.start.day == 10 && selected.end.day == 20);
        options.disabled = true;
        click(ui, cui_bounds(ui, cui_calendar_day_id(10, (cui_date){2023, 3, 15})));
        calendar_scene(ui, &themes[i], &state, &selected, options);
        assert(selected.start.day == 10 && selected.end.day == 20);
        cui_destroy(ui);
    }
}
static bool time_scene(cui_context *ui, const cui_theme *theme, cui_time_state *state,
                       cui_time *time, cui_time_options options) {
    assert(cui_begin_frame(ui, 600, 300, .016f));
#ifdef CUI_PICKER_BASE_ONLY
    (void)theme;
    bool changed = cui_base_time_field(ui, 20, cui_string("Time"), state, time, options);
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        cui_begin_box(ui, s[i].id, (cui_box){.width = 60, .height = 24});
        cui_end_box(ui);
    }
    assert(cui_end_frame(ui).count == 0);
#else
    bool changed = cui_time_field(ui, 20, theme, cui_string("Time"), state, time, options);
    cui_end_frame(ui);
#endif
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void time_events(void) {
#ifdef CUI_PICKER_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 300});
    assert(ui);
    cui_time_state state = {0};
    cui_time time = {23, 59, 59};
    cui_time_options options = {.seconds = true};
    time_scene(ui, &theme, &state, &time, options);
    cui_focus(ui, cui_time_segment_id(20, 0));
    key(ui, CUI_KEY_UP, 0);
    assert(time_scene(ui, &theme, &state, &time, options) && time.hour == 0);
    key(ui, CUI_KEY_RIGHT, 0);
    key(ui, CUI_KEY_UP, 0);
    assert(time_scene(ui, &theme, &state, &time, options) && time.minute == 0 &&
           state.segment == 1);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "1"});
    time_scene(ui, &theme, &state, &time, options);
    assert(time.minute == 1 && state.digit_count == 1);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "2"});
    time_scene(ui, &theme, &state, &time, options);
    assert(time.minute == 12 && state.segment == 2);
    key(ui, CUI_KEY_HOME, 0);
    assert(time_scene(ui, &theme, &state, &time, options) && time.second == 0);
    options.twelve_hour = true;
    options.seconds = false;
    time_scene(ui, &theme, &state, &time, options);
    cui_focus(ui, cui_time_segment_id(20, 3));
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "p"});
    assert(time_scene(ui, &theme, &state, &time, options) && time.hour == 12);
    cui_focus(ui, cui_time_segment_id(20, 0));
    key(ui, CUI_KEY_HOME, 0);
    assert(time_scene(ui, &theme, &state, &time, options) && time.hour == 13);
    key(ui, CUI_KEY_END, 0);
    assert(time_scene(ui, &theme, &state, &time, options) && time.hour == 12);
    size_t semantic_count;
    const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
    for (size_t i = 0; i < semantic_count; ++i)
        if (semantics[i].id == cui_time_segment_id(20, 0))
            assert(semantics[i].value == 12 && semantics[i].details.minimum == 1 &&
                   semantics[i].details.maximum == 12);
    options.readonly = true;
    key(ui, CUI_KEY_DOWN, 0);
    assert(!time_scene(ui, &theme, &state, &time, options) && time.hour == 12);
    options.readonly = false;
    options.disabled = true;
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "a"});
    assert(!time_scene(ui, &theme, &state, &time, options) && time.hour == 12 && !cui_focused(ui));
    cui_destroy(ui);
}
#ifndef CUI_PICKER_BASE_ONLY
static void calendar_presentation(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 600});
    cui_theme theme = cui_theme_light();
    cui_calendar_state state = {.month = {2024, 2, 1}, .cursor = {2024, 2, 1}};
    cui_date_selection selected = {.start = {2024, 2, 1}};
    const float sizes[] = {20, 32, 48};
    assert(ui);
    for (size_t n = 0; n < 3; n++) {
        assert(cui_begin_frame(ui, 600, 600, .016f));
        cui_calendar(ui, 10, &theme, &state, &selected,
                     (cui_calendar_options){.cell_size = sizes[n]});
        cui_draw_list list = cui_end_frame(ui);
        cui_rect day = cui_bounds(ui, cui_calendar_day_id(10, selected.start));
        unsigned images = 0, centered = 0;
        for (size_t i = 0; i < list.count; i++) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_IMAGE) {
                images++;
                assert(cui_bitmap_valid(c->bitmap.bitmap));
            }
            if (c->type == CUI_DRAW_TEXT && c->text.length == 1 && c->text.data[0] == '1') {
                centered++;
                assert(fabsf(c->bounds.x + c->bounds.w / 2 - day.x - day.w / 2) < .01f);
                assert(fabsf(c->bounds.y + c->bounds.h / 2 - day.y - day.h / 2) < .01f);
            }
        }
        assert(images == 6 && centered == 1);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}

static void date_picker_presentation(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 600});
    cui_theme theme = cui_theme_light();
    cui_date_picker_state state = {0};
    cui_date_selection selected = {.start = {2024, 2, 1}};
    assert(ui);
    cui_date_picker_init(&state, 21, selected.start);
    for (unsigned frame = 0; frame < 2; frame++) {
        if (frame)
            click(ui, cui_bounds(ui, 20));
        assert(cui_begin_frame(ui, 600, 600, .016f));
        cui_date_picker(ui, 20, &theme, cui_string("Date"), &state, &selected,
                        (cui_date_picker_options){.viewport = {0, 0, 600, 600}});
        cui_draw_list list = cui_end_frame(ui);
        cui_rect trigger = cui_bounds(ui, 20);
        assert(trigger.h == 32);
        unsigned images = 0, labels = 0;
        for (size_t i = 0; i < list.count; i++) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_IMAGE)
                images++;
            if (c->type == CUI_DRAW_TEXT && c->text.length == 10 &&
                !memcmp(c->text.data, "2024-02-01", 10)) {
                labels++;
                assert(fabsf(c->bounds.y + c->bounds.h / 2 - trigger.y - trigger.h / 2) < .5f);
            }
        }
        assert(images == (frame ? 7u : 1u) && labels == 1);
        assert(state.overlay.open == (frame != 0));
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}

static bool picker_scene(cui_context *ui, cui_date_picker_state *state,
                         cui_date_selection *selection, bool disabled) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 700, 600, .016f));
    cui_begin_box(ui, 30, (cui_box){.height = 48, .width = 700, .clip = true});
    bool changed = cui_date_picker(
        ui, 31, &theme, cui_string("Date"), state, selection,
        (cui_date_picker_options){.viewport = {0, 0, 700, 600}, .disabled = disabled});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void picker_events(void) {
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 600});
    assert(ui);
    cui_date_picker_state state;
    cui_date_picker_init(&state, 32, (cui_date){2024, 2, 15});
    cui_date_selection selection = {.start = {2024, 2, 15}};
    picker_scene(ui, &state, &selection, false);
    click(ui, cui_bounds(ui, 31));
    picker_scene(ui, &state, &selection, false);
    assert(state.overlay.open && cui_focused(ui) != 31);
    cui_rect day = cui_bounds(ui, named(ui, "2024-02-20"));
    assert(day.y >= 48);
    click(ui, day);
    assert(picker_scene(ui, &state, &selection, false));
    assert(!state.overlay.open && selection.start.day == 20 && cui_focused(ui) == 31);
    key(ui, CUI_KEY_ENTER, 0);
    picker_scene(ui, &state, &selection, false);
    assert(state.overlay.open);
    for (unsigned i = 0; i < 17; ++i)
        key(ui, CUI_KEY_TAB, 0);
    picker_scene(ui, &state, &selection, false);
    assert(cui_focused(ui) != 31);
    key(ui, CUI_KEY_ESCAPE, 0);
    picker_scene(ui, &state, &selection, false);
    assert(!state.overlay.open && cui_focused(ui) == 31);
    key(ui, CUI_KEY_ENTER, 0);
    picker_scene(ui, &state, &selection, false);
    assert(state.overlay.open);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 650, .y = 500});
    picker_scene(ui, &state, &selection, false);
    assert(!state.overlay.open && cui_focused(ui) == 31);
    key(ui, CUI_KEY_ENTER, 0);
    picker_scene(ui, &state, &selection, true);
    assert(!state.overlay.open);
    cui_destroy(ui);
}
#endif
static bool equal_color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void color_models(void) {
    cui_color red = {255, 0, 0, 255}, color;
    assert(equal_color(cui_color_from_hsv((cui_hsv){0, 1, 1, 1}), red));
    assert(equal_color(cui_color_from_hsv((cui_hsv){360, 1, 1, 1}), red));
    assert(equal_color(cui_color_from_hsv((cui_hsv){120, 1, 1, 1}), (cui_color){0, 255, 0, 255}));
    assert(equal_color(cui_color_from_hsv((cui_hsv){-120, 1, 1, 1}), (cui_color){0, 0, 255, 255}));
    assert(equal_color(cui_color_from_hsv((cui_hsv){NAN, NAN, INFINITY, NAN}),
                       (cui_color){0, 0, 0, 0}));
    uint32_t random = 91827;
    for (unsigned i = 0; i < 20000; ++i) {
        random = random * 1664525u + 1013904223u;
        color = (cui_color){(uint8_t)random, (uint8_t)(random >> 8), (uint8_t)(random >> 16),
                            (uint8_t)(random >> 24)};
        assert(equal_color(cui_color_from_hsv(cui_color_to_hsv(color)), color));
        char hex[10];
        assert(cui_color_format_hex(color, true, hex, sizeof(hex)) == 9);
        cui_color parsed;
        assert(cui_color_parse_hex(cui_string(hex), &parsed) && equal_color(parsed, color));
    }
    assert(cui_color_parse_hex(cui_string("#12aBCd"), &color) &&
           equal_color(color, (cui_color){18, 171, 205, 255}));
    assert(!cui_color_parse_hex(cui_string("#123"), &color));
    assert(!cui_color_parse_hex(cui_string("#GGFFFF"), &color));
    assert(!cui_color_parse_hex(cui_string("123456"), &color));
}
static bool color_scene(cui_context *ui, const cui_theme *t, cui_color_state *state,
                        cui_color *color, cui_color_picker_options options) {
    assert(cui_begin_frame(ui, 800, 800, .016f));
#ifdef CUI_PICKER_BASE_ONLY
    (void)t;
    bool changed = cui_base_color_picker(ui, 50, state, color, options);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        cui_begin_box(ui, sem[i].id, (cui_box){.width = 240, .height = 100});
        cui_end_box(ui);
    }
    assert(cui_end_frame(ui).count == 0);
#else
    bool changed = cui_color_picker(ui, 50, t, state, color, options);
    cui_draw_list list = cui_end_frame(ui);
    bool white = false, black = false;
    for (size_t i = 0; i < list.count; ++i) {
        if (list.commands[i].type != CUI_DRAW_RECT)
            continue;
        cui_color c = list.commands[i].color;
        if (equal_color(c, (cui_color){255, 255, 255, 255}))
            white = true;
        if (equal_color(c, (cui_color){0, 0, 0, 255}))
            black = true;
    }
    assert(white && black);
    cui_rect plane = cui_bounds(ui, cui_color_part_id(50, 2));
    assert(plane.w == 240 && plane.h == 144);
#endif
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void color_events(void) {
#ifdef CUI_PICKER_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (unsigned i = 0; i < 4; ++i) {
        cui_context *ui = cui_create((cui_config){.width = 800, .height = 800});
        assert(ui);
        cui_color color = {255, 0, 0, 255}, palette[] = {{0, 0, 255, 64}, {0, 255, 0, 255}};
        cui_color_state state = {0};
        cui_color_picker_options options = {.alpha = true, .palette = palette, .palette_count = 2};
        color_scene(ui, &themes[i], &state, &color, options);
        /* Async host screen samples replace RGB while preserving alpha. */
        color.a = 73;
        cui_color sample;
        assert(cui_color_parse_hex(cui_string("#1A8F64"), &sample));
        sample.a = color.a;
        color = sample;
        cui_invalidate(ui);
        assert(!color_scene(ui, &themes[i], &state, &color, options));
        assert(equal_color(color, sample) && equal_color(state.last_color, sample));
        cui_hsv expected = cui_color_to_hsv(sample);
        assert(fabsf(state.hsv.hue - expected.hue) < .001f &&
               fabsf(state.hsv.saturation - expected.saturation) < .001f &&
               fabsf(state.hsv.value - expected.value) < .001f);
        color = (cui_color){255, 0, 0, 255};
        color_scene(ui, &themes[i], &state, &color, options);
        cui_focus(ui, cui_color_part_id(50, 20));
        key(ui, CUI_KEY_DOWN, 0);
        assert(color_scene(ui, &themes[i], &state, &color, options) && color.r == 254);
        click(ui, cui_bounds(ui, cui_color_part_id(50, 100)));
        assert(color_scene(ui, &themes[i], &state, &color, options) &&
               equal_color(color, palette[0]));
        cui_focus(ui, cui_color_part_id(50, 3));
        key(ui, CUI_KEY_END, 0);
        assert(color_scene(ui, &themes[i], &state, &color, options) && color.a == 255);
        cui_rect plane = cui_bounds(ui, cui_color_part_id(50, 2));
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                        .x = plane.x + plane.w * .5f,
                                        .y = plane.y + plane.h * .5f});
        assert(color_scene(ui, &themes[i], &state, &color, options));
        assert(fabsf(state.hsv.saturation - .5f) < .001f && fabsf(state.hsv.value - .5f) < .001f);
        cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = plane.x + plane.w, .y = plane.y});
        assert(color_scene(ui, &themes[i], &state, &color, options) && state.hsv.saturation == 1 &&
               state.hsv.value == 1 && !state.dragging);
        cui_color before = color;
        options.readonly = true;
        key(ui, CUI_KEY_HOME, 0);
        assert(!color_scene(ui, &themes[i], &state, &color, options) && equal_color(color, before));
        options.readonly = false;
        options.disabled = true;
        click(ui, cui_bounds(ui, cui_color_part_id(50, 101)));
        assert(!color_scene(ui, &themes[i], &state, &color, options) && equal_color(color, before));
        cui_destroy(ui);
    }
}

int main(void) {
    color_models();
    color_events();
    models();
    calendar_events();
    time_events();
#ifndef CUI_PICKER_BASE_ONLY
    picker_events();
    calendar_presentation();
    date_picker_presentation();
#endif
    return 0;
}
