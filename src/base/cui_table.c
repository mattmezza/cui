/* SPDX-License-Identifier: MIT */
#include "cui/cui_table.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct cui_table_model {
    size_t capacity, slots, count, columns, active;
    const cui_table_row *rows;
    cui_table_column *cols;
    cui_id *keys[2];
    size_t *indices[2];
};
static uint64_t mix(uint64_t x) {
    x ^= x >> 30;
    x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27;
    x *= UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}
cui_id cui_table_row_id(cui_id id, cui_id row) {
    return id ^ mix(row) ^ UINT64_C(0x42a275ee7923b113);
}
cui_id cui_table_header_id(cui_id id, cui_id col) {
    return id ^ mix(col) ^ UINT64_C(0x82a275ee7923b113);
}
cui_id cui_table_resize_id(cui_id id, cui_id col) {
    return cui_table_header_id(id, col) ^ UINT64_C(0x22a275ee7923b113);
}
cui_id cui_table_cell_id(cui_id id, cui_id row, cui_id col) {
    return cui_table_row_id(id, row) ^ mix(col ^ UINT64_C(0xa3aa29835af2341d));
}
cui_id cui_table_body_id(cui_id id) {
    return id ^ UINT64_C(0x49c42a1399ffe201);
}
cui_table_model *cui_table_create(size_t capacity) {
    if (!capacity || capacity > 1000000)
        return NULL;
    cui_table_model *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;
    m->capacity = capacity;
    m->slots = 1;
    while (m->slots < capacity * 2)
        m->slots *= 2;
    for (size_t b = 0; b < 2; b++) {
        m->keys[b] = calloc(m->slots, sizeof(cui_id));
        m->indices[b] = calloc(m->slots, sizeof(size_t));
        if (!m->keys[b] || !m->indices[b]) {
            cui_table_destroy(m);
            return NULL;
        }
    }
    return m;
}
void cui_table_destroy(cui_table_model *m) {
    if (m) {
        for (size_t b = 0; b < 2; b++) {
            free(m->keys[b]);
            free(m->indices[b]);
        }
        free(m);
    }
}
cui_table_error cui_table_bind(cui_table_model *m, const cui_table_row *rows, size_t n,
                               cui_table_column *cols, size_t nc) {
    if (!m || (n && !rows) || !cols || !nc || nc > 128)
        return CUI_TABLE_INVALID;
    if (n > m->capacity)
        return CUI_TABLE_CAPACITY;
    for (size_t c = 0; c < nc; c++) {
        if (!cols[c].key || !isfinite(cols[c].width) || cols[c].width < 16 ||
            cols[c].width > 65536 || !isfinite(cols[c].minimum) || !isfinite(cols[c].maximum) ||
            cols[c].minimum < 0 || cols[c].maximum < 0 ||
            (cols[c].maximum && cols[c].maximum < fmaxf(16, cols[c].minimum)) ||
            (cols[c].label.length && !cols[c].label.data))
            return CUI_TABLE_INVALID;
        for (size_t p = 0; p < c; p++)
            if (cols[p].key == cols[c].key)
                return CUI_TABLE_DUPLICATE_KEY;
    }
    size_t b = 1 - m->active;
    memset(m->keys[b], 0, m->slots * sizeof(cui_id));
    for (size_t r = 0; r < n; r++) {
        if (!rows[r].key || (rows[r].label.length && !rows[r].label.data))
            return CUI_TABLE_INVALID;
        size_t slot = (size_t)mix(rows[r].key) & (m->slots - 1);
        while (m->keys[b][slot]) {
            if (m->keys[b][slot] == rows[r].key)
                return CUI_TABLE_DUPLICATE_KEY;
            slot = (slot + 1) & (m->slots - 1);
        }
        m->keys[b][slot] = rows[r].key;
        m->indices[b][slot] = r;
    }
    m->active = b;
    m->rows = rows;
    m->count = n;
    m->cols = cols;
    m->columns = nc;
    return CUI_TABLE_OK;
}
size_t cui_table_rows(const cui_table_model *m) {
    return m ? m->count : 0;
}
size_t cui_table_columns(const cui_table_model *m) {
    return m ? m->columns : 0;
}
bool cui_table_get_row(const cui_table_model *m, size_t r, cui_table_row *out) {
    if (!m || !out || r >= m->count)
        return false;
    *out = m->rows[r];
    return true;
}
bool cui_table_get_column(const cui_table_model *m, size_t c, cui_table_column *out) {
    if (!m || !out || c >= m->columns)
        return false;
    *out = m->cols[c];
    return true;
}
bool cui_table_row_index(const cui_table_model *m, cui_id key, size_t *out) {
    if (!m || !key || !out)
        return false;
    size_t slot = (size_t)mix(key) & (m->slots - 1);
    while (m->keys[m->active][slot]) {
        if (m->keys[m->active][slot] == key) {
            *out = m->indices[m->active][slot];
            return true;
        }
        slot = (slot + 1) & (m->slots - 1);
    }
    return false;
}
static size_t column(const cui_table_model *m, cui_id key) {
    for (size_t c = 0; c < m->columns; c++)
        if (m->cols[c].key == key)
            return c;
    return SIZE_MAX;
}
bool cui_table_resize(cui_table_model *m, cui_id key, float width) {
    if (!m || !isfinite(width))
        return false;
    size_t c = column(m, key);
    if (c == SIZE_MAX || !m->cols[c].resizable || m->cols[c].disabled)
        return false;
    float lo = fmaxf(16, m->cols[c].minimum),
          hi = m->cols[c].maximum ? fminf(65536, m->cols[c].maximum) : 65536;
    width = fminf(hi, fmaxf(lo, width));
    if (width == m->cols[c].width)
        return false;
    m->cols[c].width = width;
    return true;
}
bool cui_table_move_column(cui_table_model *m, cui_id key, size_t dest) {
    if (!m || dest >= m->columns)
        return false;
    size_t c = column(m, key);
    if (c == SIZE_MAX || c == dest || !m->cols[c].movable || m->cols[c].disabled)
        return false;
    for (size_t i = c < dest ? c : dest; i <= (c > dest ? c : dest); i++)
        if (!m->cols[i].movable)
            return false;
    cui_table_column save = m->cols[c];
    if (c < dest)
        memmove(m->cols + c, m->cols + c + 1, (dest - c) * sizeof(save));
    else
        memmove(m->cols + dest + 1, m->cols + dest, (c - dest) * sizeof(save));
    m->cols[dest] = save;
    return true;
}
static float row_height(cui_table_options o) {
    return isfinite(o.row_height) && o.row_height > 0 ? fminf(o.row_height, 4096) : 32;
}
static float height(cui_table_options o) {
    return isfinite(o.height) && o.height > 0 ? o.height : 240;
}
cui_table_view cui_table_viewport(const cui_table_model *m, float scroll, float h, float row,
                                  size_t over) {
    cui_table_view v = {0};
    if (!m)
        return v;
    row = isfinite(row) && row > 0 ? fminf(row, 4096) : 32;
    h = isfinite(h) && h > 0 ? h : 240;
    v.total = (float)m->count * row;
    v.scroll = isfinite(scroll) ? fmaxf(0, fminf(scroll, fmaxf(0, v.total - h))) : 0;
    size_t first = (size_t)floorf(v.scroll / row), last = (size_t)ceilf((v.scroll + h) / row);
    if (last > m->count)
        last = m->count;
    v.begin = first > over ? first - over : 0;
    v.end = over > m->count - last ? m->count : last + over;
    v.before = (float)v.begin * row;
    v.after = (float)(m->count - v.end) * row;
    return v;
}
cui_table_view cui_table_column_viewport(const cui_table_model *m, float scroll, float width,
                                         size_t over) {
    cui_table_view v = {0};
    if (!m)
        return v;
    width = isfinite(width) && width > 0 ? width : 480;
    for (size_t c = 0; c < m->columns; c++)
        v.total += m->cols[c].width;
    v.scroll = isfinite(scroll) ? fmaxf(0, fminf(scroll, fmaxf(0, v.total - width))) : 0;
    size_t first = 0, last = 0;
    float x = 0;
    for (size_t c = 0; c < m->columns; c++) {
        float end = x + m->cols[c].width;
        if (end <= v.scroll)
            first = c + 1;
        if (x < v.scroll + width)
            last = c + 1;
        x = end;
    }
    v.begin = first > over ? first - over : 0;
    v.end = over > m->columns - last ? m->columns : last + over;
    x = 0;
    for (size_t c = 0; c < m->columns; c++) {
        if (c < v.begin)
            v.before += m->cols[c].width;
        if (c >= v.end)
            v.after += m->cols[c].width;
    }
    return v;
}
static void select(cui_table_state *s, cui_table_options o, cui_id row, cui_id col) {
    s->row = o.selection_mode == CUI_TABLE_SELECT_COLUMN ? 0 : row;
    s->column = o.selection_mode == CUI_TABLE_SELECT_ROW ? 0 : col;
}
static size_t enabled_row(const cui_table_model *m, size_t start, int direction, bool loop) {
    if (!m->count)
        return SIZE_MAX;
    size_t p = start;
    for (size_t i = 0; i < m->count; i++) {
        if (direction < 0) {
            if (!p) {
                if (!loop)
                    return SIZE_MAX;
                p = m->count;
            }
            --p;
        } else {
            if (p + 1 >= m->count) {
                if (!loop)
                    return SIZE_MAX;
                p = 0;
            } else
                ++p;
        }
        if (!m->rows[p].disabled)
            return p;
    }
    return SIZE_MAX;
}
static size_t enabled_col(const cui_table_model *m, size_t start, int direction, bool loop) {
    size_t p = start;
    for (size_t i = 0; i < m->columns; i++) {
        if (direction < 0) {
            if (!p) {
                if (!loop)
                    return SIZE_MAX;
                p = m->columns;
            }
            --p;
        } else {
            if (p + 1 >= m->columns) {
                if (!loop)
                    return SIZE_MAX;
                p = 0;
            } else
                ++p;
        }
        if (!m->cols[p].disabled)
            return p;
    }
    return SIZE_MAX;
}
cui_table_result cui_table_reduce(cui_context *ui, cui_id id, cui_table_model *m,
                                  cui_table_state *s, cui_table_options o) {
    cui_table_result out = {0};
    if (!ui || !m || !s || !m->columns) {
        out.error = CUI_TABLE_INVALID;
        return out;
    }
    cui_id oldrow = s->row, oldcol = s->column;
    size_t r = 0, c = column(m, s->cursor_column);
    if (!cui_table_row_index(m, s->cursor_row, &r) || m->rows[r].disabled) {
        s->cursor_row = 0;
        for (r = 0; r < m->count; r++)
            if (!m->rows[r].disabled) {
                s->cursor_row = m->rows[r].key;
                break;
            }
    }
    if (c == SIZE_MAX || m->cols[c].disabled) {
        s->cursor_column = 0;
        for (c = 0; c < m->columns; c++)
            if (!m->cols[c].disabled) {
                s->cursor_column = m->cols[c].key;
                break;
            }
    }
    size_t header_column = column(m, s->header_cursor);
    if (header_column == SIZE_MAX || m->cols[header_column].disabled) {
        s->header_cursor = 0;
        for (size_t p = 0; p < m->columns; p++)
            if (!m->cols[p].disabled) {
                s->header_cursor = m->cols[p].key;
                break;
            }
    }
    bool reveal_header = false;
    size_t count;
    const cui_event *ev = cui_frame_events(ui, &count);
    cui_id previous = 0;
    bool reveal = false;
    for (size_t e = 0; e < count && !o.disabled; e++) {
        cui_id target = cui_event_target(ui, e), activation = cui_event_activation(ui, e);
        if (ev[e].type == CUI_EVENT_CANCEL) {
            s->resizing = 0;
            continue;
        }
        if (s->resizing) {
            if (ev[e].type == CUI_EVENT_POINTER_MOVE && isfinite(ev[e].x))
                out.columns_changed |=
                    cui_table_resize(m, s->resizing, s->resize_width + ev[e].x - s->resize_x);
            if (ev[e].type == CUI_EVENT_POINTER_UP && ev[e].pointer_button == CUI_POINTER_PRIMARY)
                s->resizing = 0;
        }
        bool header = false, hit = false;
        size_t hc = SIZE_MAX, hr = SIZE_MAX;
        for (size_t p = 0; p < m->columns; p++) {
            if (target == cui_table_resize_id(id, m->cols[p].key)) {
                if (ev[e].type == CUI_EVENT_POINTER_DOWN &&
                    ev[e].pointer_button == CUI_POINTER_PRIMARY && m->cols[p].resizable &&
                    !m->cols[p].disabled) {
                    s->resizing = m->cols[p].key;
                    s->resize_x = ev[e].x;
                    s->resize_width = m->cols[p].width;
                }
                if (ev[e].type == CUI_EVENT_KEY_DOWN &&
                    (ev[e].key == CUI_KEY_LEFT || ev[e].key == CUI_KEY_RIGHT))
                    out.columns_changed |= cui_table_resize(
                        m, m->cols[p].key, m->cols[p].width + (ev[e].key == CUI_KEY_LEFT ? -8 : 8));
            }
            if (target == cui_table_header_id(id, m->cols[p].key)) {
                header = hit = true;
                hc = p;
                break;
            }
        }
        if (!hit) {
            cui_table_view view =
                cui_table_viewport(m, s->scroll_y, height(o), row_height(o), o.overscan);
            for (size_t p = view.begin; p < view.end && !hit; p++)
                for (size_t q = 0; q < m->columns; q++)
                    if (target == cui_table_cell_id(id, m->rows[p].key, m->cols[q].key)) {
                        hit = true;
                        hr = p;
                        hc = q;
                        break;
                    }
        }
        if (hit && hc < m->columns && !m->cols[hc].disabled) {
            if (ev[e].type == CUI_EVENT_POINTER_DOWN &&
                ev[e].pointer_button == CUI_POINTER_SECONDARY &&
                (header || !m->rows[hr].disabled)) {
                out.context_requested = true;
                out.context_column = m->cols[hc].key;
                out.context_row = header ? 0 : m->rows[hr].key;
            }
            if (header) {
                if (previous != target)
                    s->header_cursor = m->cols[hc].key;
                previous = target;
                if (ev[e].type == CUI_EVENT_KEY_DOWN && !(ev[e].modifiers & CUI_MOD_CTRL)) {
                    size_t current = column(m, s->header_cursor), next = SIZE_MAX;
                    if (ev[e].key == CUI_KEY_LEFT || ev[e].key == CUI_KEY_RIGHT)
                        next = enabled_col(m, current, ev[e].key == CUI_KEY_LEFT ? -1 : 1,
                                           o.loop_selection);
                    else if (ev[e].key == CUI_KEY_HOME || ev[e].key == CUI_KEY_END) {
                        for (size_t p = 0; p < m->columns; p++) {
                            size_t n = ev[e].key == CUI_KEY_HOME ? p : m->columns - 1 - p;
                            if (!m->cols[n].disabled) {
                                next = n;
                                break;
                            }
                        }
                    }
                    if (next != SIZE_MAX) {
                        s->header_cursor = m->cols[next].key;
                        reveal_header = true;
                        cui_focus(ui, cui_table_header_id(id, s->header_cursor));
                        if (!o.manual_selection && o.selection_mode == CUI_TABLE_SELECT_COLUMN)
                            select(s, o, 0, s->header_cursor);
                    }
                }
                if (activation == target &&
                    (ev[e].type == CUI_EVENT_KEY_DOWN || ev[e].type == CUI_EVENT_KEY_UP))
                    hc = column(m, s->header_cursor);
                if (activation == target && o.selection_mode == CUI_TABLE_SELECT_COLUMN)
                    select(s, o, 0, m->cols[hc].key);
                if (activation == target && m->cols[hc].sortable) {
                    s->sort = s->sort_column == m->cols[hc].key
                                  ? (cui_table_sort)((s->sort + 1) % 3)
                                  : CUI_TABLE_SORT_ASCENDING;
                    s->sort_column = m->cols[hc].key;
                    out.sort_changed = true;
                }
                if (ev[e].type == CUI_EVENT_KEY_DOWN && (ev[e].modifiers & CUI_MOD_CTRL) &&
                    (ev[e].key == CUI_KEY_LEFT || ev[e].key == CUI_KEY_RIGHT)) {
                    size_t dest = ev[e].key == CUI_KEY_LEFT ? (hc ? hc - 1 : hc) : hc + 1;
                    out.columns_changed |= cui_table_move_column(m, m->cols[hc].key, dest);
                }
            } else if (!m->rows[hr].disabled) {
                if (previous != target) {
                    s->cursor_row = m->rows[hr].key;
                    s->cursor_column = m->cols[hc].key;
                }
                previous = target;
                if (activation == target) {
                    select(s, o, s->cursor_row, s->cursor_column);
                    reveal = true;
                    out.activated_row = s->cursor_row;
                    out.activated_column = s->cursor_column;
                }
                if (ev[e].type == CUI_EVENT_KEY_DOWN) {
                    cui_table_row_index(m, s->cursor_row, &r);
                    c = column(m, s->cursor_column);
                    size_t nr = r, nc = c;
                    if (ev[e].key == CUI_KEY_UP || ev[e].key == CUI_KEY_DOWN) {
                        size_t n =
                            enabled_row(m, r, ev[e].key == CUI_KEY_UP ? -1 : 1, o.loop_selection);
                        if (n != SIZE_MAX)
                            nr = n;
                    }
                    if (ev[e].key == CUI_KEY_PAGEUP || ev[e].key == CUI_KEY_PAGEDOWN) {
                        size_t page = (size_t)fmaxf(
                            1, fminf((float)m->count, floorf(height(o) / row_height(o))));
                        nr = ev[e].key == CUI_KEY_PAGEUP
                                 ? (r > page ? r - page : 0)
                                 : (page > m->count - 1 - r ? m->count - 1 : r + page);
                        if (m->rows[nr].disabled) {
                            size_t n =
                                enabled_row(m, nr, ev[e].key == CUI_KEY_PAGEUP ? -1 : 1, false);
                            if (n == SIZE_MAX)
                                n = enabled_row(m, nr, ev[e].key == CUI_KEY_PAGEUP ? 1 : -1, false);
                            nr = n == SIZE_MAX ? r : n;
                        }
                    }
                    if (ev[e].key == CUI_KEY_LEFT || ev[e].key == CUI_KEY_RIGHT) {
                        size_t n =
                            enabled_col(m, c, ev[e].key == CUI_KEY_LEFT ? -1 : 1, o.loop_selection);
                        if (n != SIZE_MAX)
                            nc = n;
                    }
                    if (ev[e].key == CUI_KEY_HOME || ev[e].key == CUI_KEY_END) {
                        for (size_t p = 0; p < m->count; p++) {
                            size_t n = ev[e].key == CUI_KEY_HOME ? p : m->count - 1 - p;
                            if (!m->rows[n].disabled) {
                                nr = n;
                                break;
                            }
                        }
                    }
                    if (nr != r || nc != c) {
                        reveal = true;
                        s->cursor_row = m->rows[nr].key;
                        s->cursor_column = m->cols[nc].key;
                        if (!o.manual_selection)
                            select(s, o, s->cursor_row, s->cursor_column);
                        cui_focus(ui, cui_table_cell_id(id, s->cursor_row, s->cursor_column));
                    }
                }
            }
        }
        if (ev[e].type == CUI_EVENT_SCROLL && isfinite(ev[e].dy) &&
            target == cui_table_body_id(id)) {
            s->scroll_y -= ev[e].dy;
            if (isfinite(ev[e].dx))
                s->scroll_x -= ev[e].dx;
        }
    }
    float total = 0;
    for (size_t e = 0; e < count && !o.disabled; e++)
        if (ev[e].type == CUI_EVENT_SCROLL && cui_event_target(ui, e) == id && isfinite(ev[e].dx))
            s->scroll_x -= ev[e].dx;
    for (size_t p = 0; p < m->columns; p++)
        total += m->cols[p].width;
    float width = isfinite(o.width) && o.width > 0 ? o.width : 480;
    s->scroll_x = isfinite(s->scroll_x) ? fmaxf(0, fminf(s->scroll_x, fmaxf(0, total - width))) : 0;
    if (reveal && cui_table_row_index(m, s->cursor_row, &r) &&
        cui_focused(ui) == cui_table_cell_id(id, s->cursor_row, s->cursor_column)) {
        float top = (float)r * row_height(o);
        if (top < s->scroll_y)
            s->scroll_y = top;
        else if (top + row_height(o) > s->scroll_y + height(o))
            s->scroll_y = top + row_height(o) - height(o);
        float left = 0;
        c = column(m, s->cursor_column);
        for (size_t p = 0; p < c && p < m->columns; p++)
            left += m->cols[p].width;
        if (c < m->columns) {
            if (left < s->scroll_x)
                s->scroll_x = left;
            else if (left + m->cols[c].width > s->scroll_x + width)
                s->scroll_x = left + m->cols[c].width - width;
        }
    }
    if (reveal_header) {
        size_t hc = column(m, s->header_cursor);
        float left = 0;
        for (size_t p = 0; p < hc && p < m->columns; p++)
            left += m->cols[p].width;
        if (hc < m->columns) {
            if (left < s->scroll_x)
                s->scroll_x = left;
            else if (left + m->cols[hc].width > s->scroll_x + width)
                s->scroll_x = left + m->cols[hc].width - width;
        }
    }
    s->scroll_y = cui_table_viewport(m, s->scroll_y, height(o), row_height(o), o.overscan).scroll;
    out.rows = cui_table_viewport(m, s->scroll_y, height(o), row_height(o), 0);
    out.columns = cui_table_column_viewport(m, s->scroll_x, width, 0);
    out.visible_rows_changed = !s->viewport_initialized || s->visible_begin != out.rows.begin ||
                               s->visible_end != out.rows.end;
    out.visible_columns_changed = !s->viewport_initialized ||
                                  s->column_begin != out.columns.begin ||
                                  s->column_end != out.columns.end;
    bool near = m->count - out.rows.end <= o.load_more_threshold;
    if (!near || s->previous_row_count != m->count || !o.load_more)
        s->load_pending = false;
    out.load_more = o.load_more && !o.disabled && near && !s->load_pending;
    if (out.load_more)
        s->load_pending = true;
    s->viewport_initialized = true;
    s->visible_begin = out.rows.begin;
    s->visible_end = out.rows.end;
    s->column_begin = out.columns.begin;
    s->column_end = out.columns.end;
    s->previous_row_count = m->count;
    out.selection_changed = oldrow != s->row || oldcol != s->column;
    return out;
}
void cui_table_submit_root(cui_context *ui, cui_id id, cui_table_model *m, cui_table_state *s,
                           cui_table_options o) {
    if (!ui || !m || !s)
        return;
    cui_table_view columns = cui_table_column_viewport(m, s->scroll_x, o.width, 0);
    float width = isfinite(o.width) && o.width > 0 ? o.width : 480;
    cui_interact(ui, id, CUI_ROLE_GRID, cui_string("Data table"), o.disabled, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.row_count = (unsigned)m->count,
                               .column_count = (unsigned)m->columns,
                               .scrollable = true,
                               .scroll_x = s->scroll_x,
                               .scroll_max_x = fmaxf(0, columns.total - width)});
}
void cui_table_submit_header(cui_context *ui, cui_id id, cui_table_model *m, cui_table_state *s,
                             cui_table_options o) {
    if (!ui || !m || !s)
        return;
    cui_table_view columns = cui_table_column_viewport(m, s->scroll_x, o.width, o.column_overscan);
    size_t tab = column(m, s->header_cursor);
    if (tab < columns.begin || tab >= columns.end || m->cols[tab].disabled) {
        tab = SIZE_MAX;
        for (size_t c = columns.begin; c < columns.end; c++)
            if (!m->cols[c].disabled) {
                tab = c;
                break;
            }
    }
    for (size_t c = columns.begin; c < columns.end; c++) {
        cui_id cid = cui_table_header_id(id, m->cols[c].key);
        bool disabled = o.disabled || m->cols[c].disabled;
        cui_interact(ui, cid, CUI_ROLE_COLUMNHEADER, m->cols[c].label, disabled, false,
                     s->sort_column == m->cols[c].key ? (float)s->sort : 0);
        cui_set_tab_stop(ui, cid, c == tab);
        cui_set_semantic_details(
            ui, cid,
            (cui_semantic_details){.parent = id,
                                   .column_index = (unsigned)c + 1,
                                   .selected = o.selection_mode == CUI_TABLE_SELECT_COLUMN &&
                                               s->column == m->cols[c].key});
        if (m->cols[c].resizable) {
            cui_id grip = cui_table_resize_id(id, m->cols[c].key);
            cui_interact(ui, grip, CUI_ROLE_SLIDER, cui_string("Column width"), disabled, false,
                         m->cols[c].width);
            cui_set_tab_stop(ui, grip, false);
            cui_set_semantic_details(
                ui, grip,
                (cui_semantic_details){.parent = cid,
                                       .minimum = fmaxf(16, m->cols[c].minimum),
                                       .maximum = m->cols[c].maximum ? m->cols[c].maximum : 65536});
        }
    }
}
void cui_table_submit_body(cui_context *ui, cui_id id, cui_table_model *m, cui_table_state *s,
                           cui_table_options o, cui_table_label_fn label, void *data) {
    if (!ui || !m || !s)
        return;
    cui_id body = cui_table_body_id(id);
    cui_interact(ui, body, CUI_ROLE_GROUP, cui_string("Rows"), o.disabled, false, 0);
    cui_set_tab_stop(ui, body, false);
    cui_table_view allrows = cui_table_viewport(m, s->scroll_y, height(o), row_height(o), 0);
    cui_table_view allcols = cui_table_column_viewport(m, s->scroll_x, o.width, 0);
    float width = isfinite(o.width) && o.width > 0 ? o.width : 480;
    cui_set_semantic_details(
        ui, body,
        (cui_semantic_details){.parent = id,
                               .scrollable = true,
                               .scroll_y = s->scroll_y,
                               .scroll_x = s->scroll_x,
                               .scroll_max_y = fmaxf(0, allrows.total - height(o)),
                               .scroll_max_x = fmaxf(0, allcols.total - width)});
    cui_table_view v = cui_table_viewport(m, s->scroll_y, height(o), row_height(o), o.overscan);
    cui_table_view columns = cui_table_column_viewport(m, s->scroll_x, o.width, o.column_overscan);
    for (size_t r = v.begin; r < v.end; r++) {
        cui_id rid = cui_table_row_id(id, m->rows[r].key);
        cui_interact(ui, rid, CUI_ROLE_ROW, m->rows[r].label, o.disabled || m->rows[r].disabled,
                     false, (float)r);
        cui_set_tab_stop(ui, rid, false);
        cui_set_semantic_details(
            ui, rid,
            (cui_semantic_details){.parent = body,
                                   .row_index = (unsigned)r + 1,
                                   .selected = o.selection_mode == CUI_TABLE_SELECT_ROW &&
                                               s->row == m->rows[r].key});
        for (size_t c = columns.begin; c < columns.end; c++) {
            cui_id cid = cui_table_cell_id(id, m->rows[r].key, m->cols[c].key);
            bool selected = (o.selection_mode == CUI_TABLE_SELECT_ROW ? s->row == m->rows[r].key
                             : o.selection_mode == CUI_TABLE_SELECT_COLUMN
                                 ? s->column == m->cols[c].key
                                 : s->row == m->rows[r].key && s->column == m->cols[c].key);
            cui_interact(ui, cid, CUI_ROLE_GRIDCELL,
                         label ? label(data, r, m->cols[c].key) : m->rows[r].label,
                         o.disabled || m->rows[r].disabled || m->cols[c].disabled, false, 0);
            cui_set_tab_stop(ui, cid,
                             s->cursor_row == m->rows[r].key && s->cursor_column == m->cols[c].key);
            cui_set_semantic_details(ui, cid,
                                     (cui_semantic_details){.parent = rid,
                                                            .selected = selected,
                                                            .row_index = (unsigned)r + 1,
                                                            .column_index = (unsigned)c + 1});
        }
    }
}
cui_table_result cui_base_data_table(cui_context *ui, cui_id id, cui_table_model *m,
                                     cui_table_state *s, cui_table_options o) {
    cui_table_result result = cui_table_reduce(ui, id, m, s, o);
    if (result.error == CUI_TABLE_OK) {
        cui_table_submit_root(ui, id, m, s, o);
        cui_table_submit_header(ui, id, m, s, o);
        cui_table_submit_body(ui, id, m, s, o, NULL, NULL);
    }
    return result;
}
