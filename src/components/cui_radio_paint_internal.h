/* SPDX-License-Identifier: MIT */
#ifndef CUI_RADIO_PAINT_INTERNAL_H
#define CUI_RADIO_PAINT_INTERNAL_H
#include "cui/cui_components.h"
#include <math.h>

static cui_radio_paint cui_radio_resolve_paint(cui_context *ui, cui_id id, const cui_theme *theme,
                                               float size, float font, float density, bool checked,
                                               bool disabled, bool invalid, bool hide_focus,
                                               bool animated, float opacity,
                                               const cui_radio_styles *styles) {
    cui_color border = invalid ? theme->danger : checked ? theme->primary : theme->input;
    cui_color fill = theme->primary, mark = theme->on_primary;
    if (disabled) {
        border.a /= 2;
        fill.a /= 2;
        mark.a /= 2;
    }
    cui_color background = animated  ? cui_color_mix_oklab(theme->input_background, fill, opacity)
                           : checked ? fill
                                     : theme->input_background;
    cui_radio_paint paint = {
        .root = {.row = true, .gap = 8 * density, .radius = theme->radius * .5f},
        .indicator = {.width = size,
                      .height = size,
                      .radius = size / 2,
                      .border_width = 1,
                      .border = border,
                      .background = background,
                      .align_x = CUI_ALIGN_CENTER,
                      .align_y = CUI_ALIGN_CENTER},
        .mark = mark,
        .label = {.color = disabled ? theme->muted : theme->text, .font_size = font},
        .description = {.color = theme->muted, .font_size = font * .875f},
    };
    if (!hide_focus && cui_focused(ui) == id) {
        paint.root.border = theme->focus;
        paint.root.border_width = 2;
    }
    if (styles) {
        if (styles->instance)
            styles->instance(ui, styles->user, id, checked, disabled, &paint);
        if (checked && styles->checked)
            styles->checked(ui, styles->user, id, checked, disabled, &paint);
        if (disabled && styles->disabled)
            styles->disabled(ui, styles->user, id, checked, disabled, &paint);
    }
    paint.root.row = true;
    paint.root.layout_only = false;
    paint.root.inert = false;
    paint.indicator.layout_only = false;
    paint.indicator.inert = false;
    return paint;
}

/* Compose the indicator and leave the option root open for text and content. */
static void cui_radio_begin_option(cui_context *ui, cui_id id, cui_id indicator,
                                   const cui_radio_paint *paint, bool has_content, float opacity,
                                   float density) {
    cui_begin_box(ui, id, paint->root);
    cui_begin_box(ui, 0, (cui_box){.width = paint->indicator.width});
    if (has_content) {
        cui_begin_box(ui, 0, (cui_box){.height = paint->indicator.height * .125f});
        cui_end_box(ui);
    }
    cui_begin_box(ui, indicator, paint->indicator);
    if (opacity > 0) {
        cui_color mark = paint->mark;
        mark.a = (uint8_t)(mark.a * opacity);
        float size = fminf(paint->indicator.width, paint->indicator.height);
        cui_checkmark(ui, fmaxf(0, size - 4 * density), mark);
    }
    cui_end_box(ui);
    cui_end_box(ui);
}

static void cui_radio_paint_text(cui_context *ui, cui_str text, cui_radio_text_paint paint) {
    cui_text_ex(ui, text, paint.font_size, paint.color, paint.line_height, paint.style);
}
#endif
