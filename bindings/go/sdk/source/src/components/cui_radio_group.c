/* SPDX-License-Identifier: MIT */
#include "cui/cui_radio_group.h"
#include <math.h>
#include <string.h>
static cui_radio_group_result
cui_radio_group_render(cui_context *ui, cui_id id, const cui_theme *t,
                       const cui_radio_group_item *items, size_t count, cui_id *selected,
                       cui_radio_group_state *state, cui_radio_group_options o,
                       cui_selection_handler handler, cui_selection_event_result *observed,
                       const cui_selection_indicator_parts *parts) {
    cui_radio_group_result r = {0};
    if (!t)
        return r;
    float density = t->font_size / 16;
    float size = (o.size == CUI_CONTROL_XSMALL  ? 12
                  : o.size == CUI_CONTROL_SMALL ? 14
                  : o.size == CUI_CONTROL_LARGE ? 18
                                                : 16) *
                 density;
    float font = (o.size == CUI_CONTROL_XSMALL  ? 12
                  : o.size == CUI_CONTROL_LARGE ? 16
                                                : 14) *
                 density;
    if (o.design_size != CUI_DESIGN_INHERIT) {
        cui_design_metrics design;
        if (!cui_design_measure(o.design_size, density, &design))
            return r;
        size = design.indicator_size;
        font = design.font_size;
    }
    if (observed) {
        cui_radio_group_event_result result =
            cui_base_radio_group_events(ui, id, items, count, selected, state, o, handler);
        *observed = result.events;
        r = result.legacy;
    } else
        r = cui_base_radio_group(ui, id, items, count, selected, state, o);
    if (!r.valid)
        return r;
    cui_begin_box(ui, id, (cui_box){.row = !o.vertical, .gap = o.gap > 0 ? o.gap : 12 * density});
    for (size_t i = 0; i < count; i++) {
        bool disabled = o.disabled || items[i].disabled, checked = *selected == items[i].id;
        cui_color ink = o.invalid ? t->danger : checked ? t->primary : t->input;
        cui_color background = checked ? t->primary : t->input_background, dot = t->on_primary;
        if (disabled) {
            ink.a /= 2;
            dot.a /= 2;
            if (checked)
                background.a /= 2;
        }
        cui_box row = {.row = true, .gap = 8 * density, .radius = t->radius * .5f};
        if (!o.hide_focus_ring && cui_focused(ui) == items[i].id) {
            row.border = t->focus;
            row.border_width = 2;
        }
        cui_begin_box(ui, items[i].id, row);
        cui_begin_box(ui, 0, (cui_box){.width = size});
        if (items[i].label.length) {
            cui_begin_box(ui, 0, (cui_box){.height = size * .125f});
            cui_end_box(ui);
        }
        cui_begin_box(ui, parts ? parts[i].indicator : 0,
                      (cui_box){.width = size,
                                .height = size,
                                .radius = size / 2,
                                .border_width = 1,
                                .border = ink,
                                .background = background,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        if (checked)
            cui_checkmark(ui, size - 4 * density, dot);
        cui_end_box(ui);
        cui_end_box(ui);
        if (items[i].label.length)
            cui_text(ui, items[i].label, font, disabled ? t->muted : t->text);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return r;
}

cui_radio_group_result cui_radio_group(cui_context *ui, cui_id id, const cui_theme *theme,
                                       const cui_radio_group_item *items, size_t count,
                                       cui_id *selected, cui_radio_group_state *state,
                                       cui_radio_group_options options) {
    return cui_radio_group_render(ui, id, theme, items, count, selected, state, options,
                                  (cui_selection_handler){0}, NULL, NULL);
}
cui_radio_group_event_result cui_radio_group_events(cui_context *ui, cui_id id,
                                                    const cui_theme *theme,
                                                    const cui_radio_group_item *items, size_t count,
                                                    cui_id *selected, cui_radio_group_state *state,
                                                    cui_radio_group_options options,
                                                    cui_selection_handler handler) {
    cui_radio_group_event_result result;
    memset(&result, 0, sizeof(result));
    result.legacy = cui_radio_group_render(ui, id, theme, items, count, selected, state, options,
                                           handler, &result.events, NULL);
    return result;
}

static bool cui_radio_group_parts_copy(cui_context *ui, cui_id id,
                                       const cui_radio_group_item *items, size_t count,
                                       const cui_selection_indicator_parts *parts,
                                       size_t part_count,
                                       cui_selection_indicator_parts copied[CUI_RADIO_GROUP_MAX]) {
    bool valid = ui && id && count <= CUI_RADIO_GROUP_MAX && part_count == count &&
                 (!count || (items && parts));
    for (size_t i = 0; valid && i < count; i++) {
        valid = items[i].id && items[i].id != id && parts[i].indicator && parts[i].indicator != id;
        for (size_t j = 0; valid && j < count; j++)
            valid = parts[i].indicator != items[j].id;
        for (size_t j = 0; valid && j < i; j++)
            valid = parts[i].indicator != parts[j].indicator && items[i].id != items[j].id;
    }
    if (!valid) {
        if (ui)
            cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_INVALID_ACTION,
                                                        .operation = "radio_group_parts",
                                                        .id = id});
        return false;
    }
    if (count)
        memcpy(copied, parts, count * sizeof(*parts));
    return true;
}
cui_radio_group_result
cui_radio_group_with_parts(cui_context *ui, cui_id id, const cui_theme *theme,
                           const cui_radio_group_item *items, size_t count, cui_id *selected,
                           cui_radio_group_state *state, cui_radio_group_options options,
                           const cui_selection_indicator_parts *parts, size_t part_count) {
    cui_selection_indicator_parts copied[CUI_RADIO_GROUP_MAX];
    if (!cui_radio_group_parts_copy(ui, id, items, count, parts, part_count, copied))
        return (cui_radio_group_result){0};
    return cui_radio_group_render(ui, id, theme, items, count, selected, state, options,
                                  (cui_selection_handler){0}, NULL, copied);
}
cui_radio_group_event_result cui_radio_group_events_with_parts(
    cui_context *ui, cui_id id, const cui_theme *theme, const cui_radio_group_item *items,
    size_t count, cui_id *selected, cui_radio_group_state *state, cui_radio_group_options options,
    const cui_selection_indicator_parts *parts, size_t part_count, cui_selection_handler handler) {
    cui_radio_group_event_result result = {0};
    cui_selection_indicator_parts copied[CUI_RADIO_GROUP_MAX];
    if (!cui_radio_group_parts_copy(ui, id, items, count, parts, part_count, copied))
        return result;
    result.legacy = cui_radio_group_render(ui, id, theme, items, count, selected, state, options,
                                           handler, &result.events, copied);
    return result;
}
