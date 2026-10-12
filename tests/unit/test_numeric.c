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
    assert(strcmp(localeconv()->decimal_point, "."));
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
    cui_number_result result =
        cui_number_input(ui, 2, theme, cui_string("Quantity"), state, value, options);
    cui_end_frame(ui);
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
    cui_theme theme = cui_theme_light();
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
    cui_pagination(ui, 101, t, page, 100, false, disabled);
    cui_rating(ui, 102, t, cui_string("Quality"), rating,
               (cui_rating_options){.maximum = 5,
                                    .half_steps = true,
                                    .clearable = true,
                                    .disabled = disabled,
                                    .readonly = readonly});
    cui_step steps[] = {
        {.title = cui_string("Details"), .status = CUI_STEP_COMPLETE},
        {.title = cui_string("Payment"),
         .description = cui_string("Choose method"),
         .status = CUI_STEP_ACTIVE},
        {.title = cui_string("Finish"), .status = CUI_STEP_PENDING, .disabled = true}};
    cui_stepper(ui, 103, t, steps, 3, step, false, disabled);
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
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
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
int main(void) {
    decimal_locale();
    models();
    number_events();
    other_events();
    return 0;
}
