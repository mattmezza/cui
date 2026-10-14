/* SPDX-License-Identifier: MIT */
#include "../../src/core/cui_unicode_grapheme.h"
#include "cui/cui.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef CUI_TEST_WRAP_RENDERER
#include "cui/cui_software.h"
#include "cui/cui_text.h"
#endif
#include "cui_edit_grapheme_tests.inc"
static unsigned measures;
static cui_size measured(void *user, cui_str text, float size, cui_text_style style) {
    (void)user;
    (void)size;
    (void)style;
    ++measures;
    size_t n = 0, offset = 0;
    while (offset < text.length) {
        size_t next = cui_core_grapheme_next_valid(text, offset);
        assert(next != SIZE_MAX && next > offset);
        offset = next;
        ++n;
    }
    return (cui_size){(float)n * 10, 12};
}
static bool boundary(cui_str text, size_t target) {
    size_t offset = 0;
    while (offset < target)
        offset = cui_core_grapheme_next_valid(text, offset);
    return offset == target;
}
static void conformance(void) {
    for (size_t i = 0; i < sizeof(grapheme_cases) / sizeof(grapheme_cases[0]); ++i) {
        cui_str text = {grapheme_cases[i].text, grapheme_cases[i].length};
        size_t offset = 0;
        for (size_t j = 1; j < grapheme_cases[i].count; ++j) {
            offset = cui_core_grapheme_next_valid(text, offset);
            assert(offset == grapheme_cases[i].offsets[j]);
        }
        assert(offset == text.length);
    }
}
static void rows(const char *text, float width, const char *const *expected, size_t count) {
    cui_context *ui = cui_create(
        (cui_config){.measure_styled = measured, .max_elements = 128, .frame_bytes = 32768});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 1000, 0));
    cui_begin_box(ui, 1, (cui_box){.width = width});
    cui_text_color_span span = {0, strlen(text), {7, 8, 9, 255}};
    assert(cui_text_paragraph_colored(ui, cui_string(text), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, &span, 1));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == count && cui_bounds(ui, 1).h == (float)count * 20);
    size_t end = 0;
    for (size_t i = 0; i < count; ++i) {
        const cui_draw_command *d = &list.commands[i];
        assert(d->type == CUI_DRAW_TEXT && d->text.length == strlen(expected[i]));
        assert(!memcmp(d->text.data, expected[i], d->text.length));
        assert(d->text_source.paragraph.length == strlen(text));
        assert(!memcmp(d->text_source.paragraph.data, text, strlen(text)));
        assert(d->text_source.byte_start == end && d->text_source.byte_end == end + d->text.length);
        assert(boundary(cui_string(text), d->text_source.byte_start) &&
               boundary(cui_string(text), d->text_source.byte_end));
        assert(d->text_color_count == 1 && d->text_colors[0].byte_start == end &&
               d->text_colors[0].byte_end == end + d->text.length &&
               d->text_colors[0].color.r == 7);
        end += d->text.length;
    }
    assert(end == strlen(text));
    cui_destroy(ui);
}
#define ROWS(text, width, ...)                                                                     \
    do {                                                                                           \
        const char *const expected[] = {__VA_ARGS__};                                              \
        rows(text, width, expected, sizeof(expected) / sizeof(expected[0]));                       \
    } while (0)
static void source_cases(void) {
    ROWS("abcd", 20, "ab", "cd");
    ROWS("AA BB CC", 50, "AA ", "BB CC");
    ROWS("AA BB CC", 60, "AA BB ", "CC");
    ROWS("你好世界", 20, "你好", "世界");
    ROWS("Hello世界", 30, "Hel", "lo世", "界");
    ROWS("ab/cd", 30, "ab", "/cd");
    ROWS("ab!cd", 30, "ab!", "cd");
    ROWS("a\xc2\xa0"
         "bc",
         30,
         "a\xc2\xa0"
         "b",
         "c");
    ROWS("AA\tBB", 30, "AA", "\tBB");
    ROWS("AA\xe2\x80\x83"
         "BB",
         30, "AA",
         "\xe2\x80\x83"
         "BB");
    ROWS("  AA BB", 30, "  A", "A ", "BB");
    ROWS("    ", 20, "  ", "  ");
    ROWS("👩🏽‍🚀👨‍👩‍👧‍👦🇨🇭ä", 20,
         "👩🏽‍🚀👨‍👩‍👧‍👦", "🇨🇭ä");
    ROWS("👩🏽‍🚀🇨🇭ä", 1, "👩🏽‍🚀", "🇨🇭", "ä");
    ROWS("क्षक्ष", 10, "क्ष", "क्ष");
    ROWS("각각", 10, "각", "각");
    ROWS("אבגדה", 20, "אב", "גד", "ה");
    ROWS("a אב b", 30, "a א", "ב b");
}
static void bounded(void) {
    cui_context *ui = cui_create(
        (cui_config){.measure_styled = measured, .max_elements = 16, .frame_bytes = 256});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 1000, 0));
    char too_big[64];
    memset(too_big, 'a', sizeof(too_big));
    assert(!cui_text_paragraph_colored(ui, (cui_str){too_big, sizeof(too_big)}, 16, (cui_color){0},
                                       20, (cui_text_style){0}, true, NULL, 0));
    assert(cui_text_paragraph_colored(ui, cui_string("ok"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, NULL, 0));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 2);
    assert(cui_begin_frame(ui, 200, 1000, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 1});
    assert(cui_text_paragraph_colored(ui, cui_string("abcdef"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, false, NULL, 0));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 6);
    assert(cui_begin_frame(ui, 200, 1000, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 1});
    /* Ordinary legacy text retains the pinned Clay word policy. */
    cui_text(ui, cui_string("abcdef"), 16, (cui_color){1, 2, 3, 255});
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 6);
    cui_destroy(ui);
    ui = cui_create(
        (cui_config){.measure_styled = measured, .max_elements = 4, .frame_bytes = 4096});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 1000, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 1});
    assert(cui_text_paragraph_colored(ui, (cui_str){too_big, sizeof(too_big)}, 16, (cui_color){0},
                                      20, (cui_text_style){0}, true, NULL, 0));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    assert(cui_begin_frame(ui, 200, 1000, 0));
    assert(cui_text_paragraph_colored(ui, cui_string("ok"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, NULL, 0));
    list = cui_end_frame(ui);
    assert(list.count == 1); /* capacity failure doesn't poison next frame */
    cui_destroy(ui);
    char long_word[1024];
    memset(long_word, 'a', sizeof(long_word));
    ui = cui_create(
        (cui_config){.measure_styled = measured, .max_elements = 4096, .frame_bytes = 131072});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 15000, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 20});
    measures = 0;
    assert(cui_text_paragraph_colored(ui, (cui_str){long_word, sizeof(long_word)}, 16,
                                      (cui_color){0}, 20, (cui_text_style){0}, true, NULL, 0));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 512 && measures < 1100);
    cui_destroy(ui);
}
static cui_rect painted_bounds;
static bool painted(const cui_prepaint_info *info, const void *payload,
                    cui_prepaint_writer *writer) {
    (void)payload;
    (void)writer;
    painted_bounds = info->bounds;
    return true;
}
static void prepaint_layout(void) {
    cui_context *ui = cui_create(
        (cui_config){.measure_styled = measured, .max_elements = 128, .frame_bytes = 32768});
    assert(ui);
    cui_rect retained = {0};
    for (unsigned frame = 0; frame < 2; ++frame) {
        assert(cui_begin_frame(ui, 200, 200, 0));
        cui_begin_box(ui, 78, (cui_box){.width = 200, .height = 100, .clip = true});
        cui_begin_box(ui, 77, (cui_box){.width = frame ? 10 : 20});
        assert(cui_track_bounds(ui, 77, &retained));
        assert(
            cui_set_prepaint(ui, (cui_prepaint_options){.callback = painted, .command_budget = 2}));
        assert(cui_text_paragraph_colored(ui, cui_string("ä🇨🇭👩🏽‍🚀"), 16,
                                          (cui_color){1, 2, 3, 255}, 20, (cui_text_style){0}, true,
                                          NULL, 0));
        cui_end_box(ui);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        size_t texts = 0;
        for (size_t i = 0; i < list.count; ++i)
            texts += list.commands[i].type == CUI_DRAW_TEXT;
        assert(texts == (frame ? 3u : 2u));
        assert(retained.h == (frame ? 60 : 40) && retained.w == (frame ? 10 : 20));
        assert(painted_bounds.h == retained.h && painted_bounds.w == retained.w);
    }
    cui_destroy(ui);
}
static cui_text_layout_status fake_carets(const void *s, size_t b, cui_text_caret *o, size_t c,
                                          size_t *n) {
    (void)s;
    (void)b;
    (void)o;
    (void)c;
    (void)n;
    return CUI_TEXT_LAYOUT_UNAVAILABLE;
}
static cui_text_layout_status fake_hit(const void *s, float x, float y, cui_text_affinity a,
                                       cui_text_caret *o) {
    (void)s;
    (void)x;
    (void)y;
    (void)a;
    (void)o;
    return CUI_TEXT_LAYOUT_UNAVAILABLE;
}
static cui_text_layout_status fake_selection(const void *s, size_t a, size_t b,
                                             cui_text_selection_rect *out, size_t capacity,
                                             size_t *count) {
    (void)s;
    assert(a == 0 && b == 3);
    if (!out && !capacity) {
        *count = 2;
        return CUI_TEXT_LAYOUT_OK;
    }
    if (capacity < 2)
        return CUI_TEXT_LAYOUT_CAPACITY;
    out[0] = (cui_text_selection_rect){{0, 0, 15, 12}, 0, 2, 0};
    out[1] = (cui_text_selection_rect){{15, 0, 10, 12}, 2, 3, 0};
    *count = 2;
    return CUI_TEXT_LAYOUT_OK;
}
static void fake_release(void *session) {
    free(session);
}
static cui_text_layout_status shaped(void *user, cui_text_line_source source, float size,
                                     cui_text_style style, cui_text_geometry *out) {
    cui_context *ui = *(cui_context **)user;
    assert(!cui_frame_is_submitting(ui));
    (void)size;
    (void)style;
    assert(source.paragraph.length == 3);
    static const cui_text_geometry_ops ops = {fake_carets, fake_hit, fake_selection, fake_release};
    void *session = malloc(1);
    assert(session);
    *out = (cui_text_geometry){session, &ops, 25, 12, 3};
    return CUI_TEXT_LAYOUT_OK;
}
static void shaped_cluster(void) {
    cui_context *ui = NULL;
    ui = cui_create((cui_config){.measure_styled = measured,
                                 .measure_user = &ui,
                                 .text_geometry_line = shaped,
                                 .max_elements = 128,
                                 .frame_bytes = 32768});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 10});
    assert(cui_text_paragraph_colored(ui, cui_string("fiX"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, NULL, 0));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 2);
    assert(list.commands[0].text.length == 2 && !memcmp(list.commands[0].text.data, "fi", 2));
    assert(list.commands[0].bounds.w == 15 && list.commands[1].bounds.w == 10);
    assert(list.commands[1].text_source.byte_start == 2);
    cui_destroy(ui);
}

#ifdef CUI_TEST_WRAP_RENDERER
static cui_size font_measure(void *user, cui_str text, float size, cui_text_style style) {
    return cui_font_measure_styled(user, text, size, style);
}
static void rendered(const char *font_path, const char *rtl_path) {
    cui_font *font = cui_font_open(font_path);
    assert(font);
    if (rtl_path) {
        const char *paths[] = {rtl_path};
        assert(cui_font_register_fallbacks(font, paths, 1));
    }
    cui_context *ui = cui_create((cui_config){.measure_styled = font_measure,
                                              .measure_user = font,
                                              .text_geometry_line = cui_font_text_geometry_line,
                                              .max_elements = 256,
                                              .frame_bytes = 65536});
    assert(ui);
    cui_software *renderer = cui_software_create(200, 200, font);
    assert(renderer);
    const char *text = rtl_path ? "office אבגד ä office" : "office Ωβ ä office";
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 55});
    cui_text_color_span spans[] = {{0, 7, {255, 0, 0, 255}}, {7, strlen(text), {0, 255, 0, 255}}};
    assert(cui_text_paragraph_colored(ui, cui_string(text), 18, (cui_color){0}, 24,
                                      (cui_text_style){0}, true, spans, 2));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count >= 3);
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *d = &list.commands[i];
        assert(cui_text_line_source_valid(d->text_source));
        assert(boundary(cui_string(text), d->text_source.byte_start) &&
               boundary(cui_string(text), d->text_source.byte_end));
        assert(!memcmp(d->text_source.paragraph.data, text, strlen(text)));
        for (size_t j = 0; j < d->text_color_count; ++j)
            assert(d->text_colors[j].byte_start >= d->text_source.byte_start &&
                   d->text_colors[j].byte_end <= d->text_source.byte_end);
    }
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    size_t red = 0, green = 0;
    for (size_t i = 0; i < (size_t)w * h; ++i) {
        red += pixels[i * 4] > pixels[i * 4 + 1];
        green += pixels[i * 4 + 1] > pixels[i * 4];
    }
    assert(red && green);
    cui_software_destroy(renderer);
    cui_destroy(ui);
    cui_font_close(font);
}
#endif
int main(int argc, char **argv) {
    conformance();
    source_cases();
    bounded();
    prepaint_layout();
    shaped_cluster();
#ifdef CUI_TEST_WRAP_RENDERER
    assert(argc == 2 || argc == 3);
    rendered(argv[1], argc == 3 ? argv[2] : NULL);
#else
    (void)argc;
    (void)argv;
#endif
    return 0;
}
