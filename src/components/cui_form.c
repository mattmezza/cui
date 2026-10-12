/* SPDX-License-Identifier: MIT */
#include "cui/cui_form.h"
#include <math.h>
static float form_gap(cui_form_options o) {
    return o.size == CUI_FORM_LARGE                                ? 12
           : o.size == CUI_FORM_SMALL || o.size == CUI_FORM_XSMALL ? 6
                                                                   : 8;
}
static void label(cui_context *ui, const cui_theme *t, const cui_form_field *f, cui_form_options o,
                  float width) {
    cui_begin_box(
        ui, cui_form_label_id(f->id),
        (cui_box){
            .width = width, .row = true, .gap = 4, .align_y = CUI_ALIGN_CENTER, .clip = true});
    if (f->render_label)
        f->render_label(ui, t, f->user, width);
    else if (f->label.length)
        cui_text(ui, f->label, o.label_text_size > 0 ? o.label_text_size : t->font_size * .875f,
                 t->text);
    if (f->required && (f->label.length || f->render_label))
        cui_text(ui, cui_string("*"), t->font_size * .875f, t->danger);
    cui_end_box(ui);
}
static void field(cui_context *ui, const cui_theme *t, const cui_form_field *f, cui_form_cell cell,
                  cui_form_options o) {
    float gap = o.size == CUI_FORM_LARGE ? 8 : 4;
    float inner_gap = o.horizontal_labels ? gap : gap / 2;
    float lw = o.label_width > 0 ? o.label_width : 140;
    lw = fminf(lw, fmaxf(0, cell.width - inner_gap - 1));
    bool has_label = !f->no_label_indent;
    float control_width = cell.width - (o.horizontal_labels && has_label ? lw + inner_gap : 0);
    cui_begin_box(ui, f->id, (cui_box){.width = cell.width, .gap = gap / 2});
    cui_begin_box(
        ui, f->id ^ UINT64_C(0x77f9cf130c3825ba),
        (cui_box){.width = -1, .row = o.horizontal_labels, .gap = inner_gap, .align_y = f->align});
    if (has_label)
        label(ui, t, f, o, o.horizontal_labels ? lw : cell.width);
    cui_begin_box(ui, f->id ^ UINT64_C(0x6d83e029ab908f21), (cui_box){.width = control_width});
    if (f->render_control)
        f->render_control(ui, t, f->user, control_width);
    cui_end_box(ui);
    cui_end_box(ui);
    if (f->render_description || f->description.length || (f->invalid && f->error.length)) {
        cui_begin_box(ui, cui_form_description_id(f->id),
                      (cui_box){.width = -1, .row = o.horizontal_labels, .gap = inner_gap});
        if (has_label && o.horizontal_labels) {
            cui_begin_box(ui, 0, (cui_box){.width = lw});
            cui_end_box(ui);
        }
        cui_begin_box(ui, f->id ^ UINT64_C(0x51e73c496a39c41b), (cui_box){.width = control_width});
        if (f->invalid && f->error.length)
            cui_text(ui, f->error, t->font_size * .75f, t->danger);
        else if (f->render_description)
            f->render_description(ui, t, f->user, control_width);
        else
            cui_text(ui, f->description, t->font_size * .75f, t->muted);
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_form_associate_control(ui, f);
    cui_end_box(ui);
}
cui_form_error cui_form(cui_context *ui, cui_id id, const cui_theme *t, const cui_form_field *f,
                        size_t count, cui_form_options o) {
    if (!ui || !id || !t)
        return CUI_FORM_INVALID;
    cui_form_cell cells[256];
    size_t rows;
    cui_form_error e = cui_form_plan(f, count, o, cells, 256, &rows);
    if (e != CUI_FORM_OK)
        return e;
    for (size_t i = 0; i < count; i++)
        if (f[i].id == id)
            return CUI_FORM_DUPLICATE_ID;
    float width = o.width > 0 ? o.width : 480;
    float gx = o.gap_x > 0 ? o.gap_x : form_gap(o) * 3;
    float gy = o.gap_y > 0 ? o.gap_y : form_gap(o);
    float unit = (width - gx * ((o.columns ? o.columns : 1) - 1)) / (o.columns ? o.columns : 1);
    cui_begin_box(ui, id, (cui_box){.width = width, .gap = gy});
    e = cui_base_form(ui, id, f, count, o);
    for (size_t row = 0; row < rows; row++) {
        cui_begin_box(ui, id ^ (UINT64_C(0x514d63acb48d9587) + row),
                      (cui_box){.width = -1, .row = true, .align_y = CUI_ALIGN_START});
        size_t cursor = 0;
        for (size_t i = 0; i < count; i++)
            if (cells[i].visible && cells[i].row == row) {
                float spacer = (cells[i].column - cursor) * (unit + gx) + (cursor ? gx : 0);
                if (spacer > 0) {
                    cui_begin_box(ui, 0, (cui_box){.width = spacer});
                    cui_end_box(ui);
                }
                field(ui, t, &f[i], cells[i], o);
                cursor = cells[i].column + cells[i].span;
            }
        cui_end_box(ui);
    }
    if (o.render_footer) {
        cui_begin_box(ui, id ^ UINT64_C(0x21782c34a96dc151),
                      (cui_box){.width = -1, .row = true, .align_x = CUI_ALIGN_END});
        o.render_footer(ui, t, o.footer_user, width);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return e;
}
