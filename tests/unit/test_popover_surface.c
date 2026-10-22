/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "cui/cui_surface.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static const cui_draw_command *mesh(cui_draw_list list, size_t which) {
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_MESH && which-- == 0)
            return &list.commands[i];
    return NULL;
}
static cui_surface_options options(void) {
    return (cui_surface_options){.viewport = {0, 0, 400, 320},
                                 .anchor = {180, 140, 40, 20},
                                 .width = 120,
                                 .height = 100,
                                 .hide_close = true,
                                 .popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE};
}
static cui_draw_list scene(cui_context *ui, cui_theme *t, cui_surface_state *s,
                           cui_surface_options o, cui_popover_position p, float opacity) {
    assert(cui_begin_frame(ui, 400, 320, .016f));
    cui_begin_box(
        ui, 990,
        (cui_box){.width = 400, .height = 320, .custom_opacity = true, .opacity = opacity});
    if (cui_popover_begin(ui, t, s, (cui_str){0}, (cui_str){0}, o, p)) {
        cui_begin_box(ui, 989,
                      (cui_box){.width = 10, .height = 10, .background = {0, 0, 255, 255}});
        cui_end_box(ui);
        cui_surface_end(ui);
    }
    cui_begin_portal(ui, 991, (cui_box){.background = {0, 255, 0, 255}}, (cui_rect){350, 0, 10, 10},
                     110);
    cui_end_box(ui);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
static void smash_stack(void) {
    volatile unsigned char scratch[8192];
    for (size_t i = 0; i < sizeof scratch; ++i)
        scratch[i] = (unsigned char)i;
}
static void check_geometry(void) {
    cui_theme t = cui_theme_light();
    t.radius = 9;
    t.popover = (cui_color){255, 0, 0, 255};
    cui_surface_options o = options();
    /* Independent source live-anchor goldens with offset4+depth6. */
    const cui_rect expected[8] = {{180, 170, 120, 100}, {140, 170, 120, 100}, {100, 170, 120, 100},
                                  {180, 30, 120, 100},  {140, 30, 120, 100},  {100, 30, 120, 100},
                                  {230, 100, 120, 100}, {50, 100, 120, 100}};
    for (unsigned anchor = 0; anchor < 8; ++anchor) {
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 320});
        assert(ui);
        cui_surface_state s;
        cui_surface_init(&s, 100, CUI_SURFACE_POPOVER);
        cui_surface_open(ui, &s, 0);
        cui_popover_position p = {.mode = CUI_POPOVER_POSITION_ANCHOR,
                                  .anchor = (cui_popover_anchor)anchor,
                                  .arrow = true};
        cui_draw_list list = scene(ui, &t, &s, o, p, 1);
        assert(list.count && cui_last_error(ui) == CUI_OK);
        cui_rect b = cui_bounds(ui, 100);
        assert(!memcmp(&b, &expected[anchor], sizeof b));
        const cui_draw_command *fill = mesh(list, 0), *stroke = mesh(list, 1);
        assert(fill && stroke && !mesh(list, 2));
        assert(fill->mesh.vertex_count == 3 && stroke->mesh.vertex_count == 12);
        assert(fill->mesh.color.r == 1 && fill->mesh.color.a == 1);
        assert(fabsf(stroke->mesh.color.a - 26 / 255.0f) < .0001f);
        const cui_mesh_vertex *v = fill->mesh.vertices;
        /* Source inset uses theme9+depth6=15, targeting named corner, not center. */
        if (anchor < 6) {
            float center = anchor % 3 == 0 ? b.x + 15 : anchor % 3 == 1 ? 200 : b.x + b.w - 15;
            bool found = false;
            for (unsigned i = 0; i < 3; ++i)
                found |= v[i].x == center && v[i].y == (anchor < 3 ? b.y - 6 : b.y + b.h + 6);
            assert(found);
        } else {
            bool found = false;
            for (unsigned i = 0; i < 3; ++i)
                found |= v[i].y == 150 && v[i].x == (anchor == 6 ? b.x - 6 : b.x + b.w + 6);
            assert(found);
        }
        /* Deferred decoration follows panel ring/background/children, with join
         * between fill and slope stroke and a Core host-viewport clip wrapper. */
        size_t index = (size_t)(fill - list.commands);
        assert(index > 0 && list.commands[index - 1].type == CUI_DRAW_CLIP_PUSH);
        bool child_before = false, later_after = false;
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *command = &list.commands[i];
            if (command->type == CUI_DRAW_RECT && command->color.b == 255 && command->color.r == 0)
                child_before |= i < index;
            if (command->type == CUI_DRAW_RECT && command->color.g == 255 && command->color.r == 0)
                later_after |= i > index;
        }
        assert(child_before && later_after);
        assert(list.commands[index + 1].type == CUI_DRAW_RECT);
        assert(list.commands[index + 2].type == CUI_DRAW_MESH);
        assert(list.commands[index + 3].type == CUI_DRAW_CLIP_POP);
        assert(list.commands[index + 1].bounds.w == (anchor < 6 ? 10 : 2));
        assert(list.commands[index + 1].bounds.h == (anchor < 6 ? 2 : 10));
        size_t semantic_count;
        const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
        assert(semantics && semantic_count == 3); /* panel,title,body; no arrow */
        assert(s.overlay.bounds.x == b.x && s.overlay.bounds.w == b.w);
        /* Payload vertices survive return, unrelated stack churn and rendering. */
        cui_mesh_vertex saved[3];
        memcpy(saved, v, sizeof saved);
        smash_stack();
        assert(!memcmp(saved, v, sizeof saved));
        if (anchor == 0) {
            cui_software *soft = cui_software_create(400, 320, NULL);
            assert(soft);
            assert(cui_software_render(soft, &list, (cui_color){255, 255, 255, 255}));
            unsigned width, height;
            const unsigned char *pixels = cui_software_pixels(soft, &width, &height);
            /* Base join center straddles source ring; both pixels remain red. */
            for (unsigned y = 169; y <= 170; ++y) {
                const unsigned char *pixel = pixels + ((size_t)y * width + 195) * 4;
                assert(pixel[0] > 245 && pixel[1] < 10 && pixel[2] < 10);
            }
            cui_software_destroy(soft);
        }
        o.anchor.x += 7;
        list = scene(ui, &t, &s, o, p, .5f);
        fill = mesh(list, 0);
        assert(fill);
        assert(fabsf(fill->mesh.color.a - .5f) < .0001f);
        assert(cui_bounds(ui, 100).x == b.x + 7);
        assert(list.commands[(fill - list.commands) + 1].color.a == 127);
        o.anchor.x -= 7;
        cui_destroy(ui);
    }
}
static void check_refinements_and_clip(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_theme t = cui_theme_light();
    t.popover = (cui_color){255, 0, 0, 255};
    t.radius = 10;
    cui_surface_state s;
    cui_surface_init(&s, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &s, 0);
    cui_popover_position p = {.mode = CUI_POPOVER_POSITION_ANCHOR, .arrow = true};
    cui_surface_options o = options();
    o.popover_appearance = CUI_POPOVER_APPEARANCE_NONE;
    cui_draw_list list = scene(ui, &t, &s, o, p, 1);
    assert(mesh(list, 0) && !mesh(list, 1) && mesh(list, 0)->mesh.color.r == 1);
    o.popover_refinement =
        (cui_popover_refinement){.flags = CUI_POPOVER_REFINE_BACKGROUND | CUI_POPOVER_REFINE_RADIUS,
                                 .background = {0, 0, 255, 255},
                                 .radius = 0};
    list = scene(ui, &t, &s, o, p, 1);
    assert(mesh(list, 0)->mesh.color.b == 1);
    bool source_inset = false;
    for (size_t i = 0; i < mesh(list, 0)->mesh.vertex_count; ++i)
        source_inset |=
            mesh(list, 0)->mesh.vertices[i].x == 196 && mesh(list, 0)->mesh.vertices[i].y == 164;
    assert(source_inset);
    /* Clamping can place an arrow outside usable viewport. Clip actual vertices
     * and join in options viewport, independent of Core's wider host clip. */
    o.viewport = (cui_rect){40, 40, 80, 80};
    o.anchor = (cui_rect){80, 100, 20, 20};
    o.width = 100;
    o.height = 100;
    o.margin = 4;
    o.popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE;
    list = scene(ui, &t, &s, o, p, 1);
    assert(mesh(list, 0) && mesh(list, 1)); /* genuinely clipped fill and outline */
    cui_rect oversized = cui_bounds(ui, 100);
    assert(oversized.x == 44 && oversized.y == 44 && oversized.w == 100 && oversized.h == 100);
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_MESH)
            for (size_t j = 0; j < list.commands[i].mesh.vertex_count; ++j) {
                cui_mesh_vertex v = list.commands[i].mesh.vertices[j];
                assert(v.x >= 40 && v.x <= 120 && v.y >= 40 && v.y <= 120);
            }
    cui_rect untouched = {1, 2, 3, 4}, out = untouched;
    p.reference_rem = NAN;
    assert(!cui_popover_surface_geometry(o, p, &out));
    assert(!memcmp(&out, &untouched, sizeof out));
    assert(cui_begin_frame(ui, 400, 320, .016f));
    cui_surface_state snapshot = s;
    assert(!cui_popover_begin(ui, &t, &s, (cui_str){0}, (cui_str){0}, o, p));
    assert(!memcmp(&s, &snapshot, sizeof s));
    assert(cui_end_frame(ui).count == 0);
    cui_destroy(ui);
}
static void check_lifetime_and_failure(void) {
    cui_theme t = cui_theme_light();
    cui_surface_options o = options();
    cui_popover_position p = {.mode = CUI_POPOVER_POSITION_ANCHOR, .arrow = true};
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_surface_state a, b;
    cui_surface_init(&a, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &a, 0);
    cui_surface_init(&b, 200, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &b, 0);
    assert(cui_begin_frame(ui, 400, 320, .016f));
    assert(cui_popover_begin(ui, &t, &a, (cui_str){0}, (cui_str){0}, o, p));
    o.anchor.x = 20;
    o.layer = 102;
    assert(cui_popover_begin(ui, &t, &b, (cui_str){0}, (cui_str){0}, o, p));
    cui_surface_end(ui);
    cui_surface_end(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(mesh(list, 3));
    const cui_draw_command *first = mesh(list, 0), *second = mesh(list, 2);
    assert(first->mesh.vertices != second->mesh.vertices);
    assert(first->mesh.vertices[0].x > second->mesh.vertices[0].x);
    smash_stack();
    assert(cui_mesh_valid(first->mesh) && cui_mesh_valid(second->mesh));
    cui_destroy(ui);
    /* Deliberately too little arena for copied geometry. Registration rolls
     * back and end-frame emits nothing, then a non-arrow frame recovers. */
    ui = cui_create((cui_config){.frame_bytes = 512});
    assert(ui);
    cui_surface_init(&a, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &a, 0);
    o = options();
    assert(cui_begin_frame(ui, 400, 320, .016f));
    assert(!cui_popover_begin(ui, &t, &a, (cui_str){0}, (cui_str){0}, o, p));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    assert(cui_end_frame(ui).count == 0);
    cui_clear_error(ui);
    p.arrow = false;
    list = scene(ui, &t, &a, o, p, 1);
    assert(list.count && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    /* A later payload allocation failure must suppress the earlier valid arrow
     * too, rather than exposing a partial frame with stale borrowed vertices. */
    ui = cui_create((cui_config){.frame_bytes = 2000});
    assert(ui);
    cui_surface_init(&a, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &a, 0);
    cui_surface_init(&b, 200, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &b, 0);
    p.arrow = true;
    assert(cui_begin_frame(ui, 400, 320, .016f));
    assert(cui_popover_begin(ui, &t, &a, (cui_str){0}, (cui_str){0}, o, p));
    o.layer = 102;
    assert(!cui_popover_begin(ui, &t, &b, (cui_str){0}, (cui_str){0}, o, p));
    cui_surface_end(ui);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY && cui_end_frame(ui).count == 0);
    cui_clear_error(ui);
    p.arrow = false;
    list = scene(ui, &t, &a, o, p, 1);
    assert(list.count && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void check_dismissal(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_theme t = cui_theme_light();
    cui_surface_options o = options();
    cui_surface_state s;
    cui_surface_init(&s, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &s, 0);
    cui_popover_position p = {.mode = CUI_POPOVER_POSITION_ANCHOR, .arrow = true};
    scene(ui, &t, &s, o, p, 1);
    assert(s.overlay.open);
    /* Arrow tip is painted outside panel. Existing panel-only outside-dismiss
     * contract remains active; decoration does not create a semantic hit target. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = 195,
                                           .y = 166,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    scene(ui, &t, &s, o, p, 1);
    assert(!s.overlay.open);
    cui_destroy(ui);
}
static void check_legacy(void) {
    cui_theme t = cui_theme_light();
    cui_surface_options o = options();
    o.anchor.y = 310;
    o.side = CUI_POPUP_BELOW;
    cui_popover_position p = {.mode = CUI_POPOVER_POSITION_LEGACY,
                              .anchor = (cui_popover_anchor)99,
                              .reference_rem = NAN,
                              .offset = NAN,
                              .custom_offset = true,
                              .arrow = true};
    cui_rect expected = cui_surface_geometry(CUI_SURFACE_POPOVER, o), out;
    assert(cui_popover_surface_geometry(o, p, &out) && !memcmp(&out, &expected, sizeof out));
    /* Independent legacy oracle: gap6, opposite-side flip, dimensions retained. */
    assert(out.x == 180 && out.y == 204 && out.w == 120 && out.h == 100);
    cui_surface_options bad = o;
    bad.anchor.x = NAN;
    cui_rect saved = out;
    assert(!cui_popover_surface_geometry(bad, p, &out));
    assert(!memcmp(&saved, &out, sizeof out));
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_surface_state s;
    cui_surface_init(&s, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &s, 0);
    cui_draw_list list = scene(ui, &t, &s, o, p, 1);
    assert(list.count && !mesh(list, 0));
    cui_rect bounds = cui_bounds(ui, 100);
    assert(!memcmp(&bounds, &expected, sizeof bounds));
    cui_surface_init(&s, 101, CUI_SURFACE_DIALOG);
    assert(cui_begin_frame(ui, 400, 320, .016f));
    assert(!cui_popover_begin(ui, &t, &s, (cui_str){0}, (cui_str){0}, o, p));
    cui_end_frame(ui);
    cui_destroy(ui);
}
int main(void) {
    check_geometry();
    check_refinements_and_clip();
    check_lifetime_and_failure();
    check_dismissal();
    check_legacy();
    return 0;
}
