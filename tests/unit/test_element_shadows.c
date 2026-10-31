/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <string.h>

static const cui_corner_radii square = {0};
static const cui_box_shadow drop = {
    .layer = {.offset_x = 3, .offset_y = 2, .color = {0, 0, 0, 255}},
    .kind = CUI_BOX_SHADOW_DROP};
static cui_context *create(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 64, .max_commands = 256});
    assert(ui);
    return ui;
}
static void box(cui_context *ui, cui_id id, float size) {
    cui_begin_box(ui, id, (cui_box){.width = size, .height = size,
                                   .fixed_width = true, .fixed_height = true,
                                   .background = {200, 80, 40, 128}});
}
static unsigned count(cui_draw_list list, cui_draw_type type) {
    unsigned n = 0;
    for (size_t i = 0; i < list.count; i++)
        n += list.commands[i].type == type;
    return n;
}
static const cui_draw_command *first(cui_draw_list list, cui_draw_type type) {
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == type)
            return &list.commands[i];
    return NULL;
}
static void factory(void) {
    cui_box_shadow out = drop;
    assert(cui_box_shadow_from_css(-4096, 4096, 512, -4096, (cui_color){1, 2, 3, 4},
                                    CUI_BOX_SHADOW_INSET, &out));
    assert(out.layer.sigma == 256 && out.layer.spread == -4096 &&
           out.kind == CUI_BOX_SHADOW_INSET && out.layer.color.a == 4);
    assert(cui_box_shadow_from_css(0, 0, 0, 4096, (cui_color){0}, CUI_BOX_SHADOW_DROP, &out));
    cui_box_shadow before = out;
    const float invalid[] = {-1, 513, NAN, INFINITY};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        assert(!cui_box_shadow_from_css(0, 0, invalid[i], 0, (cui_color){0},
                                         CUI_BOX_SHADOW_DROP, &out));
        assert(memcmp(&out, &before, sizeof out) == 0);
    }
    assert(!cui_box_shadow_from_css(NAN, 0, 1, 0, (cui_color){0}, CUI_BOX_SHADOW_DROP, &out));
    assert(!cui_box_shadow_from_css(4097, 0, 1, 0, (cui_color){0}, CUI_BOX_SHADOW_DROP, &out));
    assert(!cui_box_shadow_from_css(0, 0, 1, INFINITY, (cui_color){0}, CUI_BOX_SHADOW_DROP, &out));
    assert(!cui_box_shadow_from_css(0, 0, 1, 0, (cui_color){0}, (cui_box_shadow_kind)7, &out));
    assert(!cui_box_shadow_from_css(0, 0, 1, 0, (cui_color){0}, CUI_BOX_SHADOW_DROP, NULL));
    assert(memcmp(&out, &before, sizeof out) == 0);
}
static void closed_open_portal(void) {
    cui_context *ui = create();
    for (unsigned frame = 0; frame < 3; frame++) {
        assert(cui_begin_frame(ui, 96, 96, .016f));
        cui_begin_box(ui, 1, (cui_box){.padding = (float)(frame * 5)});
        box(ui, 2, 20 + (float)frame * 3);
        if (frame == 0)
            assert(cui_element_set_box_shadows(ui, 2, &drop, 1, square));
        cui_end_box(ui);
        if (frame != 0)
            assert(cui_element_set_box_shadows(ui, 2, &drop, 1, square));
        cui_end_box(ui);
        cui_begin_portal(ui, 3, (cui_box){.background = {30, 40, 50, 255}},
                          (cui_rect){50, 45, 12, 13}, 10);
        cui_end_box(ui);
        assert(cui_element_set_shadow_preset(ui, 3, CUI_SHADOW_2XS, square));
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && count(list, CUI_DRAW_DROP_SHADOW) == 2);
        const cui_draw_command *c = first(list, CUI_DRAW_DROP_SHADOW);
        assert(c && c->shadow.caster.x == (float)(frame * 5) &&
               c->shadow.caster.y == (float)(frame * 5) &&
               c->shadow.caster.w == 20 + (float)frame * 3);
        assert(cui_bounds(ui, 2).w == c->shadow.caster.w);
        bool portal_found = false;
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].type == CUI_DRAW_DROP_SHADOW &&
                list.commands[i].shadow.caster.x == 50) {
                assert(list.commands[i].shadow.caster.y == 45 &&
                       list.commands[i].shadow.caster.w == 12);
                portal_found = true;
            }
        assert(portal_found);
    }
    /* Previous-frame existence does not admit a missing current-frame owner. */
    assert(cui_begin_frame(ui, 96, 96, .016f));
    assert(!cui_element_set_box_shadows(ui, 2, &drop, 1, square));
    assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
    cui_draw_list list = cui_end_frame(ui);
    assert(count(list, CUI_DRAW_DROP_SHADOW) == 0);
    assert(!cui_element_set_box_shadows(ui, 2, &drop, 1, square));
    cui_destroy(ui);
}
static void invalid_and_copied(void) {
    cui_context *ui = create();
    assert(cui_begin_frame(ui, 96, 96, .016f));
    box(ui, 1, 20);
    cui_end_box(ui);
    cui_box_shadow layers[8];
    for (size_t i = 0; i < 8; i++) {
        layers[i] = drop;
        layers[i].layer.offset_x = (float)i;
    }
    assert(cui_element_set_box_shadows(ui, 1, layers, 8, square));
    assert(!cui_element_set_box_shadows(ui, 1, layers, 9, square));
    assert(!cui_element_set_box_shadows(ui, 1, NULL, 1, square));
    layers[0].layer.sigma = NAN;
    assert(!cui_element_set_box_shadows(ui, 1, layers, 8, square));
    layers[0] = drop;
    layers[0].kind = (cui_box_shadow_kind)8;
    assert(!cui_element_set_box_shadows(ui, 1, layers, 8, square));
    assert(!cui_element_set_box_shadows(ui, 1, &drop, 1,
                                         (cui_corner_radii){.top_left = NAN}));
    assert(!cui_element_set_shadow_preset(ui, 1, (cui_shadow_preset)99, square));
    memset(layers, 0, sizeof layers);
    cui_draw_list list = cui_end_frame(ui);
    assert(count(list, CUI_DRAW_DROP_SHADOW) == 8);
    unsigned index = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_DROP_SHADOW)
            assert(list.commands[i].shadow.layer.offset_x == (float)index++);
    cui_destroy(ui);
}
static void rejected_targets(void) {
    for (unsigned mode = 0; mode < 5; mode++) {
        cui_context *ui = create();
        assert(cui_begin_frame(ui, 96, 96, .016f));
        cui_id owner = mode == 3 ? 76894 : 1;
        box(ui, owner, 20);
        cui_end_box(ui);
        assert(cui_element_set_box_shadows(ui, owner, &drop, 1, square));
        if (mode == 0)
            assert(!cui_element_set_shadow_preset(ui, 0, CUI_SHADOW_NONE, square));
        if (mode == 1) {
            cui_begin_box(ui, 3, (cui_box){.layout_only = true});
            assert(!cui_element_set_shadow_preset(ui, owner, CUI_SHADOW_NONE, square));
            cui_end_box(ui);
            assert(!cui_element_set_shadow_preset(ui, 3, CUI_SHADOW_MD, square));
        }
        if (mode == 2 || mode == 3) {
            /* Distinct public IDs76894/86448 collide under pinned Core fold. */
            cui_id collision = mode == 2 ? owner : 86448;
            if (mode == 3)
                assert(!cui_element_set_shadow_preset(ui, 999, CUI_SHADOW_NONE, square));
            box(ui, collision, 10);
            cui_end_box(ui);
            assert(cui_last_error(ui) == (mode == 2 ? CUI_ERROR_DUPLICATE_ID
                                                              : CUI_ERROR_INVALID_ACTION));
            assert(!cui_element_set_shadow_preset(ui, owner, CUI_SHADOW_NONE, square));
        }
        if (mode == 4)
            assert(!cui_element_set_shadow_preset(ui, 999, CUI_SHADOW_NONE, square));
        cui_draw_list list = cui_end_frame(ui);
        assert(count(list, CUI_DRAW_DROP_SHADOW) == 1);
        cui_destroy(ui);
    }
}
static void legacy_clear_and_order(void) {
    cui_context *ui = create();
    cui_software *r = cui_software_create(96, 96, NULL);
    assert(r);
    unsigned char baseline[96 * 96 * 4];
    cui_draw_command commands[16];
    size_t baseline_count = 0;
    for (unsigned mode = 0; mode < 4; mode++) {
        assert(cui_begin_frame(ui, 96, 96, .016f));
        cui_begin_box(ui, 1, (cui_box){.padding = 10, .width = 40, .height = 40,
                                       .clip = true, .custom_opacity = true, .opacity = .5f});
        box(ui, 2, 20);
        assert(cui_set_underlay(ui, (cui_rect){2, 2, 4, 4}, (cui_color){1, 2, 3, 255}, square));
        assert(cui_set_outline(ui, 1, (cui_color){90, 80, 70, 255}, square));
        cui_end_box(ui);
        if (mode == 1)
            assert(cui_element_set_shadow_preset(ui, 2, CUI_SHADOW_NONE, square));
        if (mode >= 2) {
            cui_box_shadow mixed[] = {drop, {.kind = CUI_BOX_SHADOW_INSET,
                                             .layer = {.spread = 2, .color = {0, 0, 255, 255}}}};
            assert(cui_element_set_box_shadows(ui, 2, mixed, 2, square));
            if (mode == 2)
                assert(cui_element_set_box_shadows(ui, 2, NULL, 0, square));
        }
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        assert(cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
        const unsigned char *pixels = cui_software_pixels(r, NULL, NULL);
        if (mode == 0) {
            baseline_count = list.count;
            assert(list.count <= 16);
            memcpy(commands, list.commands, list.count * sizeof *commands);
            memcpy(baseline, pixels, sizeof baseline);
        } else if (mode < 3) {
            assert(list.count == baseline_count && memcmp(baseline, pixels, sizeof baseline) == 0);
            for (size_t i = 0; i < list.count; i++) {
                assert(list.commands[i].type == commands[i].type);
                assert(memcmp(&list.commands[i].bounds, &commands[i].bounds,
                              sizeof(cui_rect)) == 0);
                assert(memcmp(&list.commands[i].color, &commands[i].color,
                              sizeof(cui_color)) == 0);
            }
        } else {
            assert(count(list, CUI_DRAW_DROP_SHADOW) == 1 &&
                   count(list, CUI_DRAW_INSET_SHADOW) == 1);
            size_t d = SIZE_MAX, b = SIZE_MAX, in = SIZE_MAX, under = SIZE_MAX;
            for (size_t i = 0; i < list.count; i++) {
                const cui_draw_command *c = &list.commands[i];
                if (c->type == CUI_DRAW_DROP_SHADOW) {
                    d = i;
                    assert(c->shadow.layer.color.a == 127);
                }
                if (c->type == CUI_DRAW_INSET_SHADOW) {
                    in = i;
                    assert(c->shadow.layer.color.a == 127);
                }
                if (c->type == CUI_DRAW_RECT && c->color.r == 200)
                    b = i;
                if (c->type == CUI_DRAW_RECT && c->color.r == 1)
                    under = i;
            }
            assert(d < b && b < in && in < under);
            /* Ancestor clip and no self-overflow clipping of shadow. */
            size_t outside = ((size_t)45 * 96 + 45) * 4;
            assert(pixels[outside] == 255 && pixels[outside + 1] == 255);
            size_t inside = ((size_t)31 * 96 + 31) * 4;
            assert(pixels[inside] < 255);
        }
    }
    cui_software_destroy(r);
    cui_destroy(ui);
}
static void untouched_clear(void) {
    cui_context *ui = create();
    cui_draw_command before[8];
    size_t n = 0;
    for (unsigned mode = 0; mode < 2; mode++) {
        assert(cui_begin_frame(ui, 96, 96, .016f));
        box(ui, 1, 20);
        cui_interaction interaction = cui_interact(ui, 1, CUI_ROLE_BUTTON,
                                                   cui_string("same owner"), false, false, 0);
        assert(!interaction.activated);
        cui_end_box(ui);
        if (mode)
            assert(cui_element_set_shadow_preset(ui, 1, CUI_SHADOW_NONE, square));
        cui_draw_list list = cui_end_frame(ui);
        if (!mode) {
            n = list.count;
            assert(n <= 8);
            memcpy(before, list.commands, n * sizeof *before);
        } else {
            assert(list.count == n);
            for (size_t i = 0; i < n; i++) {
                assert(list.commands[i].type == before[i].type);
                assert(memcmp(&list.commands[i].bounds, &before[i].bounds, sizeof(cui_rect)) == 0);
                assert(memcmp(&list.commands[i].color, &before[i].color, sizeof(cui_color)) == 0);
            }
        }
    }
    cui_destroy(ui);
}
typedef struct { cui_context *ui; unsigned calls; bool rejected; } guard_state;
static bool prepaint(const cui_prepaint_info *info, const void *payload,
                     cui_prepaint_writer *writer) {
    (void)info;
    (void)writer;
    guard_state *s = *(guard_state *const *)payload;
    s->calls++;
    s->rejected = !cui_element_set_shadow_preset(s->ui, 1, CUI_SHADOW_MD, square);
    return true;
}
static cui_size measure(void *user, cui_str text, float font) {
    (void)text;
    guard_state *s = user;
    s->calls++;
    s->rejected = !cui_element_set_shadow_preset(s->ui, 1, CUI_SHADOW_MD, square);
    return (cui_size){font, font};
}
static void diagnostic(void *user, const cui_diagnostic *detail) {
    (void)detail;
    guard_state *s = user;
    s->calls++;
    s->rejected = !cui_element_set_shadow_preset(s->ui, 1, CUI_SHADOW_MD, square);
}
static void mutation_guards(void) {
    for (unsigned mode = 0; mode < 3; mode++) {
        guard_state s = {0};
        s.ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64,
                                       .measure = mode == 1 ? measure : NULL, .measure_user = &s});
        assert(s.ui && cui_begin_frame(s.ui, 96, 96, .016f));
        box(s.ui, 1, 20);
        if (mode == 0) {
            guard_state *ptr = &s;
            assert(cui_set_prepaint(s.ui, (cui_prepaint_options){.callback = prepaint,
                       .payload = &ptr, .payload_bytes = sizeof ptr, .command_budget = 1}));
        }
        if (mode == 1)
            cui_text(s.ui, cui_string("measure"), 12, (cui_color){0, 0, 0, 255});
        cui_end_box(s.ui);
        if (mode == 2) {
            cui_set_diagnostic_callback(s.ui, diagnostic, &s);
            assert(!cui_element_set_shadow_preset(s.ui, 999, CUI_SHADOW_MD, square));
        }
        cui_draw_list list = cui_end_frame(s.ui);
        assert(s.calls && s.rejected);
        assert(count(list, CUI_DRAW_DROP_SHADOW) == 0);
        cui_destroy(s.ui);
    }
}
static void accepted_input(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        cui_context *ui = create();
        for (unsigned frame = 0; frame < 2; frame++) {
            if (frame) {
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                      .x = 5, .y = 5}));
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                                      .x = 5, .y = 5}));
            }
            assert(cui_begin_frame(ui, 96, 96, .016f));
            box(ui, 1, 20);
            cui_interaction action = cui_interact(ui, 1, CUI_ROLE_BUTTON,
                                                    cui_string("owner"), false, false, 0);
            assert(action.activated == (frame != 0));
            cui_end_box(ui);
            if (styled)
                assert(cui_element_set_box_shadows(ui, 1, &drop, 1, square));
            cui_draw_list list = cui_end_frame(ui);
            assert(count(list, CUI_DRAW_DROP_SHADOW) == styled);
            assert(cui_last_error(ui) == CUI_OK);
        }
        cui_destroy(ui);
    }
}
static void suppressed_scope_and_capacity(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 2, .max_commands = 4});
    assert(ui && cui_begin_frame(ui, 96, 96, .016f));
    box(ui, 1, 20);
    assert(cui_element_set_box_shadows(ui, 1, &drop, 1, square));
    box(ui, 2, 10);
    box(ui, 3, 5); /* Failed begin: code in this scope cannot restyle another owner. */
    assert(!cui_element_set_shadow_preset(ui, 1, CUI_SHADOW_NONE, square));
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    /* Diagnostics are sticky until explicitly cleared. A later frame recovers. */
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 96, 96, .016f));
    box(ui, 1, 20);
    cui_end_box(ui);
    cui_box_shadow layers[8];
    for (size_t i = 0; i < 8; i++)
        layers[i] = drop;
    assert(cui_element_set_box_shadows(ui, 1, layers, 8, square));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 96, 96, .016f));
    box(ui, 1, 20);
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(count(list, CUI_DRAW_DROP_SHADOW) == 0 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
int main(void) {
    factory();
    closed_open_portal();
    invalid_and_copied();
    rejected_targets();
    legacy_clear_and_order();
    mutation_guards();
    untouched_clear();
    accepted_input();
    suppressed_scope_and_capacity();
    return 0;
}
