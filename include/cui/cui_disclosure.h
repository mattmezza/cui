/* SPDX-License-Identifier: MIT */
#ifndef CUI_DISCLOSURE_H
#define CUI_DISCLOSURE_H
#include "cui.h"
#include "cui_theme.h"
#include "cui_typography.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Arrays and controlled expanded state belong to the caller. IDs must be unique,
 * nonzero and stable across frames, including IDs submitted by panel callbacks. */
typedef struct {
    cui_id id;
    cui_str label;
    bool disabled;
} cui_disclosure_item;
/* Custom title/icon body; trigger semantics and activation remain item-owned. */
typedef void (*cui_disclosure_header_fn)(cui_context *, const cui_disclosure_item *, size_t index,
                                         bool expanded, void *user);
/* Caller-owned, stable per-panel motion/measurement storage. Zero initialize. */
typedef struct {
    cui_id id;
    cui_rect measured;
    float progress, velocity;
    bool initialized;
} cui_disclosure_motion;
/* Reversible reveal; initial state and reduced motion snap.
 * Legacy step uses critical damping; response is natural-period seconds
 * (zero defaults to .2). Styled components use theme.spring_control by default. */
void cui_disclosure_motion_step(cui_disclosure_motion *, bool open, float dt, float response,
                                bool reduced_motion);
/* Explicit theme spring policy; response zero or travel false snaps. */
void cui_disclosure_motion_step_ex(cui_disclosure_motion *, bool open, float dt,
                                   cui_spring_settings, bool reduced_motion);
typedef struct {
    bool multiple, collapsible, disabled, bordered;
    cui_disclosure_header_fn render_header;
    void *header_user;
    const cui_box *trigger_boxes, *panel_boxes; /* optional per-item full box styles */
    cui_disclosure_motion *motion; /* optional per-item array, stable through end_frame */
    size_t motion_count;
    float dt, response;
    bool reduced_motion;
    size_t style_count; /* available entries in either nonnull style array */
} cui_accordion_options;
typedef struct {
    bool disabled, borderless;
    cui_disclosure_header_fn render_header;
    void *header_user;
    const cui_box *trigger_box, *panel_box; /* optional full box styles */
    cui_disclosure_motion *motion;          /* optional stable through end_frame */
    float dt, response;
    bool reduced_motion;
} cui_collapsible_options;
/* Optional visual refinements for one trigger. Custom colors can be transparent.
 * The first applicable custom background wins: disabled, pressed, hovered, then
 * expanded. Otherwise use the normal trigger box background. Focus independently
 * overrides the outline color. A custom cursor applies to enabled triggers. */
typedef struct {
    bool custom_hover, custom_pressed, custom_expanded, custom_disabled, custom_focus;
    cui_color hover, pressed, expanded, disabled, focus;
    bool custom_cursor;
    cui_cursor cursor;
} cui_disclosure_trigger_style;
/* Optional complete box and title typography. NULL fields inherit. Title
 * color also inherits unless custom_color is set, including explicit zero. */
typedef struct {
    const cui_box *box;
    const cui_typography_token *typography;
    bool custom_color;
    cui_color color;
} cui_disclosure_trigger_part;
typedef struct {
    cui_disclosure_trigger_part normal, expanded, disabled;
} cui_disclosure_trigger_appearance;
/* Joined Accordion only. An explicit zero thickness omits the divider. */
typedef struct {
    float thickness, inset_left, inset_right;
    cui_color color;
} cui_disclosure_separator_style;
/* Shared trigger defaults plus optional per-item overrides. For each field,
 * apply normal, then expanded when open, then disabled when disabled. At
 * each step, an item field overrides the shared field. The resolved box
 * replaces options.trigger_box(es); the existing background/focus/cursor
 * styles still apply. Typography/color apply to the built-in title. Custom
 * headers keep control over their own styling. Optional data is read during
 * submission. Collapsible uses item zero. */
typedef struct {
    cui_disclosure_trigger_appearance trigger;
    const cui_disclosure_trigger_appearance *items;
    size_t item_count;
    const cui_disclosure_separator_style *separator;
} cui_disclosure_appearance;
/* Optional title refinements for cui_*_present_with_title_states. NULL
 * typography or an unset custom_color inherits the corresponding field. */
typedef struct {
    const cui_typography_token *typography;
    bool custom_color;
    cui_color color;
} cui_disclosure_title_style;
typedef struct {
    cui_disclosure_title_style hovered, focus_visible, pressed;
} cui_disclosure_title_states;
/* Built-in enabled titles only. After normal/expanded appearance, apply
 * hovered, keyboard-visible focus, then pressed (pointer or Space capture).
 * An item overrides shared fields within each state. Collapsible uses item 0.
 * Hover uses Core's previous hit geometry and current mouse or pen position.
 * Tokens, arrays and colors are read during submission. A different title
 * metric can reflow FIT rows. Supply a bounded trigger box for fixed geometry. */
typedef struct {
    cui_disclosure_title_states trigger;
    const cui_disclosure_title_states *items;
    size_t item_count;
} cui_disclosure_title_interactions;
/* Optional presentation for the styled _present entry points. Width zero fits
 * content, -1 fills the parent, and a positive width uses logical pixels.
 * Per-trigger/panel box overrides replace their default boxes. A custom header
 * replaces the title and icon. Trigger state colors refine the resolved box.
 * The default trigger keeps the full accessible label and clips long visible
 * titles at bounded widths. Icons are decorative catalog names (or paths),
 * borrowed during submission and resolved using the theme's SVG loader.
 * Empty icon names use the built-in chevrons for their respective states.
 * joined places Accordion rows in one continuous surface with separators.
 * Accordion options.bordered selects the outer outline in that mode. An
 * optional container box replaces the default outer box. A nonzero width above
 * overrides the box width; items still stack vertically. Trigger styles are
 * indexed by item, or index zero for Collapsible. Optional boxes and styles are
 * read during submission. */
typedef struct {
    cui_design_size design_size;
    float width;
    cui_str collapsed_icon, expanded_icon;
    bool joined;
    const cui_box *container_box;
    const cui_disclosure_trigger_style *trigger_styles;
    size_t trigger_style_count;
    const cui_disclosure_appearance *appearance;
} cui_disclosure_presentation;
typedef void (*cui_disclosure_panel_fn)(cui_context *, size_t index, void *user);
/* Applied transition observers: see cui_disclosure_events.h. */
/* Base submits semantic triggers without layout or painting. */
bool cui_base_collapsible(cui_context *, cui_id, cui_str, bool *open, bool disabled);
bool cui_base_accordion(cui_context *, const cui_disclosure_item *, size_t count, bool *open,
                        cui_accordion_options);
/* Without motion, styled components render panels only while expanded. Size and
 * colors come from the supplied theme; callbacks can compose arbitrary CUI. */
bool cui_collapsible_ex(cui_context *, cui_id, const cui_theme *, cui_str, bool *open,
                        cui_collapsible_options, cui_disclosure_panel_fn, void *);
bool cui_collapsible(cui_context *, cui_id, const cui_theme *, cui_str, bool *open, bool disabled,
                     cui_disclosure_panel_fn, void *);
bool cui_accordion(cui_context *, cui_id container, const cui_theme *, const cui_disclosure_item *,
                   size_t count, bool *open, cui_accordion_options, cui_disclosure_panel_fn,
                   void *);
#ifdef __cplusplus
}
#endif
#endif
