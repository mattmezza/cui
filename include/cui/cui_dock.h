/* SPDX-License-Identifier: MIT */
#ifndef CUI_DOCK_H
#define CUI_DOCK_H
#include "cui.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_DOCK_NODE_LIMIT 4095
#define CUI_DOCK_PANEL_LIMIT 4096
#define CUI_DOCK_SERIALIZED_LIMIT 1048576
typedef struct cui_dock cui_dock;
typedef uint64_t cui_dock_node;
typedef enum {
    CUI_DOCK_OK,
    CUI_DOCK_INVALID,
    CUI_DOCK_CAPACITY,
    CUI_DOCK_DUPLICATE_PANEL,
    CUI_DOCK_NOT_FOUND
} cui_dock_result;
typedef enum { CUI_DOCK_TABS, CUI_DOCK_SPLIT_HORIZONTAL, CUI_DOCK_SPLIT_VERTICAL } cui_dock_kind;
typedef struct {
    size_t node_capacity, panel_capacity, max_depth, max_serialized_bytes;
} cui_dock_config;
typedef struct {
    cui_dock_node id, parent, first, second;
    cui_dock_kind kind;
    double ratio, min_width, min_height;
    size_t panel_count;
    bool has_active;
    uint64_t active_panel;
} cui_dock_node_info;
typedef struct {
    cui_dock_node node;
    cui_rect bounds;
    size_t panel_count;
    bool has_active;
    uint64_t active_panel;
} cui_dock_geometry;
cui_dock *cui_dock_create(cui_dock_config config);
void cui_dock_destroy(cui_dock *dock);
cui_dock_node cui_dock_root(const cui_dock *dock);
size_t cui_dock_memory_bytes(const cui_dock *dock);
bool cui_dock_node_get(const cui_dock *dock, cui_dock_node node, cui_dock_node_info *info);
bool cui_dock_panel_at(const cui_dock *dock, cui_dock_node tab, size_t index, uint64_t *panel);
bool cui_dock_panel_find(const cui_dock *dock, uint64_t panel, cui_dock_node *tab, size_t *index);
cui_dock_result cui_dock_insert(cui_dock *dock, cui_dock_node tab, uint64_t panel, size_t index);
cui_dock_result cui_dock_split(cui_dock *dock, cui_dock_node tab, cui_dock_kind axis, double ratio,
                               uint64_t new_panel, bool before, cui_dock_node *new_tab);
cui_dock_result cui_dock_move(cui_dock *dock, uint64_t panel, cui_dock_node destination,
                              size_t index);
/* Atomically move an existing panel into a new split beside destination.
 * Rejection preserves the model and output. A sole panel cannot split itself. */
cui_dock_result cui_dock_move_split(cui_dock *, uint64_t panel, cui_dock_node destination,
                                    cui_dock_kind axis, double ratio, bool before,
                                    cui_dock_node *new_tab);
/* Atomic two-model transfer. TABS inserts at index; split axes create a new leaf.
 * Both models and output are unchanged on rejection. Models must be distinct. */
cui_dock_result cui_dock_transfer(cui_dock *source, uint64_t panel, cui_dock *destination,
                                  cui_dock_node target, size_t index, cui_dock_kind axis,
                                  double ratio, bool before, cui_dock_node *new_tab);
cui_dock_result cui_dock_reorder(cui_dock *dock, uint64_t panel, size_t index);
cui_dock_result cui_dock_remove(cui_dock *dock, uint64_t panel);
cui_dock_result cui_dock_activate(cui_dock *dock, uint64_t panel);
cui_dock_result cui_dock_set_ratio(cui_dock *dock, cui_dock_node split, double ratio);
cui_dock_result cui_dock_set_minimum(cui_dock *dock, cui_dock_node node, double width,
                                     double height);
/* One rectangle per tab node. NULL output queries count. Minima that cannot
   fit are proportionally compressed and fit_minimum becomes false. */
cui_dock_result cui_dock_layout(const cui_dock *dock, cui_rect bounds, float gap,
                                cui_dock_geometry *output, size_t capacity, size_t *count,
                                bool *fit_minimum);
/* Derived split rectangles use the same ratio/minimum compression as layout.
 * Child minima are aggregate subtree minima (before proportional compression). */
typedef struct {
    cui_rect first, second;
    cui_size first_minimum, second_minimum;
    bool fit_minimum;
} cui_dock_split_geometry;
cui_dock_result cui_dock_split_layout(const cui_dock *, cui_dock_node split, cui_rect, float gap,
                                      cui_dock_split_geometry *);
/* Required serialized length excludes NUL. Insufficient output is untouched.
   Restore is bounded and transactional; successful restore invalidates IDs. */
cui_dock_result cui_dock_serialize(const cui_dock *dock, char *output, size_t capacity,
                                   size_t *length);
cui_dock_result cui_dock_restore(cui_dock *dock, cui_str serialized);
/* Styled adapter; the domain model above remains independently linkable. */
typedef struct cui_dock_view cui_dock_view;
typedef struct {
    uint64_t key;
    cui_str title;
    bool closable, disabled;
    void (*render)(cui_context *, uint64_t panel, void *user);
    void *user;
    cui_id focus_target; /* Optional content control; used when focus_content is enabled. */
    bool zoomable;
    void (*toolbar)(cui_context *, uint64_t panel, void *user);
} cui_dock_panel;
typedef enum {
    CUI_DOCK_SELECTED,
    CUI_DOCK_CLOSED,
    CUI_DOCK_REORDERED,
    CUI_DOCK_RESIZED,
    CUI_DOCK_MOVED,
    CUI_DOCK_SPLIT,
    CUI_DOCK_ZOOMED,
    CUI_DOCK_RESTORED
} cui_dock_event;
typedef struct {
    cui_dock_event operation; /* CLOSED, MOVED, SPLIT, REORDERED or ZOOMED */
    uint64_t panel;
    cui_dock_node source, destination;
    size_t index;
    cui_dock_kind axis;
    bool before;
} cui_dock_request;
typedef struct {
    cui_color accepted, rejected, background, text;
} cui_dock_drag_palette;
typedef struct {
    float width, height, tab_height, key_step;
    bool disabled, reduced_motion, allow_reorder, focus_content;
    double now_seconds;
    void (*on_event)(void *user, cui_dock_event, uint64_t panel, cui_dock_node node);
    void *event_user;
    /* Pure callback: queried for preview and again before commit; NULL allows. */
    bool (*allow_operation)(void *user, const cui_dock_request *);
    void *policy_user;
    bool disable_preview, custom_palette;
    cui_dock_drag_palette drag_palette;
    float toolbar_height; /* Zero defaults to 32 when a toolbar/control is present. */
} cui_dock_view_options;
typedef struct {
    cui_dock_result error;
    bool selected, closed, reordered, resized, fit_minimum;
    bool moved, split, denied;
    uint64_t moved_panel;
    uint64_t selected_panel, closed_panel, reordered_panel;
    bool zoomed, restored;
} cui_dock_view_result;
/* Bounded retained interaction storage, no per-frame allocation. Defaults 128/256. */
cui_dock_view *cui_dock_view_create(size_t node_capacity, size_t panel_capacity);
void cui_dock_view_destroy(cui_dock_view *);
void cui_dock_view_reset(cui_dock_view *);
cui_dock_view_options cui_dock_view_options_default(void);
/* View-only zoom leaves model serialization unchanged. Programmatic setters bypass policies. */
bool cui_dock_view_set_zoom(cui_dock_view *, cui_dock *, uint64_t panel, bool zoomed);
bool cui_dock_view_zoomed(const cui_dock_view *, uint64_t *panel);
cui_id cui_dock_zoom_id(cui_id view, uint64_t panel);
cui_id cui_dock_tab_id(cui_id view, uint64_t panel);
cui_id cui_dock_close_id(cui_id view, uint64_t panel);
cui_id cui_dock_group_id(cui_id view, cui_dock_node node);
cui_id cui_dock_resize_id(cui_id view, const cui_dock *, cui_dock_node split);
/* Submit within a frame. width/height must be positive finite logical pixels.
 * Registry strings/user data and view must survive renderer completion.
 * Model may not be mutated/destroyed by render or event callbacks.
 * Selection is immediate; close/reorder commit after scene submission and
 * invalidate a following frame. Missing registry keys paint a placeholder.
 * Resize uses the existing Resizable adapter and updates model ratios. */
cui_dock_view_result cui_dock_view_render(cui_context *, cui_id view_id, const cui_theme *,
                                          cui_dock *, cui_dock_view *, const cui_dock_panel *,
                                          size_t panel_count, cui_dock_view_options);
/* Independently retained edge docks around an ordinary center model/view. */
typedef enum { CUI_DOCK_CENTER, CUI_DOCK_LEFT, CUI_DOCK_RIGHT, CUI_DOCK_BOTTOM } cui_dock_region;
typedef struct cui_dock_workspace cui_dock_workspace;
typedef struct {
    cui_dock *model;
    cui_dock_view *view;
    const cui_dock_panel *panels;
    size_t panel_count;
    cui_size minimum; /* Zero dimensions default to 64 center / 48 edge. */
} cui_dock_region_content;
typedef struct {
    bool open[3];  /* left, right, bottom */
    float size[3]; /* preferred logical pixels, retained when closed/constrained */
} cui_dock_workspace_snapshot;
typedef struct {
    cui_dock_region source_region, destination_region;
    uint64_t panel;
    cui_dock *source_model, *destination_model;
    cui_dock_node source, destination;
    cui_dock_kind axis;
    bool before, opens_edge;
} cui_dock_transfer_request;
typedef struct {
    cui_dock_view_options view;
    void (*on_resize)(void *, cui_dock_region, float size);
    void *resize_user;
    bool disable_cross_region, disable_edge_opening;
    /* Pure policy and post-commit notification; model/registry storage remains host-owned. */
    bool (*allow_transfer)(void *, const cui_dock_transfer_request *);
    void (*on_transfer)(void *, const cui_dock_transfer_request *);
    void *transfer_user;
} cui_dock_workspace_options;
typedef struct {
    cui_dock_result error;
    bool fit_minimum, resized;
    cui_dock_view_result regions[4];
    bool transferred, opened, denied;
    uint64_t transferred_panel;
    cui_dock_region source_region, destination_region;
} cui_dock_workspace_result;
cui_dock_workspace *cui_dock_workspace_create(void);
void cui_dock_workspace_destroy(cui_dock_workspace *);
bool cui_dock_workspace_set_edge(cui_dock_workspace *, cui_dock_region, bool open, float size);
bool cui_dock_workspace_save(const cui_dock_workspace *, cui_dock_workspace_snapshot *);
bool cui_dock_workspace_restore(cui_dock_workspace *, const cui_dock_workspace_snapshot *);
cui_id cui_dock_region_id(cui_id workspace, cui_dock_region);
cui_id cui_dock_edge_resize_id(cui_id workspace, cui_dock_region);
/* regions are CENTER, LEFT, RIGHT, BOTTOM; configured regions require distinct models/views.
 * Model persistence remains separate from the portable edge-size/open snapshot. */
cui_dock_workspace_result cui_dock_workspace_render(cui_context *, cui_id, const cui_theme *,
                                                    cui_dock_workspace *,
                                                    const cui_dock_region_content regions[4],
                                                    cui_dock_workspace_options);
#ifdef __cplusplus
}
#endif
#endif
