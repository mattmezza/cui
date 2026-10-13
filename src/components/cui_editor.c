/* SPDX-License-Identifier: MIT */
#include "cui/cui_editor.h"
#include <math.h>
#include <stdio.h>
static cui_id cui_editor_child(cui_id id, size_t row, unsigned part) {
    return id ^ UINT64_C(0x7cf5d0e25db29861) ^
           ((uint64_t)(row + 1) * UINT64_C(0x9e3779b97f4a7c15)) ^ part;
}
static size_t cui_editor_visible_rows(cui_textarea_options options, size_t count) {
    if (!options.auto_grow)
        return options.rows ? options.rows : 1;
    size_t minimum = options.min_rows ? options.min_rows : 1;
    size_t maximum = options.max_rows ? options.max_rows : 10;
    if (maximum < minimum)
        maximum = minimum;
    return count < minimum ? minimum : count > maximum ? maximum : count;
}
static float cui_editor_prefix(cui_context *ui, cui_str text, cui_textarea_line line, size_t offset,
                               float font) {
    size_t end = offset < line.start ? line.start : offset > line.end ? line.end : offset;
    return cui_measure_text(ui, (cui_str){text.data + line.start, end - line.start}, font).width;
}
cui_textarea_result cui_editor(cui_context *ui, cui_id id, const cui_theme *theme, cui_str name,
                               cui_editor_state *editor, cui_editor_options config) {
    if (!editor || !editor->edit || (config.language.length && !config.language.data))
        return (cui_textarea_result){.error = CUI_TEXTAREA_INVALID};
    cui_edit *edit = editor->edit;
    cui_textarea_state *state = &editor->view;
    cui_textarea_options options = config.textarea;
    cui_textarea_result result = {.error = CUI_TEXTAREA_INVALID};
    cui_textarea_metrics metrics;
    if (!ui || !id || !theme || !edit || !state ||
        !cui_textarea_measure(options, theme->font_size, &metrics) || !isfinite(options.width) ||
        !isfinite(options.height) || options.height < 0 || options.height > 1000000 ||
        options.rows > 65536 || options.min_rows > 65536 || options.max_rows > 65536 ||
        (options.placeholder.length && !options.placeholder.data))
        return result;
    if (options.design_size != CUI_DESIGN_INHERIT) {
        cui_design_metrics design;
        if (!cui_design_measure(options.design_size, theme->font_size / 16, &design))
            return result;
        metrics = (cui_textarea_metrics){design.font_size, design.line_height, design.padding_x,
                                         design.padding_y};
    }
    float border = !options.no_appearance && !options.no_border ? 1 : 0;
    size_t logical_count = 1;
    cui_str document = cui_edit_get_text(edit);
    for (size_t i = 0; i < document.length; ++i)
        if (document.data[i] == '\n')
            ++logical_count;
    char digits[32];
    int digit_count = snprintf(digits, sizeof digits, "%zu", logical_count);
    float gutter =
        config.line_numbers
            ? cui_measure_text(ui, (cui_str){digits, (size_t)digit_count}, metrics.font_size)
                      .width +
                  16
            : 0;
    float px = metrics.padding_x + border, py = metrics.padding_y + border;
    float width = options.width > 0       ? options.width - 2 * px - gutter
                  : state->viewport.w > 0 ? state->viewport.w
                                          : 280 - 2 * px - gutter;
    width = fmaxf(1, width);
    float height =
        options.height > 0
            ? options.height
            : cui_editor_visible_rows(options, state->count) * metrics.line_height + 2 * py;
    if (!isfinite(height) || height > 1000000)
        return result;
    state->viewport.w = width;
    state->viewport.h = fmaxf(0, height - 2 * py);
    result = cui_base_textarea(ui, id, name, edit, state, options, metrics);
    if (result.error != CUI_TEXTAREA_OK)
        return result;
    height = options.height > 0
                 ? options.height
                 : cui_editor_visible_rows(options, state->count) * metrics.line_height + 2 * py;
    if (!isfinite(height) || height > 1000000)
        return (cui_textarea_result){.error = CUI_TEXTAREA_INVALID};
    state->viewport.h = fmaxf(0, height - 2 * py);
    result.error = cui_textarea_layout(ui, edit, state, metrics, width, !options.no_wrap);
    if (result.error != CUI_TEXTAREA_OK)
        return result;
    bool focused = cui_focused(ui) == id && !options.disabled;
    if ((focused && !state->focused) || result.changed || result.selection_changed)
        state->blink_started = cui_frame_time(ui);
    state->focused = focused;
    bool caret = focused && (cui_reduced_motion(ui) ||
                             fmod(cui_frame_time(ui) - state->blink_started, 1) < .5);
    if (focused && !cui_reduced_motion(ui))
        cui_invalidate(ui);
    cui_color background = theme->input_background;
    if (options.disabled) {
        background = theme->input;
        background.a = (uint8_t)(background.a * .2f);
    }
    cui_color foreground = options.disabled ? theme->muted : theme->text;
    cui_begin_box(ui, id,
                  (cui_box){.width = options.width ? options.width : 280,
                            .height = height,
                            .fixed_height = true,
                            .clip = true,
                            .radius = options.no_appearance ? 0 : theme->radius,
                            .background = options.no_appearance ? (cui_color){0} : background,
                            .border = options.invalid ? theme->danger
                                      : focused       ? theme->focus
                                                      : theme->input,
                            .border_width = border});
    cui_begin_box(ui, cui_editor_child(id, 0, 1), (cui_box){.height = py, .fixed_height = true});
    cui_end_box(ui);
    cui_begin_box(
        ui, cui_editor_child(id, 0, 2),
        (cui_box){.width = -1, .row = true, .height = state->viewport.h, .fixed_height = true});
    cui_begin_box(ui, cui_editor_child(id, 0, 3), (cui_box){.width = px, .fixed_width = true});
    cui_end_box(ui);
    if (config.line_numbers) {
        cui_begin_box(
            ui, id ^ UINT64_C(0x147dc03f2198af04),
            (cui_box){
                .width = gutter, .fixed_width = true, .height = state->viewport.h, .clip = true});
        size_t logical = 1;
        for (size_t row = 0; row < state->count; ++row) {
            if (row == 0 || state->lines[row - 1].hard_break) {
                char number[32];
                int n = snprintf(number, sizeof number, "%zu", logical++);
                float y = row * metrics.line_height - state->scroll_y;
                if (y + metrics.line_height > 0 && y < state->viewport.h) {
                    cui_begin_absolute_box(ui, cui_editor_child(id, row, 20), (cui_box){0},
                                           (cui_rect){0, y, gutter, metrics.line_height});
                    cui_text_with_line_height(ui, (cui_str){number, (size_t)n}, metrics.font_size,
                                              theme->muted, metrics.line_height);
                    cui_end_box(ui);
                }
            }
        }
        cui_end_box(ui);
    }
    cui_begin_box(ui, cui_textarea_viewport_id(id),
                  (cui_box){.width = -1,
                            .height = state->viewport.h,
                            .fixed_height = true,
                            .clip = true,
                            .offset_x = -state->scroll_x,
                            .offset_y = -state->scroll_y});
    cui_track_bounds(ui, cui_textarea_viewport_id(id), &state->viewport);
    float content_width = width + state->maximum_x;
    cui_begin_box(ui, cui_editor_child(id, 0, 4),
                  (cui_box){.width = content_width,
                            .fixed_width = true,
                            .height = state->count * metrics.line_height,
                            .fixed_height = true});
    cui_str text = cui_edit_get_text(edit);
    cui_edit_state selection = cui_edit_get_state(edit);
    size_t first = selection.anchor < selection.cursor ? selection.anchor : selection.cursor;
    size_t last = selection.anchor > selection.cursor ? selection.anchor : selection.cursor;
    size_t begin = (size_t)floorf(state->scroll_y / metrics.line_height);
    if (begin >= state->count)
        begin = state->count - 1;
    size_t visible = (size_t)ceilf(state->viewport.h / metrics.line_height) + 2;
    size_t end = visible > state->count - begin ? state->count : begin + visible;
    if (begin) {
        cui_begin_box(ui, cui_editor_child(id, begin, 5),
                      (cui_box){.height = begin * metrics.line_height, .fixed_height = true});
        cui_end_box(ui);
    }
    for (size_t row = begin; row < end; row++) {
        cui_textarea_line line = state->lines[row];
        cui_begin_box(ui, cui_editor_child(id, row, 10),
                      (cui_box){.width = content_width,
                                .fixed_width = true,
                                .height = metrics.line_height,
                                .fixed_height = true});
        if (first < line.next && last > line.start) {
            float left = cui_editor_prefix(ui, text, line, first, metrics.font_size);
            float right = cui_editor_prefix(ui, text, line, last, metrics.font_size);
            if (line.hard_break && last > line.end)
                right += cui_measure_text(ui, cui_string(" "), metrics.font_size).width;
            if (right > left)
                cui_set_underlay(ui, (cui_rect){left, 0, right - left, metrics.line_height},
                                 theme->selection, (cui_corner_radii){0});
        }
        if (!text.length && options.placeholder.length)
            cui_text_with_line_height(ui, options.placeholder, metrics.font_size, theme->muted,
                                      metrics.line_height);
        else if (line.end > line.start) {
            size_t start = line.start;
            while (start < line.end) {
                cui_color color = foreground;
                size_t next = config.highlight
                                  ? config.highlight(config.highlight_user, config.language, text,
                                                     start, line.end, &color)
                                  : line.end;
                if (next <= start || next > line.end ||
                    (next < text.length && ((unsigned char)text.data[next] & 0xc0) == 0x80)) {
                    next = line.end;
                    color = foreground;
                }
                float x = cui_editor_prefix(ui, text, line, start, metrics.font_size);
                cui_begin_absolute_box(ui, cui_editor_child(id, start, 30), (cui_box){0},
                                       (cui_rect){x, 0, content_width - x, metrics.line_height});
                cui_text_with_line_height(ui, (cui_str){text.data + start, next - start},
                                          metrics.font_size, color, metrics.line_height);
                cui_end_box(ui);
                start = next;
            }
        }
        if (caret && row == state->caret_line) {
            float x = cui_editor_prefix(ui, text, line, selection.cursor, metrics.font_size);
            cui_begin_absolute_box(ui, cui_editor_child(id, row, 11),
                                   (cui_box){.background = theme->caret},
                                   (cui_rect){x, 0, 1, metrics.line_height});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    if (end < state->count) {
        cui_begin_box(
            ui, cui_editor_child(id, end, 12),
            (cui_box){.height = (state->count - end) * metrics.line_height, .fixed_height = true});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, cui_editor_child(id, 0, 6), (cui_box){.width = px, .fixed_width = true});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, cui_editor_child(id, 0, 7), (cui_box){.height = py, .fixed_height = true});
    cui_end_box(ui);
    cui_end_box(ui);
    return result;
}
