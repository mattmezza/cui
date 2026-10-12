/* SPDX-License-Identifier: MIT */
#ifndef CUI_SCROLL_H
#define CUI_SCROLL_H
#include "cui.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    float x, y, grab_x, grab_y;
    bool dragging_x, dragging_y;
} cui_scroll_state;
typedef struct {
    float width, height, content_width, content_height, bar_thickness, wheel_step, gap;
    bool horizontal, vertical, disabled, content_row;
} cui_scroll_options;
typedef struct {
    cui_size viewport;
    float maximum_x, maximum_y;
} cui_scroll_geometry;
cui_scroll_geometry cui_scroll_layout(cui_scroll_options);
/* Drawing-free, ordered wheel reduction and scrollbar semantics/behavior. */
bool cui_base_scrollable(cui_context *, cui_id, cui_scroll_state *, cui_scroll_options);
/* Compose a scrollbar for an existing scroll owner without registering another group. */
bool cui_base_scrollbar(cui_context *, cui_id, cui_scroll_state *, cui_scroll_geometry,
                        cui_scroll_options, bool vertical);
/* Drawing only; call the base helper before painting the owner. */
void cui_scrollbar(cui_context *, cui_id, const cui_theme *, const cui_scroll_state *,
                   cui_scroll_geometry, cui_scroll_options, bool vertical);
cui_id cui_scrollbar_id(cui_id, bool vertical);
cui_rect cui_scrollbar_thumb(cui_rect track, float scroll, float viewport, float content,
                             bool vertical);
/* Pair end only when begin succeeds. Children are clipped and translated. */
bool cui_scrollable_begin(cui_context *, cui_id, const cui_theme *, cui_scroll_state *,
                          cui_scroll_options);
void cui_scrollable_end(cui_context *, cui_id, const cui_theme *, const cui_scroll_state *,
                        cui_scroll_options);
#ifdef __cplusplus
}
#endif
#endif
