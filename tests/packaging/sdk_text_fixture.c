/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    size_t glyphs, red, green, pixels;
} raster;
static void emit(void *user, int x, int y, int width, int height, int pitch,
                 const unsigned char *coverage, cui_color color) {
    (void)x;
    (void)y;
    raster *paint = user;
    ++paint->glyphs;
    paint->red += color.r == 255 && color.g == 0;
    paint->green += color.g == 255 && color.r == 0;
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col)
            paint->pixels += coverage[row * pitch + col] != 0;
}
int main(int argc, char **argv) {
    assert(argc == 3);
    cui_font *font = cui_font_open(argv[1]);
    assert(font && cui_font_register_fallbacks(font, (const char *const *)&argv[2], 1));
    cui_str paragraph = cui_string("office אבגד office");
    cui_text_line_source source = {paragraph, 7, 15};
    cui_text_geometry geometry = {.width = 123};
    cui_text_line_source invalid = {paragraph, 8, 15};
    assert(cui_font_text_geometry_line(font, invalid, 18, (cui_text_style){0}, &geometry) ==
               CUI_TEXT_LAYOUT_INVALID &&
           geometry.width == 123 && !geometry.session);
    assert(cui_font_text_geometry_line(font, source, 18, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_OK);
    assert(geometry.byte_length == paragraph.length && geometry.width > 0 && geometry.height > 0);
    size_t caret_count = 0, rectangle_count = 0;
    assert(geometry.ops->carets(geometry.session, 7, NULL, 0, &caret_count) == CUI_TEXT_LAYOUT_OK);
    assert(caret_count > 0 && caret_count <= 2);
    cui_text_caret carets[2], hit;
    assert(geometry.ops->carets(geometry.session, 7, carets, 2, &caret_count) ==
           CUI_TEXT_LAYOUT_OK);
    assert(geometry.ops->hit_test(geometry.session, carets[0].x, 0, carets[0].affinity, &hit) ==
               CUI_TEXT_LAYOUT_OK &&
           hit.byte_offset >= 7 && hit.byte_offset <= 15);
    assert(geometry.ops->selection(geometry.session, 7, 15, NULL, 0, &rectangle_count) ==
           CUI_TEXT_LAYOUT_OK);
    assert(rectangle_count == 4);
    cui_text_selection_rect rectangles[4];
    assert(geometry.ops->selection(geometry.session, 7, 15, rectangles, 4, &rectangle_count) ==
           CUI_TEXT_LAYOUT_OK);
    for (size_t i = 0; i < rectangle_count; ++i)
        assert(rectangles[i].byte_start >= 7 && rectangles[i].byte_end <= 15 &&
               rectangles[i].bounds.w > 0);
    float width = geometry.width;
    geometry.ops->release(geometry.session);
    cui_text_layout *layout = NULL;
    assert(cui_font_layout_line(font, paragraph, 18, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0,
                                SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    cui_text_layout_view view;
    assert(cui_text_layout_get(layout, &view));
    size_t rtl = 0, fallbacks = 0;
    for (size_t i = 0; i < view.run_count; ++i) {
        rtl += (view.runs[i].level & 1) != 0;
        fallbacks += view.runs[i].face_index != 0;
    }
    assert(view.face_count == 2 && rtl > 0 && fallbacks > 0 && view.cluster_count > 0);
    for (size_t i = 0; i < view.glyph_count; ++i)
        assert(view.glyphs[i].glyph_id != 0);
    size_t glyphs = view.glyph_count, runs = view.run_count, clusters = view.cluster_count;
    cui_text_layout_destroy(layout);
    cui_text_color_span colors[] = {{7, 11, {255, 0, 0, 255}}, {11, 15, {0, 255, 0, 255}}};
    raster paint = {0};
    assert(cui_font_draw_context_line_colored(font, source, 18, (cui_text_style){0}, 0, 0,
                                              (cui_color){0, 0, 0, 255}, colors, 2, emit, &paint));
    assert(paint.glyphs == 4 && paint.red > 0 && paint.green > 0 && paint.pixels > 0);
    printf("width=%.3f carets=%zu rectangles=%zu runs=%zu rtl=%zu fallback=%zu glyphs=%zu "
           "clusters=%zu red=%zu green=%zu pixels=%zu\n",
           width, caret_count, rectangle_count, runs, rtl, fallbacks, glyphs, clusters, paint.red,
           paint.green, paint.pixels);
    cui_font_close(font);
    return 0;
}
