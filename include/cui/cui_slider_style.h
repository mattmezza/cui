/* SPDX-License-Identifier: MIT */
#ifndef CUI_SLIDER_STYLE_H
#define CUI_SLIDER_STYLE_H
#include "cui.h"
#include "cui_background.h"
#include "cui_slider_palette.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Unreleased owned style VALUES, not a Styled widget or renderer capability.
 * SOURCE formulas keep logical-pixel offsets distinct from rem-scaled extents. */
typedef enum { CUI_SLIDER_STYLE_FIXED, CUI_SLIDER_STYLE_PARENT } cui_slider_style_layout;
enum { CUI_SLIDER_STYLE_ACTIVE = 1u, CUI_SLIDER_STYLE_DISABLED = 2u };
enum { CUI_SLIDER_STYLE_BAR_OVERRIDE = 1u, CUI_SLIDER_STYLE_THUMB_OVERRIDE = 2u };
enum {
    CUI_SLIDER_STYLE_TOP_LEFT = 1u,
    CUI_SLIDER_STYLE_TOP_RIGHT = 2u,
    CUI_SLIDER_STYLE_BOTTOM_LEFT = 4u,
    CUI_SLIDER_STYLE_BOTTOM_RIGHT = 8u
};
typedef struct {
    uint32_t version, struct_size, layout, flags;
    uint32_t color_overrides, track_corner_overrides, reserved[2];
    float rem, length, full_radius;
    cui_corner_radii track_corners;
    cui_slider_palette palette;
    cui_slider_palette_color bar_override, thumb_override;
} cui_slider_style_options_v1;
typedef struct {
    uint32_t version, struct_size, layout, flags;
    float length, cross_extent, track_extent, thumb_extent, surface_extent;
    float padding, axis_offset, cross_offset, ring_max_width, full_radius;
    cui_corner_radii track_corners;
    cui_slider_palette_color track, fill, outer, ring;
    cui_background thumb;
} cui_slider_style_plan_v1;
/* Exact version1/sizeof, reserved=0, known enums/masks. rem finite >0 with
 * 1.5*rem finite; fixed length finite >0, PARENT ignores length (output0).
 * full_radius and ACTIVE corner overrides finite >=0. Inactive overrides ignored.
 * Palette must be valid; override bits distinguish transparent authored colors.
 * Track alpha=bar.a*(active?.4:.2), outer alpha=bar.a*.5, fill unchanged.
 * Disabled preserves palette/geometry and marks omission for a future presenter.
 * Thumb copies straight-linear colors/stops into an owned SOURCE_038 background,
 * never a representative byte or gradient flattening. Pure, no allocation/UI,
 * output atomic on invalid input, input/output need no retained lifetime.
 * No animation, V1 byte conversion, renderer/host/binding support is implied. */
bool cui_slider_style_resolve_v1(const cui_slider_style_options_v1 *, cui_slider_style_plan_v1 *);
#ifdef __cplusplus
}
#endif
#endif
