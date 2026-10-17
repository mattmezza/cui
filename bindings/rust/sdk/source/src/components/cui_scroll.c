/* SPDX-License-Identifier: MIT */
#include "cui/cui_scroll.h"
#include <math.h>
static float cui_scroll_component_positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
static cui_id cui_scroll_content_id(cui_id id) {
    return id ^ UINT64_C(0x2ebf107b4533ddf1);
}
static bool cui_scroll_auto_axis(cui_scroll_options o, bool vertical) {
    float size = vertical ? o.content_height : o.content_width;
    return o.auto_measure_content && (vertical ? o.vertical : o.horizontal) &&
           (!isfinite(size) || size <= 0);
}
static cui_scroll_options cui_scroll_measured(cui_context *ui, cui_id id, cui_scroll_options o) {
    if (o.auto_measure_content) {
        cui_rect measured = cui_bounds(ui, cui_scroll_content_id(id));
        if (cui_scroll_auto_axis(o, false))
            o.content_width = measured.w;
        if (cui_scroll_auto_axis(o, true))
            o.content_height = measured.h;
    }
    return o;
}
bool cui_scrollable_begin(cui_context *ui, cui_id id, const cui_theme *t, cui_scroll_state *s,
                          cui_scroll_options o) {
    if (!ui || !id || !t || !s)
        return false;
    o = cui_scroll_resolve(t, o);
    cui_scroll_options measured = cui_scroll_measured(ui, id, o);
    cui_scroll_geometry g = cui_scroll_layout(measured);
    cui_base_scrollable(ui, id, s, measured);
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
        ui, o.auto_measure_content ? cui_scroll_content_id(id) : 0,
        (cui_box){
            .width = cui_scroll_auto_axis(o, false)
                         ? 0
                         : fmaxf(g.viewport.width,
                                 cui_scroll_component_positive(o.content_width, g.viewport.width)),
            .height = cui_scroll_auto_axis(o, true)
                          ? 0
                          : fmaxf(g.viewport.height, cui_scroll_component_positive(
                                                         o.content_height, g.viewport.height)),
            .row = o.content_row,
            .gap = isfinite(o.gap) && o.gap > 0 ? o.gap : 0});
    return true;
}
void cui_scrollbar(cui_context *ui, cui_id id, const cui_theme *t, const cui_scroll_state *s,
                   cui_scroll_geometry g, cui_scroll_options o, bool vertical) {
    if (!ui || !id || !t || !s)
        return;
    o = cui_scroll_resolve(t, o);
    if (o.hide_scrollbars)
        return;
    float width = cui_scroll_component_positive(o.width, 320);
    float height = cui_scroll_component_positive(o.height, 240);
    float bar = vertical ? width - g.viewport.width : height - g.viewport.height;
    float length = vertical ? g.viewport.height : g.viewport.width;
    float value = vertical ? s->y : s->x;
    float maximum = vertical ? g.maximum_y : g.maximum_x;
    cui_rect thumb = cui_scrollbar_thumb_ex(
        (cui_rect){0, 0, vertical ? bar : length, vertical ? length : bar}, value, length,
        length + maximum, vertical, o.scrollbar_style ? o.scrollbar_style->min_thumb : 0);
    float start = vertical ? thumb.y : thumb.x;
    cui_id bar_id = cui_scrollbar_id(id, vertical);
    const cui_scrollbar_style *style = o.scrollbar_style;
    float opacity = cui_scrollbar_opacity(ui, id, bar_id, style, value,
                                          vertical ? s->dragging_y : s->dragging_x);
    cui_color track = style && style->custom_track_color ? style->track_color : t->border;
    cui_color fill = style && style->custom_thumb_color ? style->thumb_color
                     : o.disabled || maximum <= 0       ? t->border
                     : cui_focused(ui) == bar_id        ? t->focus
                                                        : t->muted;
    if (style && style->no_track)
        track.a = 0;
    cui_begin_box(ui, bar_id,
                  (cui_box){.width = vertical ? bar : length,
                            .height = vertical ? length : bar,
                            .row = !vertical,
                            .background = track,
                            .radius = style && isfinite(style->track_radius)
                                          ? fmaxf(0, style->track_radius)
                                          : 0,
                            .custom_opacity = true,
                            .opacity = opacity});
    if (start > 0) {
        cui_begin_box(ui, 0,
                      (cui_box){.width = vertical ? bar : start, .height = vertical ? start : bar});
        cui_end_box(ui);
    }
    cui_begin_box(ui, 0,
                  (cui_box){.width = thumb.w,
                            .height = thumb.h,
                            .background = fill,
                            .radius = style && isfinite(style->thumb_radius)
                                          ? fmaxf(0, style->thumb_radius)
                                          : 0});
    cui_end_box(ui);
    cui_end_box(ui);
}
void cui_scrollable_end(cui_context *ui, cui_id id, const cui_theme *t, const cui_scroll_state *s,
                        cui_scroll_options o) {
    o = cui_scroll_measured(ui, id, cui_scroll_resolve(t, o));
    cui_scroll_geometry g = cui_scroll_layout(o);
    cui_end_box(ui); /* content */
    cui_end_box(ui); /* viewport */
    if (o.vertical && !o.hide_scrollbars)
        cui_scrollbar(ui, id, t, s, g, o, true);
    cui_end_box(ui); /* body row */
    if (o.horizontal && !o.hide_scrollbars)
        cui_scrollbar(ui, id, t, s, g, o, false);
    cui_end_box(ui); /* root */
}

void cui_scrollable_end_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_scroll_state *s,
                           cui_scroll_options o) {
    if (!ui || !id || !t || !s)
        return;
    o = cui_scroll_measured(ui, id, cui_scroll_resolve(t, o));
    cui_base_scrollable_finish(ui, id, s, o);
    cui_set_box_scroll_offset(ui, id ^ UINT64_C(0x75b21b4d3cba0f89), -s->x, -s->y);
    cui_scrollable_end(ui, id, t, s, o);
}
void cui_scrollbar_overlay_paint(cui_context *ui, cui_id id, const cui_theme *theme, float x,
                                 float y, cui_rect viewport, cui_size content,
                                 cui_scroll_options o) {
    if (!ui || !id || !theme || !isfinite(x) || !isfinite(y) || !isfinite(viewport.x) ||
        !isfinite(viewport.y) || !isfinite(viewport.w) || !isfinite(viewport.h) ||
        viewport.w <= 0 || viewport.h <= 0 || !isfinite(content.width) ||
        !isfinite(content.height) || content.width < 0 || content.height < 0)
        return;
    o = cui_scroll_resolve(theme, o);
    if (o.hide_scrollbars || !o.scrollbar_style ||
        (o.scrollbar_style->visibility != CUI_SCROLLBAR_VISIBLE &&
         o.scrollbar_style->visibility != CUI_SCROLLBAR_AUTO_HIDE))
        return;
    cui_scroll_state *state =
        cui_resource_get(ui, id, UINT64_C(0x30914be927c650a8), sizeof(*state), NULL, NULL);
    if (!state)
        return;
    float bar = fminf(cui_scroll_component_positive(o.bar_thickness, 10),
                      fminf(viewport.w, viewport.h) * .5f);
    cui_scroll_geometry g = {.viewport = {viewport.w, viewport.h},
                             .maximum_x = o.horizontal ? fmaxf(0, content.width - viewport.w) : 0,
                             .maximum_y = o.vertical ? fmaxf(0, content.height - viewport.h) : 0};
    state->x = fminf(fmaxf(0, x), g.maximum_x);
    state->y = fminf(fmaxf(0, y), g.maximum_y);
    o.width = viewport.w + bar;
    o.height = viewport.h + bar;
    if (o.vertical && g.maximum_y > 0) {
        cui_begin_absolute_box(
            ui, 0, (cui_box){0},
            (cui_rect){viewport.x + viewport.w - bar, viewport.y, bar, viewport.h});
        cui_scrollbar(ui, id, theme, state, g, o, true);
        cui_end_box(ui);
    }
    if (o.horizontal && g.maximum_x > 0) {
        cui_begin_absolute_box(
            ui, 0, (cui_box){0},
            (cui_rect){viewport.x, viewport.y + viewport.h - bar, viewport.w, bar});
        cui_scrollbar(ui, id, theme, state, g, o, false);
        cui_end_box(ui);
    }
}
bool cui_scrollbar_overlay(cui_context *ui, cui_id id, const cui_theme *theme, float *x, float *y,
                           cui_rect viewport, cui_size content, cui_scroll_options o) {
    o = cui_scroll_resolve(theme, o);
    bool changed = cui_base_scrollbar_overlay(ui, id, x, y, viewport, content, o);
    if (x && y)
        cui_scrollbar_overlay_paint(ui, id, theme, *x, *y, viewport, content, o);
    return changed;
}
