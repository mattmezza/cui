/* SPDX-License-Identifier: MIT */
/* Shared active-field serializer/cases, NOT expected-output generation from candidate. */
#include "cui/cui_software.h"
#include <assert.h>
#include <string.h>
static unsigned char result[1024 * 1024];
static size_t length;
static void bytes(const void *p, size_t n) {
    assert(n <= sizeof(result) - length);
    memcpy(result + length, p, n);
    length += n;
}
static void number(uint32_t v) {
    unsigned char b[4] = {(unsigned char)v, (unsigned char)(v >> 8), (unsigned char)(v >> 16),
                          (unsigned char)(v >> 24)};
    bytes(b, 4);
}
static void record(void *user, int x, int y, int w, int h, int pitch, const unsigned char *data) {
    (void)user;
    if (!w || !h)
        return; /* Canonicalize no-coverage callbacks; pixels remain independently checked. */
    number((uint32_t)x);
    number((uint32_t)y);
    number((uint32_t)w);
    number((uint32_t)h);
    for (int row = 0; row < h; row++)
        bytes(data + (ptrdiff_t)row * pitch, (size_t)w);
}
static void colored(void *user, int x, int y, int w, int h, int pitch, const unsigned char *data,
                    cui_color color) {
    if (!w || !h)
        return;
    record(user, x, y, w, h, pitch, data);
    bytes(&color, 4);
}
static cui_draw_command fixture(unsigned i) {
    static const cui_text_color_span spans[2] = {{0, 2, {255, 0, 0, 180}},
                                                 {2, 9, {0, 80, 255, 230}}};
    static const cui_text_gradient_stop stops[2] = {{0, {255, 0, 0, 0}}, {1, {0, 0, 255, 255}}};
    cui_draw_command c = {.type = CUI_DRAW_TEXT,
                          .bounds = {3, 2, 80, 40},
                          .font_size = 14,
                          .color = {30, 80, 210, 190},
                          .text = CUI_STR("Affi caf\xc3\xa9")};
    if (i == 1)
        c.text = (cui_str)CUI_STR("one\nTwo");
    if (i == 2)
        c.text = (cui_str)CUI_STR("A\xe2\x80\xa8"
                                  "B");
    if (i == 3)
        c.text_style.letter_spacing = 1.25f;
    if (i == 4) {
        c.text_source = (cui_text_line_source){(cui_str)CUI_STR("abc caf\xc3\xa9"), 0, 9};
        c.text = c.text_source.paragraph;
        c.text_colors = spans;
        c.text_color_count = 2;
    }
    if (i == 5) {
        c.text_source = (cui_text_line_source){(cui_str)CUI_STR("xx abc caf\xc3\xa9 zz"), 3, 12};
        c.text = (cui_str){c.text_source.paragraph.data + 3, 9};
        c.text_paint = (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                                        .x0 = 3,
                                        .y0 = 2,
                                        .x1 = 70,
                                        .y1 = 25,
                                        .stops = stops,
                                        .stop_count = 2};
    }
    if (i == 6)
        c.text_style.weight = CUI_TEXT_WEIGHT_BOLD;
    if (i == 7) {
        c.text = (cui_str)CUI_STR("g");
        c.font_size = 5;
    }
    return c;
}
static void baseline(cui_font *font) {
    length = 0;
    bytes("CUI_FONT_V1", 11);
    cui_software *software = cui_software_create(96, 72, font);
    assert(software);
    for (unsigned i = 0; i < 8; i++) {
#ifdef CUI_TEXT_NO_BIDI_FIXTURE
        if (i != 0 && i != 6 && i != 7)
            continue;
#endif
        cui_draw_command c = fixture(i);
        float density = i == 7 ? 16 : 1;
        number(i);
        size_t begin = length;
        number(0);
        bool ok = c.text_color_count
                      ? cui_font_draw_context_line_colored_scaled(
                            font, c.text_source, c.font_size, c.text_style, c.bounds.x, c.bounds.y,
                            c.color, c.text_colors, c.text_color_count, density, colored, NULL)
                      : cui_font_draw_context_line_scaled(font, c.text, c.text_source, c.font_size,
                                                          c.text_style, c.bounds.x, c.bounds.y,
                                                          density, record, NULL);
        assert(ok);
        size_t n = length - begin - 4;
        assert(n <= UINT32_MAX);
        uint32_t value = (uint32_t)n;
        for (unsigned j = 0; j < 4; j++)
            result[begin + j] = (unsigned char)(value >> (8 * j));
        cui_draw_list list = {&c, 1, 96, 72};
        assert(cui_software_render(software, &list, (cui_color){9, 17, 25, 70}));
        bytes(cui_software_pixels(software, NULL, NULL), 96 * 72 * 4);
    }
    cui_software_destroy(software);
}
