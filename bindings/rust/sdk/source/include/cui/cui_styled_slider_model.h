/* SPDX-License-Identifier: MIT */
#ifndef CUI_STYLED_SLIDER_MODEL_H
#define CUI_STYLED_SLIDER_MODEL_H
#include "cui.h"
#include "cui_base_slider_model.h"
#include "cui_draw_v2.h"
#include "cui_slider_style.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Unreleased C-only typed Styled facade. Owned presentation/style records;
 * reducer state/handler and external acceptance use the existing Base contract.
 * Exact version1/sizeof, reserved=0, known tags/masks. style.flags must be0;
 * actual drag/disabled policy derives it. See docs/STYLED_SLIDER_MODEL.md.
 * No current adapter supports V2 backgrounds; a copied host declaration is a
 * caller contract, never proof of real rendering or physical AT support. */
typedef enum {
    CUI_STYLED_SLIDER_SOLID_V1 = 0,
    CUI_STYLED_SLIDER_BACKGROUND_V2 = 1
} cui_styled_slider_render_mode;
enum { CUI_STYLED_SLIDER_REDUCED_MOTION = 1u, CUI_STYLED_SLIDER_HIDE_FOCUS = 2u };
typedef struct {
    uint32_t version, struct_size, render_mode, flags;
    uint32_t reserved[2];
    cui_slider_style_options_v1 style;
    cui_spring_settings spring_control;
    cui_draw_v2_support support; /* inactive in SOLID_V1 */
} cui_styled_slider_model_options_v1;
typedef struct {
    double position, velocity, target, updated_at;
} cui_styled_slider_model_motion_v1;
typedef struct {
    uint32_t version, struct_size, initialized_mask, previous_kind;
    cui_id owner;
    uint64_t last_admitted_frame;
    double last_clock, next_wake_seconds;
    cui_styled_slider_model_motion_v1 thumbs[2]; /* START=0, END=1 */
} cui_styled_slider_model_state_v1;
typedef enum {
    CUI_STYLED_SLIDER_RENDER_OK = 0,
    CUI_STYLED_SLIDER_INVALID_ARGUMENT,
    CUI_STYLED_SLIDER_INVALID_STATE,
    CUI_STYLED_SLIDER_UNSUPPORTED,
    CUI_STYLED_SLIDER_BASE_REJECTED,
    CUI_STYLED_SLIDER_CORE_FAILURE
} cui_styled_slider_render_status;
typedef struct {
    uint32_t version, struct_size, render_status, reserved;
    cui_slider_model_result_v1 base;
    double next_wake_seconds;
} cui_styled_slider_model_result_v1;
enum {
    CUI_STYLED_SLIDER_SURFACE = 0,
    CUI_STYLED_SLIDER_POINTER_RING = 1,
    CUI_STYLED_SLIDER_FOCUS_RING = 2
};
bool cui_styled_slider_model_options_from_theme_v1(const cui_theme *, float rem,
                                                   uint32_t orientation,
                                                   cui_styled_slider_model_options_v1 *out);
bool cui_styled_slider_model_state_init_v1(uint32_t state_size,
                                           cui_styled_slider_model_state_v1 *out);
cui_id cui_styled_slider_model_part_id_v1(cui_id owner, uint32_t thumb, uint32_t decorative_part);
cui_styled_slider_model_result_v1 cui_styled_slider_model_events_v1(
    cui_context *, cui_id owner, cui_slider_model_value_v1 confirmed, cui_slider_model_state_v1 *,
    const cui_slider_model_options_v1 *, cui_styled_slider_model_state_v1 *,
    const cui_styled_slider_model_options_v1 *, cui_slider_model_handler_v1);

/* Options constructor snapshots theme palette/full-radius/spring; horizontal
 * parent fill, vertical fixed120, SOLID_V1. Pure/atomic, no borrowed lifetime.
 * Init zeroes presentation, no context/capture/model ownership. Getter is pure
 * identity only: Single START/disabled/cold/hidden/zero-area parts may be absent.
 * Facade borrows handler/user/strings synchronously. External owner/userdata may
 * mutate; active UI/reducer/presentation/options/string storage may not. Visible
 * V1 gradients reject before input/state/callbacks. Matching V2 frame and real
 * negotiated support are host obligations; later failure cannot roll back Base
 * requests. Zero-generation/inert/layout-only/blocked/recovery calls never
 * advance presentation or return a wake. Deadlines use absolute Core frame time
 * and are valid for scheduling only after successful end_frame. Legacy APIs and
 * Base structs are unchanged; all5 typed bindings/native numeric hosts pending. */
#ifdef __cplusplus
}
#endif
#endif
