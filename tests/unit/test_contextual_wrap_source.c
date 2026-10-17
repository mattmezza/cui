/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#ifdef CUI_TEST_WRAP_RENDERER
#include "cui/cui_software.h"
#include "cui/cui_text.h"
#include <stdlib.h>
#endif
static cui_size measure(void *user, cui_str text, float size, cui_text_style style) {
    (void)size;
    unsigned *families = user;
    if (style.family)
        ++*families;
    size_t scalars = 0;
    for (size_t i = 0; i < text.length; ++i)
        scalars += ((unsigned char)text.data[i] & 0xc0u) != 0x80u;
    return (cui_size){(float)scalars * 10, 12};
}
static void valid_draw(const cui_draw_command *d) {
    assert(d->type == CUI_DRAW_TEXT && cui_text_line_source_valid(d->text_source));
    assert(d->text.length == d->text_source.byte_end - d->text_source.byte_start);
    assert(!memcmp(d->text.data, d->text_source.paragraph.data + d->text_source.byte_start,
                   d->text.length));
    size_t end = d->text_source.byte_start;
    for (size_t i = 0; i < d->text_color_count; ++i) {
        cui_text_color_span s = d->text_colors[i];
        assert(s.byte_start >= end && s.byte_end <= d->text_source.byte_end);
        assert(cui_text_line_source_valid(
            (cui_text_line_source){d->text_source.paragraph, s.byte_start, s.byte_end}));
        end = s.byte_end;
    }
}
static void wrapped_ranges(void) {
    unsigned families = 0;
    cui_context *ui = cui_create((cui_config){.measure_styled = measure,
                                              .measure_user = &families,
                                              .max_elements = 128,
                                              .frame_bytes = 16384});
    assert(ui && cui_begin_frame(ui, 200, 200, 0));
    char text[] = "Éé aa bb";
    cui_text_color_span spans[] = {{0, 7, {255, 0, 0, 255}}, {7, 10, {0, 255, 0, 255}}};
    cui_begin_box(ui, 11, (cui_box){.width = 50});
    assert(cui_text_paragraph_colored(ui, cui_string(text), 16, (cui_color){0}, 20,
                                      (cui_text_style){.family = 3, .weight = CUI_TEXT_WEIGHT_BOLD},
                                      true, spans, 2));
    cui_end_box(ui);
    memset(text, 'x', sizeof(text) - 1);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 2 && families > 0);
    valid_draw(&list.commands[0]);
    valid_draw(&list.commands[1]);
    assert(list.commands[0].text_source.byte_start == 0 &&
           list.commands[0].text_source.byte_end == 5);
    assert(list.commands[1].text_source.byte_start == 5 &&
           list.commands[1].text_source.byte_end == 10);
    assert(list.commands[0].text_color_count == 1 && list.commands[0].text_colors[0].byte_end == 5);
    assert(list.commands[1].text_color_count == 2 &&
           list.commands[1].text_colors[0].byte_start == 5);
    assert(list.commands[1].text_colors[1].byte_start == 7 &&
           list.commands[1].text_colors[1].color.g == 255);
    assert(list.commands[1].bounds.y - list.commands[0].bounds.y == 20);
    assert(cui_bounds(ui, 11).h == 40);
    /* Parent width changes in this frame, without prior-frame bounds. */
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_begin_box(ui, 11, (cui_box){.width = 120});
    assert(cui_text_paragraph_colored(ui, cui_string("Éé aa bb"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, spans, 2));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 1 && cui_bounds(ui, 11).h == 20);
    /* Different original ranges sharing one paragraph retain distinct identity. */
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_str shared = cui_string("AA BB CC");
    cui_text_color_span blue = {0, 8, {0, 0, 255, 255}};
    cui_text_color_span red = {0, 8, {255, 0, 0, 255}};
    assert(cui_text_paragraph_colored(ui, shared, 16, (cui_color){0}, 20, (cui_text_style){0}, true,
                                      &blue, 1));
    assert(cui_text_paragraph_colored(ui, shared, 16, (cui_color){0}, 20, (cui_text_style){0}, true,
                                      &red, 1));
    list = cui_end_frame(ui);
    assert(list.count == 2);
    assert(list.commands[0].text_colors[0].color.b == 255 &&
           list.commands[1].text_colors[0].color.r == 255);
    assert(list.commands[0].text.data != list.commands[1].text.data);
    cui_destroy(ui);
}
static void paragraphs(void) {
    unsigned families = 0;
    cui_context *ui = cui_create((cui_config){.measure_styled = measure,
                                              .measure_user = &families,
                                              .max_elements = 128,
                                              .frame_bytes = 16384});
    const char *text = "a\r\n\nb\xe2\x80\xa8"
                       "c\xe2\x80\xa9"
                       "d\xc2\x85"
                       "e\rf\n";
    cui_text_color_span span = {0, strlen(text), {3, 4, 5, 255}};
    assert(ui && cui_begin_frame(ui, 200, 300, 0));
    cui_begin_box(ui, 12, (cui_box){.width = 100});
    assert(cui_text_paragraph_colored(ui, cui_string(text), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, &span, 1));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 6 && cui_bounds(ui, 12).h == 160);
    const float ys[] = {4, 44, 64, 84, 104, 124};
    for (size_t i = 0; i < 6; ++i) {
        valid_draw(&list.commands[i]);
        assert(list.commands[i].text.length == 1 && list.commands[i].text.data[0] == 'a' + (char)i);
        assert(list.commands[i].bounds.y == ys[i] && list.commands[i].bounds.h == 20);
        assert(list.commands[i].text_color_count == 1 &&
               list.commands[i].text_colors[0].color.r == 3);
    }
    assert(list.commands[1].text_source.paragraph.length == 5);
    assert(!memcmp(list.commands[1].text_source.paragraph.data,
                   "b\xe2\x80\xa8"
                   "c",
                   5));
    assert(list.commands[2].text_source.paragraph.data ==
           list.commands[1].text_source.paragraph.data);
    assert(list.commands[2].text_source.byte_start == 4 &&
           list.commands[2].text_colors[0].byte_start == 4);
    assert(cui_begin_frame(ui, 200, 300, 0));
    cui_begin_box(ui, 12, (cui_box){.width = 100});
    assert(cui_text_paragraph_colored(ui, cui_string("\n\n"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, NULL, 0));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 0 && cui_bounds(ui, 12).h == 60);
    assert(cui_begin_frame(ui, 200, 300, 0));
    assert(cui_text_paragraph_colored(ui, cui_string("a b c"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, false, NULL, 0));
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 5);
    cui_destroy(ui);
}
static void failures(void) {
    cui_context *ui = cui_create(
        (cui_config){.max_elements = 16, .frame_bytes = 48 + 2 * sizeof(cui_text_color_span)});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(!cui_text_paragraph_colored(ui, cui_string("\xf0\x80\x80\x80"), 16, (cui_color){0}, 20,
                                       (cui_text_style){0}, true, NULL, 0));
    cui_text_color_span broken = {1, 2, {0}};
    assert(!cui_text_paragraph_colored(ui, cui_string("é"), 16, (cui_color){0}, 20,
                                       (cui_text_style){0}, true, &broken, 1));
    char large[256];
    memset(large, 'x', sizeof(large));
    assert(!cui_text_paragraph_colored(ui, (cui_str){large, sizeof(large)}, 16, (cui_color){0}, 20,
                                       (cui_text_style){0}, true, NULL, 0));
    assert(cui_text_paragraph_colored(ui, cui_string("ok"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, NULL, 0));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 2);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(!cui_text_context_line_colored(ui,
                                          (cui_text_line_source){cui_string("a\xe2\x80\xa8"
                                                                            "b"),
                                                                 0, 5},
                                          16, (cui_color){0}, 20, (cui_text_style){0}, NULL, 0));
    assert(!cui_text_context_line_colored(ui, (cui_text_line_source){cui_string("a\nb"), 0, 3}, 16,
                                          (cui_color){0}, 20, (cui_text_style){0}, NULL, 0));
    assert(cui_text_context_line_colored(ui, (cui_text_line_source){cui_string("a b"), 0, 3}, 16,
                                         (cui_color){0}, 20, (cui_text_style){0}, NULL, 0));
    list = cui_end_frame(ui);
    assert(list.count == 1);
    cui_destroy(ui);
    ui = cui_create((cui_config){
        .measure_styled = measure,
        .measure_user = &(unsigned){0},
        .max_elements = 64,
        .frame_bytes = 48 + 17 * (2 * sizeof(size_t) + sizeof(float) + sizeof(uint32_t)) +
                       2 * sizeof(cui_text_color_span)});
    assert(ui && cui_begin_frame(ui, 200, 300, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 40});
    cui_text_color_span all = {0, 17, {255, 0, 0, 255}};
    assert(cui_text_paragraph_colored(ui, cui_string("aa aa aa aa aa aa"), 16, (cui_color){0}, 20,
                                      (cui_text_style){0}, true, &all, 1));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
}
#ifdef CUI_TEST_WRAP_RENDERER
static cui_size font_measure(void *user, cui_str text, float size, cui_text_style style) {
    return cui_font_measure_styled(user, text, size, style);
}
static void renderer_snapshot(const char *path) {
    cui_font *font = cui_font_open(path);
    assert(font);
    cui_software *renderer = cui_software_create(200, 100, font);
    assert(renderer);
    cui_context *ui = cui_create((cui_config){.measure_styled = font_measure,
                                              .measure_user = font,
                                              .text_geometry_line = cui_font_text_geometry_line,
                                              .max_elements = 128,
                                              .frame_bytes = 16384});
    assert(ui);
    float width = cui_font_measure(font, cui_string("AA BB "), 18).width + .5f;
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = width});
    cui_text_color_span spans[] = {{0, 3, {255, 0, 0, 255}}, {3, 8, {0, 255, 0, 255}}};
    assert(cui_text_paragraph_colored(ui, cui_string("AA BB CC"), 18, (cui_color){0}, 24,
                                      (cui_text_style){0}, true, spans, 2));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 2);
    for (size_t i = 0; i < list.count; ++i)
        valid_draw(&list.commands[i]);
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    size_t red = 0, green = 0, second_green = 0;
    for (unsigned y = 0; y < h; ++y)
        for (unsigned x = 0; x < w; ++x) {
            const unsigned char *p = pixels + ((size_t)y * w + x) * 4;
            if (p[0] > p[1] && p[0] > p[2])
                ++red;
            if (p[1] > p[0] && p[1] > p[2]) {
                ++green;
                if (y >= 24)
                    ++second_green;
            }
        }
    assert(red > 0 && green > 0 && second_green > 0);
    /* A copied snapshot owns its paragraph and spans across frame invalidation. */
    cui_draw_command saved[2];
    char paragraphs[2][32], text[2][32];
    cui_text_color_span colors[2][4];
    for (size_t i = 0; i < 2; ++i) {
        saved[i] = list.commands[i];
        assert(saved[i].text_source.paragraph.length < 32 && saved[i].text.length < 32 &&
               saved[i].text_color_count < 4);
        memcpy(paragraphs[i], saved[i].text_source.paragraph.data,
               saved[i].text_source.paragraph.length);
        memcpy(text[i], saved[i].text.data, saved[i].text.length);
        memcpy(colors[i], saved[i].text_colors, saved[i].text_color_count * sizeof(colors[i][0]));
        saved[i].text_source.paragraph.data = paragraphs[i];
        saved[i].text.data = text[i];
        saved[i].text_colors = colors[i];
    }
    unsigned char *snapshot = malloc((size_t)w * h * 4);
    assert(snapshot);
    memcpy(snapshot, pixels, (size_t)w * h * 4);
    assert(cui_begin_frame(ui, 200, 100, 0));
    (void)cui_end_frame(ui);
    cui_draw_list retained = list;
    retained.commands = saved;
    assert(cui_software_render(renderer, &retained, (cui_color){0, 0, 0, 255}));
    assert(!memcmp(snapshot, cui_software_pixels(renderer, NULL, NULL), (size_t)w * h * 4));
    free(snapshot);
    cui_destroy(ui);
    cui_software_destroy(renderer);
    cui_font_close(font);
}
#endif
int main(int argc, char **argv) {
    wrapped_ranges();
    paragraphs();
    failures();
#ifdef CUI_TEST_WRAP_RENDERER
    assert(argc == 2);
    renderer_snapshot(argv[1]);
#else
    (void)argc;
    (void)argv;
#endif
    return 0;
}
