/* SPDX-License-Identifier: MIT */
#ifndef CUI_BUTTON_STYLES_H
#define CUI_BUTTON_STYLES_H
#include "cui_components.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Fully initialized paint. Geometry, border widths, clipping, layout and input
 * fields of root are inspection-only; patches copy only the listed fields. */
typedef struct {
    cui_box root;
    cui_color foreground;
} cui_button_style_paint;
enum {
    CUI_BUTTON_PAINT_BACKGROUND = 1u << 0,
    CUI_BUTTON_PAINT_BORDER = 1u << 1,
    CUI_BUTTON_PAINT_FOREGROUND = 1u << 2,
    CUI_BUTTON_PAINT_RADIUS = 1u << 3,
    CUI_BUTTON_PAINT_CORNERS = 1u << 4,
    CUI_BUTTON_PAINT_OPACITY = 1u << 5
};
typedef struct {
    unsigned fields; /* Explicit presence: zero/transparent values are valid. */
    cui_button_style_paint paint;
} cui_button_paint_patch;
typedef enum {
    CUI_BUTTON_STYLE_INSTANCE,
    CUI_BUTTON_STYLE_HOVER,
    CUI_BUTTON_STYLE_ACTIVE,
    CUI_BUTTON_STYLE_SELECTED,
    CUI_BUTTON_STYLE_DISABLED,
    CUI_BUTTON_STYLE_FOCUS
} cui_button_style_phase;
typedef struct {
    cui_id id;
    cui_button_style_phase phase;
    bool hovered, pressed, focus_visible;
    bool selected, open, disabled, loading;
} cui_button_style_info;
/* Synchronous borrowed pure paint calculation. Do not call CUI, queue/consume
 * input, submit children, mutate owner/options/styles, or reenter submission.
 * current/info/out expire on return; out starts zero. No callback/user retention.
 * Unknown masks or invalid present numeric values discard the whole patch;
 * geometry writes are ignored. Opacity must be finite0..1, radii finite>=0. */
typedef void (*cui_button_style_fn)(void *user, const cui_button_style_info *info,
                                    const cui_button_style_paint *current,
                                    cui_button_paint_patch *out);
typedef struct {
    cui_button_style_fn instance, hovered, active, selected, disabled, focused;
    void *user;
} cui_button_styles;
/* Component caller-instance replay: normal+instance; eligible pointer layer;
 * selected/open+selected callback+instance replay; disabled+disabled callback+
 * instance replay; loading fade; existing focus paint+focused callback. Instance
 * runs once, its value patch is replayed without calling it again. This differs
 * from Base/Checkbox state-over-instance ordering. Pressed suppresses hover;
 * disabled/loading suppress pointer callbacks; selected/open suppress both.
 * Loading multiplies root opacity by0.8 unless disabled. Focus follows existing
 * focus-visible/hide-focus-ring gates. Zero styles preserve legacy behavior.
 * CORNERS copies custom_corners+corner_radii; OPACITY sets custom_opacity=true.
 * No event-facade/group bridge, arbitrary Styled/layout/font/gradient/shadow API. */
bool cui_button_styled_ex(cui_context *, cui_id, const cui_theme *, cui_str, cui_button_options,
                          cui_button_styles);
/* Paint only: caller registers the Button; no input reduction or semantics. */
void cui_button_styled_paint(cui_context *, cui_id, const cui_theme *, cui_str, cui_button_options,
                             cui_button_styles);
/* Additive shadow-aware facade. Existing records above keep their ABI/rules.
 * New patches accept the six scalar bits plus SHADOWS; fixed arrays are owned
 * values, never retained callback pointers. No gradients/font/layout refinement. */
enum { CUI_BUTTON_SURFACE_SHADOWS = 1u << 6 };
typedef struct {
    unsigned rejected_phases; /* 1u << phase; zeroed on entry, independently of activation. */
} cui_button_surface_status;
typedef struct {
    cui_button_style_paint paint;
    size_t shadow_count;
    cui_box_shadow shadows[CUI_BOX_SHADOW_MAX];
} cui_button_surface_paint;
typedef struct {
    unsigned fields;
    cui_button_surface_paint value;
} cui_button_surface_patch;
/* Same borrowed pure-calculation rules as cui_button_style_fn. out starts zero.
 * Whole invalid patches are ignored and reported through optional status;
 * later eligible phases continue. Scalars use the old presence bits, but NEW
 * facade radii/corners are bounded to [0,16384]. Shadows: <=8 mixed layers,
 * sigma [0,256], offset/spread [-4096,4096], all finite and valid kinds.
 * A nonempty list requires final effective root radii within [0,16384], including
 * options-derived radii; otherwise the whole phase patch is rejected. Unused
 * entries/absent fields are ignored. No Core error for rejected phase patches. */
typedef void (*cui_button_surface_fn)(void *user, const cui_button_style_info *info,
                                      const cui_button_surface_paint *current,
                                      cui_button_surface_patch *out);
typedef struct {
    cui_button_surface_fn instance, hovered, active, selected, disabled, focused;
    void *user;
} cui_button_surface_styles;
/* Normal+instance; eligible pointer phase; selected/open+callback+instance replay;
 * disabled colors+shadow clear+callback+instance replay; loading opacity *0.8;
 * existing focus paint+focused callback. Instance runs once. Present SHADOWS
 * replaces the entire list (zero clears DROP+INSET); absent inherits. Frame-local,
 * no builtin/default shadow. Resolved root corners are used for every layer.
 * status may be NULL; bool remains the existing aggregate activation result.
 * Input/semantics/actions/content retain their existing exactly-once behavior.
 * Later element setters/bridge overrides replace this decoration normally.
 * Existing event/group APIs and typed builders do not accept these callbacks. */
bool cui_button_surface_ex(cui_context *, cui_id, const cui_theme *, cui_str, cui_button_options,
                           cui_button_surface_styles, cui_button_surface_status *);
/* Independently registered owner, paint only: no semantics/input reduction. */
void cui_button_surface_paint_only(cui_context *, cui_id, const cui_theme *, cui_str,
                                   cui_button_options, cui_button_surface_styles,
                                   cui_button_surface_status *);
#ifdef __cplusplus
}
#endif
#endif
