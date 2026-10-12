/* SPDX-License-Identifier: MIT */
#ifndef CUI_CHOICE_H
#define CUI_CHOICE_H
#include "cui.h"
#include "cui_base.h"
#include "cui_edit.h"
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
typedef void (*cui_choice_activation_fn)(cui_id item, void *user);
/* Styled row body; semantic name/disabled/selection remain supplied by item.
 * index is relative to the displayed array, including filtered Combobox arrays. */
typedef void (*cui_choice_render_fn)(cui_context *, const cui_choice_item *, size_t index,
                                     void *user);
typedef struct {
    bool disabled, menu;
    float dt;
    size_t visible_begin, visible_count;  /* count=0 submits all items */
    cui_choice_activation_fn on_activate; /* synchronous, in event order */
    void *activation_user;
    cui_choice_render_fn render;
    void *render_user;
} cui_list_options;
typedef struct {
    cui_overlay overlay;
    cui_list_state list;
} cui_choice_popup;
typedef struct {
    bool disabled;
    cui_rect viewport;
    float width, dt;
    cui_popup_side side;         /* preferred anchor side; defaults below */
    cui_choice_render_fn render; /* optional fixed-height option body */
    void *render_user;
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
bool cui_base_selection_remove(cui_context *, cui_id chip, cui_str name, bool *selected,
                               bool disabled);
bool cui_base_list_multi(cui_context *, const cui_choice_item *, size_t count, bool *selected,
                         cui_list_state *, cui_list_options);
bool cui_list_multi(cui_context *, cui_id container, const cui_theme *, const cui_choice_item *,
                    size_t count, bool *selected, cui_list_state *, cui_list_options);
/* Multi-select keeps the popup open while toggling options; Escape/outside
 * dismiss. The selected summary and chips remain caller-controlled. */
bool cui_select_multi(cui_context *, cui_id trigger, const cui_theme *, cui_str name,
                      cui_str placeholder, const cui_choice_item *, size_t count, bool *selected,
                      cui_choice_popup *, cui_choice_popup_options);
typedef struct {
    bool changed, query_changed;
    size_t matching_count;
    cui_edit_result error;
} cui_combobox_result;
/* Caller supplies filtered scratch capacity for at least count items. Query is
 * an independently owned single-line editor; committed IDs survive filtering.
 * Search is ASCII case-insensitive substring matching, byte-exact otherwise. */
cui_combobox_result cui_base_combobox(cui_context *, cui_id input, cui_str name, cui_edit *query,
                                      const cui_choice_item *, size_t count,
                                      cui_choice_item *filtered, size_t filtered_capacity,
                                      cui_id *selected, cui_choice_popup *, bool disabled);
cui_combobox_result cui_combobox(cui_context *, cui_id input, const cui_theme *, cui_str name,
                                 cui_edit *query, const cui_choice_item *, size_t count,
                                 cui_choice_item *filtered, size_t filtered_capacity,
                                 cui_id *selected, cui_choice_popup *, cui_choice_popup_options);
cui_combobox_result cui_base_combobox_multi(cui_context *, cui_id input, cui_str name, cui_edit *,
                                            const cui_choice_item *, size_t count,
                                            cui_choice_item *filtered, bool *filtered_selected,
                                            size_t filtered_capacity, bool *selected,
                                            cui_choice_popup *, bool disabled);
cui_combobox_result cui_combobox_multi(cui_context *, cui_id input, const cui_theme *, cui_str name,
                                       cui_edit *, const cui_choice_item *, size_t count,
                                       cui_choice_item *filtered, bool *filtered_selected,
                                       size_t filtered_capacity, bool *selected, cui_choice_popup *,
                                       cui_choice_popup_options);

#define CUI_MENU_DEPTH_MAX 16
typedef enum {
    CUI_MENU_ACTION,
    CUI_MENU_CHECK,
    CUI_MENU_RADIO,
    CUI_MENU_SEPARATOR,
    CUI_MENU_LABEL,
    CUI_MENU_SUBMENU
} cui_menu_kind;
typedef void (*cui_menu_render_fn)(cui_context *, void *user);
typedef struct cui_menu_entry {
    cui_id id;
    cui_str label, icon, shortcut;
    bool disabled;
    cui_menu_kind kind;
    bool *checked;
    int *radio_value;
    int radio_option;
    const struct cui_menu_entry *children;
    size_t child_count;
    cui_menu_render_fn render; /* optional custom row body; keep within control height */
    void *render_user;
} cui_menu_entry;
typedef struct {
    cui_overlay overlay;
    cui_list_state levels[CUI_MENU_DEPTH_MAX];
    cui_id path[CUI_MENU_DEPTH_MAX - 1];
    cui_rect panels[CUI_MENU_DEPTH_MAX];
    size_t depth;
    bool depth_limit;
    float row_height, padding; /* previous painted metrics for pointer hover */
    bool contextual;
    cui_rect anchor;
} cui_menu_tree_state;
typedef struct {
    cui_choice_popup_options popup;
    bool keep_open_checks, checks_on_right;
} cui_menu_tree_options;
void cui_menu_tree_init(cui_menu_tree_state *, cui_id popup);
/* Ordered reducer before painting. Descriptors and pointed-to check/radio values
 * are borrowed; IDs must be globally unique throughout the tree. */
bool cui_base_menu_tree(cui_context *, cui_id trigger, cui_str name, const cui_menu_entry *,
                        size_t count, cui_id *action, cui_menu_tree_state *, bool disabled,
                        float dt, bool keep_open_checks);
bool cui_menu_entry_is_disabled(const cui_menu_entry *);
void cui_base_menu_entry(cui_context *, const cui_menu_entry *, bool disabled, bool expanded,
                         cui_id parent);
bool cui_menu_tree(cui_context *, cui_id trigger, const cui_theme *, cui_str name,
                   const cui_menu_entry *, size_t count, cui_id *action, cui_menu_tree_state *,
                   cui_menu_tree_options);
/* Context-menu target is an existing visible box; this does not submit or paint
 * that target again. Secondary press within its previous bounds opens at the
 * pointer. Call once per state after rendering the receiving area. */
void cui_context_menu_open(cui_context *, cui_menu_tree_state *, cui_id restore_focus,
                           cui_rect anchor);
bool cui_base_context_menu(cui_context *, cui_id target, const cui_menu_entry *, size_t count,
                           cui_id *action, cui_menu_tree_state *, bool disabled, float dt,
                           bool keep_open_checks);
bool cui_context_menu(cui_context *, cui_id target, const cui_theme *, cui_str name,
                      const cui_menu_entry *, size_t count, cui_id *action, cui_menu_tree_state *,
                      cui_menu_tree_options);
#ifdef __cplusplus
}
#endif
#endif
