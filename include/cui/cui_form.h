/* SPDX-License-Identifier: MIT */
#ifndef CUI_FORM_H
#define CUI_FORM_H
#include "cui.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { CUI_FORM_MEDIUM, CUI_FORM_XSMALL, CUI_FORM_SMALL, CUI_FORM_LARGE } cui_form_size;
typedef enum {
    CUI_FORM_OK,
    CUI_FORM_INVALID,
    CUI_FORM_CAPACITY,
    CUI_FORM_DUPLICATE_ID
} cui_form_error;
typedef void (*cui_form_content_fn)(cui_context *, const cui_theme *, void *user, float width);
typedef struct {
    cui_id id, control;
    cui_str label, description, error;
    bool hidden, required, invalid, disabled, readonly, no_label_indent;
    size_t column_span;           /* Zero defaults to one. */
    int column_start, column_end; /* CSS grid lines: one-based; negatives count from end. */
    cui_align align;
    cui_form_content_fn render_control, render_label, render_description;
    void *user;
} cui_form_field;
typedef struct {
    float width, label_width, label_text_size, gap_x, gap_y;
    size_t columns; /* Zero defaults to one; maximum 64. */
    bool horizontal_labels;
    cui_form_size size;
    cui_str name;
    cui_form_content_fn render_footer;
    void *footer_user;
} cui_form_options;
typedef struct {
    size_t row, column, span;
    float width;
    bool visible;
} cui_form_cell;
cui_form_options cui_form_options_default(void);
cui_id cui_form_label_id(cui_id field);
cui_id cui_form_description_id(cui_id field);
cui_form_error cui_form_plan(const cui_form_field *, size_t count, cui_form_options,
                             cui_form_cell *, size_t capacity, size_t *rows);
/* Passive semantics and association only; caller owns values and validation. */
cui_form_error cui_base_form(cui_context *, cui_id, const cui_form_field *, size_t,
                             cui_form_options);
void cui_form_associate_control(cui_context *, const cui_form_field *);
cui_form_error cui_form(cui_context *, cui_id, const cui_theme *, const cui_form_field *, size_t,
                        cui_form_options);
#ifdef __cplusplus
}
#endif
#endif
