/* SPDX-License-Identifier: MIT */
#ifndef CUI_NUMERIC_SELECTION_EVENTS_H
#define CUI_NUMERIC_SELECTION_EVENTS_H
#include "cui_component_events.h"
#include "cui_numeric.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { bool control; size_t change_count; } cui_numeric_selection_event_result;
typedef struct {
    cui_component_event_metadata event;
    size_t before, after, total;
} cui_pagination_change_event;
typedef void (*cui_pagination_change_fn)(void *user, const cui_pagination_change_event *event);
typedef struct { cui_pagination_change_fn change; void *user; } cui_pagination_event_handler;
/* Aggregate final CHANGE; control preserves the legacy bool, not render success.
 * Callback/user are borrowed only for this call; absent callback ignores user.
 * Passive/layout-only/disabled calls preserve reduction but never notify.
 * Events may be copied. Do not mutate the owner, recurse, destroy any borrowed
 * storage/context or unwind a foreign exception during the callback. Schedule
 * owner changes and teardown after the outer call returns. */
cui_numeric_selection_event_result cui_base_pagination_events(cui_context *, cui_id,
    size_t *, size_t total, bool compact, bool disabled, cui_pagination_event_handler);
cui_numeric_selection_event_result cui_pagination_events(cui_context *, cui_id,
    const cui_theme *, size_t *, size_t total, bool compact, bool disabled,
    cui_pagination_event_handler);
typedef struct {
    cui_component_event_metadata event;
    size_t before, after, count;
} cui_stepper_change_event;
typedef void (*cui_stepper_change_fn)(void *user, const cui_stepper_change_event *event);
typedef struct { cui_stepper_change_fn change; void *user; } cui_stepper_event_handler;
/* Same aggregate observation/lifetime rules as Pagination; zero-based indices.
 * Step descriptors and their strings remain borrowed throughout the outer call. */
cui_numeric_selection_event_result cui_base_stepper_events(cui_context *, cui_id,
    const cui_step *, size_t count, size_t *current, bool disabled, cui_stepper_event_handler);
cui_numeric_selection_event_result cui_stepper_events(cui_context *, cui_id,
    const cui_theme *, const cui_step *, size_t count, size_t *current,
    bool vertical, bool disabled, cui_stepper_event_handler);
#ifdef __cplusplus
}
#endif
#endif
