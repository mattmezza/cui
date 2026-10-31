/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "cui/cui_stepper_presentation.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static const cui_step steps[] = {
    {.title = CUI_STR("Alpha")}, {.title = CUI_STR("Beta")}, {.title = CUI_STR("C")}};
static cui_size measure(void *u, cui_str s, float size) {
    (void)u;
    (void)size;
    return (cui_size){s.length * 8.f, 16};
}
static bool same(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
typedef struct {
    cui_context *ui;
    cui_theme theme;
    cui_stepper_visual_options visual;
    cui_stepper_presentation_options presentation;
    size_t selected, calls, captured;
    bool inert, omitted, blocker, modal, clipped;
} fixture;
static void request(void *data, const cui_stepper_navigation_request *r) {
    fixture *f = data;
    assert(r->selected == f->captured);
    f->selected = r->requested;
    f->calls++;
}
static fixture create(void) {
    fixture f = {.ui = cui_create((cui_config){.measure = measure}), .theme = cui_theme_light()};
    assert(f.ui);
    f.theme.secondary = (cui_color){20, 30, 40, 255};
    f.theme.secondary_hover = (cui_color){50, 60, 70, 255};
    f.theme.buttons[CUI_THEME_BUTTON_SECONDARY].active = (cui_color){80, 90, 100, 255};
    f.presentation.layout = CUI_STEPPER_LAYOUT_FILL;
    return f;
}
static cui_draw_list frame(fixture *f, float width) {
    assert(cui_begin_frame(f->ui, width, 500, .016f));
    cui_begin_box(
        f->ui, 77,
        (cui_box){
            .width = -1, .height = f->clipped ? 12 : 0, .clip = f->clipped, .inert = f->inert});
    f->captured = f->selected;
    if (!f->omitted)
        assert(cui_stepper_present(f->ui, 10, &f->theme, steps, 3, f->selected, f->visual,
                                   f->presentation, (cui_stepper_navigation_handler){request, f})
                   .valid);
    cui_end_box(f->ui);
    if (f->blocker) {
        /* Independent passive overlay, no fake input/semantic event. */
        cui_begin_portal(f->ui, 900, (cui_box){.background = {7, 17, 27, 255}},
                         (cui_rect){0, 0, width, 500}, 10);
        assert(cui_set_hover_hitbox(f->ui, true));
        cui_end_box(f->ui);
    }
    if (f->modal) {
        assert(cui_begin_focus_scope(f->ui, 950, true));
        cui_begin_portal(f->ui, 951, (cui_box){0}, (cui_rect){700, 400, 20, 20}, 20);
        cui_end_box(f->ui);
        cui_end_focus_scope(f->ui);
    }
    cui_draw_list list = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return list;
}
static cui_color indicator(fixture *f, cui_draw_list list, unsigned i) {
    cui_rect b = cui_bounds(f->ui, cui_numeric_part_id(10, 1000 + i));
    for (size_t k = 0; k < list.count; k++) {
        const cui_draw_command *c = &list.commands[k];
        if (c->type == CUI_DRAW_RECT && c->bounds.x == b.x && c->bounds.y == b.y &&
            c->bounds.w == b.w && c->bounds.h == b.h && c->color.a) {
            cui_software *software = cui_software_create(800, 500, NULL);
            assert(software);
            cui_draw_list single = {c, 1, list.width, list.height};
            assert(cui_software_render(software, &single, (cui_color){0}));
            unsigned stride;
            const unsigned char *pixels = cui_software_pixels(software, &stride, NULL);
            size_t at = ((size_t)(b.y + b.h / 2) * stride + (size_t)(b.x + b.w / 2)) * 4;
            assert(pixels[at] == c->color.r && pixels[at + 1] == c->color.g &&
                   pixels[at + 2] == c->color.b && pixels[at + 3] == c->color.a);
            cui_software_destroy(software);
            return c->color;
        }
    }
    assert(false);
    return (cui_color){0};
}
static void pointer(fixture *f, cui_event_type type, float x, float y) {
    assert(cui_queue_event(
        f->ui, (cui_event){.type = type, .x = x, .y = y, .pointer_type = CUI_POINTER_MOUSE}));
}
static void interactions(void) {
    fixture f = create();
    cui_draw_list list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    cui_rect b = cui_bounds(f.ui, cui_numeric_part_id(10, 1001));
    cui_rect trigger = cui_bounds(f.ui, cui_numeric_part_id(10, 2));
    pointer(&f, CUI_EVENT_POINTER_MOVE, b.x + 12, b.y + 12);
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary_hover));
    f.blocker = true;
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    cui_draw_command decoration[64];
    assert(list.count <= 64);
    size_t decorative_count = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type != CUI_DRAW_TEXT)
            decoration[decorative_count++] = list.commands[i];
    cui_draw_list composed = {decoration, decorative_count, 600, 500};
    cui_software *r = cui_software_create(600, 500, NULL);
    assert(r && cui_software_render(r, &composed, (cui_color){0}));
    const unsigned char *rgba = cui_software_pixels(r, NULL, NULL);
    size_t pixel = ((size_t)(b.y + 12) * 600 + (size_t)(b.x + 12)) * 4;
    assert(rgba[pixel] == 7 && rgba[pixel + 1] == 17 && rgba[pixel + 2] == 27 &&
           rgba[pixel + 3] == 255);
    cui_software_destroy(r);
    f.blocker = false;
    f.modal = true;
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    f.modal = false;
    pointer(&f, CUI_EVENT_POINTER_MOVE, trigger.x + 2, trigger.y + trigger.h - 2);
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary)); /* Label-only hover. */
    pointer(&f, CUI_EVENT_POINTER_DOWN, trigger.x + 2, trigger.y + trigger.h - 2);
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    pointer(&f, CUI_EVENT_POINTER_MOVE, b.x + 12, b.y + 12);
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.buttons[CUI_THEME_BUTTON_SECONDARY].active));
    f.inert = true;
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    f.inert = false;
    pointer(&f, CUI_EVENT_CANCEL, b.x + 12, b.y + 12);
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary_hover) && !f.calls);
    f.visual.disabled = true;
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    f.visual.disabled = false;
    frame(&f, 600);
    cui_focus(f.ui, cui_numeric_part_id(10, 2));
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.buttons[CUI_THEME_BUTTON_SECONDARY].active));
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    list = frame(&f, 600);
    assert(f.calls == 1 && f.selected == 1);
    assert(same(indicator(&f, list, 1), f.theme.secondary_hover)); /* Captured selected0. */
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.primary));
    f.omitted = true;
    frame(&f, 600);
    f.omitted = false;
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 2), f.theme.secondary));
    size_t n;
    (void)cui_semantics(f.ui, &n);
    assert(n == 4);
    cui_destroy(f.ui);
}
static void geometry(void) {
    const float widths[] = {600, 240, 700};
    for (unsigned vertical = 0; vertical < 2; vertical++)
        for (unsigned density = 0; density < 4; density++) {
            fixture f = create();
            f.visual.vertical = vertical != 0;
            f.visual.density = (cui_stepper_density)density;
            f.presentation.height = vertical ? 300 : 0;
            const float sides[] = {24, 8, 18, 32}, strokes[] = {2, 1.5f, 2, 3};
            for (size_t w = 0; w < 3; w++) {
                cui_draw_list list = frame(&f, widths[w]);
                assert(cui_bounds(f.ui, 10).w == widths[w]);
                size_t connectors = 0;
                for (size_t k = 0; k < list.count; k++) {
                    const cui_draw_command *c = &list.commands[k];
                    if (c->type != CUI_DRAW_RECT || !same(c->color, f.theme.border))
                        continue;
                    assert(connectors < 2);
                    cui_rect item =
                        cui_bounds(f.ui, cui_numeric_part_id(10, 2000 + (unsigned)connectors));
                    float side = sides[density], stroke = strokes[density];
                    assert(fabsf(c->bounds.x - item.x - (vertical ? side / 2 : side + 4)) < .01f);
                    assert(fabsf(c->bounds.y - item.y - (vertical ? side + 4 : side / 2)) < .01f);
                    assert(fabsf(c->bounds.w - (vertical ? stroke : item.w - side - 8)) < .01f);
                    assert(fabsf(c->bounds.h - (vertical ? item.h - side - 8 : stroke)) < .01f);
                    connectors++;
                }
                assert(connectors == 2);
                size_t n;
                (void)cui_semantics(f.ui, &n);
                assert(n == 4);
            }
            f.theme.radius = 0;
            cui_draw_list list = frame(&f, 600);
            for (size_t k = 0; k < list.count; k++)
                if (list.commands[k].type == CUI_DRAW_RECT)
                    assert(list.commands[k].radius == 0);
            cui_destroy(f.ui);
        }
}
static void clipping(void) {
    fixture f = create();
    f.clipped = true;
    frame(&f, 600);
    cui_rect b = cui_bounds(f.ui, cui_numeric_part_id(10, 1001));
    assert(b.h == 24);
    pointer(&f, CUI_EVENT_POINTER_MOVE, b.x + 12, b.y + 18);
    cui_draw_list list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    pointer(&f, CUI_EVENT_POINTER_MOVE, b.x + 12, b.y + 6);
    list = frame(&f, 600);
    assert(same(indicator(&f, list, 1), f.theme.secondary_hover));
    /* Render the actual clipped decorative stream, excluding unsupported text
     * from this deliberately font-free software fixture. */
    cui_draw_command commands[64];
    size_t count = 0;
    assert(list.count <= 64);
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type != CUI_DRAW_TEXT)
            commands[count++] = list.commands[i];
    cui_draw_list decorated = {commands, count, 600, 500};
    cui_software *r = cui_software_create(600, 500, NULL);
    assert(r);
    assert(cui_software_render(r, &decorated, (cui_color){0}));
    const unsigned char *p = cui_software_pixels(r, NULL, NULL);
    size_t inside = ((size_t)(b.y + 6) * 600 + (size_t)(b.x + 12)) * 4;
    size_t outside = ((size_t)(b.y + 18) * 600 + (size_t)(b.x + 12)) * 4;
    assert(p[inside] == f.theme.secondary_hover.r && p[inside + 3] == 255);
    assert(p[outside + 3] == 0);
    cui_software_destroy(r);
    cui_destroy(f.ui);
}
static void invalid(void) {
    fixture f = create();
    const cui_stepper_presentation_options bad[] = {
        {.layout = (cui_stepper_layout)7},
        {.width = 1},
        {.layout = CUI_STEPPER_LAYOUT_FILL, .width = -1},
        {.layout = CUI_STEPPER_LAYOUT_FILL, .height = NAN},
        {.layout = CUI_STEPPER_LAYOUT_FILL, .width = INFINITY}};
    assert(cui_begin_frame(f.ui, 600, 500, .016f));
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++)
        assert(!cui_stepper_present(f.ui, 10, &f.theme, steps, 3, 0, f.visual, bad[i],
                                    (cui_stepper_navigation_handler){request, &f})
                    .valid);
    const unsigned char pixel[4] = {0};
    cui_stepper_icon bad_icon = {.present = true, .bitmap = {pixel, 1, 2, SIZE_MAX, 0}};
    const cui_stepper_visual_options invalid_visual[] = {{.density = (cui_stepper_density)-1},
                                                         {.design_size = (cui_design_size)99},
                                                         {.icon_count = 3},
                                                         {.icons = &bad_icon, .icon_count = 1}};
    for (size_t i = 0; i < sizeof invalid_visual / sizeof *invalid_visual; i++)
        assert(!cui_stepper_present(f.ui, 10, &f.theme, steps, 1, 0, invalid_visual[i],
                                    f.presentation, (cui_stepper_navigation_handler){request, &f})
                    .valid);
    size_t n;
    (void)cui_semantics(f.ui, &n);
    assert(!n && !f.calls);
    assert(cui_stepper_present(f.ui, 10, &f.theme, steps, 3, 0, f.visual, f.presentation,
                               (cui_stepper_navigation_handler){request, &f})
               .valid);
    assert(cui_end_frame(f.ui).count);
    cui_destroy(f.ui);
}
static void sizing_palette_lifetime(void) {
    fixture f = create();
    f.presentation.custom_palette = true;
    f.presentation.palette = (cui_stepper_indicator_palette){
        {101, 12, 13, 255}, {21, 112, 23, 255}, {31, 32, 123, 255}, {211, 212, 213, 255}};
    for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; ++size) {
        f.visual.design_size = (cui_design_size)size;
        cui_design_metrics m;
        assert(cui_design_measure((cui_design_size)size, f.theme.font_size / 16, &m));
        cui_draw_list list = frame(&f, 800);
        assert(same(indicator(&f, list, 1), f.presentation.palette.normal));
        cui_rect b = cui_bounds(f.ui, cui_numeric_part_id(10, 1001));
        assert(fabsf(b.w - 24 * m.indicator_size / 16) < .01f);
    }
    f.visual.design_size = CUI_DESIGN_INHERIT;
    static const unsigned char pixels[] = {255, 255, 255, 128};
    cui_stepper_icon icons[3] = {{0}, {.present = true, .bitmap = {pixels, 1, 1, 4, 17}}, {0}};
    f.visual.icons = icons;
    f.visual.icon_count = 3;
    cui_draw_list list = frame(&f, 800);
    memset(icons, 0, sizeof icons); /* Prepaint never retains the array. */
    bool found = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_IMAGE) {
            const cui_draw_command *c = &list.commands[i];
            assert(c->bitmap.bitmap.pixels == pixels && c->bitmap.bitmap.revision == 17);
            assert(c->bitmap.custom_tint &&
                   same(c->bitmap.tint, f.presentation.palette.foreground));
            assert(c->custom_corners && c->corner_radii.top_left == 12);
            cui_software *r = cui_software_create(800, 500, NULL);
            assert(r);
            cui_draw_list one = {c, 1, 800, 500};
            assert(cui_software_render(r, &one, (cui_color){0}));
            const unsigned char *rgba = cui_software_pixels(r, NULL, NULL);
            size_t at = ((size_t)(c->bounds.y + 12) * 800 + (size_t)(c->bounds.x + 12)) * 4;
            assert(rgba[at] == 211 && rgba[at + 1] == 212 && rgba[at + 2] == 213 &&
                   rgba[at + 3] == 128);
            cui_software_destroy(r);
            found = true;
        }
    assert(found);
    cui_destroy(f.ui);
}
static void omission_and_boundaries(void) {
    fixture f = create();
    frame(&f, 600);
    cui_rect b = cui_bounds(f.ui, cui_numeric_part_id(10, 1001));
    pointer(&f, CUI_EVENT_POINTER_DOWN, b.x + 12, b.y + 12);
    assert(
        same(indicator(&f, frame(&f, 600), 1), f.theme.buttons[CUI_THEME_BUTTON_SECONDARY].active));
    f.omitted = true;
    frame(&f, 600);
    pointer(&f, CUI_EVENT_POINTER_UP, b.x + 12, b.y + 12);
    f.omitted = false;
    frame(&f, 600);
    assert(!f.calls);
    assert(cui_begin_frame(f.ui, 800, 500, .016f));
    cui_step many[100];
    for (size_t i = 0; i < 100; i++)
        many[i] = (cui_step){.title = CUI_STR("x")};
    assert(!cui_stepper_present(f.ui, 10, &f.theme, many, 101, 0, f.visual, f.presentation,
                                (cui_stepper_navigation_handler){0})
                .valid);
    assert(cui_stepper_present(f.ui, 10, &f.theme, many, 100, 99, f.visual, f.presentation,
                               (cui_stepper_navigation_handler){0})
               .valid);
    (void)cui_end_frame(f.ui);
    assert(cui_last_error(f.ui) == CUI_OK);
    for (size_t count = 0; count < 2; count++) {
        assert(cui_begin_frame(f.ui, 800, 500, .016f));
        assert(cui_stepper_present(f.ui, 10, &f.theme, many, count, 0, f.visual, f.presentation,
                                   (cui_stepper_navigation_handler){0})
                   .valid);
        cui_draw_list list = cui_end_frame(f.ui);
        for (size_t i = 0; i < list.count; i++)
            assert(list.commands[i].type != CUI_DRAW_RECT ||
                   !same(list.commands[i].color, f.theme.border));
    }
    cui_destroy(f.ui);
}
static void unequal_height_alignment(void) {
    fixture f = create();
    const cui_step unequal[] = {{.title = CUI_STR("Alpha"), .description = CUI_STR("Description")},
                                {.title = CUI_STR("Beta")},
                                {.title = CUI_STR("C")}};
    const float explicit_heights[] = {0, 120};
    for (size_t h = 0; h < 2; h++) {
        f.presentation.height = explicit_heights[h];
        assert(cui_begin_frame(f.ui, 600, 500, .016f));
        assert(cui_stepper_present(f.ui, 10, &f.theme, unequal, 3, 0, f.visual, f.presentation,
                                   (cui_stepper_navigation_handler){0})
                   .valid);
        cui_draw_list list = cui_end_frame(f.ui);
        assert(cui_last_error(f.ui) == CUI_OK);
        cui_rect root = cui_bounds(f.ui, 10);
        /* Host metric16: short trigger24+gap4+title16=44; long adds
         * description16+gap4=64. Root intrinsic maximum64, explicit120. */
        assert(root.h == (h ? 120 : 64));
        for (unsigned i = 0; i < 3; i++) {
            cui_rect item = cui_bounds(f.ui, cui_numeric_part_id(10, 2000 + i));
            cui_rect trigger = cui_bounds(f.ui, cui_numeric_part_id(10, 1 + i));
            assert(item.h == (i ? 44 : 64));
            assert(fabsf(item.y + item.h / 2 - root.y - root.h / 2) < .01f);
            assert(trigger.y == item.y && trigger.h == item.h);
        }
        unsigned connectors = 0;
        for (size_t i = 0; i < list.count; i++) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type != CUI_DRAW_RECT || !same(c->color, f.theme.border))
                continue;
            assert(connectors < 2);
            cui_rect item = cui_bounds(f.ui, cui_numeric_part_id(10, 2000 + connectors));
            assert(c->bounds.x == item.x + 28 && c->bounds.y == item.y + 12);
            assert(c->bounds.w == item.w - 32 && c->bounds.h == 2);
            connectors++;
        }
        assert(connectors == 2);
    }
    cui_destroy(f.ui);
}
static void multiple_requests(void) {
    fixture f = create();
    frame(&f, 600);
    for (unsigned i = 0; i < 3; i++)
        assert(cui_queue_event(f.ui,
                               (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = cui_numeric_part_id(10, 1 + i),
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    cui_draw_list list = frame(&f, 600);
    assert(f.calls == 3 && f.selected == 2 && f.captured == 0);
    assert(same(indicator(&f, list, 0), f.theme.primary));
    assert(same(indicator(&f, list, 1), f.theme.secondary));
    size_t n;
    const cui_semantic *s = cui_semantics(f.ui, &n);
    assert(n == 4 && s[1].details.selected && !s[2].details.selected && !s[3].details.selected);
    cui_destroy(f.ui);
}
static void content_geometry_compatibility(void) {
    fixture old = create(), modern = create();
    modern.presentation.layout = CUI_STEPPER_LAYOUT_CONTENT;
    assert(cui_begin_frame(old.ui, 600, 500, .016f));
    assert(cui_stepper_visual_navigation(old.ui, 10, &old.theme, steps, 3, 0, old.visual,
                                         (cui_stepper_navigation_handler){0})
               .valid);
    cui_draw_list a = cui_end_frame(old.ui);
    cui_draw_list b = frame(&modern, 600);
    size_t ai = 0, bi = 0, compared = 0;
    while (ai < a.count && bi < b.count) {
        while (ai < a.count && a.commands[ai].type != CUI_DRAW_RECT &&
               a.commands[ai].type != CUI_DRAW_TEXT)
            ai++;
        while (bi < b.count && b.commands[bi].type != CUI_DRAW_RECT &&
               b.commands[bi].type != CUI_DRAW_TEXT)
            bi++;
        if (ai == a.count || bi == b.count)
            break;
        const cui_draw_command *x = &a.commands[ai++], *y = &b.commands[bi++];
        assert(x->type == y->type && x->bounds.x == y->bounds.x && x->bounds.y == y->bounds.y &&
               x->bounds.w == y->bounds.w && x->bounds.h == y->bounds.h && x->radius == y->radius &&
               x->font_size == y->font_size && x->text.length == y->text.length);
        if (x->text.length)
            assert(!memcmp(x->text.data, y->text.data, x->text.length));
        compared++;
    }
    while (ai < a.count && a.commands[ai].type != CUI_DRAW_RECT &&
           a.commands[ai].type != CUI_DRAW_TEXT)
        ai++;
    while (bi < b.count && b.commands[bi].type != CUI_DRAW_RECT &&
           b.commands[bi].type != CUI_DRAW_TEXT)
        bi++;
    assert(ai == a.count && bi == b.count && compared == 11);
    cui_destroy(old.ui);
    cui_destroy(modern.ui);
}
static void custom_press_and_capacity(void) {
    for (unsigned touch = 0; touch < 2; touch++) {
        fixture f = create();
        f.presentation.custom_palette = true;
        f.presentation.palette = (cui_stepper_indicator_palette){
            {11, 22, 33, 255}, {44, 55, 66, 255}, {77, 88, 99, 255}, {111, 122, 133, 255}};
        frame(&f, 600);
        cui_rect b = cui_bounds(f.ui, cui_numeric_part_id(10, 1001));
        assert(cui_queue_event(
            f.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                              .x = b.x + 12,
                              .y = b.y + 12,
                              .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE}));
        cui_draw_list list = frame(&f, 600);
        assert(same(indicator(&f, list, 1), f.presentation.palette.pressed));
        assert(cui_queue_event(
            f.ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                              .x = b.x + 12,
                              .y = b.y + 12,
                              .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE}));
        list = frame(&f, 600);
        assert(f.calls == 1 && f.selected == 1);
        assert(!same(indicator(&f, list, 1), f.theme.primary));
        list = frame(&f, 600);
        assert(same(indicator(&f, list, 1), f.theme.primary));
        f.visual.disabled = true;
        list = frame(&f, 600);
        assert(same(indicator(&f, list, 1), f.theme.primary));
        cui_destroy(f.ui);
    }
    fixture f = create();
    cui_destroy(f.ui);
    f.ui = cui_create((cui_config){.measure = measure, .max_commands = 8});
    assert(f.ui);
    assert(cui_begin_frame(f.ui, 600, 500, .016f));
    assert(cui_stepper_present(f.ui, 10, &f.theme, steps, 3, 0, f.visual, f.presentation,
                               (cui_stepper_navigation_handler){0})
               .valid);
    assert(cui_end_frame(f.ui).count == 0 && cui_last_error(f.ui) != CUI_OK);
    cui_clear_error(f.ui);
    assert(cui_begin_frame(f.ui, 600, 500, .016f));
    assert(cui_stepper_present(f.ui, 10, &f.theme, NULL, 0, 0, f.visual, f.presentation,
                               (cui_stepper_navigation_handler){0})
               .valid);
    (void)cui_end_frame(f.ui);
    assert(cui_last_error(f.ui) == CUI_OK);
    cui_destroy(f.ui);
}
int main(void) {
    unequal_height_alignment();
    multiple_requests();
    content_geometry_compatibility();
    custom_press_and_capacity();
    clipping();
    sizing_palette_lifetime();
    omission_and_boundaries();
    geometry();
    interactions();
    invalid();
    return 0;
}
