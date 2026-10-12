/* SPDX-License-Identifier: MIT */
#ifndef CUI_CHOICE_H
#define CUI_CHOICE_H
#include "cui.h"
#include "cui_overlay.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    cui_id id;
    cui_str label;
    bool disabled;
} cui_choice_item;
typedef enum {
    CUI_TAB_DEFAULT,
    CUI_TAB_UNDERLINE,
    CUI_TAB_PILL,
    CUI_TAB_OUTLINE,
    CUI_TAB_SEGMENTED
} cui_tab_variant;
typedef struct {
    bool vertical, disabled, manual_activation;
    cui_tab_variant variant;
} cui_tabs_options;
typedef struct {
    bool vertical, disabled, multiple, allow_empty;
} cui_toggle_group_options;
typedef void (*cui_tab_panel_fn)(cui_context *, size_t index, void *user);
/* Selection is a stable item ID rather than a reorder-sensitive array index.
 * Zero means no selection. Invalid/disabled selections are left caller-owned;
 * they produce no visible panel. Base controls draw nothing. */
bool cui_base_tabs(cui_context *, const cui_choice_item *, size_t count, cui_id *selected,
                   cui_tabs_options);
bool cui_base_toggle_group(cui_context *, const cui_choice_item *, size_t count, bool *selected,
                           cui_toggle_group_options);
bool cui_tabs(cui_context *, cui_id container, const cui_theme *, const cui_choice_item *,
              size_t count, cui_id *selected, cui_tabs_options, cui_tab_panel_fn, void *);
bool cui_toggle_group(cui_context *, cui_id container, const cui_theme *, const cui_choice_item *,
                      size_t count, bool *selected, cui_toggle_group_options);
/* Flat selectable lists and anchored popup controls. App-owned state preserves
 * the active cursor, ASCII typeahead prefix and scroll window across frames. */
typedef struct {
    cui_id active;
    char search[64];
    size_t search_length;
    float search_time;
    size_t first_visible;
} cui_list_state;
typedef struct {
    bool disabled, menu;
    float dt;
    size_t visible_begin, visible_count; /* count=0 submits all items */
} cui_list_options;
typedef struct {
    cui_overlay overlay;
    cui_list_state list;
} cui_choice_popup;
typedef struct {
    bool disabled;
    cui_rect viewport;
    float width, dt;
} cui_choice_popup_options;
void cui_choice_popup_init(cui_choice_popup *, cui_id popup_id);
/* Apply dismissal and trigger activation before submitting the open popup. */
void cui_base_choice_popup_trigger(cui_context *, cui_id trigger, cui_str name, bool disabled,
                                   const cui_choice_item *, size_t count, cui_id selected,
                                   cui_choice_popup *);
bool cui_base_list(cui_context *, const cui_choice_item *, size_t count, cui_id *selected,
                   cui_list_state *, cui_list_options);
bool cui_list(cui_context *, cui_id container, const cui_theme *, const cui_choice_item *,
              size_t count, cui_id *selected, cui_list_state *, cui_list_options);
bool cui_select(cui_context *, cui_id trigger, const cui_theme *, cui_str name, cui_str placeholder,
                const cui_choice_item *, size_t count, cui_id *selected, cui_choice_popup *,
                cui_choice_popup_options);
/* Menu returns true when an action ID was activated, including reactivation
 * of the currently selected action. The caller owns the command dispatch. */
bool cui_menu(cui_context *, cui_id trigger, const cui_theme *, cui_str name,
              const cui_choice_item *, size_t count, cui_id *action, cui_choice_popup *,
              cui_choice_popup_options);
#ifdef __cplusplus
}
#endif
#endif
