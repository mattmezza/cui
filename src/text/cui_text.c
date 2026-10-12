/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
struct cui_font {
    FT_Library library;
    FT_Face face;
    hb_font_t *shaper;
    struct cui_font *variants[4][2][2];
    bool owns_variant[4][2][2];
};
static bool valid_style(cui_text_style style) {
    return style.weight >= CUI_TEXT_WEIGHT_NORMAL && style.weight <= CUI_TEXT_WEIGHT_BOLD;
}
static cui_font *selected(cui_font *font, cui_text_style style) {
    if (!font || !valid_style(style))
        return NULL;
    cui_font *variant = font->variants[style.weight][style.italic][style.monospace];
    return variant ? variant : font;
}
cui_font *cui_font_open(const char *path) {
    cui_font *f = calloc(1, sizeof(*f));
    if (!f)
        return NULL;
    if (!path || FT_Init_FreeType(&f->library) || FT_New_Face(f->library, path, 0, &f->face)) {
        cui_font_close(f);
        return NULL;
    }
    f->shaper = hb_ft_font_create_referenced(f->face);
    if (!f->shaper) {
        cui_font_close(f);
        return NULL;
    }
    return f;
}
void cui_font_close(cui_font *f) {
    if (!f)
        return;
    for (size_t weight = 0; weight < 4; weight++)
        for (size_t italic = 0; italic < 2; italic++)
            for (size_t mono = 0; mono < 2; mono++)
                if (f->owns_variant[weight][italic][mono])
                    cui_font_close(f->variants[weight][italic][mono]);
    if (f->shaper)
        hb_font_destroy(f->shaper);
    if (f->face)
        FT_Done_Face(f->face);
    if (f->library)
        FT_Done_FreeType(f->library);
    free(f);
}
bool cui_font_set_style(cui_font *font, cui_text_style style, cui_font *variant) {
    if (!font || !valid_style(style) ||
        (style.weight == CUI_TEXT_WEIGHT_NORMAL && !style.italic && !style.monospace) ||
        font == variant)
        return false;
    cui_font **slot = &font->variants[style.weight][style.italic][style.monospace];
    bool *owned = &font->owns_variant[style.weight][style.italic][style.monospace];
    if (*slot == variant)
        return true;
    if (*owned)
        cui_font_close(*slot);
    *slot = variant;
    *owned = false;
    return true;
}
bool cui_font_register_style(cui_font *font, cui_text_style style, const char *path) {
    if (!font || !valid_style(style) ||
        (style.weight == CUI_TEXT_WEIGHT_NORMAL && !style.italic && !style.monospace) || !path)
        return false;
    cui_font *variant = cui_font_open(path);
    if (!variant)
        return false;
    cui_font **slot = &font->variants[style.weight][style.italic][style.monospace];
    bool *owned = &font->owns_variant[style.weight][style.italic][style.monospace];
    if (*owned)
        cui_font_close(*slot);
    *slot = variant;
    *owned = true;
    return true;
}
static hb_buffer_t *shape(cui_font *f, cui_str s, float size) {
    if (!f || !isfinite(size) || size <= 0 || size > 4096 || s.length > INT_MAX ||
        (!s.data && s.length))
        return NULL;
    if (FT_Set_Char_Size(f->face, 0, (FT_F26Dot6)lroundf(size * 64), 72, 72))
        return NULL;
    hb_ft_font_changed(f->shaper);
    hb_buffer_t *b = hb_buffer_create();
    hb_buffer_add_utf8(b, s.data, (int)s.length, 0, (int)s.length);
    hb_buffer_guess_segment_properties(b);
    hb_shape(f->shaper, b, NULL, 0);
    return b;
}
bool cui_font_get_metrics(void *font, float size, cui_text_style style, cui_font_metrics *out) {
    cui_font *f = selected(font, style);
    if (!f || !out || !isfinite(size) || size <= 0 || size > 4096 ||
        FT_Set_Char_Size(f->face, 0, (FT_F26Dot6)lroundf(size * 64), 72, 72))
        return false;
    cui_font_metrics result = {(float)f->face->size->metrics.ascender / 64,
                               -(float)f->face->size->metrics.descender / 64,
                               (float)f->face->size->metrics.height / 64};
    if (result.ascent < 0 || result.descent < 0 || result.line_height <= 0 ||
        result.ascent + result.descent <= 0)
        return false;
    *out = result;
    return true;
}
cui_size cui_font_measure_styled(void *font, cui_str s, float size, cui_text_style style) {
    cui_size result = {0, 0};
    cui_font *f = selected(font, style);
    hb_buffer_t *b = shape(f, s, size);
    if (!b)
        return result;
    unsigned n;
    hb_glyph_position_t *p = hb_buffer_get_glyph_positions(b, &n);
    for (unsigned i = 0; i < n; i++)
        result.width += (float)p[i].x_advance / 64;
    result.width = fabsf(result.width);
    result.height = (float)f->face->size->metrics.height / 64;
    hb_buffer_destroy(b);
    return result;
}
cui_size cui_font_measure(void *font, cui_str s, float size) {
    return cui_font_measure_styled(font, s, size, (cui_text_style){0});
}
bool cui_font_draw_styled(cui_font *font, cui_str s, float size, cui_text_style style, float x,
                          float y, cui_glyph_fn emit, void *user) {
    if (!isfinite(x) || !isfinite(y) || fabsf(x) > 1e6f || fabsf(y) > 1e6f)
        return false;
    cui_font *f = selected(font, style);
    hb_buffer_t *b = shape(f, s, size);
    if (!b || !emit) {
        if (b)
            hb_buffer_destroy(b);
        return false;
    }
    unsigned n;
    hb_glyph_position_t *p = hb_buffer_get_glyph_positions(b, &n);
    hb_glyph_info_t *g = hb_buffer_get_glyph_infos(b, NULL);
    float baseline = y + (float)f->face->size->metrics.ascender / 64;
    bool ok = true;
    for (unsigned i = 0; i < n; i++) {
        if (fabsf(x) > 1e7f || fabsf(baseline) > 1e7f) {
            ok = false;
            break;
        }
        if (FT_Load_Glyph(f->face, g[i].codepoint, FT_LOAD_DEFAULT) ||
            FT_Render_Glyph(f->face->glyph, FT_RENDER_MODE_NORMAL)) {
            ok = false;
            continue;
        }
        FT_GlyphSlot slot = f->face->glyph;
        emit(user, (int)lroundf(x + (float)p[i].x_offset / 64) + slot->bitmap_left,
             (int)lroundf(baseline - (float)p[i].y_offset / 64) - slot->bitmap_top,
             (int)slot->bitmap.width, (int)slot->bitmap.rows, slot->bitmap.pitch,
             slot->bitmap.buffer);
        x += (float)p[i].x_advance / 64;
        baseline -= (float)p[i].y_advance / 64;
    }
    hb_buffer_destroy(b);
    return ok;
}
bool cui_font_draw(cui_font *f, cui_str s, float size, float x, float y, cui_glyph_fn emit,
                   void *user) {
    return cui_font_draw_styled(f, s, size, (cui_text_style){0}, x, y, emit, user);
}
