/* SPDX-License-Identifier: MIT */
/* Real portable bridge renderer, inspected without adding a public draw-list API. */
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include "cui/cui_software.h"
#include <assert.h>

static cui_bridge_box_shadows_v1 mixed(void) {
    return (cui_bridge_box_shadows_v1){
        .struct_size = 224,
        .schema_version = 1,
        .count = 2,
        .layers = {{4, 3, 0, 0, 0, 0x000000ff}, {2, 0, 0, 1, 1, 0x102040ff}}};
}
static unsigned shadows(cui_draw_list list, cui_draw_type type) {
    unsigned n = 0;
    for (size_t i = 0; i < list.count; ++i)
        n += list.commands[i].type == type;
    return n;
}
static cui_draw_list render(cui_bridge_app *app, unsigned scope) {
    app->frame_error = CUI_BRIDGE_OK;
    assert(cui_begin_frame(app->ui, 360, 240, .016f));
    cui_begin_box(app->ui, 1,
                  (cui_box){.width = -1,
                            .height = -1,
                            .padding = 16,
                            .inert = scope == 1,
                            .layout_only = scope == 2,
                            .background = {255, 255, 255, 255}});
    if (scope != 3)
        bridge_render_node(app, app->root);
    cui_end_box(app->ui);
    cui_draw_list list = cui_end_frame(app->ui);
    assert(cui_last_error(app->ui) == CUI_OK && app->frame_error == CUI_BRIDGE_OK);
    return list;
}
static cui_bridge_app *create(cui_bridge_kind kind, cui_bridge_node *target) {
    cui_bridge_options options = {.struct_size = sizeof(options),
                                  .abi_version = 1,
                                  .width = 360,
                                  .height = 240,
                                  .max_nodes = 16};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, child;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           0);
    assert(cui_bridge_node_create(app, root, kind, "target", target) == 0);
    if (kind != CUI_BRIDGE_ROW && kind != CUI_BRIDGE_COLUMN)
        assert(cui_bridge_set_text(app, *target, "Paint root") == 0);
    if (kind == CUI_BRIDGE_ROW || kind == CUI_BRIDGE_COLUMN) {
        assert(cui_bridge_node_create(app, *target, CUI_BRIDGE_LABEL, "child", &child) == 0);
        assert(cui_bridge_set_text(app, child, "Nested content") == 0);
    }
    return app;
}
/* Compare box paint without requiring a platform font rasterizer. Text geometry
 * and semantics are checked against the original, unfiltered draw list. */
static bool render_paint(cui_software *renderer, const cui_draw_list *source) {
    cui_draw_command *commands = malloc(source->count * sizeof(*commands));
    assert(commands || !source->count);
    cui_draw_list paint = *source;
    paint.commands = commands;
    paint.count = 0;
    for (size_t i = 0; i < source->count; ++i)
        if (source->commands[i].type != CUI_DRAW_TEXT)
            commands[paint.count++] = source->commands[i];
    bool ok = cui_software_render(renderer, &paint, (cui_color){255, 255, 255, 255});
    free(commands);
    return ok;
}
static void kinds_and_pixels(void) {
    const cui_bridge_kind kinds[] = {CUI_BRIDGE_ROW,    CUI_BRIDGE_COLUMN, CUI_BRIDGE_LABEL,
                                     CUI_BRIDGE_BUTTON, CUI_BRIDGE_INPUT,  CUI_BRIDGE_BADGE};
    const size_t baseline_bytes = 360 * 240 * 4;
    unsigned char *baseline = malloc(baseline_bytes);
    assert(baseline);
    for (unsigned k = 0; k < 6; ++k) {
        cui_bridge_node handle;
        cui_bridge_app *app = create(kinds[k], &handle);
        cui_software *renderer = cui_software_create(360, 240, NULL);
        assert(renderer);
        cui_draw_list list = render(app, 0);
        size_t commands = list.count, semantics;
        cui_draw_command text[16];
        char text_bytes[16][128];
        size_t text_count = 0;
        for (size_t i = 0; i < list.count; ++i)
            if (list.commands[i].type == CUI_DRAW_TEXT) {
                assert(text_count < 16);
                assert(list.commands[i].text.length < sizeof text_bytes[0]);
                memcpy(text_bytes[text_count], list.commands[i].text.data,
                       list.commands[i].text.length);
                text[text_count++] = list.commands[i];
            }
        assert(text_count > 0);
        (void)cui_semantics(app->ui, &semantics);
        cui_rect before = cui_bounds(app->ui, app->nodes[handle.slot].id);
        assert(before.w > 0 && before.h > 0);
        assert(render_paint(renderer, &list));
        memcpy(baseline, cui_software_pixels(renderer, NULL, NULL), baseline_bytes);
        cui_bridge_box_shadows_v1 value = mixed();
        assert(cui_bridge_set_box_shadows_v1(app, handle, 224, &value) == 0);
        memset(&value, 0, sizeof value); /* Caller storage no longer belongs to C. */
        list = render(app, 0);
        assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 1 &&
               shadows(list, CUI_DRAW_INSET_SHADOW) == 1);
        size_t matched_text = 0;
        for (size_t i = 0; i < list.count; ++i)
            if (list.commands[i].type == CUI_DRAW_TEXT) {
                assert(matched_text < text_count);
                const cui_draw_command *a = &text[matched_text++], *b = &list.commands[i];
                assert(!memcmp(&a->bounds, &b->bounds, sizeof a->bounds));
                assert(a->font_size == b->font_size &&
                       !memcmp(&a->text_style, &b->text_style, sizeof a->text_style));
                assert(a->text.length == b->text.length &&
                       !memcmp(text_bytes[matched_text - 1], b->text.data, a->text.length));
                assert(!memcmp(&a->color, &b->color, sizeof a->color));
                assert(a->text_color_count == b->text_color_count &&
                       a->text_paint.kind == b->text_paint.kind);
            }
        assert(matched_text == text_count);
        cui_rect after = cui_bounds(app->ui, app->nodes[handle.slot].id);
        size_t after_semantics;
        (void)cui_semantics(app->ui, &after_semantics);
        assert(!memcmp(&before, &after, sizeof before) && semantics == after_semantics);
        assert(render_paint(renderer, &list));
        assert(memcmp(baseline, cui_software_pixels(renderer, NULL, NULL), baseline_bytes));
        list = render(app, 1); /* Inert ancestry still paints the override. */
        assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 1 &&
               shadows(list, CUI_DRAW_INSET_SHADOW) == 1);
        list = render(app, 2); /* Layout-only pass cannot mutate a painted box. */
        assert(!shadows(list, CUI_DRAW_DROP_SHADOW) && !shadows(list, CUI_DRAW_INSET_SHADOW));
        list = render(app, 3); /* Omitted tree has no paint even with retained styles. */
        assert(!shadows(list, CUI_DRAW_DROP_SHADOW) &&
               app->nodes[handle.slot].shadows.schema_version == 1);
        list = render(app, 0);
        assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 1);
        if (kinds[k] == CUI_BRIDGE_ROW || kinds[k] == CUI_BRIDGE_COLUMN) {
            uint32_t slot = app->nodes[handle.slot].first_child;
            cui_bridge_node child = {slot, app->nodes[slot].generation};
            value = mixed();
            assert(cui_bridge_set_box_shadows_v1(app, child, 224, &value) == 0);
            list = render(app, 0);
            assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 2 &&
                   shadows(list, CUI_DRAW_INSET_SHADOW) == 2);
            assert(cui_bridge_reset_box_shadows(app, child) == 0);
        }
        value = mixed();
        value.count = 0;
        assert(cui_bridge_set_box_shadows_v1(app, handle, 224, &value) == 0);
        list = render(app, 0);
        assert(!shadows(list, CUI_DRAW_DROP_SHADOW) && !shadows(list, CUI_DRAW_INSET_SHADOW));
        assert(app->nodes[handle.slot].shadows.schema_version == 1);
        assert(cui_bridge_reset_box_shadows(app, handle) == 0);
        list = render(app, 0);
        assert(!app->nodes[handle.slot].shadows.schema_version && list.count == commands);
        assert(render_paint(renderer, &list));
        assert(!memcmp(baseline, cui_software_pixels(renderer, NULL, NULL), baseline_bytes));
        cui_software_destroy(renderer);
        cui_bridge_destroy(app);
    }
    free(baseline);
}
static void validation_and_lifetime(void) {
    cui_bridge_node target;
    cui_bridge_app *app = create(CUI_BRIDGE_BUTTON, &target);
    cui_bridge_box_shadows_v1 value = mixed(), saved;
    value.count = 8;
    for (unsigned i = 2; i < 8; ++i)
        value.layers[i] = value.layers[i % 2];
    assert(cui_bridge_set_box_shadows_v1(app, target, 224, &value) == 0);
    saved = value;
    for (unsigned mode = 0; mode < 7; ++mode) {
        value = saved;
        if (mode == 0)
            value.count = 9;
        if (mode == 1)
            value.layers[0].sigma = NAN;
        if (mode == 2)
            value.layers[0].kind = 2;
        if (mode == 3)
            value.reserved = 1;
        if (mode == 4)
            value.schema_version = 2;
        if (mode == 5)
            value.top_right = INFINITY;
        if (mode == 6)
            value.struct_size = 223;
        assert(cui_bridge_set_box_shadows_v1(app, target, 224, &value) ==
               CUI_BRIDGE_INVALID_ARGUMENT);
        assert(!memcmp(&saved, &app->nodes[target.slot].shadows, sizeof saved));
    }
    assert(cui_bridge_set_box_shadows_v1(app, target, 223, &saved) == CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_node invalid = {999, 999};
    app->stepping = true;
    assert(cui_bridge_set_box_shadows_v1(app, invalid, 0, NULL) == CUI_BRIDGE_INVALID_STATE);
    app->stepping = false;
    assert(cui_bridge_set_box_shadows_v1(app, invalid, 0, NULL) == CUI_BRIDGE_STALE_HANDLE);
    app->run_dispatching = true; /* Existing setters permit host after-frame callbacks. */
    assert(cui_bridge_set_box_shadows_v1(app, target, 224, &saved) == 0);
    app->run_dispatching = false;
    cui_draw_list list = render(app, 0);
    assert(shadows(list, CUI_DRAW_DROP_SHADOW) == 4 && shadows(list, CUI_DRAW_INSET_SHADOW) == 4);
    cui_bridge_node root = {app->root, app->nodes[app->root].generation}, other;
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CHECKBOX, "unsupported", &other) == 0);
    assert(cui_bridge_set_box_shadows_v1(app, other, 0, NULL) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_node_remove(app, target) == 0);
    assert(cui_bridge_set_box_shadows_v1(app, target, 0, NULL) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BADGE, "replacement", &other) == 0);
    assert(other.slot == target.slot && other.generation != target.generation);
    assert(!app->nodes[other.slot].shadows.schema_version);
    list = render(app, 0);
    assert(!shadows(list, CUI_DRAW_DROP_SHADOW));
    cui_bridge_destroy(app);
}
static void factories(void) {
    cui_bridge_box_shadow_v1 layer = {0}, before;
    assert(cui_bridge_box_shadow_from_css_v1(0, 1, 512, -2, 0x12345678, 1, 24, &layer) == 0);
    assert(layer.sigma == 256 && layer.rgba == 0x12345678 && layer.kind == 1);
    before = layer;
    assert(cui_bridge_box_shadow_from_css_v1(0, 0, 513, 0, 0, 0, 24, &layer) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!memcmp(&before, &layer, sizeof layer));
    for (uint32_t p = 0; p <= CUI_SHADOW_2XL; ++p) {
        cui_bridge_box_shadows_v1 value;
        assert(cui_bridge_shadow_preset_v1(p, 1, 2, 3, 4, 224, &value) == 0);
        cui_drop_shadow expected[2];
        size_t count;
        assert(cui_shadow_preset_layers((cui_shadow_preset)p, expected, 2, &count));
        assert(value.count == count && value.bottom_left == 3);
        for (size_t i = 0; i < count; ++i) {
            cui_box_shadow actual = bridge_shadow_layer(value.layers[i]);
            assert(!memcmp(&actual.layer, &expected[i], sizeof expected[i]));
        }
        cui_bridge_box_shadows_v1 old = value;
        assert(cui_bridge_shadow_preset_v1(999, 0, 0, 0, 0, 224, &value) ==
               CUI_BRIDGE_INVALID_ARGUMENT);
        assert(!memcmp(&old, &value, sizeof old));
    }
    assert(cui_bridge_box_shadows_v1_size() == 224);
    assert(offsetof(cui_bridge_box_shadows_v1, layers) == 32);
    for (uint32_t k = 0; k <= 80; ++k)
        assert(cui_bridge_box_shadows_supported_v1(k) ==
               (k == 1 || k == 2 || k == 3 || k == 4 || k == 11 || k == 12));
}
static unsigned measured;
static cui_size measure_guard(void *user, cui_str text, float font) {
    (void)text;
    cui_bridge_app *app = user;
    assert(!cui_frame_paint_enabled(app->ui));
    bridge_apply_shadows(app, &app->nodes[2]);
    measured++;
    return (cui_size){font * 4, font};
}
static void guards_and_input(void) {
    cui_bridge_node target;
    cui_bridge_app *app = create(CUI_BRIDGE_BUTTON, &target);
    cui_bridge_box_shadows_v1 value = mixed();
    assert(cui_bridge_set_box_shadows_v1(app, target, 224, &value) == 0);
    cui_destroy(app->ui);
    app->ui = cui_create((cui_config){.measure = measure_guard, .measure_user = app});
    assert(app->ui);
    render(app, 0);
    assert(measured);
    assert(cui_bridge_step(app, .016f) == 0);
    cui_bridge_rect b;
    assert(cui_bridge_node_bounds(app, target, &b) == 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, b.x + b.width / 2,
                                    b.y + b.height / 2) == 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, b.x + b.width / 2,
                                    b.y + b.height / 2) == 0);
    assert(cui_bridge_step(app, .016f) == 0);
    cui_bridge_event event;
    assert(cui_bridge_poll_event(app, &event) == 0 && event.node.slot == target.slot);
    assert(cui_begin_frame(app->ui, 360, 240, .016f));
    app->frame_error = CUI_BRIDGE_OK;
    bridge_apply_shadows(app, &app->nodes[target.slot]); /* Missing current target is an error. */
    assert(app->frame_error == CUI_BRIDGE_CORE_ERROR &&
           cui_last_error(app->ui) == CUI_ERROR_INVALID_ACTION);
    (void)cui_end_frame(app->ui);
    cui_clear_error(app->ui);
    render(app, 0);
    cui_bridge_destroy(app);
}
int main(void) {
    factories();
    kinds_and_pixels();
    validation_and_lifetime();
    guards_and_input();
    printf("bridge node bytes=%zu; owned shadow field bytes=%zu\n", sizeof(cui_bridge_node_record),
           sizeof(((cui_bridge_node_record *)0)->shadows));
    return 0;
}
