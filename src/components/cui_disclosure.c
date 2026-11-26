/* SPDX-License-Identifier: MIT */
#include "cui/cui_disclosure_events.h"
#include "cui/cui_icon.h"
#include <math.h>

typedef struct {
    cui_disclosure_presentation options;
    cui_design_metrics metrics;
    bool fill_width;
} cui_disclosure_paint;

static bool cui_disclosure_resolve(const cui_theme *theme, cui_disclosure_presentation p,
                                   cui_disclosure_paint *paint) {
    if (!theme || !isfinite(p.width) || (p.width < 0 && p.width != -1) ||
        (p.trigger_style_count && !p.trigger_styles))
        return false;
    cui_design_metrics m = {.font_size = theme->font_size,
                            .control_height = theme->control_height,
                            .gap = theme->gap,
                            .padding_x = theme->padding,
                            .padding_y = theme->padding};
    if (p.design_size && !cui_design_measure(p.design_size, theme->font_size / 16, &m))
        return false;
    const cui_box *box = p.container_box;
    *paint = (cui_disclosure_paint){
        .options = p,
        .metrics = m,
        .fill_width =
            p.width != 0 || (box && (box->width != 0 || box->fixed_width || box->width_percent))};
    return true;
}

static cui_box cui_disclosure_layout(const cui_theme *theme, const cui_disclosure_paint *paint) {
    if (!paint)
        return (cui_box){.padding = theme->padding, .gap = theme->gap};
    return (cui_box){.width = paint->fill_width       ? 1
                              : paint->options.joined ? -1
                                                      : 0,
                     .width_percent = paint->fill_width,
                     .gap = paint->metrics.gap,
                     .custom_padding = true,
                     .padding_edges = {paint->metrics.padding_x, paint->metrics.padding_x,
                                       paint->metrics.padding_y, paint->metrics.padding_y}};
}

static cui_box cui_disclosure_container(const cui_theme *theme, const cui_disclosure_paint *p,
                                        bool bordered) {
    if (!p)
        return (cui_box){.gap = theme->gap};
    cui_box box = {.gap = p->metrics.gap};
    if (p->options.joined) {
        float radius = bordered ? theme->radius_lg : 0;
        /* Keep rectangular descendants inside the rounded card corners. */
        box = (cui_box){.background = theme->surface,
                        .border = theme->border,
                        .border_width = bordered ? 1 : 0,
                        .radius = radius,
                        .padding = bordered ? fmaxf(1, ceilf(radius * .3f)) : 0,
                        .clip = true};
    }
    if (p->options.container_box)
        box = *p->options.container_box;
    if (p->options.width != 0) {
        box.width = p->options.width;
        box.width_percent = false;
        box.fixed_width = false;
    }
    box.row = false;
    return box;
}

static const cui_disclosure_trigger_style *cui_disclosure_style(const cui_disclosure_paint *p,
                                                                size_t index) {
    return p && index < p->options.trigger_style_count ? &p->options.trigger_styles[index] : NULL;
}

static void cui_disclosure_set_cursor(cui_context *ui, cui_id id, bool disabled,
                                      const cui_disclosure_paint *paint, size_t index) {
    const cui_disclosure_trigger_style *style = cui_disclosure_style(paint, index);
    if (style && style->custom_cursor && !disabled)
        (void)cui_set_cursor(ui, id, style->cursor);
}

typedef struct {
    cui_disclosure_trigger_style style;
    cui_color normal;
    cui_corner_radii corners;
    bool captured;
} cui_disclosure_trigger_paint;

static cui_corner_radii cui_disclosure_corners(cui_box box) {
    cui_corner_radii corners =
        box.custom_corners ? box.corner_radii
                           : (cui_corner_radii){box.radius, box.radius, box.radius, box.radius};
    float *values[] = {&corners.top_left, &corners.top_right, &corners.bottom_left,
                       &corners.bottom_right};
    for (size_t i = 0; i < 4; ++i)
        *values[i] = isfinite(*values[i]) ? fmaxf(0, *values[i]) : 0;
    return corners;
}

static bool cui_disclosure_trigger_prepaint(const cui_prepaint_info *info, const void *data,
                                            cui_prepaint_writer *writer) {
    const cui_disclosure_trigger_paint *p = data;
    bool pressed = info->modal_allowed && !info->inert && p->captured &&
                   (info->capture != info->owner || info->pointer_visible);
    cui_color color = p->normal;
    if (pressed && p->style.custom_pressed)
        color = p->style.pressed;
    else if (info->pointer_visible && p->style.custom_hover)
        color = p->style.hover;
    if (!color.a)
        return true;
    cui_draw_command fill = {.type = CUI_DRAW_RECT,
                             .bounds = info->bounds,
                             .color = color,
                             .corner_radii = p->corners,
                             .custom_corners = true};
    return cui_prepaint_emit(writer, &fill, false);
}

static void cui_disclosure_title(cui_context *ui, cui_id id, const cui_theme *theme, cui_str label,
                                 bool open, bool disabled, bool bounded,
                                 const cui_disclosure_paint *paint) {
    cui_color color = disabled ? theme->muted : theme->text;
    cui_id indicator = id ^ UINT64_C(0x52fc0dc74f3d9a22);
    if (paint) {
        if (bounded)
            cui_begin_box(ui, 0, (cui_box){.width = -1, .clip = true});
        cui_text_line(ui, label, paint->metrics.font_size, color, paint->metrics.line_height,
                      (cui_text_style){0});
        if (bounded)
            cui_end_box(ui);
        cui_str name = open ? paint->options.expanded_icon : paint->options.collapsed_icon;
        if (!name.length)
            name = cui_string(open ? "chevron-up" : "chevron-down");
        (void)cui_icon_named(ui, indicator, theme, name,
                             (cui_icon_options){.size = paint->metrics.indicator_size
                                                            ? CUI_ICON_CUSTOM
                                                            : CUI_ICON_XSMALL,
                                                .custom_size = paint->metrics.indicator_size,
                                                .custom_color = true,
                                                .color = theme->muted});
        return;
    }
    cui_text(ui, label, theme->font_size, color);
    if (!disabled) {
        cui_begin_box(ui, id ^ UINT64_C(0x52fc0dc74f3d9a21), (cui_box){.width = -1});
        cui_end_box(ui);
        (void)cui_icon_named(
            ui, indicator, theme, cui_string("chevron-down"),
            (cui_icon_options){.size = CUI_ICON_XSMALL,
                               .custom_color = true,
                               .color = theme->muted,
                               .custom_transform = true,
                               .transform = open ? (cui_icon_transform){-1, 0, 0, -1, 0, 0}
                                                 : cui_icon_identity()});
    }
}

static void cui_disclosure_trigger(cui_context *ui, cui_id id, const cui_theme *theme,
                                   cui_str label, bool open, bool disabled, bool bordered,
                                   const cui_disclosure_item *item, size_t index,
                                   cui_disclosure_header_fn render, void *user,
                                   const cui_box *style, const cui_disclosure_paint *paint) {
    cui_box box = cui_disclosure_layout(theme, paint);
    box.height = paint ? paint->metrics.control_height : theme->control_height;
    box.row = true;
    box.align_y = CUI_ALIGN_CENTER;
    bool joined = paint && paint->options.joined;
    box.radius = joined ? 0 : theme->radius;
    if (!joined)
        box.background = theme->surface;
    box.border = theme->border;
    box.border_width = bordered && !joined ? 1 : 0;
    if (style)
        box = *style;
    const cui_disclosure_trigger_style *state = cui_disclosure_style(paint, index);
    if (cui_focused(ui) == id) {
        box.border = state && state->custom_focus ? state->focus : theme->focus;
        box.border_width = 2;
    }
    if (state) {
        if (disabled && state->custom_disabled)
            box.background = state->disabled;
        else if (open && state->custom_expanded)
            box.background = state->expanded;
    }
    cui_color normal = box.background;
    bool resolve_state = state && !disabled && (state->custom_hover || state->custom_pressed) &&
                         cui_frame_paint_enabled(ui);
    if (resolve_state)
        box.background = (cui_color){0};
    cui_begin_box(ui, id, box);
    if (resolve_state) {
        cui_disclosure_trigger_paint payload = {.style = *state,
                                                .normal = normal,
                                                .corners = cui_disclosure_corners(box),
                                                .captured = cui_pressed(ui, id)};
        (void)cui_set_prepaint(ui,
                               (cui_prepaint_options){.callback = cui_disclosure_trigger_prepaint,
                                                      .payload = &payload,
                                                      .payload_bytes = sizeof payload,
                                                      .command_budget = 3});
    }
    if (render) {
        cui_disclosure_item effective = *item;
        effective.disabled = disabled;
        render(ui, &effective, index, open, user);
    } else {
        bool bounded = style ? box.width != 0 || box.fixed_width || box.width_percent
                             : paint && paint->fill_width;
        cui_disclosure_title(ui, id, theme, label, open, disabled, bounded, paint);
    }
    cui_end_box(ui);
}
static void cui_disclosure_panel(cui_context *ui, cui_id trigger, cui_id parent, cui_str label,
                                 bool open, cui_box box, cui_disclosure_motion *motion, float dt,
                                 float response, cui_spring_settings settings, bool reduced_motion,
                                 size_t index, cui_disclosure_panel_fn panel, void *user,
                                 bool fill_width) {
    if (!panel || (!open && !motion))
        return;
    cui_id panel_id = trigger ^ UINT64_C(0x1d8f322539deca61);
    if (motion) {
        if (motion->id != trigger) {
            *motion = (cui_disclosure_motion){.id = trigger};
        }
        if (isfinite(response) && response > 0)
            settings.response = response;
        cui_disclosure_motion_step_ex(motion, open, dt, settings, reduced_motion);
        float height = isfinite(motion->measured.h) ? fmaxf(0, motion->measured.h) : 0;
        cui_begin_box(ui, trigger ^ UINT64_C(0x48aaf2a21d3ed09f),
                      (cui_box){.width = fill_width ? -1 : 0,
                                .height = height * fminf(1, fmaxf(0, motion->progress)),
                                .fixed_height = true,
                                .clip = true,
                                .layout_only = motion->progress == 0,
                                .inert = !open});
        if (motion->progress != (open ? 1 : 0))
            cui_invalidate(ui);
    }
    cui_begin_box(ui, panel_id, box);
    cui_interact(ui, panel_id, CUI_ROLE_GROUP, label, false, false, 0);
    cui_set_semantic_details(ui, panel_id,
                             (cui_semantic_details){.labelled_by = trigger, .parent = parent});
    if (motion)
        cui_track_bounds(ui, panel_id, &motion->measured);
    panel(ui, index, user);
    cui_end_box(ui);
    if (motion)
        cui_end_box(ui);
}
static cui_disclosure_event_result
cui_collapsible_styled_common(cui_context *ui, cui_id id, const cui_theme *theme, cui_str label,
                              bool *open, cui_collapsible_options options,
                              cui_disclosure_panel_fn panel, void *user,
                              cui_disclosure_handler handler, const cui_disclosure_paint *paint) {
    if (!theme || !open)
        return (cui_disclosure_event_result){0};
    cui_disclosure_event_result result = {0};
    if (handler.on_change)
        result = cui_base_collapsible_events(ui, id, label, open, options.disabled, handler);
    else
        result.changed = cui_base_collapsible(ui, id, label, open, options.disabled);
    cui_begin_box(ui, id ^ UINT64_C(0xb7c1c9d453922783),
                  cui_disclosure_container(theme, paint, !options.borderless));
    cui_disclosure_item item = {.id = id, .label = label, .disabled = options.disabled};
    cui_disclosure_trigger(ui, id, theme, label, *open, options.disabled, !options.borderless,
                           &item, 0, options.render_header, options.header_user,
                           options.trigger_box, paint);
    cui_disclosure_set_cursor(ui, id, options.disabled, paint, 0);
    cui_box panel_box =
        options.panel_box ? *options.panel_box : cui_disclosure_layout(theme, paint);
    cui_disclosure_panel(ui, id, 0, label, *open, panel_box, options.motion, options.dt,
                         options.response, theme->spring_control, options.reduced_motion, 0, panel,
                         user, paint && paint->fill_width);
    cui_end_box(ui);
    return result;
}
bool cui_collapsible_ex(cui_context *ui, cui_id id, const cui_theme *theme, cui_str label,
                        bool *open, cui_collapsible_options options, cui_disclosure_panel_fn panel,
                        void *user) {
    return cui_collapsible_styled_common(ui, id, theme, label, open, options, panel, user,
                                         (cui_disclosure_handler){0}, NULL)
        .changed;
}
cui_disclosure_event_result cui_collapsible_events(cui_context *ui, cui_id id,
                                                   const cui_theme *theme, cui_str label,
                                                   bool *open, cui_collapsible_options options,
                                                   cui_disclosure_panel_fn panel, void *user,
                                                   cui_disclosure_handler handler) {
    if (!ui || !id || !theme || !open)
        return (cui_disclosure_event_result){0};
    return cui_collapsible_styled_common(ui, id, theme, label, open, options, panel, user, handler,
                                         NULL);
}
cui_disclosure_event_result cui_collapsible_present(cui_context *ui, cui_id id,
                                                    const cui_theme *theme, cui_str label,
                                                    bool *open, cui_collapsible_options options,
                                                    cui_disclosure_presentation presentation,
                                                    cui_disclosure_panel_fn panel, void *user,
                                                    cui_disclosure_handler handler) {
    cui_disclosure_paint paint;
    presentation.joined = false;
    if (!ui || !id || !open || !cui_disclosure_resolve(theme, presentation, &paint))
        return (cui_disclosure_event_result){0};
    return cui_collapsible_styled_common(ui, id, theme, label, open, options, panel, user, handler,
                                         &paint);
}
bool cui_collapsible(cui_context *ui, cui_id id, const cui_theme *theme, cui_str label, bool *open,
                     bool disabled, cui_disclosure_panel_fn panel, void *user) {
    return cui_collapsible_ex(ui, id, theme, label, open,
                              (cui_collapsible_options){.disabled = disabled}, panel, user);
}
static cui_disclosure_event_result cui_accordion_styled_common(
    cui_context *ui, cui_id container, const cui_theme *theme, const cui_disclosure_item *items,
    size_t count, bool *open, cui_accordion_options options, cui_disclosure_panel_fn panel,
    void *user, cui_disclosure_handler handler, const cui_disclosure_paint *paint) {
    if (!theme || !items || !open)
        return (cui_disclosure_event_result){0};
    cui_disclosure_event_result result = {0};
    if (handler.on_change)
        result = cui_base_accordion_events(ui, container, items, count, open, options, handler);
    else
        result.changed = cui_base_accordion(ui, items, count, open, options);
    cui_interact(ui, container, CUI_ROLE_GROUP, cui_string(""), options.disabled, false, 0);
    cui_set_semantic_details(ui, container,
                             (cui_semantic_details){.orientation = CUI_ORIENTATION_VERTICAL});
    cui_begin_box(ui, container, cui_disclosure_container(theme, paint, options.bordered));
    bool joined = paint && paint->options.joined;
    for (size_t i = 0; i < count; ++i) {
        bool disabled = options.disabled || items[i].disabled;
        cui_disclosure_trigger(
            ui, items[i].id, theme, items[i].label, open[i], disabled, options.bordered, &items[i],
            i, options.render_header, options.header_user,
            options.trigger_boxes && i < options.style_count ? &options.trigger_boxes[i] : NULL,
            paint);
        cui_set_semantic_details(
            ui, items[i].id,
            (cui_semantic_details){.expanded = open[i],
                                   .controls = items[i].id ^ UINT64_C(0x1d8f322539deca61),
                                   .parent = container});
        cui_disclosure_set_cursor(ui, items[i].id, disabled, paint, i);
        cui_box panel_box = options.panel_boxes && i < options.style_count
                                ? options.panel_boxes[i]
                                : cui_disclosure_layout(theme, paint);
        cui_disclosure_motion *motion =
            options.motion && i < options.motion_count ? &options.motion[i] : NULL;
        cui_disclosure_panel(ui, items[i].id, container, items[i].label, open[i], panel_box, motion,
                             options.dt, options.response, theme->spring_control,
                             options.reduced_motion, i, panel, user,
                             paint && (paint->fill_width || joined));
        if (joined && i + 1 < count) {
            cui_begin_box(ui, 0, (cui_box){.width = -1, .height = 1, .background = theme->border});
            cui_end_box(ui);
        }
    }
    cui_end_box(ui);
    return result;
}

bool cui_accordion(cui_context *ui, cui_id container, const cui_theme *theme,
                   const cui_disclosure_item *items, size_t count, bool *open,
                   cui_accordion_options options, cui_disclosure_panel_fn panel, void *user) {
    return cui_accordion_styled_common(ui, container, theme, items, count, open, options, panel,
                                       user, (cui_disclosure_handler){0}, NULL)
        .changed;
}
cui_disclosure_event_result cui_accordion_events(cui_context *ui, cui_id container,
                                                 const cui_theme *theme,
                                                 const cui_disclosure_item *items, size_t count,
                                                 bool *open, cui_accordion_options options,
                                                 cui_disclosure_panel_fn panel, void *user,
                                                 cui_disclosure_handler handler) {
    if (!ui || !container || !theme || !items || !count || !open)
        return (cui_disclosure_event_result){0};
    for (size_t i = 0; i < count; ++i)
        if (!items[i].id)
            return (cui_disclosure_event_result){0};
    return cui_accordion_styled_common(ui, container, theme, items, count, open, options, panel,
                                       user, handler, NULL);
}

cui_disclosure_event_result
cui_accordion_present(cui_context *ui, cui_id container, const cui_theme *theme,
                      const cui_disclosure_item *items, size_t count, bool *open,
                      cui_accordion_options options, cui_disclosure_presentation presentation,
                      cui_disclosure_panel_fn panel, void *user, cui_disclosure_handler handler) {
    cui_disclosure_paint paint;
    if (!ui || !container || !items || !count || !open ||
        !cui_disclosure_resolve(theme, presentation, &paint))
        return (cui_disclosure_event_result){0};
    for (size_t i = 0; i < count; ++i)
        if (!items[i].id)
            return (cui_disclosure_event_result){0};
    return cui_accordion_styled_common(ui, container, theme, items, count, open, options, panel,
                                       user, handler, &paint);
}
