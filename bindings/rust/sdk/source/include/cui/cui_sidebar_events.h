/* SPDX-License-Identifier: MIT */
#ifndef CUI_SIDEBAR_EVENTS_H
#define CUI_SIDEBAR_EVENTS_H
#include "cui_component_events.h"
#include "cui_sidebar.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Applied row invocation, after legacy expansion/activation callbacks. */
typedef struct {
    cui_component_event_metadata event;
    cui_id item;
    unsigned level;
} cui_sidebar_activation;
typedef void (*cui_sidebar_activation_fn)(void *, const cui_sidebar_activation *);
typedef struct {
    cui_sidebar_activation_fn on_activate;
    void *user;
} cui_sidebar_item_handler;
typedef struct {
    cui_sidebar_item_result control;
    size_t activation_count; /* Actual observer calls; missing observer means zero. */
    bool valid, count_overflow;
} cui_sidebar_item_event_result;
typedef struct {
    bool valid, submitted, count_overflow;
    size_t activation_count;
} cui_sidebar_menu_event_result;
/* No retained handler; no expansion/collapse/header/footer events in this slice.
 * Typed observers must not mutate active item/input storage, recurse or destroy ui. */
cui_sidebar_item_event_result cui_base_sidebar_item_events(cui_context *,
                                                           const cui_sidebar_menu_item *,
                                                           bool collapsed, unsigned level,
                                                           cui_sidebar_item_handler);
cui_sidebar_menu_event_result cui_sidebar_menu_events(cui_context *, cui_id, const cui_theme *,
                                                      const cui_sidebar_menu_item *, size_t,
                                                      bool collapsed, cui_sidebar_item_handler);
#ifdef __cplusplus
}
#endif
#endif
