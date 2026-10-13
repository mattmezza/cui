/* SPDX-License-Identifier: MIT */
#ifndef CUI_EDITOR_H
#define CUI_EDITOR_H
#include "cui_base.h"
#include "cui_textarea.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Return the exclusive UTF-8 byte end of the next token, within line_end.
 * Invalid boundaries fall back to ordinary text. Never mutate the edit model. */
typedef size_t (*cui_editor_highlight_fn)(void *user, cui_str language, cui_str document,
                                          size_t start, size_t line_end, cui_color *color);
typedef struct {
    cui_str label, detail, insert_text;
} cui_editor_completion_item;
typedef struct {
    const cui_editor_completion_item *items; /* Borrowed until close. */
    size_t count, selected, replace_start, replace_end;
    size_t cursor, anchor; /* Selection captured when the proposal was opened. */
    uint64_t revision;
    bool open;
} cui_editor_completion_state;
typedef struct {
    cui_edit *edit; /* Borrowed, must outlive this state. */
    cui_textarea_state view;
    cui_editor_completion_state completion;
} cui_editor_state;
typedef struct {
    cui_textarea_options textarea;
    bool line_numbers;
    cui_str language; /* Borrowed for the draw call. */
    cui_editor_highlight_fn highlight;
    void *highlight_user;
    float completion_width; /* Maximum popup width; zero defaults to 320. */
    size_t completion_rows; /* Zero defaults to eight; maximum 32 visible rows. */
} cui_editor_options;
bool cui_editor_state_init(cui_editor_state *, cui_edit *, cui_textarea_line *, size_t capacity);
/* Host supplies validated UTF-8 items and one original document byte range.
 * No copying/filtering/provider calls. Empty count closes the menu. Invalid
 * input preserves both existing menu and document. Item bytes remain immutable
 * and alive until the menu closes. Later document revisions dismiss it. */
cui_edit_result cui_editor_show_completions(cui_editor_state *, const cui_editor_completion_item *,
                                            size_t count, size_t replace_start, size_t replace_end);
void cui_editor_cancel_completion(cui_editor_state *);
cui_id cui_editor_completion_item_id(cui_id editor_id, size_t index);
/* One atomic replace/history step. Failure preserves text/selection/history
 * and leaves the current menu open, except that stale revisions dismiss it. */
cui_edit_result cui_editor_accept_completion(cui_editor_state *);
typedef cui_textarea_fold cui_editor_fold;
/* Attach caller-owned ranges; edits detach them, requiring host refresh. */
bool cui_editor_set_folds(cui_editor_state *, cui_editor_fold *, size_t count);
/* Collapsing moves hidden selection endpoints to the opening line end.
 * Other selections, text, and edit history stay unchanged. */
cui_edit_result cui_editor_set_folded(cui_editor_state *, size_t index, bool collapsed);
cui_editor_options cui_editor_default(void);
/* Literal, case-sensitive UTF-8 search; selects a match, optionally wrapping.
 * Missing/invalid queries preserve selection. Reverse search starts before the
 * selection; forward search starts after it. No query pointer is retained. */
cui_edit_result cui_editor_find(cui_editor_state *, cui_str query, bool reverse, bool wrap);
/* Replace the exact selected literal match. No match returns BOUNDARY unchanged.
 * Successful replacement is one undo step and collapses the selection to its end.
 * Programmatic APIs do not enforce view readonly/disabled options. */
cui_edit_result cui_editor_replace_current(cui_editor_state *, cui_str query, cui_str replacement);
/* Replace all nonoverlapping case-sensitive literal matches in one undo step.
 * Scratch is caller-owned byte storage, disjoint from model storage/query/
 * replacement/count; its contents are unspecified on failure. Count is written
 * only on success. No matches succeeds with zero and preserves state. Success
 * with matches resets selection and scroll to document start. */
cui_edit_result cui_editor_replace_all(cui_editor_state *, cui_str query, cui_str replacement,
                                       char *scratch, size_t capacity, size_t *count);
typedef struct {
    cui_edit *query; /* Borrowed single-line model. */
    cui_input_behavior_state input;
    float scroll_x;
    bool open;
} cui_editor_search_state;
typedef struct {
    bool query_changed, navigated, closed;
    size_t matches;
    cui_edit_result error;
} cui_editor_search_result;
bool cui_editor_search_init(cui_editor_search_state *, cui_edit *query);
/* Counts nonoverlapping literal matches; invalid/empty queries return zero. */
size_t cui_editor_match_count(const cui_editor_state *, cui_str query);
/* Draw before the editor to intercept Ctrl/Meta-F on editor_id. Enter and
 * Shift-Enter in the query navigate; Escape closes and returns editor focus.
 * Call every frame even when closed. Width zero defaults to 500 pixels. */
cui_editor_search_result cui_editor_find_bar(cui_context *, cui_id, cui_id editor_id,
                                             const cui_theme *, cui_editor_state *,
                                             cui_editor_search_state *, float width, bool disabled);
cui_textarea_result cui_base_editor(cui_context *, cui_id, cui_str name, cui_editor_state *,
                                    cui_editor_options, cui_textarea_metrics);
cui_textarea_result cui_editor(cui_context *, cui_id, const cui_theme *, cui_str name,
                               cui_editor_state *, cui_editor_options);
#ifdef __cplusplus
}
#endif
#endif
