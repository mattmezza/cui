/* SPDX-License-Identifier: MIT */
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include "cui/cui_software.h"
#include <assert.h>

static cui_bridge_node create(cui_bridge_app *a, cui_bridge_node p, cui_bridge_kind k,
                              const char *key) {
    cui_bridge_node n;
    assert(cui_bridge_node_create(a, p, k, key, &n) == CUI_BRIDGE_OK);
    return n;
}
static cui_draw_list paint(cui_bridge_app *a) {
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    assert(cui_begin_frame(a->ui, 800, 800, .016f));
    cui_begin_box(a->ui, 1, (cui_box){.width = -1, .height = -1});
    bridge_render_node(a, a->root);
    cui_end_box(a->ui);
    cui_draw_list list = cui_end_frame(a->ui);
    assert(cui_last_error(a->ui) == CUI_OK);
    return list;
}
static unsigned drain(cui_bridge_app *a) {
    unsigned count = 0;
    cui_bridge_event_ex e;
    char text[8];
    uint32_t required;
    while (cui_bridge_poll_event_ex(a, &e, text, sizeof text, &required) == CUI_BRIDGE_OK) {
        assert(e.type == CUI_BRIDGE_EVENT_CHANGE);
        count++;
    }
    return count;
}
static void click(cui_bridge_app *a, cui_bridge_node n, unsigned part) {
    cui_bridge_rect r;
    assert(cui_bridge_numeric_part_bounds(a, n, part, &r) == CUI_BRIDGE_OK);
    assert(r.width > 0 && r.height > 0);
    assert(cui_bridge_queue_pointer(a, CUI_BRIDGE_POINTER_DOWN, r.x + r.width / 2,
                                    r.y + r.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(a, CUI_BRIDGE_POINTER_UP, r.x + r.width / 2,
                                    r.y + r.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
}
int main(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .width = 800,
                            .height = 800};
    cui_bridge_status status;
    cui_bridge_app *a = cui_bridge_create(&o, &status);
    assert(a && status == CUI_BRIDGE_OK);
    cui_bridge_node root = create(a, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node steps = create(a, root, CUI_BRIDGE_STEPPER, "steps");
    cui_bridge_node first = create(a, steps, CUI_BRIDGE_ITEM, "one");
    cui_bridge_node second = create(a, steps, CUI_BRIDGE_ITEM, "two");
    assert(cui_bridge_set_text(a, first, "First") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(a, second, "Second") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_selected(a, steps, first) == CUI_BRIDGE_OK);
    (void)paint(a);
    cui_bridge_rect legacy_bounds;
    assert(cui_bridge_node_bounds(a, steps, &legacy_bounds) == CUI_BRIDGE_OK);
    click(a, steps, 2);
    assert(drain(a) == 1);
    click(a, steps, 2);
    assert(drain(a) == 0);
    assert(cui_bridge_set_stepper_visual(a, root, 1, 0, 0) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_stepper_visual(a, steps, 2, 0, 0) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_stepper_visual(a, steps, 1, 0, 4) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_design_size(a, steps, CUI_BRIDGE_SIZE_LG) == CUI_BRIDGE_OK);
    uint8_t pixels[16] = {255, 255, 255, 255, 255, 255, 255, 255,
                          255, 255, 255, 255, 255, 255, 255, 255};
    assert(cui_bridge_set_image_rgba(a, first, pixels, 2, 2, 8, 16) == CUI_BRIDGE_OK);
    unsigned char *owned = a->nodes[first.slot].image_pixels;
    pixels[0] = 0;
    assert(owned[0] == 255);
    assert(cui_bridge_set_image_rgba(a, first, pixels, 2, 2, UINT32_MAX, 16) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(a->nodes[first.slot].image_pixels == owned && owned[0] == 255);
    assert(cui_bridge_set_image_rgba(a, root, pixels, 2, 2, 8, 16) == CUI_BRIDGE_INVALID_STATE);
    for (unsigned density = 0; density < 4; density++) {
        assert(cui_bridge_set_stepper_visual(a, steps, 1, 0, density) == CUI_BRIDGE_OK);
        cui_draw_list list = paint(a);
        unsigned bitmaps = 0;
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].type == CUI_DRAW_IMAGE)
                bitmaps++;
        assert(bitmaps == (density == 1 ? 0u : 1u));
        cui_software *renderer = cui_software_create(800, 800, NULL);
        assert(renderer);
        /* Glyph rasterization is outside this fixture; retain every clip,
         * bitmap and decoration command for the software pixel assertions. */
        cui_draw_command *commands = malloc(list.count * sizeof *commands);
        assert(commands);
        cui_draw_list visual = list;
        visual.commands = commands;
        visual.count = 0;
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].type != CUI_DRAW_TEXT)
                commands[visual.count++] = list.commands[i];
        assert(cui_software_render(renderer, &visual, (cui_color){0, 0, 0, 255}));
        unsigned w, h;
        const unsigned char *out = cui_software_pixels(renderer, &w, &h);
        size_t lit = 0;
        for (size_t i = 0; i < (size_t)w * h; i++)
            if (out[i * 4] || out[i * 4 + 1] || out[i * 4 + 2])
                lit++;
        assert(lit > 50);
        for (size_t i = 0; i < visual.count; i++)
            if (commands[i].type == CUI_DRAW_IMAGE) {
                assert(commands[i].bitmap.bitmap.pixels == a->nodes[first.slot].image_pixels);
                unsigned x = (unsigned)(commands[i].bounds.x + commands[i].bounds.w / 2);
                unsigned y = (unsigned)(commands[i].bounds.y + commands[i].bounds.h / 2);
                const unsigned char *pixel = out + ((size_t)y * w + x) * 4;
                assert(pixel[0] == a->theme.on_primary.r && pixel[1] == a->theme.on_primary.g &&
                       pixel[2] == a->theme.on_primary.b && pixel[3] == 255);
            }
        cui_software_destroy(renderer);
        free(commands);
    }
    assert(cui_bridge_set_stepper_visual(a, steps, 1, 1, 0) == CUI_BRIDGE_OK);
    (void)paint(a);
    cui_bridge_rect r1, r2;
    assert(cui_bridge_numeric_part_bounds(a, steps, 1, &r1) == CUI_BRIDGE_OK);
    assert(cui_bridge_numeric_part_bounds(a, steps, 2, &r2) == CUI_BRIDGE_OK);
    assert(r2.y > r1.y && r2.x == r1.x);
    click(a, steps, 1);
    /* The changed selection is queued after paint; this frame's semantics and
     * decoration still describe the selected snapshot (the second Item). */
    size_t semantic_count;
    const cui_semantic *semantics = cui_semantics(a->ui, &semantic_count);
    bool saw_second = false;
    for (size_t i = 0; i < semantic_count; i++)
        if (semantics[i].id == cui_numeric_part_id(a->nodes[steps.slot].id, 2)) {
            assert(semantics[i].details.selected);
            saw_second = true;
        }
    assert(saw_second);
    assert(drain(a) == 1);
    click(a, steps, 1);
    assert(drain(a) == 0);
    assert(cui_bridge_set_disabled(a, second, 1) == CUI_BRIDGE_OK);
    (void)paint(a);
    click(a, steps, 2);
    assert(drain(a) == 0);
    assert(cui_bridge_set_disabled(a, steps, 1) == CUI_BRIDGE_OK);
    (void)paint(a);
    click(a, steps, 2);
    assert(drain(a) == 0);
    assert(cui_bridge_set_disabled(a, steps, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_disabled(a, second, 0) == CUI_BRIDGE_OK);
    (void)paint(a);
    cui_focus(a->ui, cui_numeric_part_id(a->nodes[steps.slot].id, 1));
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_TAB, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_TAB, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ENTER, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_ENTER, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    assert(drain(a) == 1);
    cui_bridge_node selected;
    assert(cui_bridge_get_selected(a, steps, &selected) == CUI_BRIDGE_OK &&
           selected.slot == second.slot);
    assert(cui_bridge_set_image_rgba(a, first, pixels, 2, 2, 8, 16) == CUI_BRIDGE_OK);
    assert(a->nodes[first.slot].image_frame.revision == 2);
    assert(cui_bridge_set_stepper_visual(a, steps, 0, 1, 3) == CUI_BRIDGE_OK);
    cui_draw_list legacy = paint(a);
    for (size_t i = 0; i < legacy.count; i++)
        assert(legacy.commands[i].type != CUI_DRAW_IMAGE);
    cui_bridge_rect restored;
    assert(cui_bridge_node_bounds(a, steps, &restored) == CUI_BRIDGE_OK);
    assert(restored.width == legacy_bounds.width && restored.height == legacy_bounds.height);
    assert(cui_bridge_node_remove(a, first) == CUI_BRIDGE_OK);
    assert(a->nodes[first.slot].image_pixels == NULL);
    cui_bridge_node menu = create(a, root, CUI_BRIDGE_MENU, "menu");
    cui_bridge_node wrong = create(a, menu, CUI_BRIDGE_ITEM, "wrong");
    assert(cui_bridge_set_image_rgba(a, wrong, pixels, 2, 2, 8, 16) == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_destroy(a);
    return 0;
}
