/* SPDX-License-Identifier: MIT */
#ifndef CUI_SLIDER_MODEL_PRESENTATION_INTERNAL_H
#define CUI_SLIDER_MODEL_PRESENTATION_INTERNAL_H
#include "../base/cui_base_slider_model_internal.h"
#include "cui/cui_slider_style.h"
/* Geometry seam only; caller validates/snapshots a resolver plan and appearance
 * before reduction. No renderer conversion/support is implied. Decoration is
 * called once inside each visible outer thumb with current fractional geometry;
 * it may compose paint only, never dispatch/re-register semantics. All borrowed
 * data remains immutable through the Base call. Disabled retains transparent
 * semantic outer boxes and omits decoration entirely. */
typedef void (*cui_slider_source_decoration_internal)(cui_context *, cui_id, uint32_t,
                                                      const cui_slider_style_plan_v1 *, void *);
typedef struct {
    cui_slider_style_plan_v1 plan;
    cui_box track, fill, outer;
    cui_slider_source_decoration_internal decorate;
    void *user;
} cui_slider_source_presentation_internal;
static inline void cui_slider_source_box(cui_context *ui, cui_id id, cui_box b) {
    cui_begin_box(ui, id, b);
    cui_end_box(ui);
}
static inline cui_box cui_slider_source_root_internal(const cui_slider_style_plan_v1 *p,
                                                      uint32_t axis) {
    bool vertical = axis == CUI_SLIDER_MODEL_VERTICAL, fixed = p->layout == CUI_SLIDER_STYLE_FIXED;
    return (cui_box){.row = vertical,
                     .align_x = CUI_ALIGN_CENTER,
                     .align_y = CUI_ALIGN_CENTER,
                     .width = vertical ? p->cross_extent
                              : fixed  ? p->length
                                       : -1,
                     .height = vertical ? (fixed ? p->length : -1) : p->cross_extent,
                     .fixed_width = vertical || fixed,
                     .fixed_height = !vertical || fixed};
}
static inline void cui_slider_source_thumb(cui_context *ui,
                                           const cui_slider_source_presentation_internal *p,
                                           bool vertical, bool disabled, cui_id id, uint32_t thumb,
                                           double fraction) {
    float rem = p->plan.thumb_extent, track = p->plan.track_extent;
    cui_begin_box(ui, 0,
                  (cui_box){.row = !vertical,
                            .width = vertical ? 0 : -1,
                            .height = vertical ? -1 : 0,
                            .fixed_width = vertical,
                            .fixed_height = !vertical});
    cui_slider_source_box(ui, 0,
                          (cui_box){.width = vertical ? 0 : (float)fraction,
                                    .height = vertical ? (float)(1 - fraction) : 0,
                                    .width_percent = !vertical,
                                    .height_percent = vertical,
                                    .fixed_width = vertical,
                                    .fixed_height = !vertical});
    cui_begin_box(ui, 0,
                  (cui_box){.width = 0, .height = 0, .fixed_width = true, .fixed_height = true});
    cui_rect relative =
        vertical ? (cui_rect){-track + p->plan.cross_offset, -p->plan.axis_offset - rem, rem, rem}
                 : (cui_rect){p->plan.axis_offset, -track + p->plan.cross_offset, rem, rem};
    cui_begin_absolute_box(ui, id, disabled ? (cui_box){0} : p->outer, relative);
    if (!disabled && p->decorate)
        p->decorate(ui, id, thumb, &p->plan, p->user);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
}
static inline void cui_slider_source_compose_internal(cui_context *ui, const cui_id ids[5],
                                                      cui_slider_model_value_v1 value,
                                                      const cui_slider_model_options_v1 *o,
                                                      const cui_base_slider_model_layout_v1 *unused,
                                                      void *user) {
    (void)unused;
    const cui_slider_source_presentation_internal *p = user;
    bool vertical = o->orientation == CUI_SLIDER_MODEL_VERTICAL;
    bool range = value.kind == CUI_SLIDER_MODEL_RANGE;
    bool reverse = (o->flags & CUI_SLIDER_MODEL_REVERSE_FILL_BIT) != 0;
    double start = 0, end = 0;
    if (range)
        (void)cui_slider_model_to_fraction_v1(o->domain, value.start, &start);
    (void)cui_slider_model_to_fraction_v1(o->domain, value.end, &end);
    double a = range ? start : reverse ? end : 0;
    double b = range ? end : reverse ? 1 : end;
    float track = p->plan.track_extent;
    cui_box bar = p->track;
    bar.row = vertical;
    bar.width = vertical ? track : -1;
    bar.height = vertical ? -1 : track;
    bar.fixed_width = vertical;
    bar.fixed_height = !vertical;
    cui_begin_box(ui, ids[CUI_SLIDER_MODEL_TRACK], bar);
    cui_begin_box(ui, 0,
                  (cui_box){.row = !vertical,
                            .width = vertical ? track : -1,
                            .height = vertical ? -1 : track,
                            .fixed_width = vertical,
                            .fixed_height = !vertical});
    double before = vertical ? 1 - b : a;
    cui_slider_source_box(ui, 0,
                          (cui_box){.width = vertical ? track : (float)before,
                                    .height = vertical ? (float)before : track,
                                    .width_percent = !vertical,
                                    .height_percent = vertical,
                                    .fixed_width = vertical,
                                    .fixed_height = !vertical});
    cui_box fill = p->fill;
    fill.width = vertical ? track : (float)(b - a);
    fill.height = vertical ? (float)(b - a) : track;
    fill.width_percent = !vertical;
    fill.height_percent = vertical;
    fill.fixed_width = vertical;
    fill.fixed_height = !vertical;
    cui_slider_source_box(ui, ids[CUI_SLIDER_MODEL_FILL], fill);
    cui_end_box(ui);
    bool disabled = (o->flags & CUI_SLIDER_MODEL_DISABLED_BIT) != 0;
    if (range)
        cui_slider_source_thumb(ui, p, vertical, disabled, ids[CUI_SLIDER_MODEL_START_THUMB],
                                CUI_SLIDER_MODEL_START, start);
    cui_slider_source_thumb(ui, p, vertical, disabled, ids[CUI_SLIDER_MODEL_END_THUMB],
                            CUI_SLIDER_MODEL_END, end);
    cui_end_box(ui);
}

#endif
