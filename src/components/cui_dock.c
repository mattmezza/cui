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
    uint64_t close_key, reorder_key;
    size_t reorder_index;
    bool pending_drop, edge_before;
    cui_dock_node destination;
    cui_dock_kind edge_axis;
};
static bool allowed(dock_scene *s, cui_dock_event operation, uint64_t panel,
                    cui_dock_node destination, size_t index, cui_dock_kind axis, bool before) {
    if (!s->options.allow_operation)
        return true;
    cui_dock_node source = 0;
    cui_dock_panel_find(s->dock, panel, &source, NULL);
    cui_dock_request request = {operation, panel, source, destination, index, axis, before};
    return s->options.allow_operation(s->options.policy_user, &request);
}
static bool contains(cui_rect b, float x, float y) {
    return b.w > 0 && b.h > 0 && x >= b.x && y >= b.y && x < b.x + b.w && y < b.y + b.h;
}
static void drop_target(dock_scene *s, cui_dock_node node, float x, float y) {
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
        drop_target(s, info.first, x, y);
        drop_target(s, info.second, x, y);
        return;
    }
    cui_rect b = cui_bounds(s->ui, cui_dock_group_id(s->id, node));
    if (!contains(b, x, y))
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
static void drag_events(dock_scene *s) {
    cui_dock_view *v = s->view;
    if (s->options.disabled || !s->options.allow_reorder) {
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
                    contains(cui_bounds(s->ui, cui_dock_tab_id(s->id, panel->key)), event.x,
                             event.y) &&
                    !contains(cui_bounds(s->ui, cui_dock_close_id(s->id, panel->key)), event.x,
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
                drop_target(s, cui_dock_root(s->dock), event.x, event.y);
            v->dragging = false;
        }
    }
}
static void drag_preview(dock_scene *s) {
    cui_dock_view *v = s->view;
    if (!v->dragging || !v->crossed || s->options.disable_preview || s->options.disabled)
        return;
    dock_scene preview = *s;
    preview.pending_drop = false;
    drop_target(&preview, cui_dock_root(s->dock), v->pointer_x, v->pointer_y);
    cui_dock_drag_palette palette =
        s->options.custom_palette ? s->options.drag_palette
                                  : (cui_dock_drag_palette){s->theme->primary, s->theme->danger,
                                                            s->theme->surface, s->theme->text};
    if (preview.pending_drop) {
        bool permitted = allowed(
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
static bool first_active(const cui_dock *model, cui_dock_node node, uint64_t *key) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, node, &info))
        return false;
    if (info.kind == CUI_DOCK_TABS) {
        if (!info.has_active)
            return false;
        *key = info.active_panel;
        return true;
    }
    return first_active(model, info.first, key) || first_active(model, info.second, key);
}
static cui_id dock_part(cui_id id, uint64_t key, uint64_t tag) {
    uint64_t x = id ^ (key + tag);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    return x ? x : tag;
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
static const cui_dock_panel *panel_get(const dock_scene *s, uint64_t key) {
    for (size_t i = 0; i < s->panel_count; ++i)
        if (s->panels[i].key == key)
            return &s->panels[i];
    return NULL;
}
static void emit(dock_scene *s, cui_dock_event event, uint64_t key, cui_dock_node node) {
    if (s->options.on_event)
        s->options.on_event(s->options.event_user, event, key, node);
}
static dock_cache *cache_get(dock_scene *s, cui_dock_node node) {
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
static cui_dock_result preflight(const cui_dock *model, cui_dock_node node, size_t *nodes,
                                 size_t *panels) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, node, &info))
        return CUI_DOCK_NOT_FOUND;
    ++*nodes;
    if (info.kind == CUI_DOCK_TABS) {
        *panels += info.panel_count;
        return CUI_DOCK_OK;
    }
    cui_dock_result error = preflight(model, info.first, nodes, panels);
    return error == CUI_DOCK_OK ? preflight(model, info.second, nodes, panels) : error;
}
static void close_suffix(cui_context *ui, const cui_choice_item *item, size_t index, void *user) {
    dock_scene *s = user;
    (void)item;
    uint64_t key = s->view->keys[index];
    const cui_dock_panel *panel = panel_get(s, key);
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
static void render_node(dock_scene *, cui_dock_node, float, float);
static void render_tabs(dock_scene *s, cui_dock_node_info info, dock_cache *cache, float w,
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
    for (size_t i = 0; i < info.panel_count; ++i) {
        uint64_t key;
        cui_dock_panel_at(s->dock, info.id, i, &key);
        const cui_dock_panel *panel = panel_get(s, key);
        s->view->keys[i] = key;
        s->view->items[i] =
            (cui_choice_item){.id = cui_dock_tab_id(s->id, key),
                              .label = panel ? panel->title : cui_string("Unavailable panel"),
                              .disabled = s->options.disabled || (panel && panel->disabled)};
        s->view->parts[i] = (cui_tab_parts){0};
        if (panel && panel->closable)
            s->view->parts[i] =
                (cui_tab_parts){.suffix = close_suffix, .suffix_width = 28, .user = s};
        if (info.has_active && info.active_panel == key)
            selected = s->view->items[i].id;
    }
    if (!s->options.allow_reorder || s->options.disabled)
        memset(&cache->drag, 0, sizeof cache->drag);
    cui_id strip = dock_part(s->id, info.id, UINT64_C(0x124b79d89af392e1));
    bool changed =
        cui_tabs(s->ui, strip, s->theme, s->view->items, info.panel_count, &selected,
                 (cui_tabs_options){.disabled = s->options.disabled,
                                    .variant = CUI_TAB_DEFAULT,
                                    .height = s->options.tab_height,
                                    .scroll = &cache->scroll,
                                    .bar_extent = w,
                                    .reveal_selected = true,
                                    .scrollbar = true,
                                    .parts = s->view->parts,
                                    .parts_count = info.panel_count,
                                    .drag = s->options.allow_reorder ? &cache->drag : NULL},
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
                    emit(s, CUI_DOCK_SELECTED, key, info.id);
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
    const cui_dock_panel *active = info.has_active ? panel_get(s, info.active_panel) : NULL;
    for (size_t i = 0;
         active && active->closable && !active->disabled && !s->options.disabled && i < event_count;
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
    bool controls = active && (active->toolbar || active->zoomable || s->view->zoomed);
    float toolbar_height =
        controls ? fminf(s->options.toolbar_height, fmaxf(0, h - s->options.tab_height)) : 0;
    if (controls) {
        cui_begin_box(s->ui, dock_part(s->id, info.id, UINT64_C(0x2e89ac418f65c15a)),
                      (cui_box){.width = -1,
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
        if (active->zoomable || s->view->zoomed) {
            bool zoomed = s->view->zoomed;
            if (cui_button_ex(s->ui, cui_dock_zoom_id(s->id, active->key), s->theme,
                              cui_string(zoomed ? "Restore workspace" : "Maximize panel"),
                              (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                                   .size = CUI_CONTROL_SMALL,
                                                   .disabled = !zoomed && (s->options.disabled ||
                                                                           active->disabled)})) {
                s->pending_zoom = true;
                s->zoom_in = !zoomed;
                s->zoom_key = active->key;
            }
        }
        cui_end_box(s->ui);
    }
    float content_height = fmaxf(0, h - s->options.tab_height - toolbar_height);
    cui_begin_box(s->ui, dock_part(s->id, info.id, UINT64_C(0x3721baf9de47c631)),
                  (cui_box){.width = -1,
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
} child_scene;
static void render_child(cui_context *ui, void *user) {
    child_scene *child = user;
    (void)ui;
    float span = child->cache->slots[child->index].size;
    render_node(child->scene, child->node, child->horizontal ? span : child->cross,
                child->horizontal ? child->cross : span);
}
static void render_node(dock_scene *s, cui_dock_node node, float w, float h) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(s->dock, node, &info)) {
        s->result.error = CUI_DOCK_NOT_FOUND;
        return;
    }
    dock_cache *cache = cache_get(s, node);
    if (!cache) {
        s->result.error = CUI_DOCK_CAPACITY;
        return;
    }
    if (info.kind == CUI_DOCK_TABS) {
        render_tabs(s, info, cache, w, h);
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
    child_scene children[2] = {{s, info.first, cache, 0, cross, horizontal},
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
        panels[i].content = render_child;
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
                emit(s, CUI_DOCK_RESIZED, 0, node);
            }
        }
    }
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
        if (panels[i].title.length && !panels[i].title.data)
            return invalid;
        for (size_t j = 0; j < i; ++j)
            if (panels[i].key == panels[j].key)
                return invalid;
    }
    size_t nodes = 0, model_panels = 0;
    cui_dock_result error = preflight(model, cui_dock_root(model), &nodes, &model_panels);
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
    drag_events(&scene);
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
            emit(&scene, CUI_DOCK_RESTORED, view->zoom_key, 0);
            uint64_t survivor;
            if (first_active(model, cui_dock_root(model), &survivor))
                cui_focus(ui, cui_dock_tab_id(id, survivor));
            visible_root = cui_dock_root(model);
        }
    }
    render_node(&scene, visible_root, options.width, options.height);
    drag_preview(&scene);
    cui_end_box(ui);
    if (scene.pending_zoom) {
        if (scene.zoom_in && !allowed(&scene, CUI_DOCK_ZOOMED, scene.zoom_key, visible_root, 0,
                                      CUI_DOCK_TABS, false))
            scene.result.denied = true;
        else {
            view->zoomed = scene.zoom_in;
            view->zoom_key = scene.zoom_key;
            scene.result.zoomed = scene.zoom_in;
            scene.result.restored = !scene.zoom_in;
            emit(&scene, scene.zoom_in ? CUI_DOCK_ZOOMED : CUI_DOCK_RESTORED, scene.zoom_key,
                 visible_root);
            cui_focus(ui, cui_dock_tab_id(id, scene.zoom_key));
            cui_invalidate(ui);
        }
    } else if (scene.pending_close) {
        cui_dock_node node;
        if (!allowed(&scene, CUI_DOCK_CLOSED, scene.close_key, 0, 0, CUI_DOCK_TABS, false))
            scene.result.denied = true;
        else if (cui_dock_panel_find(model, scene.close_key, &node, NULL) &&
                 cui_dock_remove(model, scene.close_key) == CUI_DOCK_OK) {
            scene.result.closed = true;
            scene.result.closed_panel = scene.close_key;
            emit(&scene, CUI_DOCK_CLOSED, scene.close_key, node);
            cui_dock_node_info info;
            uint64_t survivor;
            if (cui_dock_node_get(model, node, &info) && info.has_active)
                cui_focus(ui, cui_dock_tab_id(id, info.active_panel));
            else if (first_active(model, cui_dock_root(model), &survivor))
                cui_focus(ui, cui_dock_tab_id(id, survivor));
            cui_invalidate(ui);
        }
    } else if (scene.pending_drop) {
        cui_dock_node destination = scene.destination;
        cui_dock_result drop;
        if (!allowed(&scene, scene.edge_axis == CUI_DOCK_TABS ? CUI_DOCK_MOVED : CUI_DOCK_SPLIT,
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
            emit(&scene, scene.result.split ? CUI_DOCK_SPLIT : CUI_DOCK_MOVED, view->drag_key,
                 destination);
            cui_focus(ui, cui_dock_tab_id(id, view->drag_key));
            cui_invalidate(ui);
        } else
            scene.result.error = drop;
    } else if (scene.pending_reorder) {
        cui_dock_node node;
        if (!allowed(&scene, CUI_DOCK_REORDERED, scene.reorder_key, 0, scene.reorder_index,
                     CUI_DOCK_TABS, false))
            scene.result.denied = true;
        else if (cui_dock_panel_find(model, scene.reorder_key, &node, NULL) &&
                 cui_dock_reorder(model, scene.reorder_key, scene.reorder_index) == CUI_DOCK_OK) {
            scene.result.reordered = true;
            scene.result.reordered_panel = scene.reorder_key;
            emit(&scene, CUI_DOCK_REORDERED, scene.reorder_key, node);
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
    state->snapshot.open[edge - 1] = open;
    state->snapshot.size[edge - 1] = size;
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
} workspace_scene;
typedef struct {
    workspace_scene *scene;
    unsigned level, slot;
    bool edge;
    float cross;
} workspace_child;
static void workspace_layer(workspace_scene *, unsigned, float, float);
static void workspace_region(workspace_scene *s, cui_dock_region region, float width,
                             float height) {
    if (width <= 0 || height <= 0)
        return;
    const cui_dock_region_content *content = &s->regions[region];
    cui_dock_view_options options = s->options.view;
    options.width = width;
    options.height = height;
    cui_dock_view_result result =
        cui_dock_view_render(s->ui, cui_dock_region_id(s->id, region), s->theme, content->model,
                             content->view, content->panels, content->panel_count, options);
    float min_width = content->minimum.width > 0 ? content->minimum.width : region == 0 ? 64 : 48;
    float min_height = content->minimum.height > 0 ? content->minimum.height
                       : region == 0               ? 64
                                                   : 48;
    s->result.fit_minimum &= width >= min_width && height >= min_height;
    s->result.regions[region] = result;
    s->result.fit_minimum &= result.fit_minimum;
    if (result.error != CUI_DOCK_OK)
        s->result.error = result.error;
}
static void workspace_render_child(cui_context *ui, void *user) {
    (void)ui;
    workspace_child *child = user;
    float span = child->scene->state->slots[child->level][child->slot].size;
    bool horizontal = child->level < 2;
    float width = horizontal ? span : child->cross, height = horizontal ? child->cross : span;
    if (child->edge)
        workspace_region(child->scene, (cui_dock_region)(child->level + 1), width, height);
    else
        workspace_layer(child->scene, child->level + 1, width, height);
}
static cui_size model_minimum(const cui_dock *model, cui_dock_node node) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(model, node, &info))
        return (cui_size){0};
    if (info.kind == CUI_DOCK_TABS)
        return (cui_size){(float)info.min_width, (float)info.min_height};
    cui_size a = model_minimum(model, info.first), b = model_minimum(model, info.second);
    return info.kind == CUI_DOCK_SPLIT_HORIZONTAL
               ? (cui_size){a.width + b.width, fmaxf(a.height, b.height)}
               : (cui_size){fmaxf(a.width, b.width), a.height + b.height};
}
static float region_minimum(workspace_scene *s, unsigned region, bool horizontal) {
    float value = horizontal ? s->regions[region].minimum.width : s->regions[region].minimum.height;
    cui_size minimum =
        model_minimum(s->regions[region].model, cui_dock_root(s->regions[region].model));
    return fmaxf(value > 0     ? value
                 : region == 0 ? 64
                               : 48,
                 horizontal ? minimum.width : minimum.height);
}
static void workspace_layer(workspace_scene *s, unsigned level, float width, float height) {
    if (level == 3) {
        workspace_region(s, CUI_DOCK_CENTER, width, height);
        return;
    }
    if (!s->state->snapshot.open[level]) {
        workspace_layer(s, level + 1, width, height);
        return;
    }
    bool horizontal = level < 2;
    float extent = horizontal ? width : height, cross = horizontal ? height : width;
    unsigned edge_slot = level == 0 ? 0 : 1;
    float edge_min = region_minimum(s, level + 1, horizontal);
    float remaining_min = region_minimum(s, 0, horizontal);
    if (horizontal && s->state->snapshot.open[2])
        remaining_min = fmaxf(remaining_min, region_minimum(s, 3, true));
    if (level == 0 && s->state->snapshot.open[1])
        remaining_min += region_minimum(s, 2, true);
    bool fit = edge_min + remaining_min <= extent;
    s->result.fit_minimum &= fit && cross >= region_minimum(s, level + 1, !horizontal);
    float edge_size =
        fit ? fminf(extent - remaining_min, fmaxf(edge_min, s->state->snapshot.size[level]))
            : (edge_min + remaining_min > 0 ? extent * edge_min / (edge_min + remaining_min) : 0);
    cui_resizable_state *resize = &s->state->resize[level];
    cui_resizable_slot *slots = s->state->slots[level];
    if (resize->count == 2 && !resize->dragging) {
        slots[edge_slot].size = edge_size;
        slots[1 - edge_slot].size = extent - edge_size;
    }
    workspace_child children[2] = {{s, level, 0, edge_slot == 0, cross},
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
        panels[i].content = workspace_render_child;
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
    cui_resizable_error error =
        cui_resizable(s->ui, dock_part(s->id, level + 1, UINT64_C(0x40c96f5e8193287b)), s->theme,
                      resize, panels, 2, options);
    if (error != CUI_RESIZABLE_OK && error != CUI_RESIZABLE_INFEASIBLE)
        s->result.error = CUI_DOCK_INVALID;
    if (fabsf(slots[edge_slot].size - edge_size) > .001f) {
        s->state->snapshot.size[level] = slots[edge_slot].size;
        s->result.resized = true;
        if (s->options.on_resize)
            s->options.on_resize(s->options.resize_user, (cui_dock_region)(level + 1),
                                 slots[edge_slot].size);
    }
}
cui_dock_workspace_result cui_dock_workspace_render(cui_context *ui, cui_id id,
                                                    const cui_theme *theme,
                                                    cui_dock_workspace *state,
                                                    const cui_dock_region_content regions[4],
                                                    cui_dock_workspace_options options) {
    cui_dock_workspace_result invalid = {.error = CUI_DOCK_INVALID};
    if (!ui || !id || !theme || !state || !regions || !isfinite(options.view.width) ||
        options.view.width <= 0 || !isfinite(options.view.height) || options.view.height <= 0)
        return invalid;
    for (unsigned i = 0; i < 4; ++i) {
        if (i && !state->snapshot.open[i - 1])
            continue;
        const cui_dock_region_content *region = &regions[i];
        if (!region->model || !region->view || (region->panel_count && !region->panels) ||
            !isfinite(region->minimum.width) || region->minimum.width < 0 ||
            !isfinite(region->minimum.height) || region->minimum.height < 0)
            return invalid;
        for (unsigned j = 0; j < i; ++j)
            if ((!j || state->snapshot.open[j - 1]) &&
                (regions[j].model == region->model || regions[j].view == region->view))
                return invalid;
        if (region->panel_count > CUI_DOCK_PANEL_LIMIT)
            return invalid;
        for (size_t p = 0; p < region->panel_count; ++p) {
            if (region->panels[p].title.length && !region->panels[p].title.data)
                return invalid;
            for (size_t q = 0; q < p; ++q)
                if (region->panels[p].key == region->panels[q].key)
                    return invalid;
        }
        size_t nodes = 0, panels = 0;
        cui_dock_result error =
            preflight(region->model, cui_dock_root(region->model), &nodes, &panels);
        if (error != CUI_DOCK_OK)
            return (cui_dock_workspace_result){.error = error};
        if (nodes > region->view->node_capacity || panels > region->view->panel_capacity)
            return (cui_dock_workspace_result){.error = CUI_DOCK_CAPACITY};
    }
    if (state->id != id) {
        for (unsigned i = 0; i < 3; ++i)
            cui_resizable_clear(&state->resize[i]);
        state->id = id;
    }
    workspace_scene scene = {
        ui, id, theme, state, regions, options, {.error = CUI_DOCK_OK, .fit_minimum = true}};
    cui_begin_box(ui, id,
                  (cui_box){.width = options.view.width,
                            .height = options.view.height,
                            .clip = true,
                            .background = theme->background});
    cui_interact(ui, id, CUI_ROLE_GROUP, cui_string("Dock area"), options.view.disabled, false, 0);
    workspace_layer(&scene, 0, options.view.width, options.view.height);
    cui_end_box(ui);
    return scene.result;
}
