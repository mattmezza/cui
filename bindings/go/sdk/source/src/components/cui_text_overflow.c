/* SPDX-License-Identifier: MIT */
#include "cui/cui_text_overflow.h"
#include "cui_text_overflow_internal.h"
#include <math.h>
#include <string.h>
typedef struct {
    cui_text_ellipsis_options options;
    size_t length;
} cui_ellipsis_payload;
typedef struct {
    cui_prepaint_writer *writer;
    cui_text_ellipsis_options options;
} cui_ellipsis_prepaint_measurement;
static float cui_ellipsis_prepaint_width(void *user, cui_str text) {
    const cui_ellipsis_prepaint_measurement *measurement = user;
    return cui_prepaint_measure(measurement->writer, text, measurement->options.font_size,
                                measurement->options.text_style)
        .width;
}
static bool cui_ellipsis_paint(const cui_prepaint_info *info, const void *data,
                               cui_prepaint_writer *writer) {
    const cui_ellipsis_payload *p = data;
    cui_str text = {(const char *)data + sizeof *p, p->length};
    cui_text_ellipsis_options o = p->options;
    if (!text.length || info->bounds.w <= 0)
        return true;
    cui_size natural = cui_prepaint_measure(writer, text, o.font_size, o.text_style);
    cui_str tail = cui_string("…");
    cui_ellipsis_prepaint_measurement measurement = {writer, o};
    cui_ellipsis_fit fit = cui_text_fit_ellipsis(text, info->bounds.w, natural.width, tail,
                                                 cui_ellipsis_prepaint_width, &measurement);
    cui_color color = o.color;
    color.a = (uint8_t)(color.a * info->opacity);
    cui_draw_command command = {
        .type = CUI_DRAW_TEXT,
        .bounds = {info->bounds.x, info->bounds.y, fit.prefix_width, natural.height},
        .color = color,
        .font_size = o.font_size,
        .text_style = o.text_style,
        .text = {text.data, fit.prefix}};
    if (fit.prefix && !cui_prepaint_emit(writer, &command, false))
        return false;
    if (fit.show_tail) {
        command.bounds.x += fit.prefix_width;
        command.bounds.w = fit.tail_width;
        command.text = tail;
        return cui_prepaint_emit(writer, &command, false);
    }
    return true;
}
bool cui_text_ellipsis(cui_context *ui, cui_id id, cui_str text, cui_text_ellipsis_options o) {
    if (ui && !id) {
        cui_diagnostic diagnostic = {.error = CUI_ERROR_INVALID_ACTION,
                                     .detail = CUI_DIAGNOSTIC_ZERO_ID,
                                     .operation = "cui_text_ellipsis",
                                     .id = id};
        cui_report_diagnostic(ui, &diagnostic);
        return false;
    }
    if (!ui || (text.length && !text.data) || !isfinite(o.width) || o.width == 0 ||
        !isfinite(o.font_size) || o.font_size <= 0 ||
        (text.length &&
         (memchr(text.data, '\n', text.length) || memchr(text.data, '\r', text.length))))
        return false;
    cui_size natural = cui_measure_text_ex(ui, text, o.font_size, o.text_style);
    cui_begin_box(ui, id,
                  (cui_box){.width = o.width,
                            .fixed_width = o.width > 0,
                            .height = natural.height,
                            .fixed_height = true,
                            .clip = true});
    cui_interact(ui, id, CUI_ROLE_LABEL, text, false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_ellipsis_payload payload = {o, text.length};
    bool ok = cui_set_prepaint_parts(ui,
                                     (cui_prepaint_options){.callback = cui_ellipsis_paint,
                                                            .payload = &payload,
                                                            .payload_bytes = sizeof payload,
                                                            .command_budget = 4},
                                     text.data, text.length);
    cui_end_box(ui);
    return ok;
}
