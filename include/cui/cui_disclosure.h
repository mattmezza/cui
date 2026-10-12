/* SPDX-License-Identifier: MIT */
#ifndef CUI_DISCLOSURE_H
#define CUI_DISCLOSURE_H
#include "cui.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Arrays and controlled expanded state belong to the caller. IDs must be unique,
 * nonzero and stable across frames, including IDs submitted by panel callbacks. */
typedef struct {
    cui_id id;
    cui_str label;
    bool disabled;
} cui_disclosure_item;
/* Custom title/icon body; trigger semantics and activation remain item-owned. */
typedef void (*cui_disclosure_header_fn)(cui_context *, const cui_disclosure_item *, size_t index,
                                         bool expanded, void *user);
typedef struct {
    bool multiple, collapsible, disabled, bordered;
    cui_disclosure_header_fn render_header;
    void *header_user;
    const cui_box *trigger_boxes, *panel_boxes; /* optional per-item full box styles */
    size_t style_count; /* available entries in either nonnull style array */
} cui_accordion_options;
typedef struct {
    bool disabled, borderless;
    cui_disclosure_header_fn render_header;
    void *header_user;
    const cui_box *trigger_box, *panel_box; /* optional full box styles */
} cui_collapsible_options;
typedef void (*cui_disclosure_panel_fn)(cui_context *, size_t index, void *user);
/* Base submits semantic triggers without layout or painting. */
bool cui_base_collapsible(cui_context *, cui_id, cui_str, bool *open, bool disabled);
bool cui_base_accordion(cui_context *, const cui_disclosure_item *, size_t count, bool *open,
                        cui_accordion_options);
/* Styled components render panels synchronously only while expanded. Size and
 * colors come from the supplied theme; callbacks can compose arbitrary CUI. */
bool cui_collapsible_ex(cui_context *, cui_id, const cui_theme *, cui_str, bool *open,
                        cui_collapsible_options, cui_disclosure_panel_fn, void *);
bool cui_collapsible(cui_context *, cui_id, const cui_theme *, cui_str, bool *open, bool disabled,
                     cui_disclosure_panel_fn, void *);
bool cui_accordion(cui_context *, cui_id container, const cui_theme *, const cui_disclosure_item *,
                   size_t count, bool *open, cui_accordion_options, cui_disclosure_panel_fn,
                   void *);
#ifdef __cplusplus
}
#endif
#endif
