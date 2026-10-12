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
    CUI_KEY_PAGEDOWN
} cui_key;
enum { CUI_MOD_SHIFT = 1, CUI_MOD_CTRL = 2, CUI_MOD_ALT = 4, CUI_MOD_META = 8 };
typedef enum { CUI_POINTER_PRIMARY, CUI_POINTER_SECONDARY, CUI_POINTER_MIDDLE } cui_pointer_button;
typedef struct {
    cui_event_type type;
    float x, y, dx, dy;
    cui_key key;
    unsigned modifiers;
    bool repeat;
    cui_pointer_button pointer_button; /* Zero preserves primary-button initializers. */
    char text[64];
} cui_event;
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
    CUI_ROLE_LOG
} cui_role;
typedef enum {
    CUI_ORIENTATION_UNSPECIFIED,
    CUI_ORIENTATION_HORIZONTAL,
    CUI_ORIENTATION_VERTICAL
} cui_orientation;
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
} cui_box;
typedef enum {
    CUI_DRAW_RECT,
    CUI_DRAW_BORDER,
    CUI_DRAW_TEXT,
    CUI_DRAW_CLIP_PUSH,
    CUI_DRAW_CLIP_POP
} cui_draw_type;
typedef struct {
    cui_draw_type type;
    cui_rect bounds;
    cui_color color;
    float radius, stroke, font_size;
    cui_str text;
    cui_corner_radii corner_radii;
    cui_border_edges border_edges;
    bool custom_corners, custom_border_edges;
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
typedef struct {
    float width, height;
    size_t max_elements, max_commands, frame_bytes;
    cui_measure_fn measure;
    void *measure_user;
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
void cui_begin_box(cui_context *ui, cui_id id, cui_box box);
/* Floating viewport coordinates, independent of ancestor clipping/layout.
 * layer must be positive. Close with cui_end_box. Portal dimensions come from
 * bounds (zero dimensions fit contents); box controls styling and child layout. */
void cui_begin_portal(cui_context *ui, cui_id id, cui_box box, cui_rect bounds, int16_t layer);
void cui_end_box(cui_context *ui);
void cui_text(cui_context *ui, cui_str text, float font_size, cui_color color);
/* Uses the same configured font measurement as layout; no frame allocation. */
cui_size cui_measure_text(cui_context *, cui_str text, float font_size);
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
/* Validated activation generated by this event, before widgets consume it.
 * Zero for cancelled pointer/Space capture, repeats, and non-activation events. */
cui_id cui_event_activation(cui_context *ui, size_t index);
const cui_semantic *cui_semantics(cui_context *ui, size_t *count);
cui_rect cui_bounds(cui_context *ui, cui_id id);
/* Register one current-layout bounds result per ID for this frame. The retained
 * pointer must remain valid through cui_end_frame, which writes the computed
 * rectangle (zero if absent) and invalidates only when its fields change.
 * Repeating the same ID/pointer is idempotent; conflicting IDs/pointers fail
 * with DUPLICATE_ID. Registrations are bounded by max_elements. Null arguments
 * or zero IDs return false. Call only between begin_frame and end_frame. */
bool cui_track_bounds(cui_context *ui, cui_id id, cui_rect *retained);
/* Previous-frame clipped region, excluding higher interactive layers/modal scopes. */
bool cui_hovered(cui_context *ui, cui_id id);
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
const cui_event *cui_frame_events(cui_context *ui, size_t *count);
cui_error cui_last_error(cui_context *ui);
void cui_clear_error(cui_context *ui);
bool cui_needs_redraw(cui_context *ui);
void cui_invalidate(cui_context *ui);
#ifdef __cplusplus
}
#endif
#endif
