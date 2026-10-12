/* SPDX-License-Identifier: MIT */
#include "cui/cui_table.h"
#include <math.h>
static cui_id style_id(cui_id id, unsigned part) {
    return id ^ (UINT64_C(0x09153d4ea892181d) + part * UINT64_C(0x9e3779b97f4a7c15));
}
static cui_align align(cui_table_align a) {
    return a == CUI_TABLE_ALIGN_RIGHT    ? CUI_ALIGN_END
           : a == CUI_TABLE_ALIGN_CENTER ? CUI_ALIGN_CENTER
                                         : CUI_ALIGN_START;
}
void cui_table_begin(cui_context *ui, cui_id id, const cui_theme *t, float width) {
    if (!ui || !t)
        return;
    cui_interact(ui, id, CUI_ROLE_TABLE, cui_string("Table"), false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .background = t->surface,
                            .border = t->border,
                            .border_width = 1,
                            .radius = t->radius});
}
void cui_table_begin_row(cui_context *ui, cui_id id, const cui_theme *t, float height,
                         bool header) {
    if (!ui || !t)
        return;
    cui_interact(ui, id, CUI_ROLE_ROW, cui_string(header ? "Header" : "Row"), false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(ui, id,
                  (cui_box){.height = height,
                            .row = true,
                            .background = header ? t->background : t->surface});
}
void cui_table_begin_cell(cui_context *ui, cui_id id, float width, float height,
                          cui_table_align a) {
    cui_interact(ui, id, CUI_ROLE_CELL, cui_string("Cell"), false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .height = height,
                            .padding = 4,
                            .clip = true,
                            .align_x = align(a),
                            .align_y = CUI_ALIGN_CENTER});
}
void cui_table_end(cui_context *ui) {
    cui_end_box(ui);
}
cui_table_result cui_data_table(cui_context *ui, cui_id id, const cui_theme *t, cui_table_model *m,
                                cui_table_state *s, cui_table_options o, cui_table_label_fn label,
                                cui_table_cell_fn render, void *data) {
    if (!ui || !t || !m || !s)
        return (cui_table_result){.error = CUI_TABLE_INVALID};
    cui_table_result result = cui_table_reduce(ui, id, m, s, o);
    if (result.error != CUI_TABLE_OK)
        return result;
    float width = isfinite(o.width) && o.width > 0 ? o.width : 480,
          height = isfinite(o.height) && o.height > 0 ? o.height : 240;
    float row = isfinite(o.row_height) && o.row_height > 0 ? fminf(o.row_height, 4096) : 32,
          header = isfinite(o.header_height) && o.header_height > 0 ? o.header_height : 32;
    cui_table_view columns = cui_table_column_viewport(m, s->scroll_x, width, o.column_overscan);
    float total = columns.total - columns.before - columns.after;
    cui_table_column col;
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .background = t->surface,
                            .border = t->border,
                            .border_width = 1,
                            .radius = t->radius});
    cui_table_submit_root(ui, id, m, s, o);
    cui_begin_box(ui, style_id(id, 1),
                  (cui_box){.width = width,
                            .height = header,
                            .clip = true,
                            .offset_x = columns.before - s->scroll_x,
                            .background = t->background});
    cui_table_submit_header(ui, id, m, s, o);
    cui_begin_box(ui, style_id(id, 2), (cui_box){.width = total, .height = header, .row = true});
    for (size_t c = columns.begin; c < columns.end; c++) {
        cui_table_get_column(m, c, &col);
        cui_id cid = cui_table_header_id(id, col.key);
        cui_begin_box(ui, cid,
                      (cui_box){.width = col.width,
                                .height = header,
                                .row = true,
                                .padding = 4,
                                .gap = 2,
                                .clip = true,
                                .background = t->background,
                                .border = t->focus,
                                .border_width = cui_focused(ui) == cid ? 2 : 0});
        float grip = col.resizable ? 8 : 0;
        cui_begin_box(ui, style_id(cid, 1),
                      (cui_box){.width = fmaxf(1, col.width - 8 - grip),
                                .height = fmaxf(1, header - 8),
                                .align_x = align(col.align),
                                .align_y = CUI_ALIGN_CENTER});
        cui_text(ui, col.label, t->font_size, col.disabled || o.disabled ? t->muted : t->text);
        if (s->sort_column == col.key && s->sort != CUI_TABLE_SORT_DEFAULT)
            cui_text(ui, cui_string(s->sort == CUI_TABLE_SORT_ASCENDING ? "^" : "v"), t->font_size,
                     t->primary);
        cui_end_box(ui);
        if (col.resizable) {
            cui_begin_box(
                ui, cui_table_resize_id(id, col.key),
                (cui_box){.width = 8,
                          .height = fmaxf(1, header - 8),
                          .background = cui_focused(ui) == cui_table_resize_id(id, col.key)
                                            ? t->focus
                                            : t->border});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_table_view v = cui_table_viewport(m, s->scroll_y, height, row, o.overscan);
    cui_begin_box(ui, cui_table_body_id(id),
                  (cui_box){.width = width,
                            .height = height,
                            .clip = true,
                            .offset_x = columns.before - s->scroll_x,
                            .offset_y = v.before - v.scroll,
                            .background = t->surface});
    cui_table_submit_body(ui, id, m, s, o, label, data);
    for (size_t r = v.begin; r < v.end; r++) {
        cui_table_row rr;
        cui_table_get_row(m, r, &rr);
        cui_id rid = cui_table_row_id(id, rr.key);
        cui_begin_box(ui, rid, (cui_box){.width = total, .height = row, .row = true});
        for (size_t c = columns.begin; c < columns.end; c++) {
            cui_table_get_column(m, c, &col);
            cui_id cid = cui_table_cell_id(id, rr.key, col.key);
            bool selected = o.selection_mode == CUI_TABLE_SELECT_ROW ? s->row == rr.key
                            : o.selection_mode == CUI_TABLE_SELECT_COLUMN
                                ? s->column == col.key
                                : s->row == rr.key && s->column == col.key;
            cui_begin_box(ui, cid,
                          (cui_box){.width = col.width,
                                    .height = row,
                                    .padding = 4,
                                    .clip = true,
                                    .align_x = align(col.align),
                                    .align_y = CUI_ALIGN_CENTER,
                                    .background = selected               ? t->primary
                                                  : o.striped && (r & 1) ? t->background
                                                                         : t->surface,
                                    .border = cui_focused(ui) == cid ? t->focus : t->border,
                                    .border_width = cui_focused(ui) == cid ? 2
                                                    : o.bordered           ? 1
                                                                           : 0});
            cui_str value = label ? label(data, r, col.key) : rr.label;
            if (render)
                render(ui, t, data, r, col.key);
            else
                cui_text(ui, value, t->font_size,
                         o.disabled || rr.disabled || col.disabled ? t->muted
                         : selected                                ? t->on_primary
                                                                   : t->text);
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_table_end(ui);
    return result;
}
