/* SPDX-License-Identifier: MIT */
#include "cui/cui_table.h"
#include <math.h>
#include <stdio.h>
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
void cui_table_begin_section(cui_context *ui, cui_id id, const cui_theme *t,
                             cui_table_section section) {
    if (!ui || !t)
        return;
    cui_interact(ui, id, CUI_ROLE_GROUP,
                 cui_string(section == CUI_TABLE_SECTION_HEADER   ? "Table header"
                            : section == CUI_TABLE_SECTION_FOOTER ? "Table footer"
                                                                  : "Table body"),
                 false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(
        ui, id,
        (cui_box){.background = section == CUI_TABLE_SECTION_HEADER ? t->background : t->surface,
                  .border = t->border,
                  .border_width = section == CUI_TABLE_SECTION_FOOTER ? 1 : 0});
}
void cui_table_begin_head_cell(cui_context *ui, cui_id id, float width, float height,
                               cui_table_align a, cui_str name) {
    cui_interact(ui, id, CUI_ROLE_COLUMNHEADER, name, false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_set_semantic_details(ui, id, (cui_semantic_details){.readonly = true});
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .height = height,
                            .padding = 4,
                            .clip = true,
                            .align_x = align(a),
                            .align_y = CUI_ALIGN_CENTER});
}
bool cui_table_begin_cell_span(cui_context *ui, cui_id id, const float *widths, size_t count,
                               size_t start, size_t span, float height, cui_table_align a) {
    float width;
    if (!ui || !cui_table_span_width(widths, count, start, span, &width))
        return false;
    cui_table_begin_cell(ui, id, width, height, a);
    return true;
}
void cui_table_caption(cui_context *ui, cui_id id, const cui_theme *t, cui_str text) {
    if (!ui || !t)
        return;
    cui_interact(ui, id, CUI_ROLE_LABEL, text, false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(ui, id, (cui_box){.height = t->font_size + 12, .padding = 4});
    cui_text(ui, text, t->font_size, t->muted);
    cui_end_box(ui);
}
static void header_cell(cui_context *ui, cui_id id, const cui_theme *t, cui_table_model *m,
                        cui_table_state *s, cui_table_options o, size_t c, float header) {
    cui_table_column col;
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
    cui_table_submit_header_column(ui, id, m, s, o, c);
    float grip = col.resizable ? 8 : 0;
    cui_begin_box(ui, style_id(cid, 1),
                  (cui_box){.width = fmaxf(1, col.width - 8 - grip),
                            .height = fmaxf(1, header - 8),
                            .align_x = align(col.align),
                            .align_y = CUI_ALIGN_CENTER});
    if (o.render_header)
        o.render_header(ui, t, o.render_userdata, col.key);
    else
        cui_text(ui, col.label, t->font_size, col.disabled || o.disabled ? t->muted : t->text);
    if (s->sort_column == col.key && s->sort != CUI_TABLE_SORT_DEFAULT)
        cui_text(ui, cui_string(s->sort == CUI_TABLE_SORT_ASCENDING ? "^" : "v"), t->font_size,
                 t->primary);
    cui_end_box(ui);
    if (col.resizable) {
        cui_begin_box(ui, cui_table_resize_id(id, col.key),
                      (cui_box){.width = 8,
                                .height = fmaxf(1, header - 8),
                                .background = cui_focused(ui) == cui_table_resize_id(id, col.key)
                                                  ? t->focus
                                                  : t->border});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
static void body_cell(cui_context *ui, cui_id id, const cui_theme *t, cui_table_model *m,
                      cui_table_state *s, cui_table_options o, size_t r, size_t c, float row,
                      cui_table_label_fn label, cui_table_cell_fn render, void *data) {
    cui_table_row rr;
    cui_table_column col;
    cui_table_get_row(m, r, &rr);
    cui_table_get_column(m, c, &col);
    cui_id cid = cui_table_cell_id(id, rr.key, col.key);
    bool selected = cui_table_is_selected(s, o, rr.key, col.key);
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
    cui_table_submit_cell(ui, id, m, s, o, r, c, label, data);
    if (render)
        render(ui, t, data, r, col.key);
    else
        cui_text(ui, label ? label(data, r, col.key) : rr.label, t->font_size,
                 o.disabled || rr.disabled || col.disabled ? t->muted
                 : selected                                ? t->on_primary
                                                           : t->text);
    cui_end_box(ui);
}
cui_table_result cui_data_table(cui_context *ui, cui_id id, const cui_theme *t, cui_table_model *m,
                                cui_table_state *s, cui_table_options o, cui_table_label_fn label,
                                cui_table_cell_fn render, void *data) {
    if (!ui || !t || !m || !s)
        return (cui_table_result){.error = CUI_TABLE_INVALID};
    cui_table_result result = cui_table_reduce(ui, id, m, s, o);
    if (result.error != CUI_TABLE_OK && result.error != CUI_TABLE_CAPACITY)
        return result;
    float width = isfinite(o.width) && o.width > 0 ? o.width : 480,
          h = isfinite(o.height) && o.height > 0 ? o.height : 240;
    float row = isfinite(o.row_height) && o.row_height > 0 ? fminf(o.row_height, 4096) : 32,
          header = isfinite(o.header_height) && o.header_height > 0 ? o.header_height : 32;
    size_t fixed = cui_table_fixed_columns(m);
    float pin = cui_table_fixed_width(m), row_header = cui_table_row_header_width(o),
          available = fmaxf(1, width - pin - row_header);
    cui_table_view cols = cui_table_column_viewport(m, s->scroll_x, width - row_header,
                                                    o.column_overscan),
                   rows = cui_table_viewport(m, s->scroll_y, h, row, o.overscan);
    float span = cols.total - cols.before - cols.after;
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
                            .row = true,
                            .clip = true,
                            .background = t->background});
    if (row_header) {
        cui_begin_box(
            ui, style_id(id, 5),
            (cui_box){.width = row_header, .height = header, .background = t->background});
        cui_end_box(ui);
    }
    if (fixed) {
        cui_begin_box(ui, style_id(id, 3),
                      (cui_box){.width = pin, .height = header, .row = true, .clip = true});
        for (size_t c = 0; c < fixed; c++)
            header_cell(ui, id, t, m, s, o, c, header);
        cui_end_box(ui);
    }
    cui_begin_box(ui, style_id(id, 4),
                  (cui_box){.width = available,
                            .height = header,
                            .clip = true,
                            .offset_x = cols.before - s->scroll_x});
    cui_begin_box(ui, style_id(id, 2), (cui_box){.width = span, .height = header, .row = true});
    for (size_t c = cols.begin; c < cols.end; c++)
        header_cell(ui, id, t, m, s, o, c, header);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, cui_table_body_id(id),
                  (cui_box){.width = width,
                            .height = h,
                            .clip = true,
                            .offset_y = o.loading ? 0 : rows.before - rows.scroll,
                            .background = t->surface});
    cui_table_submit_body_root(ui, id, m, s, o);
    if (o.loading || !cui_table_rows(m)) {
        cui_id placeholder = cui_table_body_id(id) ^ UINT64_C(0x790416281c39bdd1);
        cui_begin_box(ui, placeholder,
                      (cui_box){.width = width,
                                .height = h,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_interact(ui, placeholder, CUI_ROLE_STATUS,
                     cui_string(o.loading ? "Loading rows" : "No rows"), false, false, 0);
        cui_set_tab_stop(ui, placeholder, false);
        cui_set_semantic_details(
            ui, placeholder,
            (cui_semantic_details){.parent = cui_table_body_id(id), .busy = o.loading});
        cui_table_placeholder_fn placeholder_render = o.loading ? o.render_loading : o.render_empty;
        if (placeholder_render)
            placeholder_render(ui, t, o.render_userdata);
        else
            cui_text(ui, cui_string(o.loading ? "Loading rows" : "No rows"), t->font_size,
                     t->muted);
        cui_end_box(ui);
    }
    for (size_t r = rows.begin; r < rows.end && !o.loading; r++) {
        cui_table_row rr;
        cui_table_get_row(m, r, &rr);
        cui_id rid = cui_table_row_id(id, rr.key);
        cui_begin_box(ui, rid, (cui_box){.width = width, .height = row, .row = true});
        cui_table_submit_row(ui, id, m, s, o, r);
        if (row_header) {
            cui_id row_id = cui_table_row_header_id(id, rr.key);
            cui_begin_box(ui, row_id,
                          (cui_box){.width = row_header,
                                    .height = row,
                                    .padding = 4,
                                    .clip = true,
                                    .background = t->background,
                                    .border = t->focus,
                                    .border_width = cui_focused(ui) == row_id ? 2 : 0});
            char number[32];
            snprintf(number, sizeof(number), "%zu", r + 1);
            if (o.render_row_header)
                o.render_row_header(ui, t, o.render_userdata, r);
            else
                cui_text(ui, cui_string(number), t->font_size,
                         o.disabled || rr.disabled ? t->muted : t->text);
            cui_end_box(ui);
        }
        if (fixed) {
            cui_begin_box(ui, style_id(rid, 3),
                          (cui_box){.width = pin, .height = row, .row = true, .clip = true});
            for (size_t c = 0; c < fixed; c++)
                body_cell(ui, id, t, m, s, o, r, c, row, label, render, data);
            cui_end_box(ui);
        }
        cui_begin_box(ui, style_id(rid, 4),
                      (cui_box){.width = available,
                                .height = row,
                                .clip = true,
                                .offset_x = cols.before - s->scroll_x});
        cui_begin_box(ui, style_id(rid, 2), (cui_box){.width = span, .height = row, .row = true});
        for (size_t c = cols.begin; c < cols.end; c++)
            body_cell(ui, id, t, m, s, o, r, c, row, label, render, data);
        cui_end_box(ui);
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    return result;
}
