/* SPDX-License-Identifier: MIT */
const sizes = ['CUI_DESIGN_INHERIT', 'CUI_DESIGN_XS', 'CUI_DESIGN_SM',
 'CUI_DESIGN_BASE', 'CUI_DESIGN_MD', 'CUI_DESIGN_LG', 'CUI_DESIGN_XL',
 'CUI_DESIGN_2XL', 'CUI_DESIGN_3XL', 'CUI_DESIGN_4XL', 'CUI_DESIGN_5XL',
 'CUI_DESIGN_6XL', 'CUI_DESIGN_7XL', 'CUI_DESIGN_8XL', 'CUI_DESIGN_9XL'];

export function disclosureHeadingUsage(accordion,interactive=false){
 const separator=accordion?(`cui_disclosure_separator_style divider = {
    .thickness = 2, .inset_left = 12, .inset_right = 12,
    .color = theme.border,
};
`):'';
 const functionSuffix=interactive?'_with_title_states':'';
 const titleArgument=interactive?' &title_states,':'';
 const styles=interactive?`cui_typography_token hover_title = title;
hover_title.style.weight = CUI_TEXT_WEIGHT_SEMIBOLD;
cui_typography_token pressed_title = hover_title;
pressed_title.style.italic = true;
cui_disclosure_title_interactions title_states = {
    .trigger = {
        .hovered = {.typography = &hover_title},
        .focus_visible = {.typography = &open_title, .custom_color = true,
                          .color = theme.accent},
        .pressed = {.typography = &pressed_title, .custom_color = true,
                    .color = theme.accent},
    },
};
`:'';
 const call=accordion?(`cui_accordion_present${functionSuffix}(ui, id, &theme, items, 3, expanded,
    (cui_accordion_options){.collapsible = true, .bordered = true},
    presentation,${titleArgument} draw_panel, user, (cui_disclosure_handler){0});`):(`cui_collapsible_present${functionSuffix}(ui, id, &theme, label, &open,
    (cui_collapsible_options){0}, presentation,${titleArgument} draw_panel, user,
    (cui_disclosure_handler){0});`);
 return `cui_typography_token title = {.size = 16, .line_height = 22, .wrap = true};
cui_typography_token open_title = title;
open_title.style.weight = CUI_TEXT_WEIGHT_BOLD;
${separator}cui_disclosure_appearance appearance = {
    .trigger = {
        .normal = {.typography = &title},
        .expanded = {.typography = &open_title, .custom_color = true,
                     .color = theme.accent},
    },${accordion?'\n    .separator = &divider,':''}
};
cui_disclosure_presentation presentation = {
    .width = -1, ${accordion?'.joined = true, ':''}.appearance = &appearance,
};
${styles}${call}`;
}

export function paginationUsage(size, disabled = false, mode = 0, visiblePages = 5, french = false, customSize = 0) {
 const custom = customSize ? `,\n                                 .custom_size = ${Number.isInteger(customSize) ? customSize.toFixed(1) : customSize}f` : '';
 const numbered = mode === 1 || mode === 3;
 const arrows = mode === 2 || mode === 3;
 const layout = mode === 2 ? '.icon_only = true' :
                numbered ? `.visible_pages = ${visiblePages}, .ellipsis_viewport = viewport` : '.compact = true';
 const navigation = mode === 3 ? ',\n                                 .navigation = CUI_PAGINATION_NAV_CHEVRONS' : '';
 const tooltip = arrows ? ',\n                                 .tooltip_viewport = viewport' : '';
 const labels = french ? `static const cui_pagination_labels labels = {
    .group = CUI_STR("Pages de résultats"),
    .first = CUI_STR("Première"), .previous = CUI_STR("Précédente"),
    .next = CUI_STR("Suivante"), .last = CUI_STR("Dernière"),
    .earlier_pages = CUI_STR("Pages précédentes"), .later_pages = CUI_STR("Pages suivantes"),
};

` : '';
 return `#include <cui/cui_numeric.h>

${labels}/* The application retains page and supplies the layout's current content width. */
bool draw_results_pages(cui_context *ui, cui_id id, const cui_theme *theme,
                        size_t *page, size_t total_pages, float content_width${numbered || arrows ? ', cui_rect viewport' : ''}) {
    return cui_pagination_ex(ui, id, theme, page, total_pages,
        (cui_pagination_options){${layout}, .disabled = ${disabled},
                                 .design_size = ${sizes[size] || 'CUI_DESIGN_BASE'},
                                 .available_width = content_width${custom}${navigation}${tooltip}${french ? ',\n                                 .labels = &labels' : ''}});
}`;
}

export function ratingUsage(size, disabled = false, hearts = false, half = false, preview = true) {
 const path = "M12 21 Q2 15 2 8 Q2 3 7 3 Q10 3 12 6 Q14 3 17 3 Q22 3 22 8 Q22 15 12 21 Z";
 const svg = "<svg width='24' height='24' xmlns='http://www.w3.org/2000/svg'>";
 const shapes = hearts ? `/* Start with two zeroed icon states. Call once before drawing. */
bool prepare_heart_rating(cui_icon_state *outline, cui_icon_state *fill) {
    return cui_icon_set_data(outline, cui_string(
        "${svg}"
        "<path d='${path}' fill='none' stroke='white' stroke-width='1.6'/></svg>")) &&
           cui_icon_set_data(fill, cui_string(
        "${svg}"
        "<path d='${path}' fill='white'/></svg>"));
}
/* Destroy both states with cui_icon_state_destroy after their last draw is rendered. */

` : '';
 return `#include <cui/cui_numeric.h>

${shapes}/* The application retains the rating and calls this inside each frame. */
bool draw_rating(cui_context *ui, cui_id id, const cui_theme *theme, float *rating,
                 float content_width${hearts?', cui_icon_state *outline, cui_icon_state *fill':''}) {
    return cui_rating_present(ui, id, theme, cui_string("Rate your experience"), rating,
        (cui_rating_options){.maximum = 5, .half_steps = ${half}, .disabled = ${disabled},
                             .width = content_width},
        (cui_rating_presentation){.design_size = ${sizes[size] || 'CUI_DESIGN_BASE'},
            .selection = CUI_RATING_STEP_DOWN_FILLED, .hover_preview = ${preview}${hearts?`,
            .outline_icon = outline, .fill_icon = fill,
            .custom_fill = true, .fill = {190, 49, 85, 255}`:''}});
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
export function carouselBasisUsage(visible, vertical = false, looping = false, size = 3) {
    const basis = visible === 2 ? '.5f' : visible === 3 ? '1.f / 3' : null;
    const fit = visible === 4;
    const frame = fit
        ? `cui_carousel_fit *fit[] = {&rail->fit[0], &rail->fit[1], &rail->fit[2], &rail->fit[3]};
    bool drawn = cui_carousel_with_layout(ui, id, theme, &rail->state, slides, 4, options,
        (cui_carousel_layout){.fit = fit});`
        : basis
        ? `const float basis[] = {${Array(4).fill(basis).join(', ')}};
    bool drawn = cui_carousel_with_basis(ui, id, theme, &rail->state, slides, 4, options, basis);`
        : 'bool drawn = cui_carousel(ui, id, theme, &rail->state, slides, 4, options);';
    const axis = vertical ? '\n    cui_carousel_set_axis(&rail->state, CUI_ORIENTATION_VERTICAL);' : '';
    const loop = looping ? '\n    cui_carousel_set_looping(&rail->state, true);' : '';
    return `#include <cui/cui_carousel.h>
#include <math.h>

typedef struct {
    cui_carousel_state state;
    cui_carousel_geometry geometry[4];${fit ? '\n    cui_carousel_fit fit[4];' : ''}
} workflow_rail;

/* Keep rail at a stable address across frames. Call once. */
bool workflow_init(workflow_rail *rail) {
    if (!cui_carousel_state_init(&rail->state, rail->geometry, 4, 4))
        return false;${axis}${loop}
    ${fit ? 'for (size_t i = 0; i < 4; ++i) rail->fit[i] = (cui_carousel_fit){0};\n    ' : ''}return true;
}

static const char *const stages[] = {"Build", "Preview", "Test", "Ship"};${fit ? `
static const char *const details[] = {NULL, "Review changes", NULL, "Publish the release"};` : ''}
static void workflow_slide(cui_context *ui, const cui_theme *theme, size_t index, void *user) {
    (void)user;
    ${fit ? `cui_begin_box(ui, 0, (cui_box){.gap = 6, .align_x = CUI_ALIGN_CENTER});
    ` : ''}cui_text(ui, cui_string(stages[index]), theme->font_size, theme->text);${fit ? `
    if (details[index])
        cui_text(ui, cui_string(details[index]), theme->font_size * .875f, theme->text);
    cui_end_box(ui);` : ''}
}

static cui_carousel_control_options workflow_control(float width, float fallback,
    const cui_theme *theme) {
    cui_design_metrics metrics;
    const float size = cui_design_measure(${sizes[size] || 'CUI_DESIGN_BASE'}, theme->font_size / 16, &metrics)
        ? metrics.control_height : fallback;
    return (cui_carousel_control_options){
        .size = CUI_CAROUSEL_CONTROL_CUSTOM, .custom_size = fminf(size, width)};
}

static void workflow_arrows(cui_context *ui, const cui_theme *theme,
    cui_carousel_state *state, cui_id root, void *user) {
    (void)user;
    cui_carousel_control_options options = workflow_control(state->frame_width, 32, theme);
    cui_begin_box(ui, 0, (cui_box){.width = -1,
        .row = state->frame_width >= 2 * options.custom_size + 8,
        .gap = 8, .align_x = CUI_ALIGN_CENTER});
    cui_carousel_control_inline(ui, cui_id_from(ui, "prev"), theme, state, root,
        false, options);
    cui_carousel_control_inline(ui, cui_id_from(ui, "next"), theme, state, root,
        true, options);
    cui_end_box(ui);
}

static void workflow_pages(cui_context *ui, const cui_theme *theme,
    cui_carousel_state *state, cui_id root, void *user) {
    (void)user;
    cui_carousel_control_options options = workflow_control(state->frame_width, 20, theme);
    const size_t columns = (size_t)fmaxf(1, floorf((state->frame_width + 4) / (options.custom_size + 4)));
    cui_box layout = {.width = -1, .gap = 4, .align_x = CUI_ALIGN_CENTER};
    if (!cui_carousel_pagination_begin(ui, cui_id_from(ui, "pages"), cui_string("Stages"), &layout))
        return;
    for (size_t first = 0; first < state->count; first += columns) {
        const size_t end = first + columns < state->count ? first + columns : state->count;
        cui_begin_box(ui, 0, (cui_box){.width = -1, .row = true, .gap = 4, .align_x = CUI_ALIGN_CENTER});
        for (size_t i = first; i < end; ++i) {
            options.label = cui_string(stages[i]);
            cui_carousel_pagination_item(ui, cui_id_from(ui, stages[i]), theme, state, root, i, options);
        }
        cui_end_box(ui);
    }
    cui_carousel_pagination_end(ui);
}

/* Submit between begin_frame and end_frame. Give each instance a stable key. */
bool workflow_draw(cui_context *ui, const char *key, const cui_theme *theme,
    workflow_rail *rail, double now_seconds) {
    if (!cui_push_id(ui, key))
        return false;
    cui_id id = cui_id_from(ui, "carousel");
    cui_carousel_item slides[4];
    const char *const keys[] = {"build-slide", "preview-slide", "test-slide", "ship-slide"};
    for (size_t i = 0; i < 4; ++i)
        slides[i] = (cui_carousel_item){.id = cui_id_from(ui, keys[i]), .label = cui_string(stages[i]),
                                       .render = workflow_slide${fit ? `,
                                       .custom_box = true, .box = {
                                           .padding = 12, .radius = theme->radius,
                                           .align_x = CUI_ALIGN_CENTER, .align_y = CUI_ALIGN_CENTER,
                                           .background = theme->secondary}` : ''}};
    cui_carousel_options options = cui_carousel_default();
    options.now_seconds = now_seconds;
    options.controls = workflow_arrows;
    options.pagination = workflow_pages;
    ${frame}
    cui_pop_id(ui);
    return drawn;
}`;
}

export const buttonFocusModes = [
 'CUI_BUTTON_FOCUS_LEGACY', 'CUI_BUTTON_FOCUS_VARIANT', 'CUI_BUTTON_FOCUS_OUTER_RING',
];
export function selectedUsage(name, mode, size = 3, disabled = false, focusMode = 0) {
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
 const focus = buttonFocusModes[focusMode] ?? buttonFocusModes[0];
 return `#include <cui/cui_button_styles.h>
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
    return cui_button_group_focus_ex(ui, cui_id_from(ui, "calendar-view"), theme,
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
        }, ${focus}, NULL);
}

/* After the last draw list is rendered, at application shutdown: */
void calendar_view_destroy(calendar_view *view) {
    free(view->workspace);
    *view = (calendar_view){0};
}`;
}
export const radioStyledUsage = `#include "cui/cui_radio_group.h"

static void delivery_panel(cui_context *ui, void *user, cui_id id, cui_box *paint) {
    (void)ui;
    (void)id;
    const cui_theme *theme = user;
    paint->padding = 10;
    paint->radius = theme->radius;
    paint->background = theme->surface;
    paint->border = theme->border;
    paint->border_width = 1;
}

static void delivery_option(cui_context *ui, void *user, cui_id id, bool checked,
                            bool disabled, cui_radio_paint *paint) {
    const cui_theme *theme = user;
    paint->root.padding = 8;
    paint->root.radius = theme->radius;
    if (checked) {
        paint->root.background = theme->secondary;
        paint->label.style.weight = CUI_TEXT_WEIGHT_SEMIBOLD;
    } else if (!disabled && cui_hovered(ui, id)) {
        paint->root.background = theme->list_hover;
    }
}

void delivery_options(cui_context *ui, cui_theme *theme, cui_id *selected,
                      cui_radio_group_state *state) {
    const cui_radio_group_item items[] = {
        {cui_id_from(ui, "standard"), cui_string("Standard"), false},
        {cui_id_from(ui, "priority"), cui_string("Priority"), false}
    };
    cui_radio_group_present_styled(ui, cui_id_from(ui, "delivery"), theme,
        items, 2, selected, state,
        (cui_radio_group_options){.name = cui_string("Delivery options"), .vertical = true},
        (cui_radio_group_presentation){0}, NULL, 0,
        (cui_radio_group_styles){
            .group = delivery_panel, .group_user = theme,
            .items = {.instance = delivery_option, .user = theme}
        }, (cui_selection_handler){0});
}`;


export const radioControlledUsage = `#include "cui/cui_components.h"

typedef struct {
    bool checked, accept_requests;
    unsigned requests;
} choice_owner;

static void request_choice(void *user, const cui_component_boolean_request *request) {
    choice_owner *owner = user;
    owner->requests++;
    /* A radio requests false -> true; checked activation makes no request. */
    if (owner->accept_requests) owner->checked = request->requested;
}

void controlled_choice(cui_context *ui, const cui_theme *theme, choice_owner *owner) {
    if (cui_button(ui, cui_id_from(ui, "clear-choice"), theme,
                   cui_string("Clear selection"), false)) owner->checked = false;
    const bool submitted = owner->checked;
    cui_radio_events(ui, cui_id_from(ui, "choice"), theme,
        cui_string("Standalone choice"), submitted,
        (cui_radio_options){0}, (cui_radio_presentation){0}, (cui_radio_styles){0},
        (cui_component_boolean_handler){.callback = request_choice, .user = owner});
    /* Owner changes appear on the next submission; schedule that frame. */
    if (submitted != owner->checked) cui_invalidate(ui);
}`;

export function iconTextDefaultsUsage(name, rotate, customColor) {
 const rotation = rotate
     ? '\n    icon.custom_transform = true;\n    icon.transform = cui_icon_rotation(.785398163f);'
     : '';
 const tint = customColor ? '\n    icon.custom_color = true;\n    icon.color = theme->primary;' : '';
 return `#include "cui/cui_components.h"
#include "cui/cui_icon.h"

void icon_label(cui_context *ui, const cui_theme *theme) {
    cui_begin_box(ui, cui_id_from(ui, "icon-text-row"),
                  (cui_box){.row = true, .align_y = CUI_ALIGN_CENTER, .gap = 8});
    cui_set_text_defaults(ui, (cui_text_defaults){
        .font_size = 28, .custom_color = true, .color = theme->info});
    cui_icon_options icon = {0};${rotation}${tint}
    cui_icon_named(ui, cui_id_from(ui, "status-icon"), theme, cui_string("${name}"), icon);
    cui_label(ui, theme, cui_string("${name}"));
    cui_end_box(ui);
}`;
}
