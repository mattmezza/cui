/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_str chart_number(char *buffer, size_t capacity, double value,
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
bool cui_chart(cui_context *ui, cui_id id, cui_chart_state *state, const cui_chart_options *o,
               cui_str name) {
    if (!ui || !id || !state || !o || !name.data || !name.length || !state->pixels ||
        state->byte_capacity < (size_t)o->width * o->height * 4 || o->width > 4096 ||
        o->height > 4096 || !o->width || !o->height)
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
        size_t label_count = o->x_tick_count ? o->x_tick_count : o->count > 8 ? 8 : o->count;
        if (label_count > 32)
            label_count = 32;
        size_t slots = o->point_count ? o->point_count : o->count;
        for (size_t slot = 0; slot < label_count; slot++) {
            size_t i = label_count > 1 ? (slots - 1) * slot / (label_count - 1) : 0;
            if (i >= o->count)
                continue;
            cui_str label = o->data[i].label;
            if (!label.length || !label.data)
                continue;
            float left = o->show_y_axis && !o->y_axis_inside ? 50 : 24;
            float x = slots > 1 ? (left + (o->width - 12 - left) * (float)i / (float)(slots - 1)) *
                                      w / o->width
                                : w / 2;
            cui_begin_absolute_box(
                ui, id ^ (UINT64_C(0x5db92a387c4e1100) + i),
                (cui_box){.width = 56, .height = 12, .fixed_width = true, .fixed_height = true},
                (cui_rect){x - 28, h - 14, 56, 12});
            cui_text(ui, label, 10, o->foreground);
            cui_end_box(ui);
        }
    }
    if (o->show_y_axis && o->kind != CUI_CHART_PIE && o->kind != CUI_CHART_RADAR &&
        o->kind != CUI_CHART_SANKEY && w >= 60 && h >= 40) {
        double lo, hi;
        if (cui_chart_value_domain(o, &lo, &hi)) {
            unsigned count = o->tick_count ? o->tick_count : 5;
            for (unsigned i = 0; i < count; i++) {
                char text[48];
                double value = hi - (hi - lo) * i / (count - 1);
                cui_str label =
                    chart_number(text, sizeof text, value, o->y_tick_format, o->format_user);
                float y = (12 + (o->height - 32) * (float)i / (count - 1)) * h / o->height;
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
    if (!o->hide_legend && o->kind == CUI_CHART_PIE && w >= 100 && h >= 40) {
        size_t limit = o->count < 6 ? o->count : 6;
        for (size_t i = 0; i < limit; i++) {
            cui_str label = o->data[i].label;
            if (!label.length || !label.data)
                continue;
            cui_begin_absolute_box(
                ui, id ^ (UINT64_C(0xa6a9f030b2543000) + i),
                (cui_box){.width = 58, .height = 12, .fixed_width = true, .fixed_height = true},
                (cui_rect){w - 60, 2 + 12 * (float)i, 58, 12});
            cui_text(ui, label, 10, o->foreground);
            cui_end_box(ui);
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
        for (size_t series = 0; series <= o->extra_series_count; series++) {
            double number =
                series ? o->extra_series[series - 1].values[state->hovered] : state->hovered_value;
            char value[48];
            cui_str label =
                chart_number(value, sizeof value, number, o->tooltip_format, o->format_user);
            if (!label.length)
                continue;
            cui_begin_absolute_box(
                ui, id ^ (UINT64_C(0xb48d186a501e6b77) + series),
                (cui_box){.width = 72,
                          .height = 16,
                          .fixed_width = true,
                          .fixed_height = true,
                          .background = o->background},
                (cui_rect){w > 74 ? w - 74 : 0, 16 + 16 * (float)series, 72, 16});
            cui_text(ui, label, 11, o->foreground);
            cui_end_box(ui);
        }
    }
    cui_end_box(ui);
    return ok;
}
