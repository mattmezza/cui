/* SPDX-License-Identifier: MIT */
const sizes = ['CUI_DESIGN_INHERIT', 'CUI_DESIGN_XS', 'CUI_DESIGN_SM',
 'CUI_DESIGN_BASE', 'CUI_DESIGN_MD', 'CUI_DESIGN_LG', 'CUI_DESIGN_XL',
 'CUI_DESIGN_2XL', 'CUI_DESIGN_3XL', 'CUI_DESIGN_4XL', 'CUI_DESIGN_5XL',
 'CUI_DESIGN_6XL', 'CUI_DESIGN_7XL', 'CUI_DESIGN_8XL', 'CUI_DESIGN_9XL'];

export function paginationUsage(size, disabled = false, mode = 0, visiblePages = 5) {
 const layout = mode === 2 ? '.icon_only = true' :
                mode === 1 ? `.visible_pages = ${visiblePages}, .ellipsis_viewport = viewport` : '.compact = true';
 return `#include <cui/cui_numeric.h>

/* The application retains page and supplies the layout's current content width. */
bool draw_results_pages(cui_context *ui, cui_id id, const cui_theme *theme,
                        size_t *page, size_t total_pages, float content_width${mode === 1 ? ', cui_rect viewport' : ''}) {
    return cui_pagination_ex(ui, id, theme, page, total_pages,
        (cui_pagination_options){${layout}, .disabled = ${disabled},
                                 .design_size = ${sizes[size] || 'CUI_DESIGN_BASE'},
                                 .available_width = content_width});
}`;
}

export function timeFieldUsage(size, ordered = false) {
 if(ordered)return `#include <cui/cui_components.h>
#include <cui/cui_time_field_v1.h>

typedef struct {
    cui_time_field_options_v1 options;
    cui_time_field_state_v1 start_state, end_state;
    cui_time start, end;
    bool saved;
} schedule_example;

static bool schedule_init(schedule_example *schedule) {
    schedule->start = (cui_time){9, 30, 0};
    schedule->end = (cui_time){10, 45, 0};
    schedule->saved = false;
    if (cui_time_field_options_init_v1(&schedule->options) != CUI_TIME_TEXT_OK)
        return false;
    schedule->options.flags |= CUI_TIME_FIELD_IME_V1;
    return cui_time_field_state_init_v1(&schedule->start_state, schedule->start,
                                        &schedule->options) == CUI_TIME_TEXT_OK &&
           cui_time_field_state_init_v1(&schedule->end_state, schedule->end,
                                        &schedule->options) == CUI_TIME_TEXT_OK;
}

/* Queue host input first. Draw the returned list with the host renderer. */
static cui_draw_list schedule_frame(cui_context *ui, const cui_theme *theme,
        float width, float height, float dt, schedule_example *schedule) {
    cui_time_field_frame_entry_v1 fields[] = {
        {1, &schedule->start_state, &schedule->start, &schedule->options},
        {2, &schedule->end_state, &schedule->end, &schedule->options},
    };
    if (cui_begin_time_field_frame_v1(ui, width, height, dt, fields, 2) != CUI_TIME_TEXT_OK)
        return (cui_draw_list){0};
    cui_begin_box(ui, 4, (cui_box){.padding = 16, .gap = 8});
    cui_time_field_result_v1 start =
        cui_time_field_from_frame_v1(ui, 1, theme, cui_string("Start time"),
        ${sizes[size] || "CUI_DESIGN_BASE"}, (cui_time_handler){0});
    cui_time_field_result_v1 end =
        cui_time_field_from_frame_v1(ui, 2, theme, cui_string("End time"),
        ${sizes[size] || "CUI_DESIGN_BASE"}, (cui_time_handler){0});
    if (start.changed || end.changed)
        schedule->saved = false;
    if (cui_button(ui, 3, theme, cui_string("Save schedule"), false))
        schedule->saved = true;
    if (schedule->saved)
        cui_label(ui, theme, cui_string("Schedule saved."));
    cui_end_box(ui);
    return cui_end_frame(ui);
}`;
 return `#include <cui/cui_time_field_v1.h>

/* Call init once per field. Keep the options, state and time between frames. */
typedef struct {
    cui_time_field_options_v1 options;
    cui_time_field_state_v1 state;
    cui_time value;
} time_field_example;

static bool time_field_init(time_field_example *field) {
    field->value = (cui_time){9, 30, 0};
    if (cui_time_field_options_init_v1(&field->options) != CUI_TIME_TEXT_OK)
        return false;
    field->options.flags = CUI_TIME_FIELD_IME_V1;
    return cui_time_field_state_init_v1(&field->state, field->value,
                                      &field->options) == CUI_TIME_TEXT_OK;
}

static cui_time_field_result_v1 time_field_draw(cui_context *ui, cui_id id,
        const cui_theme *theme, time_field_example *field) {
    return cui_time_field_with_size_v1(ui, id, theme, cui_string("Time"),
        &field->state, &field->value, &field->options,
        ${sizes[size] || "CUI_DESIGN_BASE"}, (cui_time_handler){0});
}`;
}

export const liveDataTableUsage = `#include <cui/cui_table.h>

/* On the UI thread, before drawing. Keep the new rows and columns alive
 * while the model borrows them. Capture before editing the current rows. */
cui_table_error update_live_data_table(cui_table_model *model, cui_table_state *state,
    cui_table_options options, const cui_table_row *rows, size_t count,
    cui_table_column *columns, size_t column_count) {
    cui_table_row_anchor anchor = cui_table_capture_row_anchor(model, state->scroll_y, options);
    cui_table_error result = cui_table_bind(model, rows, count, columns, column_count);
    if (result == CUI_TABLE_OK)
        state->scroll_y = cui_table_restore_row_anchor(model, anchor, options);
    return result;
}

/* Inside a frame, with the same stable table ID and options across updates. */
cui_table_result draw_live_data_table(cui_context *ui, cui_id id, const cui_theme *theme,
    cui_table_model *model, cui_table_state *state, cui_table_options options) {
    return cui_data_table(ui, id, theme, model, state, options, NULL, NULL, NULL);
}`;

export const pagedDataTableUsage = `#include <cui/cui_table.h>

/* Keep model, state and pending across frames. A true return requests one page.
 * Start the request and return promptly. Preserve the loaded rows while waiting. */
bool draw_paged_data_table(cui_context *ui, cui_id id, const cui_theme *theme,
    cui_table_model *model, cui_table_state *state, cui_table_options options,
    bool more_rows, bool *pending) {
    options.load_more = more_rows && !options.disabled;
    options.load_more_threshold = 2;
    cui_table_result result = cui_data_table(ui, id, theme, model, state, options,
        NULL, NULL, NULL);
    if (!result.load_more || *pending)
        return false;
    *pending = true;
    return true;
}

/* Apply a current request on the UI thread before drawing. Pass the complete
 * retained row snapshot in display order, including the newly arrived page.
 * Keep prior rows alive until bind succeeds. Use effective scrollbar options. */
cui_table_error apply_data_table_page(cui_table_model *model, cui_table_state *state,
    cui_table_options options, const cui_table_row *rows, size_t count,
    cui_table_column *columns, size_t column_count, bool *pending) {
    cui_table_row_anchor anchor = cui_table_capture_row_anchor(model, state->scroll_y, options);
    cui_table_error result = cui_table_bind(model, rows, count, columns, column_count);
    if (result == CUI_TABLE_OK)
        state->scroll_y = cui_table_restore_row_anchor(model, anchor, options);
    *pending = false;
    return result;
}`;

export const virtualInboxUsage = `#include <cui/cui_virtual_scroll.h>

/* Retain list and state. Supply the updated key order before drawing.
 * NULL heights keeps previous measurements and gives new rows the default. */
cui_virtual_result update_virtual_inbox(cui_virtual_list *list, cui_virtual_widget_state *state,
    const uint64_t *next_keys, size_t next_count) {
    cui_virtual_anchor anchor = cui_virtual_capture_anchor(list, state->scroll.y);
    cui_virtual_result result = cui_virtual_rebuild(list, next_keys, NULL, next_count);
    if (result == CUI_VIRTUAL_OK) {
        size_t index;
        if (anchor.valid && !cui_virtual_index(list, anchor.key, &index))
            anchor.within = 0; /* Put the replacement row at the top. */
        float viewport = state->last_viewport.height > 0 ? state->last_viewport.height : 172;
        state->scroll.y = (float)cui_virtual_restore_anchor(list, anchor, viewport);
    }
    return result;
}

/* Inside a frame. The renderer runs only for visible rows and overscan. */
cui_virtual_widget_result draw_virtual_inbox(cui_context *ui, cui_id id, const cui_theme *theme,
    const cui_virtual_list *list, cui_virtual_widget_state *state,
    cui_virtual_render_item_fn paint_row, void *userdata) {
    return cui_virtual_list_draw(ui, id, theme, list, state,
        (cui_virtual_widget_options){.scroll = {.width = 420, .height = 172}, .overscan = 1},
        paint_row, userdata);
}`;

// These are complete C translation units. The application owns their state
// and calls draw only between begin_frame and end_frame.
export function selectedUsage(name, mode, size = 3, disabled = false) {
 const design = sizes[size] || 'CUI_DESIGN_BASE';
 if (name === 'Input' && mode === 9) return `#include <cui/cui_components.h>
#include <cui/cui_regex.h>

typedef struct {
    cui_regex compiled;
    cui_regex_session session;
    bool ready;
} unicode_name_view;

/* Zero-initialize view. Prepare outside UI frames; link the optional regex archive.
 * Keep this owner at a stable address until the last Input call finishes. */
bool unicode_name_init(unicode_name_view *view) {
    if (view->ready) return true;
    if (cui_regex_compile(cui_string("^[\\\\p{L} ]*$"),
            (cui_regex_options){.max_pattern_bytes = 64, .max_text_bytes = 1024},
            &view->compiled) == CUI_REGEX_OK &&
        cui_regex_session_create(&view->compiled, 1024 * 1024, &view->session) == CUI_REGEX_OK) {
        view->ready = true;
        return true;
    }
    (void)cui_regex_session_destroy(&view->session);
    (void)cui_regex_destroy(&view->compiled);
    return false;
}

/* Retain edit/state across frames; invoke between begin_frame/end_frame.
 * Caller must initialize successfully before drawing. No JavaScript validation. */
cui_input_result draw_unicode_name(cui_context *ui, cui_id id,
    const cui_theme *theme, cui_edit *edit, cui_input_state *state,
    unicode_name_view *view) {
    return cui_input_ex(ui, id, theme, cui_string("Unicode name"), edit, state,
        (cui_input_options){
            .matcher = cui_regex_matcher(&view->session),
            .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
            .design_size = ${design}, .disabled = ${disabled}, .width = 280
        });
}

/* At shutdown outside any matcher lease; reset Input state before replacing edit. */
void unicode_name_destroy(unicode_name_view *view) {
    (void)cui_regex_session_destroy(&view->session);
    (void)cui_regex_destroy(&view->compiled);
    *view = (unicode_name_view){0};
}`;
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
