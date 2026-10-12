/* SPDX-License-Identifier: MIT */
#include "cui/cui_scroll.h"
#include <math.h>
static float cui_scroll_component_positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
bool cui_scrollable_begin(cui_context *ui, cui_id id, const cui_theme *t, cui_scroll_state *s,
                          cui_scroll_options o) {
    if (!ui || !id || !t || !s)
        return false;
    cui_scroll_geometry g = cui_scroll_layout(o);
    cui_base_scrollable(ui, id, s, o);
    cui_begin_box(ui, id,
                  (cui_box){.width = cui_scroll_component_positive(o.width, 320),
                            .height = cui_scroll_component_positive(o.height, 240),
                            .background = t->surface});
    cui_begin_box(ui, 0, (cui_box){.width = -1, .height = g.viewport.height, .row = true});
    cui_begin_box(ui, id ^ UINT64_C(0x75b21b4d3cba0f89),
                  (cui_box){.width = g.viewport.width,
                            .height = g.viewport.height,
                            .clip = true,
                            .offset_x = -s->x,
                            .offset_y = -s->y});
    cui_begin_box(
        ui, 0,
        (cui_box){.width = fmaxf(g.viewport.width,
                                 cui_scroll_component_positive(o.content_width, g.viewport.width)),
                  .height = fmaxf(g.viewport.height, cui_scroll_component_positive(
                                                         o.content_height, g.viewport.height)),
                  .row = o.content_row,
                  .gap = isfinite(o.gap) && o.gap > 0 ? o.gap : 0});
    return true;
}
static void cui_scroll_component_bar(cui_context *ui, cui_id id, const cui_theme *t,
                                     const cui_scroll_state *s, cui_scroll_geometry g,
                                     cui_scroll_options o, bool vertical) {
    float width = cui_scroll_component_positive(o.width, 320);
    float height = cui_scroll_component_positive(o.height, 240);
    float bar = vertical ? width - g.viewport.width : height - g.viewport.height;
    float length = vertical ? g.viewport.height : g.viewport.width;
    float value = vertical ? s->y : s->x;
    float maximum = vertical ? g.maximum_y : g.maximum_x;
    cui_rect thumb =
        cui_scrollbar_thumb((cui_rect){0, 0, vertical ? bar : length, vertical ? length : bar},
                            value, length, length + maximum, vertical);
    float start = vertical ? thumb.y : thumb.x;
    cui_id bar_id = cui_scrollbar_id(id, vertical);
    cui_begin_box(ui, bar_id,
                  (cui_box){.width = vertical ? bar : length,
                            .height = vertical ? length : bar,
                            .row = !vertical,
                            .background = t->border});
    if (start > 0) {
        cui_begin_box(ui, 0,
                      (cui_box){.width = vertical ? bar : start, .height = vertical ? start : bar});
        cui_end_box(ui);
    }
    cui_begin_box(ui, 0,
                  (cui_box){.width = thumb.w,
                            .height = thumb.h,
                            .background = o.disabled || maximum <= 0  ? t->border
                                          : cui_focused(ui) == bar_id ? t->focus
                                                                      : t->muted});
    cui_end_box(ui);
    cui_end_box(ui);
}
void cui_scrollable_end(cui_context *ui, cui_id id, const cui_theme *t, const cui_scroll_state *s,
                        cui_scroll_options o) {
    cui_scroll_geometry g = cui_scroll_layout(o);
    cui_end_box(ui); /* content */
    cui_end_box(ui); /* viewport */
    if (o.vertical)
        cui_scroll_component_bar(ui, id, t, s, g, o, true);
    cui_end_box(ui); /* body row */
    if (o.horizontal)
        cui_scroll_component_bar(ui, id, t, s, g, o, false);
    cui_end_box(ui); /* root */
}
