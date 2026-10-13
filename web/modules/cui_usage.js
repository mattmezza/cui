/* SPDX-License-Identifier: MIT */
const sizes = ['CUI_DESIGN_INHERIT', 'CUI_DESIGN_XS', 'CUI_DESIGN_SM',
 'CUI_DESIGN_BASE', 'CUI_DESIGN_MD', 'CUI_DESIGN_LG', 'CUI_DESIGN_XL',
 'CUI_DESIGN_2XL', 'CUI_DESIGN_3XL', 'CUI_DESIGN_4XL', 'CUI_DESIGN_5XL',
 'CUI_DESIGN_6XL', 'CUI_DESIGN_7XL', 'CUI_DESIGN_8XL', 'CUI_DESIGN_9XL'];

// These are complete C translation units. The application owns their state
// and calls draw only between begin_frame and end_frame.
export function selectedUsage(name, mode, size = 3, disabled = false) {
 const design = sizes[size] || 'CUI_DESIGN_BASE';
 if (name === 'Input' && mode >= 4 && mode <= 6) return `#include <cui/cui_components.h>

/* Retain edit and state across frames. Reset state before replacing edit.
 * Call inside a successful cui_begin_frame / cui_end_frame pair.
 * The platform adapter supplies text/IME events and clipboard callbacks. */
cui_input_result draw_password(cui_context *ui, cui_id id,
    const cui_theme *theme, cui_edit *edit, cui_input_state *state) {
    return cui_input_ex(ui, id, theme, cui_string("Password"), edit, state,
        (cui_input_options){
            .masked = ${mode === 5 ? 'false' : 'true'},
            .content_type = ${mode === 6 ? 'CUI_TEXT_INPUT_PURPOSE_NEW_PASSWORD' : 'CUI_TEXT_INPUT_PURPOSE_PASSWORD'},
            .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
            .design_size = ${design},
            .disabled = ${disabled}, .width = 280
        });
}
/* Reveal intentionally exposes plaintext; keep PASSWORD purpose set.
 * See the Password Input guide for model construction and secure host setup. */`;
 if (name === 'Input' && (mode === 7 || mode === 8)) return `#include <cui/cui_components.h>

/* Retain edit and state across frames. Formatting commits the stored value.
 * Workspace is borrowed for this call and must not overlap model/IME memory. */
cui_input_result draw_formatted_input(cui_context *ui, cui_id id,
    const cui_theme *theme, cui_edit *edit, cui_input_state *state,
    char *workspace, size_t capacity) {
    return cui_input_ex(ui, id, theme, cui_string("${mode === 7 ? 'Phone number' : 'Amount'}"), edit, state,
        (cui_input_options){
            .format = ${mode === 7 ? '{.kind = CUI_EDIT_FORMAT_PATTERN, .pattern = {"(999) 999-9999", 14}}' : "{.kind = CUI_EDIT_FORMAT_NUMBER, .has_separator = true, .separator = ',', .fraction_limited = true, .fraction_digits = 2}"},
            .format_workspace = {workspace, capacity},
            .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
            .design_size = ${design}, .disabled = ${disabled}, .width = 280
        });
}
/* ${mode === 7 ? '4155550123 becomes (415) 555-0123.' : '12345.678 becomes 12,345.67.'}
 * Construct a single-line edit model; size workspace for final UTF-8 plus NUL.
 * Check result.error for configuration/capacity failures; Undo restores one edit. */`;
 if (name !== 'Button' || mode < 1 || mode > 3) return null;
 return `#include <cui/cui_components.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool selected[3];
    cui_button_group_style_workspace *workspace;
} calendar_view;

bool calendar_view_init(calendar_view *view) {
    *view = (calendar_view){0};
    view->selected[0] = true;
    view->workspace = calloc(1, sizeof *view->workspace);
    return view->workspace != NULL;
}

static void select_view(void *user, const cui_button_group_request *request) {
    calendar_view *view = user;
    memset(view->selected, 0, sizeof view->selected);
    for (size_t i = 0; i < request->selected_count; ++i)
        view->selected[request->selected_indices[i]] = true;
}

/* Retain view across frames; call between begin_frame and end_frame.
 * Stable scoped IDs let selection and focus survive redraws. */
bool draw_calendar_view(cui_context *ui, const cui_theme *theme,
                       calendar_view *view) {
    if (!view->workspace) return false;
    cui_button_group_styled_item items[] = {
        {.id = cui_id_from(ui, "calendar-day"), .label = {"Day", 3}},
        {.id = cui_id_from(ui, "calendar-week"), .label = {"Week", 4}},
        {.id = cui_id_from(ui, "calendar-month"), .label = {"Month", 5}}
    };
    for (size_t i = 0; i < 3; ++i) {
        items[i].options.custom_cursor = true;
        items[i].options.cursor = CUI_CURSOR_POINTER;
    }
    return cui_button_group_ex(ui, cui_id_from(ui, "calendar-view"), theme,
        items, view->selected, 3, view->workspace,
        (cui_button_group_style_options){
            .behavior = {
                .multiple = ${mode === 2}, .vertical = ${mode === 3},
                .disabled = ${disabled},
                .activation_policy = CUI_BUTTON_GROUP_ALL_ACTIVATIONS,
                .accessibility_label = {"Calendar view", 13},
                .on_change = select_view, .change_user = view
            },
            .design_size = ${design},
            .override_variant = true, .variant = CUI_BUTTON_OUTLINE
        }, NULL);
}

/* After the last draw list is rendered, at application shutdown: */
void calendar_view_destroy(calendar_view *view) {
    free(view->workspace);
    *view = (calendar_view){0};
}`;
}
