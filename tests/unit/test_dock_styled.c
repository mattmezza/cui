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
    assert(cui_dock_view_set_zoom(view, model, 1, true));
    frame();
    assert(cui_dock_view_zoomed(view, &maximized) && maximized == 1);
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
    puts("styled dock: tabs, keyboard, close, stable reorder, split resize, minima, persistence "
         "passed");
    return 0;
}
