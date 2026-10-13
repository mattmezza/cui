/* SPDX-License-Identifier: MIT */
#include "cui/cui_dock.h"
#include "cui/cui_components.h"
#include "cui/cui_resizable.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_dock_node node;
    cui_resizable_state resize;
    cui_resizable_slot slots[2];
    cui_tab_scroll_state scroll;
    cui_tab_drag_state drag;
    cui_menu_tree_state menu;
    bool menu_initialized;
    uint64_t menu_key;
    cui_menu_entry menu_entries[CUI_DOCK_CUSTOM_MENU_LIMIT + 10];
} dock_cache;
struct cui_dock_view {
    size_t node_capacity, panel_capacity, count;
    dock_cache *cache;
    cui_choice_item *items;
    cui_tab_parts *parts;
    uint64_t *keys;
    const cui_dock *model;
    cui_id id;
    bool dragging, crossed, zoomed;
    uint64_t zoom_key;
    uint64_t drag_key;
    float drag_x, drag_y, pointer_x, pointer_y;
};
typedef struct dock_scene dock_scene;
struct dock_scene {
    cui_context *ui;
    cui_id id;
    const cui_theme *theme;
    cui_dock *dock;
    cui_dock_view *view;
    const cui_dock_panel *panels;
    size_t panel_count;
    cui_dock_view_options options;
    cui_dock_view_result result;
    bool pending_close, pending_reorder, pending_zoom, zoom_in;
    uint64_t zoom_key;
    bool pending_bulk, keep_active, pending_menu_split, pending_balance, pending_custom;
    cui_dock_node menu_node;
    uint64_t menu_key;
    cui_dock_kind menu_axis;
    cui_id custom_action;
    uint64_t close_key, reorder_key;
    size_t reorder_index;
    bool pending_drop, edge_before;
    cui_dock_node destination;
    cui_dock_kind edge_axis;
};
static bool dock_allowed(dock_scene *s, cui_dock_event operation, uint64_t panel,
                         cui_dock_node destination, size_t index, cui_dock_kind axis, bool before) {
    if (!s->options.allow_operation)
        return true;
    cui_dock_node source = 0;
    cui_dock_panel_find(s->dock, panel, &source, NULL);
    cui_dock_request request = {operation, panel, source, destination, index, axis, before};
    return s->options.allow_operation(s->options.policy_user, &request);
}
static bool dock_contains(cui_rect b, float x, float y) {
    return b.w > 0 && b.h > 0 && x >= b.x && y >= b.y && x < b.x + b.w && y < b.y + b.h;
}
static void dock_drop_target(dock_scene *s, cui_dock_node node, float x, float y) {
    if (s->view->zoomed && node == cui_dock_root(s->dock)) {
        cui_dock_node maximized;
        if (!cui_dock_panel_find(s->dock, s->view->zoom_key, &maximized, NULL))
            return;
        node = maximized;
    }
    cui_dock_node_info info;
    if (!cui_dock_node_get(s->dock, node, &info))
        return;
    if (info.kind != CUI_DOCK_TABS) {
        dock_drop_target(s, info.first, x, y);
        dock_drop_target(s, info.second, x, y);
        return;
    }
    cui_rect b = cui_bounds(s->ui, cui_dock_group_id(s->id, node));
    if (!dock_contains(b, x, y))
        return;
    cui_dock_node source;
    if (!cui_dock_panel_find(s->dock, s->view->drag_key, &source, NULL))
        return;
    float dx = fminf(32, b.w * .2f), dy = fminf(32, b.h * .2f);
    bool edge = false;
    if (y >= b.y + s->options.tab_height) {
        if (x < b.x + dx || x > b.x + b.w - dx) {
            edge = true;
            s->edge_axis = CUI_DOCK_SPLIT_HORIZONTAL;
            s->edge_before = x < b.x + dx;
        } else if (y < b.y + s->options.tab_height + dy || y > b.y + b.h - dy) {
            edge = true;
            s->edge_axis = CUI_DOCK_SPLIT_VERTICAL;
            s->edge_before = y < b.y + s->options.tab_height + dy;
        }
    }
    if (source != node || edge) {
        s->pending_drop = true;
        s->destination = node;
        if (!edge)
            s->edge_axis = CUI_DOCK_TABS;
    }
}
static void dock_drag_events(dock_scene *s) {
    cui_dock_view *v = s->view;
    if (s->options.disabled || !s->options.allow_reorder || s->options.collapsed) {
        v->dragging = false;
        return;
    }
    if (v->dragging) {
        bool enabled = false;
        for (size_t i = 0; i < s->panel_count; ++i)
            if (s->panels[i].key == v->drag_key && !s->panels[i].disabled)
                enabled = true;
        if (!enabled || !cui_dock_panel_find(s->dock, v->drag_key, NULL, NULL)) {
            v->dragging = false;
            for (size_t i = 0; i < v->count; ++i)
                memset(&v->cache[i].drag, 0, sizeof v->cache[i].drag);
        }
    }
    size_t count;
    const cui_event *events = cui_frame_events(s->ui, &count);
    for (size_t e = 0; e < count; ++e) {
        cui_event event = events[e];
        if (event.type == CUI_EVENT_POINTER_MOVE || event.type == CUI_EVENT_POINTER_DOWN ||
            event.type == CUI_EVENT_POINTER_UP) {
            v->pointer_x = event.x;
            v->pointer_y = event.y;
        }
        if (event.type == CUI_EVENT_CANCEL ||
            (event.type == CUI_EVENT_KEY_DOWN && event.key == CUI_KEY_ESCAPE)) {
            v->dragging = false;
            for (size_t i = 0; i < v->count; ++i)
                memset(&v->cache[i].drag, 0, sizeof v->cache[i].drag);
        } else if (event.type == CUI_EVENT_POINTER_DOWN &&
                   event.pointer_button == CUI_POINTER_PRIMARY) {
            for (size_t i = 0; i < s->panel_count; ++i) {
                const cui_dock_panel *panel = &s->panels[i];
                if (!panel->disabled && cui_dock_panel_find(s->dock, panel->key, NULL, NULL) &&
                    dock_contains(cui_bounds(s->ui, cui_dock_tab_id(s->id, panel->key)), event.x,
                                  event.y) &&
                    !dock_contains(cui_bounds(s->ui, cui_dock_close_id(s->id, panel->key)), event.x,
                                   event.y)) {
                    v->dragging = true;
                    v->crossed = false;
                    v->drag_key = panel->key;
                    v->drag_x = event.x;
                    v->drag_y = event.y;
                    break;
                }
            }
        } else if (v->dragging && event.type == CUI_EVENT_POINTER_MOVE) {
            if (hypotf(event.x - v->drag_x, event.y - v->drag_y) >= 6)
                v->crossed = true;
        } else if (v->dragging && event.type == CUI_EVENT_POINTER_UP &&
                   event.pointer_button == CUI_POINTER_PRIMARY) {
            if (v->crossed)
                dock_drop_target(s, cui_dock_root(s->dock), event.x, event.y);
            v->dragging = false;
        }
    }
}
static void dock_drag_preview(dock_scene *s) {
    cui_dock_view *v = s->view;
    if (!v->dragging || !v->crossed || s->options.disable_preview || s->options.disabled)
        return;
    dock_scene preview = *s;
    preview.pending_drop = false;
    dock_drop_target(&preview, cui_dock_root(s->dock), v->pointer_x, v->pointer_y);
    cui_dock_drag_palette palette =
        s->options.custom_palette ? s->options.drag_palette
                                  : (cui_dock_drag_palette){s->theme->primary, s->theme->danger,
                                                            s->theme->surface, s->theme->text};
    if (preview.pending_drop) {
        bool permitted = dock_allowed(
            s, preview.edge_axis == CUI_DOCK_TABS ? CUI_DOCK_MOVED : CUI_DOCK_SPLIT, v->drag_key,
            preview.destination, SIZE_MAX, preview.edge_axis, preview.edge_before);
        cui_rect b = cui_bounds(s->ui, cui_dock_group_id(s->id, preview.destination));
        b.y += s->options.tab_height;
        b.h = fmaxf(0, b.h - s->options.tab_height);
        if (preview.edge_axis == CUI_DOCK_SPLIT_HORIZONTAL) {
            b.w *= .5f;
            if (!preview.edge_before)
                b.x += b.w;
        } else if (preview.edge_axis == CUI_DOCK_SPLIT_VERTICAL) {
            b.h *= .5f;
            if (!preview.edge_before)
                b.y += b.h;
        }
        cui_color ink = permitted ? palette.accepted : palette.rejected;
        cui_color fill = ink;
        fill.a = (uint8_t)(ink.a / 5);
        cui_begin_portal(
            s->ui, s->id ^ UINT64_C(0x17d932756d9271a9),
            (cui_box){.background = fill, .border = ink, .border_width = 2, .inert = true}, b, 91);
        cui_end_box(s->ui);
    }
    const cui_dock_panel *panel = NULL;
    for (size_t i = 0; i < s->panel_count; ++i)
        if (s->panels[i].key == v->drag_key)
            panel = &s->panels[i];
    cui_rect workspace = cui_bounds(s->ui, s->id);
    float width = fminf(180, workspace.w), height = fminf(32, workspace.h);
    float x = fmaxf(workspace.x, fminf(v->pointer_x + 12, workspace.x + workspace.w - width));
    float y = fmaxf(workspace.y, fminf(v->pointer_y + 12, workspace.y + workspace.h - height));
    cui_begin_portal(s->ui, s->id ^ UINT64_C(0x86ab919826a387b5),
                     (cui_box){.background = palette.background,
                               .border = s->theme->border,
                               .border_width = 1,
                               .clip = true,
                               .inert = true,
                               .padding = 6},
                     (cui_rect){x, y, width, height}, 92);
    cui_text(s->ui, panel ? panel->title : cui_string("Panel"), s->theme->font_size, palette.text);
    cui_end_box(s->ui);
}
static bool dock_first_active(const cui_dock *model, cui_dock_node node, uint64_t *key) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, node, &info))
        return false;
    if (info.kind == CUI_DOCK_TABS) {
        if (!info.has_active)
            return false;
        *key = info.active_panel;
        return true;
    }
    return dock_first_active(model, info.first, key) || dock_first_active(model, info.second, key);
}
static cui_id dock_part(cui_id id, uint64_t key, uint64_t tag) {
    uint64_t x = id ^ (key + tag);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    return x ? x : tag;
}
cui_id cui_dock_menu_id(cui_id view, uint64_t panel) {
    return dock_part(view, panel, UINT64_C(0x734ca23d83928712));
}
cui_id cui_dock_menu_action_id(cui_id view, uint64_t panel, cui_dock_menu_action action) {
    return dock_part(view, panel,
                     UINT64_C(0x5129837a389cbe01) + (uint64_t)action * UINT64_C(0x178ca9376153));
}
cui_id cui_dock_zoom_id(cui_id view, uint64_t panel) {
    return dock_part(view, panel, UINT64_C(0x46ac8d37ab16317e));
}
bool cui_dock_view_set_zoom(cui_dock_view *view, cui_dock *model, uint64_t panel, bool zoomed) {
    if (!view || (zoomed && !cui_dock_panel_find(model, panel, NULL, NULL)))
        return false;
    if (zoomed && view->model && view->model != model)
        cui_dock_view_reset(view);
    if (zoomed) {
        view->model = model;
        cui_dock_activate(model, panel);
    }
    view->zoomed = zoomed;
    view->zoom_key = panel;
    return true;
}
bool cui_dock_view_zoomed(const cui_dock_view *view, uint64_t *panel) {
    if (!view || !view->zoomed)
        return false;
    if (panel)
        *panel = view->zoom_key;
    return true;
}
cui_id cui_dock_tab_id(cui_id view, uint64_t panel) {
    return dock_part(view, panel, UINT64_C(0x4989cd35cae1d3a7));
}
cui_id cui_dock_close_id(cui_id view, uint64_t panel) {
    return dock_part(view, panel, UINT64_C(0x137a4ef7928dcad1));
}
cui_id cui_dock_group_id(cui_id view, cui_dock_node node) {
    return dock_part(view, node, UINT64_C(0xe3d2a54f74c98b51));
}
cui_id cui_dock_resize_id(cui_id view, const cui_dock *model, cui_dock_node split) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, split, &info) || info.kind == CUI_DOCK_TABS)
        return 0;
    cui_id preceding = dock_part(view, info.first, UINT64_C(0x729d1cc28b3a17a1));
    return cui_resizable_handle_id(cui_dock_group_id(view, split), preceding);
}
cui_dock_view *cui_dock_view_create(size_t nodes, size_t panels) {
    if (!nodes)
        nodes = 128;
    if (!panels)
        panels = 256;
    if (nodes > CUI_DOCK_NODE_LIMIT || panels > CUI_DOCK_PANEL_LIMIT)
        return NULL;
    cui_dock_view *view = calloc(1, sizeof *view);
    if (!view)
        return NULL;
    view->node_capacity = nodes;
    view->panel_capacity = panels;
    view->cache = calloc(nodes, sizeof *view->cache);
    view->items = calloc(panels, sizeof *view->items);
    view->parts = calloc(panels, sizeof *view->parts);
    view->keys = calloc(panels, sizeof *view->keys);
    if (!view->cache || !view->items || !view->parts || !view->keys) {
        cui_dock_view_destroy(view);
        return NULL;
    }
    return view;
}
void cui_dock_view_destroy(cui_dock_view *view) {
    if (!view)
        return;
    free(view->cache);
    free(view->items);
    free(view->parts);
    free(view->keys);
    free(view);
}
void cui_dock_view_reset(cui_dock_view *view) {
    if (!view)
        return;
    memset(view->cache, 0, view->node_capacity * sizeof *view->cache);
    view->count = 0;
    view->dragging = false;
    view->crossed = false;
    view->zoomed = false;
    view->model = NULL;
    view->id = 0;
}
cui_dock_view_options cui_dock_view_options_default(void) {
    return (cui_dock_view_options){
        .width = 640, .height = 480, .tab_height = 36, .key_step = 8, .allow_reorder = true};
}
static const cui_dock_panel *dock_panel_get(const dock_scene *s, uint64_t key) {
    for (size_t i = 0; i < s->panel_count; ++i)
        if (s->panels[i].key == key)
            return &s->panels[i];
    return NULL;
}
static void dock_emit(dock_scene *s, cui_dock_event event, uint64_t key, cui_dock_node node) {
    if (s->options.on_event)
        s->options.on_event(s->options.event_user, event, key, node);
}
static dock_cache *dock_cache_get(dock_scene *s, cui_dock_node node) {
    for (size_t i = 0; i < s->view->count; ++i)
        if (s->view->cache[i].node == node)
            return &s->view->cache[i];
    size_t index = s->view->count;
    for (size_t i = 0; i < s->view->count; ++i) {
        cui_dock_node_info info;
        if (!cui_dock_node_get(s->dock, s->view->cache[i].node, &info)) {
            index = i;
            break;
        }
    }
    if (index == s->view->node_capacity)
        return NULL;
    if (index == s->view->count)
        ++s->view->count;
    dock_cache *cache = &s->view->cache[index];
    memset(cache, 0, sizeof *cache);
    cache->node = node;
    cui_resizable_state_init(&cache->resize, cache->slots, 2);
    return cache;
}
static cui_dock_result dock_preflight(const cui_dock *model, cui_dock_node node, size_t *nodes,
                                      size_t *panels) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, node, &info))
        return CUI_DOCK_NOT_FOUND;
    ++*nodes;
    if (info.kind == CUI_DOCK_TABS) {
        *panels += info.panel_count;
        return CUI_DOCK_OK;
    }
    cui_dock_result error = dock_preflight(model, info.first, nodes, panels);
    return error == CUI_DOCK_OK ? dock_preflight(model, info.second, nodes, panels) : error;
}
static void dock_close_suffix(cui_context *ui, const cui_choice_item *item, size_t index,
                              void *user) {
    dock_scene *s = user;
    (void)item;
    uint64_t key = s->view->keys[index];
    const cui_dock_panel *panel = dock_panel_get(s, key);
    if (!panel || !panel->closable)
        return;
    if (cui_button_ex(ui, cui_dock_close_id(s->id, key), s->theme, cui_string("Close panel"),
                      (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                           .size = CUI_CONTROL_SMALL,
                                           .icon = cui_string("close"),
                                           .icon_only = true,
                                           .disabled = s->options.disabled || panel->disabled})) {
        s->pending_close = true;
        s->close_key = key;
    }
}
static void dock_render_menu(dock_scene *s, dock_cache *cache, cui_dock_node_info info,
                             const cui_dock_panel *active) {
    if (!active || s->options.hide_menu)
        return;
    cui_id trigger = cui_dock_menu_id(s->id, active->key);
    if (!cache->menu_initialized || cache->menu_key != active->key) {
        cui_menu_tree_init(&cache->menu, dock_part(s->id, info.id, UINT64_C(0x896ec6237838192a)));
        cache->menu_initialized = true;
        cache->menu_key = active->key;
    }
    bool disabled = s->options.disabled || active->disabled;
    size_t count = 0;
    for (size_t i = 0; i < active->menu_count; ++i) {
        cache->menu_entries[count] = active->menu_items[i];
        cache->menu_entries[count++].disabled |= disabled;
    }
    if (count)
        cache->menu_entries[count++] = (cui_menu_entry){.kind = CUI_MENU_SEPARATOR};
    const char *labels[] = {
        "Close panel",         "Close other panels",
        "Close all panels",    s->view->zoomed ? "Restore workspace" : "Maximize panel",
        "Split horizontally",  "Split vertically",
        "Balance parent split"};
    const char *icons[] = {
        "close",      "copy",         "close",   s->view->zoomed ? "minimize" : "maximize",
        "panel-left", "panel-bottom", "maximize"};
    size_t closable = 0, others = 0;
    for (size_t i = 0; i < info.panel_count; ++i) {
        uint64_t key;
        cui_dock_panel_at(s->dock, info.id, i, &key);
        const cui_dock_panel *panel = dock_panel_get(s, key);
        if (panel && panel->closable && !panel->disabled) {
            ++closable;
            if (key != active->key)
                ++others;
        }
    }

    for (unsigned i = 0; i < 7; ++i) {
        if (i == CUI_DOCK_MENU_ZOOM && !s->view->zoomed &&
            active->zoom_control != CUI_DOCK_ZOOM_BOTH &&
            active->zoom_control != CUI_DOCK_ZOOM_MENU)
            continue;
        bool unavailable = disabled;
        if (i == 0)
            unavailable |= !active->closable;
        if (i == 1)
            unavailable |= !others;
        if (i == 2)
            unavailable |= !closable;
        if (i == 3)
            unavailable = s->view->zoomed ? false : disabled || !active->zoomable;
        if (i == 4 || i == 5)
            unavailable |= info.panel_count < 2 || s->view->zoomed;
        if (i == 6)
            unavailable |= !info.parent || s->view->zoomed;
        cache->menu_entries[count++] = (cui_menu_entry){
            .id = cui_dock_menu_action_id(s->id, active->key, (cui_dock_menu_action)i),
            .label = cui_string(labels[i]),
            .icon = cui_string(icons[i]),
            .disabled = unavailable,
            .kind = CUI_MENU_ACTION};
    }
    cui_id action = 0;
    bool activated = cui_base_menu_tree(
        s->ui, trigger, cui_string("Panel actions"), cache->menu_entries, count, &action,
        &cache->menu, disabled && !s->view->zoomed, cui_frame_delta(s->ui), false);
    cui_button_paint(s->ui, trigger, s->theme, cui_string("Panel actions"),
                     (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                          .size = CUI_CONTROL_SMALL,
                                          .icon = cui_string("ellipsis"),
                                          .icon_only = true,
                                          .selected = cache->menu.overlay.open,
                                          .disabled = disabled && !s->view->zoomed});
    cui_rect viewport = cui_bounds(s->ui, s->id);
    if (viewport.w <= 0 || viewport.h <= 8)
        viewport = (cui_rect){0, 0, s->options.width, s->options.height};
    cui_menu_popup_paint(
        s->ui, s->theme, cui_string("Panel actions"), cache->menu_entries, count, &cache->menu,
        cui_bounds(s->ui, trigger),
        (cui_menu_tree_options){.popup = {.viewport = viewport, .width = 240, .align_end = true},
                                .size = CUI_MENU_SMALL});
    if (!activated)
        return;
    for (unsigned i = 0; i < 7; ++i)
        if (action == cui_dock_menu_action_id(s->id, active->key, (cui_dock_menu_action)i)) {
            s->menu_key = active->key;
            s->menu_node = info.id;
            if (i == 0) {
                s->pending_close = true;
                s->close_key = active->key;
            }
            if (i == 1 || i == 2) {
                s->pending_bulk = true;
                s->keep_active = i == 1;
            }
            if (i == 3) {
                s->pending_zoom = true;
                s->zoom_in = !s->view->zoomed;
                s->zoom_key = active->key;
            }
            if (i == 4 || i == 5) {
                s->pending_menu_split = true;
                s->menu_axis = i == 4 ? CUI_DOCK_SPLIT_HORIZONTAL : CUI_DOCK_SPLIT_VERTICAL;
            }
            if (i == 6)
                s->pending_balance = true;
            return;
        }
    s->pending_custom = true;
    s->menu_key = active->key;
    s->custom_action = action;
}
typedef struct {
    dock_scene *scene;
    cui_id *selected;
    float suffix_width;
} dock_title_context;
static void dock_title_body(cui_context *ui, const cui_choice_item *item, size_t index,
                            void *user) {
    dock_title_context *context = user;
    dock_scene *s = context->scene;
    uint64_t key = s->view->keys[index];
    const cui_dock_panel *panel = dock_panel_get(s, key);
    if (panel && panel->title_render) {
        cui_begin_box(ui, dock_part(s->id, key, UINT64_C(0x348651f58752a026)),
                      (cui_box){.width = panel->title_width,
                                .max_width = panel->title_width,
                                .clip = true,
                                .inert = item->disabled});
        panel->title_render(ui, key, panel->user);
        cui_end_box(ui);
    } else {
        float density = s->theme->font_size / 16;
        cui_tab_metrics metrics = cui_tab_size_metrics(CUI_TAB_DEFAULT, CUI_TAB_MEDIUM, density);
        cui_color ink = *context->selected == item->id || cui_hovered(ui, item->id)
                            ? s->theme->tab_active_foreground
                            : s->theme->tab_foreground;
        if (item->disabled)
            ink = s->theme->muted;
        cui_text(ui, item->label, metrics.font_size, ink);
    }
}
static void dock_title_tail(cui_context *ui, void *user) {
    dock_title_context *context = user;
    dock_scene *s = context->scene;
    if (s->options.collapsed)
        return;
    for (size_t i = 0; i < s->panel_count; ++i) {
        const cui_dock_panel *panel = &s->panels[i];
        if (cui_dock_tab_id(s->id, panel->key) != *context->selected)
            continue;
        cui_begin_box(ui, dock_part(s->id, panel->key, UINT64_C(0xf52720d3a973ba34)),
                      (cui_box){.width = context->suffix_width,
                                .max_width = context->suffix_width,
                                .height = s->options.tab_height,
                                .clip = true,
                                .inert = s->options.disabled || panel->disabled});
        if (panel->title_suffix)
            panel->title_suffix(ui, panel->key, panel->user);
        cui_end_box(ui);
        return;
    }
}
static bool dock_title_valid(const cui_dock_panel *panel) {
    return isfinite(panel->title_width) && panel->title_width >= 0 &&
           isfinite(panel->title_suffix_width) && panel->title_suffix_width >= 0 &&
           (!panel->title_render || panel->title_width > 0) &&
           (!panel->title_suffix || panel->title_suffix_width > 0);
}
static void dock_render_node(dock_scene *, cui_dock_node, float, float);
static void dock_render_tabs(dock_scene *s, cui_dock_node_info info, dock_cache *cache, float w,
                             float h) {
    cui_id group = cui_dock_group_id(s->id, info.id);
    cui_begin_box(s->ui, group,
                  (cui_box){.width = w,
                            .height = h,
                            .fixed_width = w == 0,
                            .fixed_height = h == 0,
                            .clip = true,
                            .background = s->theme->surface});
    cui_interact(s->ui, group, CUI_ROLE_GROUP, cui_string("Dock tab group"), s->options.disabled,
                 false, 0);
    if (!info.panel_count) {
        cui_text(s->ui, cui_string("Empty workspace"), s->theme->font_size, s->theme->muted);
        cui_end_box(s->ui);
        return;
    }
    cui_id selected = 0;
    bool custom_title = false;
    float suffix_width = 0;
    for (size_t i = 0; i < info.panel_count; ++i) {
        uint64_t key;
        cui_dock_panel_at(s->dock, info.id, i, &key);
        const cui_dock_panel *panel = dock_panel_get(s, key);
        if (panel) {
            custom_title |= panel->title_render != NULL;
            if (panel->title_suffix && !s->options.collapsed)
                suffix_width = fmaxf(suffix_width, panel->title_suffix_width);
        }
        s->view->keys[i] = key;
        s->view->items[i] =
            (cui_choice_item){.id = cui_dock_tab_id(s->id, key),
                              .label = panel ? panel->title : cui_string("Unavailable panel"),
                              .disabled = s->options.disabled || (panel && panel->disabled)};
        s->view->parts[i] = (cui_tab_parts){0};
        if (panel && panel->closable && !s->options.collapsed)
            s->view->parts[i] =
                (cui_tab_parts){.suffix = dock_close_suffix, .suffix_width = 28, .user = s};
        if (!s->options.collapsed && info.has_active && info.active_panel == key)
            selected = s->view->items[i].id;
    }
    if (!s->options.allow_reorder || s->options.disabled || s->options.collapsed)
        memset(&cache->drag, 0, sizeof cache->drag);
    cui_id strip = dock_part(s->id, info.id, UINT64_C(0x124b79d89af392e1));
    suffix_width = fminf(suffix_width, fmaxf(0, w - 1));
    dock_title_context title_context = {s, &selected, suffix_width};
    bool changed = cui_tabs(
        s->ui, strip, s->theme, s->view->items, info.panel_count, &selected,
        (cui_tabs_options){.disabled = s->options.disabled,
                           .variant = CUI_TAB_DEFAULT,
                           .height = s->options.tab_height,
                           .scroll = &cache->scroll,
                           .bar_extent = fmaxf(1, w - suffix_width),
                           .render = custom_title ? dock_title_body : NULL,
                           .render_user = &title_context,
                           .suffix = suffix_width > 0 ? dock_title_tail : NULL,
                           .content_user = &title_context,
                           .reveal_selected = true,
                           .scrollbar = true,
                           .parts = s->view->parts,
                           .parts_count = info.panel_count,
                           .drag = s->options.allow_reorder && !s->options.collapsed ? &cache->drag
                                                                                     : NULL},
        NULL, NULL);
    if (changed) {
        for (size_t i = 0; i < info.panel_count; ++i)
            if (s->view->items[i].id == selected) {
                uint64_t key = s->view->keys[i];
                if (cui_dock_activate(s->dock, key) == CUI_DOCK_OK) {
                    if (s->view->zoomed)
                        s->view->zoom_key = key;
                    info.active_panel = key;
                    info.has_active = true;
                    s->result.selected = true;
                    s->result.selected_panel = key;
                    dock_emit(s, CUI_DOCK_SELECTED, key, info.id);
                }
                break;
            }
    }
    if (cache->drag.requested) {
        size_t source = SIZE_MAX, target = info.panel_count;
        for (size_t i = 0; i < info.panel_count; ++i) {
            if (s->view->items[i].id == cache->drag.moved)
                source = i;
            if (s->view->items[i].id == cache->drag.before)
                target = i;
        }
        if (source != SIZE_MAX) {
            if (target > source)
                --target;
            if (target != source) {
                s->pending_reorder = true;
                s->reorder_key = s->view->keys[source];
                s->reorder_index = target;
            }
        }
        cache->drag.requested = false;
    }
    cui_id focused = cui_focused(s->ui);
    size_t event_count;
    const cui_event *events = cui_frame_events(s->ui, &event_count);
    const cui_dock_panel *active = info.has_active ? dock_panel_get(s, info.active_panel) : NULL;
    for (size_t i = 0; active && active->closable && !active->disabled && !s->options.disabled &&
                       !s->options.collapsed && i < event_count;
         ++i) {
        cui_event e = events[i];
        if (e.type == CUI_EVENT_KEY_DOWN && e.key == CUI_KEY_W && !e.repeat &&
            (e.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) && !e.ime_composing &&
            !cui_event_consumed(s->ui, i) &&
            (focused == cui_dock_tab_id(s->id, active->key) ||
             focused == cui_dock_close_id(s->id, active->key))) {
            s->pending_close = true;
            s->close_key = active->key;
            cui_consume_event(s->ui, i);
        }
    }
    if (s->options.collapsed) {
        cui_end_box(s->ui);
        return;
    }
    bool toolbar_zoom = active && active->zoomable &&
                        (active->zoom_control == CUI_DOCK_ZOOM_BOTH ||
                         active->zoom_control == CUI_DOCK_ZOOM_TOOLBAR);
    bool controls =
        active && (active->toolbar || toolbar_zoom || s->view->zoomed || !s->options.hide_menu);
    float toolbar_height =
        controls ? fminf(s->options.toolbar_height, fmaxf(0, h - s->options.tab_height)) : 0;
    if (controls) {
        cui_begin_box(s->ui, dock_part(s->id, info.id, UINT64_C(0x2e89ac418f65c15a)),
                      (cui_box){.width = w,
                                .max_width = w,
                                .height = toolbar_height,
                                .fixed_height = toolbar_height == 0,
                                .row = true,
                                .clip = true,
                                .background = s->theme->surface});
        if (active->toolbar) {
            cui_begin_box(s->ui, dock_part(s->id, info.id, UINT64_C(0x4f856e247ac4c168)),
                          (cui_box){.width = -1,
                                    .height = -1,
                                    .clip = true,
                                    .inert = s->options.disabled || active->disabled});
            active->toolbar(s->ui, active->key, active->user);
            cui_end_box(s->ui);
        }
        if (toolbar_zoom || s->view->zoomed) {
            bool zoomed = s->view->zoomed;
            if (cui_button_ex(
                    s->ui, cui_dock_zoom_id(s->id, active->key), s->theme,
                    cui_string(zoomed ? "Restore workspace" : "Maximize panel"),
                    (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                         .size = CUI_CONTROL_SMALL,
                                         .icon = cui_string(zoomed ? "minimize" : "maximize"),
                                         .icon_only = true,
                                         .disabled = !zoomed &&
                                                     (s->options.disabled || active->disabled)})) {
                s->pending_zoom = true;
                s->zoom_in = !zoomed;
                s->zoom_key = active->key;
            }
        }
        dock_render_menu(s, cache, info, active);
        cui_end_box(s->ui);
    }
    float content_height = fmaxf(0, h - s->options.tab_height - toolbar_height);
    cui_begin_box(s->ui, dock_part(s->id, info.id, UINT64_C(0x3721baf9de47c631)),
                  (cui_box){.width = w,
                            .max_width = w,
                            .height = content_height,
                            .fixed_height = content_height == 0,
                            .clip = true,
                            .inert = s->options.disabled || (active && active->disabled)});
    if (active && active->render)
        active->render(s->ui, active->key, active->user);
    else
        cui_text(s->ui, cui_string("Panel content is not registered"), s->theme->font_size,
                 s->theme->muted);
    if (changed && s->options.focus_content && active && active->focus_target)
        cui_focus(s->ui, active->focus_target);
    cui_end_box(s->ui);
    cui_end_box(s->ui);
}
typedef struct {
    dock_scene *scene;
    cui_dock_node node;
    dock_cache *cache;
    size_t index;
    float cross;
    bool horizontal;
} dock_child_scene;
static void dock_render_child(cui_context *ui, void *user) {
    dock_child_scene *child = user;
    (void)ui;
    float span = child->cache->slots[child->index].size;
    dock_render_node(child->scene, child->node, child->horizontal ? span : child->cross,
                     child->horizontal ? child->cross : span);
}
static void dock_render_node(dock_scene *s, cui_dock_node node, float w, float h) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(s->dock, node, &info)) {
        s->result.error = CUI_DOCK_NOT_FOUND;
        return;
    }
    dock_cache *cache = dock_cache_get(s, node);
    if (!cache) {
        s->result.error = CUI_DOCK_CAPACITY;
        return;
    }
    if (info.kind == CUI_DOCK_TABS) {
        dock_render_tabs(s, info, cache, w, h);
        return;
    }
    bool horizontal = info.kind == CUI_DOCK_SPLIT_HORIZONTAL;
    float extent = horizontal ? w : h, cross = horizontal ? h : w;
    cui_dock_split_geometry geometry;
    cui_dock_result error =
        cui_dock_split_layout(s->dock, node, (cui_rect){0, 0, w, h}, 0, &geometry);
    if (error != CUI_DOCK_OK) {
        s->result.error = error;
        return;
    }
    s->result.fit_minimum &= geometry.fit_minimum;
    float first = horizontal ? geometry.first.w : geometry.first.h;
    if (cache->resize.count == 2 && !cache->resize.dragging) {
        cache->slots[0].size = first;
        cache->slots[1].size = extent - first;
    }
    dock_child_scene children[2] = {{s, info.first, cache, 0, cross, horizontal},
                                    {s, info.second, cache, 1, cross, horizontal}};
    cui_resizable_panel panels[2];
    for (size_t i = 0; i < 2; ++i) {
        cui_size minimum = i == 0 ? geometry.first_minimum : geometry.second_minimum;
        panels[i] = cui_resizable_panel_default(
            dock_part(s->id, children[i].node, UINT64_C(0x729d1cc28b3a17a1)));
        panels[i].initial_size = i == 0 ? first : extent - first;
        panels[i].has_initial_size = true;
        panels[i].custom_range = true;
        panels[i].minimum =
            geometry.fit_minimum ? (horizontal ? minimum.width : minimum.height) : 0;
        panels[i].maximum = fmaxf(extent, panels[i].minimum);
        panels[i].content = dock_render_child;
        panels[i].content_user = &children[i];
        panels[i].accessibility_label =
            cui_string(i == 0 ? "First dock region" : "Second dock region");
    }
    cui_resizable_options options = cui_resizable_options_default();
    options.axis = horizontal ? CUI_RESIZABLE_HORIZONTAL : CUI_RESIZABLE_VERTICAL;
    options.extent = extent;
    options.cross_size = cross;
    options.disabled = s->options.disabled;
    options.key_step = s->options.key_step;
    options.reduced_motion = s->options.reduced_motion;
    options.now_seconds = s->options.now_seconds;
    options.box.clip = true;
    options.box.fixed_width = w == 0;
    options.box.fixed_height = h == 0;
    options.box.width = w;
    options.box.height = h;
    cui_resizable_error resize = cui_resizable(s->ui, cui_dock_group_id(s->id, node), s->theme,
                                               &cache->resize, panels, 2, options);
    if (resize != CUI_RESIZABLE_OK && resize != CUI_RESIZABLE_INFEASIBLE) {
        s->result.error = CUI_DOCK_INVALID;
        return;
    }
    if (extent > 0) {
        double ratio = fmin(.999999, fmax(.000001, cache->slots[0].size / extent));
        if (fabs(ratio - info.ratio) > 1e-6 && fabsf(cache->slots[0].size - first) > .001f) {
            if (cui_dock_set_ratio(s->dock, node, ratio) == CUI_DOCK_OK) {
                s->result.resized = true;
                dock_emit(s, CUI_DOCK_RESIZED, 0, node);
            }
        }
    }
}
static bool dock_custom_menu_valid(cui_id view, uint64_t key, const cui_menu_entry *entries,
                                   size_t count, size_t depth,
                                   cui_id ids[CUI_DOCK_CUSTOM_MENU_LIMIT], size_t *used,
                                   size_t *total) {
    if ((count && !entries) || depth >= CUI_MENU_DEPTH_MAX)
        return false;
    for (size_t i = 0; i < count; ++i) {
        const cui_menu_entry *entry = &entries[i];
        if (++*total > CUI_DOCK_CUSTOM_MENU_LIMIT || entry->kind < CUI_MENU_ACTION ||
            entry->kind > CUI_MENU_SUBMENU)
            return false;
        if ((entry->label.length && !entry->label.data) ||
            (entry->icon.length && !entry->icon.data) ||
            (entry->shortcut.length && !entry->shortcut.data))
            return false;
        if (entry->kind != CUI_MENU_SEPARATOR && entry->kind != CUI_MENU_LABEL) {
            if (!entry->id)
                return false;
            for (size_t j = 0; j < *used; ++j)
                if (ids[j] == entry->id)
                    return false;
            for (unsigned j = 0; j < 7; ++j)
                if (entry->id == cui_dock_menu_action_id(view, key, (cui_dock_menu_action)j))
                    return false;
            ids[(*used)++] = entry->id;
        }
        if (entry->child_count &&
            !dock_custom_menu_valid(view, key, entry->children, entry->child_count, depth + 1, ids,
                                    used, total))
            return false;
    }
    return true;
}
cui_dock_view_result cui_dock_view_render(cui_context *ui, cui_id id, const cui_theme *theme,
                                          cui_dock *model, cui_dock_view *view,
                                          const cui_dock_panel *panels, size_t count,
                                          cui_dock_view_options options) {
    cui_dock_view_result invalid = {.error = CUI_DOCK_INVALID};
    if (!ui || !id || !theme || !model || !view || (count && !panels) ||
        count > CUI_DOCK_PANEL_LIMIT || !isfinite(options.width) || options.width <= 0 ||
        !isfinite(options.height) || options.height <= 0 || !isfinite(options.tab_height) ||
        options.tab_height < 0 || !isfinite(options.key_step) || options.key_step < 0 ||
        !isfinite(options.now_seconds) || !isfinite(options.toolbar_height) ||
        options.toolbar_height < 0)
        return invalid;
    if (!options.tab_height)
        options.tab_height = 36;
    if (!options.toolbar_height)
        options.toolbar_height = 32;
    if (!options.key_step)
        options.key_step = 8;
    for (size_t i = 0; i < count; ++i) {
        cui_id custom_ids[CUI_DOCK_CUSTOM_MENU_LIMIT];
        size_t used = 0, total = 0;
        if (!dock_custom_menu_valid(id, panels[i].key, panels[i].menu_items, panels[i].menu_count,
                                    0, custom_ids, &used, &total))
            return invalid;
        if (!dock_title_valid(&panels[i]))
            return invalid;
        if (panels[i].zoom_control < CUI_DOCK_ZOOM_BOTH ||
            panels[i].zoom_control > CUI_DOCK_ZOOM_NONE)
            return invalid;
        if (panels[i].title.length && !panels[i].title.data)
            return invalid;
        for (size_t j = 0; j < i; ++j)
            if (panels[i].key == panels[j].key)
                return invalid;
    }
    size_t nodes = 0, model_panels = 0;
    cui_dock_result error = dock_preflight(model, cui_dock_root(model), &nodes, &model_panels);
    if (error != CUI_DOCK_OK)
        return (cui_dock_view_result){.error = error};
    if (nodes > view->node_capacity || model_panels > view->panel_capacity)
        return (cui_dock_view_result){.error = CUI_DOCK_CAPACITY};
    if ((view->model && view->model != model) || (view->id && view->id != id))
        cui_dock_view_reset(view);
    view->model = model;
    view->id = id;
    dock_scene scene = {.ui = ui,
                        .id = id,
                        .theme = theme,
                        .dock = model,
                        .view = view,
                        .panels = panels,
                        .panel_count = count,
                        .options = options,
                        .result = {.error = CUI_DOCK_OK, .fit_minimum = true}};
    dock_drag_events(&scene);
    size_t leaves = 0;
    bool fit = true;
    error = cui_dock_layout(model, (cui_rect){0, 0, options.width, options.height}, 0, NULL, 0,
                            &leaves, &fit);
    if (error != CUI_DOCK_OK && error != CUI_DOCK_CAPACITY)
        return (cui_dock_view_result){.error = error};
    scene.result.fit_minimum = fit;
    cui_begin_box(ui, id,
                  (cui_box){.width = options.width,
                            .height = options.height,
                            .clip = true,
                            .background = theme->background});
    cui_interact(ui, id, CUI_ROLE_GROUP, cui_string("Dock workspace"), options.disabled, false, 0);
    cui_dock_node visible_root = cui_dock_root(model);
    if (view->zoomed) {
        if (cui_dock_panel_find(model, view->zoom_key, &visible_root, NULL)) {
            cui_dock_node_info info;
            cui_dock_node_get(model, visible_root, &info);
            scene.result.fit_minimum =
                options.width >= info.min_width && options.height >= info.min_height;
        } else {
            view->zoomed = false;
            scene.result.restored = true;
            dock_emit(&scene, CUI_DOCK_RESTORED, view->zoom_key, 0);
            uint64_t survivor;
            if (dock_first_active(model, cui_dock_root(model), &survivor))
                cui_focus(ui, cui_dock_tab_id(id, survivor));
            visible_root = cui_dock_root(model);
        }
    }
    dock_render_node(&scene, visible_root, options.width, options.height);
    dock_drag_preview(&scene);
    cui_end_box(ui);
    if (scene.pending_zoom) {
        if (scene.zoom_in && !dock_allowed(&scene, CUI_DOCK_ZOOMED, scene.zoom_key, visible_root, 0,
                                           CUI_DOCK_TABS, false))
            scene.result.denied = true;
        else {
            view->zoomed = scene.zoom_in;
            view->zoom_key = scene.zoom_key;
            scene.result.zoomed = scene.zoom_in;
            scene.result.restored = !scene.zoom_in;
            dock_emit(&scene, scene.zoom_in ? CUI_DOCK_ZOOMED : CUI_DOCK_RESTORED, scene.zoom_key,
                      visible_root);
            cui_focus(ui, cui_dock_tab_id(id, scene.zoom_key));
            cui_invalidate(ui);
        }
    } else if (scene.pending_close) {
        cui_dock_node node;
        if (!dock_allowed(&scene, CUI_DOCK_CLOSED, scene.close_key, 0, 0, CUI_DOCK_TABS, false))
            scene.result.denied = true;
        else if (cui_dock_panel_find(model, scene.close_key, &node, NULL) &&
                 cui_dock_remove(model, scene.close_key) == CUI_DOCK_OK) {
            scene.result.closed = true;
            scene.result.closed_panel = scene.close_key;
            scene.result.closed_count = 1;
            dock_emit(&scene, CUI_DOCK_CLOSED, scene.close_key, node);
            cui_dock_node_info info;
            uint64_t survivor;
            if (cui_dock_node_get(model, node, &info) && info.has_active)
                cui_focus(ui, cui_dock_tab_id(id, info.active_panel));
            else if (dock_first_active(model, cui_dock_root(model), &survivor))
                cui_focus(ui, cui_dock_tab_id(id, survivor));
            cui_invalidate(ui);
        }
    } else if (scene.pending_bulk) {
        cui_dock_node_info info;
        size_t count = 0;
        if (cui_dock_node_get(model, scene.menu_node, &info))
            for (size_t i = 0; i < info.panel_count; ++i) {
                uint64_t key;
                cui_dock_panel_at(model, scene.menu_node, i, &key);
                const cui_dock_panel *panel = dock_panel_get(&scene, key);
                if ((scene.keep_active && key == scene.menu_key) || !panel || !panel->closable ||
                    panel->disabled)
                    continue;
                if (!dock_allowed(&scene, CUI_DOCK_CLOSED, key, 0, 0, CUI_DOCK_TABS, false)) {
                    scene.result.denied = true;
                    break;
                }
                view->keys[count++] = key;
            }
        if (!scene.result.denied && count) {
            scene.result.error = cui_dock_remove_many(model, view->keys, count);
            if (scene.result.error == CUI_DOCK_OK) {
                scene.result.closed = true;
                scene.result.closed_count = count;
                scene.result.closed_panel = view->keys[count - 1];
                for (size_t i = 0; i < count; ++i)
                    dock_emit(&scene, CUI_DOCK_CLOSED, view->keys[i], scene.menu_node);
                uint64_t survivor;
                if (dock_first_active(model, cui_dock_root(model), &survivor))
                    cui_focus(ui, cui_dock_tab_id(id, survivor));
                cui_invalidate(ui);
            }
        }
    } else if (scene.pending_menu_split) {
        if (!dock_allowed(&scene, CUI_DOCK_SPLIT, scene.menu_key, scene.menu_node, SIZE_MAX,
                          scene.menu_axis, false))
            scene.result.denied = true;
        else if (nodes + 2 > view->node_capacity)
            scene.result.error = CUI_DOCK_CAPACITY;
        else {
            cui_dock_node destination;
            scene.result.error = cui_dock_move_split(model, scene.menu_key, scene.menu_node,
                                                     scene.menu_axis, .5, false, &destination);
            if (scene.result.error == CUI_DOCK_OK) {
                scene.result.moved = scene.result.split = true;
                scene.result.moved_panel = scene.menu_key;
                dock_emit(&scene, CUI_DOCK_SPLIT, scene.menu_key, destination);
                cui_focus(ui, cui_dock_tab_id(id, scene.menu_key));
                cui_invalidate(ui);
            }
        }
    } else if (scene.pending_balance) {
        cui_dock_node_info info;
        if (cui_dock_node_get(model, scene.menu_node, &info) && info.parent) {
            if (!dock_allowed(&scene, CUI_DOCK_RESIZED, scene.menu_key, info.parent, 0,
                              CUI_DOCK_TABS, false))
                scene.result.denied = true;
            else if (cui_dock_set_ratio(model, info.parent, .5) == CUI_DOCK_OK) {
                scene.result.resized = true;
                dock_emit(&scene, CUI_DOCK_RESIZED, scene.menu_key, info.parent);
                cui_invalidate(ui);
            }
        }
    } else if (scene.pending_custom) {
        const cui_dock_panel *panel = dock_panel_get(&scene, scene.menu_key);
        if (panel && panel->menu_action)
            panel->menu_action(panel->user, scene.menu_key, scene.custom_action);
    } else if (scene.pending_drop) {
        cui_dock_node destination = scene.destination;
        cui_dock_result drop;
        if (!dock_allowed(
                &scene, scene.edge_axis == CUI_DOCK_TABS ? CUI_DOCK_MOVED : CUI_DOCK_SPLIT,
                view->drag_key, destination, SIZE_MAX, scene.edge_axis, scene.edge_before)) {
            scene.result.denied = true;
            drop = CUI_DOCK_OK;
        } else if (scene.edge_axis == CUI_DOCK_TABS)
            drop = cui_dock_move(model, view->drag_key, destination, SIZE_MAX);
        else if (nodes + 2 > view->node_capacity)
            drop = CUI_DOCK_CAPACITY;
        else
            drop = cui_dock_move_split(model, view->drag_key, destination, scene.edge_axis, .5,
                                       scene.edge_before, &destination);
        if (drop == CUI_DOCK_OK && !scene.result.denied) {
            scene.result.moved = true;
            scene.result.split = scene.edge_axis != CUI_DOCK_TABS;
            scene.result.moved_panel = view->drag_key;
            dock_emit(&scene, scene.result.split ? CUI_DOCK_SPLIT : CUI_DOCK_MOVED, view->drag_key,
                      destination);
            cui_focus(ui, cui_dock_tab_id(id, view->drag_key));
            cui_invalidate(ui);
        } else
            scene.result.error = drop;
    } else if (scene.pending_reorder) {
        cui_dock_node node;
        if (!dock_allowed(&scene, CUI_DOCK_REORDERED, scene.reorder_key, 0, scene.reorder_index,
                          CUI_DOCK_TABS, false))
            scene.result.denied = true;
        else if (cui_dock_panel_find(model, scene.reorder_key, &node, NULL) &&
                 cui_dock_reorder(model, scene.reorder_key, scene.reorder_index) == CUI_DOCK_OK) {
            scene.result.reordered = true;
            scene.result.reordered_panel = scene.reorder_key;
            dock_emit(&scene, CUI_DOCK_REORDERED, scene.reorder_key, node);
            cui_invalidate(ui);
        }
    }
    return scene.result;
}

struct cui_dock_workspace {
    cui_dock_workspace_snapshot snapshot;
    cui_resizable_state resize[3];
    cui_resizable_slot slots[3][2];
    cui_id id;
    bool dragging, crossed, pending_focus;
    cui_dock_region source_region, focus_region;
    uint64_t drag_key, focus_key, initial_active;
    bool has_initial_active, cross_region;
    bool bottom_gesture;
    cui_dock_workspace_snapshot bottom_initial;
    float start_x, start_y, pointer_x, pointer_y;
};
cui_dock_workspace *cui_dock_workspace_create(void) {
    cui_dock_workspace *state = calloc(1, sizeof *state);
    if (!state)
        return NULL;
    state->snapshot.size[0] = state->snapshot.size[1] = 200;
    state->snapshot.size[2] = 160;
    for (size_t i = 0; i < 3; ++i)
        cui_resizable_state_init(&state->resize[i], state->slots[i], 2);
    return state;
}
void cui_dock_workspace_destroy(cui_dock_workspace *state) {
    free(state);
}
bool cui_dock_workspace_set_edge(cui_dock_workspace *state, cui_dock_region edge, bool open,
                                 float size) {
    if (!state || edge < CUI_DOCK_LEFT || edge > CUI_DOCK_BOTTOM || !isfinite(size) || size <= 0)
        return false;
    if (!open && state->snapshot.noncollapsible[edge - 1] && state->snapshot.open[edge - 1])
        return false;
    if (edge == CUI_DOCK_BOTTOM)
        state->bottom_gesture = false;
    state->snapshot.open[edge - 1] = open;
    state->snapshot.size[edge - 1] = size;
    cui_resizable_clear(&state->resize[edge - 1]);
    return true;
}
bool cui_dock_workspace_set_collapsible(cui_dock_workspace *state, cui_dock_region edge,
                                        bool collapsible) {
    if (!state || edge < CUI_DOCK_LEFT || edge > CUI_DOCK_BOTTOM)
        return false;
    state->snapshot.noncollapsible[edge - 1] = !collapsible;
    if (edge == CUI_DOCK_BOTTOM)
        state->bottom_gesture = false;
    cui_resizable_clear(&state->resize[edge - 1]);
    return true;
}
bool cui_dock_workspace_save(const cui_dock_workspace *state, cui_dock_workspace_snapshot *out) {
    if (!state || !out)
        return false;
    *out = state->snapshot;
    return true;
}
bool cui_dock_workspace_restore(cui_dock_workspace *state,
                                const cui_dock_workspace_snapshot *saved) {
    if (!state || !saved)
        return false;
    for (size_t i = 0; i < 3; ++i)
        if (!isfinite(saved->size[i]) || saved->size[i] <= 0)
            return false;
    state->snapshot = *saved;
    state->bottom_gesture = false;
    state->dragging = false;
    state->pending_focus = false;
    for (size_t i = 0; i < 3; ++i)
        cui_resizable_clear(&state->resize[i]);
    return true;
}
cui_id cui_dock_region_id(cui_id workspace, cui_dock_region region) {
    if (region < CUI_DOCK_CENTER || region > CUI_DOCK_BOTTOM)
        return 0;
    return dock_part(workspace, (uint64_t)region, UINT64_C(0x87213591a36ef249));
}
cui_id cui_dock_edge_resize_id(cui_id workspace, cui_dock_region region) {
    if (region < CUI_DOCK_LEFT || region > CUI_DOCK_BOTTOM)
        return 0;
    cui_id group = dock_part(workspace, (uint64_t)region, UINT64_C(0x40c96f5e8193287b));
    cui_id preceding = dock_part(workspace, (uint64_t)region, UINT64_C(0x61da9f37b4f591d8));
    return cui_resizable_handle_id(group, preceding);
}
typedef struct {
    cui_context *ui;
    cui_id id;
    const cui_theme *theme;
    cui_dock_workspace *state;
    const cui_dock_region_content *regions;
    cui_dock_workspace_options options;
    cui_dock_workspace_result result;
    bool pending_transfer;
    cui_dock_transfer_request transfer;
} dock_workspace_scene;
static const cui_dock_panel *dock_region_panel(const cui_dock_region_content *region,
                                               uint64_t key) {
    for (size_t i = 0; i < region->panel_count; ++i)
        if (region->panels[i].key == key)
            return &region->panels[i];
    return NULL;
}
static bool dock_region_ready(const cui_dock_region_content *region) {
    return region->model && region->view && (!region->panel_count || region->panels);
}
static bool dock_leaf_at(dock_workspace_scene *s, unsigned region, cui_dock_node node, float x,
                         float y, cui_dock_node *out) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(s->regions[region].model, node, &info))
        return false;
    if (info.kind != CUI_DOCK_TABS)
        return dock_leaf_at(s, region, info.first, x, y, out) ||
               dock_leaf_at(s, region, info.second, x, y, out);
    if (!dock_contains(
            cui_bounds(s->ui,
                       cui_dock_group_id(cui_dock_region_id(s->id, (cui_dock_region)region), node)),
            x, y))
        return false;
    *out = node;
    return true;
}
static cui_dock_node dock_first_leaf(const cui_dock *model, cui_dock_node node) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, node, &info))
        return 0;
    return info.kind == CUI_DOCK_TABS ? node : dock_first_leaf(model, info.first);
}
static bool dock_workspace_target(dock_workspace_scene *s, cui_dock_transfer_request *request) {
    cui_dock_workspace *state = s->state;
    cui_rect area = cui_bounds(s->ui, s->id);
    if (!dock_contains(area, state->pointer_x, state->pointer_y))
        return false;
    unsigned destination = 4;
    bool opens = false;
    float band = fminf(24, fminf(area.w, area.h) * .2f);
    if (!s->options.disable_edge_opening) {
        if (!state->snapshot.open[0] && state->pointer_x < area.x + band)
            destination = 1;
        else if (!state->snapshot.open[1] && state->pointer_x > area.x + area.w - band)
            destination = 2;
        else if (!state->snapshot.open[2] && state->pointer_y > area.y + area.h - band)
            destination = 3;
        opens = destination < 4;
    }
    if (destination == 4)
        for (unsigned i = 0; i < 4; ++i)
            if ((!i || state->snapshot.open[i - 1]) &&
                dock_contains(cui_bounds(s->ui, cui_dock_region_id(s->id, (cui_dock_region)i)),
                              state->pointer_x, state->pointer_y)) {
                destination = i;
                break;
            }
    if (destination == 4 || destination == (unsigned)state->source_region ||
        !dock_region_ready(&s->regions[destination]))
        return false;
    const cui_dock_region_content *source = &s->regions[state->source_region],
                                  *target = &s->regions[destination];
    const cui_dock_panel *panel = dock_region_panel(source, state->drag_key);
    if (!panel || panel->disabled || !dock_region_ready(source))
        return false;
    cui_dock_node source_node, target_node;
    if (!cui_dock_panel_find(source->model, state->drag_key, &source_node, NULL))
        return false;
    if (opens)
        target_node = dock_first_leaf(target->model, cui_dock_root(target->model));
    else if (!dock_leaf_at(s, destination, cui_dock_root(target->model), state->pointer_x,
                           state->pointer_y, &target_node))
        return false;
    *request = (cui_dock_transfer_request){state->source_region,
                                           (cui_dock_region)destination,
                                           state->drag_key,
                                           source->model,
                                           target->model,
                                           source_node,
                                           target_node,
                                           CUI_DOCK_TABS,
                                           false,
                                           opens};
    if (!opens) {
        dock_scene probe = {.ui = s->ui,
                            .id = cui_dock_region_id(s->id, (cui_dock_region)destination),
                            .dock = target->model,
                            .view = target->view,
                            .options = s->options.view};
        if (!probe.options.tab_height)
            probe.options.tab_height = 36;
        /* Derive edge placement from target geometry without requiring panel membership. */
        cui_rect b = cui_bounds(s->ui, cui_dock_group_id(probe.id, target_node));
        if (state->pointer_y >= b.y + probe.options.tab_height) {
            float dx = fminf(32, b.w * .2f), dy = fminf(32, b.h * .2f);
            if (state->pointer_x < b.x + dx || state->pointer_x > b.x + b.w - dx) {
                request->axis = CUI_DOCK_SPLIT_HORIZONTAL;
                request->before = state->pointer_x < b.x + dx;
            } else if (state->pointer_y < b.y + probe.options.tab_height + dy ||
                       state->pointer_y > b.y + b.h - dy) {
                request->axis = CUI_DOCK_SPLIT_VERTICAL;
                request->before = state->pointer_y < b.y + probe.options.tab_height + dy;
            }
        }
    }
    return true;
}
static void dock_workspace_restore_active(dock_workspace_scene *s) {
    cui_dock_workspace *state = s->state;
    cui_dock *model = s->regions[state->source_region].model;
    if (!state->has_initial_active || !model)
        return;
    cui_dock_node node;
    cui_dock_node_info info;
    if (!cui_dock_panel_find(model, state->initial_active, &node, NULL) ||
        !cui_dock_node_get(model, node, &info))
        return;
    if (info.active_panel != state->initial_active &&
        cui_dock_activate(model, state->initial_active) == CUI_DOCK_OK) {
        if (s->options.view.on_event)
            s->options.view.on_event(s->options.view.event_user, CUI_DOCK_SELECTED,
                                     state->initial_active, node);
        s->result.regions[state->source_region].selected = true;
        s->result.regions[state->source_region].selected_panel = state->initial_active;
        cui_invalidate(s->ui);
    }
    state->pending_focus = true;
    state->focus_region = state->source_region;
    state->focus_key = state->initial_active;
}
static void dock_workspace_events(dock_workspace_scene *s) {
    cui_dock_workspace *state = s->state;
    if (s->options.view.disabled || !s->options.view.allow_reorder ||
        s->options.disable_cross_region) {
        if (state->dragging && state->crossed && state->cross_region)
            dock_workspace_restore_active(s);
        state->dragging = false;
        return;
    }
    if (state->dragging && state->source_region != CUI_DOCK_CENTER &&
        !state->snapshot.open[state->source_region - 1]) {
        if (state->crossed && state->cross_region)
            dock_workspace_restore_active(s);
        state->dragging = false;
    }
    size_t count;
    const cui_event *events = cui_frame_events(s->ui, &count);
    for (size_t e = 0; e < count; ++e) {
        cui_event event = events[e];
        if (event.type == CUI_EVENT_CANCEL ||
            (event.type == CUI_EVENT_KEY_DOWN && event.key == CUI_KEY_ESCAPE)) {
            if (state->dragging && state->crossed && state->cross_region)
                dock_workspace_restore_active(s);
            state->dragging = false;
        } else if (event.type == CUI_EVENT_POINTER_DOWN &&
                   event.pointer_button == CUI_POINTER_PRIMARY) {
            state->dragging = false;
            if (!cui_event_hit_within_scope(s->ui, e, s->id))
                continue;
            for (unsigned r = 0; r < 4; ++r) {
                if ((r && !state->snapshot.open[r - 1]) || !dock_region_ready(&s->regions[r]))
                    continue;
                cui_id view_id = cui_dock_region_id(s->id, (cui_dock_region)r);
                for (size_t p = 0; p < s->regions[r].panel_count; ++p) {
                    const cui_dock_panel *panel = &s->regions[r].panels[p];
                    if (!panel->disabled &&
                        cui_dock_panel_find(s->regions[r].model, panel->key, NULL, NULL) &&
                        dock_contains(cui_bounds(s->ui, cui_dock_tab_id(view_id, panel->key)),
                                      event.x, event.y) &&
                        dock_contains(cui_bounds(s->ui, view_id), event.x, event.y) &&
                        !dock_contains(cui_bounds(s->ui, cui_dock_close_id(view_id, panel->key)),
                                       event.x, event.y)) {
                        state->dragging = true;
                        state->crossed = false;
                        state->drag_key = panel->key;
                        state->source_region = (cui_dock_region)r;
                        cui_dock_node node;
                        cui_dock_node_info info;
                        cui_dock_panel_find(s->regions[r].model, panel->key, &node, NULL);
                        cui_dock_node_get(s->regions[r].model, node, &info);
                        state->has_initial_active = info.has_active;
                        state->initial_active = info.active_panel;
                        state->cross_region = false;
                        state->start_x = event.x;
                        state->start_y = event.y;
                    }
                }
            }
        } else if (state->dragging &&
                   (event.type == CUI_EVENT_POINTER_MOVE || event.type == CUI_EVENT_POINTER_UP)) {
            state->pointer_x = event.x;
            state->pointer_y = event.y;
            state->cross_region =
                !dock_contains(cui_bounds(s->ui, cui_dock_region_id(s->id, state->source_region)),
                               event.x, event.y);
            if (hypotf(event.x - state->start_x, event.y - state->start_y) >= 6)
                state->crossed = true;
            if (event.type == CUI_EVENT_POINTER_UP && event.pointer_button == CUI_POINTER_PRIMARY) {
                if (state->crossed && cui_event_hit_within_scope(s->ui, e, s->id))
                    s->pending_transfer = dock_workspace_target(s, &s->transfer);
                state->dragging = false;
            }
        }
    }
}
typedef struct {
    dock_workspace_scene *scene;
    unsigned level, slot;
    bool edge;
    float cross;
} dock_workspace_child;
static void dock_workspace_preview(dock_workspace_scene *s) {
    if (!s->state->dragging || !s->state->crossed || s->options.view.disable_preview)
        return;
    cui_dock_transfer_request request;
    if (!dock_workspace_target(s, &request))
        return;
    const cui_dock_panel *panel =
        dock_region_panel(&s->regions[request.destination_region], request.panel);
    bool permitted = panel && !panel->disabled &&
                     !cui_dock_view_zoomed(s->regions[request.destination_region].view, NULL);
    dock_scene permission = {.dock = request.source_model, .options = s->options.view};
    if (permitted)
        permitted = dock_allowed(
            &permission, request.axis == CUI_DOCK_TABS ? CUI_DOCK_MOVED : CUI_DOCK_SPLIT,
            request.panel, request.destination, SIZE_MAX, request.axis, request.before);
    if (permitted && s->options.allow_transfer)
        permitted = s->options.allow_transfer(s->options.transfer_user, &request);
    cui_rect bounds;
    if (request.opens_edge) {
        bounds = cui_bounds(s->ui, s->id);
        unsigned index = request.destination_region - 1;
        if (request.destination_region == CUI_DOCK_BOTTOM) {
            bounds.h = fminf(bounds.h, s->state->snapshot.size[index]);
            bounds.y += s->options.view.height - bounds.h;
        } else {
            bounds.w = fminf(bounds.w, s->state->snapshot.size[index]);
            if (request.destination_region == CUI_DOCK_RIGHT)
                bounds.x += s->options.view.width - bounds.w;
        }
    } else {
        bounds = cui_bounds(s->ui,
                            cui_dock_group_id(cui_dock_region_id(s->id, request.destination_region),
                                              request.destination));
        float tab = s->options.view.tab_height > 0 ? s->options.view.tab_height : 36;
        bounds.y += tab;
        bounds.h = fmaxf(0, bounds.h - tab);
        if (request.axis == CUI_DOCK_SPLIT_HORIZONTAL) {
            bounds.w *= .5f;
            if (!request.before)
                bounds.x += bounds.w;
        }
        if (request.axis == CUI_DOCK_SPLIT_VERTICAL) {
            bounds.h *= .5f;
            if (!request.before)
                bounds.y += bounds.h;
        }
    }
    cui_color ink = s->options.view.custom_palette
                        ? (permitted ? s->options.view.drag_palette.accepted
                                     : s->options.view.drag_palette.rejected)
                        : (permitted ? s->theme->primary : s->theme->danger);
    cui_color fill = ink;
    fill.a = (uint8_t)(ink.a / 5);
    cui_begin_portal(s->ui, s->id ^ UINT64_C(0x5e928ea123ec931b),
                     (cui_box){.background = fill, .border = ink, .border_width = 2, .inert = true},
                     bounds, 93);
    cui_end_box(s->ui);
}
static void dock_workspace_layer(dock_workspace_scene *, unsigned, float, float);
static void dock_workspace_region(dock_workspace_scene *s, cui_dock_region region, float width,
                                  float height) {
    if (width <= 0 || height <= 0)
        return;
    const cui_dock_region_content *content = &s->regions[region];
    cui_dock_view_options options = s->options.view;
    options.width = width;
    options.height = height;
    if (region == CUI_DOCK_BOTTOM && s->options.collapsible_bottom && !s->state->snapshot.open[2]) {
        options.collapsed = true;
        options.tab_height = fminf(29, height);
    }
    cui_dock_view_result result =
        cui_dock_view_render(s->ui, cui_dock_region_id(s->id, region), s->theme, content->model,
                             content->view, content->panels, content->panel_count, options);
    float min_width = content->minimum.width > 0 ? content->minimum.width : region == 0 ? 64 : 48;
    float min_height = content->minimum.height > 0 ? content->minimum.height
                       : region == 0               ? 64
                                                   : 48;
    s->result.fit_minimum &= width >= min_width && (options.collapsed || height >= min_height);
    s->result.regions[region] = result;
    s->result.fit_minimum &= options.collapsed || result.fit_minimum;
    if (result.error != CUI_DOCK_OK)
        s->result.error = result.error;
}
static void dock_workspace_render_child(cui_context *ui, void *user) {
    (void)ui;
    dock_workspace_child *child = user;
    float span = child->scene->state->slots[child->level][child->slot].size;
    bool horizontal = child->level < 2;
    float width = horizontal ? span : child->cross, height = horizontal ? child->cross : span;
    if (child->edge)
        dock_workspace_region(child->scene, (cui_dock_region)(child->level + 1), width, height);
    else
        dock_workspace_layer(child->scene, child->level + 1, width, height);
}
static cui_size dock_model_minimum(const cui_dock *model, cui_dock_node node) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, node, &info))
        return (cui_size){0};
    if (info.kind == CUI_DOCK_TABS)
        return (cui_size){(float)info.min_width, (float)info.min_height};
    cui_size a = dock_model_minimum(model, info.first), b = dock_model_minimum(model, info.second);
    return info.kind == CUI_DOCK_SPLIT_HORIZONTAL
               ? (cui_size){a.width + b.width, fmaxf(a.height, b.height)}
               : (cui_size){fmaxf(a.width, b.width), a.height + b.height};
}
static float dock_region_minimum(dock_workspace_scene *s, unsigned region, bool horizontal) {
    float value = horizontal ? s->regions[region].minimum.width : s->regions[region].minimum.height;
    cui_size minimum =
        dock_model_minimum(s->regions[region].model, cui_dock_root(s->regions[region].model));
    return fmaxf(value > 0     ? value
                 : region == 0 ? 64
                               : 48,
                 horizontal ? minimum.width : minimum.height);
}
static void dock_workspace_layer(dock_workspace_scene *s, unsigned level, float width,
                                 float height) {
    if (level == 3) {
        dock_workspace_region(s, CUI_DOCK_CENTER, width, height);
        return;
    }
    bool collapse = level == 2 && s->options.collapsible_bottom &&
                    dock_region_ready(&s->regions[CUI_DOCK_BOTTOM]);
    if (level == 2 && !collapse && s->state->bottom_gesture) {
        s->state->bottom_gesture = false;
        cui_resizable_clear(&s->state->resize[2]);
    }
    if (!s->state->snapshot.open[level] && !collapse) {
        dock_workspace_layer(s, level + 1, width, height);
        return;
    }
    bool horizontal = level < 2;
    float extent = horizontal ? width : height, cross = horizontal ? height : width;
    unsigned edge_slot = level == 0 ? 0 : 1;
    float natural_min = dock_region_minimum(s, level + 1, horizontal);
    bool can_close = !s->state->snapshot.noncollapsible[level];
    float edge_min = collapse && (can_close || !s->state->snapshot.open[2]) ? fminf(29, natural_min)
                                                                            : natural_min;
    float remaining_min = dock_region_minimum(s, 0, horizontal);
    if (horizontal && s->state->snapshot.open[2])
        remaining_min = fmaxf(remaining_min, dock_region_minimum(s, 3, true));
    if (level == 0 && s->state->snapshot.open[1])
        remaining_min += dock_region_minimum(s, 2, true);
    bool fit = edge_min + remaining_min <= extent;
    s->result.fit_minimum &= fit && cross >= dock_region_minimum(s, level + 1, !horizontal);
    float preferred =
        collapse && !s->state->snapshot.open[2] ? edge_min : s->state->snapshot.size[level];
    float edge_size =
        fit ? fminf(extent - remaining_min, fmaxf(edge_min, preferred))
            : (edge_min + remaining_min > 0 ? extent * edge_min / (edge_min + remaining_min) : 0);
    cui_resizable_state *resize = &s->state->resize[level];
    cui_resizable_slot *slots = s->state->slots[level];
    if (resize->count == 2 && !resize->dragging) {
        slots[edge_slot].size = edge_size;
        slots[1 - edge_slot].size = extent - edge_size;
    }
    dock_workspace_child children[2] = {{s, level, 0, edge_slot == 0, cross},
                                        {s, level, 1, edge_slot == 1, cross}};
    cui_resizable_panel panels[2];
    for (unsigned i = 0; i < 2; ++i) {
        panels[i] = cui_resizable_panel_default(
            dock_part(s->id, level + 1,
                      i == 0 ? UINT64_C(0x61da9f37b4f591d8) : UINT64_C(0x187d5c928a64937b)));
        panels[i].has_initial_size = true;
        panels[i].initial_size = i == edge_slot ? edge_size : extent - edge_size;
        panels[i].custom_range = true;
        panels[i].minimum = fit ? (i == edge_slot ? edge_min : remaining_min) : 0;
        panels[i].maximum = fmaxf(extent, panels[i].minimum);
        panels[i].content = dock_workspace_render_child;
        panels[i].content_user = &children[i];
        panels[i].accessibility_label =
            cui_string(i == edge_slot ? "Edge dock" : "Center workspace");
    }
    cui_resizable_options options = cui_resizable_options_default();
    options.axis = horizontal ? CUI_RESIZABLE_HORIZONTAL : CUI_RESIZABLE_VERTICAL;
    options.extent = extent;
    options.cross_size = cross;
    options.disabled = s->options.view.disabled;
    options.reduced_motion = s->options.view.reduced_motion;
    options.now_seconds = s->options.view.now_seconds;
    options.key_step = s->options.view.key_step > 0 ? s->options.view.key_step : 8;
    options.box = (cui_box){.width = width,
                            .height = height,
                            .fixed_width = width == 0,
                            .fixed_height = height == 0,
                            .clip = true};
    bool canceled = false, completed = false, keyboard = false;
    if (collapse) {
        size_t count;
        const cui_event *events = cui_frame_events(s->ui, &count);
        cui_id handle = cui_dock_edge_resize_id(s->id, CUI_DOCK_BOTTOM);
        for (size_t i = 0; i < count; ++i) {
            cui_event e = events[i];
            if (!s->options.view.disabled && e.type == CUI_EVENT_POINTER_DOWN &&
                e.pointer_button == CUI_POINTER_PRIMARY && cui_event_target(s->ui, i) == handle) {
                s->state->bottom_initial = s->state->snapshot;
                s->state->bottom_gesture = true;
            }
            canceled |= s->state->bottom_gesture &&
                        (e.type == CUI_EVENT_CANCEL ||
                         (e.type == CUI_EVENT_KEY_DOWN && e.key == CUI_KEY_ESCAPE));
            completed |= s->state->bottom_gesture && e.type == CUI_EVENT_POINTER_UP &&
                         e.pointer_button == CUI_POINTER_PRIMARY;
            keyboard |= e.type == CUI_EVENT_KEY_DOWN && cui_event_target(s->ui, i) == handle &&
                        (e.key == CUI_KEY_UP || e.key == CUI_KEY_DOWN || e.key == CUI_KEY_HOME ||
                         e.key == CUI_KEY_END);
        }
        canceled |= s->state->bottom_gesture &&
                    (s->options.view.disabled || !cui_pointer_scope_allowed(s->ui, handle));
    }
    cui_resizable_error error =
        cui_resizable(s->ui, dock_part(s->id, level + 1, UINT64_C(0x40c96f5e8193287b)), s->theme,
                      resize, panels, 2, options);
    if (error != CUI_RESIZABLE_OK && error != CUI_RESIZABLE_INFEASIBLE)
        s->result.error = CUI_DOCK_INVALID;
    float changed_size = slots[edge_slot].size;
    if (collapse && canceled) {
        s->state->snapshot = s->state->bottom_initial;
        s->state->bottom_gesture = false;
        cui_resizable_clear(resize);
        cui_invalidate(s->ui);
        return;
    }
    if (collapse && (resize->dragging || completed || keyboard)) {
        bool open = !can_close || changed_size > edge_min;
        if (!can_close)
            changed_size = fmaxf(natural_min, changed_size);
        if (completed || keyboard) {
            open = !can_close || changed_size >= (edge_min + natural_min) / 2;
            changed_size = open ? fmaxf(natural_min, changed_size) : edge_min;
            s->state->bottom_gesture = false;
        }
        if (open != s->state->snapshot.open[2]) {
            s->state->snapshot.open[2] = open;
            s->result.opened |= open;
            cui_invalidate(s->ui);
        }
        if (changed_size >= natural_min)
            s->state->snapshot.size[2] = changed_size;
        if (completed || keyboard)
            cui_resizable_clear(resize);
    }
    if (fabsf(changed_size - edge_size) > .001f || (collapse && (completed || keyboard))) {
        if (!collapse)
            s->state->snapshot.size[level] = changed_size;
        s->result.resized = true;
        if (s->options.on_resize)
            s->options.on_resize(s->options.resize_user, (cui_dock_region)(level + 1),
                                 changed_size);
        cui_invalidate(s->ui);
    }
}
cui_dock_workspace_result cui_dock_workspace_render(cui_context *ui, cui_id id,
                                                    const cui_theme *theme,
                                                    cui_dock_workspace *state,
                                                    const cui_dock_region_content regions[4],
                                                    cui_dock_workspace_options options) {
    cui_dock_workspace_result invalid = {.error = CUI_DOCK_INVALID};
    if (!ui || !id || !theme || !state || !regions || !isfinite(options.view.width) ||
        options.view.width <= 0 || !isfinite(options.view.height) || options.view.height <= 0 ||
        !isfinite(options.view.tab_height) || options.view.tab_height < 0 ||
        !isfinite(options.view.toolbar_height) || options.view.toolbar_height < 0 ||
        !isfinite(options.view.key_step) || options.view.key_step < 0 ||
        !isfinite(options.view.now_seconds))
        return invalid;
    for (unsigned i = 0; i < 4; ++i) {
        const cui_dock_region_content *region = &regions[i];
        if (i && !state->snapshot.open[i - 1] && !region->model && !region->view &&
            !region->panels && !region->panel_count)
            continue;
        if (!region->model || !region->view || (region->panel_count && !region->panels) ||
            !isfinite(region->minimum.width) || region->minimum.width < 0 ||
            !isfinite(region->minimum.height) || region->minimum.height < 0)
            return invalid;
        for (unsigned j = 0; j < i; ++j)
            if (regions[j].model &&
                (regions[j].model == region->model || regions[j].view == region->view))
                return invalid;
        if (region->panel_count > CUI_DOCK_PANEL_LIMIT)
            return invalid;
        for (size_t p = 0; p < region->panel_count; ++p) {
            if (!dock_title_valid(&region->panels[p]))
                return invalid;
            if (region->panels[p].zoom_control < CUI_DOCK_ZOOM_BOTH ||
                region->panels[p].zoom_control > CUI_DOCK_ZOOM_NONE)
                return invalid;
            if (region->panels[p].title.length && !region->panels[p].title.data)
                return invalid;
            cui_id menu_ids[CUI_DOCK_CUSTOM_MENU_LIMIT];
            size_t menu_used = 0, menu_total = 0;
            if (!dock_custom_menu_valid(cui_dock_region_id(id, (cui_dock_region)i),
                                        region->panels[p].key, region->panels[p].menu_items,
                                        region->panels[p].menu_count, 0, menu_ids, &menu_used,
                                        &menu_total))
                return invalid;
            for (size_t q = 0; q < p; ++q)
                if (region->panels[p].key == region->panels[q].key)
                    return invalid;
        }
        size_t nodes = 0, panels = 0;
        cui_dock_result error =
            dock_preflight(region->model, cui_dock_root(region->model), &nodes, &panels);
        if (error != CUI_DOCK_OK)
            return (cui_dock_workspace_result){.error = error};
        if (nodes > region->view->node_capacity || panels > region->view->panel_capacity)
            return (cui_dock_workspace_result){.error = CUI_DOCK_CAPACITY};
    }
    if (state->id != id) {
        for (unsigned i = 0; i < 3; ++i)
            cui_resizable_clear(&state->resize[i]);
        state->id = id;
        state->bottom_gesture = false;
        state->dragging = false;
        state->pending_focus = false;
    }
    dock_workspace_scene scene = {.ui = ui,
                                  .id = id,
                                  .theme = theme,
                                  .state = state,
                                  .regions = regions,
                                  .options = options,
                                  .result = {.error = CUI_DOCK_OK, .fit_minimum = true}};
    if (!cui_begin_focus_scope(ui, id, false))
        return invalid;
    if (options.collapsible_bottom && !options.view.disabled && !state->snapshot.open[2] &&
        dock_region_ready(&regions[CUI_DOCK_BOTTOM])) {
        size_t event_count;
        const cui_event *events = cui_frame_events(ui, &event_count);
        for (size_t e = 0; e < event_count; ++e) {
            bool activates = (events[e].type == CUI_EVENT_POINTER_UP &&
                              events[e].pointer_button == CUI_POINTER_PRIMARY) ||
                             (events[e].type == CUI_EVENT_KEY_DOWN &&
                              (events[e].key == CUI_KEY_ENTER || events[e].key == CUI_KEY_SPACE));
            bool permitted = events[e].type == CUI_EVENT_KEY_DOWN
                                 ? cui_pointer_scope_allowed(ui, id)
                                 : cui_event_hit_within_scope(ui, e, id);
            if (!activates || !permitted)
                continue;
            for (size_t p = 0; p < regions[3].panel_count; ++p) {
                const cui_dock_panel *panel = &regions[3].panels[p];
                cui_id tab = cui_dock_tab_id(cui_dock_region_id(id, CUI_DOCK_BOTTOM), panel->key);
                if (!panel->disabled && cui_event_target(ui, e) == tab &&
                    cui_dock_panel_find(regions[3].model, panel->key, NULL, NULL)) {
                    state->snapshot.open[2] = true;
                    cui_resizable_clear(&state->resize[2]);
                    scene.result.opened = true;
                    cui_invalidate(ui);
                }
            }
        }
    }
    dock_workspace_events(&scene);
    cui_begin_box(ui, id,
                  (cui_box){.width = options.view.width,
                            .height = options.view.height,
                            .clip = true,
                            .background = theme->background});
    cui_interact(ui, id, CUI_ROLE_GROUP, cui_string("Dock area"), options.view.disabled, false, 0);
    cui_set_semantic_details(ui, id,
                             (cui_semantic_details){.pointer_activatable = true, .no_focus = true});
    dock_workspace_layer(&scene, 0, options.view.width, options.view.height);
    dock_workspace_preview(&scene);
    cui_end_box(ui);
    if (state->pending_focus) {
        cui_id focus =
            cui_dock_tab_id(cui_dock_region_id(id, state->focus_region), state->focus_key);
        const cui_dock_panel *focused_panel =
            dock_region_panel(&regions[state->focus_region], state->focus_key);
        if (options.view.focus_content && focused_panel && focused_panel->focus_target)
            focus = focused_panel->focus_target;
        cui_focus(ui, focus);
        state->pending_focus = false;
    }
    if (scene.pending_transfer &&
        (scene.result.error != CUI_DOCK_OK || cui_last_error(ui) != CUI_OK)) {
        dock_workspace_restore_active(&scene);
        if (scene.result.error == CUI_DOCK_OK)
            scene.result.error = CUI_DOCK_INVALID;
        scene.pending_transfer = false;
    }
    if (scene.pending_transfer) {
        cui_dock_transfer_request *request = &scene.transfer;
        const cui_dock_region_content *target = &regions[request->destination_region];
        const cui_dock_panel *panel = dock_region_panel(target, request->panel);
        size_t nodes = 0, panels = 0;
        cui_dock_result error =
            dock_preflight(target->model, cui_dock_root(target->model), &nodes, &panels);
        if (!panel)
            error = CUI_DOCK_NOT_FOUND;
        else if (panel->disabled || cui_dock_view_zoomed(target->view, NULL))
            error = CUI_DOCK_INVALID;
        else if (nodes + (request->axis == CUI_DOCK_TABS ? 0 : 2) > target->view->node_capacity ||
                 panels + 1 > target->view->panel_capacity)
            error = CUI_DOCK_CAPACITY;
        dock_scene permission = {.dock = request->source_model, .options = options.view};
        if (error == CUI_DOCK_OK &&
            !dock_allowed(
                &permission, request->axis == CUI_DOCK_TABS ? CUI_DOCK_MOVED : CUI_DOCK_SPLIT,
                request->panel, request->destination, SIZE_MAX, request->axis, request->before))
            scene.result.denied = true;
        if (!scene.result.denied && error == CUI_DOCK_OK && options.allow_transfer &&
            !options.allow_transfer(options.transfer_user, request))
            scene.result.denied = true;
        if (!scene.result.denied && error == CUI_DOCK_OK) {
            cui_dock_node destination;
            error = cui_dock_transfer(request->source_model, request->panel,
                                      request->destination_model, request->destination, SIZE_MAX,
                                      request->axis, .5, request->before, &destination);
            if (error == CUI_DOCK_OK) {
                scene.result.transferred = true;
                scene.result.opened = request->opens_edge;
                scene.result.transferred_panel = request->panel;
                scene.result.source_region = request->source_region;
                scene.result.destination_region = request->destination_region;
                request->destination = destination;
                if (request->opens_edge)
                    state->snapshot.open[request->destination_region - 1] = true;
                state->pending_focus = true;
                state->focus_region = request->destination_region;
                state->focus_key = request->panel;
                if (options.on_transfer)
                    options.on_transfer(options.transfer_user, request);
                cui_invalidate(ui);
            }
        }
        if (error != CUI_DOCK_OK)
            scene.result.error = error;
        if (error != CUI_DOCK_OK || scene.result.denied)
            dock_workspace_restore_active(&scene);
    }
    cui_end_focus_scope(ui);
    return scene.result;
}
