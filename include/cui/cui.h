/* SPDX-License-Identifier: MIT */
#ifndef CUI_H
#define CUI_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_VERSION "0.1.0-dev"
typedef uint64_t cui_id;
typedef struct {
    const char *data;
    size_t length;
} cui_str;
typedef struct {
    float x, y, w, h;
} cui_rect;
typedef struct {
    uint8_t r, g, b, a;
} cui_color;
/* Alpha-premultiplied Oklab interpolation; amount is the second color weight.
 * Clamp to [0,1]; non-finite amount selects the first color. */
cui_color cui_color_mix_oklab(cui_color first, cui_color second, float amount);
typedef struct cui_context cui_context;
typedef enum {
    CUI_OK,
    CUI_ERROR_CAPACITY,
    CUI_ERROR_DUPLICATE_ID,
    CUI_ERROR_LIFECYCLE,
    CUI_ERROR_LAYOUT
} cui_error;
typedef enum {
    CUI_EVENT_POINTER_MOVE,
    CUI_EVENT_POINTER_DOWN,
    CUI_EVENT_POINTER_UP,
    CUI_EVENT_KEY_DOWN,
    CUI_EVENT_KEY_UP,
    CUI_EVENT_TEXT,
    CUI_EVENT_SCROLL,
    CUI_EVENT_CANCEL
} cui_event_type;
typedef enum {
    CUI_KEY_NONE,
    CUI_KEY_TAB,
    CUI_KEY_ENTER,
    CUI_KEY_SPACE,
    CUI_KEY_ESCAPE,
    CUI_KEY_LEFT,
    CUI_KEY_RIGHT,
    CUI_KEY_UP,
    CUI_KEY_DOWN,
    CUI_KEY_HOME,
    CUI_KEY_END,
    CUI_KEY_BACKSPACE,
    CUI_KEY_DELETE,
    CUI_KEY_A,
    CUI_KEY_Z,
    CUI_KEY_Y,
    CUI_KEY_C,
    CUI_KEY_X,
    CUI_KEY_V,
    CUI_KEY_PAGEUP,
    CUI_KEY_PAGEDOWN,
    /* Appended to preserve the original key values used by host adapters. */
    CUI_KEY_B,
    CUI_KEY_D,
    CUI_KEY_E,
    CUI_KEY_F,
    CUI_KEY_G,
    CUI_KEY_H,
    CUI_KEY_I,
    CUI_KEY_J,
    CUI_KEY_K,
    CUI_KEY_L,
    CUI_KEY_M,
    CUI_KEY_N,
    CUI_KEY_O,
    CUI_KEY_P,
    CUI_KEY_Q,
    CUI_KEY_R,
    CUI_KEY_S,
    CUI_KEY_T,
    CUI_KEY_U,
    CUI_KEY_W,
    CUI_KEY_0,
    CUI_KEY_1,
    CUI_KEY_2,
    CUI_KEY_3,
    CUI_KEY_4,
    CUI_KEY_5,
    CUI_KEY_6,
    CUI_KEY_7,
    CUI_KEY_8,
    CUI_KEY_9,
} cui_key;
/* ASCII shortcut identity, independent of text/IME input. Letters normalize to
 * uppercase; unsupported characters/keys map to NONE/zero. */
cui_key cui_key_from_character(char character);
char cui_key_character(cui_key key);
enum { CUI_MOD_SHIFT = 1, CUI_MOD_CTRL = 2, CUI_MOD_ALT = 4, CUI_MOD_META = 8 };
typedef enum { CUI_POINTER_PRIMARY, CUI_POINTER_SECONDARY, CUI_POINTER_MIDDLE } cui_pointer_button;
typedef enum {
    CUI_SCROLL_PHASE_NONE,
    CUI_SCROLL_PHASE_STARTED,
    CUI_SCROLL_PHASE_MOVED,
    CUI_SCROLL_PHASE_ENDED,
    CUI_SCROLL_PHASE_CANCELLED
} cui_scroll_phase;
typedef struct {
    cui_event_type type;
    float x, y, dx, dy;
    cui_key key;
    unsigned modifiers;
    bool repeat;
    cui_pointer_button pointer_button; /* Zero preserves primary-button initializers. */
    char text[64];
    /* Optional host gesture metadata. Zero preserves legacy wheel steps.
     * Precise deltas are logical pixels; zero-delta end/cancel events are valid. */
    bool scroll_precise;
    cui_scroll_phase scroll_phase;
} cui_event;
/* Caller-owned gesture filter. Clock is monotonic seconds; no allocation.
 * Precise gestures lock their initial dominant axis, release on strong turns,
 * and reset after28ms silence or explicit end/cancel. Line wheels are unchanged.
 * Invalid input leaves both state and event unchanged. */
typedef struct {
    double last_time;
    bool has_last, locked, horizontal;
} cui_scroll_gesture;
bool cui_scroll_gesture_filter(cui_scroll_gesture *, cui_event *, double now);

typedef enum {
    CUI_ROLE_NONE,
    CUI_ROLE_BUTTON,
    CUI_ROLE_CHECKBOX,
    CUI_ROLE_SWITCH,
    CUI_ROLE_RADIO,
    CUI_ROLE_SLIDER,
    CUI_ROLE_TEXTBOX,
    CUI_ROLE_LABEL,
    CUI_ROLE_TAB,
    CUI_ROLE_GROUP,
    CUI_ROLE_TABLIST,
    CUI_ROLE_TABPANEL,
    CUI_ROLE_LIST,
    CUI_ROLE_LISTBOX,
    CUI_ROLE_OPTION,
    CUI_ROLE_MENU,
    CUI_ROLE_MENUITEM,
    CUI_ROLE_DIALOG,
    CUI_ROLE_ALERT,
    CUI_ROLE_STATUS,
    CUI_ROLE_IMAGE,
    CUI_ROLE_PROGRESSBAR,
    CUI_ROLE_TREE,
    CUI_ROLE_TREEITEM,
    CUI_ROLE_MENUITEMCHECKBOX,
    CUI_ROLE_MENUITEMRADIO,
    CUI_ROLE_TABLE,
    CUI_ROLE_ROW,
    CUI_ROLE_CELL,
    CUI_ROLE_COLUMNHEADER,
    CUI_ROLE_GRIDCELL,
    CUI_ROLE_GRID,
    CUI_ROLE_SCROLLBAR,
    CUI_ROLE_ROWHEADER,
    CUI_ROLE_ROWGROUP,
    CUI_ROLE_ALERTDIALOG,
    CUI_ROLE_LINK,
    CUI_ROLE_LISTITEM,
    CUI_ROLE_LOG,
    CUI_ROLE_TOOLBAR,
    CUI_ROLE_SEPARATOR,
    CUI_ROLE_REGION
} cui_role;
typedef enum {
    CUI_ORIENTATION_UNSPECIFIED,
    CUI_ORIENTATION_HORIZONTAL,
    CUI_ORIENTATION_VERTICAL
} cui_orientation;
typedef enum {
    CUI_CURSOR_DEFAULT,
    CUI_CURSOR_POINTER,
    CUI_CURSOR_TEXT,
    CUI_CURSOR_RESIZE_HORIZONTAL,
    CUI_CURSOR_RESIZE_VERTICAL,
    CUI_CURSOR_MOVE,
    CUI_CURSOR_NOT_ALLOWED
} cui_cursor;
typedef struct {
    bool expanded, selected, readonly, multiline, required, invalid, multiselectable, busy,
        scrollable;
    cui_orientation orientation;
    cui_id controls, labelled_by, parent;
    /* Passive label/addon pointer clicks focus this target before later queued keys. */
    cui_id focus_target;
    cui_str description;
    float minimum, maximum;
    unsigned level, pos_in_set, set_size;
    unsigned row_index, column_index, row_count, column_count;
    float scroll_x, scroll_y, scroll_max_x, scroll_max_y;
    /* Route only dominant-axis wheel motion for the declared orientation. */
    bool scroll_axis_lock;
    /* Horizontal masks retain horizontal-dominant wheel events at both edges,
     * including when content fits. Normal scroll owners leave this false. */
    bool scroll_trap_horizontal;
    bool scroll_gesture_lock; /* Opt-in per-owner precise gesture filtering. */
    /* Opt a normally passive semantic container into keyboard/pointer focus. */
    bool focusable;
    /* Primary pointer activation without opting into focus or keyboard activation. */
    bool pointer_activatable;
    bool handles_tab; /* Focused editable control consumes Tab and Shift-Tab itself. */
    cui_cursor cursor;
    bool no_focus; /* Suppress role-default focus; pointer activation remains opt-in. */
} cui_semantic_details;
typedef struct {
    cui_id id;
    cui_role role;
    cui_rect bounds;
    cui_str name;
    bool disabled, checked, focused;
    float value;
    cui_semantic_details details;
    int tab_index; /* 0: Tab stop; -1: pointer/programmatic focus only. */
} cui_semantic;
typedef struct {
    bool hovered, pressed, focused, focus_visible, activated;
} cui_interaction;
typedef enum { CUI_ALIGN_START, CUI_ALIGN_CENTER, CUI_ALIGN_END } cui_align;
typedef struct {
    float top_left, top_right, bottom_left, bottom_right;
} cui_corner_radii;
typedef struct {
    float left, right, top, bottom;
} cui_border_edges;
typedef struct {
    float width, height, padding, gap, radius;
    bool row, clip;
    cui_color background, border;
    float border_width;
    cui_align align_x, align_y;
    float offset_x, offset_y; /* Child translation in clipped boxes; negative scrolls forward. */
    cui_corner_radii corner_radii;
    cui_border_edges border_edges;
    /* Flags preserve zero corners/edges. Radii are clamped to half the shorter
     * side by adapters; border widths use the same integer layout units as
     * border_width. Negative/nonfinite box values normalize to zero. */
    bool custom_corners, custom_border_edges;
    /* Explicit zero dimensions; otherwise zero retains the FIT convention. */
    bool fixed_width, fixed_height;
    /* Measure/layout descendants, including portals, without paint, input or
     * semantics. Application callbacks still run and must avoid side effects. */
    bool layout_only;
    /* Preserve painting/layout but suppress descendant semantics and input. */
    bool inert;
    /* Positive finite caps apply to FIT/GROW/fixed; zero means unbounded. */
    float max_width, max_height;
    /* Inherited per-draw opacity; custom_opacity distinguishes zero from default1.
     * Finite values clamp to0..1; nonfinite values normalize to0. */
    float opacity;
    bool custom_opacity;
    /* Interpret the corresponding dimension as a fraction of the parent inner extent in0..1.
     * Fraction flags take precedence over fixed flags and max caps. */
    bool width_percent, height_percent;
} cui_box;
/* Borrowed straight-alpha RGBA8 pixels, valid through renderer completion.
 * Increment revision whenever pixels change; stride includes row padding. */
typedef struct {
    const unsigned char *pixels;
    unsigned width, height;
    size_t stride;
    uint64_t revision;
} cui_bitmap;
typedef struct {
    cui_bitmap bitmap;
    cui_rect source;
    float opacity;
    bool grayscale;
    cui_color tint;
    bool custom_tint;
} cui_bitmap_paint;
bool cui_bitmap_valid(cui_bitmap);
/* A zero source rectangle selects the whole bitmap. Opacity is explicitly 0..1.
 * FIT dimensions default to source pixel dimensions. */
bool cui_paint_bitmap(cui_context *, cui_id, cui_bitmap_paint, cui_box);
typedef enum {
    CUI_DRAW_RECT,
    CUI_DRAW_BORDER,
    CUI_DRAW_TEXT,
    CUI_DRAW_CLIP_PUSH,
    CUI_DRAW_CLIP_POP,
    CUI_DRAW_IMAGE
} cui_draw_type;
typedef enum {
    CUI_TEXT_WEIGHT_NORMAL,
    CUI_TEXT_WEIGHT_MEDIUM,
    CUI_TEXT_WEIGHT_SEMIBOLD,
    CUI_TEXT_WEIGHT_BOLD
} cui_text_weight;
typedef struct {
    cui_text_weight weight;
    bool italic, monospace;
} cui_text_style;
typedef struct {
    cui_draw_type type;
    cui_rect bounds;
    cui_color color;
    float radius, stroke, font_size;
    cui_text_style text_style;
    cui_str text;
    cui_corner_radii corner_radii;
    cui_border_edges border_edges;
    bool custom_corners, custom_border_edges;
    cui_bitmap_paint bitmap;
} cui_draw_command;
typedef struct {
    const cui_draw_command *commands;
    size_t count;
    float width, height;
} cui_draw_list;
typedef struct {
    float width, height;
} cui_size;
typedef cui_size (*cui_measure_fn)(void *user, cui_str text, float font_size);
typedef cui_size (*cui_measure_styled_fn)(void *user, cui_str text, float font_size,
                                          cui_text_style style);
typedef struct {
    float width, height;
    size_t max_elements, max_commands, frame_bytes;
    cui_measure_fn measure;
    cui_measure_styled_fn measure_styled; /* Preferred for regular and styled text. */
    void *measure_user;
    /* Context resources: zero selects 128 slots and 1 MiB of payload storage.
     * Nested allocations owned by payloads require their own explicit limits. */
    size_t max_resources, resource_bytes;
} cui_config;
cui_str cui_string(const char *s);
cui_context *cui_create(cui_config config);
void cui_destroy(cui_context *ui);
bool cui_queue_event(cui_context *ui, cui_event event);
bool cui_begin_frame(cui_context *ui, float width, float height, float dt);
cui_draw_list cui_end_frame(cui_context *ui);
cui_id cui_id_from(cui_context *ui, const char *key);
bool cui_push_id(cui_context *ui, const char *key);
void cui_pop_id(cui_context *ui);
/* Paint one rectangle below children of the current open box, clipped to it.
 * Coordinates are relative to the box's current-frame origin; last call wins. */
bool cui_set_underlay(cui_context *, cui_rect, cui_color, cui_corner_radii);
typedef struct {
    cui_rect bounds;
    cui_align horizontal, vertical;
    /* Growing dimensions add bounds.w/h to the current box size, clamped at zero.
     * Alignment is resolved after sizing; x/y then offset that aligned origin. */
    bool grow_width, grow_height;
} cui_underlay_layout;
bool cui_set_underlay_ex(cui_context *, cui_underlay_layout, cui_color, cui_corner_radii);
/* Borrowed bitmap below children and above the box background, with the same
 * first-frame grow/alignment and lifetime rules as solid underlays. */
bool cui_set_bitmap_underlay(cui_context *, cui_underlay_layout, cui_bitmap_paint,
                             cui_corner_radii);
void cui_begin_box(cui_context *ui, cui_id id, cui_box box);
/* Zero-flow child at parent-relative bounds, inheriting clipping and layer.
 * A width/height of -1 fills the corresponding parent extent; zero is fixed.
 * Must be nested in a box. End with cui_end_box, including invalid bounds. */
void cui_begin_absolute_box(cui_context *ui, cui_id id, cui_box box, cui_rect bounds);
/* Floating viewport coordinates, independent of ancestor clipping/layout.
 * layer must be positive. Close with cui_end_box. Portal dimensions come from
 * bounds (zero dimensions fit contents); box controls styling and child layout. */
void cui_begin_portal(cui_context *ui, cui_id id, cui_box box, cui_rect bounds, int16_t layer);
void cui_end_box(cui_context *ui);
void cui_text(cui_context *ui, cui_str text, float font_size, cui_color color);
/* Explicit logical line height; zero selects the measured natural line height. */
void cui_text_with_line_height(cui_context *, cui_str, float font_size, cui_color,
                               float line_height);
void cui_text_ex(cui_context *, cui_str, float font_size, cui_color, float line_height,
                 cui_text_style);
/* Unwrapped text for caller-laid-out lines. Overflows narrow boxes; use a
 * clipping ancestor when needed. Same style and line-height rules as text_ex. */
void cui_text_line(cui_context *, cui_str, float font_size, cui_color, float line_height,
                   cui_text_style);
/* Uses the same configured font measurement as layout; no frame allocation. */
cui_size cui_measure_text(cui_context *, cui_str text, float font_size);
cui_size cui_measure_text_ex(cui_context *, cui_str text, float font_size, cui_text_style);
/* Call after changing a host font registry or styled measurement policy. */
void cui_text_metrics_changed(cui_context *);
cui_interaction cui_interact(cui_context *ui, cui_id id, cui_role role, cui_str name, bool disabled,
                             bool checked, float value);
/* Update the controlled value of a semantic already submitted this frame. */
void cui_update_semantic(cui_context *ui, cui_id id, bool checked, float value);
/* Refine a submitted current-frame role without replacing its other metadata. */
bool cui_set_semantic_role(cui_context *, cui_id, cui_role);
void cui_set_semantic_details(cui_context *ui, cui_id id, cui_semantic_details details);
void cui_set_tab_stop(cui_context *ui, cui_id id, bool enabled);
/* Target resolved in input order against previous frame regions. */
cui_id cui_event_target(cui_context *ui, size_t index);
/* Defer an unconsumed scroll event currently routed to from_id to an eligible
 * layout ancestor. Preserves deltas and phase, clipping, modal/overlay and axis
 * constraints. Call before the ancestor handles input. False leaves it intact. */
bool cui_defer_scroll_event(cui_context *, size_t index, cui_id from_id);
/* Validated activation generated by this event, before widgets consume it.
 * Zero for cancelled pointer/Space capture, repeats, and non-activation events. */
cui_id cui_event_activation(cui_context *ui, size_t index);
/* Cooperative bubbling: leaf handlers consume events before ancestor handlers.
 * Event targets remain available for diagnostics; callers check consumption. */
bool cui_event_consumed(cui_context *, size_t index);
bool cui_consume_event(cui_context *, size_t index);
/* Capture-phase cancellation: suppress only this event's pending activation.
 * Call before submitting its target widget. Event target and pointer release
 * remain intact; false means no pending activation (including already used). */
bool cui_cancel_event_activation(cui_context *, size_t index);
/* Whether the event target belonged to this named focus scope (or a nested
 * scope) in the previous frame. The scope must be active in this frame.
 * Portals retain their logical scope. This query does not consume input. */
bool cui_event_within_scope(cui_context *, size_t index, cui_id scope);
/* Original topmost eligible pointer/scroll hit, before capture or wheel routing.
 * Uses clipped previous-frame regions, modal and overlay ordering. The named
 * logical scope must be active now. Non-pointer events return false. */
bool cui_event_hit_within_scope(cui_context *, size_t index, cui_id scope);
const cui_semantic *cui_semantics(cui_context *ui, size_t *count);
cui_rect cui_bounds(cui_context *ui, cui_id id);
/* Update a current-frame clipped box before end_frame computes layout. Useful
 * for scroll events deferred by children after their parent's begin call. */
bool cui_set_box_scroll_offset(cui_context *, cui_id, float x, float y);
/* Register one current-layout bounds result per ID for this frame. The retained
 * pointer must remain valid through cui_end_frame, which writes the computed
 * rectangle (zero if absent) and invalidates only when its fields change.
 * Repeating the same ID/pointer is idempotent; conflicting IDs/pointers fail
 * with DUPLICATE_ID. Registrations are bounded by max_elements. Null arguments
 * or zero IDs return false. Call only between begin_frame and end_frame. */
bool cui_track_bounds(cui_context *ui, cui_id id, cui_rect *retained);
/* Previous-frame clipped region, excluding higher interactive layers/modal scopes. */
bool cui_hovered(cui_context *ui, cui_id id);
/* Cursor of the captured or topmost hovered enabled semantic from the completed
 * frame. Hosts apply this after end_frame; clipping and modal scopes apply. */
cui_cursor cui_pointer_cursor(cui_context *ui);
/* Current primary-pointer or Space capture; submit behavior before querying. */
bool cui_pressed(cui_context *ui, cui_id id);
cui_id cui_focused(cui_context *ui);
void cui_focus(cui_context *ui, cui_id id);
/* Is a coordinate-based opener permitted by current/previous modal scopes?
 * owned_scope allows reanchoring the opener's own popup. Call before opening it. */
bool cui_pointer_scope_allowed(cui_context *, cui_id owned_scope);
bool cui_begin_focus_scope(cui_context *ui, cui_id id, bool modal);
void cui_end_focus_scope(cui_context *ui);
/* Withdraw current scope descendants before painting a closing popup. Paired
 * begin/end remain required. Restores a previous-frame enabled trigger, if any. */
bool cui_dismiss_focus_scope(cui_context *ui, cui_id scope, cui_id trigger);
bool cui_focus_restore(cui_context *ui, cui_id trigger);
/* Scope ancestry, using current submissions or the last completed frame. */
bool cui_focus_within(cui_context *ui, cui_id scope);
/* After submitting descendants, move through current enabled Tab stops in this
 * scope, wrapping at either end. Requires focus already inside the scope.
 * Returns true when an eligible stop exists, even if it is the current one. */
bool cui_focus_step_within(cui_context *, cui_id scope, bool forward);
/* Hover follows focus-scope ownership, including explicitly scoped portals. */
bool cui_hover_within(cui_context *ui, cui_id scope);
const cui_event *cui_frame_events(cui_context *ui, size_t *count);
cui_error cui_last_error(cui_context *ui);
void cui_clear_error(cui_context *ui);
bool cui_needs_redraw(cui_context *ui);
void cui_invalidate(cui_context *ui);
/* Stable zeroed context storage keyed by owner and namespace. Use only in-frame.
 * Finish rendering a draw list before the next begin_frame: that call collects
 * resources used last frame and destroys those omitted for a complete frame.
 * Callbacks must not reenter this context. Existing keys require identical size
 * and callbacks. Capacity/allocation failure reports CUI_ERROR_CAPACITY. */
typedef void (*cui_resource_fn)(void *payload);
void *cui_resource_get(cui_context *, cui_id owner, cui_id namespace_id, size_t bytes,
                       cui_resource_fn collect, cui_resource_fn destroy);
float cui_frame_delta(cui_context *);
double cui_frame_time(cui_context *);
bool cui_reduced_motion(cui_context *);
void cui_set_reduced_motion(cui_context *, bool);
#ifdef __cplusplus
}
#endif
#endif
