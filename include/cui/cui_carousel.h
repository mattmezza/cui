/* SPDX-License-Identifier: MIT */
#ifndef CUI_CAROUSEL_H
#define CUI_CAROUSEL_H
#include "cui_content.h"
#include "cui_icon.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_CAROUSEL_ITEM_LIMIT 1024u
#define CUI_CAROUSEL_NO_SELECTION SIZE_MAX
typedef struct {
    cui_id id;
    float start, extent; /* Unscrolled primary-axis coordinates, including padding. */
} cui_carousel_geometry;
typedef void (*cui_carousel_change_fn)(size_t index, void *);
typedef struct {
    cui_carousel_geometry *items;
    size_t capacity, count, selected;
    cui_orientation axis;
    bool looping, runtime_looping, geometry_ready;
    float viewport, cycle, maximum, offset, target, velocity;
    float response, damping; /* Pure motion parameters, default .28s/.85. */
    float pointer_x, pointer_y, gesture_start, gesture_delta;
    size_t gesture_index;
    bool dragging, axis_locked, suppress_click, scrolling, scroll_deferred, wheel_owned,
        scroll_seen;
    bool focus_ring_suppressed, scroll_contact;
    cui_orientation scroll_axis;
    double last_time, last_scroll;
    bool clock_ready;
    cui_rect frame_bounds;
    float frame_width, frame_height; /* Resolved/estimated current frame extent. */
    cui_carousel_change_fn on_change;
    void *change_user;
} cui_carousel_state;
/* Caller workspace/state survive through end_frame. No allocations. */
bool cui_carousel_state_init(cui_carousel_state *, cui_carousel_geometry *, size_t capacity,
                             size_t item_count);
bool cui_carousel_set_count(cui_carousel_state *, size_t);
bool cui_carousel_set_axis(cui_carousel_state *, cui_orientation);
void cui_carousel_set_looping(cui_carousel_state *, bool);
void cui_carousel_set_selected(cui_carousel_state *, size_t); /* Clamped, silent. */
/* Geometry is validated transactionally and copied into caller workspace.
 * Sorted, nonoverlapping positive extents; coordinates must be finite. */
bool cui_carousel_set_geometry(cui_carousel_state *, const cui_carousel_geometry *, size_t,
                               float viewport);
float cui_carousel_snap(const cui_carousel_state *, size_t);
size_t cui_carousel_nearest(const cui_carousel_state *, float offset);
size_t cui_carousel_navigation(const cui_carousel_state *, bool next);
bool cui_carousel_select(cui_carousel_state *, size_t); /* Valid different index emits once. */
bool cui_carousel_step(cui_carousel_state *, bool next);
bool cui_carousel_begin_drag(cui_carousel_state *, float x, float y);
bool cui_carousel_update_drag(cui_carousel_state *, float x, float y);
bool cui_carousel_finish_drag(cui_carousel_state *);
void cui_carousel_cancel(cui_carousel_state *);
/* Wheel deltas use CUI direction: negative moves forward. One step per 28ms burst. */
bool cui_carousel_wheel(cui_carousel_state *, float delta, double now_seconds);
/* Precise deltas keep a dominant-axis lock until end/cancel or 28ms silence. */
bool cui_carousel_scroll(cui_carousel_state *, float dx, float dy, cui_scroll_phase, double now);
/* Captured touch deltas retain their axis/offset until explicit end/cancel;
 * idle time does not terminate a contact. Host must deliver its terminal phase. */
bool cui_carousel_touch_scroll(cui_carousel_state *, float dx, float dy, cui_scroll_phase,
                               double now);
/* Spring response .28s, damping .85, settle epsilon .5px. Returns pending motion. */
bool cui_carousel_tick(cui_carousel_state *, double now_seconds, bool reduced_motion);
/* One periodic placement per logical item, never duplicate callbacks/controls. */
float cui_carousel_item_position(const cui_carousel_state *, size_t);
typedef void (*cui_carousel_render_fn)(cui_context *, const cui_theme *, size_t index, void *);
typedef struct {
    cui_id id;
    cui_str label;
    float extent; /* 0: viewport+leading padding; >0: complete outer extent. */
    cui_carousel_render_fn render;
    void *user;
    bool custom_box;
    cui_box box;
} cui_carousel_item;
typedef struct {
    float width, height, gap; /* Defaults: grow width, 200px height, 16px leading padding. */
    double now_seconds;
    bool reduced_motion, disabled, hide_focus_ring;
    cui_str label;
    bool custom_box;
    cui_box box;
    void (*controls)(cui_context *, const cui_theme *, cui_carousel_state *, cui_id root, void *);
    void *controls_user;
    void (*pagination)(cui_context *, const cui_theme *, cui_carousel_state *, cui_id root, void *);
    void *pagination_user;
    const cui_scrollbar_style
        *scrollbar_style; /* Finite carousels only; looping has no finite range. */
} cui_carousel_options;
cui_carousel_options cui_carousel_default(void);
bool cui_carousel(cui_context *, cui_id, const cui_theme *, cui_carousel_state *,
                  const cui_carousel_item *, size_t, cui_carousel_options);
/* Optional count-entry array of finite fractions in [0,1], borrowed for the
 * call. Zero or NULL uses the ordinary extent. A positive basis replaces the
 * item's extent with a fraction of (viewport + gap), including the leading
 * gap. It reserves at least one logical pixel after the gap and clips overflow
 * to the slide. Growing width uses the last layout's frame width and schedules
 * a follow-up if it changes. */
bool cui_carousel_with_basis(cui_context *, cui_id, const cui_theme *, cui_carousel_state *,
                             const cui_carousel_item *, size_t, cui_carousel_options,
                             const float *basis);
/* Optional workspace for one content-sized slide. Zero initialize before first
 * use and retain at a stable address through end_frame. Keep it with the slide
 * when reordering. Use a distinct workspace per slide and per Carousel. Fields
 * belong to the component. Reset the workspace to discard its last measurement. */
typedef struct {
    cui_rect bounds;
    cui_id slide;
    cui_orientation axis;
} cui_carousel_fit;

typedef struct {
    const float *basis;           /* Optional count-entry array as in with_basis. */
    cui_carousel_fit *const *fit; /* Optional count-entry array; NULL entries use normal sizing. */
} cui_carousel_layout;

/* A fit slide cannot also specify a positive basis or explicit extent.
 * Measure its styled slide box on the primary axis, from one logical pixel
 * up to the viewport. Its custom box can further reduce that limit.
 * The first layout reserves the viewport; subsequent layouts use the last
 * measured extent plus gap. The reserved extent clips paint and input until
 * a redraw applies a new measurement. Custom dimensions/percentage flags
 * are ignored, while padding and other styling apply to the measured box.
 *
 * Render callbacks run once per submitted slide in a frame. They must
 * provide in-flow content with a natural size; fill/percentage children
 * alone cannot provide a preferred size. A size change can schedule a
 * follow-up and realign selection silently. Follow cui_needs_redraw.
 * Both arrays are borrowed for the call. Only the workspace must survive
 * through end_frame. Neither pointer array nor borrowed slide data is retained. */
bool cui_carousel_with_layout(cui_context *, cui_id, const cui_theme *, cui_carousel_state *,
                              const cui_carousel_item *, size_t, cui_carousel_options,
                              cui_carousel_layout);

/* Navigation control: Medium32 by default; caller may select a semantic/custom size,
 * label, appearance and content. Place inside the open Carousel root; positions use retained frame
 * bounds. The built-in control inherits the root's current disabled state. */
typedef enum {
    CUI_CAROUSEL_CONTROL_DEFAULT,
    CUI_CAROUSEL_CONTROL_XSMALL,
    CUI_CAROUSEL_CONTROL_SMALL,
    CUI_CAROUSEL_CONTROL_MEDIUM,
    CUI_CAROUSEL_CONTROL_LARGE,
    CUI_CAROUSEL_CONTROL_CUSTOM
} cui_carousel_control_size;
typedef struct {
    cui_carousel_control_size size;
    float custom_size;
    cui_str label;
    bool custom_box;
    cui_box box;
    cui_carousel_render_fn content;
    void *user;
    cui_hover_state *tooltip;          /* Optional retained state for source control tooltip. */
    cui_hover_options tooltip_options; /* Host viewport is required. */
} cui_carousel_control_options;
bool cui_carousel_control(cui_context *, cui_id, const cui_theme *, cui_carousel_state *,
                          cui_id root, bool next, cui_carousel_control_options);
/* Same activation, styling and axis-aware icon as cui_carousel_control.
 * Place this version in ordinary layout flow inside the Carousel root. */
bool cui_carousel_control_inline(cui_context *, cui_id, const cui_theme *, cui_carousel_state *,
                                 cui_id root, bool next, cui_carousel_control_options);
/* Pair end only when begin succeeds. Default centered row with 8px gap. */
bool cui_carousel_pagination_begin(cui_context *, cui_id, cui_str label, const cui_box *custom_box);
void cui_carousel_pagination_end(cui_context *);
/* Pagination items default XS20, with selected metadata and the root's disabled state. */
bool cui_carousel_pagination_item(cui_context *, cui_id, const cui_theme *, cui_carousel_state *,
                                  cui_id root, size_t, cui_carousel_control_options);
#ifdef __cplusplus
}
#endif
#endif
