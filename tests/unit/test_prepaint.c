/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_prepaint_info *seen;
    unsigned *calls;
    int mode;
    double marker;
} payload;
static unsigned measure_calls;
static bool capture_control, measure_reenter;
static cui_context *measure_context;
static cui_size measure(void *user, cui_str text, float font, cui_text_style style) {
    (void)user;
    (void)style;
    measure_calls++;
    if (measure_reenter) {
        cui_begin_box(measure_context, 999, (cui_box){0});
    }
    return (cui_size){text.length * font * .5f, font};
}
static bool paint(const cui_prepaint_info *info, const void *copied, cui_prepaint_writer *writer) {
    assert((uintptr_t)copied % _Alignof(max_align_t) == 0);
    const payload *p = copied;
    assert(p->marker == 1234.5);
    *p->seen = *info;
    (*p->calls)++;
    cui_size measured = cui_prepaint_measure(writer, cui_string("rows"), 12, (cui_text_style){0});
    assert(measured.width == 24 && measured.height == 12);
    cui_draw_command c = {
        .type = CUI_DRAW_RECT, .bounds = info->bounds, .color = {11, 22, 33, 200}};
    if (measure_reenter) {
        assert(!cui_prepaint_emit(writer, &c, false));
        return true;
    }
    assert(cui_prepaint_emit(writer, &c, false));
    if (p->mode == 1)
        return false;
    if (p->mode == 2) {
        c.bounds.x = NAN;
        assert(!cui_prepaint_emit(writer, &c, false));
        return true;
    }
    if (p->mode == 3) {
        assert(
            !cui_queue_event(p->ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 1, .y = 1}));
        cui_focus(p->ui, 999);
        cui_begin_box(p->ui, 999, (cui_box){.width = 1, .height = 1});
        assert(!cui_begin_frame(p->ui, 1, 1, .016f));
        assert(cui_end_frame(p->ui).count == 0);
        cui_clear_error(p->ui);
        assert(!cui_create((cui_config){0}));
        cui_destroy(p->ui);
        return true;
    }
    if (p->mode == 4) {
        c.type = CUI_DRAW_TEXT;
        c.bounds = (cui_rect){info->bounds.x, info->bounds.y, 30, 12};
        char word[] = "saved";
        c.text = (cui_str){word, 5};
        c.font_size = 12;
        assert(cui_prepaint_emit(writer, &c, false));
        memset(word, 'x', 5);
    }
    if (p->mode == 5) {
        assert(!cui_prepaint_emit(writer, &c, false));
        return true;
    }
    if (p->mode == 6 && info->modal_allowed) {
        c.bounds = (cui_rect){info->bounds.x + 110, info->bounds.y, 40, 20};
        c.color.r = 44;
        assert(cui_prepaint_emit(writer, &c, true));
    }
    if (p->mode == 9) {
        static const unsigned char pixel[4] = {255, 255, 255, 255};
        static const cui_mesh_vertex triangle[3] = {{0, 0, 0, 0}, {10, 0, 0, 0}, {0, 10, 0, 0}};
        c.type = CUI_DRAW_IMAGE;
        c.bitmap = (cui_bitmap_paint){.bitmap = {pixel, 1, 1, 4, 0}, .opacity = .8f};
        assert(cui_prepaint_emit(writer, &c, false));
        c.type = CUI_DRAW_MESH;
        c.mesh = (cui_mesh_paint){.vertices = triangle,
                                  .vertex_count = 3,
                                  .origin_x = info->bounds.x,
                                  .origin_y = info->bounds.y,
                                  .scale = 2,
                                  .color = {.4f, .2f, .1f, .5f}};
        assert(cui_prepaint_emit(writer, &c, false));
    }
    if (p->mode == 7)
        cui_prepaint_request_frame(writer);
    if (p->mode == 8)
        assert(info->capture == 20 && cui_event_target(p->ui, 0) == 20 &&
               !cui_event_consumed(p->ui, 0) && cui_pressed(p->ui, 20));
    return true;
}
static cui_context *create(size_t commands, size_t bytes) {
    cui_context *ui = cui_create((cui_config){.max_elements = 64,
                                              .max_commands = commands,
                                              .frame_bytes = bytes,
                                              .measure_styled = measure});
    assert(ui);
    return ui;
}
static void owner(cui_context *ui, cui_prepaint_info *info, unsigned *calls, int mode,
                  size_t budget, int16_t layer, bool duplicate) {
    cui_begin_box(ui, 20,
                  (cui_box){.width = 100,
                            .height = 40,
                            .custom_opacity = true,
                            .opacity = .5f,
                            .background = {4, 0, 0, 128}});
    assert(cui_set_underlay_ex(ui, (cui_underlay_layout){.grow_width = true, .grow_height = true},
                               (cui_color){5, 0, 0, 255}, (cui_corner_radii){0}));
    cui_interact(ui, 20, capture_control ? CUI_ROLE_BUTTON : CUI_ROLE_IMAGE, cui_string("Plot"),
                 false, false, 0);
    payload p = {ui, info, calls, mode, 1234.5};
    cui_prepaint_options o = {.callback = paint,
                              .payload = &p,
                              .payload_bytes = sizeof p,
                              .command_budget = budget,
                              .hover_hitbox = true,
                              .deferred_layer = layer};
    assert(cui_set_prepaint(ui, o));
    p.marker = 0;
    p.mode = 99;
    if (duplicate)
        assert(!cui_set_prepaint(ui, o));
    cui_begin_box(ui, 21, (cui_box){.width = 10, .height = 10, .background = {70, 0, 0, 255}});
    cui_end_box(ui);
    cui_end_box(ui);
}
static size_t color_index(cui_draw_list draw, uint8_t red) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == red)
            return i;
    return SIZE_MAX;
}
static void balanced(cui_draw_list draw) {
    unsigned depth = 0;
    for (size_t i = 0; i < draw.count; i++) {
        if (draw.commands[i].type == CUI_DRAW_CLIP_PUSH)
            depth++;
        if (draw.commands[i].type == CUI_DRAW_CLIP_POP) {
            assert(depth);
            depth--;
        }
    }
    assert(!depth);
}
int main(void) {
    cui_context *ui = create(128, 4096);
    cui_prepaint_info info = {0};
    unsigned calls = 0;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 15, .y = 35}));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 300, .height = 200, .custom_opacity = true, .opacity = .5f});
    cui_begin_box(ui, 2, (cui_box){.width = 100, .height = 20});
    cui_end_box(ui);
    cui_begin_box(ui, 3, (cui_box){.width = 80, .height = 60, .clip = true});
    owner(ui, &info, &calls, 4, 5, 0, false);
    assert(calls == 0);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list d = cui_end_frame(ui);
    balanced(d);
    assert(calls == 1 && measure_calls == 1 && info.bounds.y == 20 && info.bounds.w == 100);
    assert(info.visible_bounds.w == 80 && info.pointer_visible && info.pointer_hit == 20 &&
           info.opacity == .25f);
    size_t local = color_index(d, 11), child = color_index(d, 70);
    assert(color_index(d, 4) < color_index(d, 5) && color_index(d, 5) < local && local < child &&
           d.commands[local].color.a == 50);
    unsigned backgrounds = 0;
    for (size_t i = 0; i < d.count; i++)
        backgrounds += d.commands[i].type == CUI_DRAW_RECT && d.commands[i].color.r == 4;
    assert(backgrounds == 1);
    bool saved = false;
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_TEXT) {
            assert(d.commands[i].text.length == 5 && !memcmp(d.commands[i].text.data, "saved", 5));
            saved = true;
        }
    assert(saved);
    /* Stationary pointer, new origin and passive overlay: all current frame. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 300, .height = 200});
    cui_begin_box(ui, 2, (cui_box){.width = 100, .height = 60});
    cui_end_box(ui);
    owner(ui, &info, &calls, 0, 3, 0, false);
    cui_end_box(ui);
    d = cui_end_frame(ui);
    assert(info.bounds.y == 60 && !info.pointer_inside && !info.pointer_visible && calls == 2);
    balanced(d);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 15, .y = 15}));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 6, 6, 5, false);
    cui_begin_portal(ui, 30, (cui_box){.width = 30, .height = 30, .background = {99, 0, 0, 255}},
                     (cui_rect){10, 10, 30, 30}, 10);
    assert(cui_set_hover_hitbox(ui, true));
    cui_end_box(ui);
    d = cui_end_frame(ui);
    assert(!info.pointer_visible && info.pointer_hit == 30);
    assert(color_index(d, 11) < color_index(d, 70) && color_index(d, 70) < color_index(d, 44) &&
           color_index(d, 44) < color_index(d, 99));
    balanced(d);
    /* Removing the barrier restores stationary-pointer hover. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 0, 3, 0, false);
    d = cui_end_frame(ui);
    assert(info.pointer_visible);
    balanced(d);
    /* Current modal denies the owner, even before a completed modal frame. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 6, 6, 5, false);
    assert(cui_begin_focus_scope(ui, 40, true));
    cui_begin_portal(ui, 41, (cui_box){.width = 30, .height = 30, .background = {99, 0, 0, 255}},
                     (cui_rect){200, 100, 30, 30}, 20);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    d = cui_end_frame(ui);
    assert(!info.modal_allowed && !info.pointer_visible && color_index(d, 44) == SIZE_MAX);
    balanced(d);
    cui_destroy(ui);
    for (int mode = 1; mode <= 3; mode++) {
        ui = create(64, 4096);
        assert(cui_begin_frame(ui, 300, 200, .016f));
        owner(ui, &info, &calls, mode, 6, 0, false);
        d = cui_end_frame(ui);
        assert(color_index(d, 11) == SIZE_MAX && color_index(d, 70) != SIZE_MAX);
        assert(cui_last_error(ui) == (mode == 3 ? CUI_ERROR_LIFECYCLE : CUI_ERROR_LAYOUT));
        balanced(d);
        cui_destroy(ui);
    }
    ui = create(64, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 0, 3, 0, true);
    d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_DUPLICATE_ID && color_index(d, 11) != SIZE_MAX);
    cui_destroy(ui);
    ui = create(64, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 5, 3, 0, false);
    d = cui_end_frame(ui);
    assert(d.count == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
    ui = create(3, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 0, 3, 0, false);
    d = cui_end_frame(ui);
    assert(d.count == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
    ui = create(64, 32);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 20, (cui_box){.width = 100, .height = 40});
    payload p = {ui, &info, &calls, 0, 1234.5};
    assert(!cui_set_prepaint(
        ui, (cui_prepaint_options){
                .callback = paint, .payload = &p, .payload_bytes = sizeof p, .command_budget = 3}));
    cui_end_box(ui);
    d = cui_end_frame(ui);
    assert(d.count == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
    /* Negative deferred layers are invalid: existing owner layers are >=0. */
    ui = create(128, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 60, (cui_box){.width = 100, .height = 40});
    p = (payload){ui, &info, &calls, 6, 1234.5};
    cui_prepaint_options layer_options = {.callback = paint,
                                          .payload = &p,
                                          .payload_bytes = sizeof p,
                                          .command_budget = 6,
                                          .deferred_layer = INT16_MIN};
    assert(!cui_set_prepaint(ui, layer_options));
    layer_options.deferred_layer = -1;
    assert(!cui_set_prepaint(ui, layer_options));
    layer_options.deferred_layer = INT16_MAX;
    assert(cui_set_prepaint(ui, layer_options));
    cui_end_box(ui);
    d = cui_end_frame(ui);
    balanced(d);
    assert(cui_last_error(ui) == CUI_OK && color_index(d, 44) != SIZE_MAX);
    cui_destroy(ui);
    ui = create(128, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 9, 5, 0, false);
    d = cui_end_frame(ui);
    bool image_seen = false, mesh_seen = false;
    for (size_t i = 0; i < d.count; i++) {
        if (d.commands[i].type == CUI_DRAW_IMAGE) {
            image_seen = true;
            assert(fabsf(d.commands[i].bitmap.opacity - .4f) < .0001f);
        }
        if (d.commands[i].type == CUI_DRAW_MESH) {
            mesh_seen = true;
            assert(d.commands[i].bounds.w == 20 && d.commands[i].bounds.h == 20 &&
                   d.commands[i].mesh.color.a == .25f);
        }
    }
    assert(image_seen && mesh_seen);
    balanced(d);
    cui_destroy(ui);
    /* A positive layer preceding a portal owner is also rejected atomically. */
    ui = create(128, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_portal(ui, 60, (cui_box){.width = 100, .height = 40}, (cui_rect){0, 0, 100, 40}, 10);
    p = (payload){ui, &info, &calls, 6, 1234.5};
    layer_options.payload = &p;
    layer_options.deferred_layer = 5;
    assert(!cui_set_prepaint(ui, layer_options));
    layer_options.deferred_layer = 0;
    assert(cui_set_prepaint(ui, layer_options));
    cui_end_box(ui);
    d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && color_index(d, 44) != SIZE_MAX);
    balanced(d);
    cui_destroy(ui);
    /* Two callbacks observe one frozen hit graph, including a culled owner. */
    ui = create(128, 4096);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 15, .y = 15}));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 0, 3, 0, false);
    cui_prepaint_info other = {0};
    unsigned other_calls = 0;
    cui_begin_portal(ui, 60, (cui_box){.width = 30, .height = 30}, (cui_rect){0, 0, 30, 30}, 10);
    p = (payload){ui, &other, &other_calls, 0, 1234.5};
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint,
                                                       .payload = &p,
                                                       .payload_bytes = sizeof p,
                                                       .command_budget = 3,
                                                       .hover_hitbox = true}));
    cui_end_box(ui);
    d = cui_end_frame(ui);
    assert(info.pointer_hit == 60 && other.pointer_hit == 60 && !info.pointer_visible &&
           other.pointer_visible && other_calls == 1);
    balanced(d);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_portal(ui, 60, (cui_box){.width = 30, .height = 30}, (cui_rect){500, 500, 30, 30},
                     10);
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint,
                                                       .payload = &p,
                                                       .payload_bytes = sizeof p,
                                                       .command_budget = 3,
                                                       .hover_hitbox = true}));
    cui_end_box(ui);
    d = cui_end_frame(ui);
    assert(other_calls == 2 && !other.pointer_inside && other.visible_bounds.w == 0);
    balanced(d);
    cui_destroy(ui);
    /* Existing capture/event targets survive a current-frame geometry change. */
    capture_control = true;
    ui = create(128, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 0, 3, 0, false);
    cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 15, .y = 15}));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 300, .height = 200});
    cui_begin_box(ui, 2, (cui_box){.height = 80, .width = 100});
    cui_end_box(ui);
    owner(ui, &info, &calls, 8, 3, 0, false);
    cui_end_box(ui);
    d = cui_end_frame(ui);
    assert(cui_pressed(ui, 20) && info.capture == 20 && !info.pointer_inside);
    balanced(d);
    cui_destroy(ui);
    capture_control = false;
    /* Measurement hooks execute within the same mutation guard and rollback. */
    ui = create(128, 4096);
    measure_context = ui;
    measure_reenter = true;
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 0, 3, 0, false);
    d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_LIFECYCLE && color_index(d, 11) == SIZE_MAX);
    measure_reenter = false;
    cui_destroy(ui);
    ui = create(128, 4096);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    owner(ui, &info, &calls, 7, 3, 0, false);
    cui_end_frame(ui);
    assert(cui_needs_redraw(ui));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 10, (cui_box){.width = 100, .height = 100, .layout_only = true});
    unsigned before = calls;
    owner(ui, &info, &calls, 0, 3, 0, false);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(calls == before);
    cui_destroy(ui);
    puts("prepaint: ok");
}
