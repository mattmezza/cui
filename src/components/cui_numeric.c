/* SPDX-License-Identifier: MIT */
#include "cui/cui_numeric.h"
#include <math.h>
#include <stdio.h>
static void cui_numeric_paint_button(cui_context *ui, cui_id id, const cui_theme *t, cui_str text,
                                     bool disabled) {
    cui_begin_box(ui, id,
                  (cui_box){.height = t->control_height,
                            .padding = t->padding,
                            .radius = t->radius,
                            .background = disabled ? t->surface : t->primary,
                            .border = cui_focused(ui) == id ? t->focus : t->border,
                            .border_width = 1});
    cui_text(ui, text, t->font_size, disabled ? t->muted : t->on_primary);
    cui_end_box(ui);
}
cui_number_result cui_number_input(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                                   cui_number_state *s, double *value, cui_number_options o) {
    if (!t)
        return (cui_number_result){.invalid = true, .error = CUI_EDIT_INVALID_ARGUMENT};
    cui_number_result result = cui_base_number_input(ui, id, name, s, value, o);
    if (!s || !s->edit || !value || (result.error == CUI_EDIT_INVALID_ARGUMENT && !s->initialized))
        return result;
    bool editable = !o.disabled && !o.readonly;
    float width = isfinite(o.width) && o.width > 0 ? o.width : 200;
    cui_begin_box(ui, cui_numeric_part_id(id, 0), (cui_box){.gap = t->gap, .row = true});
    if (o.prefix.length)
        cui_text(ui, o.prefix, t->font_size, t->text);
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .padding = t->padding,
                            .gap = t->gap * .5f,
                            .radius = t->radius,
                            .background = t->surface,
                            .border = s->invalid              ? t->danger
                                      : cui_focused(ui) == id ? t->focus
                                                              : t->border,
                            .border_width = 1});
    cui_text(ui, name, t->font_size * .8f, t->muted);
    cui_text(ui, cui_edit_get_text(s->edit), t->font_size, o.disabled ? t->muted : t->text);
    cui_end_box(ui);
    cui_numeric_paint_button(ui, cui_numeric_part_id(id, 1), t, cui_string("-"),
                             !editable || (o.has_minimum && *value <= o.minimum));
    cui_numeric_paint_button(ui, cui_numeric_part_id(id, 2), t, cui_string("+"),
                             !editable || (o.has_maximum && *value >= o.maximum));
    if (o.suffix.length)
        cui_text(ui, o.suffix, t->font_size, t->text);
    cui_end_box(ui);
    return result;
}
bool cui_pagination(cui_context *ui, cui_id id, const cui_theme *t, size_t *page, size_t total,
                    bool compact, bool disabled) {
    if (!t || !page)
        return false;
    bool changed = cui_base_pagination(ui, id, page, total, compact, disabled);
    cui_begin_box(ui, id, (cui_box){.row = true, .gap = t->gap});
    cui_numeric_paint_button(ui, cui_numeric_part_id(id, 1), t, cui_string("First"),
                             disabled || *page <= 1);
    cui_numeric_paint_button(ui, cui_numeric_part_id(id, 2), t, cui_string("Previous"),
                             disabled || *page <= 1);
    if (compact) {
        char label[64];
        snprintf(label, sizeof(label), "%zu / %zu", *page, total);
        cui_text(ui, cui_string(label), t->font_size, t->text);
    } else {
        cui_page_item items[11];
        size_t n = cui_pagination_items(*page, total, 2, items);
        for (size_t i = 0; i < n; ++i) {
            if (items[i].ellipsis)
                cui_text(ui, cui_string("..."), t->font_size, t->muted);
            else {
                char label[32];
                snprintf(label, sizeof(label), "%zu", items[i].page);
                cui_numeric_paint_button(ui, cui_numeric_part_id(id, 10 + (unsigned)i), t,
                                         cui_string(label), disabled || items[i].page == *page);
            }
        }
    }
    cui_numeric_paint_button(ui, cui_numeric_part_id(id, 3), t, cui_string("Next"),
                             disabled || !total || *page >= total);
    cui_numeric_paint_button(ui, cui_numeric_part_id(id, 4), t, cui_string("Last"),
                             disabled || !total || *page >= total);
    cui_end_box(ui);
    return changed;
}
bool cui_rating(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, float *value,
                cui_rating_options o) {
    if (!t || !value)
        return false;
    bool changed = cui_base_rating(ui, id, name, value, o);
    unsigned maximum = o.maximum ? o.maximum : 5;
    if (maximum > 100)
        maximum = 100;
    float width = isfinite(o.width) && o.width > 0 ? o.width : maximum * 36;
    float slot = fmaxf(1, (width - t->padding * 2 - t->gap * (maximum - 1)) / maximum);
    cui_begin_box(ui, cui_numeric_part_id(id, 0), (cui_box){.row = true, .gap = t->gap});
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .width = width,
                            .padding = t->padding,
                            .gap = t->gap,
                            .radius = t->radius,
                            .background = t->surface});
    for (unsigned i = 0; i < maximum; ++i) {
        cui_begin_box(ui, cui_numeric_part_id(id, 10 + i),
                      (cui_box){.width = slot, .height = t->control_height});
        const char *star = *value >= (float)i + 1 ? "★" : *value > (float)i ? "◐" : "☆";
        cui_text(ui, cui_string(star), t->font_size * 1.3f, o.disabled ? t->muted : t->primary);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (o.clearable)
        cui_numeric_paint_button(ui, cui_numeric_part_id(id, 1), t, cui_string("Clear"),
                                 o.disabled || o.readonly);
    cui_end_box(ui);
    return changed;
}
bool cui_stepper(cui_context *ui, cui_id id, const cui_theme *t, const cui_step *steps,
                 size_t count, size_t *current, bool vertical, bool disabled) {
    if (!t || !current || (!steps && count) || count > 100)
        return false;
    bool changed = cui_base_stepper(ui, id, steps, count, current, disabled);
    cui_begin_box(ui, id, (cui_box){.row = !vertical, .gap = t->gap});
    for (size_t i = 0; i < count; ++i) {
        const char *status = steps[i].status == CUI_STEP_ERROR      ? "Error"
                             : steps[i].status == CUI_STEP_COMPLETE ? "Complete"
                             : steps[i].status == CUI_STEP_ACTIVE   ? "Current"
                                                                    : "Pending";
        cui_begin_box(ui, cui_numeric_part_id(id, 100 + (unsigned)i), (cui_box){.gap = t->gap});
        cui_numeric_paint_button(ui, cui_numeric_part_id(id, 1 + (unsigned)i), t, steps[i].title,
                                 disabled || steps[i].disabled);
        cui_text(ui, cui_string(status), t->font_size * .8f,
                 steps[i].status == CUI_STEP_ERROR ? t->danger : t->muted);
        if (steps[i].description.length)
            cui_text(ui, steps[i].description, t->font_size * .8f, t->muted);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return changed;
}
