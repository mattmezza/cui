/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "cui/cui_stepper_visual.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static const cui_step steps[] = {
    {.title = CUI_STR("A")}, {.title = CUI_STR("B")}, {.title = CUI_STR("C")}};
static cui_size measure(void *u, cui_str s, float size) {
    (void)u;
    (void)size;
    return (cui_size){s.length * 8.f, 16};
}
typedef struct {
    cui_context *ui;
    size_t owner, calls, captured;
} fixture;
static void request(void *u, const cui_stepper_navigation_request *r) {
    fixture *f = u;
    assert(r->selected == f->captured && r->count == 3);
    assert(r->event.activation.target == cui_numeric_part_id(10, 1 + (unsigned)r->requested));
    f->owner = r->requested;
    f->calls++;
}
static cui_theme theme(void) {
    cui_theme t = cui_theme_light();
    t.on_primary = (cui_color){100, 150, 200, 255};
    return t;
}
static cui_draw_list frame(fixture *f, cui_stepper_visual_options o, const cui_step *s) {
    assert(cui_begin_frame(f->ui, 800, 400, .016f));
    cui_theme t = theme();
    f->captured = f->owner;
    cui_stepper_navigation_result r = cui_stepper_visual_navigation(
        f->ui, 10, &t, s, 3, f->owner, o, (cui_stepper_navigation_handler){request, f});
    assert(r.valid);
    cui_draw_list list = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return list;
}
static bool same(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void geometry(void) {
    const float diameter[] = {24, 8, 18, 32}, stroke[] = {2, 1.5f, 2, 3};
    const unsigned char rgba[] = {255, 255, 255, 128};
    for (unsigned vertical = 0; vertical < 2; vertical++)
        for (unsigned density = 0; density < 4; density++) {
            fixture f = {.ui = cui_create((cui_config){.measure = measure}), .owner = 1};
            assert(f.ui);
            cui_stepper_icon icons[3] = {{.present = true, .bitmap = {rgba, 1, 1, 4, 1}}};
            cui_draw_list list =
                frame(&f,
                      (cui_stepper_visual_options){.vertical = vertical != 0,
                                                   .density = (cui_stepper_density)density,
                                                   .icons = icons,
                                                   .icon_count = 3},
                      steps);
            /* Stack descriptors can disappear/change after submission: draw copies
             * bitmap metadata, while the byte buffer remains alive for render. */
            memset(icons, 0, sizeof icons);
            cui_theme t = theme();
            unsigned connectors = 0, indicators = 0, images = 0, ordinals = 0, checked = 0;
            for (size_t i = 0; i < list.count; i++) {
                const cui_draw_command *c = &list.commands[i];
                if (c->type == CUI_DRAW_RECT && c->bounds.w == diameter[density] &&
                    c->bounds.h == diameter[density]) {
                    indicators++;
                    checked += same(c->color, t.primary);
                }
                if (c->type == CUI_DRAW_RECT && c->bounds.w == (vertical ? stroke[density] : 36) &&
                    c->bounds.h == (vertical ? 20 : stroke[density])) {
                    assert(same(c->color, connectors ? t.border : t.primary));
                    /* Center between item indicators, no trailing separator. */
                    cui_rect first = cui_bounds(f.ui, cui_numeric_part_id(10, 1 + connectors));
                    assert(fabsf((vertical ? c->bounds.x + c->bounds.w / 2
                                           : c->bounds.y + c->bounds.h / 2) -
                                 (vertical ? first.x : first.y) - diameter[density] / 2) < .01f);
                    connectors++;
                    cui_software *r = cui_software_create(800, 400, NULL);
                    assert(r);
                    /* Independent expected single-rectangle pixel oracle. */
                    cui_draw_list one = {c, 1, 800, 400};
                    assert(cui_software_render(r, &one, (cui_color){0}));
                    unsigned w, h;
                    const unsigned char *p = cui_software_pixels(r, &w, &h);
                    unsigned x = (unsigned)(c->bounds.x + c->bounds.w / 2);
                    unsigned y = (unsigned)(c->bounds.y + c->bounds.h / 2);
                    assert(w == 800 && h == 400 && p[(y * w + x) * 4 + 3] > 0);
                    assert(p[(y * w + x) * 4] == c->color.r &&
                           p[(y * w + x) * 4 + 1] == c->color.g &&
                           p[(y * w + x) * 4 + 2] == c->color.b);
                    /* Raster the actual decorative stream; this fixture has no
                     * font, so omit TEXT commands rather than fake glyphs. */
                    cui_draw_command decoration[64];
                    assert(list.count <= 64);
                    size_t count = 0;
                    for (size_t k = 0; k < list.count; k++)
                        if (list.commands[k].type != CUI_DRAW_TEXT)
                            decoration[count++] = list.commands[k];
                    cui_draw_list composed = {decoration, count, 800, 400};
                    assert(cui_software_render(r, &composed, (cui_color){0}));
                    p = cui_software_pixels(r, NULL, NULL);
                    assert(p[(y * w + x) * 4] == c->color.r &&
                           p[(y * w + x) * 4 + 1] == c->color.g &&
                           p[(y * w + x) * 4 + 2] == c->color.b);
                    cui_software_destroy(r);
                }
                if (c->type == CUI_DRAW_IMAGE) {
                    images++;
                    assert(c->bitmap.bitmap.pixels == rgba && c->bitmap.custom_tint);
                    assert(same(c->bitmap.tint, t.on_primary));
                    assert(c->bounds.w == diameter[density] && c->bounds.h == diameter[density]);
                    cui_software *r = cui_software_create(800, 400, NULL);
                    assert(r);
                    cui_draw_list one = {c, 1, 800, 400};
                    assert(cui_software_render(r, &one, (cui_color){0}));
                    unsigned w;
                    const unsigned char *p = cui_software_pixels(r, &w, NULL);
                    size_t at = ((unsigned)(c->bounds.y + 2) * w + (unsigned)(c->bounds.x + 2)) * 4;
                    assert(p[at + 3] == 128 && p[at] == t.on_primary.r &&
                           p[at + 1] == t.on_primary.g && p[at + 2] == t.on_primary.b);
                    cui_software_destroy(r);
                }
                if (c->type == CUI_DRAW_TEXT && c->text.length == 1 && c->text.data[0] >= '1' &&
                    c->text.data[0] <= '3')
                    ordinals++;
            }
            assert(connectors == 2 && indicators == 3 && checked == 2);
            assert(images == (density == CUI_STEPPER_DENSITY_XS ? 0 : 1));
            assert(ordinals == (density == CUI_STEPPER_DENSITY_XS ? 0 : 2));
            size_t n;
            const cui_semantic *sem = cui_semantics(f.ui, &n);
            assert(n == 4);
            for (size_t i = 0; i < n; i++)
                assert(sem[i].id == 10 || sem[i].id == cui_numeric_part_id(10, 1) ||
                       sem[i].id == cui_numeric_part_id(10, 2) ||
                       sem[i].id == cui_numeric_part_id(10, 3));
            cui_destroy(f.ui);
        }
}
static void input(void) {
    for (unsigned mode = 0; mode < 6; mode++) {
        fixture f = {.ui = cui_create((cui_config){.measure = measure})};
        assert(f.ui);
        frame(&f, (cui_stepper_visual_options){0}, steps);
        cui_id target = cui_numeric_part_id(10, 2);
        if (mode < 2) {
            cui_rect b = cui_bounds(f.ui, target);
            cui_event e = {.type = CUI_EVENT_POINTER_DOWN,
                           .x = b.x + 2,
                           .y = b.y + 2,
                           .pointer_type = mode ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE};
            assert(cui_queue_event(f.ui, e));
            e.type = CUI_EVENT_POINTER_UP;
            assert(cui_queue_event(f.ui, e));
        } else {
            cui_focus(f.ui, target);
            cui_key key = mode == 2 ? CUI_KEY_SPACE : CUI_KEY_ENTER;
            assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
            if (key == CUI_KEY_SPACE)
                assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = key}));
        }
        cui_step copy[3];
        memcpy(copy, steps, sizeof copy);
        copy[1].disabled = mode == 5;
        cui_draw_list paint = frame(&f, (cui_stepper_visual_options){.disabled = mode == 4}, copy);
        unsigned checked = 0;
        cui_theme t = theme();
        for (size_t i = 0; i < paint.count; i++)
            if (paint.commands[i].type == CUI_DRAW_RECT && paint.commands[i].bounds.w == 24 &&
                paint.commands[i].bounds.h == 24 && same(paint.commands[i].color, t.primary))
                checked++;
        assert(checked == 1); /* Captured paint, including disabled paths. */
        assert(f.calls == (mode >= 4 ? 0 : 1));
        assert(f.owner == (mode >= 4 ? 0 : 1));
        size_t n;
        const cui_semantic *sem = cui_semantics(f.ui, &n);
        assert(n == 4 && sem[1].details.selected && !sem[2].details.selected);
        if (mode < 4) {
            /* Multiple and same-index requests still see the captured owner. */
            for (unsigned i = 0; i < 3; i++)
                assert(cui_queue_event(
                    f.ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                      .accessibility_target = cui_numeric_part_id(10, 1 + i),
                                      .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
            frame(&f, (cui_stepper_visual_options){0}, steps);
            assert(f.calls == 4 && f.owner == 2 && f.captured == 1);
        }
        cui_destroy(f.ui);
    }
}
static void design_sizes(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    assert(ui);
    cui_theme t = theme();
    for (unsigned size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; size++) {
        cui_design_metrics m;
        assert(cui_design_measure((cui_design_size)size, t.font_size / 16, &m));
        assert(cui_begin_frame(ui, 4096, 4096, .016f));
        assert(cui_stepper_visual_navigation(
                   ui, 10, &t, steps, 1, 0,
                   (cui_stepper_visual_options){.design_size = (cui_design_size)size},
                   (cui_stepper_navigation_handler){0})
                   .valid);
        cui_draw_list list = cui_end_frame(ui);
        bool indicator = false, title = false;
        for (size_t i = 0; i < list.count; i++) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_RECT && same(c->color, t.primary)) {
                assert(fabsf(c->bounds.w - 24 * m.indicator_size / 16) < .01f);
                assert(c->bounds.w == c->bounds.h);
                indicator = true;
            }
            if (c->type == CUI_DRAW_TEXT && c->text.length == 1 && c->text.data[0] == 'A') {
                assert(c->font_size == m.font_size);
                title = true;
            }
        }
        assert(indicator && title && cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}
static void invalid(void) {
    fixture f = {.ui = cui_create((cui_config){0})};
    assert(f.ui && cui_begin_frame(f.ui, 800, 400, .016f));
    cui_theme t = cui_theme_light();
    const unsigned char px[4] = {0};
    const cui_bitmap bad[] = {{0},
                              {px, 0, 1, 4, 0},
                              {px, 1, 0, 4, 0},
                              {px, 16385, 1, 65540, 0},
                              {px, 1, 1, 3, 0},
                              {px, 1, 2, SIZE_MAX, 0}};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        cui_stepper_icon icon = {.present = true, .bitmap = bad[i]};
        assert(!cui_stepper_visual_navigation(
                    f.ui, 10, &t, steps, 1, 0,
                    (cui_stepper_visual_options){.icons = &icon, .icon_count = 1},
                    (cui_stepper_navigation_handler){request, &f})
                    .valid);
    }
    assert(!cui_stepper_visual_navigation(f.ui, 10, &t, steps, 3, 0,
                                          (cui_stepper_visual_options){.icon_count = 3},
                                          (cui_stepper_navigation_handler){0})
                .valid);
    assert(!cui_stepper_visual_navigation(
                f.ui, 10, &t, steps, 3, 0,
                (cui_stepper_visual_options){.density = (cui_stepper_density)99},
                (cui_stepper_navigation_handler){0})
                .valid);
    assert(!cui_stepper_visual_navigation(
                f.ui, 10, &t, steps, 3, 0,
                (cui_stepper_visual_options){.design_size = (cui_design_size)99},
                (cui_stepper_navigation_handler){request, &f})
                .valid);
    size_t n;
    (void)cui_semantics(f.ui, &n);
    assert(!n && !f.calls);
    (void)cui_end_frame(f.ui);
    cui_destroy(f.ui);
}
int main(void) {
    geometry();
    design_sizes();
    input();
    invalid();
    return 0;
}
