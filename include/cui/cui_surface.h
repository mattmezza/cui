/* SPDX-License-Identifier: MIT */
#ifndef CUI_SURFACE_H
#define CUI_SURFACE_H
#include "cui_base.h"
#include "cui_overlay.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_SURFACE_DIALOG,
    CUI_SURFACE_ALERT,
    CUI_SURFACE_SHEET,
    CUI_SURFACE_POPOVER
} cui_surface_kind;
typedef enum { CUI_SHEET_LEFT, CUI_SHEET_RIGHT, CUI_SHEET_TOP, CUI_SHEET_BOTTOM } cui_sheet_edge;
typedef enum {
    CUI_SURFACE_NONE,
    CUI_SURFACE_ACCEPTED,
    CUI_SURFACE_CANCELLED,
    CUI_SURFACE_CLOSED
} cui_surface_result;
typedef bool (*cui_surface_decision_fn)(void *user, cui_surface_result);
typedef void (*cui_surface_closed_fn)(void *user, cui_surface_result);
typedef struct {
    cui_overlay overlay;
    cui_surface_kind kind;
    cui_surface_result result;
    float sheet_size, resize_origin, resize_size;
    bool resizing;
    cui_surface_result pending_close;
    cui_surface_closed_fn on_close;
    void *close_user;
    float body_scroll, body_maximum;
    cui_rect body_viewport, body_content;
    /* Optional sheet motion; the modal remains mounted until its exit completes. */
    float sheet_progress, sheet_motion_from, sheet_animation_seconds;
    double sheet_motion_start;
    bool sheet_motion_enabled, sheet_closing;
    cui_surface_result sheet_close_result;
} cui_surface_state;
/* Opt-in Popover defaults; zero retains the legacy surface appearance. */
typedef enum {
    CUI_POPOVER_APPEARANCE_LEGACY = 0,
    CUI_POPOVER_APPEARANCE_SOURCE,
    CUI_POPOVER_APPEARANCE_NONE
} cui_popover_appearance;
enum {
    CUI_POPOVER_REFINE_BACKGROUND = 1u << 0,
    CUI_POPOVER_REFINE_FOREGROUND = 1u << 1,
    CUI_POPOVER_REFINE_RADIUS = 1u << 2,
    CUI_POPOVER_REFINE_PADDING = 1u << 3,
    CUI_POPOVER_REFINE_BORDER = 1u << 4,
    CUI_POPOVER_REFINE_SHADOWS = 1u << 5
};
typedef struct {
    unsigned flags;
    cui_color background, foreground, border;
    float radius, padding, border_width;
    size_t shadow_count;
    cui_box_shadow shadows[CUI_BOX_SHADOW_MAX];
} cui_popover_refinement;
typedef struct {
    cui_color background, foreground, border;
    float radius, padding, border_width;
    size_t shadow_count;
    cui_box_shadow shadows[CUI_BOX_SHADOW_MAX];
} cui_popover_style;
/* Pure validators/resolver; invalid input leaves output untouched. Source p_3 is
 * 12 logical pixels at pinned 16px rem; customize padding for another host rem. */
bool cui_popover_refinement_valid(cui_popover_appearance, cui_popover_refinement);
bool cui_popover_style_resolve(const cui_theme *, cui_popover_appearance, cui_popover_refinement,
                               cui_popover_style *out);
typedef struct {
    cui_rect viewport, anchor;
    float width, height, margin;
    cui_sheet_edge edge;
    cui_popup_side side;
    bool modal_popover, no_escape_dismiss, no_outside_dismiss, alert_outside_dismiss, hide_close,
        show_alert_close;
    int16_t layer; /* Zero chooses 100; nested surfaces should use a higher layer. */
    bool no_sheet_resize;
    float sheet_minimum, sheet_maximum;
    cui_surface_decision_fn decide; /* False keeps the surface open. */
    void *decision_user;
    cui_surface_closed_fn on_close; /* Runs after close and focus restoration. */
    void *close_user;
    /* Zero preserves immediate opening/closing. Uses cui_frame_time(ui). */
    float sheet_animation_seconds;
    bool reduced_motion;
    const cui_scrollbar_style *scrollbar_style; /* Borrowed during begin; NULL uses theme. */
    /* Consumed only for POPOVER; copied values, no retained layer pointers. */
    cui_popover_appearance popover_appearance;
    cui_popover_refinement popover_refinement;
} cui_surface_options;
void cui_surface_init(cui_surface_state *, cui_id, cui_surface_kind);
void cui_surface_open(cui_context *, cui_surface_state *, cui_id trigger);
void cui_surface_close(cui_context *, cui_surface_state *, cui_surface_result);
/* Safe between frames; the next submitted frame applies the decision policy. */
void cui_surface_request_close(cui_context *, cui_surface_state *, cui_surface_result);
cui_surface_result cui_surface_take_result(cui_surface_state *);
cui_rect cui_surface_geometry(cui_surface_kind, cui_surface_options);
cui_rect cui_surface_current_geometry(const cui_surface_state *, cui_surface_options);
cui_rect cui_surface_resize_bounds(const cui_surface_state *, cui_surface_options);
/* Parts: 1 close, 2 confirm, 3 cancel, 4 title, 5 resize, 6 body viewport, 7 body content. */
cui_id cui_surface_part_id(cui_id, unsigned part);
/* Reduction precedes submission. Animated sheets retain their modal through exit. */
bool cui_base_surface_update(cui_context *, cui_surface_state *, cui_surface_options);
bool cui_base_surface_begin(cui_context *, cui_surface_state *, cui_str title, cui_str description,
                            cui_surface_options);
void cui_base_surface_end(cui_context *);
/* Submit inside a box/portal with resize part ID, after base begin. */
void cui_base_surface_submit_resize(cui_context *, const cui_surface_state *, cui_surface_options);
void cui_base_surface_submit_body(cui_context *, cui_surface_state *);
/* Caller composes arbitrary children, then ends the surface. */
bool cui_surface_begin(cui_context *, const cui_theme *, cui_surface_state *, cui_str title,
                       cui_str description, cui_surface_options);
void cui_surface_end(cui_context *);
/* Close the body and begin a fixed footer row; surface_end closes the footer. */
void cui_surface_footer_begin(cui_context *, const cui_theme *);
/* Complete confirmation composition. Results persist until explicitly taken. */
void cui_alert_dialog(cui_context *, const cui_theme *, cui_surface_state *, cui_str title,
                      cui_str description, cui_str confirm, cui_str cancel, cui_surface_options);
#ifdef __cplusplus
}
#endif
#endif
