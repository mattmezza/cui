/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_H
#define CUI_TEXT_H
#include "cui.h"
#include "cui_bidi.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_font cui_font;
typedef void (*cui_glyph_fn)(void *user, int x, int y, int width, int height, int pitch,
                             const unsigned char *coverage);
/* The font file remains caller-owned. Face operations are UI-thread confined.
 */
cui_font *cui_font_open(const char *path);
void cui_font_close(cui_font *font);
/* Borrow an actual face opened by the caller; pass NULL to remove a variant.
 * Keep variants alive through measurement/drawing. Registrations are keyed by
 * family ID, weight, italic and monospace. Nonzero families may register a
 * regular face; family zero's regular face is the one passed to font_open.
 * Fallback: exact family/style, named family regular, default family/style,
 * then the opened regular face. A family-only registration is not an OS name
 * lookup. Remove styles independently; clearing regular retains other styles. */
bool cui_font_set_style(cui_font *font, cui_text_style style, cui_font *variant);
/* Convenience path registration owns the opened face. Failure preserves the
 * previous registration; replacement and font_close release owned faces. */
bool cui_font_register_style(cui_font *font, cui_text_style style, const char *path);
/* Selected-face metrics suitable for cui_config.font_metrics. Failure preserves out. */
bool cui_font_get_metrics(void *font, float size, cui_text_style style, cui_font_metrics *out);
cui_size cui_font_measure(void *font, cui_str text, float size);
cui_size cui_font_measure_styled(void *font, cui_str text, float size, cui_text_style style);
bool cui_font_draw(cui_font *font, cui_str text, float size, float x, float y, cui_glyph_fn emit,
                   void *user);
bool cui_font_draw_styled(cui_font *font, cui_str text, float size, cui_text_style style, float x,
                          float y, cui_glyph_fn emit, void *user);
/* Optional bidi/shaping layout (CUI_TEXT_BIDI). When enabled, ordinary
 * measurement/drawing also use paragraph/script analysis and hard-line layout.
 * Disabled builds retain legacy drawing. No platform/shaping types are public. */
typedef struct cui_text_layout cui_text_layout;
typedef struct {
    uint32_t glyph_id;
    size_t cluster_byte; /* Original paragraph UTF-8 byte offset; clusters may repeat. */
    float x, y;          /* Positioned baseline-relative origin, y grows down. */
    float x_advance, y_advance;
} cui_text_glyph;
typedef struct {
    size_t byte_start, byte_length, glyph_start, glyph_count;
    uint32_t script; /* ISO 15924 four-byte tag, most significant byte first. */
    uint8_t level;
} cui_text_run;
typedef struct {
    const cui_text_glyph *glyphs;
    const cui_text_run *runs;
    size_t glyph_count, run_count;
    float width, height, ascent;
    uint8_t paragraph_level;
} cui_text_layout_view;
typedef enum {
    CUI_TEXT_LAYOUT_OK,
    CUI_TEXT_LAYOUT_INVALID,
    CUI_TEXT_LAYOUT_MEMORY,
    CUI_TEXT_LAYOUT_UNAVAILABLE
} cui_text_layout_status;
/* Analyze a full paragraph, shape one caller-selected scalar line. SIZE_MAX
 * length means through paragraph end. Empty ranges return paragraph metrics.
 * Choose complete grapheme/shaping cluster
 * boundaries. Language is a BCP47 tag or NULL for "und". Layout owns glyph/run
 * data and copies all needed text; input may be reused after return. Font and
 * selected style face must remain alive and registered until layout destruction.
 * Operations involving fonts are UI-thread confined. Failure preserves *out. */
cui_text_layout_status cui_font_layout_line(cui_font *font, cui_str paragraph, float size,
                                            cui_text_style style, cui_bidi_direction direction,
                                            size_t scalar_start, size_t scalar_length,
                                            const char *language, cui_text_layout **out);
/* View remains valid until destroy; invalid arguments preserve output. */
bool cui_text_layout_get(const cui_text_layout *layout, cui_text_layout_view *out);
void cui_text_layout_destroy(cui_text_layout *layout);
/* Paint the positioned glyphs using the selected face and saved size. Coordinates
 * specify the line's top-left; partial raster failure may already have emitted. */
bool cui_text_layout_draw(cui_text_layout *layout, float x, float y, cui_glyph_fn emit, void *user);
#ifdef __cplusplus
}
#endif
#endif
