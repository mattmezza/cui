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
    CUI_EDIT_GRAPHEME_NEXT,
    CUI_EDIT_UNICODE_WORD_PREVIOUS,
    CUI_EDIT_UNICODE_WORD_NEXT
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
#define CUI_WORD_UNICODE_VERSION "17.0.0"
/* Default UAX #29 word boundaries, including punctuation/whitespace segments.
 * Validation and strict/clamped offset behavior match the grapheme functions. */
size_t cui_word_next(cui_str text, size_t offset);
size_t cui_word_previous(cui_str text, size_t offset);
bool cui_word_boundary(cui_str text, size_t offset);
/* Borrowed atomic inline content for ordinary Input/Textarea models. IDs may
 * repeat. All span/string storage stays immutable and alive until detached. */
typedef struct {
    cui_str id, text, label;
} cui_edit_inline_token;
typedef struct {
    size_t start, end;
    cui_edit_inline_token token;
} cui_edit_token_span;
typedef struct {
    cui_str text;
    const cui_edit_token_span *tokens;
    size_t token_count;
} cui_edit_content;
/* Spans are sorted, nonoverlapping, nonempty grapheme ranges matching token.text.
 * IDs must contain a non-whitespace scalar; text/label reject controls and LS/PS.
 * All strings use the edit model's NUL-free UTF-8 contract. Maximum 65536 spans. */
bool cui_edit_content_valid(cui_edit_content);
/* Pure helpers leave output untouched on rejection. Boundary snaps only strict
 * token interiors; forward chooses end, otherwise start. Ranges expand outward;
 * collapsed insertion ranges snap right. Inputs must be scalar byte offsets;
 * the two range output pointers must differ. */
cui_edit_result cui_edit_token_boundary(cui_edit_content, size_t offset, bool forward, size_t *out);
cui_edit_result cui_edit_normalize_token_range(cui_edit_content, size_t start, size_t end,
                                               size_t *out_start, size_t *out_end);
cui_edit_content cui_edit_get_content(const cui_edit *);
/* Atomic document replacement, then annotation adoption. Capacity/validation
 * failure preserves text, selection, history and previous annotation attachment.
 * Changing annotated content cannot borrow token strings from mutable edit storage. */
cui_edit_result cui_edit_set_content(cui_edit *, cui_edit_content);
/* Annotation-only attachment preserves text/selection/history. Any subsequent
 * document revision (including undo/redo) detaches spans; the host must refresh.
 * No metadata allocation, incremental remapping or token history is supplied. */
/* Atomic borrowed-token insertion/replacement. Original endpoints must be grapheme
 * boundaries; overlapping atoms expand the replacement. Output storage must be
 * disjoint from attached spans, token strings, and model storage, and remains
 * borrowed until detached. Count and output are untouched on failure. Exact
 * token/range no-ops preserve selection/history but adopt the copied output.
 * Undo restores text only and detaches tokens; metadata-only edits add no history. */
cui_edit_result cui_edit_replace_range_with_token(cui_edit *, size_t start, size_t end,
                                                  cui_edit_inline_token,
                                                  cui_edit_token_span *output, size_t capacity,
                                                  size_t *count);
cui_edit_result cui_edit_replace_with_token(cui_edit *, cui_edit_inline_token,
                                            cui_edit_token_span *output, size_t capacity,
                                            size_t *count);
cui_edit_result cui_edit_set_tokens(cui_edit *, const cui_edit_token_span *, size_t count);
/* Token-safe view selection: nonempty ranges expand outward, collapsed cursors
 * snap left. The legacy exact scalar selection function below remains available. */
cui_edit_result cui_edit_select_atomic(cui_edit *, size_t anchor, size_t cursor);
cui_edit_result cui_edit_select(cui_edit *edit, size_t anchor, size_t cursor);
/* Legacy WORD motions skip punctuation/space and move between word starts. Letters,
 * digits, underscores and common Unicode letter/combining ranges form words.
 * UNICODE_WORD motions use Unicode default word segments: previous targets
 * the last non-whitespace segment start in the prefix, next targets the first
 * non-whitespace segment end in the suffix. Targets are grapheme-safe.
 * Vertical motions preserve a scalar column across logical LF-delimited lines;
 * visual wrapping and pixel-column movement belong to the styled textarea. */
cui_edit_result cui_edit_move(cui_edit *edit, cui_edit_motion motion, bool extend);
cui_edit_result cui_edit_insert(cui_edit *edit, cui_str text);
/* Replace a UTF-8 byte range as one history step without first changing the
 * selection. Attached atomic spans expand overlapping ranges outward; collapsed
 * insertion snaps right. Undo restores the original selection; success collapses
 * it after the inserted text. Rejection leaves text, selection and history unchanged. */
cui_edit_result cui_edit_replace_range(cui_edit *, size_t start, size_t end, cui_str text);
/* Indent selected lines (inclusive of a selection ending at line start), or
 * insert at the collapsed cursor. Outdent removes only an exact line prefix.
 * One successful change is one undo step with both selection endpoints mapped.
 * Attached inline tokens reject indent; detach or use host token-aware editing. */
cui_edit_result cui_edit_indent(cui_edit *, bool outdent, size_t tab_size, bool hard_tabs);
cui_edit_result cui_edit_backspace(cui_edit *edit);
cui_edit_result cui_edit_delete(cui_edit *edit);
/* Grapheme erasure expands partial cluster selections outward. At an interior
 * scalar offset it removes the containing cluster. Scalar erasure APIs above
 * retain their original behavior. Undo restores the original byte selection. */
cui_edit_result cui_edit_backspace_grapheme(cui_edit *edit);
cui_edit_result cui_edit_delete_grapheme(cui_edit *edit);
/* Atomic Unicode word erasure. Nonempty selections use grapheme erasure;
 * collapsed cursors delete to the corresponding UNICODE_WORD motion target. */
cui_edit_result cui_edit_backspace_word(cui_edit *edit);
cui_edit_result cui_edit_delete_word(cui_edit *edit);
cui_edit_result cui_edit_undo(cui_edit *edit);
cui_edit_result cui_edit_redo(cui_edit *edit);
/* Apply one event in caller-defined order. Ordinary Left/Right and erasure use
 * graphemes; Ctrl/Alt arrows and erasure use Unicode word segments. Explicit
 * legacy word/vertical motions retain their contracts. Pointer
 * events are ignored. */
cui_edit_result cui_edit_apply_event(cui_edit *, cui_event);
/* Applies events already routed to id; returns the first rejected edit while
   continuing independent later events. Disabled input is ignored. */
cui_edit_result cui_edit_apply_events(cui_edit *edit, cui_context *ui, cui_id id, bool disabled);
#ifdef __cplusplus
}
#endif
#endif
