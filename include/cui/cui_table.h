/* SPDX-License-Identifier: MIT */
#ifndef CUI_TABLE_H
#define CUI_TABLE_H
#include "cui.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    cui_id key;
    cui_str label;
    bool disabled;
} cui_table_row;
typedef enum {
    CUI_TABLE_ALIGN_LEFT,
    CUI_TABLE_ALIGN_CENTER,
    CUI_TABLE_ALIGN_RIGHT
} cui_table_align;
typedef struct {
    cui_id key;
    cui_str label;
    float width, minimum, maximum;
    bool sortable, resizable, movable, disabled;
    cui_table_align align;
} cui_table_column;
typedef enum {
    CUI_TABLE_SORT_DEFAULT,
    CUI_TABLE_SORT_ASCENDING,
    CUI_TABLE_SORT_DESCENDING
} cui_table_sort;
typedef enum {
    CUI_TABLE_SELECT_ROW,
    CUI_TABLE_SELECT_COLUMN,
    CUI_TABLE_SELECT_CELL
} cui_table_selection_mode;
typedef enum {
    CUI_TABLE_OK,
    CUI_TABLE_INVALID,
    CUI_TABLE_CAPACITY,
    CUI_TABLE_DUPLICATE_KEY
} cui_table_error;
typedef struct cui_table_model cui_table_model;
cui_table_model *cui_table_create(size_t row_capacity);
void cui_table_destroy(cui_table_model *);
/* Borrowed rows/columns; columns are caller controlled and moved/resized in place.
 * Binding is transactional. Keys must be nonzero, unique within their dimension. */
cui_table_error cui_table_bind(cui_table_model *, const cui_table_row *, size_t rows,
                               cui_table_column *, size_t columns);
size_t cui_table_rows(const cui_table_model *);
size_t cui_table_columns(const cui_table_model *);
bool cui_table_get_row(const cui_table_model *, size_t, cui_table_row *);
bool cui_table_get_column(const cui_table_model *, size_t, cui_table_column *);
bool cui_table_row_index(const cui_table_model *, cui_id, size_t *);
bool cui_table_resize(cui_table_model *, cui_id column, float width);
bool cui_table_move_column(cui_table_model *, cui_id column, size_t destination);
typedef struct {
    size_t begin, end;
    float before, after, total, scroll;
} cui_table_view;
cui_table_view cui_table_viewport(const cui_table_model *, float scroll, float height,
                                  float row_height, size_t overscan);
cui_table_view cui_table_column_viewport(const cui_table_model *, float scroll, float width,
                                         size_t overscan);
typedef struct {
    cui_id row, column; /* Controlled selection; zero means none. */
    cui_id cursor_row, cursor_column;
    cui_id sort_column;
    cui_table_sort sort;
    float scroll_x, scroll_y;
    cui_id resizing;
    float resize_x, resize_width;
    size_t visible_begin, visible_end, column_begin, column_end, previous_row_count;
    bool viewport_initialized, load_pending;
} cui_table_state;
typedef struct {
    float width, height, row_height, header_height;
    size_t overscan, column_overscan, load_more_threshold;
    bool disabled, striped, bordered, manual_selection, loop_selection, load_more;
    cui_table_selection_mode selection_mode;
} cui_table_options;
typedef struct {
    bool selection_changed, sort_changed, columns_changed;
    bool visible_rows_changed, visible_columns_changed, load_more;
    cui_table_view rows, columns;
    cui_id activated_row, activated_column;
    cui_table_error error;
} cui_table_result;
cui_id cui_table_row_id(cui_id table, cui_id row);
cui_id cui_table_cell_id(cui_id table, cui_id row, cui_id column);
cui_id cui_table_header_id(cui_id table, cui_id column);
cui_id cui_table_resize_id(cui_id table, cui_id column);
cui_id cui_table_body_id(cui_id table);
typedef cui_str (*cui_table_label_fn)(void *, size_t row, cui_id column);
typedef void (*cui_table_cell_fn)(cui_context *, const cui_theme *, void *, size_t row,
                                  cui_id column);
cui_table_result cui_table_reduce(cui_context *, cui_id, cui_table_model *, cui_table_state *,
                                  cui_table_options);
void cui_table_submit_root(cui_context *, cui_id, cui_table_model *, cui_table_state *,
                           cui_table_options);
void cui_table_submit_header(cui_context *, cui_id, cui_table_model *, cui_table_state *,
                             cui_table_options);
void cui_table_submit_body(cui_context *, cui_id, cui_table_model *, cui_table_state *,
                           cui_table_options, cui_table_label_fn, void *);
cui_table_result cui_base_data_table(cui_context *, cui_id, cui_table_model *, cui_table_state *,
                                     cui_table_options);
/* Callback runs only for the virtual window, inside a clipped fixed-size cell.
 * Provide a stable borrowed cell label for accessibility via label callback. */
cui_table_result cui_data_table(cui_context *, cui_id, const cui_theme *, cui_table_model *,
                                cui_table_state *, cui_table_options, cui_table_label_fn,
                                cui_table_cell_fn, void *userdata);
/* Stateless composable Table: caller determines section/cell hierarchy and content. */
void cui_table_begin(cui_context *, cui_id, const cui_theme *, float width);
void cui_table_begin_row(cui_context *, cui_id, const cui_theme *, float height, bool header);
void cui_table_begin_cell(cui_context *, cui_id, float width, float height, cui_table_align);
void cui_table_end(cui_context *);
#ifdef __cplusplus
}
#endif
#endif
