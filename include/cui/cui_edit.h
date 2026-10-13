/* SPDX-License-Identifier: MIT */
#ifndef CUI_EDIT_H
#define CUI_EDIT_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_edit cui_edit;
typedef enum {
    CUI_EDIT_OK,
    CUI_EDIT_INVALID_UTF8,
    CUI_EDIT_CAPACITY,
    CUI_EDIT_BOUNDARY,
    CUI_EDIT_NO_HISTORY,
    CUI_EDIT_INVALID_ARGUMENT,
    CUI_EDIT_LINE_BREAK,
    CUI_EDIT_CLIPBOARD_UNAVAILABLE
} cui_edit_result;
typedef struct {
    char *buffer;
    size_t capacity;
    bool multiline;
    size_t history_bytes, max_history_steps;
} cui_edit_config;
typedef struct {
    size_t length, cursor, anchor;
    bool can_undo, can_redo;
    uint64_t revision;
    bool multiline;
} cui_edit_state;
typedef enum {
    CUI_EDIT_PREVIOUS,
    CUI_EDIT_NEXT,
    CUI_EDIT_LINE_START,
    CUI_EDIT_LINE_END,
    CUI_EDIT_START,
    CUI_EDIT_END,
    CUI_EDIT_WORD_PREVIOUS,
    CUI_EDIT_WORD_NEXT,
    CUI_EDIT_LINE_PREVIOUS,
    CUI_EDIT_LINE_NEXT,
    CUI_EDIT_GRAPHEME_PREVIOUS,
    CUI_EDIT_GRAPHEME_NEXT
} cui_edit_motion;
/* The application owns the NUL-terminated buffer. Only this model may mutate
   it while attached. Capacity includes NUL; offsets are UTF-8 byte offsets. */
cui_edit *cui_edit_create(cui_edit_config config);
void cui_edit_destroy(cui_edit *edit);
cui_edit_state cui_edit_get_state(const cui_edit *edit);
cui_str cui_edit_get_text(const cui_edit *edit);
bool cui_edit_valid_utf8(cui_str text);
#define CUI_GRAPHEME_UNICODE_VERSION "17.0.0"
/* Default extended grapheme clusters (UAX #29). Length-bounded UTF-8 may contain
 * NUL. Invalid text, non-scalar offsets or offsets beyond length return SIZE_MAX
 * (boundary returns false). Next/previous are strict, clamped at either end.
 * Allocation-free full scans; offsets remain UTF-8 bytes. */
size_t cui_grapheme_next(cui_str text, size_t offset);
size_t cui_grapheme_previous(cui_str text, size_t offset);
bool cui_grapheme_boundary(cui_str text, size_t offset);
/* Forward iterator validates once, then visits clusters in linear total time.
 * Keep the borrowed text unchanged. For already validated text, an aggregate
 * initializer {text, known_grapheme_boundary} may seed traversal without a scan.
 * Other offsets must only be changed by next.
 * Next returns the new byte offset, clamped at length; invalid UTF-8 or an
 * invalid scalar offset encountered during traversal returns SIZE_MAX. */
typedef struct {
    cui_str text;
    size_t offset;
} cui_grapheme_iterator;
bool cui_grapheme_iterator_init(cui_grapheme_iterator *iterator, cui_str text);
size_t cui_grapheme_iterator_next(cui_grapheme_iterator *iterator);
cui_edit_result cui_edit_select(cui_edit *edit, size_t anchor, size_t cursor);
/* Word motions skip punctuation/space and move between word starts. Letters,
 * digits, underscores and common Unicode letter/combining ranges form words.
 * Vertical motions preserve a scalar column across logical LF-delimited lines;
 * visual wrapping and pixel-column movement belong to the styled textarea. */
cui_edit_result cui_edit_move(cui_edit *edit, cui_edit_motion motion, bool extend);
cui_edit_result cui_edit_insert(cui_edit *edit, cui_str text);
/* Replace a UTF-8 byte range as one history step without first changing the
 * selection. Undo restores the original selection; success collapses it after
 * the inserted text. Rejection leaves text, selection and history unchanged. */
cui_edit_result cui_edit_replace_range(cui_edit *, size_t start, size_t end, cui_str text);
/* Indent selected lines (inclusive of a selection ending at line start), or
 * insert at the collapsed cursor. Outdent removes only an exact line prefix.
 * One successful change is one undo step with both selection endpoints mapped. */
cui_edit_result cui_edit_indent(cui_edit *, bool outdent, size_t tab_size, bool hard_tabs);
cui_edit_result cui_edit_backspace(cui_edit *edit);
cui_edit_result cui_edit_delete(cui_edit *edit);
/* Grapheme erasure expands partial cluster selections outward. At an interior
 * scalar offset it removes the containing cluster. Scalar erasure APIs above
 * retain their original behavior. Undo restores the original byte selection. */
cui_edit_result cui_edit_backspace_grapheme(cui_edit *edit);
cui_edit_result cui_edit_delete_grapheme(cui_edit *edit);
cui_edit_result cui_edit_undo(cui_edit *edit);
cui_edit_result cui_edit_redo(cui_edit *edit);
/* Apply one event in caller-defined order. Ordinary Left/Right and erasure use
 * graphemes; word/vertical motions retain their existing contracts. Pointer
 * events are ignored. */
cui_edit_result cui_edit_apply_event(cui_edit *, cui_event);
/* Applies events already routed to id; returns the first rejected edit while
   continuing independent later events. Disabled input is ignored. */
cui_edit_result cui_edit_apply_events(cui_edit *edit, cui_context *ui, cui_id id, bool disabled);
#ifdef __cplusplus
}
#endif
#endif
