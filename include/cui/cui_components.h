/* SPDX-License-Identifier: MIT */
#ifndef CUI_COMPONENTS_H
#define CUI_COMPONENTS_H
#include "cui_base.h"
#include "cui_carousel.h"
#include "cui_choice.h"
#include "cui_chrome.h"
#include "cui_content.h"
#include "cui_description.h"
#include "cui_disclosure.h"
#include "cui_form.h"
#include "cui_icon.h"
#include "cui_numeric.h"
#include "cui_otp.h"
#include "cui_picker.h"
#include "cui_resizable.h"
#include "cui_scroll.h"
#include "cui_status.h"
#include "cui_surface.h"
#include "cui_table.h"
#include "cui_theme.h"
#include "cui_toolbar.h"
#include "cui_tree.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_BUTTON_PRIMARY,
    CUI_BUTTON_SECONDARY,
    CUI_BUTTON_OUTLINE,
    CUI_BUTTON_GHOST,
    CUI_BUTTON_DANGER,
    CUI_BUTTON_LINK,
    CUI_BUTTON_WARNING,
    CUI_BUTTON_SUCCESS,
    CUI_BUTTON_INFO
} cui_button_variant;
typedef enum {
    CUI_CONTROL_MEDIUM,
    CUI_CONTROL_SMALL,
    CUI_CONTROL_LARGE,
    CUI_CONTROL_XSMALL
} cui_control_size;
typedef void (*cui_button_content_fn)(cui_context *, void *user);
typedef struct {
    cui_button_variant variant;
    cui_control_size size;
    bool disabled, loading, selected;
    float width; /* zero fits contents; negative grows within parent */
    cui_str leading, trailing;
    cui_str icon;                  /* Catalog name/path; theme.icon_loader may replace its SVG. */
    bool icon_only;                /* Keep name for accessibility; render a square icon button. */
    cui_button_content_fn content; /* Optional arbitrary child content instead of label. */
    void *content_user;
    bool outline; /* Orthogonal colored outline; selected is presentation only. */
} cui_button_options;
/* Paint an independently registered button without reducing events or submitting semantics. */
void cui_button_paint(cui_context *, cui_id, const cui_theme *, cui_str name, cui_button_options);
bool cui_button_ex(cui_context *, cui_id, const cui_theme *, cui_str name, cui_button_options);
bool cui_button(cui_context *, cui_id, const cui_theme *, cui_str, bool disabled);
/* Paint-only antialiased check glyph for composed selection indicators. */
void cui_checkmark(cui_context *, float size, cui_color);
typedef struct {
    cui_control_size size;
    bool disabled, required, invalid, skip_tab, hide_focus_ring;
    cui_str accessibility_label;
    cui_button_content_fn content;
    void *content_user;
} cui_checkbox_options;
bool cui_checkbox_ex(cui_context *, cui_id, const cui_theme *, cui_str, cui_check_state *,
                     cui_checkbox_options);
bool cui_checkbox(cui_context *, cui_id, const cui_theme *, cui_str, bool *, bool disabled);
typedef void (*cui_boolean_change_fn)(void *user, bool requested);
typedef struct {
    double position, velocity, target, updated_at, next_wake_seconds;
    bool initialized;
    cui_hover_state tooltip;
} cui_switch_state;
typedef struct {
    cui_control_size size;
    bool disabled, skip_tab, hide_focus_ring, reduced_motion, label_left, custom_color;
    float width;
    cui_color color;
    cui_str accessibility_label, tooltip;
    cui_button_content_fn content;
    void *content_user;
    cui_boolean_change_fn on_change;
    void *change_user;
    double now_seconds;
    cui_hover_options tooltip_options;
} cui_switch_options;
cui_switch_options cui_switch_options_default(void);
cui_id cui_switch_track_id(cui_id);
cui_id cui_switch_thumb_id(cui_id);
/* Callback is a controlled request; without it, activation writes *value.
 * State is optional for static painting, retained for interruptible spring motion. */
bool cui_switch_ex(cui_context *, cui_id, const cui_theme *, cui_str label, bool *value,
                   cui_switch_state *, cui_switch_options);
bool cui_switch(cui_context *, cui_id, const cui_theme *, cui_str, bool *, bool disabled);
typedef enum { CUI_TOGGLE_GHOST, CUI_TOGGLE_OUTLINE } cui_toggle_variant;
typedef struct {
    cui_toggle_variant variant;
    cui_control_size size;
    bool disabled, skip_tab, focus_ring;
    float width;
    cui_str accessibility_label, tooltip;
    cui_button_content_fn content;
    void *content_user;
    cui_boolean_change_fn on_change;
    void *change_user;
    cui_corner_radii corner_radii;
    cui_border_edges border_edges;
    bool custom_corners, custom_border_edges;
    cui_hover_options tooltip_options;
} cui_toggle_options;
cui_toggle_options cui_toggle_options_default(void);
/* Paint only; compose with independently registered headless toggle behavior.
 * Does not register semantics, mutate state, invoke on_change or open tooltips. */
void cui_toggle_paint(cui_context *, cui_id, const cui_theme *, cui_str, bool, cui_toggle_options);
bool cui_toggle_ex(cui_context *, cui_id, const cui_theme *, cui_str, bool *,
                   cui_hover_state *tooltip_state, cui_toggle_options);
typedef struct {
    cui_id id;
    cui_str label;
    cui_toggle_options options;
    cui_hover_state *tooltip_state;
} cui_toggle_item;
typedef void (*cui_toggle_group_change_fn)(void *, const bool *requested_values, size_t count);
typedef struct {
    cui_control_size size;
    cui_toggle_variant variant;
    bool disabled, segmented, custom_gap;
    float gap;
    cui_str accessibility_label;
    cui_toggle_group_change_fn on_change;
    void *change_user;
    /* Required with on_change; borrowed synchronous request scratch, separate from values. */
    bool *requested_values;
    size_t requested_capacity;
} cui_toggle_group_style_options;
bool cui_toggle_group_ex(cui_context *, cui_id, const cui_theme *, const cui_toggle_item *,
                         bool *values, size_t count, cui_toggle_group_style_options);
bool cui_toggle(cui_context *, cui_id, const cui_theme *, cui_str, bool *, bool disabled);
typedef cui_checkbox_options cui_radio_options;
bool cui_radio_ex(cui_context *, cui_id, const cui_theme *, cui_str, int *, int option,
                  cui_radio_options);
bool cui_radio(cui_context *, cui_id, const cui_theme *, cui_str, int *, int option, bool disabled);
bool cui_slider(cui_context *, cui_id, const cui_theme *, cui_str, float *, float minimum,
                float maximum, float step, bool disabled);
typedef struct {
    cui_input_behavior_state behavior;
    float scroll_x;
} cui_input_state;
typedef enum {
    CUI_INPUT_MEDIUM,
    CUI_INPUT_SMALL,
    CUI_INPUT_LARGE,
    CUI_INPUT_XSMALL,
    CUI_INPUT_CUSTOM
} cui_input_size;
typedef struct {
    float height, font_size, line_height, padding_x, padding_y, gap;
} cui_input_metrics;
typedef struct {
    bool disabled, readonly, required, invalid;
    float width; /* Zero defaults to 280; negative grows. */
    cui_str placeholder;
    const cui_platform *clipboard;
    cui_input_size size;
    float custom_size, density; /* density 0 derives theme font_size/16. */
    float height;               /* Optional frame-height refinement; line box remains 20*density. */
    bool custom_height;  /* Preserve explicit zero height. Positive height always overrides. */
    bool custom_padding; /* Use padding_x/y in pixels, including explicit zero. */
    float padding_x, padding_y;
    bool no_appearance, no_border, hide_focus_ring;
    cui_align text_align;
} cui_input_options;
bool cui_input_measure(cui_input_options, float root_font_size, cui_input_metrics *);
cui_id cui_input_viewport_id(cui_id);
/* Paint only: compose with a reducer without applying the event stream twice. */
void cui_input_paint(cui_context *, cui_id, const cui_theme *, cui_str, cui_edit *,
                     cui_input_state *, cui_input_options);
cui_input_result cui_input_ex(cui_context *, cui_id, const cui_theme *, cui_str name, cui_edit *,
                              cui_input_state *, cui_input_options);
cui_input_result cui_input(cui_context *, cui_id, const cui_theme *, cui_str name, cui_edit *,
                           bool disabled);
void cui_label(cui_context *, const cui_theme *, cui_str);
void cui_badge(cui_context *, cui_id, const cui_theme *, cui_str);
void cui_progress(cui_context *, cui_id, const cui_theme *, float fraction, float width);
void cui_separator(cui_context *, cui_id, const cui_theme *, float width);
#ifdef __cplusplus
}
#endif
#include "cui_settings.h"
#endif
