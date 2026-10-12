/* SPDX-License-Identifier: MIT */
#include "cui/cui_picker.h"
#include "cui/cui_components.h"
#include "cui/cui_icon.h"
#include <math.h>
#include <stdio.h>
static cui_id cui_picker_style_id(cui_id id, unsigned index) {
    return id ^ (UINT64_C(0x713d9902fd4b5a97) + UINT64_C(0x9e3779b97f4a7c15) * index);
}
static unsigned cui_picker_style_months(cui_calendar_options o) {
    return o.months < 1 ? 1 : o.months > 3 ? 3 : o.months;
}
static float cui_picker_cell(cui_calendar_options o) {
    return isfinite(o.cell_size) && o.cell_size >= 20 ? fminf(o.cell_size, 128) : 32;
}
static bool cui_picker_style_selected(cui_date d, cui_date_selection s, bool range) {
    if (!cui_date_valid(s.start))
        return false;
    if (!range || !cui_date_valid(s.end))
        return !cui_date_compare(d, s.start);
    return cui_date_compare(d, s.start) >= 0 && cui_date_compare(d, s.end) <= 0;
}
static void cui_picker_paint_cell(cui_context *ui, cui_id id, const cui_theme *t, cui_str label,
                                  float size, bool disabled, bool selected) {
    cui_begin_box(ui, id,
                  (cui_box){.width = size,
                            .height = size,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER,
                            .radius = t->radius,
                            .background = selected ? t->primary : t->surface,
                            .border = cui_focused(ui) == id ? t->focus : t->border,
                            .border_width = cui_focused(ui) == id ? 2 : 0});
    cui_text(ui, label, fminf(t->font_size, size * .625f),
             disabled   ? t->muted
             : selected ? t->on_primary
                        : t->text);
    cui_end_box(ui);
}
static void cui_picker_paint_navigation(cui_context *ui, cui_id id, const cui_theme *t, float size,
                                        bool disabled, bool previous, bool year) {
    cui_begin_box(ui, id,
                  (cui_box){.width = size,
                            .height = size,
                            .row = true,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER,
                            .radius = t->radius,
                            .background = t->surface,
                            .border = cui_focused(ui) == id ? t->focus : t->border,
                            .border_width = cui_focused(ui) == id ? 2 : 0});
    for (unsigned i = 0; i < (year ? 2u : 1u); i++)
        cui_icon_named(ui, cui_picker_style_id(id, i), t,
                       cui_string(previous ? "chevron-left" : "chevron-right"),
                       (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                          .custom_size = size * (year ? .3f : .5f),
                                          .custom_color = true,
                                          .color = disabled ? t->muted : t->text});
    cui_end_box(ui);
}
static void cui_picker_paint_calendar(cui_context *ui, cui_id id, const cui_theme *t,
                                      cui_calendar_state *s, cui_date_selection *selection,
                                      cui_calendar_options o) {
    if (!t || !s || !selection)
        return;
    float cell = cui_picker_cell(o), gap = 2, width = cell * 7 + gap * 6;
    unsigned months = cui_picker_style_months(o);
    const char *weekday[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .gap = t->gap,
                            .padding = t->padding,
                            .radius = t->radius,
                            .background = t->surface});
    for (unsigned m = 0; m < months; ++m) {
        cui_date month = cui_date_add_months(s->month, (int)m), grid[42];
        if (!cui_date_valid(month))
            continue;
        cui_calendar_grid(month, o.first_weekday, grid);
        cui_begin_box(ui, cui_picker_style_id(id, 10 + m), (cui_box){.width = width, .gap = gap});
        cui_begin_box(
            ui, cui_picker_style_id(id, 20 + m),
            (cui_box){
                .row = true, .gap = gap, .width = -1, .height = cell, .align_y = CUI_ALIGN_CENTER});
        if (m == 0) {
            cui_picker_paint_navigation(ui, cui_calendar_navigation_id(id, 1), t, cell, o.disabled,
                                        true, true);
            cui_picker_paint_navigation(ui, cui_calendar_navigation_id(id, 2), t, cell, o.disabled,
                                        true, false);
        }
        char title[24];
        snprintf(title, sizeof(title), "%04d-%02d", month.year, month.month);
        cui_begin_box(ui, cui_picker_style_id(id, 40 + m),
                      (cui_box){.width = -1,
                                .height = cell,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_text(ui, cui_string(title), fminf(t->font_size, cell * .625f), t->text);
        cui_end_box(ui);
        if (m == 0) {
            cui_picker_paint_navigation(ui, cui_calendar_navigation_id(id, 3), t, cell, o.disabled,
                                        false, false);
            cui_picker_paint_navigation(ui, cui_calendar_navigation_id(id, 4), t, cell, o.disabled,
                                        false, true);
        }
        cui_end_box(ui);
        cui_begin_box(ui, cui_picker_style_id(id, 30 + m), (cui_box){.row = true, .gap = gap});
        for (unsigned w = 0; w < 7; ++w) {
            cui_begin_box(ui, cui_picker_style_id(id, 100 + m * 7 + w),
                          (cui_box){.width = cell,
                                    .height = cell * .7f,
                                    .align_x = CUI_ALIGN_CENTER,
                                    .align_y = CUI_ALIGN_CENTER});
            cui_text(ui, cui_string(weekday[(o.first_weekday % 7 + w) % 7]), t->font_size * .8f,
                     t->muted);
            cui_end_box(ui);
        }
        cui_end_box(ui);
        for (unsigned row = 0; row < 6; ++row) {
            cui_begin_box(ui, cui_picker_style_id(id, 200 + m * 6 + row),
                          (cui_box){.row = true, .gap = gap});
            for (unsigned col = 0; col < 7; ++col) {
                unsigned index = row * 7 + col;
                if (cui_date_valid(grid[index]) && grid[index].month == month.month &&
                    grid[index].year == month.year) {
                    char label[8];
                    snprintf(label, sizeof(label), "%d", grid[index].day);
                    cui_picker_paint_cell(
                        ui, cui_calendar_day_id(id, grid[index]), t, cui_string(label), cell,
                        !cui_calendar_date_enabled(grid[index], o),
                        cui_picker_style_selected(grid[index], *selection, o.range));
                } else {
                    cui_begin_box(ui, cui_picker_style_id(id, 400 + m * 42 + index),
                                  (cui_box){.width = cell, .height = cell});
                    cui_end_box(ui);
                }
            }
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
cui_calendar_result cui_calendar(cui_context *ui, cui_id id, const cui_theme *t,
                                 cui_calendar_state *s, cui_date_selection *selection,
                                 cui_calendar_options o) {
    if (!t)
        return (cui_calendar_result){0};
    cui_calendar_result result = cui_base_calendar(ui, id, s, selection, o);
    cui_picker_paint_calendar(ui, id, t, s, selection, o);
    return result;
}
bool cui_date_picker(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                     cui_date_picker_state *s, cui_date_selection *selection,
                     cui_date_picker_options o) {
    if (!ui || !t || !s || !selection)
        return false;
    bool was_open = s->overlay.open;
    cui_calendar_result result =
        cui_base_date_picker(ui, id, cui_picker_style_id(id, 900), name, s, selection, o);
    char value[40];
    cui_str label;
    if (cui_date_valid(selection->start)) {
        if (o.calendar.range && cui_date_valid(selection->end))
            snprintf(value, sizeof(value), "%04d-%02d-%02d / %04d-%02d-%02d", selection->start.year,
                     selection->start.month, selection->start.day, selection->end.year,
                     selection->end.month, selection->end.day);
        else
            snprintf(value, sizeof(value), "%04d-%02d-%02d", selection->start.year,
                     selection->start.month, selection->start.day);
        label = cui_string(value);
    } else
        label = o.placeholder.length ? o.placeholder : cui_string("Select date");
    cui_button_paint(ui, id, t, label,
                     (cui_button_options){.variant = CUI_BUTTON_OUTLINE,
                                          .width = isfinite(o.width) && o.width > 0 ? o.width : 240,
                                          .disabled = o.disabled,
                                          .icon = cui_string("calendar")});
    if (!s->overlay.open)
        return result.changed;
    if (!isfinite(o.viewport.x) || !isfinite(o.viewport.y) || !isfinite(o.viewport.w) ||
        !isfinite(o.viewport.h) || o.viewport.w <= 0 || o.viewport.h <= 0) {
        cui_overlay_close(ui, &s->overlay);
        return false;
    }
    float cell = cui_picker_cell(o.calendar),
          width = (cell * 7 + 12) * cui_picker_style_months(o.calendar) + t->padding * 2 +
                  t->gap * (cui_picker_style_months(o.calendar) - 1);
    float height = cell * 7.7f + 14 + t->padding * 2;
    cui_popup_result placement = cui_popup_place(cui_bounds(ui, id), (cui_size){width, height},
                                                 o.viewport, CUI_POPUP_BELOW, 4);
    cui_overlay_set_bounds(&s->overlay, placement.bounds);
    if (!cui_overlay_begin(ui, &s->overlay))
        return false;
    cui_begin_portal(ui, s->overlay.id,
                     (cui_box){.background = t->surface,
                               .border = t->border,
                               .border_width = 1,
                               .radius = t->radius,
                               .align_x = CUI_ALIGN_CENTER,
                               .align_y = CUI_ALIGN_CENTER,
                               .clip = true},
                     placement.bounds, 100);
    cui_calendar_options calendar = o.calendar;
    calendar.disabled = calendar.disabled || o.disabled;
    cui_calendar_submit(ui, cui_picker_style_id(id, 900), &s->calendar, *selection, calendar);
    cui_picker_paint_calendar(ui, cui_picker_style_id(id, 900), t, &s->calendar, selection,
                              calendar);
    if (!was_open)
        cui_focus(ui, cui_calendar_day_id(cui_picker_style_id(id, 900), s->calendar.cursor));
    cui_end_box(ui);
    cui_overlay_end(ui);
    cui_set_semantic_details(
        ui, id, (cui_semantic_details){.expanded = s->overlay.open, .controls = s->overlay.id});
    return result.changed;
}
bool cui_time_field(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                    cui_time_state *state, cui_time *time, cui_time_options options) {
    if (!t || !state || !time || !cui_time_valid(*time))
        return false;
    bool changed = cui_base_time_field(ui, id, name, state, time, options);
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .gap = 4,
                            .padding = t->padding,
                            .radius = t->radius,
                            .background = t->surface,
                            .border = t->border,
                            .border_width = 1});
    for (unsigned i = 0; i < 4; ++i) {
        if ((i == 2 && !options.seconds) || (i == 3 && !options.twelve_hour))
            continue;
        char text[4];
        unsigned value =
            i == 0   ? (options.twelve_hour ? (time->hour % 12 ? time->hour % 12 : 12) : time->hour)
            : i == 1 ? time->minute
                     : time->second;
        if (i == 3)
            snprintf(text, sizeof(text), "%s", time->hour >= 12 ? "PM" : "AM");
        else
            snprintf(text, sizeof(text), "%02u", value);
        cui_id segment = cui_time_segment_id(id, i);
        cui_begin_box(
            ui, segment,
            (cui_box){.width = t->font_size * 2,
                      .padding = 2,
                      .radius = t->radius,
                      .background = cui_focused(ui) == segment ? t->primary : t->surface});
        cui_text(ui, cui_string(text), t->font_size,
                 options.disabled             ? t->muted
                 : cui_focused(ui) == segment ? t->on_primary
                                              : t->text);
        cui_end_box(ui);
        if (i == 0 || (i == 1 && options.seconds))
            cui_text(ui, cui_string(":"), t->font_size, t->muted);
    }
    cui_end_box(ui);
    return changed;
}
bool cui_color_picker(cui_context *ui, cui_id id, const cui_theme *t, cui_color_state *state,
                      cui_color *color, cui_color_picker_options options) {
    if (!t || !state || !color || (!options.palette && options.palette_count) ||
        options.palette_count > 64)
        return false;
    bool changed = cui_base_color_picker(ui, id, state, color, options);
    float width =
        isfinite(options.width) && options.width >= 120 ? fminf(options.width, 1000) : 240;
    cui_begin_box(
        ui, id,
        (cui_box){
            .gap = t->gap, .padding = t->padding, .radius = t->radius, .background = t->surface});
    cui_begin_box(ui, cui_color_part_id(id, 2), (cui_box){.width = width, .height = width * .6f});
    for (unsigned y = 0; y < 8; ++y) {
        cui_begin_box(ui, cui_picker_style_id(id, 1000 + y),
                      (cui_box){.row = true, .height = width * .6f / 8});
        for (unsigned x = 0; x < 12; ++x) {
            cui_hsv hsv = {state->hsv.hue, x / 11.f, 1 - y / 7.f, 1};
            bool marker = (unsigned)lroundf(state->hsv.saturation * 11) == x &&
                          (unsigned)lroundf((1 - state->hsv.value) * 7) == y;
            cui_begin_box(ui, cui_picker_style_id(id, 1100 + y * 12 + x),
                          (cui_box){.width = width / 12,
                                    .height = width * .6f / 8,
                                    .background = cui_color_from_hsv(hsv),
                                    .border = marker ? t->focus : (cui_color){0},
                                    .border_width = marker ? 2 : 0});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_begin_box(ui, cui_color_part_id(id, 1),
                  (cui_box){.row = true, .width = width, .height = 20});
    for (unsigned x = 0; x < 24; ++x) {
        cui_begin_box(ui, cui_picker_style_id(id, 1300 + x),
                      (cui_box){.width = width / 24,
                                .height = 20,
                                .background = cui_color_from_hsv((cui_hsv){x * 15.f, 1, 1, 1})});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (options.alpha) {
        cui_begin_box(
            ui, cui_color_part_id(id, 3),
            (cui_box){.row = true, .width = width, .height = 20, .background = t->border});
        for (unsigned x = 0; x < 16; ++x) {
            cui_color c = *color;
            c.a = (uint8_t)(x * 17);
            cui_begin_box(ui, cui_picker_style_id(id, 1400 + x),
                          (cui_box){.width = width / 16, .height = 20, .background = c});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    const char *names[] = {"R", "G", "B", "A"};
    unsigned channels[] = {color->r, color->g, color->b, color->a};
    for (unsigned i = 0; i < (options.alpha ? 4u : 3u); ++i) {
        char label[24];
        snprintf(label, sizeof(label), "%s %u", names[i], channels[i]);
        cui_id part = cui_color_part_id(id, 20 + i);
        cui_begin_box(ui, part,
                      (cui_box){.width = width,
                                .height = 40,
                                .gap = 4,
                                .radius = t->radius,
                                .border = cui_focused(ui) == part ? t->focus : t->border,
                                .border_width = 1});
        cui_text(ui, cui_string(label), t->font_size, options.disabled ? t->muted : t->text);
        if (channels[i]) {
            cui_begin_box(ui, cui_picker_style_id(id, 1500 + i),
                          (cui_box){.width = width * channels[i] / 255.f,
                                    .height = 4,
                                    .background = t->primary});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    char hex[10];
    cui_color_format_hex(*color, options.alpha, hex, sizeof(hex));
    cui_begin_box(ui, cui_picker_style_id(id, 1600), (cui_box){.row = true, .gap = t->gap});
    cui_begin_box(ui, cui_picker_style_id(id, 1601),
                  (cui_box){.width = 24,
                            .height = 24,
                            .background = *color,
                            .border = t->border,
                            .border_width = 1});
    cui_end_box(ui);
    cui_text(ui, cui_string(hex), t->font_size, t->text);
    cui_end_box(ui);
    if (options.palette_count) {
        cui_begin_box(ui, cui_picker_style_id(id, 1700), (cui_box){.row = true, .gap = 4});
        for (size_t i = 0; i < options.palette_count; ++i) {
            cui_begin_box(ui, cui_color_part_id(id, 100 + (unsigned)i),
                          (cui_box){.width = 24,
                                    .height = 24,
                                    .background = options.palette[i],
                                    .border = t->border,
                                    .border_width = 1,
                                    .radius = t->radius});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return changed;
}
