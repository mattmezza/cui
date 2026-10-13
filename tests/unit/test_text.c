/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#ifndef CUI_TEXT_BIDI
#include <assert.h>
int main(void) {
    cui_text_layout *sentinel = (cui_text_layout *)(void *)&sentinel;
    cui_text_layout *before = sentinel;
    assert(cui_font_layout_line(NULL, (cui_str){0}, 0, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0, 0,
                                NULL, &sentinel) == CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(sentinel == before);
    return 0;
}
#else
#include <SheenBidi/SheenBidi.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>
#include <hb-ft.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *font_path(const char *env, const char *const *paths, size_t count) {
    const char *requested = getenv(env);
    if (requested) {
        cui_font *font = cui_font_open(requested);
        assert(font);
        cui_font_close(font);
        return requested;
    }
    for (size_t i = 0; i < count; ++i) {
        cui_font *font = cui_font_open(paths[i]);
        if (font) {
            cui_font_close(font);
            return paths[i];
        }
    }
    return NULL;
}
static cui_str string(const char *s) {
    return (cui_str){s, strlen(s)};
}
static size_t byte_at(cui_str text, size_t scalar) {
    size_t count = 0;
    for (size_t i = 0; i < text.length; ++i)
        if (((unsigned char)text.data[i] & 0xc0) != 0x80) {
            if (count++ == scalar)
                return i;
        }
    assert(count == scalar);
    return text.length;
}
static void close_float(float a, float b) {
    assert(fabsf(a - b) < 0.001f);
}
static void reference(cui_font *font, FT_Face face, hb_font_t *hb, cui_str text,
                      cui_bidi_direction direction, size_t first, size_t length) {
    cui_text_layout *layout = NULL;
    cui_text_layout_view view;
    size_t byte_start = byte_at(text, first),
           byte_end = length == SIZE_MAX ? text.length : byte_at(text, first + length);
    SBCodepointSequence sequence = {SBStringEncodingUTF8, text.data, text.length};
    SBAlgorithmRef algorithm = SBAlgorithmCreate(&sequence);
    SBLevel base = direction == CUI_BIDI_RTL   ? 1
                   : direction == CUI_BIDI_LTR ? 0
                                               : SBLevelDefaultLTR;
    SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, text.length, base);
    SBLineRef line = SBParagraphCreateLine(paragraph, byte_start, byte_end - byte_start);
    assert(line);
    assert(cui_font_layout_line(font, text, 24, (cui_text_style){0}, direction, first, length, NULL,
                                &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    assert(view.paragraph_level == SBParagraphGetBaseLevel(paragraph));
    assert(FT_Set_Char_Size(face, 0, 24 * 64, 72, 72) == 0);
    hb_ft_font_changed(hb);
    size_t run = 0, glyph = 0;
    float pen = 0;
    for (size_t r = 0; r < SBLineGetRunCount(line); ++r) {
        const SBRun *bidi = &SBLineGetRunsPtr(line)[r];
        size_t covered = 0,
               previous = (bidi->level & 1) ? bidi->offset + bidi->length : bidi->offset;
        while (covered < bidi->length) {
            assert(run < view.run_count);
            const cui_text_run *actual = &view.runs[run++];
            assert(actual->level == bidi->level && actual->byte_length > 0 &&
                   actual->glyph_start == glyph);
            if (bidi->level & 1) {
                assert(actual->byte_start + actual->byte_length == previous);
                previous = actual->byte_start;
            } else {
                assert(actual->byte_start == previous);
                previous += actual->byte_length;
            }
            covered += actual->byte_length;
            assert(covered <= bidi->length);
            hb_buffer_t *buffer = hb_buffer_create();
            hb_buffer_set_direction(buffer,
                                    (actual->level & 1) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
            hb_buffer_set_script(buffer, hb_script_from_iso15924_tag(actual->script));
            hb_buffer_set_language(buffer, hb_language_from_string("und", -1));
            hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
            hb_buffer_flags_t flags = HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES;
            if (actual->byte_start == byte_start)
                flags |= HB_BUFFER_FLAG_BOT;
            if (actual->byte_start + actual->byte_length == byte_end)
                flags |= HB_BUFFER_FLAG_EOT;
            hb_buffer_set_flags(buffer, flags);
            hb_buffer_add_utf8(buffer, text.data + byte_start, (int)(byte_end - byte_start),
                               (unsigned)(actual->byte_start - byte_start),
                               (int)actual->byte_length);
            hb_shape(hb, buffer, NULL, 0);
            assert(hb_buffer_allocation_successful(buffer));
            unsigned count;
            hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &count);
            hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, NULL);
            assert(actual->glyph_count == count);
            float advance = 0;
            for (unsigned i = 0; i < count; ++i)
                advance += (float)positions[i].x_advance / 64;
            float x = pen + (advance < 0 ? -advance : 0), y = 0;
            for (unsigned i = 0; i < count; ++i) {
                const cui_text_glyph *g = &view.glyphs[glyph++];
                assert(g->glyph_id == infos[i].codepoint &&
                       g->cluster_byte == infos[i].cluster + byte_start);
                assert(g->cluster_byte >= actual->byte_start &&
                       g->cluster_byte < actual->byte_start + actual->byte_length);
                close_float(g->x, x + (float)positions[i].x_offset / 64);
                close_float(g->y, y - (float)positions[i].y_offset / 64);
                close_float(g->x_advance, (float)positions[i].x_advance / 64);
                close_float(g->y_advance, -(float)positions[i].y_advance / 64);
                if (i) {
                    if (actual->level & 1)
                        assert(view.glyphs[glyph - 2].cluster_byte >= g->cluster_byte);
                    else
                        assert(view.glyphs[glyph - 2].cluster_byte <= g->cluster_byte);
                }
                x += (float)positions[i].x_advance / 64;
                y -= (float)positions[i].y_advance / 64;
            }
            pen += fabsf(advance);
            hb_buffer_destroy(buffer);
        }
    }
    assert(run == view.run_count && glyph == view.glyph_count);
    close_float(pen, view.width);
    close_float(view.height, (float)face->size->metrics.height / 64);
    cui_text_layout_destroy(layout);
    SBLineRelease(line);
    SBParagraphRelease(paragraph);
    SBAlgorithmRelease(algorithm);
}
typedef struct {
    uint64_t hash;
    size_t count;
} raster;
static void emit(void *user, int x, int y, int w, int h, int pitch, const unsigned char *pixels) {
    raster *r = user;
    r->count++;
    r->hash = r->hash * 31 + (unsigned)x;
    r->hash = r->hash * 31 + (unsigned)y;
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
            r->hash = r->hash * 31 + pixels[row * pitch + col];
}
static raster original_raster(FT_Face face, hb_font_t *font, cui_str text, float size, float x,
                              float y, cui_size *bounds) {
    raster pixels = {0};
    assert(!FT_Set_Char_Size(face, 0, (FT_F26Dot6)lroundf(size * 64), 72, 72));
    hb_ft_font_changed(font);
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, text.data, (int)text.length, 0, (int)text.length);
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(font, buffer, NULL, 0);
    unsigned count;
    hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &count);
    hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, NULL);
    bounds->width = 0;
    bounds->height = (float)face->size->metrics.height / 64;
    float baseline = y + (float)face->size->metrics.ascender / 64;
    for (unsigned i = 0; i < count; ++i) {
        assert(!FT_Load_Glyph(face, infos[i].codepoint, FT_LOAD_DEFAULT));
        assert(!FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));
        FT_GlyphSlot slot = face->glyph;
        emit(&pixels, (int)lroundf(x + (float)positions[i].x_offset / 64) + slot->bitmap_left,
             (int)lroundf(baseline - (float)positions[i].y_offset / 64) - slot->bitmap_top,
             (int)slot->bitmap.width, (int)slot->bitmap.rows, slot->bitmap.pitch,
             slot->bitmap.buffer);
        x += (float)positions[i].x_advance / 64;
        baseline -= (float)positions[i].y_advance / 64;
        bounds->width += (float)positions[i].x_advance / 64;
    }
    bounds->width = fabsf(bounds->width);
    hb_buffer_destroy(buffer);
    return pixels;
}
static void ordinary_lines(cui_font *font, FT_Face face, hb_font_t *hb) {
    static const char *ascii[] = {"WiiW", "ffi office", "AV 123 (abc)", "á"};
    for (size_t i = 0; i < sizeof ascii / sizeof ascii[0]; ++i) {
        cui_size expected;
        raster source = original_raster(face, hb, string(ascii[i]), 24, 3, 5, &expected),
               actual = {0};
        cui_size bounds = cui_font_measure(font, string(ascii[i]), 24);
        close_float(bounds.width, expected.width);
        close_float(bounds.height, expected.height);
        assert(cui_font_draw(font, string(ascii[i]), 24, 3, 5, emit, &actual));
        assert(actual.count == source.count && actual.hash == source.hash);
    }
    cui_str paragraph = string("אבג ab (123)");
    reference(font, face, hb, paragraph, CUI_BIDI_AUTO_LTR, 0, 6);
    reference(font, face, hb, paragraph, CUI_BIDI_AUTO_LTR, 7, SIZE_MAX);
    cui_text_layout *layouts[5] = {NULL};
    cui_text_layout_view views[5];
    assert(cui_font_layout_line(font, paragraph, 24, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0, 6,
                                NULL, &layouts[0]) == CUI_TEXT_LAYOUT_OK);
    assert(cui_font_layout_line(font, paragraph, 24, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 7,
                                SIZE_MAX, NULL, &layouts[1]) == CUI_TEXT_LAYOUT_OK);
    assert(cui_font_layout_line(font, string("abc العربية"), 24, (cui_text_style){0},
                                CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL,
                                &layouts[2]) == CUI_TEXT_LAYOUT_OK);
    for (size_t i = 3; i < 5; ++i)
        assert(cui_font_layout_line(font, string(""), 24, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0,
                                    SIZE_MAX, NULL, &layouts[i]) == CUI_TEXT_LAYOUT_OK);
    raster expected = {0}, actual = {0};
    float width = 0, height = 0;
    for (size_t i = 0; i < 5; ++i) {
        assert(cui_text_layout_get(layouts[i], &views[i]));
        if (views[i].width > width)
            width = views[i].width;
        assert(cui_text_layout_draw(layouts[i], 3, 5 + height, emit, &expected));
        height += views[i].height;
    }
    assert(views[0].paragraph_level == 1 && views[1].paragraph_level == 1 &&
           views[2].paragraph_level == 0);
    cui_str all = string("אבג ab (123)\nabc العربية\r\n\n");
    cui_size bounds = cui_font_measure(font, all, 24);
    close_float(bounds.width, width);
    close_float(bounds.height, height);
    assert(cui_font_draw(font, all, 24, 3, 5, emit, &actual));
    assert(actual.count == expected.count && actual.hash == expected.hash);
    for (size_t i = 0; i < 5; ++i)
        cui_text_layout_destroy(layouts[i]);
    /* Every P1 paragraph separator starts a new baseline; CRLF is one break. */
    static const char *breaks[] = {"\n",   "\r",       "\r\n",         "\x1c",        "\x1d",
                                   "\x1e", "\xc2\x85", "\xe2\x80\xa9", "\xe2\x80\xa8"};
    cui_font_metrics metrics;
    assert(cui_font_get_metrics(font, 24, (cui_text_style){0}, &metrics));
    for (size_t i = 0; i < sizeof breaks / sizeof breaks[0]; ++i) {
        bounds = cui_font_measure(font, string(breaks[i]), 24);
        close_float(bounds.width, 0);
        close_float(bounds.height, 2 * metrics.line_height);
        raster blank = {0};
        assert(cui_font_draw(font, string(breaks[i]), 24, 0, 0, emit, &blank) && blank.count == 0);
    }
    raster invalid = {0};
    assert(!cui_font_draw(font, string("abc\n\xc0\xaf"), 24, 0, 0, emit, &invalid) &&
           invalid.count == 0);
    bounds = cui_font_measure(font, string("abc\n\xc0\xaf"), 24);
    assert(bounds.width == 0 && bounds.height == 0);
}

static void arabic_clusters(void) {
    const char *const paths[] = {"/usr/share/fonts/noto/NotoSansArabic-Regular.ttf",
                                 "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                 "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    const char *path = font_path("CUI_TEST_ARABIC_FONT", paths, sizeof paths / sizeof paths[0]);
    if (!path) {
        puts("Arabic joining test skipped: set CUI_TEST_ARABIC_FONT");
        return;
    }
    cui_font *font = cui_font_open(path);
    assert(font);
    FT_Library library;
    FT_Face face;
    assert(!FT_Init_FreeType(&library));
    assert(!FT_New_Face(library, path, 0, &face));
    static const uint32_t needed[] = {0x627, 0x644, 0x633, 0x64e, 0x651, 0x645, 0x64f};
    for (size_t i = 0; i < sizeof needed / sizeof needed[0]; ++i)
        if (!FT_Get_Char_Index(face, needed[i])) {
            printf("Arabic joining/mark raster untested: selected font %s lacks U+%04X; set "
                   "CUI_TEST_ARABIC_FONT\n",
                   path, (unsigned)needed[i]);
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            cui_font_close(font);
            return;
        }
    hb_font_t *hb = hb_ft_font_create_referenced(face);
    cui_str text = string("السَّلَامُ");
    reference(font, face, hb, text, CUI_BIDI_RTL, 0, SIZE_MAX);
    cui_text_layout *layout = NULL;
    cui_text_layout_view view;
    assert(cui_font_layout_line(font, text, 24, (cui_text_style){0}, CUI_BIDI_RTL, 0, SIZE_MAX,
                                "ar", &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    bool repeated = false;
    for (size_t i = 0; i < view.glyph_count; ++i) {
        assert(view.glyphs[i].glyph_id != 0);
        if (i && view.glyphs[i - 1].cluster_byte == view.glyphs[i].cluster_byte)
            repeated = true;
    }
    assert(repeated && view.run_count == 1 && view.runs[0].script == HB_SCRIPT_ARABIC);
    raster pixels = {0};
    assert(cui_text_layout_draw(layout, 0, 0, emit, &pixels));
    assert(pixels.count == view.glyph_count);
    raster ordinary = {0};
    assert(cui_font_draw(font, text, 24, 0, 0, emit, &ordinary));
    assert(ordinary.hash == pixels.hash && ordinary.count == pixels.count);
    cui_size ordinary_size = cui_font_measure(font, text, 24);
    close_float(ordinary_size.width, view.width);
    close_float(ordinary_size.height, view.height);
    cui_text_layout_destroy(layout);
    /* Selected-face style layouts shape with the actual registered face. */
    cui_font *root = cui_font_open(path);
    assert(root);
    cui_text_style style = {.family = 37};
    assert(cui_font_set_style(root, style, font));
    assert(cui_font_layout_line(root, text, 24, style, CUI_BIDI_RTL, 0, SIZE_MAX, "ar", &layout) ==
           CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    raster styled = {0};
    assert(cui_text_layout_draw(layout, 0, 0, emit, &styled));
    assert(styled.hash == pixels.hash && styled.count == pixels.count);
    cui_text_layout_destroy(layout);
    cui_font_close(root);
    hb_font_destroy(hb);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    cui_font_close(font);
    printf("Arabic joining/mark raster passed using %s\n", path);
}

int main(void) {
    const char *const paths[] = {"/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                 "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                 "/usr/share/fonts/TTF/DejaVuSans.ttf",
                                 "/usr/share/fonts/liberation/LiberationSans-Regular.ttf"};
    const char *path = font_path("CUI_TEST_FONT", paths, sizeof paths / sizeof paths[0]);
    if (!path) {
        puts("text layout test needs CUI_TEST_FONT");
        return 77;
    }
    cui_font *font = cui_font_open(path);
    assert(font);
    FT_Library library;
    FT_Face face;
    assert(!FT_Init_FreeType(&library));
    assert(!FT_New_Face(library, path, 0, &face));
    hb_font_t *hb = hb_ft_font_create_referenced(face);
    reference(font, face, hb, string("abc אבג العربية 123 (xy)"), CUI_BIDI_AUTO_LTR, 0, SIZE_MAX);
    reference(font, face, hb, string("אבג العربية"), CUI_BIDI_RTL, 0, SIZE_MAX);
    reference(font, face, hb, string("abc ⁧العربية 12 אבג⁩ (xy)"), CUI_BIDI_AUTO_LTR,
              0, SIZE_MAX);
    reference(font, face, hb, string("אבג ab (123) العربية"), CUI_BIDI_AUTO_LTR, 4, 8);
    reference(font, face, hb, string("אבג ab (123) العربية"), CUI_BIDI_AUTO_LTR, 12, SIZE_MAX);
    reference(font, face, hb, string("ffi á office"), CUI_BIDI_LTR, 0, SIZE_MAX);
    cui_text_layout *layout = NULL;
    cui_text_layout_view view;
    assert(cui_font_layout_line(font, string("אבג العربية"), 24, (cui_text_style){0}, CUI_BIDI_RTL,
                                0, SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    assert(view.run_count == 2);
    assert(view.runs[0].script == HB_SCRIPT_ARABIC && view.runs[1].script == HB_SCRIPT_HEBREW);
    cui_text_layout *sentinel = layout;
    assert(cui_font_layout_line(font, string("a\nb"), 24, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0,
                                SIZE_MAX, NULL, &sentinel) == CUI_TEXT_LAYOUT_INVALID &&
           sentinel == layout);
    assert(cui_font_layout_line(font, string("\xc0\xaf"), 24, (cui_text_style){0},
                                CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL,
                                &sentinel) == CUI_TEXT_LAYOUT_INVALID &&
           sentinel == layout);
    assert(cui_font_layout_line(font, string("abc"), NAN, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0,
                                SIZE_MAX, NULL, &sentinel) == CUI_TEXT_LAYOUT_INVALID &&
           sentinel == layout);
    assert(cui_font_layout_line(font, string("abc"), 24, (cui_text_style){0}, CUI_BIDI_AUTO_LTR,
                                SIZE_MAX, 1, NULL, &sentinel) == CUI_TEXT_LAYOUT_INVALID &&
           sentinel == layout);
    cui_text_layout_destroy(layout);
    assert(cui_font_layout_line(font, string("WiiW"), 24, (cui_text_style){0}, CUI_BIDI_LTR, 0,
                                SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    cui_size size = cui_font_measure(font, string("WiiW"), 24);
    close_float(size.width, view.width);
    close_float(size.height, view.height);
    raster plain = {0}, positioned = {0};
    assert(cui_font_draw(font, string("WiiW"), 24, 3, 5, emit, &plain));
    assert(cui_text_layout_draw(layout, 3, 5, emit, &positioned));
    assert(plain.count == positioned.count && plain.hash == positioned.hash);
    cui_font_metrics metrics;
    assert(cui_font_get_metrics(font, 24, (cui_text_style){0}, &metrics));
    close_float(view.ascent, metrics.ascent);
    cui_text_layout_destroy(layout);
    assert(cui_font_layout_line(font, string(""), 24, (cui_text_style){0}, CUI_BIDI_AUTO_RTL, 0,
                                SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view) && view.glyph_count == 0 &&
           view.paragraph_level == 1);
    cui_text_layout_destroy(layout);
    cui_text_layout_destroy(NULL);
    ordinary_lines(font, face, hb);
    hb_font_destroy(hb);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    cui_font_close(font);
    arabic_clusters();
    puts("bidirectional shaping and raster regression passed");
    return 0;
}

#endif
