/* SPDX-License-Identifier: MIT */
#ifndef CUI_QUESTIONNAIRE_H
#define CUI_QUESTIONNAIRE_H
#include "cui_edit.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_questionnaire_state cui_questionnaire_state;
typedef enum {
    CUI_QUESTIONNAIRE_OK,
    CUI_QUESTIONNAIRE_INVALID,
    CUI_QUESTIONNAIRE_CAPACITY,
    CUI_QUESTIONNAIRE_DUPLICATE_ITEM,
    CUI_QUESTIONNAIRE_DUPLICATE_CHOICE,
    CUI_QUESTIONNAIRE_MULTIPLE_DEFAULTS,
    CUI_QUESTIONNAIRE_UNKNOWN_ITEM,
    CUI_QUESTIONNAIRE_UNKNOWN_CHOICE,
    CUI_QUESTIONNAIRE_ANSWER_MISMATCH
} cui_questionnaire_result;
typedef enum {
    CUI_QUESTIONNAIRE_UNANSWERED,
    CUI_QUESTIONNAIRE_ANSWERED,
    CUI_QUESTIONNAIRE_SKIPPED
} cui_questionnaire_status;
typedef enum {
    CUI_QUESTIONNAIRE_VALID,
    CUI_QUESTIONNAIRE_REQUIRED,
    CUI_QUESTIONNAIRE_UNFILLED,
    CUI_QUESTIONNAIRE_MESSAGE
} cui_questionnaire_error_kind;
typedef struct {
    cui_questionnaire_error_kind kind;
    cui_str message;
} cui_questionnaire_error;
typedef enum {
    CUI_QUESTIONNAIRE_CURRENT_CHANGED,
    CUI_QUESTIONNAIRE_ANSWER_CHANGED,
    CUI_QUESTIONNAIRE_COMPLETED,
    CUI_QUESTIONNAIRE_SUBMIT
} cui_questionnaire_event_kind;
typedef struct {
    cui_questionnaire_event_kind kind;
    size_t item, previous;
} cui_questionnaire_event;
typedef void (*cui_questionnaire_event_fn)(const cui_questionnaire_state *, cui_questionnaire_event,
                                           void *);
/* Return borrowed error text (empty is valid). It must outlive the next validation.
 * Validator input is immutable; do not mutate this state from a validator. */
typedef cui_str (*cui_questionnaire_validate_fn)(const cui_questionnaire_state *, size_t item,
                                                 void *);
typedef struct {
    cui_str value, label, description;
    bool disabled, default_selected;
} cui_questionnaire_choice;
typedef struct {
    cui_str name, label, description;
    bool required, multiple, disabled;
    const cui_questionnaire_choice *choices;
    size_t choice_count;
    cui_edit *input;
    cui_str input_label;
    bool input_disabled;
    cui_questionnaire_validate_fn validate;
    void *validate_user;
} cui_questionnaire_item;
typedef struct {
    bool selected, initial_selected, disabled;
} cui_questionnaire_choice_runtime;
typedef struct {
    size_t choice_offset;
    bool disabled, skipped, validation_attempted;
    cui_questionnaire_error internal_error;
    cui_str external_error;
    bool has_external_error;
    char *answer, *initial_answer, *initial_input;
    size_t answer_length, initial_answer_length, initial_input_length;
} cui_questionnaire_item_runtime;
typedef struct {
    cui_questionnaire_item_runtime *items;
    size_t item_capacity;
    cui_questionnaire_choice_runtime *choices;
    size_t choice_capacity;
    char *text;
    size_t text_capacity, max_answer_bytes;
} cui_questionnaire_workspace;
struct cui_questionnaire_state {
    const cui_questionnaire_item *items;
    size_t count, current, initial_current;
    cui_questionnaire_workspace storage;
    cui_questionnaire_event_fn event;
    void *event_user;
    size_t pending_item;
    double pending_deadline;
    bool initialized, complete, pending_confirm;
};
/* Definitions/editors and workspace outlive state. No allocation. Empty schemas
 * are valid. All storage regions must be disjoint. Index SIZE_MAX means none. */
cui_questionnaire_result cui_questionnaire_init(cui_questionnaire_state *,
                                                const cui_questionnaire_item *, size_t,
                                                cui_questionnaire_workspace,
                                                cui_questionnaire_event_fn, void *);
size_t cui_questionnaire_find(const cui_questionnaire_state *, cui_str name);
size_t cui_questionnaire_total(const cui_questionnaire_state *);
size_t cui_questionnaire_position(
    const cui_questionnaire_state *); /* zero based among enabled, or SIZE_MAX */
bool cui_questionnaire_choice_selected(const cui_questionnaire_state *, size_t item, size_t choice);
cui_str cui_questionnaire_freeform(const cui_questionnaire_state *, size_t item);
cui_questionnaire_status cui_questionnaire_item_status(const cui_questionnaire_state *,
                                                       size_t item);
cui_questionnaire_error cui_questionnaire_item_error(const cui_questionnaire_state *, size_t item);
cui_questionnaire_result cui_questionnaire_set_current(cui_questionnaire_state *, size_t item,
                                                       bool initial);
cui_questionnaire_result cui_questionnaire_set_answer(cui_questionnaire_state *, size_t item,
                                                      const size_t *choices, size_t count,
                                                      cui_str freeform);
cui_questionnaire_result cui_questionnaire_set_input(cui_questionnaire_state *, size_t item,
                                                     cui_str);
cui_questionnaire_result cui_questionnaire_sync_input(cui_questionnaire_state *, size_t item,
                                                      bool emit);
cui_questionnaire_result cui_questionnaire_disable_item(cui_questionnaire_state *, size_t item,
                                                        bool);
cui_questionnaire_result cui_questionnaire_disable_choice(cui_questionnaire_state *, size_t item,
                                                          size_t choice, bool);
cui_questionnaire_result cui_questionnaire_external_error(cui_questionnaire_state *, size_t item,
                                                          const cui_str *); /* NULL clears */
cui_questionnaire_result cui_questionnaire_activate(cui_questionnaire_state *, size_t item,
                                                    size_t choice);
cui_questionnaire_result cui_questionnaire_choose(cui_questionnaire_state *, size_t item,
                                                  size_t choice, double now, bool reduced_motion);
bool cui_questionnaire_tick(cui_questionnaire_state *,
                            double now); /* also services zero-delay deferred confirmation */
bool cui_questionnaire_previous(cui_questionnaire_state *);
bool cui_questionnaire_next(cui_questionnaire_state *);
bool cui_questionnaire_skip(cui_questionnaire_state *);
bool cui_questionnaire_submit(cui_questionnaire_state *);
bool cui_questionnaire_confirm(cui_questionnaire_state *);
bool cui_questionnaire_validate_item(cui_questionnaire_state *, size_t item);
bool cui_questionnaire_reset(cui_questionnaire_state *);
char cui_questionnaire_shortcut(const cui_questionnaire_state *, size_t item, size_t choice,
                                bool numbers);
bool cui_questionnaire_activate_shortcut(cui_questionnaire_state *, char key, bool numbers,
                                         double now, bool reduced_motion);
#ifdef __cplusplus
}
#endif
#endif
