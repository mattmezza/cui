/* SPDX-License-Identifier: MIT */
/* Production Core layout + queued UI input; independent frozen math oracle. */
#include "cui/cui_scroll.h"
#include "cui/cui_software.h"
#include "scrollbar_track_probe.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum {
    PANEL = 100,
    OUTER = 101,
    COLUMN = 102,
    FRAME = 103,
    BODY = 104,
    CONTENT = 105,
    OUTER_THUMB = 106,
    BODY_THUMB = 107
};
typedef struct {
    cui_context *ui;
    cui_rect viewport[2], content[2], track[2], thumb[2];
    cui_scroll_state state[2];
    float width, height, header, body, clip_height;
    cui_id prepaint_hit;
    uint32_t flags;
    unsigned compositions, hooks;
    bool shadows, inert, layout_only, nested, owner_disabled, owner_readonly, ancestor_disabled,
        cover_modal, placeholder;
    cui_scroll_state child;
} fixture;
static bool near(float a, float b) {
    return fabsf(a - b) < .06f;
}
static void rect_equal(cui_rect a, cui_rect b) {
    if (!near(a.x, b.x) || !near(a.y, b.y) || !near(a.w, b.w) || !near(a.h, b.h)) {
        fprintf(stderr, "rect %.3f %.3f %.3f %.3f != %.3f %.3f %.3f %.3f\n", a.x, a.y, a.w, a.h,
                b.x, b.y, b.w, b.h);
        assert(false);
    }
}
static cui_id owner(unsigned index) {
    return index ? BODY : OUTER;
}
static cui_id thumb_id(unsigned index) {
    return index ? BODY_THUMB : OUTER_THUMB;
}
static cui_color track_color(unsigned i) {
    return (cui_color){(uint8_t)(71 + i), 81, 91, 200};
}
static cui_color thumb_color(unsigned i) {
    return (cui_color){(uint8_t)(73 + i), 83, 93, 200};
}
static const cui_semantic *semantic(cui_context *ui, cui_id id) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].id == id)
            return &nodes[i];
    return NULL;
}
static cui_scroll_options options(fixture *f, unsigned i) {
    return (cui_scroll_options){.width = f->viewport[i].w,
                                .height = f->viewport[i].h,
                                .content_width = f->viewport[i].w,
                                .content_height = f->content[i].h,
                                .vertical = true,
                                .hide_scrollbars = true};
}
static void begin_owner(fixture *f, unsigned i) {
    if (!cui_frame_sequence(f->ui))
        return;
    cui_scroll_options o = options(f, i);
    o.disabled = f->owner_disabled || f->owner_readonly || f->ancestor_disabled;
    cui_base_scrollable(f->ui, owner(i), &f->state[i], o);
    const cui_semantic *current_owner = semantic(f->ui, owner(i));
    if (current_owner) {
        cui_semantic_details details = current_owner->details;
        details.readonly = f->owner_readonly;
        cui_set_semantic_details(f->ui, owner(i), details);
    }
    cui_scrollbar_style style = {.visibility = (f->flags & PROTOTYPE_TRACK_VISIBLE)
                                                   ? CUI_SCROLLBAR_VISIBLE
                                                   : CUI_SCROLLBAR_HIDDEN,
                                 .thickness = 6,
                                 .min_thumb = 8};
    o.hide_scrollbars = false;
    o.scrollbar_style = &style;
    o.disabled = o.disabled || !(f->flags & PROTOTYPE_TRACK_INTERACTIVE);
    if (!f->placeholder && f->viewport[i].w > 0 && f->viewport[i].h > 0) {
        float x = 0;
        cui_base_scrollbar_overlay(f->ui, owner(i), &x, &f->state[i].y,
                                   (cui_rect){0, 0, f->viewport[i].w, f->viewport[i].h},
                                   (cui_size){f->viewport[i].w, f->content[i].h}, o);
    }
    cui_id bar = cui_scrollbar_id(owner(i), true);
    if (!semantic(f->ui, bar)) {
        cui_interact(f->ui, bar, CUI_ROLE_SCROLLBAR, cui_string("Vertical scroll"), true, false, 0);
        cui_set_semantic_details(f->ui, bar,
                                 (cui_semantic_details){.controls = owner(i),
                                                        .parent = owner(i),
                                                        .orientation = CUI_ORIENTATION_VERTICAL});
    }
}
static void decorate(fixture *f, unsigned i) {
    cui_id bar = cui_scrollbar_id(owner(i), true);
    cui_begin_absolute_box(
        f->ui, bar, (cui_box){.background = track_color(i), .custom_opacity = true, .opacity = .5f},
        (cui_rect){0, 0, 6, 1});
    assert(cui_track_bounds(f->ui, bar, &f->track[i]));
    if (f->shadows) {
        cui_box_shadow shadow = {.layer = {.offset_x = -2, .sigma = 1, .color = {31, 41, 51, 160}}};
        assert(cui_set_box_shadows(f->ui, &shadow, 1, (cui_corner_radii){0}));
    }
    cui_begin_absolute_box(f->ui, thumb_id(i), (cui_box){.background = thumb_color(i)},
                           (cui_rect){0, 0, 6, 1});
    assert(cui_track_bounds(f->ui, thumb_id(i), &f->thumb[i]));
    if (f->shadows) {
        cui_box_shadow shadow = {.layer = {.offset_x = -1, .sigma = 1, .color = {33, 43, 53, 160}}};
        assert(cui_set_box_shadows(f->ui, &shadow, 1, (cui_corner_radii){0}));
    }
    cui_end_box(f->ui);
    cui_end_box(f->ui);
}
static void finish_owner(fixture *f, unsigned i) {
    if (!cui_frame_sequence(f->ui))
        return;
    cui_scroll_options o = options(f, i);
    o.disabled = f->owner_disabled || f->owner_readonly || f->ancestor_disabled;
    cui_base_scrollable_finish(f->ui, owner(i), &f->state[i], o);
    const cui_semantic *current_owner = semantic(f->ui, owner(i));
    if (current_owner) {
        cui_semantic_details details = current_owner->details;
        details.readonly = f->owner_readonly;
        cui_set_semantic_details(f->ui, owner(i), details);
    }
    assert(cui_set_box_scroll_offset(f->ui, owner(i), 0, -f->state[i].y));
}
static bool observe(const cui_prepaint_info *info, const void *payload,
                    cui_prepaint_writer *writer) {
    (void)writer;
    fixture *f = *(fixture *const *)payload;
    ++f->hooks;
    f->prepaint_hit = info->pointer_hit;
    if (info->pointer_valid) {
        /* A later input assertion checks the completed target, not this probe's bounds. */
        assert(isfinite(info->bounds.h));
    }
    return true;
}
static cui_draw_list scene(fixture *f) {
    assert(cui_begin_frame(f->ui, f->width, f->height, .016f));
    ++f->compositions;
    if (f->clip_height > 0)
        cui_begin_box(f->ui, 90,
                      (cui_box){.width = f->width, .height = f->clip_height, .clip = true});
    cui_begin_box(f->ui, PANEL,
                  (cui_box){.width = f->width,
                            .max_height = f->height,
                            .custom_opacity = true,
                            .opacity = .5f,
                            .inert = f->inert,
                            .layout_only = f->layout_only});
    cui_interact(f->ui, PANEL, CUI_ROLE_GROUP, cui_string("Ancestor"), f->ancestor_disabled, false,
                 0);
    assert(cui_set_prepaint(f->ui, (cui_prepaint_options){.callback = observe,
                                                          .payload = &f,
                                                          .payload_bytes = sizeof f,
                                                          .command_budget = 1}));
    cui_begin_box(f->ui, OUTER, (cui_box){.width = f->width, .height = -1, .clip = true});
    assert(cui_track_bounds(f->ui, OUTER, &f->viewport[0]));
    begin_owner(f, 0);
    cui_begin_box(f->ui, COLUMN, (cui_box){.width = f->width, .height = -1});
    assert(cui_track_bounds(f->ui, COLUMN, &f->content[0]));
    cui_begin_box(f->ui, 110, (cui_box){.width = f->width, .height = f->header});
    cui_end_box(f->ui);
    cui_begin_box(f->ui, FRAME, (cui_box){.width = f->width, .height = -1});
    cui_begin_absolute_box(f->ui, BODY, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
    assert(cui_track_bounds(f->ui, BODY, &f->viewport[1]));
    begin_owner(f, 1);
    cui_begin_box(f->ui, CONTENT,
                  (cui_box){.width = f->width, .height = f->body, .fixed_height = true});
    assert(cui_track_bounds(f->ui, CONTENT, &f->content[1]));
    if (f->nested) {
        cui_scroll_options child_options = {.width = 80,
                                            .height = 60,
                                            .content_width = 80,
                                            .content_height = 300,
                                            .vertical = true,
                                            .hide_scrollbars = true};
        cui_begin_box(f->ui, 200, (cui_box){.width = 80, .height = 60, .clip = true});
        cui_base_scrollable(f->ui, 200, &f->child, child_options);
        cui_begin_box(f->ui, 201, (cui_box){.width = 80, .height = 300});
        cui_end_box(f->ui);
        cui_base_scrollable_finish(f->ui, 200, &f->child, child_options);
        assert(cui_set_box_scroll_offset(f->ui, 200, 0, -f->child.y));
        cui_end_box(f->ui);
        cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, 200, 201};
        assert(cui_layout_scroll_clamp_v1(f->ui, &clamp, &f->child.y));
    }
    cui_end_box(f->ui);
    decorate(f, 1);
    finish_owner(f, 1);
    cui_end_box(f->ui);
    cui_end_box(f->ui);
    cui_begin_box(f->ui, 111, (cui_box){.width = f->width, .height = 40});
    cui_end_box(f->ui);
    cui_end_box(f->ui);
    decorate(f, 0);
    finish_owner(f, 0);
    cui_end_box(f->ui);
    cui_end_box(f->ui);
    if (f->clip_height > 0)
        cui_end_box(f->ui);
    if (f->cover_modal) {
        assert(cui_begin_focus_scope(f->ui, 900, true));
        cui_begin_portal(f->ui, 900, (cui_box){.background = {19, 29, 39, 255}},
                         (cui_rect){0, 0, f->width, f->height}, 20);
        cui_interact(f->ui, 900, CUI_ROLE_BUTTON, cui_string("Modal cover"), false, false, 0);
        cui_end_box(f->ui);
        cui_end_focus_scope(f->ui);
    }
    cui_scroll_body_layout_v1 body = {.struct_size = 80,
                                      .version = 1,
                                      .mode = CUI_SCROLL_BODY_AUTO,
                                      .panel = PANEL,
                                      .outer_viewport = OUTER,
                                      .column = COLUMN,
                                      .body_frame = FRAME,
                                      .viewport = BODY,
                                      .content = CONTENT,
                                      .max_height = f->height};
    assert(cui_layout_scroll_body_v1(f->ui, &body, &f->state[0].y, &f->state[1].y));
    for (unsigned i = 0; i < 2; ++i) {
        prototype_track_record r = {.struct_size = 48,
                                    .version = 1,
                                    .flags = f->flags,
                                    .viewport = owner(i),
                                    .bar = cui_scrollbar_id(owner(i), true),
                                    .thumb = thumb_id(i),
                                    .thickness = 6,
                                    .minimum_thumb = 8};
        assert(prototype_scrollbar_track(f->ui, &r));
        memset(&r, 0, sizeof r);
    }
    cui_draw_list draw = cui_end_frame(f->ui);
    if (cui_last_error(f->ui) != CUI_OK)
        fprintf(stderr, "scene failed: %d width%.1f height%.1f header%.1f body%.1f\n",
                cui_last_error(f->ui), f->width, f->height, f->header, f->body);
    assert(cui_last_error(f->ui) == CUI_OK);
    return draw;
}
static void geometry(fixture *f, cui_draw_list draw) {
    for (unsigned i = 0; i < 2; ++i) {
        float maximum = fmaxf(0, f->content[i].h - f->viewport[i].h);
        const cui_semantic *bar = semantic(f->ui, cui_scrollbar_id(owner(i), true));
        bool shown = (f->flags & PROTOTYPE_TRACK_VISIBLE) && maximum > 0 && f->viewport[i].h > 0;
        if (!shown || f->inert || f->layout_only)
            assert(!bar);
        else {
            assert(bar && bar->disabled == (!(f->flags & PROTOTYPE_TRACK_INTERACTIVE) ||
                                            f->owner_disabled || f->owner_readonly ||
                                            f->ancestor_disabled));
            rect_equal(bar->bounds, f->track[i]);
            assert(near(bar->details.maximum, maximum));
            assert(near(bar->value, f->state[i].y));
        }
        cui_rect expected = {f->viewport[i].x + f->viewport[i].w - 6, f->viewport[i].y, 6,
                             shown ? f->viewport[i].h : 0};
        rect_equal(f->track[i], expected);
        expected = shown ? cui_scrollbar_thumb_ex(f->track[i], f->state[i].y, f->viewport[i].h,
                                                  f->content[i].h, true, 8)
                         : (cui_rect){f->track[i].x, f->track[i].y, 6, 0};
        rect_equal(f->thumb[i], expected);
        unsigned track_seen = 0, thumb_seen = 0, shadow_seen = 0;
        for (size_t j = 0; j < draw.count; ++j) {
            cui_draw_command c = draw.commands[j];
            if (c.type == CUI_DRAW_RECT && c.color.r == track_color(i).r && c.color.g == 81) {
                if (!shown) {
                    assert(c.color.a == 0 && c.bounds.h == 0);
                    continue;
                }
                rect_equal(c.bounds, f->track[i]);
                assert(c.color.a == 50);
                ++track_seen;
            }
            if (c.type == CUI_DRAW_RECT && c.color.r == thumb_color(i).r && c.color.g == 83) {
                if (!shown) {
                    assert(c.color.a == 0 && c.bounds.h == 0);
                    continue;
                }
                rect_equal(c.bounds, f->thumb[i]);
                assert(c.color.a == 50);
                ++thumb_seen;
            }
            if (c.type == CUI_DRAW_DROP_SHADOW && c.shadow.layer.color.r == 31) {
                if (shown && near(c.shadow.caster.x, f->track[i].x) &&
                    near(c.shadow.caster.y, f->track[i].y))
                    ++shadow_seen;
            }
        }
        if (shown && !f->layout_only) {
            assert(track_seen == 1 && thumb_seen == 1);
            if (f->shadows)
                assert(shadow_seen);
        } else
            assert(!track_seen && !thumb_seen);
    }
}
static void queue(fixture *f, cui_event e) {
    assert(cui_queue_event(f->ui, e));
}
static void pointer(fixture *f, cui_event_type type, float x, float y) {
    queue(f, (cui_event){.type = type, .x = x, .y = y, .pointer_button = CUI_POINTER_PRIMARY});
}
static void interactions(fixture *f, unsigned i) {
    f->state[i].y = 0;
    scene(f);
    cui_rect thumb = f->thumb[i];
    float x = thumb.x + 3, y = thumb.y + fminf(thumb.h / 2, 3);
    pointer(f, CUI_EVENT_POINTER_DOWN, x, y);
    scene(f);
    assert(cui_event_target(f->ui, 0) == cui_scrollbar_id(owner(i), true));
    pointer(f, CUI_EVENT_POINTER_MOVE, x, y + 30);
    scene(f);
    assert(f->state[i].y > 0);
    f->height -= 20;
    scene(f); /* resize while held, exact geometry follows */
    pointer(f, CUI_EVENT_POINTER_UP, x + 100, y + 30);
    scene(f);
    float old = f->state[i].y;
    pointer(f, CUI_EVENT_POINTER_MOVE, x, y + 60);
    scene(f);
    assert(near(f->state[i].y, old));
    f->state[i].y = 0;
    scene(f);
    queue(f, (cui_event){.type = CUI_EVENT_SCROLL,
                         .x = f->viewport[i].x + 20,
                         .y = f->viewport[i].y + 20,
                         .dy = -1});
    scene(f);
    assert(f->state[i].y > 0);
    f->state[i].y = 0;
    scene(f);
    x = f->viewport[i].x + 20;
    y = f->viewport[i].y + 30;
    queue(f, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                         .pointer_type = CUI_POINTER_TOUCH,
                         .pointer_id = 1,
                         .pointer_button = CUI_POINTER_PRIMARY,
                         .x = x,
                         .y = y});
    scene(f);
    queue(f, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                         .pointer_type = CUI_POINTER_TOUCH,
                         .pointer_id = 1,
                         .x = x,
                         .y = y - 20});
    scene(f);
    assert(f->state[i].y > 0);
    queue(f, (cui_event){.type = CUI_EVENT_POINTER_UP,
                         .pointer_type = CUI_POINTER_TOUCH,
                         .pointer_id = 1,
                         .pointer_button = CUI_POINTER_PRIMARY,
                         .x = x,
                         .y = y - 20});
    scene(f);
}
static void pixels(fixture *f, cui_draw_list draw, bool alpha) {
    unsigned width = (unsigned)ceilf(f->width), height = (unsigned)ceilf(f->height);
    cui_software *software = cui_software_create(width, height, NULL);
    assert(software && cui_software_render(software, &draw, (cui_color){0}));
    const unsigned char *data = cui_software_pixels(software, NULL, NULL);
    unsigned x = (unsigned)(f->track[1].x + 3);
    for (unsigned y = (unsigned)f->clip_height; y < height; ++y)
        for (unsigned column = 0; column < width; ++column)
            assert(data[((size_t)y * width + column) * 4 + 3] == 0);
    if (alpha) {
        unsigned y = (unsigned)(f->thumb[1].y + 3);
        assert(data[((size_t)y * width + x) * 4 + 3] == 90); /* two alpha50 layers */
        y = (unsigned)(f->thumb[1].y + f->thumb[1].h + 3);
        assert(y < (unsigned)f->clip_height);
        assert(data[((size_t)y * width + x) * 4 + 3] == 50);
    }
    cui_software_destroy(software);
}
static void clipping_and_hits(fixture *f) {
    f->inert = false;
    f->flags = 3;
    f->width = 240;
    f->height = 260;
    f->header = 40;
    f->body = 800;
    f->clip_height = 120;
    f->shadows = false;
    f->state[0].y = f->state[1].y = 0;
    cui_draw_list draw = scene(f);
    geometry(f, draw);
    pixels(f, draw, true);
    float x = f->track[1].x + 3;
    pointer(f, CUI_EVENT_POINTER_MOVE, x, 70);
    scene(f);
    assert(f->prepaint_hit == cui_scrollbar_id(BODY, true));
    pointer(f, CUI_EVENT_POINTER_DOWN, x, 70);
    scene(f);
    assert(cui_event_target(f->ui, 0) == cui_scrollbar_id(BODY, true));
    pointer(f, CUI_EVENT_POINTER_UP, x, 70);
    scene(f);
    pointer(f, CUI_EVENT_POINTER_MOVE, x, 130);
    scene(f);
    assert(f->prepaint_hit != cui_scrollbar_id(BODY, true));
    pointer(f, CUI_EVENT_POINTER_DOWN, x, 130);
    scene(f);
    assert(cui_event_target(f->ui, 0) != cui_scrollbar_id(BODY, true));
    pointer(f, CUI_EVENT_POINTER_UP, x, 130);
    scene(f);
    f->shadows = true;
    pixels(f, scene(f), false);
    f->clip_height = 20;
    draw = scene(f);
    pixels(f, draw, false);
    f->clip_height = 0;
}
static void nested_consumption(fixture *f) {
    f->inert = false;
    f->flags = 3;
    f->shadows = false;
    f->nested = true;
    f->header = 40;
    f->body = 800;
    f->height = 260;
    f->width = 240;
    f->state[0].y = f->state[1].y = f->child.y = 0;
    scene(f);
    queue(f, (cui_event){.type = CUI_EVENT_SCROLL, .x = 20, .y = 60, .dy = -1});
    scene(f);
    assert(f->child.y == 48 && f->state[1].y == 0 && cui_event_consumed(f->ui, 0));
    /* Previous snapshot permits child scrolling, but current child is at edge.
     * Parent begin runs first; only parent finish may handle the deferred event. */
    f->child.y = 0;
    scene(f);
    f->child.y = 240;
    queue(f, (cui_event){.type = CUI_EVENT_SCROLL, .x = 20, .y = 60, .dy = -1});
    scene(f);
    assert(f->child.y == 240 && f->state[1].y == 48 && cui_event_consumed(f->ui, 0));
    assert(cui_event_target(f->ui, 0) == BODY);
    f->state[1].y = f->child.y = 0;
    scene(f);
    queue(f, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                         .pointer_type = CUI_POINTER_TOUCH,
                         .pointer_id = 2,
                         .pointer_button = CUI_POINTER_PRIMARY,
                         .x = 20,
                         .y = 70});
    scene(f);
    queue(f, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                         .pointer_type = CUI_POINTER_TOUCH,
                         .pointer_id = 2,
                         .x = 20,
                         .y = 50});
    scene(f);
    assert(f->child.y > 0 && f->state[1].y == 0);
    queue(f, (cui_event){.type = CUI_EVENT_POINTER_UP,
                         .pointer_type = CUI_POINTER_TOUCH,
                         .pointer_id = 2,
                         .pointer_button = CUI_POINTER_PRIMARY,
                         .x = 20,
                         .y = 50});
    scene(f);
    f->nested = false;
}
static void disabled_ownership(fixture *f) {
    f->nested = false;
    f->inert = false;
    f->flags = 3;
    f->header = 360;
    f->body = 100;
    f->height = 220;
    f->shadows = false;
    f->state[0].y = 0;
    scene(f); /* Establish this owner geometry before testing input disable. */
    for (unsigned trial = 0; trial < 3; ++trial) {
        f->owner_disabled = trial == 0;
        f->owner_readonly = trial == 1;
        f->ancestor_disabled = trial == 2;
        f->state[0].y = 20;
        geometry(f, scene(f));
        assert(f->state[0].y == 20);
        float x = f->thumb[0].x + 3, y = f->thumb[0].y + 2;
        pointer(f, CUI_EVENT_POINTER_DOWN, x, y);
        scene(f);
        pointer(f, CUI_EVENT_POINTER_MOVE, x, y + 40);
        scene(f);
        assert(f->state[0].y == 20);
        assert(!cui_pressed(f->ui, cui_scrollbar_id(OUTER, true)));
        assert(cui_focused(f->ui) != cui_scrollbar_id(OUTER, true));
        pointer(f, CUI_EVENT_POINTER_UP, x, y + 40);
        scene(f);
        queue(f, (cui_event){.type = CUI_EVENT_SCROLL, .x = 20, .y = 30, .dy = -1});
        scene(f);
        assert(f->state[0].y == 20);
    }
    f->owner_disabled = f->owner_readonly = f->ancestor_disabled = false;
}
static bool forbidden_bar_callback(const cui_prepaint_info *info, const void *payload,
                                   cui_prepaint_writer *writer) {
    (void)info;
    (void)writer;
    unsigned *calls = *(unsigned *const *)payload;
    ++*calls;
    return true;
}
static prototype_track_record standalone_with_callback(cui_context *ui, cui_id id, float y,
                                                       float placeholder, unsigned *calls) {
    cui_id bar = cui_scrollbar_id(id, true);
    cui_begin_box(ui, id, (cui_box){.width = 20, .height = 30, .clip = true});
    cui_interact(ui, id, CUI_ROLE_GROUP, cui_string("Scrollable"), false, false, 0);
    cui_set_semantic_details(ui, id,
                             (cui_semantic_details){.scrollable = true,
                                                    .scroll_y = y,
                                                    .scroll_max_y = 170,
                                                    .orientation = CUI_ORIENTATION_VERTICAL});
    cui_begin_box(ui, id + 1, (cui_box){.width = 20, .height = 200});
    cui_end_box(ui);
    cui_interact(ui, bar, CUI_ROLE_SCROLLBAR, cui_string("Vertical"), true, false, 0);
    cui_set_semantic_details(ui, bar,
                             (cui_semantic_details){.parent = id,
                                                    .controls = id,
                                                    .orientation = CUI_ORIENTATION_VERTICAL});
    cui_begin_absolute_box(ui, bar, (cui_box){0}, (cui_rect){0, 0, 6, placeholder});
    if (calls)
        assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = forbidden_bar_callback,
                                                           .payload = &calls,
                                                           .payload_bytes = sizeof calls,
                                                           .command_budget = 1}));
    cui_begin_absolute_box(ui, id + 2, (cui_box){0}, (cui_rect){0, 0, 6, placeholder});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    return (prototype_track_record){48, 1, 3, 0, id, bar, id + 2, 6, 8};
}
static prototype_track_record standalone(cui_context *ui, cui_id id, float y, float placeholder) {
    return standalone_with_callback(ui, id, y, placeholder, NULL);
}
static void isolated_ownership_and_sizing(void) {
    for (unsigned trial = 0; trial < 4; ++trial) {
        cui_context *ui = cui_create((cui_config){.width = 100, .height = 100});
        assert(ui && cui_begin_frame(ui, 100, 100, .016f));
        float y = 20;
        if (trial == 1) {
            cui_begin_box(ui, 900, (cui_box){.width = 20, .height = 30});
            cui_interact(ui, 900, CUI_ROLE_GROUP, cui_string("Disabled ancestor"), true, false, 0);
        }
        prototype_track_record r = standalone(ui, 1000, y, trial == 2 ? 0 : trial == 3 ? -1 : 1);
        if (trial == 0) {
            cui_semantic_details details = semantic(ui, 1000)->details;
            details.readonly = true;
            cui_set_semantic_details(ui, 1000, details);
            assert(!semantic(ui, 1000)->disabled);
        }
        if (trial == 1)
            cui_end_box(ui);
        cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, 1000, 1001};
        assert(cui_layout_scroll_clamp_v1(ui, &clamp, &y));
        if (trial < 2)
            assert(prototype_scrollbar_track(ui, &r));
        else
            assert(!prototype_scrollbar_track(ui, &r));
        cui_draw_list draw = cui_end_frame(ui);
        if (trial < 2) {
            assert(cui_last_error(ui) == CUI_OK && y == 20);
            const cui_semantic *bar = semantic(ui, r.bar);
            assert(bar && bar->disabled && !semantic(ui, 1000)->disabled);
            cui_rect bounds = bar->bounds;
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                   .pointer_button = CUI_POINTER_PRIMARY,
                                                   .x = bounds.x + 3,
                                                   .y = bounds.y + 2}));
            assert(cui_begin_frame(ui, 100, 100, .016f));
            assert(cui_event_target(ui, 0) != r.bar);
            cui_end_frame(ui);
            assert(y == 20);
        } else
            assert(cui_last_error(ui) != CUI_OK && draw.count == 0 && y == 20);
        cui_destroy(ui);
    }
}
static void failure_and_capacity(void) {
    for (unsigned trial = 0; trial < 5; ++trial) {
        cui_context *ui = cui_create(
            (cui_config){.width = 400,
                         .height = 400,
                         .resource_bytes = trial == 4 ? prototype_track_resource_bytes() - 1 : 0});
        assert(ui && cui_begin_frame(ui, 400, 400, .016f));
        float offsets[65];
        for (unsigned i = 0; i < 65; ++i)
            offsets[i] = 10000;
        unsigned count = trial < 2 ? 64 : 1;
        for (unsigned i = 0; i < count; ++i) {
            cui_id id = 1000 + (cui_id)i * 10;
            prototype_track_record r = standalone(ui, id, offsets[i], 1);
            cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, id, id + 1};
            bool registered = cui_layout_scroll_clamp_v1(ui, &clamp, &offsets[i]);
            if (trial == 4) {
                assert(!registered);
                break;
            }
            assert(registered);
            if (trial == 2) {
                r.flags = PROTOTYPE_TRACK_INTERACTIVE;
                assert(!prototype_scrollbar_track(ui, &r));
            } else if (trial == 3) {
                r.thumb = r.bar;
                assert(!prototype_scrollbar_track(ui, &r));
            } else
                assert(prototype_scrollbar_track(ui, &r));
        }
        if (trial == 1) {
            prototype_track_record r = standalone(ui, 3000, offsets[64], 1);
            (void)r;
            cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, 3000, 3001};
            assert(!cui_layout_scroll_clamp_v1(ui, &clamp, &offsets[64]));
        }
        if (trial == 2)
            cui_clear_error(ui); /* cannot salvage failed opt-in */
        cui_draw_list draw = cui_end_frame(ui);
        if (trial == 0) {
            assert(cui_last_error(ui) == CUI_OK);
            for (unsigned i = 0; i < 64; ++i)
                assert(offsets[i] == 170);
        } else {
            assert(cui_last_error(ui) != CUI_OK && draw.count == 0);
            for (unsigned i = 0; i < count; ++i)
                assert(offsets[i] == 10000);
        }
        cui_clear_error(ui);
        assert(cui_begin_frame(ui, 400, 400, .016f));
        cui_begin_box(ui, 10, (cui_box){.width = 20, .height = 20});
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        cui_destroy(ui);
    }
}
static void records_and_context_isolation(void) {
    for (unsigned trial = 0; trial < 8; ++trial) {
        cui_context *bad = cui_create((cui_config){.width = 100, .height = 100});
        cui_context *good = cui_create((cui_config){.width = 100, .height = 100});
        assert(bad && good);
        float bad_y = 10000, good_y = 10000;
        assert(cui_begin_frame(bad, 100, 100, .016f));
        prototype_track_record r = standalone(bad, 1000, bad_y, 1);
        cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, 1000, 1001};
        assert(cui_layout_scroll_clamp_v1(bad, &clamp, &bad_y));
        switch (trial) {
        case 0:
            r.struct_size--;
            break;
        case 1:
            r.version++;
            break;
        case 2:
            r.reserved = 1;
            break;
        case 3:
            r.flags = 4;
            break;
        case 4:
            r.thickness = NAN;
            break;
        case 5:
            r.minimum_thumb = INFINITY;
            break;
        case 6:
            r.minimum_thumb = 0;
            break;
        case 7:
            r.viewport = 9999;
            break;
        }
        assert(!prototype_scrollbar_track(bad, &r));
        assert(cui_last_error(bad) == CUI_ERROR_INVALID_ACTION);
        cui_clear_error(bad);
        assert(cui_end_frame(bad).count == 0 && cui_last_error(bad) == CUI_ERROR_LAYOUT);
        assert(bad_y == 10000);
        /* The immutable failed header must not poison another context. */
        assert(cui_begin_frame(good, 100, 100, .016f));
        r = standalone(good, 1000, good_y, 1);
        assert(cui_layout_scroll_clamp_v1(good, &clamp, &good_y));
        assert(prototype_scrollbar_track(good, &r));
        cui_end_frame(good);
        assert(cui_last_error(good) == CUI_OK && good_y == 170);
        cui_destroy(bad);
        cui_destroy(good);
    }
}
static void portal_and_callback_admission(void) {
    for (unsigned trial = 0; trial < 3; ++trial) {
        cui_context *ui = cui_create((cui_config){.width = 100, .height = 100});
        assert(ui);
        assert(cui_begin_frame(ui, 100, 100, .016f));
        float y = 20;
        unsigned callbacks = 0;
        prototype_track_record r =
            standalone_with_callback(ui, 1000, y, 1, trial == 2 ? &callbacks : NULL);
        cui_begin_portal(ui, 9000, (cui_box){.background = {91, 92, 93, 255}},
                         (cui_rect){40, 40, 10, 10}, 4);
        cui_end_box(ui);
        cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, 1000, 1001};
        assert(cui_layout_scroll_clamp_v1(ui, &clamp, &y));
        if (trial == 1)
            r.thumb = 9000;
        bool accepted = prototype_scrollbar_track(ui, &r);
        assert(accepted == (trial == 0));
        cui_draw_list draw = cui_end_frame(ui);
        if (trial == 0) {
            assert(cui_last_error(ui) == CUI_OK && y == 20);
            bool portal = false;
            for (size_t j = 0; j < draw.count; ++j)
                portal |= draw.commands[j].type == CUI_DRAW_RECT && draw.commands[j].color.r == 91;
            assert(portal); /* Independent portal survives and isn't a fake track child. */
        } else
            assert(cui_last_error(ui) != CUI_OK && draw.count == 0 && y == 20);
        assert(callbacks == 0);
        cui_destroy(ui);
    }
}
static void modal_eligibility(fixture *f) {
    f->inert = f->layout_only = f->nested = false;
    f->shadows = false;
    f->flags = 3;
    f->header = 40;
    f->body = 800;
    f->height = 260;
    f->width = 240;
    f->state[0].y = f->state[1].y = 0;
    scene(f);
    f->cover_modal = true;
    pointer(f, CUI_EVENT_POINTER_MOVE, 237, 50);
    scene(f);
    assert(f->prepaint_hit == 900);
    pointer(f, CUI_EVENT_POINTER_DOWN, 237, 50);
    scene(f);
    assert(cui_event_target(f->ui, 0) == 900 && f->state[1].y == 0);
    assert(cui_focused(f->ui) == 900);
    pointer(f, CUI_EVENT_POINTER_UP, 237, 50);
    scene(f);
    f->cover_modal = false;
    scene(f);
}
static void math_comparator(void) {
    uint32_t seed = 17;
    const float special[] = {0, -1, NAN, INFINITY, -INFINITY, 1, 12, 100};
    for (unsigned i = 0; i < 20000; ++i) {
        float v[6];
        for (unsigned j = 0; j < 6; ++j) {
            seed = seed * 1664525u + 1013904223u;
            v[j] = i < 64 ? special[(i + j) % 8] : (float)(int32_t)(seed % 20000u) - 1000;
        }
        cui_rect track = {13, 17, v[0], v[1]};
        for (unsigned axis = 0; axis < 2; ++axis) {
            cui_rect a = prototype_thumb_geometry(track, v[2], v[3], v[4], axis != 0, v[5]);
            cui_rect b = cui_scrollbar_thumb_ex(track, v[2], v[3], v[4], axis != 0, v[5]);
            cui_rect direct =
                cui_scrollbar_thumb_geometry(track, v[2], v[3], v[4], axis != 0, v[5]);
            float aa[] = {a.x, a.y, a.w, a.h}, bb[] = {b.x, b.y, b.w, b.h};
            float cc[] = {direct.x, direct.y, direct.w, direct.h};
            for (unsigned k = 0; k < 4; ++k) {
                assert((isnan(aa[k]) && isnan(bb[k])) || !memcmp(&aa[k], &bb[k], sizeof(float)));
                assert((isnan(aa[k]) && isnan(cc[k])) || !memcmp(&aa[k], &cc[k], sizeof(float)));
            }
        }
    }
}
static void lifecycle_late_alias_and_reuse(void) {
    for (unsigned trial = 0; trial < 4; ++trial) {
        cui_context *ui = cui_create((cui_config){.width = 100, .height = 100});
        assert(ui);
        assert(!cui_layout_scrollbar_track_v1(ui, NULL));
        assert(cui_last_error(ui) == CUI_ERROR_LIFECYCLE);
        cui_clear_error(ui);
        assert(cui_begin_frame(ui, 100, 100, .016f));
        union {
            float y;
            cui_rect bounds;
        } retained;
        retained.y = 10000;
        prototype_track_record old = standalone(ui, 1000, retained.y, 1);
        cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, 1000, 1001};
        assert(cui_layout_scroll_clamp_v1(ui, &clamp, &retained.y));
        assert(prototype_scrollbar_track(ui, &old));
        if (trial == 0) { /* Actual pinned folded collision after registration. */
            cui_begin_box(ui, 76894, (cui_box){.width = 1, .height = 1});
            cui_end_box(ui);
            cui_begin_box(ui, 86448, (cui_box){.width = 1, .height = 1});
            cui_end_box(ui);
        } else if (trial == 1) {
            assert(cui_track_bounds(ui, old.bar, &retained.bounds));
        } else if (trial == 2) {
            cui_begin_box(ui, old.bar, (cui_box){.width = 1, .height = 1});
            cui_end_box(ui);
        } else
            memset(&old, 0, sizeof old);
        cui_draw_list draw = cui_end_frame(ui);
        if (trial < 3)
            assert(cui_last_error(ui) != CUI_OK && draw.count == 0 && retained.y == 10000);
        else
            assert(cui_last_error(ui) == CUI_OK && retained.y == 170);
        cui_clear_error(ui);
        assert(cui_begin_frame(ui, 100, 100, .016f));
        retained.y = 10000;
        old = standalone(ui, 1000, retained.y, 1);
        assert(cui_layout_scroll_clamp_v1(ui, &clamp, &retained.y));
        assert(prototype_scrollbar_track(ui, &old));
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && retained.y == 170);
        cui_destroy(ui);
    }
}
static void authoritative_bar_policy(fixture *f) {
    f->inert = f->layout_only = f->nested = f->cover_modal = false;
    f->shadows = true;
    f->flags = PROTOTYPE_TRACK_VISIBLE | PROTOTYPE_TRACK_INTERACTIVE;
    f->header = 40;
    f->body = 800;
    f->height = 260;
    f->width = 240;
    f->state[0].y = f->state[1].y = 0;
    f->placeholder = true; /* Submit disabled semantics, no overlay reduction. */
    geometry(f, scene(f));
    cui_id bar = cui_scrollbar_id(BODY, true);
    assert(!semantic(f->ui, bar)->disabled); /* INTERACTIVE replaces placeholder. */
    assert(!semantic(f->ui, BODY)->disabled && !semantic(f->ui, bar)->details.readonly);
    f->placeholder = false;
    float x = f->thumb[1].x + 3, y = f->thumb[1].y + 2;
    pointer(f, CUI_EVENT_POINTER_DOWN, x, y);
    scene(f);
    assert(cui_event_target(f->ui, 0) == bar && cui_pressed(f->ui, bar));
    pointer(f, CUI_EVENT_POINTER_MOVE, x, y + 20);
    scene(f);
    assert(f->state[1].y > 0); /* The completed enabled snapshot routes real input. */
    float before = f->state[1].y;
    f->flags = PROTOTYPE_TRACK_VISIBLE; /* Author disabled policy before reduction. */
    pointer(f, CUI_EVENT_POINTER_MOVE, x, y + 40);
    geometry(f, scene(f)); /* Ordinary named paint and shadow oracle still runs. */
    assert(semantic(f->ui, bar)->disabled && !semantic(f->ui, BODY)->disabled);
    assert(!cui_pressed(f->ui, bar) && cui_focused(f->ui) != bar);
    assert(f->state[1].y == before);
    pointer(f, CUI_EVENT_POINTER_UP, x, y + 40);
    scene(f);
    pointer(f, CUI_EVENT_POINTER_DOWN, f->thumb[1].x + 3, f->thumb[1].y + 2);
    geometry(f, scene(f));
    assert(cui_event_target(f->ui, 0) != bar && f->state[1].y == before);
    pointer(f, CUI_EVENT_POINTER_MOVE, x, y + 60);
    geometry(f, scene(f));
    assert(f->state[1].y == before && !cui_pressed(f->ui, bar));
    pointer(f, CUI_EVENT_POINTER_UP, x, y + 60);
    scene(f);
}
static void layout_only(fixture *f) {
    f->layout_only = true;
    f->inert = false;
    f->nested = false;
    f->shadows = false;
    f->flags = 3;
    f->header = 40;
    f->body = 800;
    f->height = 260;
    f->state[0].y = f->state[1].y = 10000;
    unsigned hooks = f->hooks;
    cui_draw_list draw = scene(f);
    geometry(f, draw);
    assert(draw.count == 0 && f->state[0].y == 10000 && f->state[1].y == 10000);
    assert(f->hooks == hooks);
    f->layout_only = false;
}
int main(void) {
    math_comparator();
    failure_and_capacity();
    lifecycle_late_alias_and_reuse();
    isolated_ownership_and_sizing();
    portal_and_callback_admission();
    records_and_context_isolation();
    fixture f = {
        .width = 240, .height = 260, .header = 40, .body = 800, .flags = 3, .shadows = true};
    f.ui = cui_create((cui_config){.width = 240, .height = 260});
    assert(f.ui);
    const float widths[] = {240, 160, 280, 200, 240}, heights[] = {260, 220, 180, 300, 260},
                bodies[] = {800, 900, 500, 0, 20};
    for (unsigned i = 0; i < 5; ++i) {
        f.width = widths[i];
        f.height = heights[i];
        f.body = bodies[i];
        f.state[1].y = 10000;
        geometry(&f, scene(&f));
    }
    f.body = 800;
    f.height = 260;
    geometry(&f, scene(&f));
    interactions(&f, 1);
    f.header = 360;
    f.body = 100;
    f.height = 220;
    geometry(&f, scene(&f));
    interactions(&f, 0);
    f.flags = PROTOTYPE_TRACK_VISIBLE;
    geometry(&f, scene(&f));
    float disabled_before = f.state[0].y;
    pointer(&f, CUI_EVENT_POINTER_DOWN, f.thumb[0].x + 3, f.thumb[0].y + 2);
    scene(&f);
    pointer(&f, CUI_EVENT_POINTER_MOVE, f.thumb[0].x + 3, f.thumb[0].y + 32);
    scene(&f);
    assert(near(f.state[0].y, disabled_before));
    assert(!cui_pressed(f.ui, cui_scrollbar_id(OUTER, true)));
    assert(cui_focused(f.ui) != cui_scrollbar_id(OUTER, true));
    pointer(&f, CUI_EVENT_POINTER_UP, f.thumb[0].x + 3, f.thumb[0].y + 32);
    scene(&f);
    f.flags = 0;
    f.state[0].y = 0;
    geometry(&f, scene(&f));
    queue(&f, (cui_event){.type = CUI_EVENT_SCROLL, .x = 20, .y = 30, .dy = -1});
    scene(&f);
    assert(f.state[0].y > 0);
    f.flags = 3;
    f.inert = true;
    f.state[0].y = 10000;
    geometry(&f, scene(&f));
    assert(f.state[0].y == 10000);
    clipping_and_hits(&f);
    nested_consumption(&f);
    disabled_ownership(&f);
    assert(f.hooks == f.compositions);
    layout_only(&f);
    modal_eligibility(&f);
    authoritative_bar_policy(&f);
    printf("FIXED_TRACK_THUMB resource=%zu node=%zu record=%zu, Core/Base math80000 comparisons, "
           "geometry/alpha/shadows/wheel/touch/drag PASS\n",
           prototype_track_resource_bytes(), prototype_track_node_bytes(),
           sizeof(prototype_track_record));
    cui_destroy(f.ui);
    return 0;
}
