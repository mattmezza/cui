/* SPDX-License-Identifier: MIT */
#ifndef CUI_SKELETON_PRESENTATION_H
#define CUI_SKELETON_PRESENTATION_H
#include "cui_content.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Owned submission style; no retained layout object or extra motion owner.
 * Core's existing fixed/FIT/grow/percent/max/corner/edge normalization applies.
 * custom_background explicitly replaces palette+secondary, including alpha0.
 * Animation replaces root opacity AFTER refinement; fade a group via its parent.
 * ease/user are borrowed synchronously; neither is retained. */
typedef struct {
    cui_box root;
    bool custom_background;
    bool secondary;
    float duration;
    double now_seconds;
    bool reduced_motion;
    cui_loading_ease_fn ease;
    void *ease_user;
} cui_skeleton_presentation;
/* Source full-parent width (percent1), height16 logical pixels, period2seconds.
 * A zero-initialized record instead requests Core FIT/FIT sizing. */
cui_skeleton_presentation cui_skeleton_presentation_default(void);
/* Submit during ordinary composition of an active frame. Palette and entire
 * presentation snapshot precede ease; opacity animates background AND border.
 * Caller owns independent motion state, absolute clock and wake/reset policy.
 * Null mandatory input, zero id, invalid duration or failed motion sampling
 * returns false with state/out/paint unchanged. Callback effects cannot rollback.
 * Other root values use Core normalization; root.opacity/custom_opacity are
 * inactive because animation replaces them, even when authored opacity is NaN.
 * State/out must not overlap each other or borrowed inputs. No allocation.
 * True means component submission, not later layout/render success. Inspect
 * completed frame diagnostics; later Core errors do not rollback state/effects.
 * Registered scroll percent HEIGHT remains subject to existing Core rejection.
 * No semantics/focus/busy node; applications own meaningful loading regions. */
bool cui_skeleton_present(cui_context *, cui_id, const cui_skeleton_palette *,
                          cui_loading_motion *, cui_skeleton_presentation,
                          cui_loading_motion_result *);
#ifdef __cplusplus
}
#endif
#endif
