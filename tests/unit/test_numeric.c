/* SPDX-License-Identifier: MIT */
#include "cui/cui_numeric.h"
#include <assert.h>
#include <float.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static void decimal_locale(void) {
    const char *requested = getenv("CUI_TEST_NUMERIC_LOCALE");
    const char *locale = setlocale(LC_NUMERIC, requested ? requested : "de_DE.UTF-8");
    if (!locale) {
        assert(!requested);
        return;
    }
    if (!strcmp(localeconv()->decimal_point, ".")) {
        /* Some C runtimes accept locale names without implementing their radix. */
        assert(!requested);
        assert(setlocale(LC_NUMERIC, "C"));
        return;
    }
    double value;
    assert(cui_number_parse(cui_string("1.25e2"), &value) && value == 125);
    assert(!cui_number_parse(cui_string("1,25"), &value));
    char text[96];
    assert(cui_number_format(1.25, text, sizeof(text)) == 4 && !strcmp(text, "1.25"));
    assert(setlocale(LC_NUMERIC, "C"));
}
static double dynamic_step(void *user, double value, int direction) {
    unsigned *calls = user;
    ++*calls;
    assert(direction == 1 || direction == -1);
    return direction < 0 && value <= 1 ? .1 : .5;
}
static void models(void) {
    const char *valid[] = {"0", "-0", "+1", ".5", "1.", "-1.25e+2", "2E-3", "1e308", "5e-324"};
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
        double v = 99;
        assert(cui_number_parse(cui_string(valid[i]), &v) && isfinite(v));
    }
    const char *invalid[] = {"",      " ",      " 1",  "1 ", "NaN", "inf", "0x10", "1,5",
                             "1e999", "1e-999", "--1", ".",  "+",   "1e",  "1e-",  "1.2.3"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        double v = 99;
        assert(!cui_number_parse(cui_string(invalid[i]), &v) && v == 99);
    }
    double values[] = {DBL_MAX, -DBL_MAX, DBL_MIN, DBL_TRUE_MIN, .1, 1. / 3., nextafter(1, 2), -0.};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        char text[96];
        size_t n = cui_number_format(values[i], text, sizeof(text));
        assert(n && n == strlen(text));
        double result;
        assert(cui_number_parse(cui_string(text), &result) && result == values[i]);
        if (values[i] == 0)
            assert(signbit(result) == signbit(values[i]));
    }
    char tiny[2];
    assert(cui_number_format(123, tiny, 2) == 3 && !strcmp(tiny, "1"));
    assert(cui_number_format(NAN, tiny, 2) == 0 && !tiny[0]);
    cui_number_options options = {
        .has_minimum = true, .has_maximum = true, .minimum = 0, .maximum = 10, .step = 3};
    double v = 9;
    assert(cui_number_step(&v, 1, options) && v == 10);
    assert(!cui_number_step(&v, 1, options));
    assert(cui_number_step(&v, -1, options) && v == 7);
    v = -1;
    assert(!cui_number_step(&v, -1, options) && v == -1);
    assert(cui_number_step(&v, 1, options) && v == 2);
    unsigned calls = 0;
    cui_number_options dynamic = {.step_by = dynamic_step, .step_user = &calls};
    v = 1;
    assert(cui_number_step(&v, -1, dynamic) && v == .9 && calls == 1);
    assert(cui_number_step(&v, 1, dynamic) && v == 1.4 && calls == 2);
    options.step = NAN;
    assert(!cui_number_step(&v, 1, options));
    options = (cui_number_options){.step = DBL_MAX};
    v = DBL_MAX;
    assert(!cui_number_step(&v, 1, options) && v == DBL_MAX);
    cui_page_item items[11];
    assert(cui_pagination_items(0, 0, 2, items) == 0);
    assert(cui_pagination_items(99, 1, 3, items) == 1 && items[0].page == 1);
    size_t n = cui_pagination_items(50, 100, 2, items);
    assert(n == 9 && items[0].page == 1 && items[1].ellipsis && items[n - 1].page == 100);
    for (size_t total = 1; total <= 100; ++total)
        for (size_t current = 0; current <= total + 1; ++current)
            for (unsigned radius = 0; radius < 4; ++radius) {
                n = cui_pagination_items(current, total, radius, items);
                assert(n >= 1 && n <= 11);
                size_t previous = 0;
                bool selected = false;
                size_t clamped = current < 1 ? 1 : current > total ? total : current;
                for (size_t i = 0; i < n; ++i) {
                    if (items[i].ellipsis) {
                        assert(i && i + 1 < n && !items[i - 1].ellipsis && !items[i + 1].ellipsis);
                        continue;
                    }
                    assert(items[i].page > previous && items[i].page <= total);
                    previous = items[i].page;
                    if (items[i].page == clamped)
                        selected = true;
                }
                assert(selected && items[0].page == 1 && items[n - 1].page == total);
            }
    n = cui_pagination_items(SIZE_MAX - 1, SIZE_MAX, 3, items);
    assert(n <= 11 && items[n - 1].page == SIZE_MAX);
}
static void key(cui_context *ui, cui_key code, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code, .modifiers = modifiers}));
}
static cui_number_result number_scene(cui_context *ui, const cui_theme *theme,
                                      cui_number_state *state, double *value,
                                      cui_number_options options) {
    assert(cui_begin_frame(ui, 800, 600, .016f));
#ifdef CUI_NUMERIC_BASE_ONLY
    (void)theme;
    cui_number_result result =
        cui_base_number_input(ui, 2, cui_string("Quantity"), state, value, options);
    cui_begin_box(ui, 2, (cui_box){.width = 200, .height = 40});
    cui_end_box(ui);
    for (unsigned i = 1; i <= 2; ++i) {
        cui_begin_box(ui, cui_numeric_part_id(2, i), (cui_box){.width = 40, .height = 40});
        cui_end_box(ui);
    }
    assert(cui_end_frame(ui).count == 0);
#else
    cui_number_result result =
        cui_number_input(ui, 2, theme, cui_string("Quantity"), state, value, options);
    cui_end_frame(ui);
#endif
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void number_events(void) {
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 600});
    assert(ui);
    char buffer[128] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof(buffer),
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 16});
    assert(edit);
    cui_number_state state = {.edit = edit};
#ifdef CUI_NUMERIC_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    double value = 5;
    cui_number_options options = {
        .has_minimum = true, .has_maximum = true, .minimum = 0, .maximum = 10, .step = 2};
    number_scene(ui, &theme, &state, &value, options);
    assert(!strcmp(buffer, "5"));
    cui_focus(ui, 2);
    key(ui, CUI_KEY_UP, 0);
    assert(number_scene(ui, &theme, &state, &value, options).changed && value == 7 &&
           !strcmp(buffer, "7"));
    key(ui, CUI_KEY_A, CUI_MOD_CTRL);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "99"});
    assert(number_scene(ui, &theme, &state, &value, options).changed && value == 99 &&
           !strcmp(buffer, "99"));
    key(ui, CUI_KEY_ENTER, 0);
    cui_number_result result = number_scene(ui, &theme, &state, &value, options);
    assert(result.changed && result.submitted && value == 10 && !strcmp(buffer, "10"));
    key(ui, CUI_KEY_A, CUI_MOD_CTRL);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "abc"});
    result = number_scene(ui, &theme, &state, &value, options);
    assert(result.error == CUI_EDIT_INVALID_ARGUMENT && !result.changed && value == 10 &&
           !strcmp(buffer, "10"));
    key(ui, CUI_KEY_A, CUI_MOD_CTRL);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "-"});
    result = number_scene(ui, &theme, &state, &value, options);
    assert(result.invalid && value == 10 && !strcmp(buffer, "-"));
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "2"});
    result = number_scene(ui, &theme, &state, &value, options);
    assert(!result.invalid && value == -2);
    cui_focus(ui, 0);
    result = number_scene(ui, &theme, &state, &value, options);
    assert(result.changed && value == 0 && !strcmp(buffer, "0"));
    cui_focus(ui, 2);
    key(ui, CUI_KEY_A, CUI_MOD_CTRL);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "１２。５"});
    result = number_scene(ui, &theme, &state, &value, options);
    assert(result.changed && value == 12.5 && !strcmp(buffer, "12.5"));
    options.readonly = true;
    key(ui, CUI_KEY_DOWN, 0);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "0"});
    result = number_scene(ui, &theme, &state, &value, options);
    assert(!result.changed && value == 12.5);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 3 && sem[0].details.readonly && !sem[0].disabled);
    options.readonly = false;
    value = 4;
    result = number_scene(ui, &theme, &state, &value, options);
    assert(!result.changed && !strcmp(buffer, "4"));
    options.disabled = true;
    key(ui, CUI_KEY_DOWN, 0);
    result = number_scene(ui, &theme, &state, &value, options);
    assert(!result.changed && value == 4 && !cui_focused(ui));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void click(cui_context *ui, cui_rect bounds, float position) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = bounds.x + bounds.w * position,
                                           .y = bounds.y + bounds.h * .5f}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = bounds.x + bounds.w * position,
                                           .y = bounds.y + bounds.h * .5f}));
}
static void other_scene(cui_context *ui, const cui_theme *t, size_t *page, float *rating,
                        size_t *step, bool disabled, bool readonly) {
    assert(cui_begin_frame(ui, 1400, 800, .016f));
    cui_begin_box(ui, 100, (cui_box){.width = 1400, .gap = 8});
    cui_step steps[] = {
        {.title = cui_string("Details"), .status = CUI_STEP_COMPLETE},
        {.title = cui_string("Payment"),
         .description = cui_string("Choose method"),
         .status = CUI_STEP_ACTIVE},
        {.title = cui_string("Finish"), .status = CUI_STEP_PENDING, .disabled = true}};
#ifdef CUI_NUMERIC_BASE_ONLY
    (void)t;
    cui_base_pagination(ui, 101, page, 100, false, disabled);
    cui_base_rating(ui, 102, cui_string("Quality"), rating,
                    (cui_rating_options){.maximum = 5,
                                         .half_steps = true,
                                         .clearable = true,
                                         .disabled = disabled,
                                         .readonly = readonly});
    cui_base_stepper(ui, 103, steps, 3, step, disabled);
    size_t semantic_count;
    const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
    for (size_t i = 0; i < semantic_count; ++i) {
        cui_begin_box(ui, semantics[i].id, (cui_box){.width = 180, .height = 24});
        cui_end_box(ui);
    }
#else
    cui_pagination(ui, 101, t, page, 100, false, disabled);
    cui_rating(ui, 102, t, cui_string("Quality"), rating,
               (cui_rating_options){.maximum = 5,
                                    .half_steps = true,
                                    .clearable = true,
                                    .disabled = disabled,
                                    .readonly = readonly});
    cui_stepper(ui, 103, t, steps, 3, step, false, disabled);
#endif
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static cui_id named(cui_context *ui, const char *name) {
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (s[i].name.length == strlen(name) && !memcmp(s[i].name.data, name, strlen(name)))
            return s[i].id;
    assert(false);
    return 0;
}
static void other_events(void) {
#ifdef CUI_NUMERIC_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (size_t i = 0; i < 4; ++i) {
        cui_context *ui = cui_create((cui_config){.width = 1400, .height = 800});
        assert(ui);
        size_t page = 50, step = 0;
        float rating = 2;
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        cui_id next = named(ui, "Next");
        cui_focus(ui, next);
        key(ui, CUI_KEY_ENTER, 0);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(page == 51);
        click(ui, cui_bounds(ui, named(ui, "Last")), .5f);
        click(ui, cui_bounds(ui, named(ui, "Previous")), .5f);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(page == 99);
        click(ui, cui_bounds(ui, named(ui, "Last")), .5f);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(page == 100);
        cui_focus(ui, 102);
        key(ui, CUI_KEY_RIGHT, 0);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(rating == 2.5f);
        click(ui, cui_bounds(ui, named(ui, "4 of 5")), .25f);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(rating == 3.5f && cui_focused(ui) == 102);
        click(ui, cui_bounds(ui, named(ui, "Clear")), .5f);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(rating == 0);
        key(ui, CUI_KEY_END, 0);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(rating == 5);
        key(ui, CUI_KEY_DELETE, 0);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(rating == 0);
        rating = 3;
        click(ui, cui_bounds(ui, named(ui, "1 of 5")), .75f);
        other_scene(ui, &themes[i], &page, &rating, &step, false, true);
        assert(rating == 3);
        cui_id payment = named(ui, "Payment");
        cui_focus(ui, payment);
        key(ui, CUI_KEY_ENTER, 0);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(step == 1);
        click(ui, cui_bounds(ui, named(ui, "Finish")), .5f);
        other_scene(ui, &themes[i], &page, &rating, &step, false, false);
        assert(step == 1);
        key(ui, CUI_KEY_RIGHT, 0);
        other_scene(ui, &themes[i], &page, &rating, &step, true, false);
        assert(page == 100 && rating == 3 && step == 1);
        cui_destroy(ui);
    }
}
#ifndef CUI_NUMERIC_BASE_ONLY
static unsigned provider_calls;
static bool icon_provider(void *user, cui_str path, cui_str *svg) {
    (void)user;
    (void)svg;
    if ((path.length >= 9 && memcmp(path.data + path.length - 9, "minus.svg", 9) == 0) ||
        (path.length >= 8 && memcmp(path.data + path.length - 8, "plus.svg", 8) == 0))
        provider_calls++;
    return false;
}
static cui_draw_list sized_number_frame(cui_context *ui, cui_theme *theme, cui_number_state *state,
                                        double *value, cui_number_options options) {
    assert(cui_begin_frame(ui, 800, 600, .016f));
    cui_number_result result =
        cui_number_input(ui, 2, theme, cui_string("Quantity"), state, value, options);
    assert(result.error == CUI_EDIT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void number_sizes_and_icons(void) {
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 600});
    cui_theme theme = cui_theme_light();
    theme.font_size = 16;
    theme.icon_loader = icon_provider;
    theme.secondary_foreground = (cui_color){13, 71, 191, 255};
    char buffer[128] = "";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
    assert(ui && edit);
    cui_number_state state = {.edit = edit};
    double value = 3;
    const float heights[] = {32, 24, 44, 20}, fonts[] = {14, 14, 16, 12},
                widths[] = {32, 24, 32, 24}, icons[] = {16, 14, 24, 12};
    for (unsigned i = 0; i < 4; i++) {
        cui_number_options options = {.width = 200,
                                      .size = (cui_number_size)i,
                                      .step = 1,
                                      .has_minimum = true,
                                      .minimum = 0,
                                      .has_maximum = true,
                                      .maximum = 10};
        cui_number_metrics metrics;
        assert(cui_number_measure(options, 16, &metrics));
        assert(metrics.height == heights[i] && metrics.font_size == fonts[i] &&
               metrics.step_width == widths[i] && metrics.icon_size == icons[i]);
        cui_draw_list draw = sized_number_frame(ui, &theme, &state, &value, options);
        cui_rect frame = cui_bounds(ui, cui_numeric_part_id(2, 0)),
                 minus = cui_bounds(ui, cui_numeric_part_id(2, 1)),
                 plus = cui_bounds(ui, cui_numeric_part_id(2, 2));
        assert(frame.h == heights[i] && frame.w == 200 && minus.w == widths[i] &&
               plus.w == widths[i]);
        assert(minus.x == 1 && plus.x + plus.w == 199 && minus.h == heights[i] - 2);
        unsigned images = 0;
        bool centered = false;
        for (size_t j = 0; j < draw.count; j++) {
            cui_draw_command c = draw.commands[j];
            if (c.type == CUI_DRAW_IMAGE) {
                images++;
                assert(c.bitmap.custom_tint && c.bitmap.tint.r == 13 && c.bitmap.tint.b == 191);
                assert(c.bounds.w == icons[i] && c.bounds.h == icons[i]);
            }
            if (c.type == CUI_DRAW_TEXT) {
                assert(c.text.length != 8 || memcmp(c.text.data, "Quantity", 8));
                assert(c.text.length != 1 || (c.text.data[0] != '+' && c.text.data[0] != '-'));
                if (c.text.length == 1 && c.text.data[0] == '3') {
                    centered = fabsf(c.bounds.x + c.bounds.w * .5f - 100) < .001f &&
                               fabsf(c.bounds.y +
                                     cui_measure_text(ui, cui_string("3"), fonts[i]).height * .5f -
                                     frame.h * .5f) < .001f;
                    assert(c.font_size == fonts[i]);
                }
            }
        }
        assert(images == 2 && centered);
        key(ui, CUI_KEY_UP, 0);
        cui_focus(ui, 2);
        sized_number_frame(ui, &theme, &state, &value, options);
        assert(value == 4);
        key(ui, CUI_KEY_DOWN, 0);
        sized_number_frame(ui, &theme, &state, &value, options);
        assert(value == 3);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                               .x = minus.x + minus.w / 2,
                                               .y = minus.y + minus.h / 2}));
        draw = sized_number_frame(ui, &theme, &state, &value, options);
        bool hovered = false;
        for (size_t j = 0; j < draw.count; j++) {
            cui_draw_command c = draw.commands[j];
            hovered |= c.type == CUI_DRAW_RECT && c.bounds.w == widths[i] &&
                       c.bounds.h == heights[i] - 2 && c.color.r == theme.input.r &&
                       c.color.a == (uint8_t)lroundf(theme.input.a * .4f);
        }
        assert(hovered);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                               .x = minus.x + minus.w / 2,
                                               .y = minus.y + minus.h / 2}));
        draw = sized_number_frame(ui, &theme, &state, &value, options);
        bool held = false;
        for (size_t j = 0; j < draw.count; j++) {
            cui_draw_command c = draw.commands[j];
            held |= c.type == CUI_DRAW_RECT && c.bounds.w == widths[i] &&
                    c.bounds.h == heights[i] - 2 &&
                    c.color.a == (uint8_t)lroundf(theme.input.a * .6f);
        }
        assert(held);
        assert(cui_focused(ui) == 2);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                               .x = minus.x + minus.w / 2,
                                               .y = minus.y + minus.h / 2}));
        sized_number_frame(ui, &theme, &state, &value, options);
        assert(value == 2 && cui_focused(ui) == 2);
        cui_focus(ui, cui_numeric_part_id(2, 1));
        assert(cui_focused(ui) == 2);
        value = 3;
        options.disabled = true;
        key(ui, CUI_KEY_UP, 0);
        sized_number_frame(ui, &theme, &state, &value, options);
        assert(value == 3);
        options.disabled = false;
    }
    assert(provider_calls >= 2);
    cui_number_options bound = {.step = 1, .has_maximum = true, .maximum = 10};
    value = 10;
    sized_number_frame(ui, &theme, &state, &value, bound);
    size_t semantic_count;
    const cui_semantic *sem = cui_semantics(ui, &semantic_count);
    assert(semantic_count == 3 && sem[1].details.no_focus && sem[2].details.no_focus &&
           !sem[2].disabled);
    assert(sem[1].name.length == 9 && memcmp(sem[1].name.data, "Decrement", 9) == 0);
    assert(sem[2].name.length == 9 && memcmp(sem[2].name.data, "Increment", 9) == 0);
    cui_rect plus = cui_bounds(ui, cui_numeric_part_id(2, 2));
    cui_focus(ui, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = plus.x + plus.w / 2,
                                           .y = plus.y + plus.h / 2}));
    sized_number_frame(ui, &theme, &state, &value, bound);
    assert(cui_focused(ui) == 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = plus.x + plus.w / 2,
                                           .y = plus.y + plus.h / 2}));
    assert(cui_begin_frame(ui, 800, 600, .016f));
    cui_number_result result =
        cui_number_input(ui, 2, &theme, cui_string("Quantity"), &state, &value, bound);
    cui_end_frame(ui);
    assert(!result.changed && result.step_direction == 1 && value == 10 && cui_focused(ui) == 2);
    key(ui, CUI_KEY_DOWN, 0);
    sized_number_frame(ui, &theme, &state, &value, bound);
    assert(value == 9);
    key(ui, CUI_KEY_TAB, 0);
    sized_number_frame(ui, &theme, &state, &value, bound);
    assert(cui_focused(ui) == 2);

    cui_number_metrics custom;
    assert(cui_number_measure((cui_number_options){.size = CUI_NUMBER_CUSTOM, .custom_size = 30},
                              16, &custom) &&
           custom.height == 30 && custom.step_width == 30 && custom.icon_size == 30 &&
           custom.font_size == 26.25f);
    assert(!cui_number_measure((cui_number_options){.size = CUI_NUMBER_CUSTOM}, 16, &custom));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
#endif
#ifndef CUI_NUMERIC_BASE_ONLY
static void stepper_visible_selection(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_theme theme = cui_theme_light();
    size_t selected = 0;
    const cui_step steps[] = {{.title = cui_string("Details"), .status = CUI_STEP_COMPLETE},
                              {.title = cui_string("Review"), .status = CUI_STEP_ACTIVE},
                              {.title = cui_string("Finish"), .status = CUI_STEP_PENDING}};
    assert(ui);
    for (size_t frame = 0; frame < 3; frame++) {
        if (frame)
            click(ui, cui_bounds(ui, named(ui, frame == 1 ? "Finish" : "Details")), .5f);
        assert(cui_begin_frame(ui, 600, 400, .016f));
        cui_stepper(ui, 103, &theme, steps, 3, &selected, false, false);
        cui_draw_list list = cui_end_frame(ui);
        assert(selected == (frame == 1 ? 2 : 0));
        cui_rect active = cui_bounds(ui, named(ui, frame == 1 ? "Finish" : "Details"));
        unsigned current_labels = 0;
        for (size_t i = 0; i < list.count; i++) {
            const cui_draw_command *command = &list.commands[i];
            if (command->type == CUI_DRAW_TEXT && command->text.length == 7 &&
                !memcmp(command->text.data, "Current", 7)) {
                current_labels++;
                assert(command->bounds.x >= active.x && command->bounds.x < active.x + active.w);
            }
        }
        assert(current_labels == 1);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}

static void rating_star_graphics(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    cui_theme theme = cui_theme_light();
    float value = 2.5f;
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_rating(ui, 102, &theme, cui_string("Rating"), &value,
               (cui_rating_options){.half_steps = true});
    cui_draw_list list = cui_end_frame(ui);
    unsigned images = 0, half = 0;
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        assert(c->type != CUI_DRAW_TEXT);
        if (c->type != CUI_DRAW_IMAGE)
            continue;
        images++;
        assert(cui_bitmap_valid(c->bitmap.bitmap));
        assert(c->bounds.h > 15 && c->bounds.h < 25);
        if (c->bitmap.source.w > 0 && c->bitmap.source.w < c->bitmap.bitmap.width) {
            half++;
            assert(fabsf(c->bitmap.source.w * 2 - c->bitmap.bitmap.width) < .01f);
            assert(fabsf(c->bounds.w * 2 - c->bounds.h) < .01f);
        } else
            assert(fabsf(c->bounds.w - c->bounds.h) < .01f);
    }
    assert(images == 8 && half == 1);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
#endif

#ifndef CUI_NUMERIC_BASE_ONLY
static void design_number_sizes(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; ++theme)
        for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; ++size) {
            cui_context *ui = cui_create((cui_config){.width = 1400, .height = 800});
            char buffer[128] = "";
            cui_edit *edit =
                cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
            assert(ui && edit);
            cui_number_state state = {.edit = edit};
            double value = 3;
            cui_number_options o = {.width = 700, .step = 2, .design_size = (cui_design_size)size};
            cui_number_metrics m;
            cui_design_metrics d;
            assert(cui_design_measure(o.design_size, themes[theme].font_size / 16, &d));
            assert(cui_number_measure(o, themes[theme].font_size, &m));
            assert(m.height == d.control_height && m.font_size == d.font_size &&
                   m.step_width == d.control_height && m.icon_size == d.indicator_size);
            cui_draw_list draw = sized_number_frame(ui, &themes[theme], &state, &value, o);
            cui_rect frame = cui_bounds(ui, cui_numeric_part_id(2, 0));
            cui_rect plus = cui_bounds(ui, cui_numeric_part_id(2, 2));
            assert(frame.w == 700 && fabsf(frame.h - d.control_height) < .01f);
            assert(fabsf(plus.w - d.control_height) < .01f && plus.x + plus.w <= frame.x + frame.w);
            unsigned icons = 0, labels = 0;
            for (size_t i = 0; i < draw.count; ++i) {
                if (draw.commands[i].type == CUI_DRAW_IMAGE) {
                    ++icons;
                    assert(fabsf(draw.commands[i].bounds.w - ceilf(d.indicator_size)) < .01f);
                }
                if (draw.commands[i].type == CUI_DRAW_TEXT) {
                    ++labels;
                    assert(draw.commands[i].font_size == d.font_size);
                }
            }
            assert(icons == 2 && labels > 0);
            cui_focus(ui, 2);
            key(ui, CUI_KEY_UP, 0);
            assert(number_scene(ui, &themes[theme], &state, &value, o).changed && value == 5);
            key(ui, CUI_KEY_DOWN, 0);
            assert(number_scene(ui, &themes[theme], &state, &value, o).changed && value == 3);
            key(ui, CUI_KEY_UP, 0);
            cui_number_state saved = state;
            o.design_size = (cui_design_size)99;
            assert(number_scene(ui, &themes[theme], &state, &value, o).error ==
                   CUI_EDIT_INVALID_ARGUMENT);
            assert(value == 3 && !strcmp(buffer, "3") && !memcmp(&state, &saved, sizeof(state)));
            cui_number_metrics unchanged = m;
            assert(!cui_number_measure(o, themes[theme].font_size, &m) &&
                   !memcmp(&m, &unchanged, sizeof(m)));
            cui_destroy(ui);
            cui_edit_destroy(edit);
        }
}
#endif
static void number_composition(void) {
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 600});
    char buffer[128] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof(buffer),
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 16});
    assert(ui && edit);
    cui_number_state state = {.edit = edit};
    cui_theme theme = {0};
#ifndef CUI_NUMERIC_BASE_ONLY
    theme = cui_theme_light();
#endif
    double value = 5;
    cui_number_options options = {0};
    number_scene(ui, &theme, &state, &value, options);
    cui_focus(ui, 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    assert(number_scene(ui, &theme, &state, &value, options).error == CUI_EDIT_OK);
    assert(!state.composition);
    assert(cui_edit_select(edit, 0, 1) == CUI_EDIT_OK);
    cui_event update = {.type = CUI_EVENT_COMPOSITION_UPDATE,
                        .text_data = "５７",
                        .text_length = 6,
                        .composition_anchor = 3,
                        .composition_cursor = 6};
    assert(cui_queue_event(ui, update));
    cui_number_result r = number_scene(ui, &theme, &state, &value, options);
    assert(r.error == CUI_EDIT_OK && !r.changed && value == 5 && !strcmp(buffer, "5"));
    assert(state.composition_owned && state.composition->active);
    assert(cui_edit_composition_view(edit, state.composition).length == 6);
    key(ui, CUI_KEY_UP, 0);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    r = number_scene(ui, &theme, &state, &value, options);
    assert(!r.changed && value == 5 && state.composition->active);
    cui_event commit = {
        .type = CUI_EVENT_COMPOSITION_COMMIT, .text_data = "５７", .text_length = 6};
    assert(cui_queue_event(ui, commit));
    r = number_scene(ui, &theme, &state, &value, options);
    assert(r.changed && r.error == CUI_EDIT_OK && value == 57 && !strcmp(buffer, "57"));
    assert(cui_queue_event(ui, commit));
    assert(!number_scene(ui, &theme, &state, &value, options).changed && value == 57);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    r = number_scene(ui, &theme, &state, &value, options);
    assert(r.changed && value == 5 && !strcmp(buffer, "5"));
    key(ui, CUI_KEY_Y, CUI_MOD_CTRL);
    assert(number_scene(ui, &theme, &state, &value, options).changed && value == 57);
    assert(cui_edit_select(edit, 0, 2) == CUI_EDIT_OK);
    assert(cui_queue_event(ui, update));
    number_scene(ui, &theme, &state, &value, options);
    commit.text_data = "5x";
    commit.text_length = 2;
    assert(cui_queue_event(ui, commit));
    r = number_scene(ui, &theme, &state, &value, options);
    assert(r.error == CUI_EDIT_INVALID_ARGUMENT && state.composition->active && value == 57);
    char oversized[4096];
    memset(oversized, '1', sizeof(oversized));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .text_data = oversized,
                                           .text_length = sizeof(oversized)}));
    assert(number_scene(ui, &theme, &state, &value, options).error == CUI_EDIT_CAPACITY);
    assert(state.composition->active && state.composition->length == 6 && value == 57);
    options.readonly = true;
    number_scene(ui, &theme, &state, &value, options);
    assert(!state.composition->active && value == 57);
    options.readonly = false;
    assert(cui_queue_event(ui, update));
    number_scene(ui, &theme, &state, &value, options);
    value = 12; /* Controlled-source change cancels before accepting a stale commit. */
    commit.text_data = "99";
    commit.text_length = 2;
    assert(cui_queue_event(ui, commit));
    number_scene(ui, &theme, &state, &value, options);
    assert(value == 12 && !strcmp(buffer, "12") && !state.composition->active);
    assert(cui_queue_event(ui, update));
    number_scene(ui, &theme, &state, &value, options);
    cui_focus(ui, 0);
    number_scene(ui, &theme, &state, &value, options);
    assert(!state.composition->active && value == 12);
    cui_focus(ui, 2);
    assert(cui_queue_event(ui, update));
    number_scene(ui, &theme, &state, &value, options);
    options.disabled = true;
    number_scene(ui, &theme, &state, &value, options);
    assert(!state.composition->active && value == 12);
    options.disabled = false;
    cui_focus(ui, 2);
    assert(cui_queue_event(ui, update));
    number_scene(ui, &theme, &state, &value, options);
    char replacement[128] = "12";
    cui_edit_destroy(edit);
    edit = cui_edit_create((cui_edit_config){.buffer = replacement,
                                             .capacity = sizeof(replacement),
                                             .history_bytes = 1024,
                                             .max_history_steps = 4});
    assert(edit);
    state.edit = edit;
    assert(cui_queue_event(ui, commit));
    number_scene(ui, &theme, &state, &value, options);
    assert(!state.composition->active && !strcmp(replacement, "12") && value == 12);
    update.composition_target = 99; /* Stale host focus must not create a session. */
    assert(cui_queue_event(ui, update));
    number_scene(ui, &theme, &state, &value, options);
    assert(!state.composition->active && value == 12);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
#ifndef CUI_NUMERIC_BASE_ONLY
static void pagination_counter_center(void) {
    for (unsigned compact = 0; compact < 2; compact++) {
        for (unsigned density = 0; density < 2; density++) {
            cui_context *ui = cui_create((cui_config){.width = 900, .height = 160});
            assert(ui);
            cui_theme theme = cui_theme_light();
            theme.control_height = density ? 28 : 40;
            theme.font_size = density ? 13 : 16;
            size_t page = 3;
            assert(cui_begin_frame(ui, 900, 160, .016f));
            cui_pagination(ui, 71, &theme, &page, 20, compact != 0, false);
            cui_draw_list draw = cui_end_frame(ui);
            cui_rect button = cui_bounds(ui, cui_numeric_part_id(71, 1));
            assert(button.h > 0);
            bool found = false;
            const char *label = compact ? "3 / 20" : "...";
            for (size_t i = 0; i < draw.count; i++) {
                cui_draw_command c = draw.commands[i];
                if (c.type == CUI_DRAW_TEXT && c.text.length == strlen(label) &&
                    !memcmp(c.text.data, label, c.text.length)) {
                    assert(fabsf(c.bounds.y + c.bounds.h / 2 - button.y - button.h / 2) < .01f);
                    found = true;
                }
            }
            assert(found && cui_last_error(ui) == CUI_OK);
            cui_destroy(ui);
        }
    }
}
#endif
int main(void) {
    number_composition();
#ifndef CUI_NUMERIC_BASE_ONLY
    pagination_counter_center();
    design_number_sizes();
    number_sizes_and_icons();
    rating_star_graphics();
    stepper_visible_selection();
#endif
    decimal_locale();
    models();
    number_events();
    other_events();
    return 0;
}
