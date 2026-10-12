/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXTAREA_H
#define CUI_TEXTAREA_H
#include "cui.h"
#include "cui_edit.h"
#include "cui_platform.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { CUI_TEXTAREA_OK, CUI_TEXTAREA_INVALID, CUI_TEXTAREA_CAPACITY } cui_textarea_error;
typedef enum {
    CUI_TEXTAREA_MEDIUM,
    CUI_TEXTAREA_XSMALL,
    CUI_TEXTAREA_SMALL,
    CUI_TEXTAREA_LARGE,
    CUI_TEXTAREA_CUSTOM
} cui_textarea_size;
typedef struct {
    size_t start, end, next; /* UTF-8 byte range; next skips a hard newline. */
    float width;
    bool hard_break;
} cui_textarea_line;
typedef struct {
    cui_textarea_line *lines;
    size_t capacity, count, caret_line;
    cui_rect viewport;
    float scroll_x, scroll_y, maximum_x, maximum_y;
    float font_size, line_height, wrap_width, preferred_x;
    uint64_t revision;
    size_t cursor, anchor;
    bool dragging, preferred_valid, upstream_affinity, initialized, focused;
    double blink_started;
    cui_textarea_error error;
} cui_textarea_state;
typedef struct {
    cui_textarea_size size;
    float custom_size, width, height; /* Width0 defaults280; negative grows. Height0 uses rows. */
    size_t rows, min_rows,
        max_rows; /* Zero rows defaults1. Auto-grow uses min1/max10 by default. */
    bool auto_grow, no_wrap, no_appearance, no_border;
    bool submit_on_enter; /* Shift-Enter still inserts a newline. */
    bool disabled, readonly, required, invalid, no_tab_stop;
    bool no_indent, hard_tabs; /* Editable text handles Tab; zero tab_size defaults to two. */
    size_t tab_size;
    bool custom_role;
    cui_role role;
    cui_str placeholder;
    const cui_platform *clipboard;
} cui_textarea_options;
typedef struct {
    float font_size, line_height, padding_x, padding_y;
} cui_textarea_metrics;
typedef struct {
    bool changed, selection_changed, scrolled;
    bool pressed_enter, enter_secondary, enter_shift; /* Last Enter in this frame. */
    cui_edit_result edit_error;
    cui_textarea_error error;
} cui_textarea_result;
/* Caller owns workspace and editor. No allocation; state must not be shared by widgets. */
bool cui_textarea_state_init(cui_textarea_state *, cui_textarea_line *, size_t capacity);
cui_textarea_options cui_textarea_default(void);
bool cui_textarea_measure(cui_textarea_options, float root_font_size, cui_textarea_metrics *);
cui_id cui_textarea_viewport_id(cui_id);
/* Build measured visual rows; viewport is the editable inner bounds. */
cui_textarea_error cui_textarea_layout(cui_context *, cui_edit *, cui_textarea_state *,
                                       cui_textarea_metrics, float viewport_width, bool wrap);
size_t cui_textarea_hit_test(cui_context *, cui_edit *, cui_textarea_state *, float x, float y);
/* Base uses the state's last viewport and supplied metrics, preserving caller layout. */
cui_textarea_result cui_base_textarea(cui_context *, cui_id, cui_str name, cui_edit *,
                                      cui_textarea_state *, cui_textarea_options,
                                      cui_textarea_metrics);
cui_textarea_result cui_textarea(cui_context *, cui_id, const cui_theme *, cui_str name, cui_edit *,
                                 cui_textarea_state *, cui_textarea_options);
#ifdef __cplusplus
}
#endif
#endif
