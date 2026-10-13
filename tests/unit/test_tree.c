/* SPDX-License-Identifier: MIT */
#include "cui/cui_tree.h"
#ifndef CUI_TREE_BASE_ONLY
#include "cui/cui_components.h"
#include "cui/cui_scroll.h"
#include "cui/cui_toolbar.h"
#endif
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static void models(void) {
    cui_tree_model *m = cui_tree_create(10);
    assert(m && cui_tree_memory_bytes(m) > 0);
    cui_tree_node nodes[] = {{1, {"Root", 4}, 0, false},
                             {2, {"Child", 5}, 1, false},
                             {3, {"Leaf", 4}, 2, false},
                             {4, {"Sibling", 7}, 1, false},
                             {5, {"Other", 5}, 0, false}};
    bool expanded[] = {true, true, false, false, false};
    assert(cui_tree_rebuild(m, nodes, expanded, 5) == CUI_TREE_OK &&
           cui_tree_visible_count(m) == 5);
    size_t parent, index;
    assert(cui_tree_parent(m, 2, &parent) && parent == 1 && cui_tree_parent(m, 1, &parent) &&
           parent == 0);
    assert(!cui_tree_parent(m, 0, &parent) && cui_tree_is_branch(m, 0) &&
           !cui_tree_is_branch(m, 2));
    assert(cui_tree_set_expanded(m, 1, false) && cui_tree_visible_count(m) == 2 && !expanded[0]);
    assert(cui_tree_visible_node(m, 1, &index) && index == 4);
    assert(cui_tree_set_expanded(m, 1, true) && cui_tree_visible_count(m) == 5);
    cui_tree_node duplicate[] = {{9, {"a", 1}, 0, false}, {9, {"b", 1}, 0, false}};
    bool flags[] = {true, true};
    assert(cui_tree_rebuild(m, duplicate, flags, 2) == CUI_TREE_DUPLICATE_KEY);
    assert(cui_tree_count(m) == 5 && cui_tree_node_index(m, 3, &index) && index == 2 &&
           cui_tree_visible_count(m) == 5);
    duplicate[1].key = 8;
    duplicate[1].depth = 2;
    assert(cui_tree_rebuild(m, duplicate, flags, 2) == CUI_TREE_INVALID && cui_tree_count(m) == 5);
    assert(cui_tree_rebuild(m, nodes, expanded, 11) == CUI_TREE_CAPACITY);
    cui_tree_view view = cui_tree_viewport(m, 25, 60, 20, 1);
    assert(view.begin == 0 && view.end == 5 && view.scroll == 25 && view.total == 100 &&
           view.before == 0 && view.after == 0);
    view = cui_tree_viewport(m, INFINITY, 0, NAN, SIZE_MAX);
    assert(view.begin == 0 && view.end == 5);
    cui_tree_destroy(m);
    const size_t count = 10000;
    nodes[0].depth = 0;
    m = cui_tree_create(count);
    assert(m);
    cui_tree_node *large = calloc(count, sizeof(*large));
    bool *open = calloc(count, sizeof(*open));
    assert(large && open);
    for (size_t i = 0; i < count; ++i)
        large[i] = (cui_tree_node){.key = i + 1, .label = {"item", 4}};
    assert(cui_tree_rebuild(m, large, open, count) == CUI_TREE_OK);
    view = cui_tree_viewport(m, 160000, 240, 32, 2);
    assert(view.begin == 4998 && view.end == 5010 && view.total == 320000);
    free(large);
    free(open);
    cui_tree_destroy(m);
}
static void random_models(void) {
    const size_t count = 128;
    cui_tree_node nodes[128];
    bool expanded[128];
    uint32_t random = 173;
    unsigned depth = 0;
    for (size_t i = 0; i < count; ++i) {
        random = random * 1664525u + 1013904223u;
        depth = i ? (unsigned)(random % (depth < 6 ? depth + 2 : 7)) : 0;
        nodes[i] = (cui_tree_node){.key = i + 1, .depth = depth, .label = {"Node", 4}};
        expanded[i] = (random & 8) != 0;
    }
    cui_tree_model *model = cui_tree_create(count);
    assert(model);
    for (unsigned trial = 0; trial < 1000; ++trial) {
        random = random * 1664525u + 1013904223u;
        expanded[random % count] = !expanded[random % count];
        assert(cui_tree_rebuild(model, nodes, expanded, count) == CUI_TREE_OK);
        size_t visible = 0;
        for (size_t i = 0; i < count; ++i) {
            bool shown = true;
            unsigned ancestor_depth = nodes[i].depth;
            for (size_t j = i; j > 0 && ancestor_depth;) {
                --j;
                if (nodes[j].depth < ancestor_depth) {
                    if (!expanded[j])
                        shown = false;
                    ancestor_depth = nodes[j].depth;
                }
            }
            if (shown) {
                size_t actual;
                assert(cui_tree_visible_node(model, visible++, &actual) && actual == i);
            }
            size_t index;
            assert(cui_tree_node_index(model, nodes[i].key, &index) && index == i);
        }
        assert(visible == cui_tree_visible_count(model));
    }
    cui_tree_destroy(model);
}
static void key(cui_context *ui, cui_key code) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code}));
}
static void click(cui_context *ui, cui_rect bounds) {
    assert(bounds.w > 0 && bounds.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2}));
}
static cui_tree_result scene(cui_context *ui, const cui_theme *theme, cui_tree_model *model,
                             cui_tree_state *state, cui_id *selected, cui_tree_options options) {
    assert(cui_begin_frame(ui, 700, 500, .016f));
#ifdef CUI_TREE_BASE_ONLY
    (void)theme;
    cui_tree_result result = cui_base_tree(ui, 10, model, state, selected, options);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        cui_begin_box(ui, sem[i].id, (cui_box){.width = 200, .height = 24});
        cui_end_box(ui);
    }
    assert(cui_end_frame(ui).count == 0);
#else
    cui_tree_result result = cui_tree(ui, 10, theme, model, state, selected, options);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count);
#endif
    assert(cui_last_error(ui) == CUI_OK && result.error == CUI_TREE_OK);
    return result;
}
static void events(void) {
#ifdef CUI_TREE_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (unsigned i = 0; i < 4; ++i) {
        cui_tree_node nodes[] = {{1, {"Root", 4}, 0, false},  {2, {"Branch", 6}, 1, false},
                                 {3, {"Child", 5}, 2, false}, {4, {"Disabled", 8}, 1, true},
                                 {5, {"Beta", 4}, 1, false},  {6, {"Zulu", 4}, 0, false}};
        bool expanded[] = {true, false, false, false, false, false};
        cui_id selected = 1;
        cui_tree_model *model = cui_tree_create(16);
        assert(model && cui_tree_rebuild(model, nodes, expanded, 6) == CUI_TREE_OK);
        cui_tree_state state = {0};
        cui_tree_options options = {.height = 240, .row_height = 32, .dt = .016f};
        cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
        assert(ui);
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(state.cursor == 1);
        cui_focus(ui, cui_tree_row_id(10, 1));
        key(ui, CUI_KEY_DOWN);
        key(ui, CUI_KEY_DOWN);
        assert(scene(ui, &themes[i], model, &state, &selected, options).selection_changed &&
               selected == 5 && state.cursor == 5);
        key(ui, CUI_KEY_HOME);
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 1);
        key(ui, CUI_KEY_RIGHT);
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 2);
        key(ui, CUI_KEY_RIGHT);
        assert(scene(ui, &themes[i], model, &state, &selected, options).expansion_changed &&
               expanded[1]);
        key(ui, CUI_KEY_RIGHT);
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 3);
        key(ui, CUI_KEY_LEFT);
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 2);
        key(ui, CUI_KEY_LEFT);
        assert(scene(ui, &themes[i], model, &state, &selected, options).expansion_changed &&
               !expanded[1]);
        click(ui, cui_bounds(ui, cui_tree_toggle_id(10, 2)));
        assert(scene(ui, &themes[i], model, &state, &selected, options).expansion_changed &&
               expanded[1] && cui_focused(ui) == cui_tree_row_id(10, 2));
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "b"});
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 5);
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "b"});
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 2);
        options.dt = 1;
        scene(ui, &themes[i], model, &state, &selected, options);
        options.dt = .016f;
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"});
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 6);
        key(ui, CUI_KEY_ENTER);
        assert(scene(ui, &themes[i], model, &state, &selected, options).activated == 6);
        options.manual_selection = true;
        key(ui, CUI_KEY_UP);
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 6 && state.cursor == 5);
        options.disabled = true;
        key(ui, CUI_KEY_HOME);
        scene(ui, &themes[i], model, &state, &selected, options);
        assert(selected == 6);
        options.disabled = false;
        scene(ui, &themes[i], model, &state, &selected, options);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        bool hierarchy = false, disabled = false;
        for (size_t j = 0; j < n; ++j) {
            if (sem[j].id == cui_tree_row_id(10, 3))
                hierarchy = sem[j].details.parent == cui_tree_row_id(10, 2) &&
                            sem[j].role == CUI_ROLE_TREEITEM && sem[j].details.level == 3 &&
                            sem[j].details.pos_in_set == 1 && sem[j].details.set_size == 1;
            if (sem[j].id == cui_tree_row_id(10, 4))
                disabled = sem[j].disabled && sem[j].details.level == 2 &&
                           sem[j].details.pos_in_set == 2 && sem[j].details.set_size == 3;
        }
        assert(hierarchy && disabled);
        cui_destroy(ui);
        cui_tree_destroy(model);
    }
}
static void disabled_hierarchy(void) {
    cui_tree_node nodes[] = {{1, {"Root", 4}, 0, false},
                             {2, {"Blocked", 7}, 1, true},
                             {3, {"Grandchild", 10}, 2, false},
                             {4, {"Enabled", 7}, 1, false}};
    bool expanded[] = {true, true, false, false};
    cui_tree_model *model = cui_tree_create(4);
    assert(model && cui_tree_rebuild(model, nodes, expanded, 4) == CUI_TREE_OK);
#ifdef CUI_TREE_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    cui_tree_state state = {0};
    cui_id selected = 1;
    cui_tree_options options = {.height = 240, .row_height = 32};
    scene(ui, &theme, model, &state, &selected, options);
    cui_focus(ui, cui_tree_row_id(10, 1));
    key(ui, CUI_KEY_RIGHT);
    scene(ui, &theme, model, &state, &selected, options);
    assert(selected == 4);
    cui_focus(ui, cui_tree_row_id(10, 3));
    state.cursor = 3;
    selected = 3;
    key(ui, CUI_KEY_LEFT);
    scene(ui, &theme, model, &state, &selected, options);
    assert(selected == 1);
    cui_destroy(ui);
    cui_tree_destroy(model);
}
static void virtualization(void) {
    size_t count = 10000;
    cui_tree_model *model = cui_tree_create(count);
    assert(model);
    cui_tree_node *nodes = calloc(count, sizeof(*nodes));
    bool *expanded = calloc(count, sizeof(*expanded));
    assert(nodes && expanded);
    for (size_t i = 0; i < count; ++i)
        nodes[i] = (cui_tree_node){.key = i + 1, .label = {"Item", 4}};
    assert(cui_tree_rebuild(model, nodes, expanded, count) == CUI_TREE_OK);
    cui_tree_state state = {.scroll = 160000};
    cui_id selected = 0;
#ifdef CUI_TREE_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    state.scroll = 160005;
    scene(ui, &theme, model, &state, &selected,
          (cui_tree_options){.height = 240, .row_height = 32, .overscan = 2});
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 13 && sem[1].id == cui_tree_row_id(10, 4999));
    assert(sem[1].details.level == 1 && sem[1].details.pos_in_set == 4999 &&
           sem[1].details.set_size == 10000);
    assert(sem[0].details.scrollable && sem[0].details.scroll_max_y == 319760);
#ifndef CUI_TREE_BASE_ONLY
    assert(cui_bounds(ui, cui_tree_row_id(10, 5001)).y == -5);
    click(ui, cui_bounds(ui, cui_tree_row_id(10, 4999)));
    scene(ui, &theme, model, &state, &selected,
          (cui_tree_options){.height = 240, .row_height = 32, .overscan = 2});
    assert(selected == 0);
    click(ui, cui_bounds(ui, cui_tree_row_id(10, 5010)));
    scene(ui, &theme, model, &state, &selected,
          (cui_tree_options){.height = 240, .row_height = 32, .overscan = 2});
    assert(selected == 0);
    cui_focus(ui, cui_tree_row_id(10, 5001));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 10, .y = 100}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 10, .y = 100, .dy = -1}));
    scene(ui, &theme, model, &state, &selected,
          (cui_tree_options){.height = 240, .row_height = 32, .overscan = 2});
    assert(state.scroll == 160037);
    scene(ui, &theme, model, &state, &selected,
          (cui_tree_options){.height = 240, .row_height = 32, .overscan = 2});
    assert(state.scroll == 160037);
    assert(cui_queue_event(
        ui, (cui_event){
                .type = CUI_EVENT_SCROLL, .x = 10, .y = 100, .dy = -3.5f, .scroll_precise = true}));
    scene(ui, &theme, model, &state, &selected,
          (cui_tree_options){.height = 240, .row_height = 32, .overscan = 2});
    assert(state.scroll == 160040.5f);
#endif
    cui_destroy(ui);
    cui_tree_destroy(model);
    free(nodes);
    free(expanded);
}
#ifndef CUI_TREE_BASE_ONLY
static void tree_toolbar_artwork(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 200});
    cui_tree_model *model = cui_tree_create(4);
    cui_tree_node nodes[] = {{1, {"Root", 4}, 0, false}, {2, {"Child", 5}, 1, false}};
    bool expanded[] = {false, false};
    cui_id selected = 1;
    cui_tree_state state = {0};
    cui_theme t = cui_theme_light();
    unsigned char previous[20 * 20 * 4];
    size_t length = 0;
    assert(ui && model && cui_tree_rebuild(model, nodes, expanded, 2) == CUI_TREE_OK);
    for (unsigned phase = 0; phase < 3; phase++) {
        if (phase) {
            cui_focus(ui, cui_tree_row_id(10, 1));
            key(ui, CUI_KEY_RIGHT);
        }
        assert(cui_begin_frame(ui, 500, 200, .016f));
        if (phase == 2)
            assert(cui_consume_event(ui, 0));
        assert(cui_base_toolbar_begin(ui, 9000, cui_string("Commands"), (cui_box){.row = true}));
        cui_tree(ui, 10, &t, model, &state, &selected,
                 (cui_tree_options){.width = 240, .height = 120, .row_height = 32});
        cui_button(ui, 9001, &t, cui_string("Other"), false);
        cui_base_toolbar_end(ui, 9000, false);
        cui_draw_list draw = cui_end_frame(ui);
        unsigned images = 0;
        for (size_t i = 0; i < draw.count; i++)
            if (draw.commands[i].type == CUI_DRAW_IMAGE) {
                cui_bitmap b = draw.commands[i].bitmap.bitmap;
                images++;
                size_t bytes = b.height * b.stride;
                assert(bytes <= sizeof(previous));
                if (!phase) {
                    length = bytes;
                    memcpy(previous, b.pixels, bytes);
                }
                if (phase == 1)
                    assert(bytes != length || memcmp(previous, b.pixels, length));
            }
        assert(images == 1 && cui_last_error(ui) == CUI_OK);
        if (phase)
            assert(cui_event_consumed(ui, 0) && cui_focused(ui) == cui_tree_row_id(10, 1));
        if (phase == 1)
            assert(expanded[0]);
        if (phase == 2)
            assert(selected == 1);
    }
    cui_destroy(ui);
    cui_tree_destroy(model);
}
#endif
#ifndef CUI_TREE_BASE_ONLY
static void tree_scrollbar_skin(void) {
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    cui_tree_model *model = cui_tree_create(12);
    cui_tree_node nodes[12];
    bool expanded[12] = {0};
    for (size_t i = 0; i < 12; i++)
        nodes[i] = (cui_tree_node){i + 1, {"Row", 3}, 0, false};
    assert(cui_tree_rebuild(model, nodes, expanded, 12) == CUI_TREE_OK);
    cui_tree_state state = {0};
    cui_id selected = 1;
    cui_theme theme = cui_theme_light();
    theme.scrollbar = (cui_scrollbar_style){.visibility = CUI_SCROLLBAR_VISIBLE, .thickness = 14};
    cui_tree_options options = {.width = 240, .height = 120, .row_height = 32};
    for (unsigned i = 0; i < 3; i++)
        scene(ui, &theme, model, &state, &selected, options);
    cui_rect track = cui_bounds(ui, cui_scrollbar_id(10, true));
    assert(track.w == 14 && track.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = track.x + track.w / 2,
                                           .y = track.y + track.h - 2}));
    scene(ui, &theme, model, &state, &selected, options);
    assert(state.scroll > 0 && selected == 1);
    theme.scrollbar.visibility = CUI_SCROLLBAR_HIDDEN;
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = track.x, .y = track.y}));
    scene(ui, &theme, model, &state, &selected, options);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        assert(semantics[i].role != CUI_ROLE_SCROLLBAR);
    cui_tree_destroy(model);
    cui_destroy(ui);
}
#endif
int main(void) {
#ifndef CUI_TREE_BASE_ONLY
    tree_scrollbar_skin();
    tree_toolbar_artwork();
#endif
    models();
    random_models();
    events();
    disabled_hierarchy();
    virtualization();
    return 0;
}
