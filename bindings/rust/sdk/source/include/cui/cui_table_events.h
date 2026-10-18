/* SPDX-License-Identifier: MIT */
#ifndef CUI_TABLE_EVENTS_H
#define CUI_TABLE_EVENTS_H
#include "cui_table.h"
#include "cui_component_events.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    cui_component_event_metadata event;
    cui_id previous_column, column;
    cui_table_sort previous, value;
} cui_table_sort_change;
typedef void (*cui_table_sort_change_fn)(void *user, const cui_table_sort_change *event);
typedef struct {
    cui_component_event_metadata event;
    cui_table_selection previous, value;
    cui_table_selection_mode mode;
} cui_table_selection_change;
typedef void (*cui_table_selection_change_fn)(void *user, const cui_table_selection_change *event);
typedef struct {
    cui_table_selection_change_fn on_selection;
    cui_table_sort_change_fn on_sort;
    void *user;
} cui_table_event_handler;
typedef struct {
    cui_table_result control;
    bool valid, submitted, count_overflow;
    size_t selection_count, sort_count; /* Actual synchronous calls, not the legacy result flag. */
} cui_table_event_result;
/* Applied single-selection/sort transitions inside reduction, before final
 * semantics/rendering. on_selection + multiple_selection is rejected atomically
 * with valid=false/control.error=INVALID. Sort-only/nil observers preserve multi.
 * Selection payload captures primary IDs/effective operation mode, not multi sets.
 * No sorting of caller data, request rejection, retained registration or replay.
 * Missing observer ignores user; passive/disabled/loading paths preserve behavior.
 * Payload is pointer-free; handler/user borrow synchronously. During callbacks no
 * model/state/row/column/workspace mutation, rebind, recursion, destruction or
 * foreign unwinding. Copy facts/change unrelated state; defer edits/removal and
 * teardown until outer submission and renderer consumption are complete. */
cui_table_event_result cui_table_reduce_events(cui_context *, cui_id,
    cui_table_model *, cui_table_state *, cui_table_options, cui_table_event_handler);
cui_table_event_result cui_base_data_table_events(cui_context *, cui_id,
    cui_table_model *, cui_table_state *, cui_table_options, cui_table_event_handler);
cui_table_event_result cui_data_table_events(cui_context *, cui_id, const cui_theme *,
    cui_table_model *, cui_table_state *, cui_table_options,
    cui_table_label_fn, cui_table_cell_fn, void *, cui_table_event_handler);
/* Separate exact multiset observation. The two exclusive writable arrays each
 * contain capacity entries (<=4096), at least state.selection_capacity. Known
 * overlap/size checks fail before mutation. Scratch must additionally not alias
 * context/internal allocations, strings, callback userdata or other resources;
 * those opaque allocations cannot be validated. NULL on_selection ignores scratch.
 * Spans are canonical (row,column) multisets, borrowed ONLY during callback; copy
 * entries explicitly for retention. No allocation, owner sorting or per-item delta.
 * The callback restrictions above apply to both owner and scratch storage. */
typedef struct {
    const cui_table_selection *items;
    size_t count;
} cui_table_selection_span;
typedef struct {
    cui_table_selection *previous, *value;
    size_t capacity;
} cui_table_selection_workspace;
typedef struct {
    cui_component_event_metadata event;
    cui_table_selection previous_primary, value_primary;
    cui_table_selection_mode mode;
    cui_table_selection_span previous, value;
} cui_table_selection_set_change;
typedef void (*cui_table_selection_set_change_fn)(void *user,
                                                  const cui_table_selection_set_change *change);
typedef struct {
    cui_table_selection_set_change_fn on_selection;
    cui_table_sort_change_fn on_sort;
    void *user;
} cui_table_set_event_handler;
typedef enum {
    CUI_TABLE_SET_PREFLIGHT_OK,
    CUI_TABLE_SET_PREFLIGHT_INVALID,
    CUI_TABLE_SET_PREFLIGHT_WORKSPACE_CAPACITY,
    CUI_TABLE_SET_PREFLIGHT_OVERLAP
} cui_table_set_preflight;
typedef struct {
    cui_table_event_result events;
    cui_table_set_preflight preflight;
} cui_table_set_event_result;
cui_table_set_event_result cui_table_reduce_set_events(cui_context *, cui_id, cui_table_model *,
                                                       cui_table_state *, cui_table_options,
                                                       cui_table_selection_workspace,
                                                       cui_table_set_event_handler);
cui_table_set_event_result cui_base_data_table_set_events(cui_context *, cui_id, cui_table_model *,
                                                          cui_table_state *, cui_table_options,
                                                          cui_table_selection_workspace,
                                                          cui_table_set_event_handler);
cui_table_set_event_result cui_data_table_set_events(cui_context *, cui_id, const cui_theme *,
                                                     cui_table_model *, cui_table_state *,
                                                     cui_table_options, cui_table_label_fn,
                                                     cui_table_cell_fn, void *,
                                                     cui_table_selection_workspace,
                                                     cui_table_set_event_handler);
#ifdef __cplusplus
}
#endif
#endif
