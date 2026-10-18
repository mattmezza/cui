/* SPDX-License-Identifier: MIT */
#ifndef CUI_SETTINGS_EVENTS_H
#define CUI_SETTINGS_EVENTS_H
#include "cui_component_events.h"
#include "cui_settings.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    cui_component_event_metadata event;
    size_t previous_page, previous_group, page, group;
} cui_settings_navigation_change;
typedef struct {
    cui_component_event_metadata event;
    cui_id field_id;
    size_t page, group, item;
    bool previous, value;
} cui_settings_bool_change;
typedef struct {
    cui_component_event_metadata event;
    cui_id field_id;
    size_t page, group, item;
    double previous, value;
    bool previous_finite, value_finite;
} cui_settings_number_change;
typedef struct {
    cui_component_event_metadata event;
    cui_id field_id;
    size_t page, group, item;
    cui_id previous, value;
} cui_settings_choice_change;
typedef struct {
    void (*on_navigation)(void *, const cui_settings_navigation_change *);
    void *user;
    void (*on_boolean)(void *, const cui_settings_bool_change *);
    void (*on_number)(void *, const cui_settings_number_change *);
    void (*on_choice)(void *, const cui_settings_choice_change *);
} cui_settings_handler;
typedef struct {
    cui_settings_result result; /* Exact legacy aggregate, including filter repairs. */
    size_t navigation_count;    /* Actual callbacks; saturates at SIZE_MAX. */
    bool count_overflow;
    size_t boolean_count, number_count, choice_count; /* Actual calls, independently saturated. */
} cui_settings_event_result;
/* Notify each actual navigation pair transition after original writes/invalidation,
 * before content. Filter/default repair and same-pair pending-scroll work are silent.
 * Missing observer permits all legacy work; no passive/layout-only observations.
 * Pointer-free payload may be copied; callback/user/payload pointer borrowed.
 * Do not mutate/rebind/free active pages/values/editors/states/labels, input or
 * context, or recursively submit UI. Unrelated flags allowed; defer removal and
 * teardown until outer return plus renderer consumption. Legacy callbacks retain
 * their existing timing; scalar owners must remain readable through the new observer.
 * Built-in bool/number/choice observers aggregate one adapter after legacy callback,
 * using UNKNOWN cause/frame only. Number payload finite flags describe validity;
 * two NaNs and signed zeros compare equal. No text/reset observer. */
cui_settings_event_result cui_settings_events(cui_context *, cui_id, const cui_theme *,
                                              const cui_setting_page *, size_t,
                                              cui_settings_state *, cui_settings_options,
                                              cui_settings_handler);
#ifdef __cplusplus
}
#endif
#endif
