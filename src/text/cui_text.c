/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
typedef struct cui_font_family {
    uint32_t id;
    struct cui_font *variants[4][2][2];
    bool owns_variant[4][2][2];
    struct cui_font_family *next;
} cui_font_family;
struct cui_font {
    FT_Library library;
    FT_Face face;
    hb_font_t *shaper;
    cui_font_family default_family;
    cui_font_family *families;
};
static bool valid_style(cui_text_style style) {
    return style.weight >= CUI_TEXT_WEIGHT_NORMAL && style.weight <= CUI_TEXT_WEIGHT_BOLD;
}
static cui_font_family *family_find(cui_font *font, uint32_t id, bool create) {
    if (!id)
        return &font->default_family;
    for (cui_font_family *f = font->families; f; f = f->next)
        if (f->id == id)
            return f;
    if (!create)
        return NULL;
    cui_font_family *f = calloc(1, sizeof(*f));
    if (!f)
        return NULL;
    f->id = id;
    f->next = font->families;
    font->families = f;
    return f;
}
static cui_font *selected(cui_font *font, cui_text_style style) {
    if (!font || !valid_style(style))
        return NULL;
    cui_font_family *family = family_find(font, style.family, false);
    if (family) {
        cui_font *variant = family->variants[style.weight][style.italic][style.monospace];
        if (variant)
            return variant;
        if (style.family && family->variants[0][0][0])
            return family->variants[0][0][0];
    }
    cui_font *variant = font->default_family.variants[style.weight][style.italic][style.monospace];
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
    cui_font_family *family = &f->default_family;
    while (family) {
        for (size_t weight = 0; weight < 4; weight++)
            for (size_t italic = 0; italic < 2; italic++)
                for (size_t mono = 0; mono < 2; mono++)
                    if (family->owns_variant[weight][italic][mono])
                        cui_font_close(family->variants[weight][italic][mono]);
        cui_font_family *next = family == &f->default_family ? f->families : family->next;
        if (family != &f->default_family)
            free(family);
        family = next;
    }
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
        (!style.family && style.weight == CUI_TEXT_WEIGHT_NORMAL && !style.italic &&
         !style.monospace) ||
        font == variant)
        return false;
    cui_font_family *family = family_find(font, style.family, variant != NULL);
    if (!family) {
        return variant == NULL;
    }
    cui_font **slot = &family->variants[style.weight][style.italic][style.monospace];
    bool *owned = &family->owns_variant[style.weight][style.italic][style.monospace];
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
        (!style.family && style.weight == CUI_TEXT_WEIGHT_NORMAL && !style.italic &&
         !style.monospace) ||
        !path)
        return false;
    cui_font *variant = cui_font_open(path);
    if (!variant)
        return false;
    cui_font_family *family = family_find(font, style.family, true);
    if (!family) {
        cui_font_close(variant);
        return false;
    }
    cui_font **slot = &family->variants[style.weight][style.italic][style.monospace];
    bool *owned = &family->owns_variant[style.weight][style.italic][style.monospace];
    if (*owned)
        cui_font_close(*slot);
    *slot = variant;
    *owned = true;
    return true;
}
#ifdef CUI_TEXT_BIDI
typedef struct text_lines {
    cui_text_layout *layout;
    struct text_lines *next;
    float y;
} text_lines;
static void text_lines_destroy(text_lines *lines);
static bool text_lines_create(cui_font *font, cui_str text, float size, cui_text_style style,
                              text_lines **out, cui_size *bounds);
#endif
#ifndef CUI_TEXT_BIDI
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
#endif
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
#ifdef CUI_TEXT_BIDI
    text_lines *lines = NULL;
    if (!text_lines_create(font, s, size, style, &lines, &result))
        return (cui_size){0, 0};
    text_lines_destroy(lines);
    return result;
#else
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
#endif
}
cui_size cui_font_measure(void *font, cui_str s, float size) {
    return cui_font_measure_styled(font, s, size, (cui_text_style){0});
}
bool cui_font_draw_styled(cui_font *font, cui_str s, float size, cui_text_style style, float x,
                          float y, cui_glyph_fn emit, void *user) {
    if (!isfinite(x) || !isfinite(y) || fabsf(x) > 1e6f || fabsf(y) > 1e6f)
        return false;
#ifdef CUI_TEXT_BIDI
    text_lines *lines = NULL;
    cui_size bounds;
    if (!emit || !text_lines_create(font, s, size, style, &lines, &bounds))
        return false;
    bool result = true;
    for (text_lines *line = lines; line; line = line->next)
        if (!cui_text_layout_draw(line->layout, x, y + line->y, emit, user))
            result = false;
    text_lines_destroy(lines);
    return result;
#else
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
#endif
}
bool cui_font_draw(cui_font *f, cui_str s, float size, float x, float y, cui_glyph_fn emit,
                   void *user) {
    return cui_font_draw_styled(f, s, size, (cui_text_style){0}, x, y, emit, user);
}

struct cui_text_layout {
    cui_text_layout_view view;
    cui_text_glyph *glyphs;
    cui_text_run *runs;
    cui_font *face;
    float size;
};
void cui_text_layout_destroy(cui_text_layout *layout) {
    if (!layout)
        return;
    free(layout->glyphs);
    free(layout->runs);
    free(layout);
}
bool cui_text_layout_get(const cui_text_layout *layout, cui_text_layout_view *out) {
    if (!layout || !out)
        return false;
    *out = layout->view;
    return true;
}

#ifdef CUI_TEXT_BIDI
#include <string.h>
typedef struct {
    size_t start, end;
    hb_script_t script;
    uint8_t level;
    hb_buffer_t *buffer;
} layout_segment;
static bool weak_script(hb_script_t script) {
    return script == HB_SCRIPT_COMMON || script == HB_SCRIPT_INHERITED ||
           script == HB_SCRIPT_UNKNOWN;
}

cui_text_layout_status cui_font_layout_line(cui_font *font, cui_str text, float size,
                                            cui_text_style style, cui_bidi_direction direction,
                                            size_t start, size_t length, const char *language,
                                            cui_text_layout **out) {
    cui_font *face = selected(font, style);
    cui_bidi_paragraph *paragraph = NULL;
    cui_bidi_result analysis;
    cui_text_layout *layout = NULL;
    uint32_t *scalars = NULL, *mirrors = NULL;
    size_t *bytes = NULL, *mapping = NULL;
    uint8_t *levels = NULL;
    hb_script_t *scripts = NULL;
    cui_bidi_run *bidi_runs = NULL;
    layout_segment *segments = NULL;
    size_t segment_count = 0, glyph_count = 0;
    cui_text_layout_status status = CUI_TEXT_LAYOUT_MEMORY;
    if (!out || !face || !isfinite(size) || size <= 0 || size > 4096 || text.length > INT_MAX ||
        (text.length && !text.data) || (unsigned)direction > CUI_BIDI_RTL)
        return CUI_TEXT_LAYOUT_INVALID;
    /* Every scalar/subrun is bounded by UTF-8 length; checked typed allocations. */
    size_t capacity = text.length ? text.length : 1;
    if (capacity > SIZE_MAX / sizeof(layout_segment) || capacity == SIZE_MAX)
        return CUI_TEXT_LAYOUT_INVALID;
    scalars = malloc(capacity * sizeof *scalars);
    if (!scalars)
        goto done;
    cui_bidi_status bidi =
        cui_bidi_paragraph_create(text.data, text.length, direction, scalars, capacity, &paragraph);
    if (bidi != CUI_BIDI_OK) {
        status = bidi == CUI_BIDI_MEMORY ? CUI_TEXT_LAYOUT_MEMORY : CUI_TEXT_LAYOUT_INVALID;
        goto done;
    }
    levels = malloc(capacity * sizeof *levels);
    bytes = malloc((capacity + 1) * sizeof *bytes);
    mapping = malloc(capacity * sizeof *mapping);
    mirrors = malloc(capacity * sizeof *mirrors);
    scripts = malloc(capacity * sizeof *scripts);
    bidi_runs = malloc(capacity * sizeof *bidi_runs);
    segments = calloc(capacity, sizeof *segments);
    if (!levels || !bytes || !mapping || !mirrors || !scripts || !bidi_runs || !segments)
        goto done;
    if (cui_bidi_paragraph_levels(paragraph, levels, capacity, &analysis) != CUI_BIDI_OK)
        goto done;
    if (start > analysis.scalar_count) {
        status = CUI_TEXT_LAYOUT_INVALID;
        goto done;
    }
    if (length == SIZE_MAX)
        length = analysis.scalar_count - start;
    if (length > analysis.scalar_count - start) {
        status = CUI_TEXT_LAYOUT_INVALID;
        goto done;
    }
    size_t scalar = 0;
    for (size_t b = 0; b < text.length; ++b)
        if (((unsigned char)text.data[b] & 0xc0) != 0x80)
            bytes[scalar++] = b;
    bytes[scalar] = text.length;
    layout = calloc(1, sizeof *layout);
    if (!layout)
        goto done;
    layout->face = face;
    layout->size = size;
    if (FT_Set_Char_Size(face->face, 0, (FT_F26Dot6)lroundf(size * 64), 72, 72)) {
        status = CUI_TEXT_LAYOUT_INVALID;
        goto done;
    }
    hb_ft_font_changed(face->shaper);
    layout->view.height = (float)face->face->size->metrics.height / 64;
    layout->view.ascent = (float)face->face->size->metrics.ascender / 64;
    layout->view.paragraph_level = analysis.paragraph_level;
    if (!length) {
        *out = layout;
        layout = NULL;
        status = CUI_TEXT_LAYOUT_OK;
        goto done;
    }
    bidi = cui_bidi_paragraph_line(paragraph, start, length, levels, mapping, mirrors, bidi_runs,
                                   capacity, &analysis);
    if (bidi != CUI_BIDI_OK) {
        status = bidi == CUI_BIDI_MEMORY ? CUI_TEXT_LAYOUT_MEMORY : CUI_TEXT_LAYOUT_INVALID;
        goto done;
    }
    for (size_t r = 0; r < analysis.run_count; ++r) {
        size_t first = bidi_runs[r].logical_start, end = first + bidi_runs[r].length;
        hb_script_t previous = HB_SCRIPT_COMMON;
        for (size_t i = first; i < end; ++i) {
            scripts[i] = hb_unicode_script(hb_unicode_funcs_get_default(), scalars[i]);
            if (!weak_script(scripts[i]))
                previous = scripts[i];
            else if (!weak_script(previous))
                scripts[i] = previous;
        }
        previous = HB_SCRIPT_COMMON;
        for (size_t i = end; i > first;) {
            --i;
            if (!weak_script(scripts[i]))
                previous = scripts[i];
            else if (!weak_script(previous))
                scripts[i] = previous;
        }
        size_t before = segment_count;
        for (size_t i = first; i < end;) {
            size_t limit = i + 1;
            while (limit < end && scripts[limit] == scripts[i])
                ++limit;
            segments[segment_count++] =
                (layout_segment){i, limit, scripts[i], bidi_runs[r].level, NULL};
            i = limit;
        }
        /* Directional runs arrive in visual order. RTL script subruns also need
         * visual order; HarfBuzz alone orders glyph clusters inside each subrun. */
        if (bidi_runs[r].level & 1)
            for (size_t a = before, b = segment_count - 1; a < b; ++a, --b) {
                layout_segment temp = segments[a];
                segments[a] = segments[b];
                segments[b] = temp;
            }
    }
    for (size_t i = 0; i < segment_count; ++i) {
        layout_segment *seg = &segments[i];
        hb_buffer_t *buffer = hb_buffer_create();
        seg->buffer = buffer;
        hb_buffer_set_direction(buffer, (seg->level & 1) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
        hb_buffer_set_script(buffer, seg->script);
        hb_buffer_set_language(buffer, hb_language_from_string(language ? language : "und", -1));
        hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
        hb_buffer_flags_t flags = HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES;
        if (seg->start == start)
            flags |= HB_BUFFER_FLAG_BOT;
        if (seg->end == start + length)
            flags |= HB_BUFFER_FLAG_EOT;
        hb_buffer_set_flags(buffer, flags);
        hb_buffer_add_utf8(buffer, text.data + bytes[start],
                           (int)(bytes[start + length] - bytes[start]),
                           (unsigned)(bytes[seg->start] - bytes[start]),
                           (int)(bytes[seg->end] - bytes[seg->start]));
        hb_shape(face->shaper, buffer, NULL, 0);
        if (!hb_buffer_allocation_successful(buffer))
            goto done;
        size_t added = hb_buffer_get_length(buffer);
        if (added > SIZE_MAX - glyph_count)
            goto done;
        glyph_count += added;
    }
    if (glyph_count > SIZE_MAX / sizeof *layout->glyphs)
        goto done;
    layout->glyphs = calloc(glyph_count ? glyph_count : 1, sizeof *layout->glyphs);
    layout->runs = calloc(segment_count, sizeof *layout->runs);
    if (!layout->glyphs || !layout->runs)
        goto done;
    size_t glyph = 0;
    float pen = 0;
    for (size_t i = 0; i < segment_count; ++i) {
        layout_segment *seg = &segments[i];
        unsigned count;
        hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(seg->buffer, &count);
        hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(seg->buffer, NULL);
        float advance = 0;
        for (unsigned j = 0; j < count; ++j)
            advance += (float)positions[j].x_advance / 64;
        float x = pen + (advance < 0 ? -advance : 0), y = 0;
        layout->runs[i] = (cui_text_run){bytes[seg->start],
                                         bytes[seg->end] - bytes[seg->start],
                                         glyph,
                                         count,
                                         (uint32_t)hb_script_to_iso15924_tag(seg->script),
                                         seg->level};
        for (unsigned j = 0; j < count; ++j) {
            layout->glyphs[glyph++] = (cui_text_glyph){infos[j].codepoint,
                                                       infos[j].cluster + bytes[start],
                                                       x + (float)positions[j].x_offset / 64,
                                                       y - (float)positions[j].y_offset / 64,
                                                       (float)positions[j].x_advance / 64,
                                                       -(float)positions[j].y_advance / 64};
            x += (float)positions[j].x_advance / 64;
            y -= (float)positions[j].y_advance / 64;
        }
        pen += fabsf(advance);
    }
    layout->view.glyphs = layout->glyphs;
    layout->view.runs = layout->runs;
    layout->view.glyph_count = glyph_count;
    layout->view.run_count = segment_count;
    layout->view.width = pen;
    *out = layout;
    layout = NULL;
    status = CUI_TEXT_LAYOUT_OK;
done:
    for (size_t i = 0; i < segment_count; ++i)
        if (segments[i].buffer)
            hb_buffer_destroy(segments[i].buffer);
    cui_text_layout_destroy(layout);
    cui_bidi_paragraph_destroy(paragraph);
    free(scalars);
    free(mirrors);
    free(bytes);
    free(mapping);
    free(levels);
    free(scripts);
    free(bidi_runs);
    free(segments);
    return status;
}
#else
cui_text_layout_status cui_font_layout_line(cui_font *font, cui_str paragraph, float size,
                                            cui_text_style style, cui_bidi_direction direction,
                                            size_t start, size_t length, const char *language,
                                            cui_text_layout **out) {
    (void)font;
    (void)paragraph;
    (void)size;
    (void)style;
    (void)direction;
    (void)start;
    (void)length;
    (void)language;
    (void)out;
    return CUI_TEXT_LAYOUT_UNAVAILABLE;
}
#endif

bool cui_text_layout_draw(cui_text_layout *layout, float x, float y, cui_glyph_fn emit,
                          void *user) {
    if (!layout || !emit || !isfinite(x) || !isfinite(y) || fabsf(x) > 1e6f || fabsf(y) > 1e6f)
        return false;
    cui_font *face = layout->face;
    if (FT_Set_Char_Size(face->face, 0, (FT_F26Dot6)lroundf(layout->size * 64), 72, 72))
        return false;
    bool ok = true;
    for (size_t i = 0; i < layout->view.glyph_count; ++i) {
        const cui_text_glyph *g = &layout->glyphs[i];
        float px = x + g->x, py = y + layout->view.ascent + g->y;
        if (!isfinite(px) || !isfinite(py) || fabsf(px) > 1e7f || fabsf(py) > 1e7f)
            return false;
        if (FT_Load_Glyph(face->face, g->glyph_id, FT_LOAD_DEFAULT) ||
            FT_Render_Glyph(face->face->glyph, FT_RENDER_MODE_NORMAL)) {
            ok = false;
            continue;
        }
        FT_GlyphSlot slot = face->face->glyph;
        emit(user, (int)lroundf(px) + slot->bitmap_left, (int)lroundf(py) - slot->bitmap_top,
             (int)slot->bitmap.width, (int)slot->bitmap.rows, slot->bitmap.pitch,
             slot->bitmap.buffer);
    }
    return ok;
}

#ifdef CUI_TEXT_BIDI
/* Strict scanning is necessary before recognizing separators: malformed bytes
 * must not become boundaries that hide invalid text from paragraph validation. */
static bool text_next(cui_str text, size_t *index, uint32_t *cp) {
    size_t i = *index, more = 0;
    uint32_t value = (unsigned char)text.data[i++], minimum = 0;
    if (value < 0x80) {
    } else if (value >= 0xc2 && value <= 0xdf) {
        value &= 31;
        more = 1;
        minimum = 0x80;
    } else if (value >= 0xe0 && value <= 0xef) {
        value &= 15;
        more = 2;
        minimum = 0x800;
    } else if (value >= 0xf0 && value <= 0xf4) {
        value &= 7;
        more = 3;
        minimum = 0x10000;
    } else
        return false;
    if (more > text.length - i)
        return false;
    while (more--) {
        uint32_t b = (unsigned char)text.data[i++];
        if ((b & 0xc0) != 0x80)
            return false;
        value = (value << 6) | (b & 63);
    }
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return false;
    *index = i;
    *cp = value;
    return true;
}
static bool paragraph_separator(uint32_t cp) {
    return cp == 0xa || cp == 0xd || cp == 0x1c || cp == 0x1d || cp == 0x1e || cp == 0x85 ||
           cp == 0x2029;
}
static void text_lines_destroy(text_lines *lines) {
    while (lines) {
        text_lines *next = lines->next;
        cui_text_layout_destroy(lines->layout);
        free(lines);
        lines = next;
    }
}
static bool text_line_append(cui_font *font, cui_str paragraph, float size, cui_text_style style,
                             size_t start, size_t length, text_lines ***tail, cui_size *bounds) {
    text_lines *node = calloc(1, sizeof *node);
    cui_text_layout_view view;
    if (!node)
        return false;
    if (cui_font_layout_line(font, paragraph, size, style, CUI_BIDI_AUTO_LTR, start, length, NULL,
                             &node->layout) != CUI_TEXT_LAYOUT_OK) {
        free(node);
        return false;
    }
    (void)cui_text_layout_get(node->layout, &view);
    node->y = bounds->height;
    bounds->height += view.height;
    if (view.width > bounds->width)
        bounds->width = view.width;
    **tail = node;
    *tail = &node->next;
    return true;
}
static bool text_paragraph_append(cui_font *font, cui_str paragraph, float size,
                                  cui_text_style style, text_lines ***tail, cui_size *bounds) {
    size_t index = 0, scalar = 0, start = 0;
    while (index < paragraph.length) {
        uint32_t cp;
        if (!text_next(paragraph, &index, &cp))
            return false;
        if (cp == 0x2028) {
            if (!text_line_append(font, paragraph, size, style, start, scalar - start, tail,
                                  bounds))
                return false;
            start = scalar + 1;
        }
        ++scalar;
    }
    return text_line_append(font, paragraph, size, style, start, scalar - start, tail, bounds);
}
static bool text_lines_create(cui_font *font, cui_str text, float size, cui_text_style style,
                              text_lines **out, cui_size *bounds) {
    text_lines *head = NULL, **tail = &head;
    cui_size measure = {0, 0};
    size_t index = 0, start = 0;
    if (text.length > INT_MAX || (text.length && !text.data))
        return false;
    while (index < text.length) {
        size_t before = index;
        uint32_t cp;
        if (!text_next(text, &index, &cp))
            goto failed;
        if (paragraph_separator(cp)) {
            cui_str paragraph = {text.data + start, before - start};
            if (!text_paragraph_append(font, paragraph, size, style, &tail, &measure))
                goto failed;
            if (cp == 0xd && index < text.length && text.data[index] == '\n')
                ++index;
            start = index;
        }
    }
    {
        cui_str paragraph = {text.data ? text.data + start : NULL, text.length - start};
        if (!text_paragraph_append(font, paragraph, size, style, &tail, &measure))
            goto failed;
    }
    *out = head;
    *bounds = measure;
    return true;
failed:
    text_lines_destroy(head);
    return false;
}
#endif
