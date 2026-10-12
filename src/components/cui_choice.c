/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_base.h"
#include "cui/cui_components.h"
#include <math.h>
static void cui_choice_menu_level(cui_context *, const cui_theme *, const cui_menu_entry *, size_t,
                                  cui_menu_tree_state *, size_t, cui_rect, cui_menu_tree_options,
                                  cui_str);
static void cui_choice_ellipsized_text(cui_context *ui, cui_str label, float font_size,
                                       cui_color color, float width) {
    cui_str ellipsis = cui_string("\xe2\x80\xa6");
    float marker_width = cui_measure_text(ui, ellipsis, font_size).width;
    if (marker_width > width)
        return;
    size_t bytes = 0, upper = label.length;
    while (bytes < upper) {
        size_t middle = bytes + (upper - bytes + 1) / 2;
        while (middle < label.length && middle &&
               ((unsigned char)label.data[middle] & 0xc0) == 0x80)
            middle--;
        if (middle <= bytes) {
            middle = bytes + 1;
            while (middle < label.length && ((unsigned char)label.data[middle] & 0xc0) == 0x80)
                middle++;
        }
        if (cui_measure_text(ui, (cui_str){label.data, middle}, font_size).width + marker_width <=
            width)
            bytes = middle;
        else {
            upper = middle - 1;
            while (upper && ((unsigned char)label.data[upper] & 0xc0) == 0x80)
                upper--;
        }
    }
    cui_begin_box(ui, 0, (cui_box){.row = true, .gap = 0});
    if (bytes)
        cui_text(ui, (cui_str){label.data, bytes}, font_size, color);
    cui_text(ui, ellipsis, font_size, color);
    cui_end_box(ui);
}
static void cui_choice_draw_ex(cui_context *ui, const cui_choice_item *item, const cui_theme *theme,
                               bool selected, bool disabled, cui_tab_variant variant,
                               cui_choice_render_fn render, size_t index, void *render_user,
                               float max_width) {
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
    bool ellipsized =
        !render && isfinite(max_width) && max_width > 0 &&
        cui_measure_text(ui, item->label, theme->font_size).width + 2 * theme->padding > max_width;
    if (ellipsized) {
        box.width = max_width;
        box.clip = true;
    }
    cui_begin_box(ui, item->id, box);
    if (variant == CUI_TAB_UNDERLINE)
        cui_begin_box(ui, 0,
                      (cui_box){.height = theme->control_height > 2 ? theme->control_height - 2 : 0,
                                .padding = theme->padding});
    if (render)
        render(ui, item, index, render_user);
    else {
        cui_color color = disabled ? theme->muted
                          : selected && variant != CUI_TAB_UNDERLINE && variant != CUI_TAB_OUTLINE
                              ? theme->on_primary
                              : theme->text;
        if (ellipsized)
            cui_choice_ellipsized_text(ui, item->label, theme->font_size, color,
                                       fmaxf(0, max_width - 2 * theme->padding));
        else
            cui_text(ui, item->label, theme->font_size, color);
    }
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
static void cui_choice_draw(cui_context *ui, const cui_choice_item *item, const cui_theme *theme,
                            bool selected, bool disabled, cui_tab_variant variant) {
    cui_choice_draw_ex(ui, item, theme, selected, disabled, variant, NULL, 0, NULL, 0);
}
bool cui_tabs(cui_context *ui, cui_id container, const cui_theme *theme,
              const cui_choice_item *items, size_t count, cui_id *selected,
              cui_tabs_options options, cui_tab_panel_fn panel, void *user) {
    if (!theme || !items || !selected)
        return false;
    cui_id viewport_id = container ^ UINT64_C(0x739b1b53a6cef421);
    bool scrollable = options.scroll && isfinite(options.bar_extent) && options.bar_extent > 0;
    if (scrollable) {
        options.scroll->receiving_id = viewport_id;
        options.scroll->viewport = cui_bounds(ui, viewport_id);
        float content = count > 1 ? (float)(count - 1) * theme->gap : 0;
        for (size_t i = 0; i < count; i++) {
            float extent = options.vertical ? theme->control_height
                           : options.render
                               ? cui_bounds(ui, items[i].id).w
                               : cui_measure_text(ui, items[i].label, theme->font_size).width +
                                     2 * theme->padding;
            if (!options.vertical && !options.render && options.max_width > 0 &&
                extent > options.max_width)
                extent = options.max_width;
            if (isfinite(extent) && extent > 0)
                content += extent;
        }
        options.scroll->maximum = fmaxf(0, content - options.bar_extent);
        options.scroll->scrollbar_id = options.scrollbar && options.scroll->maximum > 0
                                           ? container ^ UINT64_C(0x973a1e35f90453bc)
                                           : 0;
        options.scroll->scrollbar_bounds = cui_bounds(ui, options.scroll->scrollbar_id);

    } else
        options.scroll = NULL;
    bool changed = cui_base_tabs(ui, items, count, selected, options);
    if (scrollable) {
        cui_tab_scroll_state *scroll = options.scroll;
        if (options.reveal_selected && scroll->last_selected != *selected && !scroll->reveal_id)
            scroll->reveal_id = *selected;
        scroll->last_selected = *selected;
        if (scroll->reveal_id) {
            float start = 0;
            bool found = false;
            for (size_t i = 0; i < count; i++) {
                float extent = options.vertical ? theme->control_height
                               : options.render
                                   ? cui_bounds(ui, items[i].id).w
                                   : cui_measure_text(ui, items[i].label, theme->font_size).width +
                                         2 * theme->padding;
                if (!options.vertical && !options.render && options.max_width > 0 &&
                    extent > options.max_width)
                    extent = options.max_width;
                if (items[i].id == scroll->reveal_id) {
                    found = true;
                    if (extent > 0 && isfinite(extent)) {
                        if (cui_tab_scroll_reveal(scroll, start, extent, options.bar_extent))
                            cui_invalidate(ui);
                        scroll->reveal_id = 0;
                    }
                    break;
                }
                if (extent > 0 && isfinite(extent))
                    start += extent + theme->gap;
            }
            if (!found)
                scroll->reveal_id = 0;
        }
    }

    cui_begin_box(ui, container, (cui_box){.gap = theme->gap});
    cui_id list_id = container ^ UINT64_C(0xcda0312561f50cab);
    cui_interact(ui, list_id, CUI_ROLE_TABLIST, cui_string(""), options.disabled, false, 0);
    cui_set_semantic_details(
        ui, list_id,
        (cui_semantic_details){.orientation = options.vertical ? CUI_ORIENTATION_VERTICAL
                                                               : CUI_ORIENTATION_HORIZONTAL});
    cui_begin_box(ui, container ^ UINT64_C(0xcda0312561f50cab),
                  (cui_box){.row = !options.vertical, .gap = theme->gap});
    if (options.prefix)
        options.prefix(ui, options.content_user);
    if (scrollable) {
        cui_interact(ui, viewport_id, CUI_ROLE_GROUP, cui_string("Tab viewport"), options.disabled,
                     false, 0);
        cui_set_tab_stop(ui, viewport_id, false);
        cui_set_semantic_details(
            ui, viewport_id,
            (cui_semantic_details){.parent = list_id,
                                   .scrollable = true,
                                   .scroll_x = options.vertical ? 0 : options.scroll->offset,
                                   .scroll_y = options.vertical ? options.scroll->offset : 0,
                                   .scroll_max_x = options.vertical ? 0 : options.scroll->maximum,
                                   .scroll_max_y = options.vertical ? options.scroll->maximum : 0});
        cui_begin_box(
            ui, viewport_id,
            (cui_box){.row = !options.vertical,
                      .gap = theme->gap,
                      .clip = true,
                      .width = options.vertical ? 0 : options.bar_extent,
                      .height = options.vertical ? options.bar_extent : theme->control_height,
                      .offset_x = options.vertical ? 0 : -options.scroll->offset,
                      .offset_y = options.vertical ? -options.scroll->offset : 0});
    }
    size_t active = count;
    for (size_t i = 0; i < count; ++i) {
        cui_choice_draw_ex(ui, &items[i], theme, items[i].id == *selected,
                           options.disabled || items[i].disabled, options.variant, options.render,
                           i, options.render_user, options.max_width);
        cui_set_semantic_details(
            ui, items[i].id,
            (cui_semantic_details){.selected = items[i].id == *selected,
                                   .controls = items[i].id ^ UINT64_C(0x691a17db80e22f45),
                                   .parent = list_id});
        if (items[i].id == *selected && !items[i].disabled)
            active = i;
    }
    if (options.drag && options.drag->active && options.drag->drop_valid) {
        cui_rect bounds = {0};
        if (options.drag->before)
            bounds = cui_bounds(ui, options.drag->before);
        else if (count) {
            bounds = cui_bounds(ui, items[count - 1].id);
            if (options.vertical)
                bounds.y += bounds.h;
            else
                bounds.x += bounds.w;
        }
        bool visible = bounds.w > 0 && bounds.h > 0;
        if (scrollable) {
            cui_rect viewport = options.scroll->viewport;
            visible &= bounds.x >= viewport.x && bounds.x <= viewport.x + viewport.w &&
                       bounds.y >= viewport.y && bounds.y <= viewport.y + viewport.h;
        }
        if (visible) {
            cui_rect marker = options.vertical ? (cui_rect){bounds.x, bounds.y, bounds.w, 2}
                                               : (cui_rect){bounds.x, bounds.y, 2, bounds.h};
            cui_begin_portal(ui, container ^ UINT64_C(0x5df27b10b325643e),
                             (cui_box){.background = theme->primary}, marker, 90);
            cui_end_box(ui);
        }
    }
    if (scrollable)
        cui_end_box(ui);
    if (options.suffix)
        options.suffix(ui, options.content_user);
    if (options.overflow && options.overflow->visible) {
        cui_tab_overflow *overflow = options.overflow;
        cui_choice_item trigger = {.id = overflow->trigger, .label = {"v", 1}};
        cui_choice_draw(ui, &trigger, theme, overflow->menu.overlay.open,
                        options.disabled || overflow->options.popup.disabled, CUI_TAB_DEFAULT);
        cui_set_semantic_details(ui, overflow->trigger, (cui_semantic_details){.parent = list_id});
        overflow->menu.row_height = theme->control_height;
        overflow->menu.padding = theme->padding;
        if (overflow->menu.overlay.open)
            cui_choice_menu_level(ui, theme, overflow->entries, count, &overflow->menu, 0,
                                  cui_bounds(ui, overflow->trigger), overflow->options,
                                  cui_string("All tabs"));
    }

    cui_end_box(ui);
    if (scrollable && options.scroll->scrollbar_id) {
        cui_tab_scroll_state *scroll = options.scroll;
        cui_rect viewport = scroll->viewport;
        cui_rect track = options.vertical
                             ? (cui_rect){viewport.x + viewport.w - 6, viewport.y, 6, viewport.h}
                             : (cui_rect){viewport.x, viewport.y + viewport.h - 6, viewport.w, 6};
        if (track.w > 0 && track.h > 0) {
            float length = options.vertical ? track.h : track.w;
            float thumb_length = length * (float)((double)options.bar_extent /
                                                  ((double)options.bar_extent + scroll->maximum));
            thumb_length = fminf(length, fmaxf(16, thumb_length));
            float thumb_start = (length - thumb_length) * scroll->offset / scroll->maximum;
            cui_rect thumb =
                options.vertical
                    ? (cui_rect){track.x, track.y + thumb_start, track.w, thumb_length}
                    : (cui_rect){track.x + thumb_start, track.y, thumb_length, track.h};
            cui_begin_portal(
                ui, scroll->scrollbar_id,
                (cui_box){.background = theme->border,
                          .radius = 3,
                          .border = theme->focus,
                          .border_width = cui_focused(ui) == scroll->scrollbar_id ? 1 : 0},
                track, 1);
            cui_interact(ui, scroll->scrollbar_id, CUI_ROLE_SCROLLBAR, cui_string("Tab scrollbar"),
                         options.disabled, false, scroll->offset);
            cui_end_box(ui);
            cui_begin_portal(
                ui, scroll->scrollbar_id ^ UINT64_C(0x247d15723f61c98b),
                (cui_box){.background = options.disabled ? theme->muted : theme->primary,
                          .radius = 3},
                thumb, 2);
            cui_end_box(ui);
        }
        cui_update_semantic(ui, scroll->scrollbar_id, false, scroll->offset);
        cui_set_semantic_details(
            ui, scroll->scrollbar_id,
            (cui_semantic_details){.orientation = options.vertical ? CUI_ORIENTATION_VERTICAL
                                                                   : CUI_ORIENTATION_HORIZONTAL,
                                   .minimum = 0,
                                   .maximum = scroll->maximum,
                                   .controls = viewport_id,
                                   .parent = list_id,
                                   .scrollable = true,
                                   .scroll_x = options.vertical ? 0 : scroll->offset,
                                   .scroll_y = options.vertical ? scroll->offset : 0,
                                   .scroll_max_x = options.vertical ? 0 : scroll->maximum,
                                   .scroll_max_y = options.vertical ? scroll->maximum : 0});
    }
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
    cui_set_semantic_details(
        ui, container,
        (cui_semantic_details){.orientation = options.vertical ? CUI_ORIENTATION_VERTICAL
                                                               : CUI_ORIENTATION_HORIZONTAL});
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
    cui_set_semantic_details(
        ui, container,
        (cui_semantic_details){.multiselectable = multiple_selected != NULL,
                               .orientation = CUI_ORIENTATION_VERTICAL,
                               .scrollable = options.visible_count && count > options.visible_count,
                               .scroll_y = state->first_visible * theme->control_height,
                               .scroll_max_y =
                                   options.visible_count && count > options.visible_count
                                       ? (count - options.visible_count) * theme->control_height
                                       : 0});
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
        if (options.render)
            options.render(ui, &items[i], i, options.render_user);
        else
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
    options.scroll_bounds = cui_bounds(ui, container);
    options.scroll_target = container;
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
                                                 options.viewport, options.side, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return false;
    cui_list_options list_options = {.disabled = options.disabled,
                                     .menu = menu,
                                     .dt = options.dt,
                                     .visible_begin = state->list.first_visible,
                                     .visible_count = rows,
                                     .render = options.render,
                                     .render_user = options.render_user,
                                     .scroll_bounds = cui_bounds(ui, state->overlay.id),
                                     .scroll_scope = state->overlay.id,
                                     .scroll_target =
                                         state->overlay.id ^ UINT64_C(0xd1bc905ee4216f11)};
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
    cui_set_semantic_details(
        ui, state->overlay.id,
        (cui_semantic_details){.labelled_by = trigger,
                               .orientation = CUI_ORIENTATION_VERTICAL,
                               .scrollable = count > rows,
                               .scroll_y = state->list.first_visible * theme->control_height,
                               .scroll_max_y =
                                   count > rows ? (count - rows) * theme->control_height : 0});
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

static void cui_choice_paint_query(cui_context *ui, cui_id input, const cui_theme *theme,
                                   cui_str name, cui_edit *query, cui_choice_popup *state,
                                   bool disabled, cui_edit_result error) {
    cui_input_state paint_state = {.behavior = state->query_behavior,
                                   .scroll_x = state->query_scroll_x};
    cui_input_paint(ui, input, theme, name, query, &paint_state,
                    (cui_input_options){.disabled = disabled, .invalid = error != CUI_EDIT_OK});
    state->query_scroll_x = paint_state.scroll_x;
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
    state->query_viewport = cui_bounds(ui, cui_input_viewport_id(input));
    state->query_font_size = theme->font_size;
    result = cui_base_combobox(ui, input, name, query, items, count, filtered, capacity, selected,
                               state, options.disabled);
    cui_choice_paint_query(ui, input, theme, name, query, state, options.disabled, result.error);
    cui_str text = cui_edit_get_text(query);
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
    state->visible_rows = rows;
    cui_popup_result placement = cui_popup_place(
        cui_bounds(ui, input),
        (cui_size){width, 2 * theme->padding + (rows ? rows : 1) * theme->control_height},
        options.viewport, options.side, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return result;
    cui_list_options list_options = {.dt = options.dt,
                                     .visible_begin = state->list.first_visible,
                                     .visible_count = rows,
                                     .render = options.render,
                                     .render_user = options.render_user,
                                     .window_changed = result.scrolled};
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
    cui_set_semantic_details(
        ui, state->overlay.id,
        (cui_semantic_details){.labelled_by = input,
                               .orientation = CUI_ORIENTATION_VERTICAL,
                               .scrollable = matched > rows,
                               .scroll_y = state->list.first_visible * theme->control_height,
                               .scroll_max_y =
                                   matched > rows ? (matched - rows) * theme->control_height : 0});
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
    options.scroll_bounds = cui_bounds(ui, container);
    options.scroll_target = container;
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
        options.viewport, options.side, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return changed;
    cui_list_options list_options = {.disabled = options.disabled,
                                     .dt = options.dt,
                                     .visible_begin = state->list.first_visible,
                                     .visible_count = rows,
                                     .render = options.render,
                                     .render_user = options.render_user,
                                     .scroll_bounds = cui_bounds(ui, state->overlay.id),
                                     .scroll_scope = state->overlay.id,
                                     .scroll_target =
                                         state->overlay.id ^ UINT64_C(0xd1bc905ee4216f11)};
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
    cui_set_semantic_details(
        ui, state->overlay.id,
        (cui_semantic_details){.labelled_by = trigger,
                               .multiselectable = true,
                               .orientation = CUI_ORIENTATION_VERTICAL,
                               .scrollable = count > rows,
                               .scroll_y = state->list.first_visible * theme->control_height,
                               .scroll_max_y =
                                   count > rows ? (count - rows) * theme->control_height : 0});
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
    state->query_viewport = cui_bounds(ui, cui_input_viewport_id(input));
    state->query_font_size = theme->font_size;
    result =
        cui_base_combobox_multi(ui, input, name, query, items, count, filtered, filtered_selected,
                                capacity, selected, state, options.disabled);
    cui_choice_paint_query(ui, input, theme, name, query, state, options.disabled, result.error);
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
    state->visible_rows = rows;
    cui_popup_result placement = cui_popup_place(
        cui_bounds(ui, input),
        (cui_size){width, 2 * theme->padding + (rows ? rows : 1) * theme->control_height},
        options.viewport, options.side, 4);
    cui_overlay_set_bounds(&state->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &state->overlay))
        return result;
    cui_list_options list_options = {.dt = options.dt,
                                     .visible_begin = state->list.first_visible,
                                     .visible_count = rows,
                                     .render = options.render,
                                     .render_user = options.render_user,
                                     .window_changed = result.scrolled};
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
    cui_set_semantic_details(
        ui, state->overlay.id,
        (cui_semantic_details){.labelled_by = input,
                               .multiselectable = true,
                               .orientation = CUI_ORIENTATION_VERTICAL,
                               .scrollable = matched > rows,
                               .scroll_y = state->list.first_visible * theme->control_height,
                               .scroll_max_y =
                                   matched > rows ? (matched - rows) * theme->control_height : 0});
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

static void cui_choice_menu_level(cui_context *ui, const cui_theme *theme,
                                  const cui_menu_entry *entries, size_t count,
                                  cui_menu_tree_state *state, size_t level, cui_rect anchor,
                                  cui_menu_tree_options options, cui_str name) {
    cui_list_state *cursor = &state->levels[level];
    float available = options.popup.viewport.h - 2 * theme->padding - 8;
    size_t rows = count;
    if (available / theme->control_height < (float)rows)
        rows = available > theme->control_height ? (size_t)(available / theme->control_height) : 1;
    if (rows > 4096)
        rows = 4096;
    if (cursor->first_visible >= count)
        cursor->first_visible = 0;
    size_t active = count;
    for (size_t i = 0; i < count; i++)
        if (entries[i].id == cursor->active)
            active = i;
    if (active < count && rows) {
        if (active < cursor->first_visible)
            cursor->first_visible = active;
        else if (active >= cursor->first_visible + rows)
            cursor->first_visible = active + 1 - rows;
    }
    float width =
        options.popup.width > 0 && isfinite(options.popup.width) ? options.popup.width : 240;
    if (width > options.popup.viewport.w)
        width = options.popup.viewport.w;
    float height = 2 * theme->padding + (rows ? rows : 1) * theme->control_height;
    cui_popup_result placement = cui_popup_place(
        anchor, (cui_size){width, height}, options.popup.viewport,
        level ? CUI_POPUP_RIGHT : options.popup.side, !level && state->contextual ? 0 : 4);
    if (!level && options.popup.align_end &&
        (options.popup.side == CUI_POPUP_BELOW || options.popup.side == CUI_POPUP_ABOVE)) {
        float x = anchor.x + anchor.w - placement.bounds.w;
        float max_x = options.popup.viewport.x + options.popup.viewport.w - placement.bounds.w;
        if (x > max_x)
            x = max_x;
        if (x < options.popup.viewport.x)
            x = options.popup.viewport.x;
        placement.bounds.x = x;
    }
    state->panels[level] = placement.bounds;
    if (!level)
        cui_overlay_set_bounds(&state->overlay, placement.bounds);
    cui_id scope =
        level ? state->path[level - 1] ^ UINT64_C(0x4b64d23e91a738c1) : state->overlay.id;
    if (!cui_begin_focus_scope(ui, scope, !level))
        return;
    cui_begin_portal(ui, scope,
                     (cui_box){.padding = theme->padding,
                               .clip = true,
                               .background = theme->surface,
                               .border = theme->border,
                               .border_width = 1,
                               .radius = theme->radius},
                     placement.bounds, (int16_t)(100 + level));
    cui_interact(ui, scope, CUI_ROLE_MENU, name, false, false, 0);
    cui_set_semantic_details(ui, scope,
                             (cui_semantic_details){.orientation = CUI_ORIENTATION_VERTICAL,
                                                    .labelled_by = level ? state->path[level - 1]
                                                                         : state->overlay.trigger});
    size_t begin = cursor->first_visible, end = rows > count - begin ? count : begin + rows;
    for (size_t i = begin; i < end; i++) {
        const cui_menu_entry *entry = &entries[i];
        bool expanded = level + 1 < state->depth && state->path[level] == entry->id;
        cui_base_menu_entry(ui, entry, options.popup.disabled, expanded, scope);
        if (entry->kind != CUI_MENU_SEPARATOR && entry->kind != CUI_MENU_LABEL)
            cui_set_tab_stop(ui, entry->id, entry->id == cursor->active);
        bool highlighted = cursor->active == entry->id;
        bool checked = entry->kind == CUI_MENU_CHECK && entry->checked ? *entry->checked
                       : entry->kind == CUI_MENU_RADIO && entry->radio_value
                           ? *entry->radio_value == entry->radio_option
                           : false;
        cui_color foreground = cui_menu_entry_is_disabled(entry) || options.popup.disabled
                                   ? theme->muted
                               : highlighted ? theme->on_primary
                                             : theme->text;
        cui_begin_box(
            ui, entry->id,
            (cui_box){.height = theme->control_height,
                      .width = -1,
                      .padding = theme->padding,
                      .gap = theme->gap,
                      .row = true,
                      .align_y = CUI_ALIGN_CENTER,
                      .background = highlighted ? theme->primary : theme->surface,
                      .border = cui_focused(ui) == entry->id ? theme->focus : theme->surface,
                      .border_width = cui_focused(ui) == entry->id ? 1 : 0});
        if (entry->kind == CUI_MENU_SEPARATOR) {
            cui_begin_box(ui, 0, (cui_box){.height = 1, .width = -1, .background = theme->border});
            cui_end_box(ui);
        } else {
            bool checkable = entry->kind == CUI_MENU_CHECK || entry->kind == CUI_MENU_RADIO;
            const char *marker = checked ? (entry->kind == CUI_MENU_RADIO ? "(*)" : "[x]")
                                 : entry->kind == CUI_MENU_RADIO ? "( )"
                                                                 : "[ ]";
            if (checkable && !options.checks_on_right)
                cui_text(ui, cui_string(marker), theme->font_size, foreground);
            else if (entry->icon.length)
                cui_text(ui, entry->icon, theme->font_size, foreground);
            cui_begin_box(ui, 0, (cui_box){.width = -1});
            if (entry->render)
                entry->render(ui, entry->render_user);
            else
                cui_text(ui, entry->label, theme->font_size,
                         entry->kind == CUI_MENU_LABEL ? theme->muted : foreground);
            cui_end_box(ui);
            if (checkable && options.checks_on_right)
                cui_text(ui, cui_string(marker), theme->font_size, foreground);
            if (entry->shortcut.length)
                cui_text(ui, entry->shortcut, theme->font_size * .8f, foreground);
            if (entry->kind == CUI_MENU_SUBMENU)
                cui_text(ui, cui_string(">"), theme->font_size, foreground);
        }
        cui_end_box(ui);
    }
    if (!count)
        cui_text(ui, cui_string("No actions"), theme->font_size, theme->muted);
    if (level + 1 < state->depth) {
        size_t parent = count;
        for (size_t i = 0; i < count; i++)
            if (entries[i].id == state->path[level])
                parent = i;
        if (parent < count) {
            cui_rect child_anchor = {placement.bounds.x,
                                     placement.bounds.y + theme->padding +
                                         (float)(parent - begin) * theme->control_height,
                                     placement.bounds.w, theme->control_height};
            cui_choice_menu_level(ui, theme, entries[parent].children, entries[parent].child_count,
                                  state, level + 1, child_anchor, options, entries[parent].label);
        }
    }
    cui_end_box(ui);
    cui_end_focus_scope(ui);
}
bool cui_menu_tree(cui_context *ui, cui_id trigger, const cui_theme *theme, cui_str name,
                   const cui_menu_entry *items, size_t count, cui_id *action,
                   cui_menu_tree_state *state, cui_menu_tree_options options) {
    if (!theme || !state || !state->overlay.id || !action || (!items && count) ||
        !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 || !isfinite(options.popup.viewport.x) ||
        !isfinite(options.popup.viewport.y) || !isfinite(options.popup.viewport.w) ||
        !isfinite(options.popup.viewport.h) || options.popup.viewport.w <= 0 ||
        options.popup.viewport.h <= 2 * theme->padding)
        return false;
    bool activated =
        cui_base_menu_tree(ui, trigger, name, items, count, action, state, options.popup.disabled,
                           options.popup.dt, options.keep_open_checks);
    cui_choice_item trigger_item = {trigger, name, options.popup.disabled};
    cui_choice_draw(ui, &trigger_item, theme, false, options.popup.disabled, CUI_TAB_DEFAULT);
    state->row_height = theme->control_height;
    state->padding = theme->padding;
    if (state->overlay.open)
        cui_choice_menu_level(ui, theme, items, count, state, 0, cui_bounds(ui, trigger), options,
                              name);
    return activated;
}

bool cui_context_menu(cui_context *ui, cui_id target, const cui_theme *theme, cui_str name,
                      const cui_menu_entry *items, size_t count, cui_id *action,
                      cui_menu_tree_state *state, cui_menu_tree_options options) {
    if (!theme || !state || !state->overlay.id || !action || (!items && count) ||
        !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 || !isfinite(options.popup.viewport.x) ||
        !isfinite(options.popup.viewport.y) || !isfinite(options.popup.viewport.w) ||
        !isfinite(options.popup.viewport.h) || options.popup.viewport.w <= 0 ||
        options.popup.viewport.h <= 2 * theme->padding)
        return false;
    bool activated =
        cui_base_context_menu(ui, target, items, count, action, state, options.popup.disabled,
                              options.popup.dt, options.keep_open_checks);
    state->row_height = theme->control_height;
    state->padding = theme->padding;
    if (state->overlay.open)
        cui_choice_menu_level(ui, theme, items, count, state, 0, state->anchor, options, name);
    return activated;
}

static cui_color cui_dropdown_intent(const cui_theme *theme, cui_dropdown_button_variant variant) {
    switch (variant) {
    case CUI_DROPDOWN_BUTTON_DANGER:
        return theme->danger;
    case CUI_DROPDOWN_BUTTON_WARNING:
        return theme->warning;
    case CUI_DROPDOWN_BUTTON_SUCCESS:
        return theme->success;
    case CUI_DROPDOWN_BUTTON_INFO:
        return theme->info;
    case CUI_DROPDOWN_BUTTON_SECONDARY:
        return theme->border;
    default:
        return theme->primary;
    }
}
static cui_color cui_dropdown_foreground(const cui_theme *theme,
                                         cui_dropdown_button_options options) {
    if (options.outline)
        return options.variant == CUI_DROPDOWN_BUTTON_SECONDARY ||
                       options.variant == CUI_DROPDOWN_BUTTON_OUTLINE ||
                       options.variant == CUI_DROPDOWN_BUTTON_GHOST
                   ? theme->text
                   : cui_dropdown_intent(theme, options.variant);
    switch (options.variant) {
    case CUI_DROPDOWN_BUTTON_DANGER:
        return theme->on_danger;
    case CUI_DROPDOWN_BUTTON_WARNING:
        return theme->on_warning;
    case CUI_DROPDOWN_BUTTON_SUCCESS:
        return theme->on_success;
    case CUI_DROPDOWN_BUTTON_INFO:
        return theme->on_info;
    case CUI_DROPDOWN_BUTTON_PRIMARY:
        return theme->on_primary;
    default:
        return options.selected ? theme->on_primary : theme->text;
    }
}
static cui_box cui_dropdown_button_box(cui_context *ui, cui_id id, const cui_theme *theme,
                                       cui_dropdown_button_options options, bool trigger,
                                       bool disabled, bool pair_surfaced) {
    float scale = options.size == CUI_DROPDOWN_BUTTON_XSMALL  ? .625f
                  : options.size == CUI_DROPDOWN_BUTTON_SMALL ? .8f
                  : options.size == CUI_DROPDOWN_BUTTON_LARGE ? 1.2f
                                                              : 1;
    cui_box box = {.height = theme->control_height * scale,
                   .padding = theme->padding * scale,
                   .row = true,
                   .align_x = CUI_ALIGN_CENTER,
                   .align_y = CUI_ALIGN_CENTER,
                   .radius = theme->radius,
                   .background = theme->primary,
                   .border = theme->border,
                   .border_width = 1};
    if (!trigger)
        box.width = isfinite(options.width) ? options.width : 0;
    if (options.variant == CUI_DROPDOWN_BUTTON_SECONDARY)
        box.background = theme->border;
    else if (options.variant == CUI_DROPDOWN_BUTTON_OUTLINE ||
             options.variant == CUI_DROPDOWN_BUTTON_GHOST)
        box.background.a = 0;
    else
        box.background = cui_dropdown_intent(theme, options.variant);
    if (options.variant == CUI_DROPDOWN_BUTTON_GHOST) {
        box.border_width = 0;
        if (pair_surfaced && !disabled)
            box.background = theme->surface;
    }
    if (options.selected && options.variant < CUI_DROPDOWN_BUTTON_DANGER)
        box.background = theme->primary;
    bool colored_outline = options.outline && options.variant != CUI_DROPDOWN_BUTTON_GHOST &&
                           options.variant != CUI_DROPDOWN_BUTTON_OUTLINE;
    if (colored_outline) {
        box.background = cui_dropdown_intent(theme, options.variant);
        box.border = box.background;
        box.background.a = options.selected ? 102 : (!disabled && cui_hovered(ui, id)) ? 51 : 26;
    }
    if (!disabled && cui_hovered(ui, id)) {
        if (!box.background.a)
            box.background = theme->surface;
        box.background.r = (uint8_t)(box.background.r * .9f);
        box.background.g = (uint8_t)(box.background.g * .9f);
        box.background.b = (uint8_t)(box.background.b * .9f);
    }
    if (disabled && box.background.a) {
        if (colored_outline)
            box.background.a /= 2;
        else
            box.background = theme->surface;
    }
    if (cui_focused(ui) == id) {
        box.border = theme->focus;
        box.border_width = 2;
    }
    box.custom_corners = true;
    box.corner_radii =
        trigger ? (cui_corner_radii){.top_right = theme->radius, .bottom_right = theme->radius}
                : (cui_corner_radii){.top_left = theme->radius, .bottom_left = theme->radius};
    box.custom_border_edges = true;
    box.border_edges = (cui_border_edges){.left = trigger ? 0 : box.border_width,
                                          .right = box.border_width,
                                          .top = box.border_width,
                                          .bottom = box.border_width};
    return box;
}

cui_dropdown_button_result cui_dropdown_button(cui_context *ui, cui_id container, cui_id action_id,
                                               cui_id trigger_id, const cui_theme *theme,
                                               cui_str action_name, cui_str menu_name,
                                               const cui_menu_entry *items, size_t count,
                                               cui_id *menu_action, cui_menu_tree_state *state,
                                               cui_dropdown_button_options options) {
    cui_dropdown_button_result result = {0};
    if (!ui || !theme || !state || !state->overlay.id || !menu_action || (!items && count) ||
        !container || (options.hide_action && options.hide_menu) ||
        (!options.hide_action && !action_id) || (!options.hide_menu && !trigger_id) ||
        action_id == trigger_id || container == action_id || container == trigger_id ||
        !isfinite(theme->control_height) || theme->control_height <= 0 ||
        !isfinite(theme->padding) || theme->padding < 0 || !isfinite(options.width))
        return result;
    if (options.inherit_action_variant)
        options.variant = options.action_variant;
    if (options.inherit_action_size)
        options.size = options.action_size;
    options.selected = options.selected || (!options.hide_action && options.action_selected);
    result = cui_base_dropdown_button(
        ui, options.hide_action ? 0 : action_id, options.hide_menu ? 0 : trigger_id, action_name,
        menu_name, items, count, menu_action, state,
        options.disabled || options.menu.popup.disabled, options.action_disabled,
        options.action_loading, options.menu.popup.dt, options.menu.keep_open_checks);
    bool all_disabled = options.disabled || options.menu.popup.disabled;
    bool action_disabled = all_disabled || options.action_disabled || options.action_loading;
    bool pair_surfaced = state->overlay.open ||
                         (!options.hide_action && cui_hovered(ui, action_id)) ||
                         (!options.hide_menu && cui_hovered(ui, trigger_id));
    cui_color trigger_foreground = cui_dropdown_foreground(theme, options);
    cui_dropdown_button_options foreground_options = options;
    foreground_options.outline = options.outline || options.action_outline;
    cui_color action_foreground = cui_dropdown_foreground(theme, foreground_options);
    if (action_disabled)
        action_foreground = theme->muted;
    if (all_disabled)
        trigger_foreground = theme->muted;
    float scale = options.size == CUI_DROPDOWN_BUTTON_XSMALL  ? .625f
                  : options.size == CUI_DROPDOWN_BUTTON_SMALL ? .8f
                  : options.size == CUI_DROPDOWN_BUTTON_LARGE ? 1.2f
                                                              : 1;
    cui_begin_box(ui, container, (cui_box){.row = true, .gap = 0});
    if (!options.hide_action) {
        cui_dropdown_button_options action_options = options;
        action_options.outline = options.outline || options.action_outline;
        cui_box action_box = cui_dropdown_button_box(ui, action_id, theme, action_options, false,
                                                     action_disabled, pair_surfaced);
        cui_begin_box(ui, action_id, action_box);
        if (options.action_loading)
            cui_text(ui, cui_string("..."), theme->font_size * scale, action_foreground);
        else {
            if (options.leading.length)
                cui_text(ui, options.leading, theme->font_size * scale, action_foreground);
            if (options.content)
                options.content(ui, options.content_user);
            else
                cui_text(ui, action_name, theme->font_size * scale, action_foreground);
            if (options.trailing.length)
                cui_text(ui, options.trailing, theme->font_size * scale, action_foreground);
        }
        cui_end_box(ui);
    }
    if (!options.hide_menu) {
        cui_box trigger_box = cui_dropdown_button_box(ui, trigger_id, theme, options, true,
                                                      all_disabled, pair_surfaced);
        cui_begin_box(ui, trigger_id, trigger_box);
        cui_text(ui, cui_string("v"), theme->font_size * scale, trigger_foreground);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    state->row_height = theme->control_height;
    state->padding = theme->padding;
    if (options.tooltip_id && options.tooltip_state) {
        cui_hover_options tooltip = options.tooltip_options;
        tooltip.trigger = action_id;
        tooltip.disabled =
            tooltip.disabled || action_disabled || options.hide_action || state->overlay.open;
        cui_tooltip(ui, options.tooltip_id, theme, options.tooltip_state, options.tooltip, tooltip);
    }
    options.menu.popup.side = options.anchor == CUI_DROPDOWN_ANCHOR_BOTTOM_RIGHT ||
                                      options.anchor == CUI_DROPDOWN_ANCHOR_BOTTOM_LEFT
                                  ? CUI_POPUP_ABOVE
                                  : CUI_POPUP_BELOW;
    options.menu.popup.align_end = options.anchor == CUI_DROPDOWN_ANCHOR_TOP_RIGHT ||
                                   options.anchor == CUI_DROPDOWN_ANCHOR_BOTTOM_RIGHT;
    if (state->overlay.open)
        cui_choice_menu_level(ui, theme, items, count, state, 0, cui_bounds(ui, trigger_id),
                              options.menu, menu_name);
    return result;
}
