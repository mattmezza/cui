/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_PAINT_INTERNAL_H
#define CUI_TEXT_PAINT_INTERNAL_H
#include "cui/cui.h"
#include <math.h>
static inline bool cui_text_paint_valid(cui_text_paint p, bool resolved) {
    if (p.kind == CUI_TEXT_PAINT_SOLID)
        return p.stop_count == 0;
    if (p.kind != CUI_TEXT_PAINT_LINEAR || !p.stops || p.stop_count < 2 || p.stop_count > 8 ||
        p.anchor < CUI_TEXT_ANCHOR_BLOCK || p.anchor > CUI_TEXT_ANCHOR_LINE || !isfinite(p.x0) ||
        !isfinite(p.y0) || !isfinite(p.x1) || !isfinite(p.y1))
        return false;
    if (!resolved && (p.x0 < 0 || p.x0 > 1 || p.y0 < 0 || p.y0 > 1 || p.x1 < 0 || p.x1 > 1 ||
                      p.y1 < 0 || p.y1 > 1 || (p.x0 == p.x1 && p.y0 == p.y1)))
        return false;
    float previous = 0;
    for (size_t i = 0; i < p.stop_count; ++i) {
        float at = p.stops[i].position;
        if (!isfinite(at) || at < previous || at > 1)
            return false;
        previous = at;
    }
    return true;
}
static inline cui_color cui_text_paint_sample(cui_text_paint p, double x, double y) {
    if (p.kind == CUI_TEXT_PAINT_SOLID)
        return p.color;
    double dx = (double)p.x1 - p.x0, dy = (double)p.y1 - p.y0;
    double length = dx * dx + dy * dy;
    double t = length > 0 ? ((x - p.x0) * dx + (y - p.y0) * dy) / length : 1;
    size_t right = 0;
    while (right < p.stop_count && p.stops[right].position <= t)
        ++right;
    if (!right)
        return p.stops[0].color;
    if (right == p.stop_count)
        return p.stops[right - 1].color;
    cui_text_gradient_stop a = p.stops[right - 1], b = p.stops[right];
    double f = (t - a.position) / (b.position - a.position);
    double alpha = a.color.a + (b.color.a - a.color.a) * f;
    if (alpha <= 0)
        return (cui_color){0};
    cui_color result;
#define CUI_PAINT_CHANNEL(ch)                                                                      \
    (uint8_t)lround(((1 - f) * a.color.ch * a.color.a + f * b.color.ch * b.color.a) / alpha)
    result.r = CUI_PAINT_CHANNEL(r);
    result.g = CUI_PAINT_CHANNEL(g);
    result.b = CUI_PAINT_CHANNEL(b);
#undef CUI_PAINT_CHANNEL
    result.a = (uint8_t)lround(alpha);
    return result;
}
static inline bool cui_text_tracking_supported(const cui_draw_list *list, bool supported) {
    for (size_t i = 0; i < list->count; ++i) {
        const cui_draw_command *c = &list->commands[i];
        if (c->type == CUI_DRAW_TEXT && (!isfinite(c->text_style.letter_spacing) ||
                                         (c->text_style.letter_spacing != 0 && !supported)))
            return false;
    }
    return true;
}
static inline bool cui_text_paints_supported(const cui_draw_list *list, bool linear) {
    for (size_t i = 0; i < list->count; ++i) {
        const cui_draw_command *c = &list->commands[i];
        if (c->text_paint.kind == CUI_TEXT_PAINT_SOLID) {
            if (c->text_paint.stop_count)
                return false;
            continue;
        }
        if (!linear || c->type != CUI_DRAW_TEXT || c->text_color_count ||
            !cui_text_paint_valid(c->text_paint, true))
            return false;
    }
    return true;
}
#endif
