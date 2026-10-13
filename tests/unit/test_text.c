/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#ifndef CUI_TEXT_BIDI
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    cui_text_layout *sentinel = (cui_text_layout *)(void *)&sentinel;
    cui_text_layout *before = sentinel;
    assert(cui_font_layout_line(NULL, (cui_str){0}, 0, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, 0, 0,
                                NULL, &sentinel) == CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(sentinel == before);
    cui_text_geometry unavailable, original_geometry;
    memset(&unavailable, 0x37, sizeof unavailable);
    original_geometry = unavailable;
    assert(cui_font_text_geometry(NULL, (cui_str){0}, 24, (cui_text_style){0}, &unavailable) ==
           CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(!memcmp(&unavailable, &original_geometry, sizeof unavailable));
    size_t unchanged = 123;
    cui_text_caret caret = {0}, original = caret;
    assert(cui_text_layout_carets(NULL, 0, &caret, 1, &unchanged) == CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(unchanged == 123 && caret.x == original.x);
    assert(cui_text_layout_hit_test(NULL, 0, 0, CUI_TEXT_DOWNSTREAM, &caret) ==
           CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(cui_text_layout_selection(NULL, 0, 0, NULL, 0, &unchanged) ==
           CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(unchanged == 123);
    const char *requested = getenv("CUI_TEST_FONT");
    const char *const paths[] = {"/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                 "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                 "/usr/share/fonts/TTF/DejaVuSans.ttf",
                                 "/usr/share/fonts/liberation/LiberationSans-Regular.ttf"};
    const char *path = requested;
    cui_font *root = requested ? cui_font_open(requested) : NULL;
    if (requested)
        assert(root);
    for (size_t i = 0; !root && i < sizeof paths / sizeof paths[0]; ++i) {
        root = cui_font_open(paths[i]);
        if (root)
            path = paths[i];
    }
    if (root) {
        cui_font *fallback = cui_font_open(path);
        assert(fallback);
        cui_font *chain[] = {fallback};
        assert(cui_font_set_fallbacks(root, chain, 1));
        assert(!cui_font_set_fallbacks(root, chain, 33));
        const char *owned[] = {path};
        assert(cui_font_register_fallbacks(root, owned, 1));
        assert(!cui_font_register_fallbacks(root, NULL, 1));
        assert(cui_font_set_fallbacks(root, NULL, 0));
        cui_font_close(root);
        cui_font_close(fallback);
    } else
        puts("BIDI-off fallback registration font lifetime check untested: set CUI_TEST_FONT");
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
typedef struct {
    cui_font *font;
    FT_Face face;
    hb_font_t *hb;
} reference_font;
static reference_font fallback_references[8];
static size_t fallback_reference_count;
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
    if (fabsf(a - b) >= 0.001f) {
        fprintf(stderr, "float mismatch %g expected %g\n", (double)a, (double)b);
        abort();
    }
}
typedef struct {
    uint64_t hash;
    size_t count;
} raster;
static void emit(void *user, int x, int y, int w, int h, int pitch, const unsigned char *pixels);
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
    raster expected_pixels = {0}, actual_pixels = {0};
    float expected_ascent = (float)face->size->metrics.ascender / 64;
    float expected_descent = -(float)face->size->metrics.descender / 64;
    float expected_height = (float)face->size->metrics.height / 64;
    bool used_fallback = false;
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
            FT_Face run_face = face;
            hb_font_t *run_hb = hb;
            if (actual->face_index) {
                bool found = false;
                for (size_t f = 0; f < fallback_reference_count; ++f)
                    if (fallback_references[f].font == view.faces[actual->face_index]) {
                        run_face = fallback_references[f].face;
                        run_hb = fallback_references[f].hb;
                        found = true;
                        break;
                    }
                assert(found);
                assert(!FT_Set_Char_Size(run_face, 0, 24 * 64, 72, 72));
                hb_ft_font_changed(run_hb);
            }
            if (actual->face_index)
                used_fallback = true;
            float ascent = (float)run_face->size->metrics.ascender / 64;
            float descent = -(float)run_face->size->metrics.descender / 64;
            float height = (float)run_face->size->metrics.height / 64;
            if (ascent > expected_ascent)
                expected_ascent = ascent;
            if (descent > expected_descent)
                expected_descent = descent;
            if (height > expected_height)
                expected_height = height;
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
            hb_shape(run_hb, buffer, NULL, 0);
            assert(hb_buffer_allocation_successful(buffer));
            unsigned count;
            hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &count);
            hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, NULL);
            assert(actual->glyph_count == count);
            float advance = 0;
            for (unsigned i = 0; i < count; ++i)
                advance += (float)positions[i].x_advance / 64;
            float x = pen + (advance < 0 ? -advance : 0), y = 0;
            unsigned group = 0;
            float group_x = x;
            for (unsigned i = 0; i < count; ++i) {
                if (!i || infos[i].cluster != infos[i - 1].cluster) {
                    group = i;
                    group_x = x;
                }
                const cui_text_glyph *g = &view.glyphs[glyph++];
                assert(g->glyph_id != 0);
                assert(g->glyph_id == infos[i].codepoint &&
                       g->cluster_byte == infos[i].cluster + byte_start);
                assert(g->cluster_byte >= actual->byte_start &&
                       g->cluster_byte < actual->byte_start + actual->byte_length);
                close_float(g->x, x + (float)positions[i].x_offset / 64);
                close_float(g->y, y - (float)positions[i].y_offset / 64);
                close_float(g->x_advance, (float)positions[i].x_advance / 64);
                close_float(g->y_advance, -(float)positions[i].y_advance / 64);
                assert(!FT_Load_Glyph(run_face, infos[i].codepoint, FT_LOAD_DEFAULT));
                assert(!FT_Render_Glyph(run_face->glyph, FT_RENDER_MODE_NORMAL));
                FT_GlyphSlot slot = run_face->glyph;
                emit(&expected_pixels, (int)lroundf(g->x) + slot->bitmap_left,
                     (int)lroundf(view.ascent + g->y) - slot->bitmap_top, (int)slot->bitmap.width,
                     (int)slot->bitmap.rows, slot->bitmap.pitch, slot->bitmap.buffer);
                if (i) {
                    if (actual->level & 1)
                        assert(view.glyphs[glyph - 2].cluster_byte >= g->cluster_byte);
                    else
                        assert(view.glyphs[glyph - 2].cluster_byte <= g->cluster_byte);
                }
                x += (float)positions[i].x_advance / 64;
                y -= (float)positions[i].y_advance / 64;
                if (i + 1 == count || infos[i + 1].cluster != infos[i].cluster) {
                    const cui_text_cluster *cluster = NULL;
                    for (size_t c = 0; c < view.cluster_count; ++c)
                        if (view.clusters[c].run_index == run - 1 &&
                            view.clusters[c].glyph_start == actual->glyph_start + group)
                            cluster = &view.clusters[c];
                    assert(cluster && cluster->glyph_count == i - group + 1);
                    close_float(cluster->left, fminf(group_x, x));
                    close_float(cluster->right, fmaxf(group_x, x));
                    size_t begin = infos[group].cluster + byte_start;
                    size_t end = (actual->level & 1)
                                     ? (group ? infos[group - 1].cluster + byte_start
                                              : actual->byte_start + actual->byte_length)
                                     : (i + 1 < count ? infos[i + 1].cluster + byte_start
                                                      : actual->byte_start + actual->byte_length);
                    if (((actual->level & 1) && i + 1 == count) || (!(actual->level & 1) && !group))
                        begin = actual->byte_start;
                    assert(cluster->byte_start == begin && cluster->byte_end == end);
                }
            }
            pen += fabsf(advance);
            hb_buffer_destroy(buffer);
        }
    }
    assert(run == view.run_count && glyph == view.glyph_count);
    assert(view.byte_start == byte_start && view.byte_end == byte_end);
    for (size_t c = 0; c < view.cluster_count; ++c) {
        size_t edge_count = 0;
        assert(cui_text_layout_carets(layout, view.clusters[c].byte_start, NULL, 0, &edge_count) ==
                   CUI_TEXT_LAYOUT_OK &&
               edge_count);
    }
    close_float(pen, view.width);
    if (used_fallback && expected_ascent + expected_descent > expected_height)
        expected_height = expected_ascent + expected_descent;
    close_float(view.ascent, expected_ascent);
    close_float(view.height, expected_height);
    assert(cui_text_layout_draw(layout, 0, 0, emit, &actual_pixels));
    assert(expected_pixels.hash == actual_pixels.hash &&
           expected_pixels.count == actual_pixels.count);
    cui_text_layout_destroy(layout);
    SBLineRelease(line);
    SBParagraphRelease(paragraph);
    SBAlgorithmRelease(algorithm);
}
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

static void fallback_contracts(const char *primary_path, cui_font *const *fallbacks, size_t count,
                               const char *arabic_path) {
    FT_Library probe_library;
    FT_Face probe;
    assert(!FT_Init_FreeType(&probe_library));
    assert(!FT_New_Face(probe_library, primary_path, 0, &probe));
    bool needs_arabic = !FT_Get_Char_Index(probe, 0x627);
    FT_Done_Face(probe);
    FT_Done_FreeType(probe_library);
    cui_font *root = cui_font_open(primary_path);
    assert(root);
    cui_text_layout *layout = NULL, *sentinel = NULL;
    cui_text_layout_view view;
    assert(cui_font_set_fallbacks(root, fallbacks, count));
    assert(!cui_font_set_fallbacks(root, fallbacks, 33));
    cui_font *bad[] = {root};
    assert(!cui_font_set_fallbacks(root, bad, 1));
    cui_font *duplicates[] = {fallbacks[0], fallbacks[0]};
    assert(!cui_font_set_fallbacks(root, duplicates, 2));
    assert(!cui_font_set_fallbacks(root, NULL, 1));
    assert(cui_font_layout_line(root, string("abc אבג السَّلَامُ"), 24, (cui_text_style){0},
                                CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL,
                                &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    for (size_t i = 0; i < view.glyph_count; ++i)
        assert(view.glyphs[i].glyph_id);
    for (size_t r = 0; r < view.run_count; ++r)
        assert(view.runs[r].face_index < view.face_count);
    raster before = {0}, after = {0};
    assert(cui_text_layout_draw(layout, 3, 5, emit, &before));
    /* A failed owned registration must retain the working borrowed chain. */
    const char *invalid_paths[] = {arabic_path, "/cui-test-nonexistent-font.ttf"};
    assert(!cui_font_register_fallbacks(root, invalid_paths, 2));
    assert(cui_font_layout_line(root, string("abc אבג السَّلَامُ"), 24, (cui_text_style){0},
                                CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL,
                                &sentinel) == CUI_TEXT_LAYOUT_OK);
    cui_text_layout_destroy(sentinel);
    assert(cui_font_set_fallbacks(root, NULL, 0));
    cui_font_close(root);
    assert(cui_text_layout_draw(layout, 3, 5, emit, &after));
    assert(before.hash == after.hash && before.count == after.count);
    cui_text_layout_destroy(layout);
    /* Owned fallback replacement/root close cannot invalidate the retained face. */
    root = cui_font_open(primary_path);
    assert(root);
    const char *owned[] = {arabic_path};
    assert(cui_font_register_fallbacks(root, owned, 1));
    assert(cui_font_layout_line(root, string("السَّلَامُ"), 24, (cui_text_style){0}, CUI_BIDI_RTL, 0,
                                SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    before = (raster){0};
    after = (raster){0};
    assert(cui_text_layout_draw(layout, 0, 0, emit, &before));
    assert(cui_font_register_fallbacks(root, NULL, 0));
    cui_font_close(root);
    cui_text_caret retained_edge;
    assert(cui_text_layout_hit_test(layout, 0, 0, CUI_TEXT_DOWNSTREAM, &retained_edge) ==
           CUI_TEXT_LAYOUT_OK);
    size_t retained_count;
    assert(cui_text_layout_selection(layout, view.byte_start, view.byte_end, NULL, 0,
                                     &retained_count) == CUI_TEXT_LAYOUT_OK &&
           retained_count == view.cluster_count);
    assert(cui_text_layout_draw(layout, 0, 0, emit, &after));
    assert(before.hash == after.hash && before.count == after.count);
    cui_text_layout_destroy(layout);
    /* Default-ignorables need no standalone glyph, and remain in HB input to
     * affect Arabic joining/marks/variation-selector cluster formation. */
    root = cui_font_open(primary_path);
    assert(root);
    assert(cui_font_set_fallbacks(root, fallbacks, count));
    assert(cui_font_layout_line(root, string("ل‍ا‌مَ️͏"), 24, (cui_text_style){0},
                                CUI_BIDI_RTL, 0, SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    for (size_t i = 0; i < view.glyph_count; ++i)
        assert(view.glyphs[i].glyph_id);
    cui_text_layout_destroy(layout);
    assert(cui_font_layout_line(root, string("A️"), 24, (cui_text_style){0}, CUI_BIDI_LTR, 0,
                                SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    sentinel = layout;
    assert(cui_font_layout_line(root, string("\xf4\x8f\xbf\xbf"), 24, (cui_text_style){0},
                                CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL,
                                &sentinel) == CUI_TEXT_LAYOUT_MISSING_GLYPH &&
           sentinel == layout);
    raster missing = {0};
    assert(!cui_font_draw(root, string("abc\n\xf4\x8f\xbf\xbf"), 24, 0, 0, emit, &missing) &&
           missing.count == 0);
    cui_size bounds = cui_font_measure(root, string("\xf4\x8f\xbf\xbf"), 24);
    assert(bounds.width == 0 && bounds.height == 0);
    cui_text_layout_destroy(layout);
    cui_font_close(root);
    /* Fallback style registration selects a whole actual bold face; layouts
     * retain it even when the style is removed and source fonts are closed. */
    if (needs_arabic) {
        root = cui_font_open(primary_path);
        cui_font *arabic = cui_font_open(arabic_path), *bold = cui_font_open(arabic_path);
        assert(root && arabic && bold);
        cui_font *ordered[] = {arabic, bold};
        assert(cui_font_set_fallbacks(root, ordered, 2));
        assert(cui_font_layout_line(root, string("العربية"), 24, (cui_text_style){0}, CUI_BIDI_RTL,
                                    0, SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
        assert(cui_text_layout_get(layout, &view));
        assert(view.faces[view.runs[0].face_index] == arabic);
        cui_text_layout_destroy(layout);
        ordered[0] = bold;
        ordered[1] = arabic;
        assert(cui_font_set_fallbacks(root, ordered, 2));
        assert(cui_font_layout_line(root, string("العربية"), 24, (cui_text_style){0}, CUI_BIDI_RTL,
                                    0, SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
        assert(cui_text_layout_get(layout, &view));
        assert(view.faces[view.runs[0].face_index] == bold);
        cui_text_layout_destroy(layout);
        cui_text_style style = {.weight = CUI_TEXT_WEIGHT_BOLD};
        assert(cui_font_set_style(arabic, style, bold));
        cui_font *chain[] = {arabic};
        assert(cui_font_set_fallbacks(root, chain, 1));
        assert(cui_font_layout_line(root, string("السَّلَامُ"), 24, style, CUI_BIDI_RTL, 0, SIZE_MAX,
                                    NULL, &layout) == CUI_TEXT_LAYOUT_OK);
        assert(cui_text_layout_get(layout, &view));
        assert(view.faces[view.runs[0].face_index] == bold);
        assert(cui_font_set_style(arabic, style, NULL));
        assert(cui_font_set_fallbacks(root, NULL, 0));
        cui_font_close(root);
        cui_font_close(arabic);
        cui_font_close(bold);
        after = (raster){0};
        assert(cui_text_layout_draw(layout, 0, 0, emit, &after) && after.count == view.glyph_count);
        cui_text_layout_destroy(layout);
    } else
        puts("primary covers Arabic; fallback first-fit/style-face case untested");
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

static void visual_factory_contract(const char *primary, const char *arabic) {
    cui_font *font = cui_font_open(primary);
    assert(font);
    const char *paths[] = {arabic};
    assert(cui_font_register_fallbacks(font, paths, 1));
    char original[] = "السَّلَامُ";
    cui_text_geometry geometry, untouched;
    memset(&geometry, 0x37, sizeof geometry);
    untouched = geometry;
    assert(cui_font_text_geometry(font, string("a\n b"), 24, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(!memcmp(&geometry, &untouched, sizeof geometry));
    assert(cui_font_text_geometry(font, string("a b"), 24, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(!memcmp(&geometry, &untouched, sizeof geometry));
    assert(cui_font_text_geometry(font, string(original), NAN, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(!memcmp(&geometry, &untouched, sizeof geometry));
    cui_text_style style = {.weight = CUI_TEXT_WEIGHT_BOLD, .italic = true};
    assert(cui_font_text_geometry(font, string(original), 24, style, &geometry) ==
           CUI_TEXT_LAYOUT_OK);
    cui_size measured = cui_font_measure_styled(font, string(original), 24, style);
    close_float(geometry.width, measured.width);
    close_float(geometry.height, measured.height);
    size_t byte_length = strlen(original);
    memset(original, 'x', byte_length);
    assert(cui_font_register_fallbacks(font, NULL, 0));
    cui_font_close(font);
    /* Querying the retained geometry never calls the now-closed host font. */
    cui_text_caret edge, before;
    memset(&edge, 0x51, sizeof edge);
    before = edge;
    size_t count = 991;
    assert(geometry.ops->carets(geometry.session, 0, &edge, 0, &count) == CUI_TEXT_LAYOUT_CAPACITY);
    assert(count == 991 && !memcmp(&edge, &before, sizeof edge));
    assert(geometry.ops->carets(geometry.session, 1, &edge, 1, &count) == CUI_TEXT_LAYOUT_INVALID);
    assert(count == 991 && !memcmp(&edge, &before, sizeof edge));
    assert(geometry.ops->carets(geometry.session, 0, &edge, 1, &count) == CUI_TEXT_LAYOUT_OK);
    close_float(edge.x, geometry.width);
    cui_text_selection_rect selected, prior;
    memset(&selected, 0x51, sizeof selected);
    prior = selected;
    count = 991;
    assert(geometry.ops->selection(geometry.session, 0, byte_length, &selected, 0, &count) ==
           CUI_TEXT_LAYOUT_CAPACITY);
    assert(count == 991 && !memcmp(&selected, &prior, sizeof selected));
    before = edge;
    assert(geometry.ops->hit_test(geometry.session, INFINITY, 0, CUI_TEXT_DOWNSTREAM, &edge) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(!memcmp(&edge, &before, sizeof edge));
    cui_release_text_geometry(&geometry);
    cui_release_text_geometry(&geometry);
    font = cui_font_open(primary);
    assert(font);
    assert(cui_font_text_geometry(font, string(""), 24, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_OK);
    cui_text_caret empty[2];
    assert(geometry.ops->carets(geometry.session, 0, empty, 2, &count) == CUI_TEXT_LAYOUT_OK &&
           count == 2);
    assert(empty[0].x == 0 && empty[1].x == 0 && geometry.byte_length == 0);
    cui_font_close(font);
    cui_release_text_geometry(&geometry);
}

static void geometry(cui_font *font) {
    cui_text_layout *layout = NULL;
    cui_text_layout_view view;
    cui_str text = string("abc אבג 123 xyz");
    assert(cui_font_layout_line(font, text, 24, (cui_text_style){0}, CUI_BIDI_LTR, 0, SIZE_MAX,
                                NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_get(layout, &view));
    assert(view.byte_start == 0 && view.byte_end == text.length);
    cui_text_caret carets[4], sentinel[4];
    memset(carets, 0x37, sizeof carets);
    memcpy(sentinel, carets, sizeof carets);
    size_t count = 991;
    assert(cui_text_layout_carets(layout, 5, carets, 4, &count) == CUI_TEXT_LAYOUT_INVALID);
    assert(count == 991 && !memcmp(carets, sentinel, sizeof carets));
    assert(cui_text_layout_carets(layout, 4, NULL, 0, &count) == CUI_TEXT_LAYOUT_OK && count == 2);
    count = 991;
    assert(cui_text_layout_carets(layout, 4, carets, 1, &count) == CUI_TEXT_LAYOUT_CAPACITY);
    assert(count == 991 && !memcmp(carets, sentinel, sizeof carets));
    assert(cui_text_layout_carets(layout, 4, carets, 4, &count) == CUI_TEXT_LAYOUT_OK &&
           count == 2);
    assert(carets[0].x != carets[1].x && carets[0].affinity != carets[1].affinity);
    cui_text_caret hit = carets[0], prior = hit;
    assert(cui_text_layout_hit_test(layout, NAN, 0, CUI_TEXT_UPSTREAM, &hit) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(!memcmp(&hit, &prior, sizeof hit));
    assert(cui_text_layout_hit_test(layout, 0, INFINITY, CUI_TEXT_UPSTREAM, &hit) ==
           CUI_TEXT_LAYOUT_INVALID);
    for (size_t c = 0; c < view.cluster_count; ++c) {
        const cui_text_cluster *cluster = &view.clusters[c];
        bool rtl = (view.runs[cluster->run_index].level & 1) != 0;
        assert(cui_text_layout_hit_test(layout, rtl ? cluster->right : cluster->left, 0,
                                        CUI_TEXT_DOWNSTREAM, &hit) == CUI_TEXT_LAYOUT_OK);
        close_float(hit.x, rtl ? cluster->right : cluster->left);
        assert(hit.affinity == CUI_TEXT_DOWNSTREAM);
    }
    cui_text_selection_rect selected[64], reversed[64];
    assert(cui_text_layout_selection(layout, 0, text.length, NULL, 0, &count) ==
           CUI_TEXT_LAYOUT_OK);
    assert(count == view.cluster_count && count < 64);
    size_t reverse_count;
    assert(cui_text_layout_selection(layout, 0, text.length, selected, 64, &count) ==
           CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_selection(layout, text.length, 0, reversed, 64, &reverse_count) ==
           CUI_TEXT_LAYOUT_OK);
    assert(count == reverse_count);
    for (size_t c = 0; c < count; ++c) {
        assert(selected[c].byte_start == view.clusters[c].byte_start);
        assert(selected[c].byte_end == reversed[c].byte_end);
        close_float(selected[c].bounds.x, view.clusters[c].left);
        close_float(selected[c].bounds.w, view.clusters[c].right - view.clusters[c].left);
        close_float(selected[c].bounds.h, view.height);
    }
    /* Selecting through the RTL letter sequence excludes the embedded number
     * run, so the selected logical prefix has separated visual coverage. */
    size_t partial_count;
    assert(cui_text_layout_selection(layout, 0, 10, reversed, 64, &partial_count) ==
           CUI_TEXT_LAYOUT_OK);
    bool gap = false;
    for (size_t c = 1; c < partial_count; ++c)
        if (reversed[c].bounds.x > reversed[c - 1].bounds.x + reversed[c - 1].bounds.w + .01f)
            gap = true;
    assert(gap);
    memcpy(reversed, selected, sizeof selected);
    reverse_count = 991;
    assert(cui_text_layout_selection(layout, 0, text.length, selected, count - 1, &reverse_count) ==
           CUI_TEXT_LAYOUT_CAPACITY);
    assert(reverse_count == 991 && !memcmp(selected, reversed, sizeof selected));
    assert(cui_text_layout_selection(layout, 5, text.length, selected, 64, &reverse_count) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(reverse_count == 991 && !memcmp(selected, reversed, sizeof selected));
    cui_text_layout_destroy(layout);
    const char *samples[] = {"á", "ffi", "a‍️͏b", "‎"};
    for (size_t s = 0; s < sizeof samples / sizeof samples[0]; ++s) {
        assert(cui_font_layout_line(font, string(samples[s]), 24, (cui_text_style){0}, CUI_BIDI_LTR,
                                    0, SIZE_MAX, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
        assert(cui_text_layout_get(layout, &view));
        for (size_t c = 0; c < view.cluster_count; ++c)
            for (size_t byte = view.clusters[c].byte_start + 1; byte < view.clusters[c].byte_end;
                 ++byte) {
                count = 991;
                assert(cui_text_layout_carets(layout, byte, carets, 4, &count) ==
                       CUI_TEXT_LAYOUT_INVALID);
                assert(count == 991);
            }
        if (s == 0)
            assert(view.cluster_count == 1 && view.clusters[0].byte_end == 3);
        if (s == 3) {
            assert(view.cluster_count == 1 && view.clusters[0].left == view.clusters[0].right);
            assert(cui_text_layout_hit_test(layout, 0, 0, CUI_TEXT_UPSTREAM, &hit) ==
                   CUI_TEXT_LAYOUT_OK);
            assert(hit.byte_offset == 3 && hit.affinity == CUI_TEXT_UPSTREAM);
        }
        cui_text_layout_destroy(layout);
    }
    assert(cui_font_layout_line(font, string("abc"), 24, (cui_text_style){0}, CUI_BIDI_LTR, 2, 0,
                                NULL, &layout) == CUI_TEXT_LAYOUT_OK);
    assert(cui_text_layout_carets(layout, 2, carets, 4, &count) == CUI_TEXT_LAYOUT_OK &&
           count == 2);
    assert(carets[0].x == 0 && carets[1].x == 0);
    assert(cui_text_layout_hit_test(layout, 1e30f, -1e30f, CUI_TEXT_DOWNSTREAM, &hit) ==
           CUI_TEXT_LAYOUT_OK);
    assert(hit.byte_offset == 2 && hit.affinity == CUI_TEXT_DOWNSTREAM);
    assert(cui_text_layout_selection(layout, 2, 2, NULL, 0, &count) == CUI_TEXT_LAYOUT_OK &&
           !count);
    cui_text_layout_destroy(layout);
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
    const char *const arabic_paths[] = {"/usr/share/fonts/noto/NotoSansArabic-Regular.ttf",
                                        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                        "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    const char *const hebrew_paths[] = {"/usr/share/fonts/noto/NotoSansHebrew-Regular.ttf",
                                        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                        "/usr/share/fonts/TTF/DejaVuSans.ttf",
                                        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf"};
    const char *fallback_paths[2] = {font_path("CUI_TEST_ARABIC_FONT", arabic_paths, 3),
                                     font_path("CUI_TEST_HEBREW_FONT", hebrew_paths, 4)};
    cui_font *fallbacks[2] = {NULL};
    for (size_t i = 0; i < 2; ++i)
        if (fallback_paths[i]) {
            cui_font *fallback = cui_font_open(fallback_paths[i]);
            assert(fallback);
            fallbacks[fallback_reference_count] = fallback;
            reference_font *ref = &fallback_references[fallback_reference_count++];
            ref->font = fallback;
            assert(!FT_New_Face(library, fallback_paths[i], 0, &ref->face));
            ref->hb = hb_ft_font_create_referenced(ref->face);
        }
    if (!fallback_paths[0] || !fallback_paths[1]) {
        puts("mixed-script font coverage untested: set CUI_TEST_ARABIC_FONT and "
             "CUI_TEST_HEBREW_FONT");
        for (size_t i = 0; i < fallback_reference_count; ++i) {
            hb_font_destroy(fallback_references[i].hb);
            FT_Done_Face(fallback_references[i].face);
            cui_font_close(fallbacks[i]);
        }
        hb_font_destroy(hb);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        cui_font_close(font);
        return 77;
    }
    assert(cui_font_set_fallbacks(font, fallbacks, fallback_reference_count));
    fallback_contracts(path, fallbacks, fallback_reference_count, fallback_paths[0]);
    geometry(font);
    visual_factory_contract(path, fallback_paths[0]);
    if (!FT_Get_Char_Index(face, 0x627)) {
        cui_font *without = cui_font_open(path);
        cui_text_layout *missing = NULL;
        assert(without);
        assert(cui_font_layout_line(without, string("العربية"), 24, (cui_text_style){0},
                                    CUI_BIDI_RTL, 0, SIZE_MAX, NULL,
                                    &missing) == CUI_TEXT_LAYOUT_MISSING_GLYPH &&
               !missing);
        assert(cui_font_set_fallbacks(without, fallbacks, fallback_reference_count));
        assert(cui_font_layout_line(without, string("العربية"), 24, (cui_text_style){0},
                                    CUI_BIDI_RTL, 0, SIZE_MAX, NULL,
                                    &missing) == CUI_TEXT_LAYOUT_OK);
        cui_text_layout_view restored;
        assert(cui_text_layout_get(missing, &restored));
        assert(restored.runs[0].face_index > 0);
        cui_text_layout_destroy(missing);
        cui_font_close(without);
        puts("primary missing Arabic verified; explicit fallback restores supported glyphs");
    } else
        puts("primary already covers Arabic; missing-primary Arabic case untested");
    if (!FT_Get_Char_Index(face, 0x5d0)) {
        cui_font *without = cui_font_open(path);
        cui_text_layout *missing = NULL;
        assert(without);
        assert(cui_font_layout_line(without, string("שָׁלוֹם"), 24, (cui_text_style){0}, CUI_BIDI_RTL,
                                    0, SIZE_MAX, NULL, &missing) == CUI_TEXT_LAYOUT_MISSING_GLYPH &&
               !missing);
        assert(cui_font_set_fallbacks(without, fallbacks, fallback_reference_count));
        assert(cui_font_layout_line(without, string("שָׁלוֹם"), 24, (cui_text_style){0}, CUI_BIDI_RTL,
                                    0, SIZE_MAX, NULL, &missing) == CUI_TEXT_LAYOUT_OK);
        cui_text_layout_view restored;
        assert(cui_text_layout_get(missing, &restored));
        assert(restored.runs[0].face_index > 0);
        cui_text_layout_destroy(missing);
        cui_font_close(without);
        puts("primary missing Hebrew verified; explicit fallback restores supported marks/glyphs");
    } else
        puts("primary already covers Hebrew; missing-primary Hebrew case untested");
    reference(font, face, hb, string("abc אבג العربية 123 (xy)"), CUI_BIDI_AUTO_LTR, 0, SIZE_MAX);
    reference(font, face, hb, string("abc שָׁלוֹם 123 السَّلَامُ"), CUI_BIDI_AUTO_LTR, 0, SIZE_MAX);
    reference(font, face, hb, string("ל‍ا‌مَ️͏"), CUI_BIDI_RTL, 0, SIZE_MAX);
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
    assert(cui_font_set_fallbacks(font, NULL, 0));
    for (size_t i = 0; i < fallback_reference_count; ++i) {
        hb_font_destroy(fallback_references[i].hb);
        FT_Done_Face(fallback_references[i].face);
        cui_font_close(fallbacks[i]);
    }
    fallback_reference_count = 0;
    hb_font_destroy(hb);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    cui_font_close(font);
    arabic_clusters();
    puts("bidirectional shaping and raster regression passed");
    return 0;
}

#endif
