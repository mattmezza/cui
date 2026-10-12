/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_base.h"
#include <math.h>
static void cui_choice_draw(cui_context *ui, const cui_choice_item *item, const cui_theme *theme,
                            bool selected, bool disabled, cui_tab_variant variant) {
    cui_box box = {.height = theme->control_height,
                   .padding = theme->padding,
                   .radius = theme->radius,
                   .background = selected ? theme->primary : theme->surface,
                   .border = cui_focused(ui) == item->id ? theme->focus : theme->border,
                   .border_width = cui_focused(ui) == item->id ? 2 : 1};
    if (variant == CUI_TAB_UNDERLINE) {
        box.background = theme->background;
        box.radius = 0;
        box.padding = 0;
        box.border_width = 0;
    } else if (variant == CUI_TAB_PILL) {
        box.radius = theme->control_height * .5f;
        if (!selected)
            box.border_width = 0;
    } else if (variant == CUI_TAB_OUTLINE) {
        box.background = theme->background;
        box.border = selected ? theme->primary : theme->border;
    } else if (variant == CUI_TAB_SEGMENTED) {
        box.radius = 0;
    }
    if (cui_focused(ui) == item->id) {
        box.border = theme->focus;
        box.border_width = 2;
    }
    cui_begin_box(ui, item->id, box);
    if (variant == CUI_TAB_UNDERLINE)
        cui_begin_box(ui, 0,
                      (cui_box){.height = theme->control_height > 2 ? theme->control_height - 2 : 0,
                                .padding = theme->padding});
    cui_text(ui, item->label, theme->font_size,
             disabled ? theme->muted
             : selected && variant != CUI_TAB_UNDERLINE && variant != CUI_TAB_OUTLINE
                 ? theme->on_primary
                 : theme->text);
    if (variant == CUI_TAB_UNDERLINE) {
        cui_end_box(ui);
        cui_begin_box(ui, 0,
                      (cui_box){.width = -1,
                                .height = 2,
                                .background = selected ? theme->primary : theme->background});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
bool cui_tabs(cui_context *ui, cui_id container, const cui_theme *theme,
              const cui_choice_item *items, size_t count, cui_id *selected,
              cui_tabs_options options, cui_tab_panel_fn panel, void *user) {
    if (!theme || !items || !selected)
        return false;
    bool changed = cui_base_tabs(ui, items, count, selected, options);
    cui_begin_box(ui, container, (cui_box){.gap = theme->gap});
    cui_id list_id = container ^ UINT64_C(0xcda0312561f50cab);
    cui_interact(ui, list_id, CUI_ROLE_TABLIST, cui_string(""), options.disabled, false, 0);
    cui_begin_box(ui, container ^ UINT64_C(0xcda0312561f50cab),
                  (cui_box){.row = !options.vertical, .gap = theme->gap});
    size_t active = count;
    for (size_t i = 0; i < count; ++i) {
        cui_choice_draw(ui, &items[i], theme, items[i].id == *selected,
                        options.disabled || items[i].disabled, options.variant);
        cui_set_semantic_details(
            ui, items[i].id,
            (cui_semantic_details){.selected = items[i].id == *selected,
                                   .controls = items[i].id ^ UINT64_C(0x691a17db80e22f45),
                                   .parent = list_id});
        if (items[i].id == *selected && !items[i].disabled)
            active = i;
    }
    cui_end_box(ui);
    if (active != count && panel) {
        cui_id panel_id = items[active].id ^ UINT64_C(0x691a17db80e22f45);
        cui_interact(ui, panel_id, CUI_ROLE_TABPANEL, items[active].label, false, false, 0);
        cui_set_semantic_details(
            ui, panel_id,
            (cui_semantic_details){.labelled_by = items[active].id, .parent = container});
        cui_begin_box(ui, items[active].id ^ UINT64_C(0x691a17db80e22f45),
                      (cui_box){.padding = theme->padding, .gap = theme->gap});
        panel(ui, active, user);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return changed;
}
bool cui_toggle_group(cui_context *ui, cui_id container, const cui_theme *theme,
                      const cui_choice_item *items, size_t count, bool *selected,
                      cui_toggle_group_options options) {
    if (!theme || !items || !selected)
        return false;
    bool changed = cui_base_toggle_group(ui, items, count, selected, options);
    cui_interact(ui, container, CUI_ROLE_GROUP, cui_string(""), options.disabled, false, 0);
    cui_begin_box(ui, container, (cui_box){.row = !options.vertical, .gap = theme->gap});
    for (size_t i = 0; i < count; ++i)
        cui_choice_draw(ui, &items[i], theme, selected[i], options.disabled || items[i].disabled,
                        CUI_TAB_DEFAULT);
    cui_end_box(ui);
    return changed;
}

static void cui_choice_paint_list(cui_context *ui, cui_id container, const cui_theme *theme,
                                  const cui_choice_item *items, size_t count, cui_id *selected,
                                  cui_list_state *state, cui_list_options options,
                                  const bool *multiple_selected) {

    cui_interact(ui, container, options.menu ? CUI_ROLE_MENU : CUI_ROLE_LISTBOX, cui_string(""),
                 options.disabled, false, 0);
    cui_begin_box(ui, container, (cui_box){.gap = 0});
    size_t begin = options.visible_count ? state->first_visible : 0;
    size_t length = options.visible_count ? options.visible_count : count;
    if (begin > count)
        begin = count;
    if (length > count - begin)
        length = count - begin;
    for (size_t i = begin; i < begin + length; i++) {
        cui_box box = {.height = theme->control_height,
                       .width = -1,
                       .padding = theme->padding,
                       .row = true,
                       .gap = theme->gap,
                       .background = state->active == items[i].id ? theme->primary : theme->surface,
                       .border = cui_focused(ui) == items[i].id ? theme->focus : theme->surface,
                       .border_width = cui_focused(ui) == items[i].id ? 1 : 0};
        cui_begin_box(ui, items[i].id, box);
        if (multiple_selected)
            cui_text(ui, cui_string(multiple_selected[i] ? "[x]" : "[ ]"), theme->font_size,
                     options.disabled || items[i].disabled ? theme->muted
                     : state->active == items[i].id        ? theme->on_primary
                                                           : theme->text);
        cui_text(ui, items[i].label, theme->font_size,
                 options.disabled || items[i].disabled ? theme->muted
                 : state->active == items[i].id        ? theme->on_primary
                                                       : theme->text);
        cui_end_box(ui);
        bool checked = multiple_selected ? multiple_selected[i] : *selected == items[i].id;
        cui_update_semantic(ui, items[i].id, checked, (float)i);
        cui_set_semantic_details(ui, items[i].id,
                                 (cui_semantic_details){.selected = checked, .parent = container});
    }
    cui_end_box(ui);
}
bool cui_list(cui_context *ui, cui_id container, const cui_theme *theme,
              const cui_choice_item *items, size_t count, cui_id *selected, cui_list_state *state,
              cui_list_options options) {
    if (!theme || !items || !selected || !state)
        return false;
    bool activated = cui_base_list(ui, items, count, selected, state, options);
    cui_choice_paint_list(ui, container, theme, items, count, selected, state, options, NULL);
    return activated;
}

static bool cui_choice_popup_render(cui_context *ui, cui_id trigger, const cui_theme *theme,
                                    cui_str name, cui_str placeholder, const cui_choice_item *items,
                                    size_t count, cui_id *selected, cui_choice_popup *state,
                                    cui_choice_popup_options options, bool menu) {
    if (!theme || !selected || !state || !state->overlay.id || (!items && count) ||
        !isfinite(options.viewport.x) || !isfinite(options.viewport.y) ||
        !isfinite(options.viewport.w) || !isfinite(options.viewport.h) || options.viewport.w <= 0 ||
        options.viewport.h <= 0 || !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 || theme->padding > options.viewport.h / 2)
        return false;
    cui_base_choice_popup_trigger(ui, trigger, name, options.disabled, items, count, *selected,
                                  state);
    cui_str display = menu ? name : placeholder;
    if (!menu)
        for (size_t i = 0; i < count; i++)
            if (items[i].id == *selected)
                display = items[i].label;
    cui_set_semantic_details(ui, trigger,
                             (cui_semantic_details){.expanded = state->overlay.open,
                                                    .controls = state->overlay.id,
                                                    .description = display});
    cui_choice_item trigger_item = {trigger, display, options.disabled};
    cui_choice_draw(ui, &trigger_item, theme, false, options.disabled, CUI_TAB_DEFAULT);
    if (!state->overlay.open)
        return false;
    float width = options.width > 0 && isfinite(options.width) ? options.width : 240;
    if (width > options.viewport.w)
        width = options.viewport.w;
    float available = options.viewport.h - 2 * theme->padding - 8;
    size_t rows = count;
    if (available / theme->control_height < (float)rows) {
        rows = available > theme->control_height ? (size_t)(available / theme->control_height) : 1;
    }
    if (rows > 4096)
        rows = 4096;
    float height = 2 * theme->padding + (rows ? rows : 1) * theme->control_height;
    cui_popup_result placement = cui_popup_place(cui_bounds(ui, trigger), (cui_size){width, height},
                                                 options.viewport, CUI_POPUP_BELOW, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return false;
    cui_list_options list_options = {.disabled = options.disabled,
                                     .menu = menu,
                                     .dt = options.dt,
                                     .visible_begin = state->list.first_visible,
                                     .visible_count = rows};
    cui_id old = *selected;
    bool activated =
        count ? cui_base_list(ui, items, count, selected, &state->list, list_options) : false;
    if (activated) {
        cui_id scope = state->overlay.id;
        cui_overlay_close(ui, &state->overlay);
        cui_dismiss_focus_scope(ui, scope, trigger);
        cui_overlay_end(ui);
        cui_update_semantic(ui, trigger, false, 0);
        cui_set_semantic_details(ui, trigger,
                                 (cui_semantic_details){.controls = scope, .description = display});
        return menu || old != *selected;
    }
    cui_begin_portal(ui, state->overlay.id,
                     (cui_box){.padding = theme->padding,
                               .clip = true,
                               .background = theme->surface,
                               .border = theme->border,
                               .border_width = 1,
                               .radius = theme->radius},
                     placement.bounds, 100);
    cui_interact(ui, state->overlay.id, menu ? CUI_ROLE_MENU : CUI_ROLE_LISTBOX, name, false, false,
                 0);
    cui_set_semantic_details(ui, state->overlay.id, (cui_semantic_details){.labelled_by = trigger});
    if (count)
        cui_choice_paint_list(ui, state->overlay.id ^ UINT64_C(0xd1bc905ee4216f11), theme, items,
                              count, selected, &state->list, list_options, NULL);
    else
        cui_text(ui, cui_string("No options"), theme->font_size, theme->muted);
    cui_end_box(ui);
    cui_overlay_end(ui);
    return false;
}
bool cui_select(cui_context *ui, cui_id trigger, const cui_theme *theme, cui_str name,
                cui_str placeholder, const cui_choice_item *items, size_t count, cui_id *selected,
                cui_choice_popup *state, cui_choice_popup_options options) {
    return cui_choice_popup_render(ui, trigger, theme, name, placeholder, items, count, selected,
                                   state, options, false);
}
bool cui_menu(cui_context *ui, cui_id trigger, const cui_theme *theme, cui_str name,
              const cui_choice_item *items, size_t count, cui_id *action, cui_choice_popup *state,
              cui_choice_popup_options options) {
    return cui_choice_popup_render(ui, trigger, theme, name, name, items, count, action, state,
                                   options, true);
}

cui_combobox_result cui_combobox(cui_context *ui, cui_id input, const cui_theme *theme,
                                 cui_str name, cui_edit *query, const cui_choice_item *items,
                                 size_t count, cui_choice_item *filtered, size_t capacity,
                                 cui_id *selected, cui_choice_popup *state,
                                 cui_choice_popup_options options) {
    cui_combobox_result result = {.error = CUI_EDIT_OK};
    if (!theme || !query || !selected || !state || !state->overlay.id || (!items && count) ||
        (!filtered && capacity) || capacity < count || cui_edit_get_state(query).multiline ||
        !isfinite(options.viewport.w) || !isfinite(options.viewport.h) || options.viewport.w <= 0 ||
        options.viewport.h <= 0 || !isfinite(options.viewport.x) || !isfinite(options.viewport.y) ||
        !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 ||
        theme->padding > options.viewport.h / 2) {
        result.error = capacity < count ? CUI_EDIT_CAPACITY : CUI_EDIT_INVALID_ARGUMENT;
        return result;
    }
    result = cui_base_combobox(ui, input, name, query, items, count, filtered, capacity, selected,
                               state, options.disabled);
    cui_box input_box = {.width = 280,
                         .padding = theme->padding,
                         .gap = theme->gap,
                         .radius = theme->radius,
                         .background = theme->surface,
                         .border = cui_focused(ui) == input ? theme->focus : theme->border,
                         .border_width = cui_focused(ui) == input ? 2 : 1};
    cui_begin_box(ui, input, input_box);
    cui_text(ui, name, theme->font_size * .8f, theme->muted);
    cui_str text = cui_edit_get_text(query);
    cui_text(ui, text.length ? text : cui_string(" "), theme->font_size,
             options.disabled ? theme->muted : theme->text);
    cui_end_box(ui);
    if (!state->overlay.open)
        return result;
    size_t matched = result.matching_count;
    float width = options.width > 0 && isfinite(options.width) ? options.width : 280;
    if (width > options.viewport.w)
        width = options.viewport.w;
    float available = options.viewport.h - 2 * theme->padding - 8;
    size_t rows = matched;
    if (available / theme->control_height < (float)rows)
        rows = available > theme->control_height ? (size_t)(available / theme->control_height) : 1;
    if (rows > 4096)
        rows = 4096;
    cui_popup_result placement = cui_popup_place(
        cui_bounds(ui, input),
        (cui_size){width, 2 * theme->padding + (rows ? rows : 1) * theme->control_height},
        options.viewport, CUI_POPUP_BELOW, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return result;
    cui_list_options list_options = {
        .dt = options.dt, .visible_begin = state->list.first_visible, .visible_count = rows};
    cui_id old = *selected;
    bool activated =
        matched ? cui_base_list(ui, filtered, matched, selected, &state->list, list_options)
                : false;
    if (activated) {
        cui_overlay_close(ui, &state->overlay);
        cui_dismiss_focus_scope(ui, state->overlay.id, input);
        cui_overlay_end(ui);
        cui_set_semantic_details(
            ui, input, (cui_semantic_details){.controls = state->overlay.id, .description = text});
        result.changed = old != *selected;
        return result;
    }
    cui_begin_portal(ui, state->overlay.id,
                     (cui_box){.padding = theme->padding,
                               .clip = true,
                               .background = theme->surface,
                               .border = theme->border,
                               .border_width = 1,
                               .radius = theme->radius},
                     placement.bounds, 100);
    cui_interact(ui, state->overlay.id, CUI_ROLE_LISTBOX, name, false, false, 0);
    cui_set_semantic_details(ui, state->overlay.id, (cui_semantic_details){.labelled_by = input});
    if (matched)
        cui_choice_paint_list(ui, state->overlay.id ^ UINT64_C(0xd1bc905ee4216f11), theme, filtered,
                              matched, selected, &state->list, list_options, NULL);
    else
        cui_text(ui, cui_string("No matches"), theme->font_size, theme->muted);
    cui_end_box(ui);
    cui_overlay_end(ui);
    return result;
}

bool cui_list_multi(cui_context *ui, cui_id container, const cui_theme *theme,
                    const cui_choice_item *items, size_t count, bool *selected,
                    cui_list_state *state, cui_list_options options) {
    if (!theme || !items || !selected || !state)
        return false;
    bool activated = cui_base_list_multi(ui, items, count, selected, state, options);
    cui_id action = 0;
    cui_choice_paint_list(ui, container, theme, items, count, &action, state, options, selected);
    return activated;
}
static bool cui_choice_chips(cui_context *ui, cui_id input, const cui_theme *theme,
                             const cui_choice_item *items, size_t count, bool *selected,
                             bool disabled) {
    bool changed = false;
    cui_begin_box(ui, input ^ UINT64_C(0x97c22bb751e0d56a), (cui_box){.gap = theme->gap});
    for (size_t i = 0; i < count; i++)
        if (selected[i]) {
            cui_id chip = items[i].id ^ UINT64_C(0x45f2b82e6c9bf447);
            bool removed = cui_base_selection_remove(ui, chip, items[i].label, &selected[i],
                                                     disabled || items[i].disabled);
            cui_choice_item chip_item = {chip, items[i].label, items[i].disabled};
            cui_choice_draw(ui, &chip_item, theme, true, disabled || items[i].disabled,
                            CUI_TAB_PILL);
            if (removed) {
                changed = true;
                cui_focus(ui, input);
            }
        }
    cui_end_box(ui);
    return changed;
}
bool cui_select_multi(cui_context *ui, cui_id trigger, const cui_theme *theme, cui_str name,
                      cui_str placeholder, const cui_choice_item *items, size_t count,
                      bool *selected, cui_choice_popup *state, cui_choice_popup_options options) {
    if (!theme || !selected || !state || !state->overlay.id || (!items && count) ||
        !isfinite(options.viewport.w) || !isfinite(options.viewport.h) || options.viewport.w <= 0 ||
        options.viewport.h <= 0 || !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 || theme->padding > options.viewport.h / 2)
        return false;
    cui_id preferred = 0;
    size_t selected_count = 0;
    for (size_t i = 0; i < count; i++)
        if (selected[i]) {
            if (!preferred)
                preferred = items[i].id;
            selected_count++;
        }
    cui_base_choice_popup_trigger(ui, trigger, name, options.disabled, items, count, preferred,
                                  state);
    cui_begin_box(ui, trigger ^ UINT64_C(0x471d50d3bf973ac9), (cui_box){.gap = theme->gap});
    cui_choice_item trigger_item = {trigger, placeholder, options.disabled};
    if (selected_count)
        trigger_item.label = cui_string("Selected options");
    cui_choice_draw(ui, &trigger_item, theme, false, options.disabled, CUI_TAB_DEFAULT);
    bool changed = false;
    if (selected_count)
        changed |= cui_choice_chips(ui, trigger, theme, items, count, selected, options.disabled);
    cui_end_box(ui);
    if (!state->overlay.open)
        return changed;
    float width = options.width > 0 && isfinite(options.width) ? options.width : 240;
    if (width > options.viewport.w)
        width = options.viewport.w;
    float available = options.viewport.h - 2 * theme->padding - 8;
    size_t rows = count;
    if (available / theme->control_height < (float)rows)
        rows = available > theme->control_height ? (size_t)(available / theme->control_height) : 1;
    if (rows > 4096)
        rows = 4096;
    cui_popup_result placement = cui_popup_place(
        cui_bounds(ui, trigger),
        (cui_size){width, 2 * theme->padding + (rows ? rows : 1) * theme->control_height},
        options.viewport, CUI_POPUP_BELOW, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return changed;
    cui_list_options list_options = {.disabled = options.disabled,
                                     .dt = options.dt,
                                     .visible_begin = state->list.first_visible,
                                     .visible_count = rows};
    if (count)
        changed |= cui_base_list_multi(ui, items, count, selected, &state->list, list_options);
    cui_begin_portal(ui, state->overlay.id,
                     (cui_box){.padding = theme->padding,
                               .clip = true,
                               .background = theme->surface,
                               .border = theme->border,
                               .border_width = 1,
                               .radius = theme->radius},
                     placement.bounds, 100);
    cui_interact(ui, state->overlay.id, CUI_ROLE_LISTBOX, name, false, false, 0);
    cui_set_semantic_details(ui, state->overlay.id, (cui_semantic_details){.labelled_by = trigger});
    cui_id action = 0;
    if (count)
        cui_choice_paint_list(ui, state->overlay.id ^ UINT64_C(0xd1bc905ee4216f11), theme, items,
                              count, &action, &state->list, list_options, selected);
    else
        cui_text(ui, cui_string("No options"), theme->font_size, theme->muted);
    cui_end_box(ui);
    cui_overlay_end(ui);
    return changed;
}

cui_combobox_result cui_combobox_multi(cui_context *ui, cui_id input, const cui_theme *theme,
                                       cui_str name, cui_edit *query, const cui_choice_item *items,
                                       size_t count, cui_choice_item *filtered,
                                       bool *filtered_selected, size_t capacity, bool *selected,
                                       cui_choice_popup *state, cui_choice_popup_options options) {
    cui_combobox_result result = {.error = CUI_EDIT_OK};
    if (!theme || !query || !selected || !state || !state->overlay.id || (!items && count) ||
        (!filtered && capacity) || capacity < count || cui_edit_get_state(query).multiline ||
        !isfinite(options.viewport.w) || !isfinite(options.viewport.h) || options.viewport.w <= 0 ||
        options.viewport.h <= 0 || !isfinite(options.viewport.x) || !isfinite(options.viewport.y) ||
        !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 ||
        theme->padding > options.viewport.h / 2) {
        result.error = capacity < count ? CUI_EDIT_CAPACITY : CUI_EDIT_INVALID_ARGUMENT;
        return result;
    }
    result =
        cui_base_combobox_multi(ui, input, name, query, items, count, filtered, filtered_selected,
                                capacity, selected, state, options.disabled);
    cui_box input_box = {.width = 280,
                         .padding = theme->padding,
                         .gap = theme->gap,
                         .radius = theme->radius,
                         .background = theme->surface,
                         .border = cui_focused(ui) == input ? theme->focus : theme->border,
                         .border_width = cui_focused(ui) == input ? 2 : 1};
    cui_begin_box(ui, input, input_box);
    cui_text(ui, name, theme->font_size * .8f, theme->muted);
    cui_str text = cui_edit_get_text(query);
    cui_text(ui, text.length ? text : cui_string(" "), theme->font_size,
             options.disabled ? theme->muted : theme->text);
    cui_end_box(ui);
    result.changed |= cui_choice_chips(ui, input, theme, items, count, selected, options.disabled);
    if (!state->overlay.open)
        return result;
    size_t matched = result.matching_count;
    float width = options.width > 0 && isfinite(options.width) ? options.width : 280;
    if (width > options.viewport.w)
        width = options.viewport.w;
    float available = options.viewport.h - 2 * theme->padding - 8;
    size_t rows = matched;
    if (available / theme->control_height < (float)rows)
        rows = available > theme->control_height ? (size_t)(available / theme->control_height) : 1;
    if (rows > 4096)
        rows = 4096;
    cui_popup_result placement = cui_popup_place(
        cui_bounds(ui, input),
        (cui_size){width, 2 * theme->padding + (rows ? rows : 1) * theme->control_height},
        options.viewport, CUI_POPUP_BELOW, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return result;
    cui_list_options list_options = {
        .dt = options.dt, .visible_begin = state->list.first_visible, .visible_count = rows};
    bool activated = matched ? cui_base_list_multi(ui, filtered, matched, filtered_selected,
                                                   &state->list, list_options)
                             : false;
    if (activated) {
        size_t source = 0;
        for (size_t i = 0; i < matched; i++) {
            while (source < count && items[source].id != filtered[i].id)
                source++;
            if (source < count)
                selected[source++] = filtered_selected[i];
        }
        result.changed = true;
    }
    cui_begin_portal(ui, state->overlay.id,
                     (cui_box){.padding = theme->padding,
                               .clip = true,
                               .background = theme->surface,
                               .border = theme->border,
                               .border_width = 1,
                               .radius = theme->radius},
                     placement.bounds, 100);
    cui_interact(ui, state->overlay.id, CUI_ROLE_LISTBOX, name, false, false, 0);
    cui_set_semantic_details(ui, state->overlay.id, (cui_semantic_details){.labelled_by = input});
    cui_id action = 0;
    if (matched)
        cui_choice_paint_list(ui, state->overlay.id ^ UINT64_C(0xd1bc905ee4216f11), theme, filtered,
                              matched, &action, &state->list, list_options, filtered_selected);
    else
        cui_text(ui, cui_string("No matches"), theme->font_size, theme->muted);
    cui_end_box(ui);
    cui_overlay_end(ui);
    return result;
}
