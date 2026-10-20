/* Test-only deterministic coverage provider for the actual software text path.
 * Rectangular glyphs exercise metrics/style/color transport, not font shaping. */
#include <string.h>
#define cui_software_create progress_software_create
#define cui_software_destroy progress_software_destroy
#define cui_software_resize progress_software_resize
#define cui_software_render progress_software_render
#define cui_software_pixels progress_software_pixels
#define cui_software_save_ppm progress_software_save_ppm
#define cui_software_interface progress_software_interface
#define cui_font_text_capabilities progress_font_capabilities
#define cui_font_draw_context_line progress_font_draw
#define cui_font_draw_context_line_colored progress_font_draw_colored
#include "cui/cui_software.h"
uint32_t progress_font_capabilities(void) {
    return 0;
}
bool progress_font_draw(cui_font *font, cui_str text, cui_text_line_source source, float size,
                        cui_text_style style, float x, float y, cui_glyph_fn emit, void *user) {
    (void)font;
    (void)source;
    unsigned char coverage[512];
    memset(coverage, 255, sizeof coverage);
    int width = (int)(size * .5f) + (style.weight == CUI_TEXT_WEIGHT_BOLD ? 2 : 0);
    for (size_t i = 0; i < text.length; ++i)
        if (text.data[i] != ' ')
            for (int row = 0; row < (int)size; ++row)
                emit(user, (int)x + (int)i * width, (int)y + row, width, 1, width, coverage);
    return true;
}
bool progress_font_draw_colored(cui_font *font, cui_text_line_source source, float size,
                                cui_text_style style, float x, float y, cui_color color,
                                const cui_text_color_span *spans, size_t count,
                                cui_color_glyph_fn emit, void *user) {
    (void)font;
    (void)source;
    (void)size;
    (void)style;
    (void)x;
    (void)y;
    (void)color;
    (void)spans;
    (void)count;
    (void)emit;
    (void)user;
    return false;
}
#define CUI_HAS_TEXT 1
#include "../../../backends/software/cui_software.c"
#undef CUI_HAS_TEXT
#undef cui_font_text_capabilities
#undef cui_font_draw_context_line
#undef cui_font_draw_context_line_colored
/* Keep software API aliases for the test's calls below. */
