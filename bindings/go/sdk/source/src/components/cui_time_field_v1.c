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
                          const cui_time_field_options_v1 *o) {
    cui_time_locale_v1 ascii;
    cui_time_locale_init_v1(&ascii);
    const cui_time_locale_v1 *l = o->text.locale ? o->text.locale : &ascii;
    float width = 0;
    for (unsigned n = 0; n < 100; n++) {
        char buffer[32];
        cui_str a = l->digits[n / 10], b = l->digits[n % 10];
        memcpy(buffer, a.data, a.length);
        memcpy(buffer + a.length, b.data, b.length);
        cui_size size =
            cui_measure_text(ui, (cui_str){buffer, a.length + b.length}, theme->font_size);
        if (size.w > width)
            width = size.w;
    }
    float period = cui_measure_text(ui, l->am, theme->font_size).w;
    float pm = cui_measure_text(ui, l->pm, theme->font_size).w;
    if (pm > period)
        period = pm;
    bool invalid = (o->flags & CUI_TIME_FIELD_OWNER_INVALID_V1) ||
                   s->validation != CUI_TIME_TEXT_OK ||
                   cui_time_validate_v1(*t, &o->text) == CUI_TIME_TEXT_OUT_OF_RANGE;
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .gap = 4,
                            .padding = theme->padding,
                            .radius = theme->radius,
                            .background = theme->surface,
                            .border = invalid ? theme->danger : theme->border,
                            .border_width = 1});
    for (unsigned i = 0; i < 4; i++) {
        if ((i == 2 && !o->text.precision) || (i == 3 && !o->text.cycle))
            continue;
        if (i == 3)
            cui_text(ui, l->period_separator, theme->font_size, theme->muted);
        cui_id child = cui_time_segment_id(id, i);
        bool focused = cui_focused(ui) == child;
        cui_begin_box(ui, child,
                      (cui_box){.width = (i == 3 ? period : width) + 4,
                                .padding = 2,
                                .radius = theme->radius,
                                .background = focused ? theme->primary : theme->surface});
        char buffer[32];
        cui_str label = cui_tfw_label(*t, o, l, i, buffer);
        cui_text(ui, label, theme->font_size,
                 (o->flags & CUI_TIME_FIELD_DISABLED_V1) ? theme->muted
                 : focused                               ? theme->on_primary
                                                         : theme->text);
        cui_end_box(ui);
        if (i == 0 || (i == 1 && o->text.precision))
            cui_text(ui, l->separator, theme->font_size, theme->muted);
    }
    cui_end_box(ui);
}
cui_time_field_result_v1 cui_time_field_v1(cui_context *ui, cui_id id, const cui_theme *theme,
                                           cui_str name, cui_time_field_state_v1 *s, cui_time *t,
                                           const cui_time_field_options_v1 *o,
                                           cui_time_handler handler) {
    cui_time_field_result_v1 r = {.status = CUI_TIME_TEXT_INVALID_ARGUMENT};
    if (!cui_tfw_theme(theme))
        return r;
    cui_time before = t ? *t : (cui_time){0};
    r = cui_time_field_v1_compose(ui, id, name, s, t, o);
    if (r.status == CUI_TIME_TEXT_OK) {
        cui_tfw_paint(ui, id, theme, s, t, o);
        if (cui_last_error(ui) != CUI_OK)
            r.status = CUI_TIME_TEXT_INVALID_ARGUMENT;
        cui_time_field_v1_observe(ui, id, before, t, o, handler, &r);
    }
    return r;
}
