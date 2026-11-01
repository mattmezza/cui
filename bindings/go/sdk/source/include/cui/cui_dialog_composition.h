/* SPDX-License-Identifier: MIT */
#ifndef CUI_DIALOG_COMPOSITION_H
#define CUI_DIALOG_COMPOSITION_H
#include "cui_scroll.h"
#include "cui_surface.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_DIALOG_COMPOSITION_VERSION_1 1u
/* New records only; never cast existing surface options/state to these. */
typedef enum {
    CUI_DIALOG_HEADER,
    CUI_DIALOG_TITLE,
    CUI_DIALOG_DESCRIPTION,
    CUI_DIALOG_CONTENT,
    CUI_DIALOG_BODY,
    CUI_DIALOG_FOOTER,
    CUI_DIALOG_SLOT_COUNT
} cui_dialog_slot_kind;
typedef enum { CUI_DIALOG_CONFIRM, CUI_DIALOG_CANCEL, CUI_DIALOG_CLOSE } cui_dialog_action_kind;
typedef enum { CUI_DIALOG_SOURCE_TOP, CUI_DIALOG_CENTERED } cui_dialog_placement;
typedef struct cui_dialog_frame cui_dialog_frame;
typedef void (*cui_dialog_slot_fn)(cui_context *, const cui_dialog_frame *, void *);
typedef struct {
    cui_dialog_slot_fn emit;
    void *user;
} cui_dialog_slot;
enum {
    CUI_DIALOG_STYLE_FOREGROUND = 1u,
    CUI_DIALOG_STYLE_FONT_SIZE = 2u,
    CUI_DIALOG_STYLE_PADDING = 4u,
    CUI_DIALOG_STYLE_GAP = 8u,
    CUI_DIALOG_STYLE_ALIGN_X = 16u,
    CUI_DIALOG_STYLE_ALIGN_Y = 32u
};
typedef struct {
    uint32_t fields;
    cui_color foreground;
    float font_size, padding, gap;
    uint32_t align_x, align_y, reserved;
} cui_dialog_slot_style_v1;
enum {
    CUI_DIALOG_PANEL_BACKGROUND = 1u,
    CUI_DIALOG_PANEL_BORDER = 2u,
    CUI_DIALOG_PANEL_RADIUS = 4u,
    CUI_DIALOG_PANEL_PADDING = 8u,
    CUI_DIALOG_PANEL_GAP = 16u,
    CUI_DIALOG_PANEL_BORDER_WIDTH = 32u
};
typedef struct {
    uint32_t struct_size, version, panel_fields, reserved;
    cui_dialog_slot_style_v1 slots[CUI_DIALOG_SLOT_COUNT];
    cui_color background, border;
    float radius, padding, gap, border_width;
} cui_dialog_refinement_v1;
typedef struct {
    uint32_t struct_size, version, present, omit;
    cui_str name, description;
    cui_dialog_slot slots[CUI_DIALOG_SLOT_COUNT];
    const cui_dialog_refinement_v1 *refinement;
} cui_dialog_composition_v1;
enum { CUI_DIALOG_PREFERRED_TOP = 1u, CUI_DIALOG_LAYER_OFFSET = 2u };
typedef struct {
    uint32_t struct_size, version, mode, reserved;
    float width, requested_height, min_height, max_height;
    cui_scroll_region_sizing_v1 content_sizing, body_sizing;
    uint32_t placement, placement_fields;
    float preferred_top, layer_offset;
} cui_dialog_sizing_v1;
/* Caller-owned; initialize before use. Keep it and surface alive through
 * end_frame. Bookkeeping fields are private values, not authoring options.
 * completed[]: Panel,Header,Title,Description,Content,BodyViewport,BodyContent,
 * Footer,Outer,Column. Valid only after successful end_frame while the owner
 * remains open; synchronous callback dismissal does not promise final owner
 * geometry. Already emitted draw data keeps ordinary frame lifetimes. Failed opt-in
 * frames provide no valid geometry. Context binding is cleared by reinit. */
typedef struct {
    uint32_t struct_size, version, private_flags, reserved;
    cui_surface_state *surface;
    const cui_context *private_context;
    float outer_y;
    uint32_t reserved2;
    cui_rect completed[10];
    uint64_t private_submit_frame;
} cui_dialog_layout_v1;
typedef struct {
    bool valid, submitted;
} cui_dialog_submit_result;
bool cui_dialog_layout_init_v1(cui_dialog_layout_v1 *, cui_surface_state *);
/* Required for each closed-to-open transition, including between frames.
 * Already-open is idempotent. Resets scroll/retained geometry before opening;
 * existing surface_open behavior is unchanged. Do not reinit a mounted layout. */
bool cui_dialog_open_v1(cui_context *, cui_dialog_layout_v1 *, cui_id trigger);
bool cui_dialog_composition_valid_v1(const cui_dialog_composition_v1 *);
/* Width420/AUTO/min96, finite viewport-constrained maximum, source-top,
 * Content FLEX1 intrinsic minimum and Body FLEX1 zero minimum. */
bool cui_dialog_sizing_default_v1(cui_surface_options, cui_dialog_sizing_v1 *out);
/* Closed tree ownership: no manual top-level begin/end. Selected callbacks run
 * once, in enum order; text measurement may repeat. Header never suppresses
 * Title/Description. Absent Content/Body are OMIT; present+NULL is empty FLEX1.
 * Title/Description absent callbacks render explicit strings when nonempty.
 * present and omit must not overlap. omit suppresses string fallbacks too.
 * Descriptors/styles are borrowed for this call, callbacks may emit arbitrary
 * balanced children. Manual wrappers cannot cross callbacks. Child text/pixels
 * follow ordinary Core renderer lifetimes. Slot/action/decide/on_close callbacks
 * may close the owner through supported helpers. They must not destroy the
 * context, free/reinitialize the active layout or surface, or invalidate borrowed
 * strings/offset storage. Defer removal/shutdown until after end_frame.
 * Distinct nested dialogs are allowed; same-layout reentry is not.
 * surface options width/height inactive; sizing owns Panel authority. Source
 * Inner clips and oversized Footer may overflow Panel; Outer does not scroll.
 * Source placement uses current Panel height, no previous-geometry paint.
 * Nonpositive available width is rejected before callbacks; no tree is submitted.
 * Defaults preserve source minimum96 even when viewport maximum is smaller.
 * Previous-input geometry is admitted only for an adjacent successful snapshot
 * in the same open lifetime. Unready ordinary composition consumes only remaining
 * previous owner-scope events; this does not accept actions or disable paint.
 * surface.body_viewport/body_content/body_maximum hold admitted previous-input
 * geometry, not current completed layout. Current ranges are published in Body
 * semantics at end_frame; current rectangles use completed[] after success.
 * Pointer/key/contact capture retains existing Core lifetime semantics. Closing
 * and reopening does not promise cancellation of every held child press; this
 * API adds layout composition, not an input-lifetime reset operation.
 * Invalid descriptors return valid=false before callbacks. Late Core failures
 * can follow callbacks; no rollback of application side effects is promised.
 * Existing surface/alert begin/end routes and synchronous dismissal unchanged. */
cui_dialog_submit_result cui_base_dialog_compose_v1(cui_context *, cui_dialog_layout_v1 *,
                                                    cui_surface_options,
                                                    const cui_dialog_sizing_v1 *,
                                                    const cui_dialog_composition_v1 *);
cui_dialog_submit_result cui_dialog_compose_v1(cui_context *, const cui_theme *,
                                               cui_dialog_layout_v1 *, cui_surface_options,
                                               const cui_dialog_sizing_v1 *,
                                               const cui_dialog_composition_v1 *);
/* Token is borrowed only during its active callback; no storage/reuse. */
bool cui_dialog_slot_style_v1_get(const cui_dialog_frame *, cui_dialog_slot_kind,
                                  cui_dialog_slot_style_v1 *out);
bool cui_dialog_slot_begin_v1(cui_context *, const cui_dialog_frame *, cui_dialog_slot_kind,
                              cui_id wrapper_id);
void cui_dialog_slot_end_v1(cui_context *, const cui_dialog_frame *);
/* Submit inside a box with id. Synchronous accepted activation -> existing
 * decide -> immediate close/focus restoration/on_close. Veto keeps open.
 * No global Enter mapping, queued decision or foreign callback adapter. */
cui_interaction cui_base_dialog_action_v1(cui_context *, const cui_dialog_frame *, cui_id, cui_str,
                                          cui_dialog_action_kind, bool disabled);
#ifdef __cplusplus
}
#endif
#endif
