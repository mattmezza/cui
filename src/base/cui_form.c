/* SPDX-License-Identifier: MIT */
#include "cui/cui_form.h"
#include <math.h>
static size_t columns(cui_form_options o) {
    return o.columns ? o.columns : 1;
}
static float width(cui_form_options o) {
    return o.width > 0 ? o.width : 480;
}
static float gap(cui_form_options o) {
    return o.size == CUI_FORM_LARGE                                ? 12
           : o.size == CUI_FORM_SMALL || o.size == CUI_FORM_XSMALL ? 6
                                                                   : 8;
}
cui_form_options cui_form_options_default(void) {
    return (cui_form_options){.columns = 1, .label_width = 140};
}
cui_id cui_form_label_id(cui_id id) {
    return id ^ UINT64_C(0x3a468db90d956242);
}
cui_id cui_form_description_id(cui_id id) {
    return id ^ UINT64_C(0xd29c8f326b970fea);
}
static int line(int value, size_t n) {
    return value < 0 ? (int)n + 2 + value : value;
}
static bool string_valid(cui_str s) {
    return !s.length || s.data;
}
cui_form_error cui_form_plan(const cui_form_field *f, size_t count, cui_form_options o,
                             cui_form_cell *cells, size_t capacity, size_t *rows) {
    size_t n = columns(o);
    if ((count && (!f || !cells)) || !rows || n > 64 || count > 256 || !isfinite(o.width) ||
        !isfinite(o.label_width) || !isfinite(o.label_text_size) || !isfinite(o.gap_x) ||
        !isfinite(o.gap_y) || o.width < 0 || o.label_width < 0 || o.label_text_size < 0 ||
        o.gap_x < 0 || o.gap_y < 0 || o.size < CUI_FORM_MEDIUM || o.size > CUI_FORM_LARGE ||
        !string_valid(o.name))
        return CUI_FORM_INVALID;
    if (capacity < count)
        return CUI_FORM_CAPACITY;
    float gx = o.gap_x > 0 ? o.gap_x : gap(o) * 3;
    if (width(o) <= gx * (n - 1))
        return CUI_FORM_INVALID;
    for (size_t i = 0; i < count; i++) {
        if (!f[i].id || !string_valid(f[i].label) || !string_valid(f[i].description) ||
            !string_valid(f[i].error) || f[i].align < CUI_ALIGN_START || f[i].align > CUI_ALIGN_END)
            return CUI_FORM_INVALID;
        int start = line(f[i].column_start, n), end = line(f[i].column_end, n);
        size_t span = f[i].column_span ? f[i].column_span : 1;
        if (span > n || (start && (start < 1 || start > (int)n)) ||
            (end && (end < 2 || end > (int)n + 1)) || (start && end && end <= start) ||
            (end && !start && end - (int)span < 1) ||
            (start && !end && (size_t)(start - 1) + span > n))
            return CUI_FORM_INVALID;
        for (size_t j = 0; j < i; j++)
            if (f[j].id == f[i].id)
                return CUI_FORM_DUPLICATE_ID;
    }
    size_t row = 0, cursor = 0, visible = 0;
    float unit = (width(o) - gx * (n - 1)) / n;
    for (size_t i = 0; i < count; i++) {
        cells[i] = (cui_form_cell){0};
        if (f[i].hidden)
            continue;
        int start = line(f[i].column_start, n), end = line(f[i].column_end, n);
        size_t span = f[i].column_span ? f[i].column_span : 1;
        if (start && end)
            span = (size_t)(end - start);
        else if (end)
            start = end - (int)span;
        size_t col = start ? (size_t)(start - 1) : cursor;
        if (col < cursor || col + span > n) {
            row++;
            cursor = 0;
            col = start ? (size_t)(start - 1) : 0;
        }
        cells[i] = (cui_form_cell){row, col, span, unit * span + gx * (span - 1), true};
        cursor = col + span;
        visible++;
    }
    *rows = visible ? row + 1 : 0;
    return CUI_FORM_OK;
}
void cui_form_associate_control(cui_context *ui, const cui_form_field *f) {
    if (!ui || !f || !f->control)
        return;
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (sem[i].id == f->control) {
            cui_semantic_details d = sem[i].details;
            d.parent = f->id;
            d.labelled_by = !f->no_label_indent && (f->label.length || f->render_label)
                                ? cui_form_label_id(f->id)
                                : 0;
            d.required |= f->required;
            d.invalid |= f->invalid;
            d.readonly |= f->readonly;
            d.description = f->invalid && f->error.length ? f->error : f->description;
            cui_set_semantic_details(ui, f->control, d);
            return;
        }
}
cui_form_error cui_base_form(cui_context *ui, cui_id id, const cui_form_field *f, size_t count,
                             cui_form_options o) {
    cui_form_cell cells[256];
    size_t rows;
    cui_form_error e = cui_form_plan(f, count, o, cells, 256, &rows);
    if (e != CUI_FORM_OK)
        return e;
    if (!ui || !id)
        return CUI_FORM_INVALID;
    for (size_t i = 0; i < count; i++)
        if (f[i].id == id)
            return CUI_FORM_DUPLICATE_ID;
    (void)cui_interact(ui, id, CUI_ROLE_GROUP, o.name, false, false, 0);
    for (size_t i = 0; i < count; i++)
        if (!f[i].hidden) {
            (void)cui_interact(ui, f[i].id, CUI_ROLE_GROUP, f[i].label, f[i].disabled, false, 0);
            cui_set_semantic_details(
                ui, f[i].id,
                (cui_semantic_details){.parent = id,
                                       .required = f[i].required,
                                       .invalid = f[i].invalid,
                                       .readonly = f[i].readonly,
                                       .controls = f[i].control,
                                       .description = f[i].invalid && f[i].error.length
                                                          ? f[i].error
                                                          : f[i].description});
            if (!f[i].no_label_indent && (f[i].label.length || f[i].render_label)) {
                (void)cui_interact(ui, cui_form_label_id(f[i].id), CUI_ROLE_GROUP, f[i].label,
                                   false, false, 0);
                cui_set_semantic_details(ui, cui_form_label_id(f[i].id),
                                         (cui_semantic_details){.parent = f[i].id});
            }
            cui_form_associate_control(ui, &f[i]);
        }
    return CUI_FORM_OK;
}
