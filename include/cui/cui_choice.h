/* SPDX-License-Identifier: MIT */
#ifndef CUI_CHOICE_H
#define CUI_CHOICE_H
#include "cui.h"
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
#ifdef __cplusplus
}
#endif
#endif
