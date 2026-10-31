/* SPDX-License-Identifier: MIT */
#include "cui/cui_stepper_presentation.h"
#include <math.h>
#include <stdio.h>

typedef struct {
    cui_id trigger;
    cui_color normal, hover, pressed;
    float radius;
    bool enabled, captured;
} cui_stepper_indicator_paint;
static bool cui_stepper_indicator_prepaint(const cui_prepaint_info *info, const void *data,
                                           cui_prepaint_writer *writer) {
    const cui_stepper_indicator_paint *p = data;
    bool eligible = p->enabled && info->modal_allowed && !info->inert;
    bool hover = eligible && info->pointer_visible;
    bool pressed = eligible && p->captured && (info->capture != p->trigger || hover);
    cui_draw_command c = {.type = CUI_DRAW_RECT,
                          .bounds = info->bounds,
                          .color = pressed ? p->pressed
                                   : hover ? p->hover
                                           : p->normal,
                          .radius = p->radius};
    return cui_prepaint_emit(writer, &c, false);
}

cui_stepper_navigation_result cui_stepper_present(cui_context *ui, cui_id id, const cui_theme *t,
                                                  const cui_step *steps, size_t count,
                                                  size_t selected, cui_stepper_visual_options o,
                                                  cui_stepper_presentation_options p,
                                                  cui_stepper_navigation_handler handler) {
    if (p.layout < CUI_STEPPER_LAYOUT_CONTENT || p.layout > CUI_STEPPER_LAYOUT_FILL ||
        !isfinite(p.width) || !isfinite(p.height) || p.width < 0 || p.height < 0 ||
        (p.layout == CUI_STEPPER_LAYOUT_CONTENT && (p.width != 0 || p.height != 0)))
        return (cui_stepper_navigation_result){0};
    if (!t || o.density < CUI_STEPPER_DENSITY_MD || o.density > CUI_STEPPER_DENSITY_LG ||
        count > 100 || (o.icons ? o.icon_count != count : o.icon_count != 0))
        return (cui_stepper_navigation_result){0};
    cui_design_metrics design = {0};
    if (o.design_size < CUI_DESIGN_INHERIT || o.design_size > CUI_DESIGN_9XL ||
        (o.design_size != CUI_DESIGN_INHERIT &&
         !cui_design_measure(o.design_size, t->font_size / 16, &design)))
        return (cui_stepper_navigation_result){0};
    for (size_t i = 0; o.icons && i < count; ++i)
        if (o.icons[i].present && !cui_bitmap_valid(o.icons[i].bitmap))
            return (cui_stepper_navigation_result){0};
    cui_stepper_navigation_result result =
        cui_base_stepper_navigation(ui, id, steps, count, selected, o.disabled, handler);
    if (!result.valid)
        return result;
    float side = o.density == CUI_STEPPER_DENSITY_XS   ? 8
                 : o.density == CUI_STEPPER_DENSITY_SM ? 18
                 : o.density == CUI_STEPPER_DENSITY_LG ? 32
                                                       : 24;
    float thickness = o.density == CUI_STEPPER_DENSITY_XS   ? 1.5f
                      : o.density == CUI_STEPPER_DENSITY_LG ? 3
                                                            : 2;
    float scale = o.design_size == CUI_DESIGN_INHERIT ? 1 : design.indicator_size / 16;
    float gap = o.design_size == CUI_DESIGN_INHERIT ? 4 : design.gap;
    float font = o.design_size == CUI_DESIGN_INHERIT ? t->font_size : design.font_size;
    side *= scale;
    thickness *= scale;
    bool fill = p.layout == CUI_STEPPER_LAYOUT_FILL;
    cui_stepper_indicator_palette colors =
        p.custom_palette
            ? p.palette
            : (cui_stepper_indicator_palette){t->secondary, t->secondary_hover,
                                              t->buttons[CUI_THEME_BUTTON_SECONDARY].active,
                                              t->secondary_foreground};
    cui_begin_box(ui, id,
                  (cui_box){.row = !o.vertical,
                            .gap = fill ? 0 : gap,
                            .width = fill ? (p.width > 0 ? p.width : -1) : 0,
                            .height = fill ? p.height : 0,
                            .align_y = fill && !o.vertical ? CUI_ALIGN_CENTER : CUI_ALIGN_START});
    for (size_t i = 0; i < count; ++i) {
        bool checked = i <= selected;
        cui_color foreground = checked ? t->on_primary : colors.foreground;
        cui_id trigger = cui_numeric_part_id(id, 1 + (unsigned)i);
        if (fill)
            cui_begin_box(
                ui, cui_numeric_part_id(id, 2000 + (unsigned)i),
                (cui_box){.width = !o.vertical ? (i + 1 < count ? -1 : 0) : -1,
                          .height = o.vertical && p.height > 0 && i + 1 < count ? -1 : 0});
        if (fill && i + 1 < count) {
            cui_set_underlay_ex(ui,
                                (cui_underlay_layout){
                                    .bounds = o.vertical ? (cui_rect){side / 2, side + 4 * scale,
                                                                      thickness, -side - 8 * scale}
                                                         : (cui_rect){side + 4 * scale, side / 2,
                                                                      -side - 8 * scale, thickness},
                                    .grow_width = !o.vertical,
                                    .grow_height = o.vertical},
                                i < selected ? t->primary : t->border, (cui_corner_radii){0});
        }
        cui_begin_box(ui, trigger, (cui_box){.row = o.vertical, .gap = o.vertical ? gap * 2 : gap});
        float radius = t->radius == 0 ? 0 : side / 2;
        cui_begin_box(ui, cui_numeric_part_id(id, 1000 + (unsigned)i),
                      (cui_box){.width = side,
                                .height = side,
                                .radius = radius,
                                .background = {0},
                                .clip = true,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_stepper_indicator_paint paint = {.trigger = trigger,
                                             .normal = checked ? t->primary : colors.normal,
                                             .hover = checked ? t->primary : colors.hover,
                                             .pressed = checked ? t->primary : colors.pressed,
                                             .radius = radius,
                                             .enabled = !o.disabled && !steps[i].disabled,
                                             .captured =
                                                 cui_interaction_state(ui, trigger).pressed};
        cui_set_prepaint(ui, (cui_prepaint_options){.callback = cui_stepper_indicator_prepaint,
                                                    .payload = &paint,
                                                    .payload_bytes = sizeof paint,
                                                    .command_budget = 3,
                                                    .hover_hitbox = true});
        if (o.density != CUI_STEPPER_DENSITY_XS) {
            if (o.icons && o.icons[i].present) {
                cui_paint_bitmap(ui, 0,
                                 (cui_bitmap_paint){.bitmap = o.icons[i].bitmap,
                                                    .opacity = 1,
                                                    .custom_tint = true,
                                                    .tint = foreground},
                                 (cui_box){.width = side, .height = side, .radius = radius});
            } else {
                char ordinal[4];
                (void)snprintf(ordinal, sizeof(ordinal), "%u", (unsigned)i + 1);
                cui_text(ui, cui_string(ordinal), font * .8f, foreground);
            }
        }
        cui_end_box(ui);
        cui_begin_box(ui, 0, (cui_box){.gap = gap});
        cui_text(ui, steps[i].title, font, o.disabled || steps[i].disabled ? t->muted : t->text);
        if (steps[i].description.length)
            cui_text(ui, steps[i].description, font * .8f, t->muted);
        if (steps[i].status == CUI_STEP_ERROR)
            cui_text(ui, cui_string("Error"), font * .8f, t->danger);
        cui_end_box(ui);
        cui_end_box(ui);
        if (fill)
            cui_end_box(ui);
        if (!fill && i + 1 < count) {
            cui_begin_box(ui, 0,
                          (cui_box){.width = o.vertical ? side : 36 * scale,
                                    .height = o.vertical ? 20 * scale : side,
                                    .align_x = CUI_ALIGN_CENTER,
                                    .align_y = CUI_ALIGN_CENTER});
            cui_begin_box(ui, 0,
                          (cui_box){.width = o.vertical ? thickness : 36 * scale,
                                    .height = o.vertical ? 20 * scale : thickness,
                                    .background = i < selected ? t->primary : t->border});
            cui_end_box(ui);
            cui_end_box(ui);
        }
    }
    cui_end_box(ui);
    return result;
}
