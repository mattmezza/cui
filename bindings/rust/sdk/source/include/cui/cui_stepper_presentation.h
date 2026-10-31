/* SPDX-License-Identifier: MIT */
#ifndef CUI_STEPPER_PRESENTATION_H
#define CUI_STEPPER_PRESENTATION_H
#include "cui_stepper_visual.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { CUI_STEPPER_LAYOUT_CONTENT = 0, CUI_STEPPER_LAYOUT_FILL } cui_stepper_layout;
typedef struct {
    cui_color normal, hover, pressed, foreground;
} cui_stepper_indicator_palette;
typedef struct {
    cui_stepper_layout layout;
    float width, height;
    bool custom_palette;
    cui_stepper_indicator_palette palette;
} cui_stepper_presentation_options;
/* Explicit opt-in presentation; old visual/navigation APIs remain unchanged.
 * CONTENT keeps fixed connector spacers and requires width=height=0. FILL uses
 * non-centered zero-flow connectors; width0 grows to available parent width,
 * height0 is intrinsic, positive finite extents are explicit. Invalid options
 * fail before Base registration/reduction; exactly one Base reducer is used.
 * Result.valid is argument preflight, not layout/resource-capacity proof.
 * Unchecked normal/hover/foreground use theme.secondary/secondary_hover/
 * secondary_foreground; pressed uses buttons[SECONDARY].active (CUI fallback,
 * not a source global-token parity claim). custom_palette replaces all four.
 * Checked primary/on_primary overrides interaction; disabled blocks feedback.
 * Radius is zero for theme.radius==0, otherwise side/2. No focus-ring extension.
 * Hover is current-layout visible indicator-only. Press observes accepted trigger
 * capture while pointer visibly overlaps indicator, or accepted Space capture.
 * Label-origin drag onto indicator follows this CUI policy too; GPUI engine
 * drag/keyboard equivalence is not claimed. Decoration adds no input targets.
 * Request timing/captured selection and all bitmap lifetimes match visual API:
 * configuration borrowed through return, bitmap bytes through render completion.
 * No centered geometry, arbitrary children, exact typography or bindings claim. */
cui_stepper_navigation_result cui_stepper_present(cui_context *, cui_id, const cui_theme *,
                                                  const cui_step *, size_t count, size_t selected,
                                                  cui_stepper_visual_options,
                                                  cui_stepper_presentation_options,
                                                  cui_stepper_navigation_handler);
#ifdef __cplusplus
}
#endif
#endif
