/* SPDX-License-Identifier: MIT */
#ifndef CUI_CONTENT_H
#define CUI_CONTENT_H
#include "cui_base.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_ALERT_INFO,
    CUI_ALERT_SUCCESS,
    CUI_ALERT_WARNING,
    CUI_ALERT_ERROR
} cui_alert_kind;
typedef struct {
    cui_alert_kind kind;
    cui_str title, message, icon;
    float width, scale;
    bool banner;
    /* Optional controlled visibility; false omits layout and semantics. */
    bool *visible;
    bool dismissible;
    /* Named SVG; empty selects the kind's default. Legacy icon text overrides it. */
    cui_str icon_name;
} cui_alert_options;
bool cui_alert(cui_context *ui, cui_id id, const cui_theme *theme, cui_alert_options options);
/* Writes first/last word initials (UTF-8 scalars), or "?". Requires 9 bytes. */
size_t cui_avatar_initials(cui_str name, char output[9]);
void cui_avatar(cui_context *ui, cui_id id, const cui_theme *theme, cui_str name, float size);
typedef enum {
    CUI_BUBBLE_FILLED,
    CUI_BUBBLE_SECONDARY,
    CUI_BUBBLE_MUTED,
    CUI_BUBBLE_TINTED,
    CUI_BUBBLE_OUTLINE,
    CUI_BUBBLE_GHOST,
    CUI_BUBBLE_DESTRUCTIVE,
    CUI_BUBBLE_INFO,
    CUI_BUBBLE_SUCCESS,
    CUI_BUBBLE_WARNING,
    CUI_BUBBLE_ERROR
} cui_bubble_variant;
/* Composable surface. Alignment and wrapping belong to the parent layout. */
void cui_begin_bubble(cui_context *ui, cui_id id, const cui_theme *theme,
                      cui_bubble_variant variant, float width);
void cui_end_bubble(cui_context *ui);
void cui_empty(cui_context *ui, cui_id id, const cui_theme *theme, cui_str title,
               cui_str description, float width);
void cui_begin_group_box(cui_context *ui, cui_id id, const cui_theme *theme, cui_str title,
                         float width);
void cui_end_group_box(cui_context *ui);
/* Platform convention selected explicitly; no platform types in public API. */
size_t cui_kbd_format(unsigned modifiers, cui_str key, bool mac_symbols, char *output,
                      size_t capacity);
void cui_kbd(cui_context *ui, cui_id id, const cui_theme *theme, cui_str chord);
/* Seconds are application-owned; invalid/negative values resolve to zero. */
float cui_loading_phase(double seconds, float period);
void cui_spinner(cui_context *ui, cui_id id, const cui_theme *theme, float size, float phase,
                 bool reduced_motion);
void cui_skeleton(cui_context *ui, cui_id id, const cui_theme *theme, float width, float height,
                  float radius, float phase, bool secondary, bool reduced_motion);
void cui_divider(cui_context *ui, cui_id id, const cui_theme *theme, float length, bool vertical);
/* Shared delayed lifecycle for hover cards/tooltips, independent of styling. */
typedef struct {
    bool open, pending, target, suppressed;
    float elapsed;
} cui_hover_state;
bool cui_hover_update(cui_hover_state *state, bool trigger_hovered, bool content_hovered,
                      bool focused, bool escape, bool disabled, float dt, float open_delay,
                      float close_delay);
typedef struct {
    cui_id trigger;
    cui_rect viewport;
    cui_popup_side side;
    float width, height, gap, dt, open_delay, close_delay;
    bool disabled;
} cui_hover_options;
/* Trigger must already have been submitted. State is persistent/application-owned.
 * Explicit dimensions clip content, flip at viewport edges, and bound placement. */
bool cui_tooltip(cui_context *ui, cui_id id, const cui_theme *theme, cui_hover_state *state,
                 cui_str text, cui_hover_options options);
/* Pair end only when begin returns true; accepts arbitrary children without a
 * modal focus trap. Hover/focus inside the open card extends its lifetime. */
bool cui_begin_hover_card(cui_context *ui, cui_id id, const cui_theme *theme,
                          cui_hover_state *state, cui_str name, cui_hover_options options);
void cui_end_hover_card(cui_context *ui);
#ifdef __cplusplus
}
#endif
#endif
