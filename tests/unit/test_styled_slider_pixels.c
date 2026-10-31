/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "styled_slider_fixture.h"
static bool inside(double x, double y, cui_rect thumb, double width, bool square) {
    if (width <= 0)
        return false;
    if (square)
        return x >= thumb.x - width && x < thumb.x + thumb.w + width && y >= thumb.y - width &&
               y < thumb.y + thumb.h + width &&
               !(x >= thumb.x && x < thumb.x + thumb.w && y >= thumb.y && y < thumb.y + thumb.h);
    double dx = x - (thumb.x + thumb.w * .5), dy = y - (thumb.y + thumb.h * .5),
           r2 = dx * dx + dy * dy;
    double inner = thumb.w * .5, outer = inner + width;
    return (r2 < outer * outer || (r2 == outer * outer && dx < 0)) &&
           !(r2 < inner * inner || (r2 == inner * inner && dx < 0));
}
static cui_rect expected(machine *m, double fraction) {
    float rem = m->style.style.rem, t = .375f * rem, cross = (1.5f * rem - t) * .5f;
    return m->options.orientation == CUI_SLIDER_MODEL_VERTICAL
               ? (cui_rect){cross - 5, (float)(1 - fraction) * m->length + 8 - rem, rem, rem}
               : (cui_rect){(float)fraction * m->length - 8, cross - 5, rem, rem};
}
static void rect(cui_rect a, cui_rect b) {
    assert(fabsf(a.x - b.x) < .02f && fabsf(a.y - b.y) < .02f && fabsf(a.w - b.w) < .02f &&
           fabsf(a.h - b.h) < .02f);
}
static unsigned rounded(double n) {
    return (unsigned)floor(n + .5);
}
static void pixels(void) {
    enum { WIDTH = 256, HEIGHT = 256, BYTES = WIDTH * HEIGHT * 4 };
    unsigned char *base = malloc(BYTES);
    assert(base);
    cui_software *r = cui_software_create(WIDTH, HEIGHT, NULL);
    assert(r);
    unsigned cases = 0;
    for (unsigned axis = 0; axis < 2; ++axis)
        for (unsigned size = 0; size < 2; ++size)
            for (unsigned square = 0; square < 2; ++square)
                for (unsigned clip = 0; clip < 2; ++clip)
                    for (unsigned opacity = 0; opacity < 2; ++opacity) {
                        machine *m = create();
                        m->length = 120;
                        m->value.kind = CUI_SLIDER_MODEL_SINGLE;
                        m->value.end = 1;
                        m->options.orientation = axis;
                        m->style.style.rem = size ? 24 : 16;
                        m->style.style.full_radius = square ? 0 : 9999;
                        m->style.style.layout = CUI_SLIDER_STYLE_PARENT;
                        m->clip = clip != 0;
                        m->opacity = opacity ? .5f : 1;
                        assert(cui_slider_palette_color_from_srgba8((cui_color){40, 0, 0, 255},
                                                                    &m->style.style.palette.bar));
                        assert(cui_slider_palette_color_from_srgba8(
                            (cui_color){80, 0, 0, 255}, &m->style.style.palette.thumb.solid));
                        m->style.style.palette.thumb.kind = CUI_SLIDER_BRUSH_SOLID;
                        assert(cui_slider_palette_color_from_srgba8((cui_color){90, 43, 177, 7},
                                                                    &m->style.style.palette.ring));
                        okay(frame(m, 0, NULL));
                        over(m, 1);
                        okay(frame(m, 0, NULL));
                        for (unsigned step = 0; step < 5; ++step) {
                            float progress = step * .25f, width = 3 * progress;
                            /* Owned valid motion seeded only after two real completed frames.
                             * Same-clock sampling isolates fractional raster geometry; actual
                             * elapsed/retarget/capture motion is proved by interaction/oracle. */
                            m->motion.thumbs[1].position = progress;
                            m->motion.thumbs[1].velocity = 0;
                            m->motion.thumbs[1].target = 1;
                            cui_draw_list list;
                            okay(frame(m, 0, &list));
                            assert(m->motion.thumbs[1].position == progress);
                            cui_rect thumb = expected(m, 1);
                            rect(cui_bounds(m->ui, part(CUI_SLIDER_MODEL_END_THUMB)), thumb);
                            rect(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_SURFACE)),
                                 (cui_rect){thumb.x + 1, thumb.y + 1, thumb.w - 2, thumb.h - 2});
                            if (width > 0)
                                rect(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_POINTER_RING)),
                                     (cui_rect){thumb.x - width, thumb.y - width,
                                                thumb.w + 2 * width, thumb.h + 2 * width});
                            else
                                assert(
                                    cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_POINTER_RING)).w ==
                                    0);
                            cui_draw_command *without = malloc(list.count * sizeof *without);
                            assert(without);
                            size_t count = 0;
                            unsigned rings = 0;
                            unsigned alpha = (unsigned)(rounded(.5 * progress * 255) * m->opacity);
                            for (size_t i = 0; i < list.count; ++i) {
                                const cui_draw_command *c = &list.commands[i];
                                if (c->type == CUI_DRAW_BORDER && c->color.r == 90 &&
                                    c->color.g == 43 && c->color.b == 177) {
                                    assert(c->stroke == width && c->color.a == alpha);
                                    ++rings;
                                } else
                                    without[count++] = *c;
                            }
                            assert(rings == (width > 0 ? 1u : 0u));
                            cui_draw_list baseline = {without, count, list.width, list.height};
                            assert(cui_software_render(r, &baseline, (cui_color){3, 5, 7, 255}));
                            memcpy(base, cui_software_pixels(r, NULL, NULL), BYTES);
                            assert(cui_software_render(r, &list, (cui_color){3, 5, 7, 255}));
                            const unsigned char *actual = cui_software_pixels(r, NULL, NULL);
                            unsigned changed = 0;
                            for (unsigned y = 0; y < HEIGHT; ++y)
                                for (unsigned x = 0; x < WIDTH; ++x) {
                                    size_t k = ((size_t)y * WIDTH + x) * 4;
                                    double px = x + .5, py = y + .5;
                                    bool painted = inside(px, py, thumb, width, square != 0);
                                    if (clip)
                                        painted =
                                            painted &&
                                            px < (axis ? 1.5 * m->style.style.rem : m->length) &&
                                            py < (axis ? m->length : 1.5 * m->style.style.rem);
                                    if (!painted)
                                        assert(!memcmp(actual + k, base + k, 4));
                                    else {
                                        const unsigned rgb[3] = {90, 43, 177};
                                        for (unsigned c = 0; c < 3; ++c) {
                                            unsigned expected = rounded(
                                                (rgb[c] * alpha + base[k + c] * (255 - alpha)) /
                                                255.);
                                            assert(abs((int)actual[k + c] - (int)expected) <= 1);
                                        }
                                        assert(actual[k + 3] == 255);
                                    }
                                    if (memcmp(actual + k, base + k, 4))
                                        ++changed;
                                }
                            assert(progress ? changed > 0 : changed == 0);
                            free(without);
                            ++cases;
                        }
                        destroy(m);
                    }
    printf("PASS %u actual Styled software images: both axes/rem/square/clip/opacity/float "
           "progress and alpha replacement\n",
           cases);
    cui_software_destroy(r);
    free(base);
}
static void focus_pixels(void) {
    enum { WIDTH = 256, HEIGHT = 256, BYTES = WIDTH * HEIGHT * 4 };
    unsigned char *base = malloc(BYTES);
    assert(base);
    cui_software *r = cui_software_create(WIDTH, HEIGHT, NULL);
    assert(r);
    unsigned cases = 0;
    for (unsigned axis = 0; axis < 2; ++axis)
        for (unsigned size = 0; size < 2; ++size)
            for (unsigned square = 0; square < 2; ++square)
                for (unsigned opacity = 0; opacity < 2; ++opacity) {
                    machine *m = create();
                    m->value.kind = CUI_SLIDER_MODEL_SINGLE;
                    m->length = 120;
                    m->options.orientation = axis;
                    m->style.style.rem = size ? 24 : 16;
                    m->style.style.full_radius = square ? 0 : 9999;
                    m->opacity = opacity ? .5f : 1;
                    assert(cui_slider_palette_color_from_srgba8((cui_color){90, 43, 177, 173},
                                                                &m->style.style.palette.ring));
                    okay(frame(m, 0, NULL));
                    cui_focus(m->ui, part(CUI_SLIDER_MODEL_END_THUMB));
                    key(m, CUI_KEY_LEFT);
                    cui_draw_list list;
                    okay(frame(m, 0, &list));
                    assert(m->count == 3 && !m->motion.next_wake_seconds);
                    cui_rect thumb = expected(m, .75),
                             ring = {thumb.x - 4, thumb.y - 4, thumb.w + 8, thumb.h + 8};
                    rect(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_FOCUS_RING)), ring);
                    assert(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_POINTER_RING)).w == 0);
                    cui_draw_command *without = malloc(list.count * sizeof *without);
                    assert(without);
                    size_t count = 0;
                    unsigned found = 0;
                    /* Existing Core prepaint opacity truncates the already quantized
                     * byte (173*.5 ->86); it does not round a second time. */
                    unsigned alpha = (unsigned)(173 * m->opacity);
                    for (size_t i = 0; i < list.count; ++i) {
                        const cui_draw_command *c = &list.commands[i];
                        if (c->type == CUI_DRAW_BORDER && c->color.r == 90 && c->color.g == 43 &&
                            c->color.b == 177) {
                            assert(c->stroke == 2 && c->color.a == alpha);
                            rect(c->bounds, ring);
                            ++found;
                        } else
                            without[count++] = *c;
                    }
                    assert(found == 1);
                    cui_draw_list baseline = {without, count, list.width, list.height};
                    assert(cui_software_render(r, &baseline, (cui_color){3, 5, 7, 255}));
                    memcpy(base, cui_software_pixels(r, NULL, NULL), BYTES);
                    assert(cui_software_render(r, &list, (cui_color){3, 5, 7, 255}));
                    const unsigned char *actual = cui_software_pixels(r, NULL, NULL);
                    cui_rect inner = {thumb.x - 2, thumb.y - 2, thumb.w + 4, thumb.h + 4};
                    unsigned changed = 0;
                    for (unsigned y = 0; y < HEIGHT; ++y)
                        for (unsigned x = 0; x < WIDTH; ++x) {
                            size_t k = ((size_t)y * WIDTH + x) * 4;
                            bool painted = inside(x + .5, y + .5, inner, 2, square != 0);
                            if (!painted)
                                assert(!memcmp(actual + k, base + k, 4));
                            else {
                                const unsigned rgb[3] = {90, 43, 177};
                                for (unsigned c = 0; c < 3; ++c) {
                                    unsigned expected = rounded(
                                        (rgb[c] * alpha + base[k + c] * (255 - alpha)) / 255.);
                                    assert(abs((int)actual[k + c] - (int)expected) <= 1);
                                }
                                assert(actual[k + 3] == 255);
                            }
                            if (memcmp(actual + k, base + k, 4))
                                ++changed;
                        }
                    assert(changed);
                    free(without);
                    destroy(m);
                    ++cases;
                }
    printf("PASS %u actual keyboard focus software images: 2px gap/stroke, authored alpha, no "
           "pointer animation\n",
           cases);
    cui_software_destroy(r);
    free(base);
}
int main(void) {
    pixels();
    focus_pixels();
    return 0;
}
