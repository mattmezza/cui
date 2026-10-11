/* SPDX-License-Identifier: MIT */
#ifndef CUI_DOCK_H
#define CUI_DOCK_H
#include "cui.h"
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
/* Required serialized length excludes NUL. Insufficient output is untouched.
   Restore is bounded and transactional; successful restore invalidates IDs. */
cui_dock_result cui_dock_serialize(const cui_dock *dock, char *output, size_t capacity,
                                   size_t *length);
cui_dock_result cui_dock_restore(cui_dock *dock, cui_str serialized);
#ifdef __cplusplus
}
#endif
#endif
