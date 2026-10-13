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
static cui_dock_view_result result;
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
}
static void frame(void) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 640, 480, .016f));
    result = cui_dock_view_render(ui, workspace, &theme, model, view, panels, 5, options);
    cui_draw_list commands = cui_end_frame(ui);
    assert(result.error == CUI_DOCK_OK && cui_last_error(ui) == CUI_OK && commands.count > 0);
    for (size_t i = 0; i < commands.count; ++i) {
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
