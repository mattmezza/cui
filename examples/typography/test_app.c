/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef CUI_TYPOGRAPHY_NATIVE
typedef struct {
    uint64_t sum;
    size_t glyphs;
} raster_proof;
static void glyph(void *user, int x, int y, int w, int h, int pitch, const unsigned char *pixels) {
    raster_proof *proof = user;
    ++proof->glyphs;
    proof->sum = proof->sum * 31 + (uint64_t)(x + 4096) + (uint64_t)(y + 4096);
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
            proof->sum = proof->sum * 31 + pixels[row * pitch + col];
}
#endif
static cui_size measure(void *user, cui_str text, float size, cui_text_style style) {
    (void)user;
    float factor = style.family == TYPOGRAPHY_HEADING_FAMILY ? .65f : .5f;
    return (cui_size){(float)text.length * size * factor, size};
}
static size_t check(cui_context *ui, const cui_typography_roles *roles, float width) {
    cui_draw_list list = typography_example_frame(ui, roles, width, 1000);
    assert(list.count && cui_last_error(ui) == CUI_OK);
    size_t body_count = 0, heading_count = 0, code_count = 0, italic_count = 0;
    float body_y = -1;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type != CUI_DRAW_TEXT)
            continue;
        assert(isfinite(c->bounds.x) && isfinite(c->bounds.y) && c->bounds.h > 0);
#ifdef CUI_TYPOGRAPHY_NATIVE
        cui_text_geometry geometry = {0};
        assert(cui_get_text_geometry_line(ui, c->text_source, c->font_size, c->text_style,
                                          &geometry) == CUI_TEXT_LAYOUT_OK);
        assert(fabsf(geometry.width - c->bounds.w) < .1f);
        cui_release_text_geometry(&geometry);
#endif
        if (c->text_style.family == TYPOGRAPHY_BODY_FAMILY) {
            assert(c->font_size == 16.5f && c->text_style.weight == CUI_TEXT_WEIGHT_NORMAL);
            if (c->text_style.italic) {
                ++italic_count;
                continue;
            }
            if (body_y >= 0)
                assert(fabsf(c->bounds.y - body_y - 26) < .01f);
            body_y = c->bounds.y;
            ++body_count;
        } else if (c->text_style.family == TYPOGRAPHY_HEADING_FAMILY) {
            assert(c->font_size == 29.5f && c->text_style.weight == CUI_TEXT_WEIGHT_BOLD);
            assert(c->bounds.x >= 24);
            ++heading_count;
        } else {
            assert(c->text_style.family == TYPOGRAPHY_CODE_FAMILY && c->text_style.monospace);
            assert(c->font_size == 14.5f);
            ++code_count;
        }
    }
    assert(body_count && heading_count && italic_count && code_count == 1);
    return body_count;
}
int main(void) {
    const cui_typography_roles roles = typography_example_roles();
    cui_config config = {.width = 900, .height = 1000, .measure_styled = measure};
#ifdef CUI_TYPOGRAPHY_NATIVE
    /* Optional explicit paths; require all five rather than silently falling back. */
    const char *body = getenv("CUI_TYPOGRAPHY_BODY");
    const char *heading = getenv("CUI_TYPOGRAPHY_HEADING");
    const char *bold = getenv("CUI_TYPOGRAPHY_HEADING_BOLD");
    const char *italic = getenv("CUI_TYPOGRAPHY_BODY_ITALIC");
    const char *code = getenv("CUI_TYPOGRAPHY_CODE");
    if (!body || !heading || !bold || !italic || !code)
        return 77;
    cui_font *font = typography_example_font(body, heading, bold, italic, code);
    assert(font);
    config.measure_styled = cui_font_measure_styled;
    config.measure_user = font;
    config.font_metrics = cui_font_get_metrics;
    config.text_geometry = cui_font_text_geometry;
    config.text_geometry_line = cui_font_text_geometry_line;
    cui_font *faces[] = {cui_font_open(body), cui_font_open(bold), cui_font_open(code),
                         cui_font_open(italic)};
    cui_typography_token emphasis = roles.body;
    emphasis.style.italic = true;
    const cui_typography_token tokens[] = {roles.body, roles.heading, roles.code, emphasis};
    for (size_t i = 0; i < 4; ++i) {
        assert(faces[i]);
        cui_size selected = cui_font_measure_styled(font, cui_string("office WWW iii"),
                                                    tokens[i].size, tokens[i].style);
        cui_size actual = cui_font_measure(faces[i], cui_string("office WWW iii"), tokens[i].size);
        assert(fabsf(selected.width - actual.width) < .01f);
        raster_proof selected_raster = {0}, actual_raster = {0};
        assert(cui_font_draw_styled(font, cui_string("office WWW iii"), tokens[i].size,
                                    tokens[i].style, 0, 0, glyph, &selected_raster));
        assert(cui_font_draw(faces[i], cui_string("office WWW iii"), tokens[i].size, 0, 0, glyph,
                             &actual_raster));
        assert(selected_raster.glyphs > 0 && selected_raster.glyphs == actual_raster.glyphs &&
               selected_raster.sum == actual_raster.sum);
        cui_font_close(faces[i]);
    }
#endif
    cui_context *ui = cui_create(config);
    assert(ui);
    size_t wide = check(ui, &roles, 900), narrow = check(ui, &roles, 320);
    assert(narrow > wide);
    assert(check(ui, &roles, 900) == wide);
    assert(roles.body.size == 16.5f && !roles.body.style.italic);
    cui_destroy(ui);
#ifdef CUI_TYPOGRAPHY_NATIVE
    cui_font_close(font);
#endif
    return 0;
}
