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
            }
        }
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
    if (!o->hide_axes && !o->hide_x_axis && o->kind != CUI_CHART_PIE &&
        o->kind != CUI_CHART_RADAR && o->kind != CUI_CHART_SANKEY && w >= 60 && h >= 40) {
        size_t slots = o->point_count ? o->point_count : o->count;
        for (size_t i = 0; i < o->count; i++) {
            if (!cui_chart_x_label_visible(o, i))
                continue;
            cui_str label = o->data[i].label;
            if (!label.length || !label.data)
                continue;
            float left = o->show_y_axis && !o->y_axis_inside ? 50 : 24;
            float x = slots > 1 ? (left + (o->width - 12 - left) * (float)i / (float)(slots - 1)) *
                                      w / o->width
                                : w / 2;
            float y = h - 14;
            if (o->kind == CUI_CHART_CANDLESTICK) {
                cui_chart_candle_geometry candle;
                if (cui_chart_candle_bounds(o, i, &candle) == CUI_PLOT_OK)
                    x = (float)(candle.wick.x_min * w / o->width);
            }
            if (o->kind == CUI_CHART_BAR) {
                cui_plot_rect bar;
                if (cui_chart_bar_bounds(o, i, &bar) == CUI_PLOT_OK) {
                    if (o->bar_alignment == CUI_CHART_BAR_LEFT ||
                        o->bar_alignment == CUI_CHART_BAR_RIGHT) {
                        x = 28;
                        y = (float)((bar.y_min + bar.y_max) * h / (2 * o->height) - 6);
                    } else
                        x = (float)((bar.x_min + bar.x_max) * w / (2 * o->width));
                }
            }
            cui_begin_absolute_box(
                ui, id ^ (UINT64_C(0x5db92a387c4e1100) + i),
                (cui_box){.width = 56, .height = 12, .fixed_width = true, .fixed_height = true},
                (cui_rect){x - 28, y, 56, 12});
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
            cui_plot_scale scale = {
                CUI_PLOT_LINEAR, lo, hi, o->height > 32 ? o->height - 20 : 12, 12, true};
            for (unsigned i = 0; i < visible; i++) {
                char text[48];
                unsigned tick = visible > 1 ? (count - 1) * i / (visible - 1) : 0;
                double pixel_y = 12 + (o->height - 32) * (double)tick / (count - 1);
                double value = lo;
                (void)cui_plot_inverse(&scale, pixel_y, &value);
                cui_str label =
                    cui_chart_number(text, sizeof text, value, o->y_tick_format, o->format_user);
                float y = (float)(pixel_y * h / o->height);
                float x = o->y_axis_inside ? 26 * w / o->width : 0;
                cui_begin_absolute_box(
                    ui, id ^ (UINT64_C(0x9120416c66313b00) + i),
                    (cui_box){.width = 48, .height = 12, .fixed_width = true, .fixed_height = true},
                    (cui_rect){x, y - 6, 48, 12});
                cui_text(ui, label, 10, o->foreground);
                cui_end_box(ui);
            }
        }
    }
    if (o->kind == CUI_CHART_RADAR && o->count <= 64 && w >= 50 && h >= 50) {
        double cx = w / 2.0, cy = h / 2.0;
        double radius =
            (o->radar_outer_radius > 0 ? o->radar_outer_radius : o->height * .4) * h / o->height;
        for (size_t i = 0; i < o->count; i++) {
            cui_str label = o->data[i].label;
            if (!label.length || !label.data)
                continue;
            double angle = -3.14159265358979323846 / 2 + 2 * 3.14159265358979323846 * i / o->count;
            float x = (float)(cx + (radius + 9) * cos(angle));
            float y = (float)(cy + (radius + 9) * sin(angle));
            cui_begin_absolute_box(
                ui, id ^ (UINT64_C(0x25a41f7c9d310000) + i),
                (cui_box){.width = 70, .height = 12, .fixed_width = true, .fixed_height = true},
                (cui_rect){x - 35, y - 6, 70, 12});
            cui_text(ui, label, 10, o->foreground);
            cui_end_box(ui);
        }
    }
    if (o->kind == CUI_CHART_SANKEY && o->count <= 64) {
        cui_chart_sankey_node nodes[256];
        if (cui_chart_sankey_layout(o, nodes, 256) == CUI_PLOT_OK)
            for (size_t i = 0; i < o->count; i++) {
                cui_str label = o->data[i].label;
                if (!label.length || !label.data)
                    continue;
                float x = nodes[i].x1 * w / o->width + 2;
                float y = (nodes[i].y0 + nodes[i].y1) * h / (2 * o->height) - 6;
                cui_begin_absolute_box(
                    ui, id ^ (UINT64_C(0xe5374a8b09ef0000) + i),
                    (cui_box){.width = 72, .height = 12, .fixed_width = true, .fixed_height = true},
                    (cui_rect){x, y, 72, 12});
                cui_text(ui, label, 10, o->foreground);
                cui_end_box(ui);
            }
    }
    if (o->kind == CUI_CHART_BAR && o->show_bar_values && o->count <= 128) {
        cui_color color = o->bar_label_color.a ? o->bar_label_color : o->foreground;
        for (size_t i = 0; i < o->count; i++) {
            cui_plot_rect bar;
            if (cui_chart_bar_bounds(o, i, &bar) != CUI_PLOT_OK)
                continue;
            char value[48];
            cui_str label = cui_chart_number(value, sizeof value, o->data[i].value,
                                             o->bar_label_format, o->format_user);
            if (!label.length)
                continue;
            float x, y;
            bool negative = o->data[i].value < 0;
            if (o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT) {
                bool right = (o->bar_alignment == CUI_CHART_BAR_LEFT) != negative;
                x = (float)((right ? bar.x_max + 2 : bar.x_min - 66) * w / o->width);
                y = (float)(((bar.y_min + bar.y_max) / 2 - 6) * h / o->height);
            } else {
                bool above = (o->bar_alignment == CUI_CHART_BAR_BOTTOM) != negative;
                x = (float)(((bar.x_min + bar.x_max) / 2 - 32) * w / o->width);
                y = (float)((above ? bar.y_min - 12 : bar.y_max + 2) * h / o->height);
            }
            cui_begin_absolute_box(
                ui, id ^ (UINT64_C(0x81ce703914b50000) + i),
                (cui_box){.width = 64, .height = 12, .fixed_width = true, .fixed_height = true},
                (cui_rect){x, y, 64, 12});
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
                cui_begin_absolute_box(
                    ui, id ^ (UINT64_C(0xa6a9f030b2543000) + i),
                    (cui_box){.width = 64, .height = 12, .fixed_width = true, .fixed_height = true},
                    (cui_rect){labels[i].text_x * w / o->width, labels[i].text_y * h / o->height,
                               64, 12});
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
    if (!o->noninteractive && state->hovered < o->count) {
        if (o->tooltip_content) {
            o->tooltip_content(ui, id, state->hovered, &o->data[state->hovered], o->tooltip_user);
        } else {
            cui_str title = o->data[state->hovered].label;
            if (title.length && title.data) {
                cui_begin_absolute_box(ui, id ^ UINT64_C(0x6c712ab004e9b668),
                                       (cui_box){.width = 72,
                                                 .height = 14,
                                                 .fixed_width = true,
                                                 .fixed_height = true,
                                                 .background = o->background},
                                       (cui_rect){w > 74 ? w - 74 : 0, 2, 72, 14});
                cui_text(ui, title, 11, o->foreground);
                cui_end_box(ui);
            }
            size_t rows = o->kind == CUI_CHART_CANDLESTICK ? 4 : o->extra_series_count + 1;
            for (size_t series = 0; series < rows; series++) {
                double number = state->hovered_value;
                if (o->kind != CUI_CHART_CANDLESTICK && series)
                    number = o->extra_series[series - 1].values[state->hovered];
                if (o->kind == CUI_CHART_CANDLESTICK) {
                    const cui_chart_datum *d = &o->data[state->hovered];
                    number = series == 0   ? d->open
                             : series == 1 ? d->high
                             : series == 2 ? d->low
                                           : d->close;
                }
                char value[48];
                cui_str formatted = cui_chart_number(value, sizeof value, number, o->tooltip_format,
                                                     o->format_user);
                if (!formatted.length)
                    continue;
                char row[52];
                cui_str label = formatted;
                if (o->kind == CUI_CHART_CANDLESTICK) {
                    static const char prefix[] = "OHLC";
                    row[0] = prefix[series];
                    row[1] = ':';
                    row[2] = ' ';
                    size_t length =
                        formatted.length < sizeof row - 3 ? formatted.length : sizeof row - 3;
                    memcpy(row + 3, formatted.data, length);
                    label = (cui_str){row, length + 3};
                }
                cui_begin_absolute_box(
                    ui, id ^ (UINT64_C(0xb48d186a501e6b77) + series),
                    (cui_box){.width = 72,
                              .height = 16,
                              .fixed_width = true,
                              .fixed_height = true,
                              .background = o->background},
                    (cui_rect){w > 74 ? w - 74 : 0, 16 + 16 * (float)series, 72, 16});
                cui_color text_color = o->foreground;
                if (o->kind == CUI_CHART_CANDLESTICK) {
                    const cui_chart_datum *d = &o->data[state->hovered];
                    cui_color direction = d->close > d->open ? o->bullish : o->bearish;
                    if (direction.a)
                        text_color = direction;
                }
                cui_text(ui, label, 11, text_color);
                cui_end_box(ui);
            }
        }
    }
    cui_end_box(ui);
    return ok;
}
