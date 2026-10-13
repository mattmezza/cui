/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_dock.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_context *ui;
static cui_dock *model;
static cui_dock_view *view;
static const cui_id workspace = 500;
static cui_dock_panel panels[5];
static unsigned rendered[5], selected_events, closed_events, reordered_events, resized_events;
static unsigned moved_events, split_events;
static cui_dock_view_result result;
static cui_draw_list last_draw;
static cui_dock_result expected_error;
static unsigned preview_backgrounds, accepted_indicators, rejected_indicators;
static cui_dock_event deny_operation;
static unsigned policy_calls;
static bool policy(void *user, const cui_dock_request *request) {
    assert(user == &policy_calls);
    ++policy_calls;
    assert(request->source && cui_dock_panel_find(model, request->panel, NULL, NULL));
    return request->operation != deny_operation;
}
static cui_dock_view_options options;
static void content(cui_context *ctx, uint64_t key, void *user) {
    (void)user;
    unsigned index = key == UINT64_MAX ? 4 : (unsigned)key;
    assert(index < 5);
    ++rendered[index];
    cui_text(ctx, cui_string("Active panel content"), 16, (cui_color){20, 30, 40, 255});
    cui_theme theme = cui_theme_light();
    cui_button(ctx, 800 + index, &theme, cui_string("Content action"), false);
}
static void event(void *user, cui_dock_event kind, uint64_t key, cui_dock_node node) {
    (void)user;
    (void)key;
    (void)node;
    if (kind == CUI_DOCK_SELECTED)
        ++selected_events;
    else if (kind == CUI_DOCK_CLOSED)
        ++closed_events;
    else if (kind == CUI_DOCK_REORDERED)
        ++reordered_events;
    else if (kind == CUI_DOCK_RESIZED)
        ++resized_events;
    else if (kind == CUI_DOCK_MOVED)
        ++moved_events;
    else if (kind == CUI_DOCK_SPLIT)
        ++split_events;
}
static void frame(void) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    result = cui_dock_view_render(ui, workspace, &theme, model, view, panels, 5, options);
    cui_draw_list commands = cui_end_frame(ui);
    last_draw = commands;
    assert(result.error == expected_error && cui_last_error(ui) == CUI_OK && commands.count > 0);
    preview_backgrounds = accepted_indicators = rejected_indicators = 0;
    for (size_t i = 0; i < commands.count; ++i) {
        cui_color color = commands.commands[i].color;
        if (color.r == 7 && color.g == 13 && color.b == 19)
            ++preview_backgrounds;
        if (color.r == 31 && color.g == 37 && color.b == 41)
            ++accepted_indicators;
        if (color.r == 43 && color.g == 47 && color.b == 53)
            ++rejected_indicators;
        cui_rect b = commands.commands[i].bounds;
        assert(isfinite(b.x) && isfinite(b.y) && isfinite(b.w) && isfinite(b.h));
    }
}
static void pointer(cui_event_type type, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y}));
}
static void click(cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    assert(b.w > 0 && b.h > 0);
    pointer(CUI_EVENT_POINTER_DOWN, b.x + b.w / 2, b.y + b.h / 2);
    pointer(CUI_EVENT_POINTER_UP, b.x + b.w / 2, b.y + b.h / 2);
    frame();
}
static void key(cui_key code, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code, .modifiers = modifiers}));
    frame();
}
static uint64_t active(cui_dock_node node) {
    cui_dock_node_info info;
    assert(cui_dock_node_get(model, node, &info) && info.has_active);
    return info.active_panel;
}
static void gesture(uint64_t key, cui_dock_node target, unsigned edge, cui_event_type cancel) {
    frame();
    frame();
    cui_rect tab = cui_bounds(ui, cui_dock_tab_id(workspace, key));
    cui_rect group = cui_bounds(ui, cui_dock_group_id(workspace, target));
    assert(tab.w > 0 && group.w > 0);
    float x = group.x + group.w / 2, y = group.y + group.h / 2;
    if (edge == 1)
        x = group.x + 2;
    if (edge == 2)
        x = group.x + group.w - 2;
    if (edge == 3)
        y = group.y + options.tab_height + 2;
    if (edge == 4)
        y = group.y + group.h - 2;
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, x, y);
    frame();
    if (cancel == CUI_EVENT_CANCEL) {
        pointer(CUI_EVENT_CANCEL, x, y);
        frame();
    }
    if (cancel == CUI_EVENT_KEY_DOWN) {
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
        frame();
    }
    pointer(CUI_EVENT_POINTER_UP, x, y);
    frame();
}
static void test_cross_group(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    assert(model);
    cui_dock_node left = cui_dock_root(model), right, found;
    assert(cui_dock_insert(model, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, left, 1, 1) == CUI_DOCK_OK);
    assert(cui_dock_split(model, left, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &right) ==
           CUI_DOCK_OK);
    gesture(1, right, 0, CUI_EVENT_CANCEL);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    gesture(1, right, 0, CUI_EVENT_KEY_DOWN);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    panels[1].disabled = true;
    gesture(1, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    panels[1].disabled = false;
    options.allow_reorder = false;
    gesture(1, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    options.allow_reorder = true;
    options.disabled = true;
    gesture(1, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    options.disabled = false;
    gesture(1, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(result.moved && !result.split && active(right) == 1 && moved_events == 1);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found == right);
    gesture(1, left, 1, CUI_EVENT_POINTER_MOVE);
    assert(result.moved && result.split && split_events == 1);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found != left);
    cui_dock_node_info info;
    assert(cui_dock_node_get(model, cui_dock_root(model), &info));
    assert(cui_focused(ui) == cui_dock_tab_id(workspace, 1));
    /* Dropping outside the workspace cancels, with no structural change. */
    frame();
    cui_rect outside_tab = cui_bounds(ui, cui_dock_tab_id(workspace, 1));
    pointer(CUI_EVENT_POINTER_DOWN, outside_tab.x + 8, outside_tab.y + outside_tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, 900, 900);
    frame();
    pointer(CUI_EVENT_POINTER_UP, 900, 900);
    frame();
    assert(!result.moved && cui_dock_panel_find(model, 1, &found, NULL) && found != left);
    /* All four edge orientations, including splitting within the same group. */
    for (unsigned edge = 2; edge <= 4; ++edge) {
        assert(cui_dock_move(model, 1, left, SIZE_MAX) == CUI_DOCK_OK);
        gesture(1, left, edge, CUI_EVENT_POINTER_MOVE);
        assert(result.split && cui_dock_panel_find(model, 1, &found, NULL) && found != left);
    }
    /* A full three-node model rejects split creation without changing serialization. */
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){.node_capacity = 3, .panel_capacity = 3});
    left = cui_dock_root(model);
    assert(cui_dock_insert(model, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, left, 1, 1) == CUI_DOCK_OK);
    assert(cui_dock_split(model, left, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &right) ==
           CUI_DOCK_OK);
    char before[4096], after[4096];
    size_t length;
    frame();
    /* Pointer down selects the dragged tab; compare after that intentional selection. */
    assert(cui_dock_activate(model, 1) == CUI_DOCK_OK);
    assert(cui_dock_serialize(model, before, sizeof before, &length) == CUI_DOCK_OK);
    /* Only release can reject; prior frames remain successful. */
    expected_error = CUI_DOCK_OK;
    frame();
    frame();
    cui_rect tab = cui_bounds(ui, cui_dock_tab_id(workspace, 1));
    cui_rect group = cui_bounds(ui, cui_dock_group_id(workspace, right));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, group.x + 2, group.y + group.h / 2);
    frame();
    expected_error = CUI_DOCK_CAPACITY;
    pointer(CUI_EVENT_POINTER_UP, group.x + 2, group.y + group.h / 2);
    frame();
    expected_error = CUI_DOCK_OK;
    assert(!result.moved);
    assert(cui_dock_serialize(model, after, sizeof after, &length) == CUI_DOCK_OK);
    assert(!strcmp(before, after));
}
static void test_policy_preview(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    cui_dock_node left = cui_dock_root(model), right, found;
    assert(cui_dock_insert(model, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, left, 1, 1) == CUI_DOCK_OK);
    assert(cui_dock_split(model, left, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &right) ==
           CUI_DOCK_OK);
    options.allow_operation = policy;
    options.policy_user = &policy_calls;
    options.custom_palette = true;
    options.drag_palette = (cui_dock_drag_palette){
        {31, 37, 41, 255}, {43, 47, 53, 255}, {7, 13, 19, 255}, {59, 61, 67, 255}};
    deny_operation = CUI_DOCK_MOVED;
    frame();
    frame();
    cui_rect tab = cui_bounds(ui, cui_dock_tab_id(workspace, 1));
    cui_rect group = cui_bounds(ui, cui_dock_group_id(workspace, right));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, group.x + group.w / 2, group.y + group.h / 2);
    frame();
    assert(preview_backgrounds && rejected_indicators && !accepted_indicators);
    unsigned calls = policy_calls;
    options.disable_preview = true;
    frame();
    assert(!preview_backgrounds && !rejected_indicators);
    options.disable_preview = false;
    options.reduced_motion = false;
    frame();
    assert(preview_backgrounds && rejected_indicators);
    options.reduced_motion = true;
    pointer(CUI_EVENT_POINTER_UP, group.x + group.w / 2, group.y + group.h / 2);
    frame();
    assert(result.denied && !result.moved && policy_calls > calls);
    assert(cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    deny_operation = CUI_DOCK_SPLIT;
    gesture(1, right, 1, CUI_EVENT_POINTER_MOVE);
    assert(result.denied && !result.split && cui_dock_panel_find(model, 1, &found, NULL) &&
           found == left);
    deny_operation = CUI_DOCK_CLOSED;
    click(cui_dock_close_id(workspace, 1));
    assert(result.denied && !result.closed && cui_dock_panel_find(model, 1, NULL, NULL));
    assert(cui_dock_activate(model, 1) == CUI_DOCK_OK);
    frame();
    cui_focus(ui, cui_dock_tab_id(workspace, 1));
    key(CUI_KEY_W, CUI_MOD_CTRL);
    assert(result.denied && cui_dock_panel_find(model, 1, NULL, NULL));
    deny_operation = CUI_DOCK_REORDERED;
    frame();
    tab = cui_bounds(ui, cui_dock_tab_id(workspace, 1));
    cui_rect first = cui_bounds(ui, cui_dock_tab_id(workspace, 0));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, first.x + 2, first.y + first.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_UP, first.x + 2, first.y + first.h / 2);
    frame();
    uint64_t key_value;
    assert(result.denied && !result.reordered && cui_dock_panel_at(model, left, 0, &key_value) &&
           key_value == 0);
    deny_operation = CUI_DOCK_CLOSED;
    frame();
    tab = cui_bounds(ui, cui_dock_tab_id(workspace, 1));
    group = cui_bounds(ui, cui_dock_group_id(workspace, right));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, group.x + group.w / 2, group.y + group.h / 2);
    frame();
    assert(preview_backgrounds && accepted_indicators && !rejected_indicators);
    pointer(CUI_EVENT_CANCEL, 0, 0);
    frame();
    assert(!preview_backgrounds && !accepted_indicators);
    /* A host disabling the source while dragging cancels preview and release. */
    frame();
    tab = cui_bounds(ui, cui_dock_tab_id(workspace, 1));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, group.x + group.w / 2, group.y + group.h / 2);
    frame();
    assert(preview_backgrounds);
    panels[1].disabled = true;
    frame();
    assert(!preview_backgrounds && !accepted_indicators);
    pointer(CUI_EVENT_POINTER_UP, group.x + group.w / 2, group.y + group.h / 2);
    frame();
    assert(!result.moved && cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    panels[1].disabled = false;
    options.allow_operation = NULL;
    options.policy_user = NULL;
    options.custom_palette = false;
}
static unsigned toolbar_calls[5], toolbar_clicks;
static void toolbar(cui_context *ctx, uint64_t panel, void *user) {
    (void)user;
    ++toolbar_calls[panel];
    cui_theme theme = cui_theme_light();
    if (cui_button(ctx, 950 + panel, &theme, cui_string("Panel tool"), false))
        ++toolbar_clicks;
}
static void test_zoom_toolbar(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    cui_dock_node left = cui_dock_root(model), right;
    assert(cui_dock_insert(model, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(model, left, CUI_DOCK_SPLIT_HORIZONTAL, .3, 1, false, &right) ==
           CUI_DOCK_OK);
    panels[0].toolbar = toolbar;
    panels[1].toolbar = toolbar;
    panels[0].zoomable = panels[1].zoomable = true;
    frame();
    frame();
    assert(toolbar_calls[0] && toolbar_calls[1]);
    click(950);
    assert(toolbar_clicks == 1);
    char before[4096], after[4096];
    size_t length;
    assert(cui_dock_serialize(model, before, sizeof before, &length) == CUI_DOCK_OK);
    options.allow_operation = policy;
    options.policy_user = &policy_calls;
    deny_operation = CUI_DOCK_ZOOMED;
    click(cui_dock_zoom_id(workspace, 0));
    assert(result.denied && !cui_dock_view_zoomed(view, NULL));
    deny_operation = CUI_DOCK_CLOSED;
    click(cui_dock_zoom_id(workspace, 0));
    uint64_t maximized;
    assert(result.zoomed && cui_dock_view_zoomed(view, &maximized) && maximized == 0);
    unsigned other = toolbar_calls[1];
    frame();
    frame();
    assert(toolbar_calls[1] == other);
    cui_rect bounds = cui_bounds(ui, cui_dock_group_id(workspace, left));
    assert(bounds.w == options.width && bounds.h == options.height);
    assert(cui_dock_serialize(model, after, sizeof after, &length) == CUI_DOCK_OK &&
           !strcmp(before, after));
    panels[0].zoomable = false;
    panels[0].disabled = true;
    options.disabled = true;
    deny_operation = CUI_DOCK_RESTORED;
    frame();
    click(cui_dock_zoom_id(workspace, 0));
    assert(result.restored && !cui_dock_view_zoomed(view, NULL));
    panels[0].disabled = false;
    options.disabled = false;
    frame();
    cui_rect old_hidden = cui_bounds(ui, cui_dock_group_id(workspace, left));
    assert(cui_dock_view_set_zoom(view, model, 1, true));
    frame();
    assert(cui_dock_view_zoomed(view, &maximized) && maximized == 1);
    frame();
    cui_rect zoom_tab = cui_bounds(ui, cui_dock_tab_id(workspace, 1));
    pointer(CUI_EVENT_POINTER_DOWN, zoom_tab.x + 8, zoom_tab.y + zoom_tab.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, old_hidden.x + old_hidden.w / 2,
            old_hidden.y + old_hidden.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_UP, old_hidden.x + old_hidden.w / 2, old_hidden.y + old_hidden.h / 2);
    frame();
    cui_dock_node location;
    assert(!result.moved && cui_dock_panel_find(model, 1, &location, NULL) && location == right);

    frame();
    click(cui_dock_zoom_id(workspace, 1));
    assert(result.restored && cui_focused(ui) == cui_dock_tab_id(workspace, 1));
    assert(cui_dock_view_set_zoom(view, model, 1, true));
    frame();
    assert(cui_dock_restore(model, (cui_str){before, strlen(before)}) == CUI_DOCK_OK);
    frame();
    assert(cui_dock_view_zoomed(view, NULL)); /* Stable key survives node generations. */
    assert(cui_dock_remove(model, 1) == CUI_DOCK_OK);
    frame();
    assert(result.restored && !cui_dock_view_zoomed(view, NULL));
    panels[0].toolbar = panels[1].toolbar = NULL;
    panels[1].zoomable = false;
    options.allow_operation = NULL;
    options.policy_user = NULL;
}
static cui_dock_workspace *edges;
static cui_dock_region_content regions[4];
static cui_dock_workspace_options edge_options;
static cui_dock_workspace_result edge_result;
static cui_dock_result edge_expected_error;
static bool edge_modal;
static unsigned edge_preview_accepted, edge_preview_rejected;
static unsigned edge_resize_events;
static void edge_resize(void *user, cui_dock_region region, float size) {
    assert(user == &edge_resize_events && region >= CUI_DOCK_LEFT && region <= CUI_DOCK_BOTTOM &&
           size > 0);
    ++edge_resize_events;
}
static void edge_frame(void) {
    cui_theme theme = cui_theme_dark();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    if (edge_modal) {
        assert(cui_begin_focus_scope(ui, 9000, true));
        cui_begin_portal(ui, 9001, (cui_box){.background = {10, 10, 10, 255}},
                         (cui_rect){0, 0, 640, 480}, 100);
        cui_interact(ui, 9001, CUI_ROLE_BUTTON, cui_string("Modal guard"), false, false, 0);
        cui_end_box(ui);
        cui_end_focus_scope(ui);
    }
    edge_result = cui_dock_workspace_render(ui, 700, &theme, edges, regions, edge_options);
    cui_draw_list commands = cui_end_frame(ui);
    assert(edge_result.error == edge_expected_error && cui_last_error(ui) == CUI_OK &&
           commands.count);
    for (size_t i = 0; i < commands.count; ++i) {
        cui_color ink = commands.commands[i].color;
        if (ink.r == 31 && ink.g == 37 && ink.b == 41)
            ++edge_preview_accepted;
        if (ink.r == 43 && ink.g == 47 && ink.b == 53)
            ++edge_preview_rejected;
        cui_rect b = commands.commands[i].bounds;
        assert(isfinite(b.x) && isfinite(b.y) && isfinite(b.w) && isfinite(b.h));
    }
}
static void test_edge_regions(void) {
    edges = cui_dock_workspace_create();
    assert(edges);
    edge_options.view = cui_dock_view_options_default();
    edge_options.view.width = 640;
    edge_options.view.height = 480;
    edge_options.view.reduced_motion = true;
    edge_options.on_resize = edge_resize;
    edge_options.resize_user = &edge_resize_events;
    for (unsigned i = 0; i < 4; ++i) {
        regions[i] = (cui_dock_region_content){.model = cui_dock_create((cui_dock_config){0}),
                                               .view = cui_dock_view_create(0, 0),
                                               .panels = panels,
                                               .panel_count = 5};
        assert(regions[i].model && regions[i].view);
        assert(cui_dock_insert(regions[i].model, cui_dock_root(regions[i].model), i, 0) ==
               CUI_DOCK_OK);
    }
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_LEFT, true, 120));
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_RIGHT, true, 140));
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_BOTTOM, true, 100));
    edge_frame();
    edge_frame();
    cui_rect bounds[4];
    for (unsigned i = 0; i < 4; ++i)
        bounds[i] = cui_bounds(ui, cui_dock_region_id(700, (cui_dock_region)i));
    assert(edge_result.fit_minimum);
    assert(bounds[1].w == 120 && bounds[2].w == 140 && bounds[3].h == 100);
    assert(bounds[0].w == 380 && bounds[0].h == 380);
    assert(bounds[1].x < bounds[0].x && bounds[2].x > bounds[0].x && bounds[3].y > bounds[0].y);
    cui_id handle = cui_dock_edge_resize_id(700, CUI_DOCK_LEFT);
    cui_focus(ui, handle);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    edge_frame();
    cui_dock_workspace_snapshot saved;
    assert(cui_dock_workspace_save(edges, &saved) && saved.size[0] > 120 &&
           edge_resize_events == 1);
    edge_frame();
    handle = cui_dock_edge_resize_id(700, CUI_DOCK_BOTTOM);
    cui_rect band = cui_bounds(ui, handle);
    pointer(CUI_EVENT_POINTER_DOWN, band.x + band.w / 2, band.y + band.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, band.x + band.w / 2, band.y + band.h / 2 - 30);
    edge_frame();
    pointer(CUI_EVENT_POINTER_UP, band.x + band.w / 2, band.y + band.h / 2 - 30);
    edge_frame();
    assert(cui_dock_workspace_save(edges, &saved) && saved.size[2] > 120 &&
           edge_resize_events >= 2);
    unsigned left_callbacks = rendered[1];
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_LEFT, false, saved.size[0]));
    edge_frame();
    assert(rendered[1] == left_callbacks);
    assert(cui_dock_workspace_restore(edges, &saved));
    edge_frame();
    assert(rendered[1] > left_callbacks);
    cui_dock_workspace_snapshot bad = saved, unchanged;
    bad.size[1] = NAN;
    assert(!cui_dock_workspace_restore(edges, &bad));
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    assert(cui_dock_set_minimum(regions[1].model, cui_dock_root(regions[1].model), 150, 100) ==
           CUI_DOCK_OK);
    edge_frame();
    assert(cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_LEFT)).w >= 150);
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    assert(cui_dock_set_minimum(regions[1].model, cui_dock_root(regions[1].model), 0, 0) ==
           CUI_DOCK_OK);
    edge_options.view.disabled = true;
    edge_frame();
    handle = cui_dock_edge_resize_id(700, CUI_DOCK_RIGHT);
    cui_focus(ui, handle);
    unsigned count = edge_resize_events;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
    edge_frame();
    assert(edge_resize_events == count);
    edge_options.view.disabled = false;
    edge_options.view.width = 90;
    edge_options.view.height = 60;
    edge_frame();
    assert(!edge_result.fit_minimum);
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    edge_options.view.width = 640;
    edge_options.view.height = 480;
    edge_frame();
    assert(edge_result.fit_minimum);
    /* Frozen closed-bottom strip: no content/actions, real resize opening/snap/cancel. */
    edge_options.collapsible_bottom = true;
    panels[3].disabled = false;
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_BOTTOM, false, 100));
    unsigned bottom_callbacks = rendered[3];
    edge_frame();
    edge_frame();
    assert(cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_BOTTOM)).h == 29);
    assert(rendered[3] == bottom_callbacks);
    cui_id bottom_view = cui_dock_region_id(700, CUI_DOCK_BOTTOM);
    assert(cui_bounds(ui, cui_dock_close_id(bottom_view, 3)).w == 0);
    assert(cui_bounds(ui, cui_dock_menu_id(bottom_view, 3)).w == 0);
    cui_focus(ui, cui_dock_tab_id(bottom_view, 3));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_W, .modifiers = CUI_MOD_CTRL}));
    edge_frame();
    assert(cui_dock_panel_find(regions[3].model, 3, NULL, NULL));
    handle = cui_dock_edge_resize_id(700, CUI_DOCK_BOTTOM);
    band = cui_bounds(ui, handle);
    assert(band.w > 0 && band.h > 0);
    pointer(CUI_EVENT_POINTER_DOWN, band.x + 20, band.y + band.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, band.x + 20, 380);
    edge_frame();
    pointer(CUI_EVENT_POINTER_UP, band.x + 20, 380);
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &saved) && saved.open[2] && saved.size[2] >= 100);
    assert(rendered[3] > bottom_callbacks);
    /* Drag nearer the closed strip snaps shut without erasing preferred size. */
    band = cui_bounds(ui, handle);
    pointer(CUI_EVENT_POINTER_DOWN, band.x + 20, band.y + band.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, band.x + 20, 449);
    edge_frame();
    pointer(CUI_EVENT_POINTER_UP, band.x + 20, 449);
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &saved) && !saved.open[2] && saved.size[2] >= 100);
    assert(cui_bounds(ui, bottom_view).h == 29);
    /* Cancel after live reopening restores the complete edge snapshot. */
    band = cui_bounds(ui, handle);
    pointer(CUI_EVENT_POINTER_DOWN, band.x + 20, band.y + band.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, band.x + 20, 350);
    edge_frame();
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    edge_options.view.disabled = true;
    band = cui_bounds(ui, handle);
    pointer(CUI_EVENT_POINTER_DOWN, band.x + 20, band.y + band.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, band.x + 20, 350);
    edge_frame();
    pointer(CUI_EVENT_POINTER_UP, band.x + 20, 350);
    edge_frame();
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    edge_options.view.disabled = false;
    edge_frame();
    /* A modal arriving during a live resize restores the pre-drag snapshot. */
    band = cui_bounds(ui, handle);
    pointer(CUI_EVENT_POINTER_DOWN, band.x + 20, band.y + band.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, band.x + 20, 350);
    edge_frame();
    edge_modal = true;
    edge_frame();
    edge_modal = false;
    edge_frame();
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    /* Clicking even the previously active tab restores the retained preferred size. */
    cui_rect tab = cui_bounds(ui, cui_dock_tab_id(bottom_view, 3));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + tab.w / 2, tab.y + tab.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_UP, tab.x + tab.w / 2, tab.y + tab.h / 2);
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &unchanged) && unchanged.open[2]);
    assert(cui_bounds(ui, bottom_view).h >= 100);
    /* Keyboard resize snaps closed, then keyboard activation restores the strip. */
    cui_focus(ui, handle);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &unchanged) && !unchanged.open[2]);
    cui_focus(ui, cui_dock_tab_id(bottom_view, 3));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &unchanged) && unchanged.open[2]);
    /* Per-edge source collapsibility survives snapshots and rejects forbidden close. */
    for (unsigned i = CUI_DOCK_LEFT; i <= CUI_DOCK_BOTTOM; ++i) {
        assert(cui_dock_workspace_set_edge(edges, (cui_dock_region)i, true, 120));
        assert(cui_dock_workspace_set_collapsible(edges, (cui_dock_region)i, false));
        assert(cui_dock_workspace_save(edges, &saved) && saved.open[i - 1] &&
               saved.noncollapsible[i - 1] && saved.size[i - 1] == 120);
        assert(!cui_dock_workspace_set_edge(edges, (cui_dock_region)i, false, 80));
        assert(cui_dock_workspace_save(edges, &unchanged) &&
               !memcmp(&saved, &unchanged, sizeof saved));
    }
    edge_frame();
    edge_frame();
    cui_focus(ui, handle);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &saved) && saved.open[2] &&
           cui_bounds(ui, bottom_view).h >= 48);
    cui_dock_workspace_snapshot invalid_fixed = saved;
    invalid_fixed.size[1] = NAN;
    assert(!cui_dock_workspace_restore(edges, &invalid_fixed));
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    assert(cui_dock_workspace_restore(edges, &saved));
    assert(cui_dock_workspace_save(edges, &unchanged) && !memcmp(&saved, &unchanged, sizeof saved));
    /* Changing collapsibility alone preserves an already closed dock. */
    cui_dock_workspace_snapshot closed_fixed = saved;
    closed_fixed.open[2] = false;
    assert(cui_dock_workspace_restore(edges, &closed_fixed));
    assert(cui_dock_workspace_set_collapsible(edges, CUI_DOCK_BOTTOM, false));
    edge_frame();
    edge_frame();
    assert(cui_bounds(ui, bottom_view).h == 29);
    assert(cui_dock_workspace_save(edges, &unchanged) && !unchanged.open[2] &&
           unchanged.noncollapsible[2]);
    cui_focus(ui, cui_dock_tab_id(bottom_view, 3));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    edge_frame();
    edge_frame();
    assert(cui_dock_workspace_save(edges, &unchanged) && unchanged.open[2]);
    assert(!cui_dock_workspace_set_collapsible(edges, CUI_DOCK_CENTER, false));
    for (unsigned i = CUI_DOCK_LEFT; i <= CUI_DOCK_BOTTOM; ++i) {
        assert(cui_dock_workspace_set_collapsible(edges, (cui_dock_region)i, true));
        assert(cui_dock_workspace_set_edge(edges, (cui_dock_region)i, false, 120));
    }
    panels[3].disabled = true;
    edge_options.collapsible_bottom = false;
    for (unsigned i = 0; i < 4; ++i) {
        cui_dock_view_destroy(regions[i].view);
        cui_dock_destroy(regions[i].model);
    }
    cui_dock_workspace_destroy(edges);
}
static unsigned region_rendered[4][5], transfer_events;
static bool transfer_denied;
static void transfer_content(cui_context *ctx, uint64_t key, void *user) {
    unsigned region = (unsigned)(uintptr_t)user;
    assert(region < 4 && key < 5);
    ++region_rendered[region][key];
    cui_text(ctx, cui_string("Owned region panel"), 16, (cui_color){200, 200, 200, 255});
}
static bool transfer_policy(void *user, const cui_dock_transfer_request *request) {
    assert(user == &transfer_events && request->source_model != request->destination_model);
    assert(cui_dock_panel_find(request->source_model, request->panel, NULL, NULL));
    return !transfer_denied;
}
static void transferred(void *user, const cui_dock_transfer_request *request) {
    assert(user == &transfer_events &&
           !cui_dock_panel_find(request->source_model, request->panel, NULL, NULL));
    assert(cui_dock_panel_find(request->destination_model, request->panel, NULL, NULL));
    ++transfer_events;
}
static void transfer_gesture(unsigned source, uint64_t key, float x, float y,
                             cui_event_type cancel) {
    edge_frame();
    edge_frame();
    cui_rect tab =
        cui_bounds(ui, cui_dock_tab_id(cui_dock_region_id(700, (cui_dock_region)source), key));
    assert(tab.w > 0);
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, x, y);
    edge_frame();
    if (cancel == CUI_EVENT_CANCEL) {
        pointer(CUI_EVENT_CANCEL, x, y);
        edge_frame();
    }
    if (cancel == CUI_EVENT_KEY_DOWN) {
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
        edge_frame();
    }
    pointer(CUI_EVENT_POINTER_UP, x, y);
    edge_frame();
}
static void region_serialize(unsigned region, char *out) {
    size_t size;
    assert(cui_dock_serialize(regions[region].model, out, 4096, &size) == CUI_DOCK_OK);
}
static bool transfer_source_veto(void *user, const cui_dock_request *request) {
    assert(user == &transfer_events && request->source && request->panel == 4);
    return request->operation != CUI_DOCK_MOVED;
}
static void rejected_transfer(unsigned source, unsigned destination, uint64_t key, float x, float y,
                              cui_dock_result error) {
    /* Compare from the original active key, including an inactive-tab drag. */
    edge_frame();
    edge_frame();
    char before_source[4096], before_destination[4096], after[4096];
    region_serialize(source, before_source);
    region_serialize(destination, before_destination);
    cui_dock_workspace_snapshot before, after_snapshot;
    assert(cui_dock_workspace_save(edges, &before));
    cui_rect tab =
        cui_bounds(ui, cui_dock_tab_id(cui_dock_region_id(700, (cui_dock_region)source), key));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, x, y);
    edge_frame();
    edge_expected_error = error;
    pointer(CUI_EVENT_POINTER_UP, x, y);
    edge_frame();
    edge_expected_error = CUI_DOCK_OK;
    assert(!edge_result.transferred && !edge_result.opened);
    region_serialize(source, after);
    assert(!strcmp(before_source, after));
    region_serialize(destination, after);
    assert(!strcmp(before_destination, after));
    assert(cui_dock_workspace_save(edges, &after_snapshot) &&
           !memcmp(&before, &after_snapshot, sizeof before));
}
static void test_workspace_transfers(void) {
    cui_dock_panel metadata[4][5];
    edges = cui_dock_workspace_create();
    assert(edges);
    edge_options = (cui_dock_workspace_options){.view = cui_dock_view_options_default(),
                                                .allow_transfer = transfer_policy,
                                                .on_transfer = transferred,
                                                .transfer_user = &transfer_events};
    for (unsigned r = 0; r < 4; ++r) {
        for (unsigned p = 0; p < 5; ++p)
            metadata[r][p] = (cui_dock_panel){.key = p,
                                              .title = cui_string("Owned panel"),
                                              .render = transfer_content,
                                              .user = (void *)(uintptr_t)r};
        regions[r] = (cui_dock_region_content){.model = cui_dock_create((cui_dock_config){0}),
                                               .view = cui_dock_view_create(0, 0),
                                               .panels = metadata[r],
                                               .panel_count = 5};
        assert(regions[r].model && regions[r].view);
        assert(cui_dock_insert(regions[r].model, cui_dock_root(regions[r].model), r, 0) ==
               CUI_DOCK_OK);
    }
    assert(cui_dock_insert(regions[0].model, cui_dock_root(regions[0].model), 4, 1) == CUI_DOCK_OK);
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_LEFT, true, 160));
    edge_frame();
    edge_frame();
    cui_rect left = cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_LEFT));
    transfer_gesture(0, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_EVENT_CANCEL);
    assert(!edge_result.transferred && cui_dock_panel_find(regions[0].model, 4, NULL, NULL));
    cui_dock_node_info cancelled_info;
    assert(cui_dock_node_get(regions[0].model, cui_dock_root(regions[0].model), &cancelled_info) &&
           cancelled_info.active_panel == 0);
    transfer_gesture(0, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_EVENT_KEY_DOWN);
    assert(!edge_result.transferred && cui_dock_panel_find(regions[0].model, 4, NULL, NULL));
    edge_modal = true;
    transfer_gesture(0, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_EVENT_POINTER_MOVE);
    assert(!edge_result.transferred && cui_dock_panel_find(regions[0].model, 4, NULL, NULL));
    edge_modal = false;
    edge_frame();
    edge_frame();
    edge_options.view.custom_palette = true;
    edge_options.view.drag_palette = (cui_dock_drag_palette){
        {31, 37, 41, 255}, {43, 47, 53, 255}, {7, 13, 19, 255}, {59, 61, 67, 255}};
    assert(cui_dock_activate(regions[0].model, 0) == CUI_DOCK_OK);
    char denied_before[4096], denied_after[4096];
    region_serialize(0, denied_before);
    transfer_denied = true;
    transfer_gesture(0, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_EVENT_POINTER_MOVE);
    assert(edge_result.denied && !edge_result.transferred && transfer_events == 0);
    assert(edge_preview_rejected > 0);
    region_serialize(0, denied_after);
    assert(!strcmp(denied_before, denied_after));
    transfer_denied = false;
    edge_options.disable_cross_region = true;
    transfer_gesture(0, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_EVENT_POINTER_MOVE);
    assert(!edge_result.transferred && cui_dock_panel_find(regions[0].model, 4, NULL, NULL));
    edge_options.disable_cross_region = false;
    unsigned source_callbacks = region_rendered[0][4],
             destination_callbacks = region_rendered[1][4];
    transfer_gesture(0, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_EVENT_POINTER_MOVE);
    assert(edge_result.transferred && !edge_result.opened && transfer_events == 1);
    assert(edge_preview_accepted > 0);
    edge_options.view.custom_palette = false;
    unsigned source_after = region_rendered[0][4];
    edge_frame();
    assert(region_rendered[0][4] == source_after && source_after >= source_callbacks &&
           region_rendered[1][4] > destination_callbacks);
    assert(cui_focused(ui) == cui_dock_tab_id(cui_dock_region_id(700, CUI_DOCK_LEFT), 4));
    /* Dropping on a closed right-edge landing zone opens it only after commit. */
    transfer_gesture(1, 4, 638, 240, CUI_EVENT_CANCEL);
    cui_dock_workspace_snapshot saved;
    assert(cui_dock_workspace_save(edges, &saved) && !saved.open[1]);
    transfer_gesture(1, 4, 638, 240, CUI_EVENT_POINTER_MOVE);
    assert(edge_result.transferred && edge_result.opened && transfer_events == 2);
    assert(cui_dock_workspace_save(edges, &saved) && saved.open[1]);
    edge_frame();
    assert(cui_focused(ui) == cui_dock_tab_id(cui_dock_region_id(700, CUI_DOCK_RIGHT), 4));
    /* Closed bottom opens into its independent registry. */
    transfer_gesture(2, 4, 320, 478, CUI_EVENT_POINTER_MOVE);
    assert(edge_result.opened && edge_result.destination_region == CUI_DOCK_BOTTOM);
    edge_frame();
    assert(region_rendered[3][4] > 0);
    /* Duplicate-key rejection preserves both serialized layouts. */
    assert(cui_dock_insert(regions[0].model, cui_dock_root(regions[0].model), 4, SIZE_MAX) ==
           CUI_DOCK_OK);
    assert(cui_dock_activate(regions[3].model, 4) == CUI_DOCK_OK);
    edge_frame();
    edge_frame();
    char before_source[4096], before_target[4096], after[4096];
    region_serialize(3, before_source);
    region_serialize(0, before_target);
    cui_rect center = cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_CENTER));
    /* Queue release separately to assert the expected rejection only there. */
    cui_rect tab = cui_bounds(ui, cui_dock_tab_id(cui_dock_region_id(700, CUI_DOCK_BOTTOM), 4));
    pointer(CUI_EVENT_POINTER_DOWN, tab.x + 8, tab.y + tab.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, center.x + center.w / 2, center.y + center.h / 2);
    edge_frame();
    edge_expected_error = CUI_DOCK_DUPLICATE_PANEL;
    pointer(CUI_EVENT_POINTER_UP, center.x + center.w / 2, center.y + center.h / 2);
    edge_frame();
    edge_expected_error = CUI_DOCK_OK;
    assert(!edge_result.transferred);
    region_serialize(3, after);
    assert(!strcmp(before_source, after));
    region_serialize(0, after);
    assert(!strcmp(before_target, after));
    assert(cui_dock_remove(regions[0].model, 4) == CUI_DOCK_OK);
    edge_frame();
    edge_frame();
    /* Successful cross-region edge split creates a new destination leaf. */
    left = cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_LEFT));
    transfer_gesture(3, 4, left.x + 2, left.y + left.h / 2, CUI_EVENT_POINTER_MOVE);
    assert(edge_result.transferred && edge_result.destination_region == CUI_DOCK_LEFT);
    cui_dock_node_info split_info;
    assert(cui_dock_node_get(regions[1].model, cui_dock_root(regions[1].model), &split_info) &&
           split_info.kind == CUI_DOCK_SPLIT_HORIZONTAL);
    edge_frame();
    cui_rect bottom = cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_BOTTOM));
    transfer_gesture(1, 4, bottom.x + bottom.w / 2, bottom.y + bottom.h / 2,
                     CUI_EVENT_POINTER_MOVE);
    assert(edge_result.transferred);
    edge_frame();
    left = cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_LEFT));
    metadata[1][4].disabled = true;
    rejected_transfer(3, 1, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_DOCK_INVALID);
    metadata[1][4].disabled = false;
    regions[1].panel_count = 4;
    rejected_transfer(3, 1, 4, left.x + left.w / 2, left.y + left.h / 2, CUI_DOCK_NOT_FOUND);
    regions[1].panel_count = 5;
    /* Full destination capacity cannot open a closed edge. */
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_RIGHT, false, 200));
    assert(cui_dock_activate(regions[3].model, 3) == CUI_DOCK_OK);
    edge_options.view.allow_operation = transfer_source_veto;
    edge_options.view.policy_user = &transfer_events;
    rejected_transfer(3, 2, 4, 638, 240, CUI_DOCK_OK);
    assert(edge_result.denied);
    edge_options.view.allow_operation = NULL;
    edge_options.view.policy_user = NULL;
    transfer_denied = true;
    rejected_transfer(3, 2, 4, 638, 240, CUI_DOCK_OK);
    assert(edge_result.denied);
    transfer_denied = false;
    cui_dock_destroy(regions[2].model);
    regions[2].model = cui_dock_create((cui_dock_config){.panel_capacity = 1});
    assert(regions[2].model);
    cui_dock_view_reset(regions[2].view);
    assert(cui_dock_insert(regions[2].model, cui_dock_root(regions[2].model), 2, 0) == CUI_DOCK_OK);
    rejected_transfer(3, 2, 4, 638, 240, CUI_DOCK_CAPACITY);
    /* Split-capacity failure also preserves both layouts. */
    cui_dock_destroy(regions[0].model);
    regions[0].model = cui_dock_create((cui_dock_config){.node_capacity = 1, .panel_capacity = 3});
    assert(regions[0].model);
    cui_dock_view_reset(regions[0].view);
    assert(cui_dock_insert(regions[0].model, cui_dock_root(regions[0].model), 0, 0) == CUI_DOCK_OK);
    edge_frame();
    edge_frame();
    center = cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_CENTER));
    rejected_transfer(3, 0, 4, center.x + 2, center.y + center.h / 2, CUI_DOCK_CAPACITY);
    /* Opening an empty closed model transfers callback ownership normally. */
    assert(cui_dock_remove(regions[1].model, 1) == CUI_DOCK_OK);
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_LEFT, false, 160));
    transfer_gesture(3, 4, 2, 240, CUI_EVENT_POINTER_MOVE);
    assert(edge_result.transferred && edge_result.opened &&
           edge_result.destination_region == CUI_DOCK_LEFT);
    edge_frame();
    assert(cui_focused(ui) == cui_dock_tab_id(cui_dock_region_id(700, CUI_DOCK_LEFT), 4));
    for (unsigned r = 0; r < 4; ++r) {
        cui_dock_view_destroy(regions[r].view);
        cui_dock_destroy(regions[r].model);
    }
    cui_dock_workspace_destroy(edges);
}
static bool oversized_hit[2];
static void oversized_content(cui_context *ctx, uint64_t key, void *user) {
    (void)user;
    cui_id id = 980 + key;
    cui_begin_box(ctx, id, (cui_box){.width = -1, .height = -1, .padding = 12, .gap = 8});
    cui_interact(ctx, id, CUI_ROLE_GROUP, cui_string("Oversized content"), false, false, 0);
    cui_set_semantic_details(ctx, id, (cui_semantic_details){.pointer_activatable = true});
    size_t event_count;
    cui_frame_events(ctx, &event_count);
    if (event_count)
        oversized_hit[key] = cui_hovered(ctx, id);
    cui_text(ctx,
             cui_string("A deliberately long single line of unwrapped content should not enlarge "
                        "the adjacent dock pane"),
             16, (cui_color){20, 30, 40, 255});
    cui_end_box(ctx);
}
static void test_oversized_content(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    cui_dock_node left = cui_dock_root(model), right;
    assert(cui_dock_insert(model, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(model, left, CUI_DOCK_SPLIT_HORIZONTAL, .5, 1, false, &right) ==
           CUI_DOCK_OK);
    panels[0].render = panels[1].render = oversized_content;
    frame();
    frame();
    cui_rect a = cui_bounds(ui, cui_dock_group_id(workspace, left)),
             b = cui_bounds(ui, cui_dock_group_id(workspace, right));
    cui_rect content_a = cui_bounds(ui, 980), content_b = cui_bounds(ui, 981);
    assert(content_a.w > a.w && content_b.w > b.w);
    assert(a.w == 320 && b.w == 320 && a.x + a.w <= b.x + .001f);
    pointer(CUI_EVENT_POINTER_DOWN, b.x + 8, b.y + 80);
    frame();
    assert(!oversized_hit[0] && oversized_hit[1]);
    pointer(CUI_EVENT_POINTER_UP, b.x + 8, b.y + 80);
    frame();
    panels[0].render = panels[1].render = content;
}
static unsigned custom_menu_calls;
static void custom_menu_action(void *user, uint64_t panel, cui_id action) {
    assert(user == &custom_menu_calls && panel == 0 && action == 970);
    ++custom_menu_calls;
}
static void menu_click(uint64_t panel, cui_dock_menu_action action) {
    frame();
    click(cui_dock_menu_id(workspace, panel));
    frame();
    click(cui_dock_menu_action_id(workspace, panel, action));
}
static bool close_last_veto(void *user, const cui_dock_request *request) {
    assert(user == &policy_calls && request->operation == CUI_DOCK_CLOSED);
    ++policy_calls;
    return request->panel != UINT64_MAX;
}
static void test_panel_menus(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    cui_dock_node root = cui_dock_root(model);
    for (unsigned i = 0; i < 4; ++i)
        assert(cui_dock_insert(model, root, i, SIZE_MAX) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, root, UINT64_MAX, SIZE_MAX) == CUI_DOCK_OK);
    panels[0].zoomable = true;
    panels[2].closable = false;
    cui_menu_entry custom = {.id = 970,
                             .label = cui_string("Inspect panel"),
                             .icon = cui_string("info"),
                             .kind = CUI_MENU_ACTION};
    panels[0].menu_items = &custom;
    panels[0].menu_count = 1;
    panels[0].menu_action = custom_menu_action;
    panels[0].user = &custom_menu_calls;
    frame();
    cui_rect icon_bounds = cui_bounds(ui, cui_dock_menu_id(workspace, 0));
    bool svg = false;
    for (size_t i = 0; i < last_draw.count; ++i) {
        const cui_draw_command *command = &last_draw.commands[i];
        svg |= command->type == CUI_DRAW_IMAGE && command->bounds.x >= icon_bounds.x &&
               command->bounds.y >= icon_bounds.y &&
               command->bounds.x + command->bounds.w <= icon_bounds.x + icon_bounds.w &&
               command->bounds.y + command->bounds.h <= icon_bounds.y + icon_bounds.h;
    }
    assert(svg);
    frame();
    click(cui_dock_menu_id(workspace, 0));
    frame();
    click(970);
    assert(custom_menu_calls == 1);
    custom.disabled = true;
    frame();
    click(cui_dock_menu_id(workspace, 0));
    frame();
    click(970);
    assert(custom_menu_calls == 1);
    key(CUI_KEY_ESCAPE, 0);
    custom.disabled = false;
    for (unsigned placement = CUI_DOCK_ZOOM_BOTH; placement <= CUI_DOCK_ZOOM_NONE; ++placement) {
        panels[0].zoom_control = (cui_dock_zoom_control)placement;
        frame();
        assert((cui_bounds(ui, cui_dock_zoom_id(workspace, 0)).w > 0) ==
               (placement == CUI_DOCK_ZOOM_BOTH || placement == CUI_DOCK_ZOOM_TOOLBAR));
        click(cui_dock_menu_id(workspace, 0));
        frame();
        assert((cui_bounds(ui, cui_dock_menu_action_id(workspace, 0, CUI_DOCK_MENU_ZOOM)).w > 0) ==
               (placement == CUI_DOCK_ZOOM_BOTH || placement == CUI_DOCK_ZOOM_MENU));
        key(CUI_KEY_ESCAPE, 0);
    }
    panels[0].zoom_control = CUI_DOCK_ZOOM_MENU;
    frame();
    menu_click(0, CUI_DOCK_MENU_ZOOM);
    assert(result.zoomed);
    frame();
    panels[0].zoomable = false;
    panels[0].zoom_control = CUI_DOCK_ZOOM_NONE;
    options.disabled = true;
    frame();
    menu_click(0, CUI_DOCK_MENU_ZOOM);
    assert(result.restored);
    options.disabled = false;
    panels[0].zoom_control = CUI_DOCK_ZOOM_BOTH;
    /* Bulk close veto rejects all eligible removals. Protected/disabled tabs survive. */
    options.allow_operation = close_last_veto;
    options.policy_user = &policy_calls;
    char before[4096], after[4096];
    size_t size;
    assert(cui_dock_serialize(model, before, sizeof before, &size) == CUI_DOCK_OK);
    menu_click(0, CUI_DOCK_MENU_CLOSE_OTHERS);
    assert(result.denied && !result.closed);
    assert(cui_dock_serialize(model, after, sizeof after, &size) == CUI_DOCK_OK &&
           !strcmp(before, after));
    options.allow_operation = NULL;
    menu_click(0, CUI_DOCK_MENU_CLOSE_OTHERS);
    assert(result.closed_count == 2 && !cui_dock_panel_find(model, 1, NULL, NULL) &&
           !cui_dock_panel_find(model, UINT64_MAX, NULL, NULL));
    assert(cui_dock_panel_find(model, 0, NULL, NULL) && cui_dock_panel_find(model, 2, NULL, NULL) &&
           cui_dock_panel_find(model, 3, NULL, NULL));
    assert(cui_dock_insert(model, root, 1, SIZE_MAX) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, root, UINT64_MAX, SIZE_MAX) == CUI_DOCK_OK);
    menu_click(0, CUI_DOCK_MENU_CLOSE_ALL);
    assert(result.closed_count == 3);
    assert(cui_dock_insert(model, root, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, root, 1, SIZE_MAX) == CUI_DOCK_OK);
    assert(cui_dock_activate(model, 0) == CUI_DOCK_OK);
    options.allow_operation = policy;
    options.policy_user = &policy_calls;
    deny_operation = CUI_DOCK_SPLIT;
    assert(cui_dock_serialize(model, before, sizeof before, &size) == CUI_DOCK_OK);
    menu_click(0, CUI_DOCK_MENU_SPLIT_HORIZONTAL);
    assert(result.denied && !result.split);
    assert(cui_dock_serialize(model, after, sizeof after, &size) == CUI_DOCK_OK &&
           !strcmp(before, after));
    options.allow_operation = NULL;
    menu_click(0, CUI_DOCK_MENU_SPLIT_HORIZONTAL);
    assert(result.split);
    frame();
    cui_dock_node split = cui_dock_root(model);
    cui_dock_node_info info;
    assert(cui_dock_node_get(model, split, &info) && info.kind == CUI_DOCK_SPLIT_HORIZONTAL);
    assert(cui_dock_set_ratio(model, split, .3) == CUI_DOCK_OK);
    frame();
    options.allow_operation = policy;
    deny_operation = CUI_DOCK_RESIZED;
    menu_click(0, CUI_DOCK_MENU_BALANCE);
    assert(result.denied && !result.resized);
    assert(cui_dock_node_get(model, split, &info) && info.ratio == .3);
    options.allow_operation = NULL;
    menu_click(0, CUI_DOCK_MENU_BALANCE);
    assert(result.resized && cui_dock_node_get(model, split, &info) && info.ratio == .5);
    menu_click(0, CUI_DOCK_MENU_CLOSE_ALL);
    assert(result.closed_count == 1 && !cui_dock_panel_find(model, 0, NULL, NULL));
    frame();
    assert(cui_dock_panel_find(model, 2, NULL, NULL) && cui_dock_panel_find(model, 3, NULL, NULL));
    panels[0].menu_items = NULL;
    panels[0].menu_count = 0;
    panels[0].menu_action = NULL;
    panels[0].user = NULL;
    panels[2].closable = true;
    /* Disabled workspace cannot open a panel menu. */
    options.disabled = true;
    frame();
    click(cui_dock_menu_id(workspace, 2));
    frame();
    assert(cui_bounds(ui, cui_dock_menu_action_id(workspace, 2, CUI_DOCK_MENU_CLOSE)).w == 0);
    options.disabled = false;
    options.hide_menu = true;
    frame();
    assert(cui_bounds(ui, cui_dock_menu_id(workspace, 2)).w == 0);
    options.hide_menu = false;
}
static unsigned title_calls[2], suffix_calls[2], suffix_actions;
static void custom_title(cui_context *ctx, uint64_t key, void *user) {
    assert(key < 2 && user == &title_calls);
    ++title_calls[key];
    cui_begin_box(ctx, 960 + key, (cui_box){.width = -1, .height = 18});
    cui_text(ctx, cui_string("A badge and a deliberately oversized title"), 14,
             (cui_color){17, 23, 29, 255});
    cui_end_box(ctx);
}
static void custom_suffix(cui_context *ctx, uint64_t key, void *user) {
    assert(key < 2 && user == &title_calls);
    ++suffix_calls[key];
    cui_theme theme = cui_theme_light();
    if (cui_button_ex(ctx, 950 + key, &theme, cui_string("Status"),
                      (cui_button_options){.size = CUI_CONTROL_SMALL,
                                           .icon = cui_string("info"),
                                           .icon_only = true}))
        ++suffix_actions;
}
static void test_title_composition(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    assert(model);
    for (unsigned i = 0; i < 2; ++i) {
        assert(cui_dock_insert(model, cui_dock_root(model), i, SIZE_MAX) == CUI_DOCK_OK);
        panels[i].title_render = custom_title;
        panels[i].title_suffix = custom_suffix;
        panels[i].title_width = 60;
        panels[i].title_suffix_width = 64;
        panels[i].user = &title_calls;
    }
    frame();
    frame();
    assert(title_calls[0] && title_calls[1] && suffix_calls[0] && !suffix_calls[1]);
    bool title_ink = false, title_clip = false;
    for (size_t i = 0; i < last_draw.count; ++i) {
        const cui_draw_command *command = &last_draw.commands[i];
        title_ink |= command->type == CUI_DRAW_TEXT && command->color.r == 17 &&
                     command->color.g == 23 && command->color.b == 29;
        title_clip |= command->type == CUI_DRAW_CLIP_PUSH && command->bounds.w == 60;
    }
    assert(title_ink && title_clip);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    bool label = false;
    for (size_t i = 0; i < count; ++i)
        if (semantics[i].id == cui_dock_tab_id(workspace, 0))
            label |= semantics[i].name.length == panels[0].title.length &&
                     !memcmp(semantics[i].name.data, panels[0].title.data, panels[0].title.length);
    assert(label);
    cui_rect suffix = cui_bounds(ui, 950);
    assert(suffix.x >= 500 && suffix.x + suffix.w <= 640);
    click(950);
    assert(suffix_actions == 1);
    unsigned old_suffix = suffix_calls[0];
    click(cui_dock_tab_id(workspace, 1));
    assert(result.selected && suffix_calls[1] && suffix_calls[0] == old_suffix);
    frame();
    assert(cui_bounds(ui, 950).w == 0 && cui_bounds(ui, 951).w > 0);
    options.width = 160;
    frame();
    frame();
    suffix = cui_bounds(ui, 951);
    assert(suffix.x >= 96 && suffix.x + suffix.w <= 160);
    pointer(CUI_EVENT_POINTER_MOVE, 163, suffix.y + suffix.h / 2);
    frame();
    assert(!cui_hovered(ui, 951));
    options.width = 640;
    char saved[4096];
    size_t saved_size;
    assert(cui_dock_serialize(model, saved, sizeof saved, &saved_size) == CUI_DOCK_OK);
    assert(cui_dock_restore(model, (cui_str){saved, saved_size}) == CUI_DOCK_OK);
    frame();
    frame();
    assert(cui_bounds(ui, 951).w > 0 && cui_bounds(ui, 950).w == 0);
    options.disabled = true;
    frame();
    click(951);
    assert(suffix_actions == 1);
    options.disabled = false;
    options.collapsed = true;
    old_suffix = suffix_calls[0] + suffix_calls[1];
    frame();
    assert(suffix_calls[0] + suffix_calls[1] == old_suffix && cui_bounds(ui, 951).w == 0);
    options.collapsed = false;
    /* Descriptor validation is atomic and precedes any scene/callback submission. */
    panels[0].title_width = NAN;
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    unsigned previous = title_calls[0];
    assert(cui_dock_view_render(ui, workspace, &theme, model, view, panels, 5, options).error ==
           CUI_DOCK_INVALID);
    assert(!cui_end_frame(ui).count && title_calls[0] == previous);
    for (unsigned i = 0; i < 2; ++i) {
        panels[i].title_render = NULL;
        panels[i].title_suffix = NULL;
        panels[i].title_width = panels[i].title_suffix_width = 0;
        panels[i].user = NULL;
    }
    frame();
}
static cui_role semantic_role(cui_id id) {
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (semantics[i].id == id)
            return semantics[i].role;
    return CUI_ROLE_NONE;
}
static void test_title_modes_padding(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    assert(model);
    options.panel_style = CUI_DOCK_PANEL_AUTO;
    options.hide_menu = true;
    panels[0].zoomable = false;
    panels[0].hide_title_bar = true;
    assert(cui_dock_insert(model, cui_dock_root(model), 0, SIZE_MAX) == CUI_DOCK_OK);
    frame();
    frame();
    cui_id title = cui_dock_tab_id(workspace, 0);
    assert(cui_bounds(ui, title).w == 0);
    float content_origin = cui_bounds(ui, 800).y;
    assert(cui_bounds(ui, cui_dock_menu_id(workspace, 0)).w == 0);
    panels[0].hide_title_bar = false;
    frame();
    frame();
    assert(semantic_role(title) == CUI_ROLE_LABEL && cui_bounds(ui, title).h == 30);
    assert(cui_bounds(ui, 800).y == content_origin + 30 &&
           cui_bounds(ui, cui_dock_close_id(workspace, 0)).w > 0);
    panels[0].title_render = custom_title;
    panels[0].title_width = 60;
    panels[0].title_suffix = custom_suffix;
    panels[0].title_suffix_width = 64;
    panels[0].user = &title_calls;
    unsigned title_before = title_calls[0], suffix_before = suffix_calls[0];
    frame();
    assert(title_calls[0] > title_before && suffix_calls[0] > suffix_before);
    assert(cui_bounds(ui, 950).h <= 30);
    panels[0].title_render = panels[0].title_suffix = NULL;
    panels[0].title_width = panels[0].title_suffix_width = 0;
    panels[0].user = NULL;
    assert(cui_dock_insert(model, cui_dock_root(model), 1, SIZE_MAX) == CUI_DOCK_OK);
    panels[0].hide_title_bar = true;
    frame();
    frame();
    assert(semantic_role(title) == CUI_ROLE_TAB &&
           cui_bounds(ui, 800).y == content_origin + options.tab_height);
    options.inner_padding = true;
    frame();
    frame();
    assert(cui_bounds(ui, 800).y == content_origin + options.tab_height + 8);
    panels[0].omit_inner_padding = true;
    frame();
    frame();
    assert(cui_bounds(ui, 800).y == content_origin + options.tab_height);
    click(cui_dock_tab_id(workspace, 1));
    frame();
    assert(cui_bounds(ui, 801).y == content_origin + options.tab_height + 8);
    assert(cui_dock_remove(model, 1) == CUI_DOCK_OK);
    panels[0].hide_title_bar = false;
    frame();
    frame();
    assert(cui_bounds(ui, 800).y ==
           content_origin + 30); /* Source padding applies only to multiple panels. */
    /* A collapsed group remains a reachable strip even for self-chromed Auto panels. */
    panels[0].hide_title_bar = true;
    options.collapsed = true;
    unsigned previous = rendered[0];
    frame();
    assert(semantic_role(title) == CUI_ROLE_TAB && rendered[0] == previous);
    options.collapsed = false;
    /* Programmatic zoom cannot strand a panel that hides its title bar. */
    panels[0].zoomable = true;
    assert(cui_dock_view_set_zoom(view, model, 0, true));
    frame();
    assert(cui_bounds(ui, cui_dock_zoom_id(workspace, 0)).w > 0);
    click(cui_dock_zoom_id(workspace, 0));
    assert(result.restored);
    frame();
    assert(cui_bounds(ui, title).w == 0 && cui_bounds(ui, 800).y == content_origin);
    /* Always-tabs overrides the Auto-only title-bar opt-out. */
    panels[0].zoomable = false;
    options.panel_style = CUI_DOCK_PANEL_TAB_BAR;
    frame();
    assert(semantic_role(title) == CUI_ROLE_TAB);
    options.panel_style = (cui_dock_panel_style)99;
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_dock_view_render(ui, workspace, &theme, model, view, panels, 5, options).error ==
           CUI_DOCK_INVALID);
    assert(!cui_end_frame(ui).count);
    options.panel_style = CUI_DOCK_PANEL_AUTO;
    panels[0].hide_title_bar = false;
    frame();
    frame();
    options.allow_operation = policy;
    options.policy_user = &policy_calls;
    deny_operation = CUI_DOCK_CLOSED;
    cui_focus(ui, title);
    key(CUI_KEY_W, CUI_MOD_CTRL);
    assert(result.denied && cui_dock_panel_find(model, 0, NULL, NULL));
    options.allow_operation = NULL;
    options.policy_user = NULL;
    click(cui_dock_close_id(workspace, 0));
    assert(result.closed && !cui_dock_panel_find(model, 0, NULL, NULL));
    frame();
    /* Simple titles remain actual cross-group drag sources, not decorative labels. */
    assert(cui_dock_insert(model, cui_dock_root(model), 0, 0) == CUI_DOCK_OK);
    cui_dock_node right, found;
    assert(cui_dock_split(model, cui_dock_root(model), CUI_DOCK_SPLIT_HORIZONTAL, .5, 1, false,
                          &right) == CUI_DOCK_OK);
    gesture(0, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(result.moved && cui_dock_panel_find(model, 0, &found, NULL) && found == right);
    frame();
    assert(semantic_role(title) == CUI_ROLE_TAB); /* Merged group returns to real tabs. */
    options.panel_style = CUI_DOCK_PANEL_TAB_BAR;
    options.inner_padding = false;
    options.hide_menu = false;
    panels[0].omit_inner_padding = false;
}
static cui_dock_node constrained_node;
static const cui_dock *constrained_model;
static cui_dock_group_constraints constraint_flags;
static unsigned constraint_calls;
static cui_dock_group_constraints group_constraints(void *user, const cui_dock *m,
                                                    cui_dock_node node) {
    assert(user == &constraint_calls);
    ++constraint_calls;
    return m == constrained_model && node == constrained_node ? constraint_flags
                                                              : (cui_dock_group_constraints){0};
}
static void test_group_constraints(void) {
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    assert(model);
    cui_dock_node left = cui_dock_root(model), right, found;
    assert(cui_dock_insert(model, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, left, 1, 1) == CUI_DOCK_OK);
    assert(cui_dock_split(model, left, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &right) ==
           CUI_DOCK_OK);
    options.group_constraints = group_constraints;
    options.constraints_user = &constraint_calls;
    constrained_model = model;
    constrained_node = left;
    constraint_flags = cui_dock_group_sealed();
    frame();
    frame();
    assert(cui_bounds(ui, cui_dock_close_id(workspace, 0)).w == 0);
    assert(cui_bounds(ui, cui_dock_close_id(workspace, 2)).w > 0);
    unsigned content_before = rendered[1];
    click(cui_dock_tab_id(workspace, 1));
    assert(result.selected && active(left) == 1 && rendered[1] > content_before);
    char before[4096], after[4096];
    size_t size;
    assert(cui_dock_serialize(model, before, sizeof before, &size) == CUI_DOCK_OK);
    gesture(1, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(!result.moved && cui_dock_serialize(model, after, sizeof after, &size) == CUI_DOCK_OK &&
           !strcmp(before, after));
    cui_focus(ui, cui_dock_tab_id(workspace, 1));
    key(CUI_KEY_W, CUI_MOD_CTRL);
    assert(!result.closed && cui_dock_panel_find(model, 1, NULL, NULL));
    menu_click(1, CUI_DOCK_MENU_CLOSE_ALL);
    assert(!result.closed);
    /* A locked target refuses incoming moves while the source stays selectable. */
    constrained_node = right;
    frame();
    gesture(1, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(!result.moved && cui_dock_panel_find(model, 1, &found, NULL) && found == left);
    assert(cui_dock_serialize(model, after, sizeof after, &size) == CUI_DOCK_OK &&
           !strcmp(before, after));
    /* Unclosable alone does not prevent moving; clearing constraints reenables behavior. */
    constrained_node = left;
    constraint_flags = (cui_dock_group_constraints){.unclosable = true};
    frame();
    gesture(1, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(result.moved && cui_dock_panel_find(model, 1, &found, NULL) && found == right);
    constraint_flags = cui_dock_group_in_split(true);
    frame();
    assert(cui_bounds(ui, cui_dock_close_id(workspace, 0)).w == 0);
    gesture(0, right, 0, CUI_EVENT_POINTER_MOVE);
    assert(!result.moved && cui_dock_panel_find(model, 0, &found, NULL) && found == left);
    /* Collapsing one group suppresses only its content/actions; siblings still render. */
    constraint_flags = (cui_dock_group_constraints){.collapsed = true};
    unsigned left_before = rendered[0], right_before = rendered[1];
    frame();
    assert(rendered[0] == left_before && rendered[1] > right_before);
    assert(cui_bounds(ui, cui_dock_tab_id(workspace, 0)).w > 0);
    assert(cui_bounds(ui, cui_dock_menu_id(workspace, 0)).w == 0);
    constraint_flags = (cui_dock_group_constraints){0};
    frame();
    click(cui_dock_close_id(workspace, 0));
    assert(result.closed);
    frame();
    /* A preserve-last bulk close retains the active key, not an empty dock. */
    constrained_node = cui_dock_root(model);
    constraint_flags = cui_dock_group_in_split(true);
    assert(cui_dock_activate(model, 1) == CUI_DOCK_OK);
    frame();
    menu_click(1, CUI_DOCK_MENU_CLOSE_ALL);
    assert(result.closed_count == 1 && cui_dock_panel_find(model, 1, NULL, NULL) &&
           !cui_dock_panel_find(model, 2, NULL, NULL));
    frame();
    assert(cui_bounds(ui, cui_dock_close_id(workspace, 1)).w == 0);
    assert(constraint_calls);
    options.group_constraints = NULL;
    options.constraints_user = NULL;
    frame();
    assert(cui_bounds(ui, cui_dock_close_id(workspace, 1)).w > 0);
}
static void test_workspace_constraints(void) {
    edges = cui_dock_workspace_create();
    assert(edges);
    edge_options = (cui_dock_workspace_options){.view = cui_dock_view_options_default()};
    edge_options.view.width = 640;
    edge_options.view.height = 480;
    edge_options.view.group_constraints = group_constraints;
    edge_options.view.constraints_user = &constraint_calls;
    for (unsigned r = 0; r < 4; ++r) {
        regions[r] = (cui_dock_region_content){.model = cui_dock_create((cui_dock_config){0}),
                                               .view = cui_dock_view_create(0, 0),
                                               .panels = panels,
                                               .panel_count = 5};
        assert(regions[r].model && regions[r].view);
    }
    assert(cui_dock_insert(regions[0].model, cui_dock_root(regions[0].model), 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(regions[1].model, cui_dock_root(regions[1].model), 2, 0) == CUI_DOCK_OK);
    assert(cui_dock_workspace_set_edge(edges, CUI_DOCK_LEFT, true, 160));
    edge_frame();
    edge_frame();
    constrained_model = regions[1].model;
    constrained_node = cui_dock_root(constrained_model);
    constraint_flags = cui_dock_group_sealed();
    cui_rect left = cui_bounds(ui, cui_dock_region_id(700, CUI_DOCK_LEFT));
    char a[4096], b[4096], after[4096];
    region_serialize(0, a);
    region_serialize(1, b);
    transfer_gesture(0, 0, left.x + left.w / 2, left.y + left.h / 2, CUI_EVENT_POINTER_MOVE);
    assert(!edge_result.transferred);
    region_serialize(0, after);
    assert(!strcmp(a, after));
    region_serialize(1, after);
    assert(!strcmp(b, after));
    /* Locking the source also prevents opening an otherwise valid closed edge. */
    constrained_model = regions[0].model;
    constrained_node = cui_dock_root(constrained_model);
    transfer_gesture(0, 0, 639, 240, CUI_EVENT_POINTER_MOVE);
    cui_dock_workspace_snapshot saved;
    assert(cui_dock_workspace_save(edges, &saved) && !saved.open[1] && !edge_result.transferred);
    /* A locked closed destination stays closed; removing the lock permits atomic transfer. */
    constrained_model = regions[2].model;
    constrained_node = cui_dock_root(constrained_model);
    transfer_gesture(0, 0, 639, 240, CUI_EVENT_POINTER_MOVE);
    assert(cui_dock_workspace_save(edges, &saved) && !saved.open[1] && !edge_result.transferred);
    region_serialize(0, after);
    assert(!strcmp(a, after));
    constraint_flags = (cui_dock_group_constraints){0};
    edge_frame();
    edge_frame();
    cui_rect source_tab =
        cui_bounds(ui, cui_dock_tab_id(cui_dock_region_id(700, CUI_DOCK_CENTER), 0));
    pointer(CUI_EVENT_POINTER_DOWN, source_tab.x + 8, source_tab.y + source_tab.h / 2);
    edge_frame();
    pointer(CUI_EVENT_POINTER_MOVE, 639, 240);
    edge_frame();
    constraint_flags = cui_dock_group_sealed(); /* Target revokes permission after preview. */
    pointer(CUI_EVENT_POINTER_UP, 639, 240);
    edge_frame();
    assert(edge_result.denied && !edge_result.transferred);
    assert(cui_dock_workspace_save(edges, &saved) && !saved.open[1]);
    region_serialize(0, after);
    assert(!strcmp(a, after));
    constraint_flags = (cui_dock_group_constraints){0};
    transfer_gesture(0, 0, 639, 240, CUI_EVENT_POINTER_MOVE);
    assert(edge_result.transferred && edge_result.opened &&
           cui_dock_panel_find(regions[2].model, 0, NULL, NULL));
    for (unsigned r = 0; r < 4; ++r) {
        cui_dock_view_destroy(regions[r].view);
        cui_dock_destroy(regions[r].model);
    }
    cui_dock_workspace_destroy(edges);
    constrained_model = NULL;
}
int main(void) {
    ui = cui_create((cui_config){.width = 640, .height = 480});
    model = cui_dock_create((cui_dock_config){0});
    view = cui_dock_view_create(16, 16);
    assert(ui && model && view);
    options = cui_dock_view_options_default();
    options.on_event = event;
    options.reduced_motion = true;
    const char *titles[] = {"Files", "Editor", "Search", "Disabled", "History"};
    for (size_t i = 0; i < 5; ++i)
        panels[i] = (cui_dock_panel){.key = i == 4 ? UINT64_MAX : i,
                                     .title = cui_string(titles[i]),
                                     .closable = true,
                                     .disabled = i == 3,
                                     .render = content,
                                     .focus_target = 800 + i};
    cui_dock_node left = cui_dock_root(model);
    assert(cui_dock_insert(model, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, left, 1, SIZE_MAX) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, left, 3, SIZE_MAX) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(model, left, 700, 500) == CUI_DOCK_OK);
    frame();
    assert(!result.fit_minimum);
    assert(cui_dock_set_minimum(model, left, 0, 0) == CUI_DOCK_OK);
    assert(rendered[0] == 1 && rendered[1] == 0);
    cui_id stable = cui_dock_tab_id(workspace, 1);
    click(stable);
    assert(active(left) == 1 && result.selected && selected_events == 1);
    assert(rendered[1] > 0);
    click(cui_dock_tab_id(workspace, 3));
    assert(active(left) == 1 && selected_events == 1);
    cui_focus(ui, stable);
    key(CUI_KEY_HOME, 0);
    assert(active(left) == 0);
    key(CUI_KEY_RIGHT, 0);
    assert(active(left) == 1);
    key(CUI_KEY_RIGHT, 0);
    assert(active(left) == 0); /* Disabled tab is skipped. */
    options.focus_content = true;
    click(stable);
    assert(cui_focused(ui) == 801);
    options.focus_content = false;
    cui_dock_node right, bottom;
    assert(cui_dock_split(model, left, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &right) ==
           CUI_DOCK_OK);
    assert(cui_dock_split(model, right, CUI_DOCK_SPLIT_VERTICAL, .5, UINT64_MAX, false, &bottom) ==
           CUI_DOCK_OK);
    frame();
    frame();
    cui_dock_node split = cui_dock_root(model);
    cui_id handle = cui_dock_resize_id(workspace, model, split);
    cui_focus(ui, handle);
    key(CUI_KEY_RIGHT, 0);
    cui_dock_node_info info;
    assert(cui_dock_node_get(model, split, &info) && info.ratio > .5 && result.resized &&
           resized_events == 1);
    frame();
    cui_rect band = cui_bounds(ui, handle);
    pointer(CUI_EVENT_POINTER_DOWN, band.x + band.w / 2, band.y + band.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, band.x + band.w / 2 + 60, band.y + band.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_UP, band.x + band.w / 2 + 60, band.y + band.h / 2);
    frame();
    assert(cui_dock_node_get(model, split, &info) && info.ratio > .58 && resized_events >= 2);
    assert(cui_dock_set_minimum(model, left, 100, 80) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(model, right, 100, 80) == CUI_DOCK_OK);
    options.width = 120;
    options.height = 100;
    frame();
    assert(!result.fit_minimum);
    options.width = 640;
    options.height = 480;
    frame();
    /* Same-group drag reorders stable keys without changing the active panel. */
    cui_rect first = cui_bounds(ui, cui_dock_tab_id(workspace, 0));
    cui_rect second = cui_bounds(ui, stable);
    pointer(CUI_EVENT_POINTER_DOWN, second.x + second.w / 2, second.y + second.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_MOVE, first.x + 2, first.y + first.h / 2);
    frame();
    pointer(CUI_EVENT_POINTER_UP, first.x + 2, first.y + first.h / 2);
    frame();
    uint64_t panel;
    assert(cui_dock_panel_at(model, left, 0, &panel) && panel == 1 && reordered_events == 1);
    assert(active(left) == 1 && cui_dock_tab_id(workspace, 1) == stable);
    frame();
    click(cui_dock_close_id(workspace, 1));
    assert(result.closed && !cui_dock_panel_find(model, 1, NULL, NULL) && closed_events == 1);
    frame();
    assert(active(left) == 0);
    cui_focus(ui, cui_dock_tab_id(workspace, 0));
    key(CUI_KEY_W, CUI_MOD_CTRL);
    assert(result.closed && !cui_dock_panel_find(model, 0, NULL, NULL));
    frame();
    click(cui_dock_close_id(workspace, 3));
    assert(cui_dock_panel_find(model, 3, NULL, NULL));
    options.disabled = true;
    click(cui_dock_tab_id(workspace, UINT64_MAX));
    assert(!result.selected);
    options.disabled = false;
    char saved[4096];
    size_t length;
    assert(cui_dock_serialize(model, saved, sizeof saved, &length) == CUI_DOCK_OK);
    assert(cui_dock_restore(model, (cui_str){saved, length}) == CUI_DOCK_OK);
    frame();
    assert(cui_dock_panel_find(model, UINT64_MAX, NULL, NULL));
    cui_dock_view_reset(view);
    frame();
    /* Closing a one-tab split collapses its node and focuses a surviving leaf. */
    click(cui_dock_close_id(workspace, UINT64_MAX));
    assert(result.closed && !cui_dock_panel_find(model, UINT64_MAX, NULL, NULL));
    frame();
    assert(cui_focused(ui) != cui_dock_tab_id(workspace, UINT64_MAX));
    cui_dock_view *small = cui_dock_view_create(1, 1);
    assert(small && cui_begin_frame(ui, 640, 480, .016f));
    cui_theme theme = cui_theme_light();
    assert(cui_dock_view_render(ui, workspace, &theme, model, small, panels, 5, options).error ==
           CUI_DOCK_CAPACITY);
    cui_end_frame(ui);
    cui_dock_view_destroy(small);
    test_cross_group();
    test_policy_preview();
    test_zoom_toolbar();
    test_edge_regions();
    test_workspace_transfers();
    test_oversized_content();
    test_panel_menus();
    test_title_composition();
    test_title_modes_padding();
    test_group_constraints();
    test_workspace_constraints();
    /* Invalid registries submit nothing; missing metadata remains a safe placeholder. */
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_dock_panel duplicate[2] = {panels[0], panels[0]};
    assert(cui_dock_view_render(ui, workspace, &theme, model, view, duplicate, 2, options).error ==
           CUI_DOCK_INVALID);
    cui_end_frame(ui);
    assert(cui_begin_frame(ui, 640, 480, .016f));
    assert(cui_dock_view_render(ui, workspace, &theme, model, view, NULL, 0, options).error ==
           CUI_DOCK_OK);
    assert(cui_end_frame(ui).count > 0);
    cui_dock_destroy(model);
    model = cui_dock_create((cui_dock_config){0});
    assert(model);
    frame(); /* Empty root renders a workspace without panel callbacks. */
    cui_dock_view_destroy(view);
    cui_dock_destroy(model);
    cui_destroy(ui);
    puts("styled dock: tabs, gestures, resize, policies, previews, toolbar, zoom, edge regions "
         "and persistence passed");
    return 0;
}
