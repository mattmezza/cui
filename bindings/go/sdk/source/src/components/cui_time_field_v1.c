/* SPDX-License-Identifier: MIT */
#include "../base/cui_time_field_v1_internal.h"
#include <math.h>
#include <string.h>
static bool cui_tfw_theme(const cui_theme *t) {
    return t && isfinite(t->font_size) && t->font_size > 0 && isfinite(t->padding) &&
           t->padding >= 0 && isfinite(t->radius) && t->radius >= 0;
}
static cui_str cui_tfw_label(cui_time value, const cui_time_field_options_v1 *o,
                             const cui_time_locale_v1 *l, unsigned segment, char *buffer) {
    if (segment == 3)
        return value.hour >= 12 ? l->pm : l->am;
    unsigned n = segment == 0
                     ? (o->text.cycle ? (value.hour % 12 ? value.hour % 12 : 12) : value.hour)
                 : segment == 1 ? value.minute
                                : value.second;
    cui_str a = l->digits[n / 10], b = l->digits[n % 10];
    memcpy(buffer, a.data, a.length);
    memcpy(buffer + a.length, b.data, b.length);
    return (cui_str){buffer, a.length + b.length};
}
static void cui_tfw_paint(cui_context *ui, cui_id id, const cui_theme *theme,
                          const cui_time_field_state_v1 *s, const cui_time *t,
                          const cui_time_field_options_v1 *o, const cui_design_metrics *design,
                          bool ordered) {
    cui_time_locale_v1 ascii;
    cui_time_locale_init_v1(&ascii);
    const cui_time_locale_v1 *l = o->text.locale ? o->text.locale : &ascii;
    float font = design ? design->font_size : theme->font_size;
    float width = 0;
    for (unsigned n = 0; n < 100; n++) {
        char buffer[32];
        cui_str a = l->digits[n / 10], b = l->digits[n % 10];
        memcpy(buffer, a.data, a.length);
        memcpy(buffer + a.length, b.data, b.length);
        cui_size size = cui_measure_text(ui, (cui_str){buffer, a.length + b.length}, font);
        if (size.width > width)
            width = size.width;
    }
    float period = cui_measure_text(ui, l->am, font).width;
    float pm = cui_measure_text(ui, l->pm, font).width;
    if (pm > period)
        period = pm;
    bool invalid = (o->flags & CUI_TIME_FIELD_OWNER_INVALID_V1) ||
                   s->validation != CUI_TIME_TEXT_OK ||
                   cui_time_validate_v1(*t, &o->text) == CUI_TIME_TEXT_OUT_OF_RANGE;
    bool ime = (o->flags & CUI_TIME_FIELD_IME_V1) != 0;
    cui_id composing = 0;
    cui_str preedit = ime ? cui_time_field_preedit_v1(ui, id, &composing) : (cui_str){0};
    bool disabled = (o->flags & CUI_TIME_FIELD_DISABLED_V1) != 0;
    cui_id focus = cui_focused(ui);
    cui_box field = {.row = true,
                     .gap = 4,
                     .padding = theme->padding,
                     .radius = theme->radius,
                     .background = theme->surface,
                     .border = invalid ? theme->danger : theme->border,
                     .border_width = 1};
    bool active = (!design && !ordered) || (!disabled && cui_pointer_scope_allowed(ui, 0));
    float cell_padding = 2;
    if (design) {
        field.height = design->control_height;
        field.fixed_height = true;
        field.align_y = CUI_ALIGN_CENTER;
        field.gap = design->gap * .5f;
        field.custom_padding = true;
        field.padding_edges =
            (cui_border_edges){.left = design->padding_x, .right = design->padding_x};
        field.custom_opacity = disabled;
        field.opacity = .5f;
        field.background = theme->input_background;
        field.border = invalid ? theme->danger : theme->input;
        cell_padding = fminf(6, fmaxf(2, field.gap));
    }
    cui_begin_box(ui, id, field);
    for (unsigned i = 0; i < 4; i++) {
        if ((i == 2 && !o->text.precision) || (i == 3 && !o->text.cycle))
            continue;
        if (i == 3)
            cui_text(ui, l->period_separator, font, theme->muted);
        cui_id child = cui_time_segment_id(id, i);
        bool focused = (ordered ? focus == id && s->segment == i : focus == child) && active;
        if (design && focused && !theme->hide_focus_ring) {
            float r = theme->radius;
            (void)cui_set_outline(ui, 2, theme->focus, (cui_corner_radii){r, r, r, r});
        }
        bool show_preedit = child == composing && preedit.length;
        float cell_width = i == 3 ? period : width;
        cui_box cell = {.width = cell_width + 2 * cell_padding,
                        .padding = 2,
                        .radius = theme->radius,
                        .clip = show_preedit,
                        .background = focused ? theme->primary : theme->surface};
        if (design) {
            cell.align_x = CUI_ALIGN_CENTER;
            cell.align_y = CUI_ALIGN_CENTER;
            cell.height = design->line_height;
            cell.fixed_height = true;
            cell.radius *= .5f;
            cell.custom_padding = true;
            cell.padding_edges = (cui_border_edges){.left = cell_padding, .right = cell_padding};
            cell.background = focused ? theme->selection : (cui_color){0};
        }
        cui_begin_box(ui, child, cell);
        char buffer[32];
        cui_str label = show_preedit ? preedit : cui_tfw_label(*t, o, l, i, buffer);
        cui_color foreground = disabled             ? theme->muted
                               : focused && !design ? theme->on_primary
                                                    : theme->text;
        if (show_preedit)
            cui_text_line(ui, label, font, foreground, 0, (cui_text_style){0});
        else
            cui_text(ui, label, font, foreground);
        if (ime && focused &&
            !(o->flags & (CUI_TIME_FIELD_DISABLED_V1 | CUI_TIME_FIELD_READONLY_V1)) &&
            cui_frame_sequence(ui) && cui_pointer_scope_allowed(ui, 0)) {
            /* A private anchor leaves the public cell available to application bounds tracking. */
            cui_id anchor = child ^ UINT64_C(0xc678858a17bd9932);
            float x = cell_padding, y = 2, measured_width = 0;
            if (design) {
                cui_size measured = cui_measure_text(ui, label, font);
                measured_width = measured.width;
                x += fmaxf(0, (cell_width - measured.width) * .5f);
                y = fmaxf(0, (cell.height - measured.height) * .5f);
            } else if (show_preedit)
                measured_width = cui_measure_text(ui, label, font).width;
            if (show_preedit)
                x = fminf(x + measured_width, cell_padding + cell_width);
            cui_begin_absolute_box(ui, anchor, (cui_box){0}, (cui_rect){x, y, 1, font});
            cui_end_box(ui);
            (void)cui_set_text_input_geometry_rect(ui, child, anchor, font);
        }
        cui_end_box(ui);
        if (i == 0 || (i == 1 && o->text.precision))
            cui_text(ui, l->separator, font, theme->muted);
    }
    cui_end_box(ui);
}
cui_time_field_result_v1 cui_time_field_with_size_v1(cui_context *ui, cui_id id,
                                                     const cui_theme *theme, cui_str name,
                                                     cui_time_field_state_v1 *s, cui_time *t,
                                                     const cui_time_field_options_v1 *o,
                                                     cui_design_size size,
                                                     cui_time_handler handler) {
    cui_time_field_result_v1 r = {.status = CUI_TIME_TEXT_INVALID_ARGUMENT};
    bool scaled = size != CUI_DESIGN_INHERIT;
    cui_design_metrics design;
    if (!cui_tfw_theme(theme) ||
        (scaled && !cui_design_measure(size, theme->font_size / 16, &design)))
        return r;
    cui_time before = t ? *t : (cui_time){0};
    r = cui_time_field_v1_compose(ui, id, name, s, t, o);
    if (r.status == CUI_TIME_TEXT_OK) {
        cui_tfw_paint(ui, id, theme, s, t, o, scaled ? &design : NULL, false);
        if (cui_last_error(ui) != CUI_OK)
            r.status = CUI_TIME_TEXT_INVALID_ARGUMENT;
        cui_time_field_v1_observe(ui, id, before, t, o, handler, &r);
    }
    return r;
}
cui_time_field_result_v1 cui_time_field_v1(cui_context *ui, cui_id id, const cui_theme *theme,
                                           cui_str name, cui_time_field_state_v1 *s, cui_time *t,
                                           const cui_time_field_options_v1 *o,
                                           cui_time_handler handler) {
    return cui_time_field_with_size_v1(ui, id, theme, name, s, t, o, CUI_DESIGN_INHERIT, handler);
}
cui_time_field_result_v1 cui_time_field_from_frame_v1(cui_context *ui, cui_id id,
                                                      const cui_theme *theme, cui_str name,
                                                      cui_design_size size,
                                                      cui_time_handler handler) {
    cui_time_field_result_v1 r = {.status = CUI_TIME_TEXT_INVALID_ARGUMENT};
    cui_design_metrics design;
    bool scaled = size != CUI_DESIGN_INHERIT;
    if (!cui_tfw_theme(theme) ||
        (scaled && !cui_design_measure(size, theme->font_size / 16, &design)))
        return r;
    cui_tfw_frame_field *field = cui_tfw_frame_start(ui, id, name);
    if (!field || cui_last_error(ui) != CUI_OK)
        return r;
    cui_tfw_paint(ui, id, theme, &field->state, &field->value, field->entry.options,
                  scaled ? &design : NULL, true);
    return cui_tfw_frame_apply(ui, field, handler);
}
