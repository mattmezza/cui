/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_str cui_chart_number(char *buffer, size_t capacity, double value,
                                cui_chart_format_fn formatter, void *user) {
    size_t length;
    if (formatter)
        length = formatter(value, buffer, capacity, user);
    else {
        int written = snprintf(buffer, capacity, "%.3g", value);
        length = written > 0 ? (size_t)written : 0;
    }
    if (length >= capacity)
        length = capacity ? capacity - 1 : 0;
    return (cui_str){buffer, length};
}
static void cui_chart_hover_spring(float *position, float *velocity, float target, float dt,
                                   float response) {
    float omega = 6.28318530718f / response;
    float displacement = *position - target;
    float movement = *velocity + omega * displacement;
    float decay = expf(-omega * dt);
    *position = target + (displacement + movement * dt) * decay;
    *velocity = (*velocity - omega * movement * dt) * decay;
    if (fabsf(*position - target) < .1f && fabsf(*velocity) < .1f) {
        *position = target;
        *velocity = 0;
    }
}
static float cui_chart_hover_ease(float progress, bool entering) {
    progress = fmaxf(0, fminf(1, progress));
    float x1 = entering ? .16f : .4f, y1 = entering ? 1.f : 0.f;
    float x2 = entering ? .3f : 1.f, y2 = 1.f;
    float lo = 0, hi = 1;
    for (unsigned i = 0; i < 20; i++) {
        float t = (lo + hi) / 2, inv = 1 - t;
        float x = 3 * inv * inv * t * x1 + 3 * inv * t * t * x2 + t * t * t;
        if (x < progress)
            lo = t;
        else
            hi = t;
    }
    float t = (lo + hi) / 2, inv = 1 - t;
    return 3 * inv * inv * t * y1 + 3 * inv * t * t * y2 + t * t * t;
}
static bool cui_chart_sankey_label_lines(const cui_chart_options *o,
                                         const cui_chart_sankey_node *node, size_t index,
                                         cui_chart_sankey_label out[16], size_t *count,
                                         char value_buffer[48]) {
    size_t n = 0;
    if (o->sankey_labels)
        n = o->sankey_labels(&o->data[index], index, node->throughput, out, 16,
                             o->sankey_labels_user);
    else {
        float font = o->sankey_font_size > 0 ? o->sankey_font_size : 10;
        if (o->sankey_show_values) {
            cui_str value = cui_chart_number(value_buffer, 48, node->throughput,
                                             o->sankey_value_format, o->format_user);
            out[n++] = (cui_chart_sankey_label){.text = value, .font_size = font};
        }
        if (o->sankey_show_labels && o->data[index].label.length)
            out[n++] = (cui_chart_sankey_label){.text = o->data[index].label, .font_size = font};
    }
    if (n > 16)
        return false;
    for (size_t i = 0; i < n; i++)
        if ((out[i].text.length && !out[i].text.data) || !isfinite(out[i].font_size) ||
            out[i].font_size < 0)
            return false;
    *count = n;
    return true;
}
float cui_chart_measure_value_axis_gutter(cui_context *ui, const cui_chart_options *o) {
    if (!ui || !o || !o->show_y_axis || o->y_axis_inside || !o->data || !o->count ||
        (o->kind == CUI_CHART_BAR &&
         (o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT)) ||
        o->kind == CUI_CHART_PIE || o->kind == CUI_CHART_RADAR || o->kind == CUI_CHART_SANKEY)
        return 0;
    double lo, hi;
    if (!cui_chart_value_domain(o, &lo, &hi))
        return 0;
    unsigned count = o->tick_count ? o->tick_count : 5;
    if (count < 2 || count > 1000)
        return 0;
    unsigned visible = count < 32 ? count : 32;
    float gutter = 32;
    for (unsigned i = 0; i < visible; i++) {
        unsigned tick = (count - 1) * i / (visible - 1);
        double value = hi - (hi - lo) * tick / (count - 1);
        char buffer[48];
        cui_str label =
            cui_chart_number(buffer, sizeof buffer, value, o->y_tick_format, o->format_user);
        cui_size measured = cui_measure_text(ui, label, 10);
        if (isfinite(measured.width))
            gutter = fmaxf(gutter, measured.width + 4);
    }
    return gutter;
}
cui_plot_result cui_chart_measure_sankey_margins(cui_context *ui, const cui_chart_options *o,
                                                 cui_chart_sankey_margins *out) {
    if (!ui || !o || !out || o->kind != CUI_CHART_SANKEY || !o->width || !o->height ||
        !isfinite(o->sankey_font_size) || o->sankey_font_size < 0 ||
        !isfinite(o->sankey_label_gap) || o->sankey_label_gap < 0)
        return CUI_PLOT_INVALID;
    cui_chart_options plain = *o;
    plain.sankey_margins = (cui_chart_sankey_margins){0};
    cui_chart_sankey_node nodes[256];
    cui_plot_result status = cui_chart_sankey_layout(&plain, nodes, 256);
    if (status != CUI_PLOT_OK)
        return status;
    cui_chart_sankey_margins measured = {0};
    if (!o->sankey_show_labels && !o->sankey_show_values && !o->sankey_labels) {
        *out = measured;
        return CUI_PLOT_OK;
    }
    size_t last_layer = 0;
    for (size_t i = 0; i < o->count; i++)
        if (nodes[i].layer > last_layer)
            last_layer = nodes[i].layer;
    float gap = o->sankey_label_gap > 0 ? o->sankey_label_gap : 6;
    for (size_t i = 0; i < o->count; i++) {
        cui_chart_sankey_label lines[16] = {0};
        size_t line_count = 0;
        char value_buffer[48];
        if (!cui_chart_sankey_label_lines(o, &nodes[i], i, lines, &line_count, value_buffer))
            return CUI_PLOT_INVALID;
        if (!line_count)
            continue;
        float width = 0, block_height = 0;
        for (size_t line = 0; line < line_count; line++) {
            float size = lines[line].font_size > 0 ? lines[line].font_size : 10;
            width = fmaxf(width, cui_measure_text(ui, lines[line].text, size).width);
            block_height += size + 2;
        }
        if (!isfinite(width) || width < 0)
            return CUI_PLOT_INVALID;
        if (nodes[i].layer == 0)
            measured.left = fmaxf(measured.left, width + gap);
        else if (nodes[i].layer == last_layer)
            measured.right = fmaxf(measured.right, width + gap);
        else
            measured.top = fmaxf(measured.top, block_height + 2);
    }
    measured.left = fminf(measured.left, o->width * .2f);
    measured.right = fminf(measured.right, o->width * .2f);
    measured.bottom = 2;
    float vertical = measured.top + measured.bottom;
    if (vertical > o->height * .6f) {
        float scale = o->height * .6f / vertical;
        measured.top *= scale;
        measured.bottom *= scale;
    }
    plain.sankey_margins = measured;
    if (cui_chart_sankey_layout(&plain, nodes, 256) != CUI_PLOT_OK)
        return CUI_PLOT_INVALID;
    *out = measured;
    return CUI_PLOT_OK;
}
static bool cui_chart_x_label_visible(const cui_chart_options *o, size_t index) {
    if (!o->x_tick_count) {
        size_t margin = o->x_tick_margin ? o->x_tick_margin : 1;
        return (index + 1) % margin == 0;
    }
    if (o->x_tick_count >= o->count)
        return true;
    if (o->x_tick_count == 1)
        return index == 0;
    for (unsigned tick = 0; tick < o->x_tick_count; tick++) {
        size_t selected = (size_t)lround((double)tick * (o->count - 1) / (o->x_tick_count - 1));
        if (selected == index)
            return true;
    }
    return false;
}
cui_chart_options cui_chart_theme(cui_chart_options o, const cui_theme *theme) {
    if (!theme)
        return o;
    if (!o.background.a)
        o.background = theme->background;
    if (!o.foreground.a)
        o.foreground = theme->text;
    if (!o.grid.a)
        o.grid = theme->border;
    cui_color colors[8] = {theme->primary, theme->success, theme->warning, theme->danger,
                           theme->info,    theme->accent,  theme->muted,   theme->text};
    for (size_t i = 0; i < 8; i++)
        if (!o.series[i].a)
            o.series[i] = colors[i];
    if (!o.bullish.a)
        o.bullish = theme->success;
    if (!o.bearish.a)
        o.bearish = theme->danger;
    return o;
}
cui_plot_result cui_chart_render_animated(cui_context *ui, cui_chart_state *state,
                                          const cui_chart_options *options) {
    if (!ui || !state || !options || !isfinite(options->appear_duration) ||
        options->appear_duration < 0)
        return CUI_PLOT_INVALID;
    cui_chart_options frame = *options;
    bool restart = !state->appear_started || state->active_appear_key != options->appear_key;
    double now = cui_frame_time(ui);
    double started = restart ? now : state->appear_started_at;
    double duration = options->appear_duration > 0 ? options->appear_duration : 1;
    frame.reveal_enabled = true;
    frame.reduced_motion = options->reduced_motion || cui_reduced_motion(ui);
    if (options->disable_appear || frame.reduced_motion)
        frame.reveal_progress = 1;
    else {
        double elapsed = fmax(0, now - started);
        double t = fmax(0, fmin(1, elapsed / duration));
        frame.reveal_progress = (float)(1 - pow(1 - t, 4));
    }
    cui_plot_result result = cui_chart_render(state, &frame);
    if (result != CUI_PLOT_OK)
        return result;
    state->appear_started = true;
    state->appear_started_at = started;
    state->active_appear_key = options->appear_key;
    if (frame.reveal_progress < 1)
        cui_invalidate(ui);
    return CUI_PLOT_OK;
}
bool cui_chart(cui_context *ui, cui_id id, cui_chart_state *state, const cui_chart_options *o,
               cui_str name) {
    if (!ui || !id || !state || !o || !name.data || !name.length || !state->pixels || !o->data ||
        !o->count || o->count > 4096 || o->extra_series_count > 7 ||
        (o->extra_series_count && !o->extra_series) || !isfinite(o->logical_width) ||
        !isfinite(o->logical_height) || o->width > 4096 || o->height > 4096 || !o->width ||
        !o->height || o->point_count > 1000000 ||
        state->byte_capacity < (size_t)o->width * o->height * 4)
        return false;
    float w = o->logical_width > 0 ? o->logical_width : (float)o->width;
    float h = o->logical_height > 0 ? o->logical_height : (float)o->height;
    cui_interact(ui, id, CUI_ROLE_IMAGE, name, false, false, 0);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.pointer_activatable = !o->noninteractive, .no_focus = true});
    if (!o->noninteractive) {
        size_t count = 0;
        const cui_event *events = cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; i++) {
            if (events[i].type == CUI_EVENT_POINTER_MOVE) {
                if (cui_event_target(ui, i) != id && !cui_hovered(ui, id)) {
                    state->hovered = SIZE_MAX;
                    continue;
                }
                cui_chart_hit hit = cui_chart_hit_test(o, events[i].x - state->bounds.x,
                                                       events[i].y - state->bounds.y);
                state->hovered = hit.valid ? hit.hovered : SIZE_MAX;
                state->hovered_value = hit.valid ? hit.value : 0;
                if (hit.valid) {
                    state->tooltip_cursor_x = events[i].x - state->bounds.x;
                    state->tooltip_cursor_y = events[i].y - state->bounds.y;
                    state->tooltip_cursor_valid = true;
                }
            }
        }
    }
    {
        bool active = !o->noninteractive && state->hovered < o->count;
        bool instant = o->reduced_motion || cui_reduced_motion(ui);
        if (active) {
            state->tooltip_visual_index = state->hovered;
            state->tooltip_visual_value = state->hovered_value;
        }
        float dt = cui_frame_delta(ui);
        if (!isfinite(dt) || dt < 0)
            dt = 0;
        if (dt == 0 && active && state->tooltip_phase == 0)
            dt = 1.f / 60;
        float duration = o->hover_duration > 0 ? o->hover_duration : .12f;
        if (!state->tooltip_started || state->tooltip_active != active) {
            state->tooltip_from = state->tooltip_focus;
            state->tooltip_phase = 0;
            state->tooltip_active = active;
            state->tooltip_started = true;
        }
        if (instant) {
            state->tooltip_phase = 1;
            state->tooltip_focus = active ? 1 : 0;
        } else {
            state->tooltip_phase = fminf(1, state->tooltip_phase + dt / duration);
            float eased = cui_chart_hover_ease(state->tooltip_phase, active);
            state->tooltip_focus =
                state->tooltip_from + ((active ? 1.f : 0.f) - state->tooltip_from) * eased;
        }
        if (!active && state->tooltip_focus <= 0)
            state->tooltip_visual_index = SIZE_MAX;
        if (!instant && state->tooltip_phase < 1)
            cui_invalidate(ui);
    }
    if (o->kind == CUI_CHART_BAR || o->kind == CUI_CHART_PIE || o->kind == CUI_CHART_SANKEY) {
        bool active = !o->noninteractive && state->hovered < o->count;
        if (active)
            state->emphasis_visual_index = state->hovered;
        bool instant = o->reduced_motion || cui_reduced_motion(ui);
        float duration = o->hover_duration > 0 ? o->hover_duration : .12f;
        float dt = cui_frame_delta(ui);
        if (!isfinite(dt) || dt < 0)
            dt = 0;
        if (dt == 0 && active && state->emphasis_focus == 0)
            dt = 1.f / 60;
        dt = fminf(dt, .25f);
        state->emphasis_focus =
            instant ? (active ? 1 : 0)
                    : fmaxf(0, fminf(1, state->emphasis_focus + (active ? dt : -dt) / duration));
        bool moving = state->emphasis_focus > 0 && state->emphasis_focus < 1;
        if (o->kind == CUI_CHART_BAR && active) {
            float target;
            if (cui_chart_bar_band_center(o, state->hovered, &target) == CUI_PLOT_OK) {
                if (!state->emphasis_started || instant) {
                    state->emphasis_center = target;
                    state->emphasis_velocity = 0;
                    state->emphasis_started = true;
                } else
                    cui_chart_hover_spring(&state->emphasis_center, &state->emphasis_velocity,
                                           target, dt, duration);
                moving |= fabsf(state->emphasis_center - target) > .1f ||
                          fabsf(state->emphasis_velocity) > .1f;
            }
        }
        if (o->kind == CUI_CHART_PIE)
            for (size_t i = 0; i < o->count && i < 256; i++) {
                float target = active && state->hovered == i ? 1 : 0;
                if (instant) {
                    state->pie_lift[i] = target;
                    state->pie_lift_velocity[i] = 0;
                } else
                    cui_chart_hover_spring(&state->pie_lift[i], &state->pie_lift_velocity[i],
                                           target, dt, duration);
                moving |= fabsf(state->pie_lift[i] - target) > .001f ||
                          fabsf(state->pie_lift_velocity[i]) > .001f;
            }
        if (!active && state->emphasis_focus == 0)
            state->emphasis_started = false;
        if (!active && state->emphasis_focus == 0)
            state->emphasis_visual_index = SIZE_MAX;
        if (!instant && moving)
            cui_invalidate(ui);
        cui_chart_options frame = *o;
        if (!frame.reveal_enabled && state->appear_started && state->rendered_reveal_progress < 1) {
            frame.reveal_enabled = true;
            frame.reveal_progress = state->rendered_reveal_progress;
        }
        if (cui_chart_render(state, &frame) != CUI_PLOT_OK)
            return false;
    }
    cui_begin_box(
        ui, id,
        (cui_box){
            .width = w, .height = h, .fixed_width = true, .fixed_height = true, .clip = true});
    (void)cui_track_bounds(ui, id, &state->bounds);
    cui_bitmap_paint paint = {
        .bitmap = {state->pixels, o->width, o->height, (size_t)o->width * 4, state->revision},
        .opacity = 1};
    bool ok = cui_paint_bitmap(
        ui, id ^ UINT64_C(0x381c6b293497e8a1), paint,
        (cui_box){.width = w, .height = h, .fixed_width = true, .fixed_height = true});
    if (o->kind == CUI_CHART_BAR && !o->noninteractive && state->emphasis_focus > 0 &&
        state->emphasis_visual_index < o->count) {
        cui_plot_rect bar;
        if (cui_chart_bar_bounds(o, state->emphasis_visual_index, &bar) == CUI_PLOT_OK) {
            bool horizontal =
                o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT;
            float thickness = horizontal ? (bar.y_max - bar.y_min) * h / o->height
                                         : (bar.x_max - bar.x_min) * w / o->width;
            float band_x = horizontal ? 24 * w / o->width
                                      : state->emphasis_center * w / o->width - thickness / 2;
            float band_y = horizontal ? state->emphasis_center * h / o->height - thickness / 2
                                      : 12 * h / o->height;
            float band_w = horizontal ? (o->width - 36) * w / o->width : thickness;
            float band_h = horizontal ? thickness : (o->height - 32) * h / o->height;
            cui_color band = o->foreground.a ? o->foreground : (cui_color){32, 32, 32, 255};
            band.a = (unsigned char)lroundf(band.a * .08f * state->emphasis_focus);
            if (band.a && band_w > 0 && band_h > 0) {
                cui_begin_absolute_box(ui, id ^ UINT64_C(0x9c97c1765a400001),
                                       (cui_box){.width = band_w,
                                                 .height = band_h,
                                                 .fixed_width = true,
                                                 .fixed_height = true,
                                                 .background = band},
                                       (cui_rect){band_x, band_y, band_w, band_h});
                cui_end_box(ui);
            }
        }
    }
    if (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA) {
        bool active = !o->noninteractive && state->hovered < o->count;
        float target_x = 0, target_y[8] = {0}, pixel_x, pixel_y;
        if (active &&
            cui_chart_point_coordinates(o, state->hovered, 0, &pixel_x, &pixel_y) != CUI_PLOT_OK)
            active = false;
        if (active) {
            target_x = pixel_x * w / o->width;
            for (size_t series = 0; series <= o->extra_series_count; series++) {
                if (cui_chart_point_coordinates(o, state->hovered, series, &pixel_x, &pixel_y) ==
                    CUI_PLOT_OK)
                    target_y[series] = pixel_y * h / o->height;
            }
            if (!state->hover_visual_started) {
                state->hover_visual_started = true;
                state->hover_progress = 0;
                state->hover_x = target_x;
                state->hover_vx = 0;
                for (size_t series = 0; series <= o->extra_series_count; series++) {
                    state->hover_y[series] = target_y[series];
                    state->hover_vy[series] = 0;
                }
            }
            state->hover_visual_index = state->hovered;
        }
        if (state->hover_visual_started) {
            bool instant = o->reduced_motion || cui_reduced_motion(ui);
            float duration = o->hover_duration > 0 ? o->hover_duration : .12f;
            float dt = cui_frame_delta(ui);
            if (!isfinite(dt) || dt < 0)
                dt = 0;
            if (dt == 0 && active && state->hover_progress == 0)
                dt = 1.f / 60;
            dt = fminf(dt, .25f);
            state->hover_progress =
                instant
                    ? (active ? 1 : 0)
                    : fmaxf(0, fminf(1, state->hover_progress + (active ? dt : -dt) / duration));
            if (active) {
                if (instant) {
                    state->hover_x = target_x;
                    state->hover_vx = 0;
                } else
                    cui_chart_hover_spring(&state->hover_x, &state->hover_vx, target_x, dt,
                                           duration);
                for (size_t series = 0; series <= o->extra_series_count; series++)
                    if (instant) {
                        state->hover_y[series] = target_y[series];
                        state->hover_vy[series] = 0;
                    } else
                        cui_chart_hover_spring(&state->hover_y[series], &state->hover_vy[series],
                                               target_y[series], dt, duration);
            }
            if (state->hover_progress > 0 && !instant)
                cui_invalidate(ui);
            if (state->hover_progress == 0)
                state->hover_visual_started = false;
        }
        if (state->hover_visual_started && state->hover_progress > 0) {
            float progress = state->hover_progress;
            float opacity = progress * progress * (3 - 2 * progress);
            float x = state->hover_x;
            float top = (o->custom_y_padding ? o->y_padding_top : 10) * h / o->height;
            float bottom =
                (o->height - 20 - (o->custom_y_padding ? o->y_padding_bottom : 0)) * h / o->height;
            cui_color crosshair = o->grid.a ? o->grid : (cui_color){120, 120, 120, 150};
            crosshair.a = (unsigned char)lroundf(crosshair.a * opacity);
            cui_begin_absolute_box(ui, id ^ UINT64_C(0x9f39c738cd235001),
                                   (cui_box){.width = 1,
                                             .height = bottom - top,
                                             .fixed_width = true,
                                             .fixed_height = true,
                                             .background = crosshair},
                                   (cui_rect){x, top, 1, bottom - top});
            cui_end_box(ui);
            for (size_t series = 0; series <= o->extra_series_count; series++) {
                float dot_x = state->hover_x;
                float dot_y = state->hover_y[series];
                cui_color dot = series ? o->extra_series[series - 1].color : o->series[0];
                if (!dot.a)
                    dot = o->series[series].a ? o->series[series] : o->foreground;
                dot.a = (unsigned char)lroundf(dot.a * opacity);
                cui_color halo = o->background;
                halo.a = (unsigned char)lroundf(halo.a * opacity);
                cui_begin_absolute_box(ui, id ^ (UINT64_C(0x8e4ad6f381012000) + series),
                                       (cui_box){.width = 10,
                                                 .height = 10,
                                                 .radius = 5,
                                                 .fixed_width = true,
                                                 .fixed_height = true,
                                                 .background = halo},
                                       (cui_rect){dot_x - 5, dot_y - 5, 10, 10});
                cui_end_box(ui);
                cui_begin_absolute_box(ui, id ^ (UINT64_C(0x9e4ad6f381012000) + series),
                                       (cui_box){.width = 6,
                                                 .height = 6,
                                                 .radius = 3,
                                                 .fixed_width = true,
                                                 .fixed_height = true,
                                                 .background = dot},
                                       (cui_rect){dot_x - 3, dot_y - 3, 6, 6});
                cui_end_box(ui);
            }
        }
    }
    if (o->kind == CUI_CHART_RADAR && !o->noninteractive && state->hovered < o->count)
        for (size_t series = 0; series <= o->extra_series_count; series++) {
            float pixel_x, pixel_y;
            if (cui_chart_radar_vertex(o, state->hovered, series, &pixel_x, &pixel_y) !=
                CUI_PLOT_OK)
                continue;
            float dot_x = pixel_x * w / o->width;
            float dot_y = pixel_y * h / o->height;
            cui_color dot = series ? o->extra_series[series - 1].color : o->series[0];
            if (!dot.a)
                dot = o->series[series].a ? o->series[series] : o->foreground;
            cui_begin_absolute_box(ui, id ^ (UINT64_C(0x46a5c19a4f310000) + series),
                                   (cui_box){.width = 10,
                                             .height = 10,
                                             .radius = 5,
                                             .fixed_width = true,
                                             .fixed_height = true,
                                             .background = o->background},
                                   (cui_rect){dot_x - 5, dot_y - 5, 10, 10});
            cui_end_box(ui);
            cui_begin_absolute_box(ui, id ^ (UINT64_C(0x56a5c19a4f310000) + series),
                                   (cui_box){.width = 6,
                                             .height = 6,
                                             .radius = 3,
                                             .fixed_width = true,
                                             .fixed_height = true,
                                             .background = dot},
                                   (cui_rect){dot_x - 3, dot_y - 3, 6, 6});
            cui_end_box(ui);
        }
    if (!o->hide_axes && !o->hide_x_axis && o->kind != CUI_CHART_PIE &&
        o->kind != CUI_CHART_RADAR && o->kind != CUI_CHART_SANKEY && w >= 60 && h >= 40) {
        size_t slots = o->point_count ? o->point_count : o->count;
        for (size_t i = 0; i < o->count; i++) {
            if (!cui_chart_x_label_visible(o, i))
                continue;
            cui_str label = o->data[i].label;
            if (!label.length || !label.data)
                continue;
            bool point_chart = o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA;
            float left = point_chart ? (o->show_y_axis && !o->y_axis_inside
                                            ? (o->value_axis_gutter > 0 ? o->value_axis_gutter : 32)
                                            : 0)
                                     : (o->show_y_axis && !o->y_axis_inside
                                            ? (o->value_axis_gutter > 0 ? o->value_axis_gutter : 50)
                                            : 24);
            float right = point_chart ? o->width - left : o->width - 12;
            float x = slots > 1
                          ? (left + (right - left) * (float)i / (float)(slots - 1)) * w / o->width
                          : w / 2;
            float y = h - 14;
            cui_size measured = cui_measure_text(ui, label, 10);
            float box_width = fmaxf(1, measured.width);
            float box_height = fmaxf(12, measured.height);
            float box_x = x - box_width / 2;
            if (o->kind == CUI_CHART_CANDLESTICK) {
                cui_chart_candle_geometry candle;
                if (cui_chart_candle_bounds(o, i, &candle) == CUI_PLOT_OK) {
                    x = (float)(candle.wick.x_min * w / o->width);
                    box_x = x - box_width / 2;
                }
            }
            if (o->kind == CUI_CHART_BAR) {
                float band_center;
                if (cui_chart_bar_band_center(o, i, &band_center) == CUI_PLOT_OK) {
                    double lo, hi;
                    if (!cui_chart_value_domain(o, &lo, &hi))
                        continue;
                    float fraction = (float)((0 - lo) / (hi - lo));
                    if (o->bar_alignment == CUI_CHART_BAR_LEFT ||
                        o->bar_alignment == CUI_CHART_BAR_RIGHT) {
                        float baseline = (o->bar_alignment == CUI_CHART_BAR_LEFT
                                              ? 24 + (o->width - 36) * fraction
                                              : o->width - 12 - (o->width - 36) * fraction) *
                                         w / o->width;
                        box_x = o->bar_alignment == CUI_CHART_BAR_LEFT ? baseline - 2 - box_width
                                                                       : baseline + 2;
                        y = band_center * h / o->height - box_height / 2;
                    } else {
                        x = band_center * w / o->width;
                        box_x = x - box_width / 2;
                        float top = 12 * h / o->height;
                        float bottom = (o->height > 32 ? o->height - 20 : 12) * h / o->height;
                        float baseline = o->bar_alignment == CUI_CHART_BAR_BOTTOM
                                             ? bottom - (bottom - top) * fraction
                                             : top + (bottom - top) * fraction;
                        bool below =
                            (o->data[i].value < 0) == (o->bar_alignment == CUI_CHART_BAR_TOP);
                        y = baseline + (below ? 2 : -12);
                    }
                }
            }
            cui_begin_absolute_box(ui, id ^ (UINT64_C(0x5db92a387c4e1100) + i),
                                   (cui_box){.width = box_width,
                                             .height = box_height,
                                             .fixed_width = true,
                                             .fixed_height = true},
                                   (cui_rect){box_x, y, box_width, box_height});
            cui_text(ui, label, 10, o->foreground);
            cui_end_box(ui);
        }
    }
    if (o->show_y_axis && o->kind != CUI_CHART_PIE && o->kind != CUI_CHART_RADAR &&
        o->kind != CUI_CHART_SANKEY && w >= 60 && h >= 40 && o->width >= 60 && o->height >= 40) {
        double lo, hi;
        if (cui_chart_value_domain(o, &lo, &hi)) {
            unsigned count = o->tick_count ? o->tick_count : 5;
            unsigned visible = count < 32 ? count : 32;
            bool horizontal_bar =
                o->kind == CUI_CHART_BAR &&
                (o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT);
            bool point_chart = o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA;
            double top = point_chart ? (o->custom_y_padding ? o->y_padding_top : 10) : 12;
            double bottom = point_chart
                                ? o->height - 20 - (o->custom_y_padding ? o->y_padding_bottom : 0)
                                : (o->height > 32 ? o->height - 20 : 12);
            cui_plot_scale scale = {CUI_PLOT_LINEAR, lo, hi, bottom, top, true};
            for (unsigned i = 0; i < visible; i++) {
                char text[48];
                unsigned tick = visible > 1 ? (count - 1) * i / (visible - 1) : 0;
                double pixel_y = top + (bottom - top) * (double)tick / (count - 1);
                double value = lo;
                if (horizontal_bar)
                    value = lo + (hi - lo) * tick / (count - 1);
                else
                    (void)cui_plot_inverse(&scale, pixel_y, &value);
                cui_str label =
                    cui_chart_number(text, sizeof text, value, o->y_tick_format, o->format_user);
                float y = (float)(pixel_y * h / o->height);
                float gutter =
                    o->value_axis_gutter > 0
                        ? o->value_axis_gutter
                        : (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA ? 32 : 50);
                float x = o->y_axis_inside ? 26 * w / o->width : 0;
                float label_width = o->y_axis_inside ? 48 : gutter - 4;
                if (horizontal_bar) {
                    float left = 24 * w / o->width;
                    float right = (o->width - 12) * w / o->width;
                    float fraction = (float)tick / (count - 1);
                    x = (o->bar_alignment == CUI_CHART_BAR_LEFT
                             ? left + (right - left) * fraction
                             : right - (right - left) * fraction) -
                        24;
                    y = o->y_axis_inside ? h - 14 : h - 28;
                    label_width = 48;
                }
                cui_begin_absolute_box(ui, id ^ (UINT64_C(0x9120416c66313b00) + i),
                                       (cui_box){.width = label_width,
                                                 .height = 12,
                                                 .fixed_width = true,
                                                 .fixed_height = true},
                                       (cui_rect){x, y - 6, label_width, 12});
                cui_text(ui, label, 10, o->foreground);
                cui_end_box(ui);
            }
        }
    }
    if (o->kind == CUI_CHART_RADAR && w >= 50 && h >= 50) {
        double cx = w / 2.0, cy = h / 2.0;
        double radius =
            (o->radar_outer_radius > 0 ? o->radar_outer_radius : o->height * .4) * h / o->height;
        for (size_t i = 0; i < o->count; i++) {
            cui_str label = o->data[i].label;
            bool custom = o->radar_label_measure && o->radar_label_paint;
            if (!custom && (!label.length || !label.data))
                continue;
            double angle = -3.14159265358979323846 / 2 + 2 * 3.14159265358979323846 * i / o->count;
            float gap = o->radar_label_gap > 0 ? o->radar_label_gap : 10;
            float x = (float)(cx + (radius + gap) * cos(angle));
            float y = (float)(cy + (radius + gap) * sin(angle));
            cui_size measured =
                custom ? o->radar_label_measure(ui, &o->data[i], i, o->radar_label_user)
                       : cui_measure_text(ui, label, 10);
            if (!isfinite(measured.width) || !isfinite(measured.height) || measured.width <= 0 ||
                measured.height <= 0)
                continue;
            float label_width = fmaxf(1, fminf(w, measured.width));
            float label_height = custom ? fminf(h, measured.height) : fmaxf(12, measured.height);
            float left = fmaxf(0, fminf(w - label_width, x - label_width / 2));
            float top = fmaxf(0, fminf(h - label_height, y - label_height / 2));
            cui_begin_absolute_box(ui, id ^ (UINT64_C(0x25a41f7c9d310000) + i),
                                   (cui_box){.width = label_width,
                                             .height = label_height,
                                             .fixed_width = true,
                                             .fixed_height = true},
                                   (cui_rect){left, top, label_width, label_height});
            if (custom)
                o->radar_label_paint(ui, id ^ (UINT64_C(0x35a41f7c9d310000) + i), &o->data[i], i,
                                     o->radar_label_user);
            else
                cui_text(ui, label, 10,
                         o->radar_label_color.a ? o->radar_label_color : o->foreground);
            cui_end_box(ui);
        }
    }
    if (o->kind == CUI_CHART_SANKEY &&
        (o->sankey_show_labels || o->sankey_show_values || o->sankey_labels)) {
        cui_chart_sankey_node nodes[256];
        if (cui_chart_sankey_layout(o, nodes, 256) == CUI_PLOT_OK)
            for (size_t i = 0; i < o->count; i++) {
                cui_chart_sankey_label lines[16] = {0};
                size_t line_count = 0;
                char value_buffer[48];
                if (!cui_chart_sankey_label_lines(o, &nodes[i], i, lines, &line_count,
                                                  value_buffer) ||
                    !line_count)
                    continue;
                float gap = o->sankey_label_gap > 0 ? o->sankey_label_gap : 6;
                bool first = nodes[i].layer == 0;
                bool last = true;
                for (size_t j = 0; j < o->count; j++)
                    if (nodes[j].layer > nodes[i].layer)
                        last = false;
                float block_height = 0, text_width = 0;
                for (size_t line = 0; line < line_count; line++) {
                    float font = lines[line].font_size > 0 ? lines[line].font_size : 10;
                    block_height += font + 2;
                    text_width =
                        fmaxf(text_width, cui_measure_text(ui, lines[line].text, font).width);
                }
                text_width = fmaxf(1, fminf(w * .2f, text_width));
                float center_y = (nodes[i].y0 + nodes[i].y1) * h / (2 * o->height);
                float y = first || last
                              ? fmaxf(0, fminf(h - block_height, center_y - block_height / 2))
                              : nodes[i].y0 * h / o->height - block_height - 2;
                float node_left = nodes[i].x0 * w / o->width;
                float node_right = nodes[i].x1 * w / o->width;
                float x = first  ? fmaxf(0, node_left - gap - text_width)
                          : last ? fminf(w - text_width, node_right + gap)
                                 : fmaxf(0, fminf(w - text_width,
                                                  (node_left + node_right - text_width) / 2));
                for (size_t line = 0; line < line_count; line++) {
                    float font = lines[line].font_size > 0 ? lines[line].font_size : 10;
                    float line_height = font + 2;
                    cui_begin_absolute_box(ui, id ^ (UINT64_C(0xe5374a8b09ef0000) + i * 16 + line),
                                           (cui_box){.width = text_width,
                                                     .height = line_height,
                                                     .fixed_width = true,
                                                     .fixed_height = true},
                                           (cui_rect){x, y, text_width, line_height});
                    cui_text(ui, lines[line].text, font,
                             lines[line].color.a ? lines[line].color : o->foreground);
                    cui_end_box(ui);
                    y += line_height;
                }
            }
    }
    if (o->kind == CUI_CHART_BAR && o->show_bar_values && o->count <= 128) {
        for (size_t i = 0; i < o->count; i++) {
            cui_plot_rect bar;
            if (cui_chart_bar_bounds(o, i, &bar) != CUI_PLOT_OK)
                continue;
            char value[48];
            cui_str label = cui_chart_number(value, sizeof value, o->data[i].value,
                                             o->bar_label_format, o->format_user);
            if (!label.length)
                continue;
            cui_size measured = cui_measure_text(ui, label, 10);
            float label_width = fmaxf(1, measured.width);
            float label_height = fmaxf(12, measured.height);
            float x, y;
            bool negative = o->data[i].value < 0;
            if (o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT) {
                bool right = (o->bar_alignment == CUI_CHART_BAR_LEFT) != negative;
                x = (float)((right ? bar.x_max + 2 : bar.x_min - 2) * w / o->width);
                if (!right)
                    x -= label_width;
                y = (float)((bar.y_min + bar.y_max) / 2 * h / o->height) - label_height / 2;
            } else {
                bool above = (o->bar_alignment == CUI_CHART_BAR_BOTTOM) != negative;
                x = (float)((bar.x_min + bar.x_max) / 2 * w / o->width) - label_width / 2;
                y = (float)((above ? bar.y_min : bar.y_max + 2) * h / o->height);
                if (above)
                    y -= label_height;
            }
            cui_begin_absolute_box(ui, id ^ (UINT64_C(0x81ce703914b50000) + i),
                                   (cui_box){.width = label_width,
                                             .height = label_height,
                                             .fixed_width = true,
                                             .fixed_height = true},
                                   (cui_rect){x, y, label_width, label_height});
            cui_color color = o->bar_label_color_fn
                                  ? o->bar_label_color_fn(&o->data[i], i, o->bar_label_color_user)
                                  : (o->bar_label_color.a ? o->bar_label_color : o->foreground);
            cui_text(ui, label, 10, color);
            cui_end_box(ui);
        }
    }
    if (o->kind == CUI_CHART_PIE && o->pie_show_labels && o->count <= 256) {
        cui_chart_pie_label labels[256];
        if (cui_chart_pie_labels(o, labels, 256) == CUI_PLOT_OK) {
            float opacity = fmaxf(0, fminf(1, (state->rendered_reveal_progress - .7f) / .3f));
            cui_color color = o->pie_label_color.a ? o->pie_label_color
                              : o->foreground.a    ? o->foreground
                                                   : (cui_color){32, 32, 32, 255};
            color.a = (unsigned char)lroundf(color.a * opacity);
            for (size_t i = 0; i < o->count; i++) {
                if (!labels[i].visible || !color.a)
                    continue;
                cui_size measured = cui_measure_text(ui, labels[i].text, 10);
                float label_width = fmaxf(1, fminf(w, measured.width));
                float label_height = fmaxf(12, measured.height);
                float anchor_x = labels[i].anchor_x * w / o->width;
                float anchor_y = labels[i].anchor_y * h / o->height;
                float label_x = labels[i].right ? anchor_x + 4 : anchor_x - 4 - label_width;
                float label_y = anchor_y - label_height / 2;
                label_x = fmaxf(0, fminf(w - label_width, label_x));
                label_y = fmaxf(0, fminf(h - label_height, label_y));
                cui_begin_absolute_box(ui, id ^ (UINT64_C(0xa6a9f030b2543000) + i),
                                       (cui_box){.width = label_width,
                                                 .height = label_height,
                                                 .fixed_width = true,
                                                 .fixed_height = true},
                                       (cui_rect){label_x, label_y, label_width, label_height});
                cui_text(ui, labels[i].text, 10, color);
                cui_end_box(ui);
            }
        }
    }
    if (!o->hide_legend && (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA) && w >= 100 &&
        h >= 40)
        for (size_t i = 0; i < o->extra_series_count; i++) {
            cui_str label = o->extra_series[i].name;
            if (!label.length || !label.data)
                continue;
            cui_color color =
                o->extra_series[i].color.a ? o->extra_series[i].color : o->series[(i + 1) % 8];
            cui_begin_absolute_box(
                ui, id ^ (UINT64_C(0xe24a05cd17230000) + i),
                (cui_box){.width = 72, .height = 12, .fixed_width = true, .fixed_height = true},
                (cui_rect){w - 74, 2 + 12 * (float)i, 72, 12});
            cui_text(ui, label, 10, color);
            cui_end_box(ui);
        }
    if (!o->noninteractive && state->tooltip_visual_index < o->count && state->tooltip_focus > 0) {
        size_t tooltip_index = state->tooltip_visual_index;
        const cui_chart_datum *datum = &o->data[tooltip_index];
        cui_str title = {0};
        char row_text[8][52] = {{0}};
        size_t row_length[8] = {0}, row_count = 0;
        cui_color row_color[8] = {{0}};
        float tooltip_width = 150;
        if (!o->tooltip_content) {
            title = o->tooltip_title ? o->tooltip_title(datum, tooltip_index, o->tooltip_user)
                                     : datum->label;
            if (title.data && title.length)
                tooltip_width =
                    fmaxf(tooltip_width,
                          cui_measure_text_ex(ui, title, 11,
                                              (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD})
                                  .width +
                              16);
            size_t rows = o->kind == CUI_CHART_CANDLESTICK ? 4 : o->extra_series_count + 1;
            for (size_t series = 0; series < rows; series++) {
                double number =
                    o->kind == CUI_CHART_SANKEY ? state->tooltip_visual_value : datum->value;
                if (o->kind != CUI_CHART_CANDLESTICK && series)
                    number = o->extra_series[series - 1].values[tooltip_index];
                if (o->kind == CUI_CHART_CANDLESTICK)
                    number = series == 0   ? datum->open
                             : series == 1 ? datum->high
                             : series == 2 ? datum->low
                                           : datum->close;
                char value[48];
                cui_str formatted;
                if (o->tooltip_value) {
                    size_t written = o->tooltip_value(datum, tooltip_index, series, number, value,
                                                      sizeof value, o->tooltip_user);
                    formatted =
                        (cui_str){value, written < sizeof value ? written : sizeof value - 1};
                } else
                    formatted = cui_chart_number(value, sizeof value, number, o->tooltip_format,
                                                 o->format_user);
                if (!formatted.length)
                    continue;
                if (o->kind == CUI_CHART_CANDLESTICK) {
                    static const char prefix[] = "OHLC";
                    row_text[row_count][0] = prefix[series];
                    row_text[row_count][1] = ':';
                    row_text[row_count][2] = ' ';
                    size_t length = formatted.length < sizeof row_text[0] - 3
                                        ? formatted.length
                                        : sizeof row_text[0] - 3;
                    memcpy(row_text[row_count] + 3, formatted.data, length);
                    row_length[row_count] = length + 3;
                } else {
                    memcpy(row_text[row_count], formatted.data, formatted.length);
                    row_length[row_count] = formatted.length;
                }
                row_color[row_count] = o->foreground;
                if (o->kind == CUI_CHART_CANDLESTICK) {
                    cui_color direction = datum->close > datum->open ? o->bullish : o->bearish;
                    if (direction.a)
                        row_color[row_count] = direction;
                }
                if (o->tooltip_value_color)
                    row_color[row_count] = o->tooltip_value_color(datum, tooltip_index, series,
                                                                  number, o->tooltip_user);
                tooltip_width = fmaxf(
                    tooltip_width,
                    cui_measure_text(ui, (cui_str){row_text[row_count], row_length[row_count]}, 11)
                            .width +
                        16);
                row_count++;
            }
        }
        float tooltip_height =
            o->tooltip_content ? 64
                               : 16 + (title.length && title.data ? 18 : 0) + 16 * (float)row_count;
        float cursor_x = state->tooltip_cursor_valid ? state->tooltip_cursor_x : 0;
        float cursor_y = state->tooltip_cursor_valid ? state->tooltip_cursor_y : 0;
        float tooltip_x = cursor_x < w / 2 ? cursor_x + 8 : cursor_x - 8 - tooltip_width;
        float tooltip_y = cursor_y < h / 2 ? cursor_y + 8 : cursor_y - 8 - tooltip_height;
        cui_begin_portal(ui, id ^ UINT64_C(0xd8a736411cb50000),
                         (cui_box){.width = tooltip_width,
                                   .height = tooltip_height,
                                   .fixed_width = true,
                                   .fixed_height = true,
                                   .radius = 4,
                                   .background = o->background,
                                   .border = o->grid,
                                   .border_width = 1,
                                   .opacity = state->tooltip_focus,
                                   .custom_opacity = true},
                         (cui_rect){state->bounds.x + tooltip_x, state->bounds.y + tooltip_y,
                                    tooltip_width, tooltip_height},
                         120);
        if (o->tooltip_content)
            o->tooltip_content(ui, id, tooltip_index, datum, o->tooltip_user);
        else {
            float row_y = 8;
            if (title.length && title.data) {
                cui_begin_absolute_box(ui, id ^ UINT64_C(0x6c712ab004e9b668),
                                       (cui_box){.width = tooltip_width - 16,
                                                 .height = 14,
                                                 .fixed_width = true,
                                                 .fixed_height = true},
                                       (cui_rect){8, row_y, tooltip_width - 16, 14});
                cui_text_ex(ui, title, 11, o->foreground, 0,
                            (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD});
                cui_end_box(ui);
                row_y += 18;
            }
            for (size_t series = 0; series < row_count; series++) {
                cui_begin_absolute_box(ui, id ^ (UINT64_C(0xb48d186a501e6b77) + series),
                                       (cui_box){.width = tooltip_width - 16,
                                                 .height = 16,
                                                 .fixed_width = true,
                                                 .fixed_height = true},
                                       (cui_rect){8, row_y, tooltip_width - 16, 16});
                cui_text(ui, (cui_str){row_text[series], row_length[series]}, 11,
                         row_color[series]);
                cui_end_box(ui);
                row_y += 16;
            }
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return ok;
}
