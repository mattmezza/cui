/* SPDX-License-Identifier: MIT */
#include "cui/cui_numeric.h"
#include "../base/cui_pagination_internal.h"
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include "cui/cui_icon.h"
#include "cui/cui_number_events.h"
#include <math.h>
#include <stdio.h>
static void cui_numeric_paint_button(cui_context *ui, cui_id id, const cui_theme *t, cui_str text,
                                     bool disabled) {
    cui_begin_box(ui, id,
                  (cui_box){.height = t->control_height,
                            .padding = t->padding,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER,
                            .radius = t->radius,
                            .background = disabled ? t->surface : t->primary,
                            .border = cui_focused(ui) == id ? t->focus : t->border,
                            .border_width = 1});
    cui_text(ui, text, t->font_size, disabled ? t->muted : t->on_primary);
    cui_end_box(ui);
}
bool cui_number_measure(cui_number_options options, float root_font_size,
                        cui_number_metrics *metrics) {
    if (!metrics || options.size < CUI_NUMBER_MEDIUM || options.size > CUI_NUMBER_CUSTOM)
        return false;
    cui_input_metrics input;
    if (!cui_input_measure((cui_input_options){.size = (cui_input_size)options.size,
                                               .custom_size = options.custom_size,
                                               .density = options.density,
                                               .design_size = options.design_size},
                           root_font_size, &input))
        return false;
    float d = options.density > 0 ? options.density : root_font_size / 16;
    float width = options.size == CUI_NUMBER_XSMALL || options.size == CUI_NUMBER_SMALL ? 24 : 32;
    float icon = options.size == CUI_NUMBER_XSMALL  ? 12
                 : options.size == CUI_NUMBER_SMALL ? 14
                 : options.size == CUI_NUMBER_LARGE ? 24
                                                    : 16;
    if (options.design_size != CUI_DESIGN_INHERIT) {
        width = input.height / d;
        icon = input.font_size / d;
    } else if (options.size == CUI_NUMBER_CUSTOM) {
        width = options.custom_size / d;
        icon = options.custom_size / d;
    }
    *metrics =
        (cui_number_metrics){input.height,    input.font_size, input.line_height, input.padding_x,
                             input.padding_y, width * d,       icon * d};
    return true;
}
static void cui_number_step_paint(cui_context *ui, cui_id id, const cui_theme *theme,
                                  cui_number_metrics metrics, bool increment, bool disabled,
                                  float height, float radius) {
    cui_color background = {0};
    if (!disabled && (cui_hovered(ui, id) || cui_pressed(ui, id))) {
        background = theme->input;
        background.a = (uint8_t)lroundf(background.a * (cui_pressed(ui, id) ? .6f : .4f));
    }
    cui_begin_box(
        ui, id,
        (cui_box){.width = metrics.step_width,
                  .height = height,
                  .fixed_height = true,
                  .align_x = CUI_ALIGN_CENTER,
                  .align_y = CUI_ALIGN_CENTER,
                  .background = background,
                  .custom_corners = true,
                  .corner_radii =
                      increment ? (cui_corner_radii){.top_right = radius, .bottom_right = radius}
                                : (cui_corner_radii){.top_left = radius, .bottom_left = radius}});
    cui_color color = theme->secondary_foreground;
    cui_icon_named(ui, id ^ UINT64_C(0xf753713882a65049), theme,
                   increment ? cui_string("plus") : cui_string("minus"),
                   (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                      .custom_size = metrics.icon_size,
                                      .custom_color = true,
                                      .color = color});
    cui_end_box(ui);
}
static cui_number_result cui_number_input_common(cui_context *ui, cui_id id, const cui_theme *t,
                                                 cui_str name, cui_number_state *s, double *value,
                                                 cui_number_options o,
                                                 const cui_number_event_handler *handler,
                                                 size_t *change_count) {
    cui_number_metrics metrics;
    if (!ui || !id || !t || !cui_number_measure(o, t->font_size, &metrics) || !isfinite(o.width) ||
        (o.prefix.length && !o.prefix.data) || (o.suffix.length && !o.suffix.data) ||
        (o.placeholder.length && !o.placeholder.data))
        return (cui_number_result){.invalid = true, .error = CUI_EDIT_INVALID_ARGUMENT};
    cui_number_result result;
    if (handler) {
        cui_number_event_result notification =
            cui_base_number_input_events(ui, id, name, s, value, o, *handler);
        result = notification.control;
        *change_count = notification.change_count;
    } else
        result = cui_base_number_input(ui, id, name, s, value, o);
    if (!s || !s->edit || !value || result.error == CUI_EDIT_INVALID_ARGUMENT)
        return result;
    bool editable = !o.disabled && !o.readonly;
    float width = o.width ? o.width : 200, border = o.no_appearance ? 0 : 1;
    float inner = fmaxf(0, metrics.height - 2 * border);
    cui_color frame_border = t->input;
    if (o.disabled)
        frame_border.a = (uint8_t)lroundf(frame_border.a * .5f);
    else if (s->invalid)
        frame_border = t->danger;
    else if (cui_focused(ui) == id && !o.hide_focus_ring)
        frame_border = t->focus;
    cui_color background = t->input_background;
    if (o.disabled) {
        background = t->input;
        background.a = (uint8_t)lroundf(background.a * .2f);
    }
    cui_begin_box(ui, cui_numeric_part_id(id, 0),
                  (cui_box){.width = width,
                            .height = metrics.height,
                            .fixed_height = true,
                            .row = true,
                            .padding = border,
                            .gap = 0,
                            .align_y = CUI_ALIGN_CENTER,
                            .radius = t->radius,
                            .clip = true,
                            .background = o.no_appearance ? (cui_color){0} : background,
                            .border = frame_border,
                            .border_width = border,
                            .custom_opacity = o.disabled,
                            .opacity = .5f});
    float radius = o.no_appearance ? t->radius : fmaxf(0, t->radius - 1);
    cui_number_step_paint(ui, cui_numeric_part_id(id, 1), t, metrics, false, !editable, inner,
                          radius);
    if (o.prefix.length)
        cui_text_with_line_height(ui, o.prefix, metrics.font_size, t->text, metrics.line_height);
    float middle_width =
        width > 0 ? fmaxf(1, width - 2 * border - 2 * metrics.step_width -
                                 cui_measure_text(ui, o.prefix, metrics.font_size).width -
                                 cui_measure_text(ui, o.suffix, metrics.font_size).width)
                  : -1;
    cui_input_state input = {.behavior = {.composition = s->composition}, .scroll_x = s->scroll_x};
    cui_input_paint(ui, id, t, name, s->edit, &input,
                    (cui_input_options){.size = (cui_input_size)o.size,
                                        .design_size = o.design_size,
                                        .custom_size = o.custom_size,
                                        .density = o.density,
                                        .height = inner,
                                        .custom_height = true,
                                        .width = middle_width,
                                        .no_appearance = true,
                                        .no_border = true,
                                        .hide_focus_ring = true,
                                        .disabled = o.disabled,
                                        .readonly = o.readonly,
                                        .invalid = s->invalid,
                                        .placeholder = o.placeholder,
                                        .text_align = CUI_ALIGN_CENTER});
    s->scroll_x = input.scroll_x;
    if (o.suffix.length)
        cui_text_with_line_height(ui, o.suffix, metrics.font_size, t->text, metrics.line_height);
    cui_number_step_paint(ui, cui_numeric_part_id(id, 2), t, metrics, true, !editable, inner,
                          radius);
    cui_end_box(ui);
    return result;
}
cui_number_result cui_number_input(cui_context *ui, cui_id id, const cui_theme *theme, cui_str name,
                                   cui_number_state *state, double *value,
                                   cui_number_options options) {
    return cui_number_input_common(ui, id, theme, name, state, value, options, NULL, NULL);
}
cui_number_event_result cui_number_input_events(cui_context *ui, cui_id id, const cui_theme *theme,
                                                cui_str name, cui_number_state *state,
                                                double *value, cui_number_options options,
                                                cui_number_event_handler handler) {
    cui_number_event_result result = {0};
    result.control = cui_number_input_common(ui, id, theme, name, state, value, options, &handler,
                                             &result.change_count);
    return result;
}
static bool cui_pagination_follow_anchor(const cui_prepaint_info *info, const void *payload,
                                         cui_prepaint_writer *writer) {
    const cui_rect *before = payload;
    cui_rect after = info->bounds;
    if (before->x != after.x || before->y != after.y || before->w != after.w ||
        before->h != after.h)
        cui_prepaint_request_frame(writer);
    return true;
}
typedef struct {
    const cui_theme *theme;
    const cui_design_metrics *metrics;
    float available_width, occupied;
    cui_id popup_trigger;
    bool row_open;
} cui_pagination_layout;
static void cui_pagination_paint_icon(cui_context *ui, cui_id id, const cui_theme *theme,
                                      const char *icon, float size, cui_color color) {
    cui_icon_named(
        ui, cui_numeric_part_id(id, 0), theme, cui_string(icon),
        (cui_icon_options){
            .size = CUI_ICON_CUSTOM, .custom_size = size, .custom_color = true, .color = color});
}
static void cui_pagination_paint_item(cui_context *ui, cui_pagination_layout *layout, cui_id id,
                                      cui_str label, bool disabled, const char *leading_icon,
                                      const char *trailing_icon) {
    const cui_theme *t = layout->theme;
    const cui_design_metrics *m = layout->metrics;
    cui_color text = id ? (disabled ? t->muted : t->on_primary) : (disabled ? t->muted : t->text);
    if (!m) {
        if (id)
            cui_numeric_paint_button(ui, id, t, label, disabled);
        else
            cui_text(ui, label, t->font_size, text);
        return;
    }
    float width = m->control_height;
    float icon_size = fminf(m->indicator_size, m->control_height);
    bool has_icon = leading_icon || trailing_icon;
    if (label.length) {
        width = ceilf(cui_measure_text(ui, label, m->font_size).width);
        if (id)
            width += 2 * ceilf(m->padding_x);
        if (has_icon)
            width += ceilf(icon_size) + ceilf(m->gap);
    }
    if (layout->available_width > 0) {
        float occupied = layout->occupied + ceilf(m->gap) + width;
        if (layout->row_open && occupied > layout->available_width) {
            cui_end_box(ui);
            layout->row_open = false;
        }
        if (!layout->row_open) {
            cui_begin_box(ui, 0,
                          (cui_box){.row = true, .gap = m->gap, .align_y = CUI_ALIGN_CENTER});
            layout->row_open = true;
            occupied = width;
        }
        layout->occupied = occupied;
    }
    if (id)
        cui_begin_box(ui, id,
                      (cui_box){.row = has_icon,
                                .gap = has_icon ? m->gap : 0,
                                .width = width,
                                .height = m->control_height,
                                .fixed_width = true,
                                .fixed_height = true,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER,
                                .radius = t->radius,
                                .background = disabled ? t->surface : t->primary,
                                .border = cui_focused(ui) == id ? t->focus : t->border,
                                .border_width = 1});
    if (id && id == layout->popup_trigger) {
        cui_rect anchor = cui_bounds(ui, id);
        cui_set_prepaint(ui, (cui_prepaint_options){.callback = cui_pagination_follow_anchor,
                                                    .payload = &anchor,
                                                    .payload_bytes = sizeof(anchor),
                                                    .command_budget = 1});
    }
    if (leading_icon)
        cui_pagination_paint_icon(ui, id, t, leading_icon, icon_size, text);
    if (label.length)
        /* Center the font's measured line box inside the fixed control height. */
        cui_text_line(ui, label, m->font_size, text, 0, (cui_text_style){0});
    if (trailing_icon)
        cui_pagination_paint_icon(ui, id, t, trailing_icon, icon_size, text);
    if (id)
        cui_end_box(ui);
}
enum { CUI_PAGINATION_MENU_MAX = 100 };
#define CUI_PAGINATION_MENU_RESOURCE UINT64_C(0xf570525ef9572f4f)
typedef struct {
    size_t first, last;
} cui_pagination_gap;
typedef struct {
    cui_menu_tree_state menu;
    cui_pagination_gap gap;
    size_t total, count, owner_page;
    unsigned side, budget;
    cui_menu_entry entries[CUI_PAGINATION_MENU_MAX];
    char labels[CUI_PAGINATION_MENU_MAX][32];
} cui_pagination_menu;

static void cui_pagination_gaps(size_t page, size_t total, unsigned budget,
                                cui_pagination_gap out[2]) {
    out[0] = out[1] = (cui_pagination_gap){0};
    cui_page_item items[CUI_PAGINATION_MAX_ITEMS];
    size_t count = cui_pagination_visible_items(page, total, budget, items);
    for (size_t i = 1; i + 1 < count; ++i) {
        if (!items[i].ellipsis)
            continue;
        size_t first = items[i - 1].page + 1, last = items[i + 1].page - 1;
        unsigned side = last < page ? 0 : 1;
        out[side] = (cui_pagination_gap){first, last};
    }
}
static cui_id cui_pagination_gap_id(cui_id id, unsigned side) {
    return cui_numeric_part_id(id, 5 + side);
}
static cui_id cui_pagination_popup_id(cui_id id, unsigned side) {
    return cui_numeric_part_id(id, 7 + side);
}
static cui_str cui_pagination_gap_name(const cui_pagination_labels *labels, unsigned side) {
    return side ? labels->later_pages : labels->earlier_pages;
}
static void cui_pagination_open_menu(cui_context *ui, cui_pagination_menu *state, cui_id id,
                                     unsigned side, cui_pagination_gap gap, cui_rect anchor) {
    cui_menu_tree_init(&state->menu, cui_pagination_popup_id(id, side));
    state->side = side;
    state->gap = gap;
    size_t first = gap.first, last = gap.last;
    if (last - first >= CUI_PAGINATION_MENU_MAX) {
        if (side)
            last = first + CUI_PAGINATION_MENU_MAX - 1;
        else
            first = last - CUI_PAGINATION_MENU_MAX + 1;
    }
    state->count = last - first + 1;
    for (size_t i = 0; i < state->count; ++i) {
        snprintf(state->labels[i], sizeof(state->labels[i]), "%zu", first + i);
        state->entries[i] =
            (cui_menu_entry){.id = cui_numeric_part_id(state->menu.overlay.id, 100 + (unsigned)i),
                             .label = cui_string(state->labels[i]),
                             .kind = CUI_MENU_ACTION};
    }
    state->menu.drag_scroll = true;
    cui_context_menu_open(ui, &state->menu, cui_pagination_gap_id(id, side), anchor);
}
static void cui_pagination_menu_activated(void *user, const cui_menu_command *event) {
    *(cui_menu_command *)user = *event;
}
static cui_pagination_menu *cui_pagination_reduce_menu(cui_context *ui, cui_id id, size_t page,
                                                       size_t total, cui_pagination_options options,
                                                       const cui_pagination_labels *labels,
                                                       cui_pagination_jump *jump) {
    if (!id || !cui_frame_sequence(ui))
        return NULL;
    size_t current = !total ? 0 : page > total ? total : page < 1 ? 1 : page;
    cui_pagination_gap gaps[2];
    cui_pagination_gaps(current, total, options.visible_pages, gaps);
    cui_pagination_menu *state = cui_resource_find(ui, id, CUI_PAGINATION_MENU_RESOURCE);
    bool interactive = !options.disabled && cui_frame_input_snapshot_ready(ui);
    if (state && state->menu.overlay.open &&
        (!interactive || page != state->owner_page || total != state->total ||
         options.visible_pages != state->budget))
        cui_overlay_close(ui, &state->menu.overlay);
    if (!interactive || (!gaps[0].first && !gaps[1].first))
        return state;

    size_t n;
    const cui_event *events = cui_frame_events(ui, &n);
    for (size_t e = 0; e < n; ++e) {
        if (cui_event_consumed(ui, e))
            continue;
        bool arrow = events[e].type == CUI_EVENT_KEY_DOWN &&
                     (events[e].key == CUI_KEY_DOWN || events[e].key == CUI_KEY_UP);
        cui_id activation = arrow ? cui_event_target(ui, e) : cui_event_activation(ui, e);
        unsigned side = activation == cui_pagination_gap_id(id, 1);
        if (!gaps[side].first || activation != cui_pagination_gap_id(id, side))
            continue;
        if (state && state->menu.overlay.open) {
            if (!arrow && side == state->side) {
                cui_overlay_close(ui, &state->menu.overlay);
                cui_consume_event(ui, e);
            }
            continue;
        }
        cui_rect anchor = cui_bounds(ui, activation);
        if (anchor.w <= 0 || anchor.h <= 0)
            continue;
        if (!state)
            state =
                cui_resource_get(ui, id, CUI_PAGINATION_MENU_RESOURCE, sizeof(*state), NULL, NULL);
        if (!state)
            return NULL;
        cui_pagination_open_menu(ui, state, id, side, gaps[side], anchor);
        state->owner_page = page;
        state->total = total;
        state->budget = options.visible_pages;
        cui_consume_event(ui, e);
        /* The Menu detects Up from the trigger and focuses the last row. */
        break;
    }
    if (state && state->menu.overlay.open) {
        cui_id action = 0;
        cui_menu_command command = {0};
        cui_base_menu_tree_events(
            ui, cui_pagination_gap_id(id, state->side),
            cui_pagination_gap_name(labels, state->side), state->entries, state->count, &action,
            &state->menu, false, cui_frame_delta(ui), false,
            (cui_menu_command_handler){cui_pagination_menu_activated, &command});
        if (action && command.command == action &&
            (command.event.flags & CUI_COMPONENT_EVENT_HAS_INDEX)) {
            size_t first = state->side ? state->gap.first : state->gap.last - (state->count - 1);
            for (size_t i = 0; i < state->count; ++i)
                if (state->entries[i].id == action)
                    *jump = (cui_pagination_jump){first + i, command.event.event_index};
        }
    }
    return state;
}
static void cui_pagination_paint_menu(cui_context *ui, cui_id id, const cui_theme *theme,
                                      cui_pagination_menu *state, cui_rect viewport,
                                      const cui_pagination_labels *labels) {
    if (!state || !state->menu.overlay.open)
        return;
    cui_id trigger = cui_pagination_gap_id(id, state->side);
    cui_rect anchor = cui_bounds(ui, trigger);
    float density = theme->font_size / 16;
    cui_menu_metrics metrics = cui_menu_size_metrics(CUI_MENU_SMALL, density, theme->radius);
    cui_size label =
        cui_measure_text(ui, state->entries[state->count - 1].label, metrics.font_size);
    cui_menu_popup_paint(
        ui, theme, cui_pagination_gap_name(labels, state->side), state->entries, state->count,
        &state->menu, anchor,
        (cui_menu_tree_options){.popup = {.viewport = viewport,
                                          .side = CUI_POPUP_BELOW,
                                          .width = fmaxf(88, label.width + 2 * metrics.padding_x +
                                                                 2 * metrics.padding + 12)},
                                .size = CUI_MENU_SMALL,
                                .max_height = 240});
}
typedef struct {
    cui_id trigger;
    cui_hover_state hover;
} cui_pagination_tooltip_state;

static bool cui_pagination_tooltip_input(cui_context *ui, cui_id trigger, bool *moved) {
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = count; i > 0; --i) {
        const cui_event *event = &events[i - 1];
        switch (event->type) {
        case CUI_EVENT_POINTER_MOVE:
            *moved = (event->pointer_type == CUI_POINTER_MOUSE ||
                      event->pointer_type == CUI_POINTER_PEN) &&
                     cui_event_hovered(ui, i - 1, trigger);
            return *moved;
        case CUI_EVENT_POINTER_DOWN:
        case CUI_EVENT_POINTER_UP:
        case CUI_EVENT_POINTER_LEAVE:
        case CUI_EVENT_CANCEL:
        case CUI_EVENT_SCROLL:
        case CUI_EVENT_GESTURE:
        case CUI_EVENT_KEY_DOWN:
        case CUI_EVENT_ACCESSIBILITY_ACTION:
            return false;
        default:
            break;
        }
    }
    return true;
}

static void cui_pagination_paint_tooltip(cui_context *ui, cui_id id, const cui_theme *theme,
                                         size_t page, size_t total, cui_rect viewport,
                                         const cui_pagination_labels *labels) {
    if (!isfinite(viewport.x) || !isfinite(viewport.y) || !isfinite(viewport.w) ||
        !isfinite(viewport.h) || viewport.w <= 0 || viewport.h <= 0 ||
        !cui_frame_input_snapshot_ready(ui))
        return;
    cui_id previous = cui_numeric_part_id(id, 2), next = cui_numeric_part_id(id, 3);
    bool previous_hovered = page > 1 && cui_hovered(ui, previous);
    bool next_hovered = page < total && cui_hovered(ui, next);
    if (!previous_hovered && !next_hovered)
        return;
    cui_id trigger = previous_hovered ? previous : next;
    bool moved = false;
    if (cui_pressed(ui, trigger) || !cui_pagination_tooltip_input(ui, trigger, &moved))
        return;

    const cui_id resource = UINT64_C(0x697b02c76a514cf1);
    cui_pagination_tooltip_state *state = cui_resource_find(ui, id, resource);
    if (!state && moved)
        state = cui_resource_get(ui, id, resource, sizeof(*state), NULL, NULL);
    if (!state)
        return;
    bool entered = state->trigger != trigger;
    if (entered) {
        if (!moved) {
            *state = (cui_pagination_tooltip_state){0};
            return;
        }
        *state = (cui_pagination_tooltip_state){.trigger = trigger};
    }
    cui_str text = previous_hovered ? labels->previous : labels->next;
    float width = cui_measure_text(ui, text, theme->font_size).width + 2 * theme->padding;
    cui_tooltip(ui, cui_numeric_part_id(id, previous_hovered ? 7 : 8), theme, &state->hover, text,
                (cui_hover_options){.trigger = trigger,
                                    .viewport = viewport,
                                    .side = CUI_POPUP_BELOW,
                                    .width = width,
                                    .gap = theme->gap,
                                    .dt = entered ? 0 : cui_frame_delta(ui),
                                    .open_delay = .4f});
}

static bool cui_pagination_common(cui_context *ui, cui_id id, const cui_theme *t, size_t *page,
                                  size_t total, cui_pagination_options options, bool legacy,
                                  const cui_pagination_event_handler *handler,
                                  size_t *change_count) {
    if (!t || !page || !isfinite(options.available_width) || options.available_width < 0)
        return false;
    cui_design_metrics metrics = {.font_size = t->font_size,
                                  .control_height = t->control_height,
                                  .padding_x = t->padding,
                                  .gap = t->gap,
                                  .indicator_size = t->font_size};
    if (options.design_size != CUI_DESIGN_INHERIT &&
        !cui_design_measure(options.design_size, t->font_size / 16, &metrics))
        return false;
    bool icon = options.icon_only, disabled = options.disabled;
    bool ends = cui_pagination_has_end_buttons(options);
    bool chevrons = icon || options.navigation == CUI_PAGINATION_NAV_CHEVRONS;
    cui_pagination_labels labels = cui_pagination_resolve_labels(options.labels);
    cui_rect viewport = options.ellipsis_viewport;
    bool menus = !icon && !options.compact && (viewport.w != 0 || viewport.h != 0);
    if (menus && (!isfinite(viewport.x) || !isfinite(viewport.y) || !isfinite(viewport.w) ||
                  !isfinite(viewport.h) || viewport.w <= 0 || viewport.h <= 8))
        return false;
    cui_pagination_jump jump = {0};
    cui_pagination_menu *menu =
        menus ? cui_pagination_reduce_menu(ui, id, *page, total, options, &labels, &jump) : NULL;
    bool changed;
    if (handler) {
        cui_numeric_selection_event_result result =
            cui_base_pagination_observe_internal(ui, id, page, total, options, jump, *handler);
        changed = result.control;
        *change_count = result.change_count;
    } else
        changed = cui_base_pagination_reduce_internal(ui, id, page, total, options, jump);
    cui_pagination_gap gaps[2] = {0};
    if (menus)
        cui_pagination_gaps(*page, total, options.visible_pages, gaps);
    if (menu) {
        if (*page != menu->owner_page && menu->menu.overlay.open)
            cui_overlay_close(ui, &menu->menu.overlay);
        if (!gaps[menu->side].first && cui_focused(ui) == cui_pagination_gap_id(id, menu->side))
            cui_focus_restore(ui, cui_numeric_part_id(id, menu->side ? 3 : 2));
    }
    bool wrap = options.available_width > 0;
    cui_pagination_layout layout = {.theme = t,
                                    .metrics = legacy ? NULL : &metrics,
                                    .available_width = options.available_width,
                                    .popup_trigger = menu && menu->menu.overlay.open
                                                         ? cui_pagination_gap_id(id, menu->side)
                                                         : 0};
    cui_begin_box(ui, id, (cui_box){.row = !wrap, .gap = metrics.gap, .align_y = CUI_ALIGN_CENTER});
    if (ends)
        cui_pagination_paint_item(ui, &layout, cui_numeric_part_id(id, 1), labels.first,
                                  disabled || *page <= 1, NULL, NULL);
    cui_pagination_paint_item(ui, &layout, cui_numeric_part_id(id, 2),
                              icon ? (cui_str){0} : labels.previous, disabled || *page <= 1,
                              chevrons ? "chevron-left" : NULL, NULL);
    if (!icon && options.compact) {
        char label[64];
        snprintf(label, sizeof(label), "%zu / %zu", *page, total);
        cui_pagination_paint_item(ui, &layout, 0, cui_string(label), false, NULL, NULL);
    } else if (!icon) {
        cui_page_item items[CUI_PAGINATION_MAX_ITEMS];
        size_t n = cui_pagination_visible_items(*page, total, options.visible_pages, items);
        for (size_t i = 0; i < n; ++i) {
            if (items[i].ellipsis && menus) {
                unsigned side = items[i + 1].page > *page;
                cui_id trigger = cui_pagination_gap_id(id, side);
                bool open = menu && menu->side == side && menu->menu.overlay.open;
                cui_interact(ui, trigger, CUI_ROLE_BUTTON, cui_pagination_gap_name(&labels, side),
                             disabled, open, 0);
                cui_set_semantic_details(
                    ui, trigger,
                    (cui_semantic_details){.parent = id,
                                           .expanded = open,
                                           .controls = cui_pagination_popup_id(id, side),
                                           .cursor = CUI_CURSOR_POINTER});
                cui_pagination_paint_item(ui, &layout, trigger, (cui_str){0}, disabled, "ellipsis",
                                          NULL);
            } else if (items[i].ellipsis)
                cui_pagination_paint_item(ui, &layout, 0, cui_string("..."), true, NULL, NULL);
            else {
                char label[32];
                snprintf(label, sizeof(label), "%zu", items[i].page);
                cui_pagination_paint_item(ui, &layout, cui_numeric_part_id(id, 10 + (unsigned)i),
                                          cui_string(label), disabled || items[i].page == *page,
                                          NULL, NULL);
            }
        }
    }
    cui_pagination_paint_item(
        ui, &layout, cui_numeric_part_id(id, 3), icon ? (cui_str){0} : labels.next,
        disabled || !total || *page >= total, NULL, chevrons ? "chevron-right" : NULL);
    if (ends)
        cui_pagination_paint_item(ui, &layout, cui_numeric_part_id(id, 4), labels.last,
                                  disabled || !total || *page >= total, NULL, NULL);
    if (layout.row_open)
        cui_end_box(ui);
    cui_end_box(ui);
    cui_pagination_paint_menu(ui, id, t, menu, viewport, &labels);
    if (chevrons && !disabled && (!menu || !menu->menu.overlay.open))
        cui_pagination_paint_tooltip(ui, id, t, *page, total, options.tooltip_viewport, &labels);
    return changed;
}
bool cui_pagination(cui_context *ui, cui_id id, const cui_theme *theme, size_t *page, size_t total,
                    bool compact, bool disabled) {
    return cui_pagination_common(ui, id, theme, page, total,
                                 (cui_pagination_options){.compact = compact, .disabled = disabled},
                                 true, NULL, NULL);
}
bool cui_pagination_ex(cui_context *ui, cui_id id, const cui_theme *theme, size_t *page,
                       size_t total, cui_pagination_options options) {
    return cui_pagination_common(ui, id, theme, page, total, options, false, NULL, NULL);
}
cui_numeric_selection_event_result cui_pagination_events(cui_context *ui, cui_id id,
                                                         const cui_theme *theme, size_t *page,
                                                         size_t total, bool compact, bool disabled,
                                                         cui_pagination_event_handler handler) {
    cui_numeric_selection_event_result result = {0};
    result.control =
        cui_pagination_common(ui, id, theme, page, total,
                              (cui_pagination_options){.compact = compact, .disabled = disabled},
                              true, &handler, &result.change_count);
    return result;
}
cui_numeric_selection_event_result cui_pagination_events_ex(cui_context *ui, cui_id id,
                                                            const cui_theme *theme, size_t *page,
                                                            size_t total,
                                                            cui_pagination_options options,
                                                            cui_pagination_event_handler handler) {
    cui_numeric_selection_event_result result = {0};
    result.control = cui_pagination_common(ui, id, theme, page, total, options, false, &handler,
                                           &result.change_count);
    return result;
}
typedef struct {
    cui_icon_state outline, fill;
    bool initialized;
} cui_rating_icons;
static void cui_rating_icons_collect(void *payload) {
    cui_rating_icons *icons = payload;
    cui_icon_state_collect(&icons->outline);
    cui_icon_state_collect(&icons->fill);
}
static void cui_rating_icons_destroy(void *payload) {
    cui_rating_icons *icons = payload;
    cui_icon_state_destroy(&icons->outline);
    cui_icon_state_destroy(&icons->fill);
}
static cui_rating_icons *cui_rating_icons_get(cui_context *ui, cui_id id, float size) {
    cui_rating_icons *icons = cui_resource_get(ui, id, UINT64_C(0x929f4a4758cf0103), sizeof(*icons),
                                               cui_rating_icons_collect, cui_rating_icons_destroy);
    if (!icons)
        return NULL;
    if (!icons->initialized) {
        cui_icon_state_init(&icons->outline);
        cui_icon_state_init(&icons->fill);
        icons->initialized = true;
        const char *outline = "<svg width='24' height='24' viewBox='0 0 24 24' "
                              "xmlns='http://www.w3.org/2000/svg'><path d='M12 2 L15 8.2 L22 9.2 "
                              "L17 14 L18.2 21 L12 17.7 L5.8 21 L7 14 L2 9.2 L9 8.2 Z' fill='none' "
                              "stroke='white' stroke-width='1.6' stroke-linejoin='round'/></svg>";
        const char *fill =
            "<svg width='24' height='24' viewBox='0 0 24 24' "
            "xmlns='http://www.w3.org/2000/svg'><path d='M12 2 L15 8.2 L22 9.2 L17 14 L18.2 21 L12 "
            "17.7 L5.8 21 L7 14 L2 9.2 L9 8.2 Z' fill='white'/></svg>";
        if (!cui_icon_set_data(&icons->outline, cui_string(outline)) ||
            !cui_icon_set_data(&icons->fill, cui_string(fill)))
            return NULL;
    }
    if (cui_icon_prepare(&icons->outline, size, size, cui_icon_identity()) != CUI_ICON_OK ||
        cui_icon_prepare(&icons->fill, size, size, cui_icon_identity()) != CUI_ICON_OK)
        return NULL;
    return icons;
}
static bool cui_rating_common(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                              float *value, cui_rating_options o,
                              const cui_rating_event_handler *handler, size_t *change_count) {
    if (!t || !value)
        return false;
    bool changed;
    if (handler) {
        cui_numeric_selection_event_result result =
            cui_base_rating_events(ui, id, name, value, o, *handler);
        changed = result.control;
        *change_count = result.change_count;
    } else
        changed = cui_base_rating(ui, id, name, value, o);
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
    float side = fminf(slot, fminf(t->control_height, t->font_size * 1.3f));
    cui_rating_icons *icons = side > 0 ? cui_rating_icons_get(ui, id, side) : NULL;
    cui_color color = o.disabled ? t->muted : t->primary;
    for (unsigned i = 0; i < maximum; ++i) {
        cui_begin_box(ui, cui_numeric_part_id(id, 10 + i),
                      (cui_box){.width = slot,
                                .height = t->control_height,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_begin_box(ui, 0, (cui_box){.width = side, .height = side});
        if (icons) {
            cui_bitmap outline = cui_icon_state_bitmap(&icons->outline);
            cui_bitmap fill = cui_icon_state_bitmap(&icons->fill);
            cui_set_bitmap_underlay(
                ui, (cui_underlay_layout){.bounds = {0, 0, side, side}},
                (cui_bitmap_paint){
                    .bitmap = outline, .opacity = 1, .custom_tint = true, .tint = color},
                (cui_corner_radii){0});
            float fraction = fmaxf(0, fminf(1, *value - (float)i));
            if (fraction > 0) {
                cui_begin_absolute_box(ui, 0, (cui_box){0},
                                       (cui_rect){0, 0, side * fraction, side});
                cui_paint_bitmap(
                    ui, 0,
                    (cui_bitmap_paint){.bitmap = fill,
                                       .source = {0, 0, fill.width * fraction, fill.height},
                                       .opacity = 1,
                                       .custom_tint = true,
                                       .tint = color},
                    (cui_box){.width = side * fraction, .height = side});
                cui_end_box(ui);
            }
        }
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (o.clearable)
        cui_numeric_paint_button(ui, cui_numeric_part_id(id, 1), t, cui_string("Clear"),
                                 o.disabled || o.readonly);
    cui_end_box(ui);
    return changed;
}
bool cui_rating(cui_context *ui, cui_id id, const cui_theme *theme, cui_str name, float *value,
                cui_rating_options options) {
    return cui_rating_common(ui, id, theme, name, value, options, NULL, NULL);
}
cui_numeric_selection_event_result cui_rating_events(cui_context *ui, cui_id id,
                                                     const cui_theme *theme, cui_str name,
                                                     float *value, cui_rating_options options,
                                                     cui_rating_event_handler handler) {
    cui_numeric_selection_event_result result = {0};
    result.control =
        cui_rating_common(ui, id, theme, name, value, options, &handler, &result.change_count);
    return result;
}
/* Private shared paint seam; not part of the public API. */
void cui_numeric_stepper_paint(cui_context *, cui_id, const cui_theme *, const cui_step *, size_t,
                               size_t, bool, bool);
void cui_numeric_stepper_paint(cui_context *ui, cui_id id, const cui_theme *t,
                               const cui_step *steps, size_t count, size_t current, bool vertical,
                               bool disabled) {
    cui_begin_box(ui, id, (cui_box){.row = !vertical, .gap = t->gap});
    for (size_t i = 0; i < count; ++i) {
        bool selected = i == current;
        const char *status = selected                               ? "Current"
                             : steps[i].status == CUI_STEP_ERROR    ? "Error"
                             : steps[i].status == CUI_STEP_COMPLETE ? "Complete"
                                                                    : "Pending";
        cui_begin_box(ui, cui_numeric_part_id(id, 100 + (unsigned)i), (cui_box){.gap = t->gap});
        cui_button_paint(
            ui, cui_numeric_part_id(id, 1 + (unsigned)i), t, steps[i].title,
            (cui_button_options){.variant = selected ? CUI_BUTTON_PRIMARY : CUI_BUTTON_OUTLINE,
                                 .disabled = disabled || steps[i].disabled,
                                 .selected = selected});
        cui_text(ui, cui_string(status), t->font_size * .8f,
                 steps[i].status == CUI_STEP_ERROR ? t->danger : t->muted);
        if (steps[i].description.length)
            cui_text(ui, steps[i].description, t->font_size * .8f, t->muted);
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
static bool cui_stepper_common(cui_context *ui, cui_id id, const cui_theme *t,
                               const cui_step *steps, size_t count, size_t *current, bool vertical,
                               bool disabled, const cui_stepper_event_handler *handler,
                               size_t *change_count) {
    if (!t || !current || (!steps && count) || count > 100)
        return false;
    bool changed;
    if (handler) {
        cui_numeric_selection_event_result result =
            cui_base_stepper_events(ui, id, steps, count, current, disabled, *handler);
        changed = result.control;
        *change_count = result.change_count;
    } else
        changed = cui_base_stepper(ui, id, steps, count, current, disabled);
    cui_numeric_stepper_paint(ui, id, t, steps, count, *current, vertical, disabled);
    return changed;
}
bool cui_stepper(cui_context *ui, cui_id id, const cui_theme *theme, const cui_step *steps,
                 size_t count, size_t *current, bool vertical, bool disabled) {
    return cui_stepper_common(ui, id, theme, steps, count, current, vertical, disabled, NULL, NULL);
}
cui_numeric_selection_event_result cui_stepper_events(cui_context *ui, cui_id id,
                                                      const cui_theme *theme, const cui_step *steps,
                                                      size_t count, size_t *current, bool vertical,
                                                      bool disabled,
                                                      cui_stepper_event_handler handler) {
    cui_numeric_selection_event_result result = {0};
    result.control = cui_stepper_common(ui, id, theme, steps, count, current, vertical, disabled,
                                        &handler, &result.change_count);
    return result;
}
