/* SPDX-License-Identifier: MIT */
#ifndef CUI_SLIDER_PALETTE_H
#define CUI_SLIDER_PALETTE_H
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Owned straight linear RGB/alpha, [0,1]; hidden RGB at alpha0 survives.
 * Source HSLA shader values and encoded sRGBA8 use distinct constructors. */
typedef struct {
    float r, g, b, a;
} cui_slider_palette_color;
typedef struct {
    float position;
    cui_slider_palette_color color;
} cui_slider_palette_stop;
typedef enum { CUI_SLIDER_BRUSH_SOLID = 0, CUI_SLIDER_BRUSH_LINEAR = 1 } cui_slider_brush_kind;
typedef enum { CUI_SLIDER_BRUSH_SRGB = 0, CUI_SLIDER_BRUSH_OKLAB = 1 } cui_slider_brush_space;
typedef enum { CUI_SLIDER_BRUSH_SOURCE_038 = 0 } cui_slider_brush_projection;
typedef struct {
    cui_slider_brush_kind kind;
    cui_slider_palette_color solid;
    cui_slider_brush_space space;
    cui_slider_brush_projection projection;
    float angle_degrees;
    cui_slider_palette_stop stops[2];
} cui_slider_palette_brush;
typedef struct {
    cui_slider_palette_color bar, ring;
    cui_slider_palette_brush thumb;
} cui_slider_palette;
typedef enum {
    CUI_SLIDER_PALETTE_LIGHT = 0,
    CUI_SLIDER_PALETTE_DARK = 1,
    CUI_SLIDER_PALETTE_HIGH_CONTRAST = 2,
    CUI_SLIDER_PALETTE_TERMINAL = 3
} cui_slider_palette_builtin;
/* Pure, no allocation/context/diagnostics. Invalid/null output leaves it untouched.
 * sRGBA8 decodes RGB transfer. Source HSLA uses finite normalized [0,1] inputs
 * and pinned source shader formula, bounding unit-gamut roundoff without
 * additional transfer decode. This is not GPU-f32 bit identity. */
bool cui_slider_palette_color_from_srgba8(cui_color, cui_slider_palette_color *out);
bool cui_slider_palette_color_from_source_hsla(float h, float s, float l, float a,
                                               cui_slider_palette_color *out);
/* SOLID ignores inactive gradient fields. LINEAR ignores solid, validates enums,
 * both owned colors, angle[0,360] and positions[0,1]; reversed/equal positions
 * remain authored. SRGB encodes linear RGB before straight mixing then decodes;
 * OKLAB converts to/from Oklab for mixing. Alpha interpolates independently,
 * with final premultiplication, not CSS premultiplied-stop interpolation.
 * SOURCE_038 projects
 * using source aspect-scaled direction, not CSS. See docs/SLIDER_PALETTE.md.
 * Values only: no renderer, sampler, setter or current-frame presence guarantee. */
bool cui_slider_palette_valid(const cui_slider_palette *);
/* CUI adaptation: primary/on_primary/focus encoded bytes, no preset inference.
 * Builtins use the same rule; they are distinct from named source presets. */
bool cui_slider_palette_from_theme(const cui_theme *, cui_slider_palette *out);
bool cui_slider_palette_builtin_get(cui_slider_palette_builtin, cui_slider_palette *out);
/* Same index/name order as cui_theme_preset_get. Exact pinned source token rules,
 * including slider.background (NOT slider.bar.background) and owned Aurora
 * gradient thumb. No byte-theme flattening. Invalid index leaves output intact;
 * invalid name index returns empty. Copies need no borrowed source lifetime. */
size_t cui_slider_palette_preset_count(void);
cui_str cui_slider_palette_preset_name(size_t index);
bool cui_slider_palette_preset_get(size_t index, cui_slider_palette *out);
#ifdef __cplusplus
}
#endif
#endif
