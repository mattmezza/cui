/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdlib.h>
static int fail_part_allocation;
static size_t part_heap_calls;
static void *part_test_malloc(size_t size) {
    ++part_heap_calls;
    return malloc(size);
}
static void *part_test_calloc(size_t count, size_t size) {
    ++part_heap_calls;
    if (fail_part_allocation) {
        fail_part_allocation = 0;
        return NULL;
    }
    return calloc(count, size);
}
#define calloc part_test_calloc
#define malloc part_test_malloc
#define main shadow_root_test_main
#include "test_bridge_shadows.c"
#undef main
#undef calloc
#undef malloc

static cui_bridge_shadow_target_v1 target(uint32_t role) {
    return (cui_bridge_shadow_target_v1){16, 1, role, 0};
}
static cui_bridge_status set_part(cui_bridge_app *app, cui_bridge_node n, uint32_t role,
                                  const cui_bridge_box_shadows_v1 *style) {
    cui_bridge_shadow_target_v1 t = target(role);
    return cui_bridge_set_target_box_shadows_v1(app, n, 16, &t, 224, style);
}
static void reset_part(cui_bridge_app *app, cui_bridge_node n, uint32_t role) {
    cui_bridge_shadow_target_v1 t = target(role);
    assert(cui_bridge_reset_target_box_shadows_v1(app, n, 16, &t) == 0);
}
static bool current_id(cui_context *ui, cui_id id) {
    size_t required = 0;
    (void)cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, NULL, 0, &required);
    assert(required);
    char *json = malloc(required);
    assert(json);
    assert(cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, required, &required));
    char needle[64];
    snprintf(needle, sizeof needle, "\"id\":\"%llu\"", (unsigned long long)id);
    bool found = strstr(json, needle) != NULL;
    free(json);
    return found;
}
typedef struct {
    cui_draw_command command;
    char bytes[128];
} part_text_snapshot;
static size_t capture_part_text(cui_draw_list list, part_text_snapshot out[16]) {
    size_t count = 0;
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT) {
            assert(count < 16 && list.commands[i].text.length < sizeof out[count].bytes);
            out[count].command = list.commands[i];
            memcpy(out[count].bytes, list.commands[i].text.data, list.commands[i].text.length);
            ++count;
        }
    return count;
}
static void check_part_text(cui_draw_list list, const part_text_snapshot expected[16],
                            size_t count) {
    size_t matched = 0;
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT) {
            assert(matched < count);
            const cui_draw_command *a = &expected[matched].command, *b = &list.commands[i];
            assert(!memcmp(&a->bounds, &b->bounds, sizeof a->bounds));
            assert(a->font_size == b->font_size);
            assert(!memcmp(&a->text_style, &b->text_style, sizeof a->text_style));
            assert(!memcmp(&a->color, &b->color, sizeof a->color));
            assert(a->text.length == b->text.length &&
                   !memcmp(expected[matched].bytes, b->text.data, a->text.length));
            assert(a->text_color_count == b->text_color_count &&
                   a->text_paint.kind == b->text_paint.kind);
            ++matched;
        }
    assert(matched == count);
}
static void part_roots(void) {
    const cui_bridge_kind kinds[] = {CUI_BRIDGE_CHECKBOX, CUI_BRIDGE_RADIO, CUI_BRIDGE_SWITCH,
                                     CUI_BRIDGE_SLIDER};
    for (unsigned k = 0; k < 4; ++k) {
        cui_bridge_node n;
        cui_bridge_app *app = create(kinds[k], &n);
        cui_bridge_node_record *node = &app->nodes[n.slot];
        cui_bridge_box_shadows_v1 style = mixed();
        cui_set_layout_debug(app->ui, true);
        cui_software *renderer = cui_software_create(360, 240, NULL);
        assert(renderer);
        /* Keep the full 345600-byte pixel oracle off the stack across Debug render calls. */
        const size_t baseline_bytes = 360u * 240u * 4u;
        unsigned char *baseline_pixels = malloc(baseline_bytes);
        assert(baseline_pixels);
        cui_draw_list original = render(app, 0);
        part_text_snapshot text[16];
        size_t text_count = capture_part_text(original, text);
        assert(text_count);
        cui_rect row = cui_bounds(app->ui, node->id);
        size_t semantics;
        (void)cui_semantics(app->ui, &semantics);
        assert(!node->part_shadows);
        assert(render_paint(renderer, &original));
        memcpy(baseline_pixels, cui_software_pixels(renderer, NULL, NULL), baseline_bytes);
        if (k < 2)
            assert(!current_id(app->ui, bridge_indicator_id(node)));
        unsigned baseline = shadows(original, CUI_DRAW_DROP_SHADOW);
        cui_bridge_shadow_target_v1 main = target(CUI_BRIDGE_SHADOW_MAIN);
        cui_bridge_shadow_target_info_v1 info;
        assert(cui_bridge_shadow_target_query_v1(app, n, 16, &main, 16, &info) == 0);
        uint32_t physical = k < 2 ? CUI_BRIDGE_SHADOW_INDICATOR : CUI_BRIDGE_SHADOW_TRACK;
        assert(info.flags & CUI_BRIDGE_SHADOW_SUPPORTED);
        assert(info.canonical_role == physical);
        fail_part_allocation = 1;
        assert(set_part(app, n, CUI_BRIDGE_SHADOW_MAIN, &style) == CUI_BRIDGE_OUT_OF_MEMORY);
        assert(!node->part_shadows && !node->shadows.schema_version);
        assert(set_part(app, n, CUI_BRIDGE_SHADOW_MAIN, &style) == 0);
        const cui_bridge_box_shadows_v1 *saved = bridge_target_style(node, physical);
        assert(saved && saved->count == 2);
        style.layers[0].sigma = NAN;
        assert(set_part(app, n, physical, &style) == CUI_BRIDGE_INVALID_ARGUMENT);
        assert(saved->layers[0].sigma == 0);
        style = mixed();
        cui_draw_list part_only = render(app, 0);
        check_part_text(part_only, text, text_count);
        assert(shadows(part_only, CUI_DRAW_DROP_SHADOW) == baseline + 1);
        assert(render_paint(renderer, &part_only));
        assert(memcmp(baseline_pixels, cui_software_pixels(renderer, NULL, NULL), baseline_bytes));
        assert(set_part(app, n, CUI_BRIDGE_SHADOW_ROOT, &style) == 0);
        assert(cui_bridge_set_box_shadows_v1(app, n, 224, &style) == CUI_BRIDGE_INVALID_ARGUMENT);
        cui_draw_list list = render(app, 0);
        check_part_text(list, text, text_count);
        assert(shadows(list, CUI_DRAW_DROP_SHADOW) == baseline + 2);
        assert(render_paint(renderer, &list));
        assert(memcmp(baseline_pixels, cui_software_pixels(renderer, NULL, NULL), baseline_bytes));
        cui_rect after = cui_bounds(app->ui, node->id);
        assert(!memcmp(&row, &after, sizeof row));
        size_t after_semantics;
        (void)cui_semantics(app->ui, &after_semantics);
        assert(semantics == after_semantics);
        cui_id part = k < 2    ? bridge_indicator_id(node)
                      : k == 2 ? cui_switch_track_id(node->id)
                               : cui_slider_track_id(node->id);
        cui_rect caster = cui_bounds(app->ui, part);
        assert(caster.w > 0 && caster.h > 0 && caster.w < row.w);
        if (k == 2) {
            assert(set_part(app, n, CUI_BRIDGE_SHADOW_THUMB, &style) == 0);
            list = render(app, 0);
            assert(shadows(list, CUI_DRAW_DROP_SHADOW) == baseline + 3);
            reset_part(app, n, CUI_BRIDGE_SHADOW_THUMB);
        } else {
            assert(set_part(app, n, CUI_BRIDGE_SHADOW_THUMB, &style) ==
                   CUI_BRIDGE_INVALID_ARGUMENT);
        }
        list = render(app, 1);
        assert(shadows(list, CUI_DRAW_DROP_SHADOW) == baseline + 2);
        list = render(app, 2);
        assert(!shadows(list, CUI_DRAW_DROP_SHADOW));
        list = render(app, 3);
        assert(!shadows(list, CUI_DRAW_DROP_SHADOW) && bridge_target_style(node, physical));
        style.count = 0;
        assert(set_part(app, n, physical, &style) == 0);
        assert(bridge_target_style(node, physical));
        reset_part(app, n, CUI_BRIDGE_SHADOW_ROOT);
        list = render(app, 0);
        assert(!shadows(list, CUI_DRAW_DROP_SHADOW));
        reset_part(app, n, CUI_BRIDGE_SHADOW_MAIN);
        assert(!node->part_shadows);
        list = render(app, 0);
        assert(shadows(list, CUI_DRAW_DROP_SHADOW) == baseline);
        assert(render_paint(renderer, &list));
        assert(!memcmp(baseline_pixels, cui_software_pixels(renderer, NULL, NULL), baseline_bytes));
        if (k < 2)
            assert(!current_id(app->ui, bridge_indicator_id(node)));
        free(baseline_pixels);
        cui_software_destroy(renderer);
        assert(set_part(app, n, physical, &style) == 0);
        assert(cui_bridge_node_remove(app, n) == 0);
        cui_bridge_node replacement, root = {app->root, app->nodes[app->root].generation};
        assert(cui_bridge_node_create(app, root, kinds[k], "reused", &replacement) == 0);
        assert(replacement.slot == n.slot && !app->nodes[replacement.slot].part_shadows);
        assert(set_part(app, n, physical, &style) == CUI_BRIDGE_STALE_HANDLE);
        cui_bridge_destroy(app);
    }
}
static void group_parts(void) {
    cui_bridge_node group;
    cui_bridge_app *app = create(CUI_BRIDGE_RADIO_GROUP, &group);
    cui_bridge_node a, b;
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "a", &a) == 0);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "b", &b) == 0);
    assert(cui_bridge_set_text(app, a, "First") == 0 && cui_bridge_set_text(app, b, "Second") == 0);
    cui_bridge_box_shadows_v1 style = mixed();
    assert(set_part(app, group, CUI_BRIDGE_SHADOW_MAIN, &style) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(set_part(app, a, CUI_BRIDGE_SHADOW_INDICATOR, &style) == 0);
    assert(set_part(app, b, CUI_BRIDGE_SHADOW_ROOT, &style) == 0);
    cui_draw_list list = render(app, 0);
    assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 2);
    /* All peers are named by the C group route, even the unstyled second indicator. */
    assert(cui_bounds(app->ui, bridge_indicator_id(&app->nodes[a.slot])).w > 0);
    assert(cui_bounds(app->ui, bridge_indicator_id(&app->nodes[b.slot])).w > 0);
    /* Exercise the real owner's captured slot map under reordered retained links. */
    app->nodes[group.slot].first_child = b.slot;
    app->nodes[group.slot].last_child = a.slot;
    app->nodes[b.slot].next_sibling = a.slot;
    app->nodes[a.slot].next_sibling = 0;
    app->nodes[group.slot].value = a.slot;
    list = render(app, 0);
    assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 2 && app->nodes[group.slot].value == a.slot);
    assert(bridge_target_style(&app->nodes[a.slot], CUI_BRIDGE_SHADOW_INDICATOR));
    assert(!bridge_target_style(&app->nodes[b.slot], CUI_BRIDGE_SHADOW_INDICATOR));
    reset_part(app, a, CUI_BRIDGE_SHADOW_MAIN);
    assert(!app->nodes[a.slot].part_shadows);
    list = render(app, 0);
    assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 1);
    cui_bridge_node root = {app->root, app->nodes[app->root].generation}, tabs, item;
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TABS, "tabs", &tabs) == 0);
    assert(cui_bridge_node_create(app, tabs, CUI_BRIDGE_ITEM, "item", &item) == 0);
    assert(set_part(app, item, CUI_BRIDGE_SHADOW_INDICATOR, &style) == CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_shadow_target_v1 t = target(CUI_BRIDGE_SHADOW_INDICATOR);
    cui_bridge_shadow_target_info_v1 info = {0};
    assert(cui_bridge_shadow_target_query_v1(app, item, 16, &t, 16, &info) == 0);
    assert(!info.flags && info.canonical_role == UINT32_MAX);
    cui_bridge_destroy(app);
}
static void old_root_topology_unchanged(void) {
    cui_bridge_node group, button;
    cui_bridge_app *app = create(CUI_BRIDGE_GROUP_BOX, &group);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_BUTTON, "nested", &button) == 0);
    cui_bridge_box_shadows_v1 style = mixed();
    assert(set_part(app, button, CUI_BRIDGE_SHADOW_ROOT, &style) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!app->nodes[button.slot].part_shadows);
    assert(cui_bridge_set_box_shadows_v1(app, button, 224, &style) == 0);
    cui_draw_list list = render(app, 0);
    assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 1);
    cui_bridge_destroy(app);
}
static void whole_group64(void) {
    cui_bridge_status status;
    cui_bridge_app *app =
        cui_bridge_create(&(cui_bridge_options){.struct_size = sizeof(cui_bridge_options),
                                                .abi_version = 1,
                                                .max_nodes = 80,
                                                .width = 360,
                                                .height = 240},
                          &status);
    assert(app && status == 0);
    cui_bridge_node root, group, items[64];
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           0);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RADIO_GROUP, "g", &group) == 0);
    for (unsigned i = 0; i < 64; ++i) {
        char key[24];
        snprintf(key, sizeof key, "item-%u", i);
        assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, key, &items[i]) == 0);
    }
    cui_set_layout_debug(app->ui, true);
    cui_bridge_box_shadows_v1 style = mixed();
    assert(set_part(app, items[0], CUI_BRIDGE_SHADOW_ROOT, &style) == 0);
    render(app, 0);
    for (unsigned i = 0; i < 64; ++i)
        assert(!current_id(app->ui, bridge_indicator_id(&app->nodes[items[i].slot])));
    /* Empty still opts into naming every peer; capability query never allocates. */
    style.count = 0;
    cui_bridge_shadow_target_v1 t = target(CUI_BRIDGE_SHADOW_MAIN);
    cui_bridge_shadow_target_info_v1 info;
    fail_part_allocation = 1;
    size_t heap_before_query = part_heap_calls;
    for (unsigned i = 0; i < 64; ++i)
        assert(cui_bridge_shadow_target_query_v1(app, items[i], 16, &t, 16, &info) == 0 &&
               info.flags == 3);
    assert(fail_part_allocation == 1 && part_heap_calls == heap_before_query);
    fail_part_allocation = 0;
    assert(set_part(app, items[0], CUI_BRIDGE_SHADOW_MAIN, &style) == 0);
    render(app, 0);
    for (unsigned i = 0; i < 64; ++i)
        assert(current_id(app->ui, bridge_indicator_id(&app->nodes[items[i].slot])));
    reset_part(app, items[0], CUI_BRIDGE_SHADOW_INDICATOR);
    render(app, 0);
    for (unsigned i = 0; i < 64; ++i)
        assert(!current_id(app->ui, bridge_indicator_id(&app->nodes[items[i].slot])));
    assert(set_part(app, items[63], CUI_BRIDGE_SHADOW_MAIN, &style) == 0);
    assert(cui_bridge_node_remove(app, items[63]) == 0);
    render(app, 0);
    for (unsigned i = 0; i < 63; ++i)
        assert(!current_id(app->ui, bridge_indicator_id(&app->nodes[items[i].slot])));
    cui_bridge_destroy(app);
}
static cui_size measure_parts(void *user, cui_str text, float font) {
    (void)text;
    cui_bridge_app *app = user;
    assert(!cui_frame_paint_enabled(app->ui));
    bridge_apply_parts(app, &app->nodes[2]);
    return (cui_size){font * 4, font};
}
static void part_input_errors(void) {
    cui_bridge_node n;
    cui_bridge_app *app = create(CUI_BRIDGE_CHECKBOX, &n);
    cui_bridge_box_shadows_v1 style = mixed();
    assert(set_part(app, n, CUI_BRIDGE_SHADOW_INDICATOR, &style) == 0);
    cui_destroy(app->ui);
    app->ui = cui_create((cui_config){.measure = measure_parts, .measure_user = app});
    assert(app->ui);
    assert(cui_bridge_step(app, .016f) == 0);
    cui_bridge_rect b;
    assert(cui_bridge_node_bounds(app, n, &b) == 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, b.x + b.width / 2,
                                    b.y + b.height / 2) == 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, b.x + b.width / 2,
                                    b.y + b.height / 2) == 0);
    assert(cui_bridge_step(app, .016f) == 0);
    assert(app->nodes[n.slot].checked);
    app->run_dispatching = true;
    assert(set_part(app, n, CUI_BRIDGE_SHADOW_MAIN, &style) == 0);
    app->run_dispatching = false;
    assert(cui_begin_frame(app->ui, 360, 240, .016f));
    app->frame_error = CUI_BRIDGE_OK;
    bridge_apply_parts(app, &app->nodes[n.slot]);
    assert(app->frame_error == CUI_BRIDGE_CORE_ERROR);
    (void)cui_end_frame(app->ui);
    cui_clear_error(app->ui);
    render(app, 0);
    cui_destroy(app->ui);
    app->ui =
        cui_create((cui_config){.max_elements = 2, .measure = bridge_measure, .measure_user = app});
    assert(app->ui);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CORE_ERROR);
    assert(bridge_target_style(&app->nodes[n.slot], CUI_BRIDGE_SHADOW_INDICATOR));
    cui_destroy(app->ui);
    app->ui = cui_create((cui_config){.measure = bridge_measure, .measure_user = app});
    assert(app->ui);
    render(app, 0);
    cui_bridge_shadow_target_v1 t = target(CUI_BRIDGE_SHADOW_MAIN);
    t.reserved = 1;
    assert(cui_bridge_set_target_box_shadows_v1(app, n, 16, &t, 224, &style) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    app->stepping = true;
    assert(cui_bridge_set_target_box_shadows_v1(app, (cui_bridge_node){999, 999}, 0, NULL, 0,
                                                NULL) == CUI_BRIDGE_INVALID_STATE);
    app->stepping = false;
    cui_bridge_destroy(app);
}
int main(void) {
    part_roots();
    group_parts();
    old_root_topology_unchanged();
    whole_group64();
    part_input_errors();
    printf("part node bytes=%zu; pointer=%zu; payloads=%zu/%zu\n", sizeof(cui_bridge_node_record),
           sizeof(void *), sizeof(bridge_part_shadows) + 224, sizeof(bridge_part_shadows) + 448);
    return 0;
}
