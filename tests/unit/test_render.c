/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const unsigned char *pixel(cui_software *r, int x, int y) {
    unsigned w;
    cui_software_pixels(r, &w, NULL);
    return cui_software_pixels(r, NULL, NULL) + ((size_t)y * w + (unsigned)x) * 4;
}
#ifdef CUI_HAS_TEXT
typedef struct {
    unsigned glyphs, coverage;
} glyph_stats;
static void glyph_count(void *user, int x, int y, int w, int h, int pitch,
                        const unsigned char *data) {
    glyph_stats *s = user;
    (void)x;
    (void)y;
    if (!data)
        return;
    s->glyphs++;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            s->coverage += data[j * pitch + i] != 0;
}
static void test_text(void) {
    assert(!cui_font_open(NULL));
    assert(!cui_font_open("/cui-test/nonexistent/font.ttf"));
    const char *requested = getenv("CUI_TEST_FONT");
    const char *paths[] = {requested, "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    cui_font *font = NULL;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]) && !font; i++)
        if (paths[i])
            font = cui_font_open(paths[i]);
    if (!font) {
        puts("text raster test skipped: set CUI_TEST_FONT to a font file");
        return;
    }
    const char *text = "office caf\xc3\xa9";
    cui_str str = {text, strlen(text)};
    cui_size size = cui_font_measure(font, str, 18);
    assert(size.width > 40 && size.height > 18);
    cui_size double_size = cui_font_measure(font, str, 36);
    assert(double_size.width > size.width * 1.8f);
    glyph_stats stats = {0};
    assert(cui_font_draw(font, str, 18, 0, 0, glyph_count, &stats));
    assert(stats.glyphs > 0 && stats.coverage > 0);
    stats = (glyph_stats){0};
    assert(cui_font_draw(font, (cui_str){"ffi", 3}, 18, 0, 0, glyph_count, &stats));
    assert(stats.glyphs > 0 && stats.glyphs <= 3);
    if (!requested)
        assert(stats.glyphs < 3); /* packaged default fonts have an ffi ligature */
    const char rtl[] = {'\xd8', '\xb3', '\xd9', '\x84', '\xd8', '\xa7', '\xd9', '\x85'};
    stats = (glyph_stats){0};
    assert(cui_font_draw(font, (cui_str){rtl, sizeof(rtl)}, 20, 0, 0, glyph_count, &stats));
    assert(stats.glyphs > 0);
    assert(cui_font_measure(font, str, NAN).width == 0);
    assert(cui_font_measure(font, str, -1).width == 0);
    assert(cui_font_measure(font, (cui_str){NULL, 4}, 18).width == 0);
    assert(!cui_font_draw(font, str, 18, INFINITY, 0, glyph_count, &stats));
    cui_font_close(font);
    puts("HarfBuzz multilingual shaping and FreeType coverage passed");
}
#endif
static void asymmetric_shapes(void) {
    cui_software *r = cui_software_create(64, 64, NULL);
    assert(r);
    cui_draw_command c = {.type = CUI_DRAW_RECT,
                          .bounds = {8, 8, 48, 48},
                          .color = {255, 0, 0, 128},
                          .custom_corners = true,
                          .corner_radii = {12, 0, 0, 6},
                          .custom_border_edges = true,
                          .border_edges = {0, 4, 2, 6}};
    cui_draw_list list = {&c, 1, 64, 64};
    assert(cui_software_render(r, &list, (cui_color){0}));
    assert(pixel(r, 8, 8)[3] == 0 && pixel(r, 55, 8)[3] == 128 && pixel(r, 8, 55)[3] == 128 &&
           pixel(r, 55, 55)[3] == 0);
    c.type = CUI_DRAW_BORDER;
    assert(cui_software_render(r, &list, (cui_color){0}));
    assert(pixel(r, 8, 32)[3] == 0 && pixel(r, 54, 32)[3] == 128 && pixel(r, 32, 8)[3] == 128 &&
           pixel(r, 32, 54)[3] == 128 && pixel(r, 32, 32)[3] == 0);
    cui_draw_command clipped[] = {
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {32, 0, 32, 64}}, c, {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list clip_list = {clipped, 3, 64, 64};
    assert(cui_software_render(r, &clip_list, (cui_color){0}));
    assert(pixel(r, 20, 8)[3] == 0 && pixel(r, 40, 8)[3] == 128);
    c.border_edges = (cui_border_edges){0};
    assert(cui_software_render(r, &list, (cui_color){0}));
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            assert(pixel(r, x, y)[3] == 0);
    c.corner_radii.top_left = NAN;
    assert(!cui_software_render(r, &list, (cui_color){0}));
    c.corner_radii.top_left = 12;
    c.border_edges.bottom = INFINITY;
    assert(!cui_software_render(r, &list, (cui_color){0}));
    cui_software_destroy(r);
}
static void test_underlay_pixels(void) {
    cui_context *ui = cui_create((cui_config){.width = 64, .height = 48});
    cui_software *renderer = cui_software_create(64, 48, NULL);
    assert(ui && renderer && cui_begin_frame(ui, 64, 48, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 40, .height = 40, .background = {255, 255, 255, 255}});
    assert(cui_set_underlay(ui, (cui_rect){-20, 10, 80, 20}, (cui_color){255, 0, 0, 255},
                            (cui_corner_radii){0}));
    cui_begin_box(ui, 2, (cui_box){.width = 20, .height = 20, .background = {0, 0, 255, 255}});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    assert(pixel(renderer, 5, 15)[2] == 255 && pixel(renderer, 5, 15)[0] == 0);
    assert(pixel(renderer, 30, 15)[0] == 255 && pixel(renderer, 30, 15)[2] == 0);
    assert(pixel(renderer, 45, 15)[0] == 0);
    assert(pixel(renderer, 30, 30)[0] == 255 && pixel(renderer, 30, 30)[2] == 255);
    cui_destroy(ui);
    cui_software_destroy(renderer);
}

static void test_bitmap_pixels(void) {
    const unsigned char pixels[] = {255, 0, 0,   255, 0,   255, 0,   128, 99, 99, 99, 99,
                                    0,   0, 255, 255, 255, 255, 255, 255, 99, 99, 99, 99};
    cui_software *r = cui_software_create(8, 8, NULL);
    assert(r);
    cui_draw_command d = {
        .type = CUI_DRAW_IMAGE,
        .bounds = {0, 0, 8, 8},
        .bitmap = {.bitmap = {pixels, 2, 2, 12, 1}, .source = {0, 0, 2, 2}, .opacity = 1}};
    cui_draw_list list = {.commands = &d, .count = 1, .width = 8, .height = 8};
    assert(cui_software_render(r, &list, (cui_color){0}));
    assert(pixel(r, 1, 1)[0] == 255 && pixel(r, 6, 1)[1] == 255);
    assert(pixel(r, 6, 1)[3] == 128 && pixel(r, 1, 6)[2] == 255);
    d.bitmap.source = (cui_rect){1, 0, 1, 1};
    d.bitmap.opacity = .5f;
    assert(cui_software_render(r, &list, (cui_color){0}));
    assert(pixel(r, 1, 6)[1] == 255 && pixel(r, 1, 6)[3] == 64);
    d.bitmap.grayscale = true;
    d.radius = 4;
    assert(cui_software_render(r, &list, (cui_color){0}));
    assert(pixel(r, 0, 0)[3] == 0);
    assert(pixel(r, 4, 4)[0] == 182 && pixel(r, 4, 4)[1] == 182);
    d.bitmap.source.w = 2;
    assert(!cui_software_render(r, &list, (cui_color){0}));
    cui_software_destroy(r);
}
int main(void) {
    test_bitmap_pixels();
    test_underlay_pixels();
    asymmetric_shapes();
    cui_software *r = cui_software_create(16, 16, NULL);
    assert(r);
    cui_draw_command commands[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 16, 16}, .color = {255, 0, 0, 128}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {4, 4, 8, 8}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {6, 2, 8, 6}},
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 16, 16}, .color = {0, 255, 0, 255}},
        {.type = CUI_DRAW_CLIP_POP},
        {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {commands, 6, 16, 16};
    assert(cui_software_render(r, &list, (cui_color){0, 0, 255, 255}));
    assert(pixel(r, 0, 0)[0] == 128 && pixel(r, 0, 0)[2] == 127 && pixel(r, 0, 0)[3] == 255);
    assert(pixel(r, 6, 4)[1] == 255);
    assert(pixel(r, 5, 4)[1] == 0);
    assert(pixel(r, 6, 8)[1] == 0);
    commands[0] = (cui_draw_command){.type = CUI_DRAW_RECT,
                                     .bounds = {2, 2, 12, 12},
                                     .radius = 6,
                                     .color = {255, 255, 255, 255}};
    list.count = 1;
    assert(cui_software_render(r, &list, (cui_color){0, 0, 0, 0}));
    assert(pixel(r, 2, 2)[3] == 0);
    assert(pixel(r, 8, 8)[3] == 255);
    commands[0] = (cui_draw_command){.type = CUI_DRAW_BORDER,
                                     .bounds = {2, 2, 12, 12},
                                     .radius = 3,
                                     .stroke = 2,
                                     .color = {255, 255, 255, 255}};
    assert(cui_software_render(r, &list, (cui_color){0, 0, 0, 0}));
    assert(pixel(r, 8, 2)[3] == 255);
    assert(pixel(r, 8, 8)[3] == 0);
    commands[0] = (cui_draw_command){
        .type = CUI_DRAW_RECT, .bounds = {0, 0, 1, 1}, .color = {100, 40, 20, 128}};
    assert(cui_software_render(r, &list, (cui_color){0, 0, 0, 0}));
    assert(pixel(r, 0, 0)[0] == 100 && pixel(r, 0, 0)[3] == 128);
    commands[0].type = CUI_DRAW_CLIP_POP;
    assert(!cui_software_render(r, &list, (cui_color){0}));
    commands[0] = (cui_draw_command){.type = CUI_DRAW_RECT, .bounds = {1e30f, 1e30f, 10, 10}};
    assert(cui_software_render(r, &list, (cui_color){0}));
    commands[0].bounds.x = NAN;
    assert(!cui_software_render(r, &list, (cui_color){0}));
    assert(!cui_software_resize(r, 0, 16));
    assert(cui_software_resize(r, 8, 8));
    unsigned w, h;
    cui_software_pixels(r, &w, &h);
    assert(w == 8 && h == 8);
    cui_software_destroy(r);
    puts("software renderer geometry, nested clipping and straight alpha passed");
#ifdef CUI_HAS_TEXT
    test_text();
#endif
    return 0;
}
