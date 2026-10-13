/* SPDX-License-Identifier: MIT */
#ifndef CUI_EDITOR_H
#define CUI_EDITOR_H
#include "cui_textarea.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Return the exclusive UTF-8 byte end of the next token, within line_end.
 * Invalid boundaries fall back to ordinary text. Never mutate the edit model. */
typedef size_t (*cui_editor_highlight_fn)(void *user, cui_str language, cui_str document,
                                          size_t start, size_t line_end, cui_color *color);
typedef struct {
    cui_edit *edit; /* Borrowed, must outlive this state. */
    cui_textarea_state view;
} cui_editor_state;
typedef struct {
    cui_textarea_options textarea;
    bool line_numbers;
    cui_str language; /* Borrowed for the draw call. */
    cui_editor_highlight_fn highlight;
    void *highlight_user;
} cui_editor_options;
bool cui_editor_state_init(cui_editor_state *, cui_edit *, cui_textarea_line *, size_t capacity);
cui_editor_options cui_editor_default(void);
/* Literal, case-sensitive UTF-8 search; selects a match, optionally wrapping.
 * Missing/invalid queries preserve selection. Reverse search starts before the
 * selection; forward search starts after it. No query pointer is retained. */
cui_edit_result cui_editor_find(cui_editor_state *, cui_str query, bool reverse, bool wrap);
cui_textarea_result cui_base_editor(cui_context *, cui_id, cui_str name, cui_editor_state *,
                                    cui_editor_options, cui_textarea_metrics);
cui_textarea_result cui_editor(cui_context *, cui_id, const cui_theme *, cui_str name,
                               cui_editor_state *, cui_editor_options);
#ifdef __cplusplus
}
#endif
#endif
