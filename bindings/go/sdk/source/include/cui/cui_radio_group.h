/* SPDX-License-Identifier: MIT */
#ifndef CUI_RADIO_GROUP_H
#define CUI_RADIO_GROUP_H
#include "cui_components.h"
#include "cui_disclosure.h"
#include "cui_selection_events.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_RADIO_GROUP_MAX 64
typedef struct {
    cui_id id;
    cui_str label;
    bool disabled;
} cui_radio_group_item;
typedef struct {
    cui_id focused;
    bool had_focus;
} cui_radio_group_state;
typedef void (*cui_radio_group_change_fn)(void *user, cui_id requested);
typedef struct {
    bool vertical, disabled, required, invalid, hide_focus_ring, skip_tab;
    float gap; /* zero defaults to 12 logical pixels; negative/NaN invalid. */
    cui_control_size size;
    cui_design_size design_size;
    cui_str name;
    cui_radio_group_change_fn on_change;
    void *change_user;
} cui_radio_group_options;
typedef struct {
    bool valid, requested, changed;
    cui_id tab_stop;
} cui_radio_group_result;
/* Stable nonzero unique item IDs, <=64 items, selected0 means none. Callback
 * requests never mutate *selected; caller may synchronously accept or reject.
 * Without callback activation writes *selected. No callback for already selected.
 * Invalid/removed selection paints none; caller owns cleanup of its model. */
cui_radio_group_result cui_base_radio_group(cui_context *, cui_id, const cui_radio_group_item *,
                                            size_t count, cui_id *selected, cui_radio_group_state *,
                                            cui_radio_group_options);
cui_radio_group_result cui_radio_group(cui_context *, cui_id, const cui_theme *,
                                       const cui_radio_group_item *, size_t count, cui_id *selected,
                                       cui_radio_group_state *, cui_radio_group_options);
/* Additive post-transition observers. Legacy on_change remains a synchronous
 * owner request and runs first. Missing observer preserves ordinary mutation.
 * No context destruction/freeing active state or recursive submission in callbacks. */
typedef struct {
    cui_selection_event_result events;
    cui_radio_group_result legacy;
} cui_radio_group_event_result;
cui_radio_group_event_result cui_base_radio_group_events(cui_context *, cui_id,
                                                         const cui_radio_group_item *, size_t,
                                                         cui_id *, cui_radio_group_state *,
                                                         cui_radio_group_options,
                                                         cui_selection_handler);
cui_radio_group_event_result cui_radio_group_events(cui_context *, cui_id, const cui_theme *,
                                                    const cui_radio_group_item *, size_t, cui_id *,
                                                    cui_radio_group_state *,
                                                    cui_radio_group_options, cui_selection_handler);
/* Opt-in named indicators, one copied descriptor per item, count<=64. Parts
 * must be nonzero and full-ID distinct across group/items/indicators. Local
 * invalidity rejects before Base; global/folded collision/capacity is a later
 * composition failure and does not roll back owner callbacks or values. Parts
 * are copied before callbacks; items retain their existing borrowed contract.
 * No extra semantics/input targets. Legacy APIs retain anonymous indicators. */
cui_radio_group_result cui_radio_group_with_parts(cui_context *, cui_id, const cui_theme *,
                                                  const cui_radio_group_item *, size_t, cui_id *,
                                                  cui_radio_group_state *, cui_radio_group_options,
                                                  const cui_selection_indicator_parts *,
                                                  size_t part_count);
cui_radio_group_event_result cui_radio_group_events_with_parts(
    cui_context *, cui_id, const cui_theme *, const cui_radio_group_item *, size_t, cui_id *,
    cui_radio_group_state *, cui_radio_group_options, const cui_selection_indicator_parts *,
    size_t part_count, cui_selection_handler);
/* Optional caller-owned decoration state. Zero initialize and retain between
 * frames. Stable item IDs survive reorder; omitted items are cleared on each
 * successful submission. Reusing storage with a different group resets it.
 * Initial/reduced-motion submissions snap. The check and circular fill fade.
 * next_wake_seconds is host frame time, or zero when settled. */
typedef struct {
    cui_id id;
    cui_disclosure_motion items[CUI_RADIO_GROUP_MAX];
    double next_wake_seconds;
} cui_radio_group_motion;
typedef struct {
    cui_radio_group_motion *motion;
    const cui_selection_indicator_parts *parts;
    size_t part_count;
    bool custom_tab_index;
    int tab_index;
} cui_radio_group_presentation;
/* Optional parts follow the copied/validated with_parts contract. With no parts,
 * part_count must be zero. NULL motion preserves immediate decoration. Selection,
 * callbacks, observers and semantics remain synchronous and final this frame.
 * Opt into signed tab order for the group with custom_tab_index. Its single
 * Tab stop keeps this position as focus moves among options. Lower indices
 * come first, equal indices follow submission order, and skip_tab remains
 * independent. Arrow, Home and End order still follows the items array. */
cui_radio_group_event_result
cui_radio_group_present(cui_context *, cui_id, const cui_theme *, const cui_radio_group_item *,
                        size_t, cui_id *, cui_radio_group_state *, cui_radio_group_options,
                        cui_radio_group_presentation, cui_selection_handler);
/* Optional content below an item's ordinary label. A positive max_width caps
 * the label column in logical pixels; zero fits the available layout. */
typedef struct {
    cui_str description;
    cui_button_content_fn content;
    void *content_user;
    float max_width;
    const cui_radio_tooltip *tooltip;
} cui_radio_group_content;
/* Supply one descriptor per item in the same order, or NULL/zero. Descriptors
 * are copied before selection callbacks. Strings and userdata follow ordinary
 * frame lifetimes. A nonempty description is also the radio's semantic
 * description. The optional content callback runs once per composed row after
 * selection is resolved, in an inert subtree. Submit decorative content only.
 * Keep the item label as the accessible name. No added focus or input target.
 * Optional tooltip descriptors and their state must remain valid throughout
 * the call. Give each hint a distinct ID and state. Invalid hint IDs, text
 * or state reject the submission before selection. Explicit descriptions
 * take precedence over tooltip text in the option's semantic description. */
cui_radio_group_event_result cui_radio_group_present_with_content(
    cui_context *, cui_id, const cui_theme *, const cui_radio_group_item *, size_t, cui_id *,
    cui_radio_group_state *, cui_radio_group_options, cui_radio_group_presentation,
    const cui_radio_group_content *, size_t content_count, cui_selection_handler);
#ifdef __cplusplus
}
#endif
#endif
