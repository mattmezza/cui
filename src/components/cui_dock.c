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
    bool dragging, crossed;
    uint64_t drag_key;
    float drag_x, drag_y;
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
    bool pending_close, pending_reorder;
    uint64_t close_key, reorder_key;
    size_t reorder_index;
    bool pending_drop, edge_before;
    cui_dock_node destination;
    cui_dock_kind edge_axis;
};
static bool contains(cui_rect b, float x, float y) {
    return b.w > 0 && b.h > 0 && x >= b.x && y >= b.y && x < b.x + b.w && y < b.y + b.h;
}
static void drop_target(dock_scene *s, cui_dock_node node, float x, float y) {
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
    size_t count;
    const cui_event *events = cui_frame_events(s->ui, &count);
    for (size_t e = 0; e < count; ++e) {
        cui_event event = events[e];
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
    float content_height = fmaxf(0, h - s->options.tab_height);
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
        !isfinite(options.now_seconds))
        return invalid;
    if (!options.tab_height)
        options.tab_height = 36;
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
    if (view->model != model || view->id != id)
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
    render_node(&scene, cui_dock_root(model), options.width, options.height);
    cui_end_box(ui);
    if (scene.pending_close) {
        cui_dock_node node;
        if (cui_dock_panel_find(model, scene.close_key, &node, NULL) &&
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
        if (scene.edge_axis == CUI_DOCK_TABS)
            drop = cui_dock_move(model, view->drag_key, destination, SIZE_MAX);
        else if (nodes + 2 > view->node_capacity)
            drop = CUI_DOCK_CAPACITY;
        else
            drop = cui_dock_move_split(model, view->drag_key, destination, scene.edge_axis, .5,
                                       scene.edge_before, &destination);
        if (drop == CUI_DOCK_OK) {
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
        if (cui_dock_panel_find(model, scene.reorder_key, &node, NULL) &&
            cui_dock_reorder(model, scene.reorder_key, scene.reorder_index) == CUI_DOCK_OK) {
            scene.result.reordered = true;
            scene.result.reordered_panel = scene.reorder_key;
            emit(&scene, CUI_DOCK_REORDERED, scene.reorder_key, node);
            cui_invalidate(ui);
        }
    }
    return scene.result;
}
