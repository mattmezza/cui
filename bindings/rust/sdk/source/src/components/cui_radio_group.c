/* SPDX-License-Identifier: MIT */
#include "cui/cui_radio_group.h"
#include "cui_radio_paint_internal.h"
#include "cui_radio_tooltip_internal.h"
#include <math.h>
#include <string.h>
static void cui_radio_group_describe(cui_context *ui, size_t first, cui_id id, cui_str text) {
    size_t count;
    const cui_semantic *items = cui_semantics(ui, &count);
    for (size_t i = first; i < count; i++)
        if (items[i].id == id) {
            cui_semantic_details details = items[i].details;
            details.description = text;
            cui_set_semantic_details(ui, id, details);
            return;
        }
}

static cui_radio_group_result cui_radio_group_render(
    cui_context *ui, cui_id id, const cui_theme *t, const cui_radio_group_item *items, size_t count,
    cui_id *selected, cui_radio_group_state *state, cui_radio_group_options o,
    cui_selection_handler handler, cui_selection_event_result *observed,
    cui_radio_group_presentation presentation, const cui_radio_group_content *content,
    const cui_radio_group_styles *styles) {
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
    size_t first_semantic = 0;
    if (content)
        cui_semantics(ui, &first_semantic);
    if (observed) {
        cui_radio_group_event_result result =
            cui_base_radio_group_events(ui, id, items, count, selected, state, o, handler);
        *observed = result.events;
        r = result.legacy;
    } else
        r = cui_base_radio_group(ui, id, items, count, selected, state, o);
    if (!r.valid)
        return r;
    if (presentation.custom_tab_index)
        for (size_t i = 0; i < count; i++)
            cui_set_tab_index(ui, items[i].id, presentation.tab_index);
    if (content) {
        for (size_t i = 0; i < count; i++)
            if (content[i].description.length)
                cui_radio_group_describe(ui, first_semantic, items[i].id, content[i].description);
    }
    cui_radio_group_motion *motion = presentation.motion;
    if (motion) {
        cui_disclosure_motion samples[CUI_RADIO_GROUP_MAX] = {0};
        if (motion->id == id)
            for (size_t i = 0; i < count; i++)
                for (size_t j = 0; j < CUI_RADIO_GROUP_MAX; j++)
                    if (motion->items[j].id == items[i].id) {
                        samples[i] = motion->items[j];
                        break;
                    }
        motion->id = id;
        motion->next_wake_seconds = 0;
        bool active = false;
        for (size_t i = 0; i < count; i++) {
            bool checked = *selected == items[i].id;
            samples[i].id = items[i].id;
            cui_disclosure_motion_step_ex(&samples[i], checked, cui_frame_delta(ui),
                                          t->spring_control, cui_reduced_motion(ui));
            if (!isfinite(samples[i].progress) || !isfinite(samples[i].velocity)) {
                samples[i].progress = checked ? 1 : 0;
                samples[i].velocity = 0;
            }
            active |= samples[i].progress != (checked ? 1 : 0) || samples[i].velocity != 0;
        }
        memcpy(motion->items, samples, sizeof(samples));
        double now = cui_frame_time(ui);
        motion->next_wake_seconds = active && isfinite(now) && now <= 1e12 ? now + 1. / 60 : 0;
        if (motion->next_wake_seconds)
            cui_invalidate(ui);
    }
    cui_box group = {.row = !o.vertical, .gap = o.gap > 0 ? o.gap : 12 * density};
    if (styles && styles->group)
        styles->group(ui, styles->group_user, id, &group);
    group.row = !o.vertical;
    group.layout_only = false;
    group.inert = false;
    cui_begin_box(ui, id, group);
    for (size_t i = 0; i < count; i++) {
        bool disabled = o.disabled || items[i].disabled, checked = *selected == items[i].id;
        float opacity = motion ? fminf(1, fmaxf(0, motion->items[i].progress)) : checked ? 1 : 0;
        cui_radio_paint paint = cui_radio_resolve_paint(
            ui, items[i].id, t, size, font, density, checked, disabled, o.invalid,
            o.hide_focus_ring, motion != NULL, opacity, styles ? &styles->items : NULL);
        const cui_radio_group_content *extra = content ? &content[i] : NULL;
        bool rich = extra && (extra->description.length || extra->content || extra->max_width > 0);
        cui_radio_begin_option(ui, items[i].id,
                               presentation.parts ? presentation.parts[i].indicator : 0, &paint,
                               items[i].label.length || rich, opacity, density);
        if (rich) {
            cui_begin_box(ui, 0, (cui_box){.gap = 3 * density, .max_width = extra->max_width});
            if (items[i].label.length)
                cui_radio_paint_text(ui, items[i].label, paint.label);
            if (extra->description.length)
                cui_radio_paint_text(ui, extra->description, paint.description);
            if (extra->content) {
                cui_begin_box(ui, 0, (cui_box){.inert = true});
                extra->content(ui, extra->content_user);
                cui_end_box(ui);
            }
            cui_end_box(ui);
        } else if (items[i].label.length)
            cui_radio_paint_text(ui, items[i].label, paint.label);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    for (size_t i = 0; content && i < count; i++) {
        bool disabled = o.disabled || items[i].disabled;
        if (cui_radio_tooltip_show(ui, t, items[i].id, disabled, content[i].tooltip) &&
            content[i].description.length)
            cui_radio_group_describe(ui, first_semantic, items[i].id, content[i].description);
    }
    return r;
}

cui_radio_group_result cui_radio_group(cui_context *ui, cui_id id, const cui_theme *theme,
                                       const cui_radio_group_item *items, size_t count,
                                       cui_id *selected, cui_radio_group_state *state,
                                       cui_radio_group_options options) {
    return cui_radio_group_render(ui, id, theme, items, count, selected, state, options,
                                  (cui_selection_handler){0}, NULL,
                                  (cui_radio_group_presentation){0}, NULL, NULL);
}
cui_radio_group_event_result cui_radio_group_events(cui_context *ui, cui_id id,
                                                    const cui_theme *theme,
                                                    const cui_radio_group_item *items, size_t count,
                                                    cui_id *selected, cui_radio_group_state *state,
                                                    cui_radio_group_options options,
                                                    cui_selection_handler handler) {
    cui_radio_group_event_result result;
    memset(&result, 0, sizeof(result));
    result.legacy =
        cui_radio_group_render(ui, id, theme, items, count, selected, state, options, handler,
                               &result.events, (cui_radio_group_presentation){0}, NULL, NULL);
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
                                  (cui_selection_handler){0}, NULL,
                                  (cui_radio_group_presentation){.parts = copied}, NULL, NULL);
}
cui_radio_group_event_result cui_radio_group_events_with_parts(
    cui_context *ui, cui_id id, const cui_theme *theme, const cui_radio_group_item *items,
    size_t count, cui_id *selected, cui_radio_group_state *state, cui_radio_group_options options,
    const cui_selection_indicator_parts *parts, size_t part_count, cui_selection_handler handler) {
    cui_radio_group_event_result result = {0};
    cui_selection_indicator_parts copied[CUI_RADIO_GROUP_MAX];
    if (!cui_radio_group_parts_copy(ui, id, items, count, parts, part_count, copied))
        return result;
    result.legacy = cui_radio_group_render(
        ui, id, theme, items, count, selected, state, options, handler, &result.events,
        (cui_radio_group_presentation){.parts = copied}, NULL, NULL);
    return result;
}

cui_radio_group_event_result
cui_radio_group_present(cui_context *ui, cui_id id, const cui_theme *theme,
                        const cui_radio_group_item *items, size_t count, cui_id *selected,
                        cui_radio_group_state *state, cui_radio_group_options options,
                        cui_radio_group_presentation presentation, cui_selection_handler handler) {
    return cui_radio_group_present_with_content(ui, id, theme, items, count, selected, state,
                                                options, presentation, NULL, 0, handler);
}

static bool cui_radio_group_content_valid(cui_id id, const cui_radio_group_item *items,
                                          size_t count, const cui_radio_group_content *content,
                                          const cui_selection_indicator_parts *parts) {
    for (size_t i = 0; i < count; i++) {
        const cui_radio_group_content *item = &content[i];
        if ((item->description.length && !item->description.data) || !isfinite(item->max_width) ||
            item->max_width < 0)
            return false;
        const cui_radio_tooltip *hint = item->tooltip;
        if (!hint)
            continue;
        if (!cui_radio_tooltip_valid(hint, id, 0))
            return false;
        for (size_t j = 0; j < count; j++)
            if (hint->id == items[j].id || (parts && hint->id == parts[j].indicator))
                return false;
        for (size_t j = 0; j < i; j++)
            if (content[j].tooltip &&
                (hint->id == content[j].tooltip->id || hint->state == content[j].tooltip->state))
                return false;
    }
    return true;
}

cui_radio_group_event_result cui_radio_group_present_with_content(
    cui_context *ui, cui_id id, const cui_theme *theme, const cui_radio_group_item *items,
    size_t count, cui_id *selected, cui_radio_group_state *state, cui_radio_group_options options,
    cui_radio_group_presentation presentation, const cui_radio_group_content *content,
    size_t content_count, cui_selection_handler handler) {
    return cui_radio_group_present_styled(ui, id, theme, items, count, selected, state, options,
                                          presentation, content, content_count,
                                          (cui_radio_group_styles){0}, handler);
}

cui_radio_group_event_result cui_radio_group_present_styled(
    cui_context *ui, cui_id id, const cui_theme *theme, const cui_radio_group_item *items,
    size_t count, cui_id *selected, cui_radio_group_state *state, cui_radio_group_options options,
    cui_radio_group_presentation presentation, const cui_radio_group_content *content,
    size_t content_count, cui_radio_group_styles styles, cui_selection_handler handler) {
    cui_radio_group_event_result result = {0};
    cui_selection_indicator_parts copied[CUI_RADIO_GROUP_MAX];
    if (presentation.parts || presentation.part_count) {
        if (!cui_radio_group_parts_copy(ui, id, items, count, presentation.parts,
                                        presentation.part_count, copied))
            return result;
        if (presentation.parts)
            presentation.parts = copied;
    }
    cui_radio_group_content copied_content[CUI_RADIO_GROUP_MAX];
    if (content || content_count) {
        bool valid = content && content_count == count && count <= CUI_RADIO_GROUP_MAX &&
                     (!count || items) &&
                     cui_radio_group_content_valid(id, items, count, content, presentation.parts);
        if (!valid) {
            if (ui)
                cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_INVALID_ACTION,
                                                            .operation = "radio_group_content",
                                                            .id = id});
            return result;
        }
        if (count)
            memcpy(copied_content, content, count * sizeof(*content));
    }
    result.legacy = cui_radio_group_render(ui, id, theme, items, count, selected, state, options,
                                           handler, &result.events, presentation,
                                           content ? copied_content : NULL, &styles);
    return result;
}
