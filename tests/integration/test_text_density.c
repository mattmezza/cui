/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    size_t area, count;
    unsigned long hash;
} pixels;
static void glyph(void *user, int x, int y, int w, int h, int pitch, const unsigned char *data) {
    pixels *p = user;
    p->count++;
    p->area += (size_t)w * h;
    p->hash = p->hash * 33 + (unsigned)x;
    p->hash = p->hash * 33 + (unsigned)y;
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++)
            p->hash = p->hash * 33 + data[row * pitch + col];
}
static void colored(void *user, int x, int y, int w, int h, int pitch, const unsigned char *data,
                    cui_color color) {
    assert(color.r == 17 && color.g == 42);
    glyph(user, x, y, w, h, pitch, data);
}
int main(void) {
    const char *path = getenv("CUI_TEST_FONT");
    cui_font *font = path ? cui_font_open(path) : NULL;
    const char *paths[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    for (size_t i = 0; !font && i < sizeof paths / sizeof paths[0]; i++)
        font = cui_font_open(paths[i]);
    if (!font)
        return 77;
    cui_str text = cui_string("AV fi Retina");
    cui_size before = cui_font_measure(font, text, 17);
    cui_font_metrics metrics, after_metrics;
    assert(cui_font_get_metrics(font, 17, (cui_text_style){0}, &metrics));
    pixels one = {0}, two = {0}, fractional = {0}, again = {0};
    assert(cui_font_draw_context_line_scaled(font, text, (cui_text_line_source){0}, 17,
                                             (cui_text_style){0}, 3, 5, 1, glyph, &one));
    assert(cui_font_draw_context_line_scaled(font, text, (cui_text_line_source){0}, 17,
                                             (cui_text_style){0}, 3, 5, 2, glyph, &two));
    assert(cui_font_draw_context_line_scaled(font, text, (cui_text_line_source){0}, 17,
                                             (cui_text_style){0}, 3, 5, 1.5f, glyph, &fractional));
    assert(two.count == one.count && two.area > one.area * 3);
    assert(fractional.count == one.count && fractional.area > one.area);
    assert(cui_font_draw_context_line(font, text, (cui_text_line_source){0}, 17,
                                      (cui_text_style){0}, 3, 5, glyph, &again));
    assert(!memcmp(&one, &again, sizeof one));
    /* Revisit 2x after other sizes: the raster cache must key physical size,
     * while repeated paints must preserve both coverage and logical positions. */
    pixels two_again = {0};
    assert(cui_font_draw_context_line_scaled(font, text, (cui_text_line_source){0}, 17,
                                             (cui_text_style){0}, 3, 5, 2, glyph, &two_again));
    assert(two_again.count == two.count && two_again.area == two.area &&
           two_again.hash == two.hash);
    cui_size after = cui_font_measure(font, text, 17);
    assert(before.width == after.width && before.height == after.height);
    assert(cui_font_get_metrics(font, 17, (cui_text_style){0}, &after_metrics));
    assert(!memcmp(&metrics, &after_metrics, sizeof metrics));
    assert(!cui_font_draw_context_line_scaled(font, text, (cui_text_line_source){0}, 17,
                                              (cui_text_style){0}, 0, 0, NAN, glyph, &again));
    assert(!cui_font_draw_context_line_scaled(font, text, (cui_text_line_source){0}, 17,
                                              (cui_text_style){0}, 0, 0, 0, glyph, &again));
    cui_text_layout *layout = NULL;
    if (cui_font_layout_line(font, text, 17, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0, text.length,
                             NULL, &layout) == CUI_TEXT_LAYOUT_OK) {
        pixels direct = {0}, color = {0};
        cui_text_caret caret_before, caret_after;
        cui_text_layout_view view_before, view_after;
        assert(cui_text_layout_get(layout, &view_before));
        assert(cui_text_layout_hit_test(layout, 12, 4, CUI_TEXT_DOWNSTREAM, &caret_before) ==
               CUI_TEXT_LAYOUT_OK);
        assert(cui_text_layout_draw_scaled(layout, 3, 5, 2, glyph, &direct));
        assert(direct.hash == two.hash && direct.area == two.area);
        assert(cui_font_draw_context_line_colored_scaled(
            font, (cui_text_line_source){text, 0, text.length}, 17, (cui_text_style){0}, 3, 5,
            (cui_color){17, 42, 0, 255}, NULL, 0, 2, colored, &color));
        assert(color.hash == two.hash && color.area == two.area);
        assert(cui_text_layout_get(layout, &view_after));
        assert(!memcmp(&view_before, &view_after, sizeof view_before));
        assert(cui_text_layout_hit_test(layout, 12, 4, CUI_TEXT_DOWNSTREAM, &caret_after) ==
               CUI_TEXT_LAYOUT_OK);
        assert(caret_before.x == caret_after.x &&
               caret_before.byte_offset == caret_after.byte_offset);
        cui_text_layout_destroy(layout);
    }
    cui_font_close(font);
    puts("Density 1x/1.5x/2x preserves logical metrics and rasterizes additional glyph pixels");
    return 0;
}
