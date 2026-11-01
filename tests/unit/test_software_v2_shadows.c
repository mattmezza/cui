/* SPDX-License-Identifier: MIT */
#include <stddef.h>
static size_t kernel_calls, fail_kernel_call;
#define CUI_SOFTWARE_V2_SHADOW_SAMPLE_HOOK()                                                       \
    do {                                                                                           \
        ++kernel_calls;                                                                            \
        if (fail_kernel_call && kernel_calls == fail_kernel_call)                                  \
            return CUI_ERROR_INVALID_ACTION;                                                       \
    } while (0)
#define main ordinary_paint_fixture_main
#include "test_software_v2_paint.c"
#undef main
static cui_draw_command_v2 shadow(cui_draw_type kind) {
    cui_draw_command_v2 c = legacy(kind, (cui_rect){NAN, NAN, NAN, NAN}, (cui_color){0});
    c.payload.legacy.shadow = (cui_drop_shadow_paint){.caster = {2.25f, 2.5f, 4.5f, 3.25f},
                                                      .radii = {.5f, 1.5f, 1, .25f},
                                                      .layer = {.offset_x = .375f,
                                                                .offset_y = -.25f,
                                                                .sigma = 1.5f,
                                                                .spread = .25f,
                                                                .color = {40, 80, 120, 173}}};
    return c;
}
static void shadow_frames(void) {
    unsigned char actual[CAPACITY], expected[CAPACITY];
    cui_color clears[] = {{11, 19, 27, 0}, {11, 19, 27, 73}};
    const float transforms[][4] = {{1, 1, 0, 0},
                                   {1, 1, .375f, -.625f},
                                   {2, .75f, -3, .125f},
                                   {.5f, 3, .25f, -6},
                                   {1.f / 65536, 4, .5f - 4.5f / 65536, -8},
                                   {64, 1.f / 65536, -144, .5f - 4.f / 65536}};
    for (unsigned f = 0; f < sizeof transforms / sizeof *transforms; ++f)
        for (unsigned kind = 0; kind < 2; ++kind)
            for (unsigned sigma = 0; sigma < 2; ++sigma)
                for (unsigned clear = 0; clear < 2; ++clear) {
                    cui_draw_command_v2 c[18], ordinary[12];
                    size_t n = fixture(ordinary);
                    /* Preserve ordinary order, interleave shadows and detached tail. */
                    c[0] = shadow(kind ? CUI_DRAW_INSET_SHADOW : CUI_DRAW_DROP_SHADOW);
                    if (!sigma)
                        c[0].payload.legacy.shadow.layer.sigma = 0;
                    memcpy(c + 1, ordinary, 3 * sizeof *c);
                    memcpy(c + 5, ordinary + 3, (n - 3) * sizeof *c);
                    c[4] = shadow(kind ? CUI_DRAW_DROP_SHADOW : CUI_DRAW_INSET_SHADOW);
                    if (!sigma)
                        c[4].payload.legacy.shadow.layer.sigma = 0;
                    c[n + 2] = shadow(CUI_DRAW_DROP_SHADOW);
                    c[n + 2].payload.legacy.shadow.caster.x += 2;
                    c[n + 2].payload.legacy.shadow.layer.spread = -.5f;
                    cui_draw_list_v2 l = list(c, n + 3);
                    cui_render_v2_options o = options();
                    o.scale_x = transforms[f][0];
                    o.scale_y = transforms[f][1];
                    o.origin_x = transforms[f][2];
                    o.origin_y = transforms[f][3];
                    memset(actual, 0xa7, sizeof actual);
                    memset(expected, 0xa7, sizeof expected);
                    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
                    allocator a = {.check_reentrant = true};
                    kernel_calls = 0;
                    assert(paint(&t, &l, &o, clears[clear], &a) == CUI_OK);
                    assert(a.calls == 2 && a.frees == 2 && !a.live);
                    /* Exact two-block peak with interleaved BG/A8 producers. */
                    size_t prepared = 0, shadow_count = 0, depth = 0;
                    cui_software_v2_clip clips[64], clip = cui_software_v2_clip_all();
                    for (size_t k = 0; k < l.count; ++k) {
                        if (c[k].kind == CUI_DRAW_CLIP_PUSH) {
                            clips[depth++] = clip;
                            clip = cui_software_v2_clip_push(clip, c[k].payload.legacy.bounds);
                        } else if (c[k].kind == CUI_DRAW_CLIP_POP)
                            clip = clips[--depth];
                        else if (c[k].kind == CUI_DRAW_V2_BACKGROUND) {
                            cui_background_prepared_query q;
                            assert(cui_background_prepare_query(c[k].payload.background, &o,
                                                                cui_software_v2_clip_cover(clip),
                                                                &q) == CUI_OK);
                            prepared += q.bytes;
                        } else if (cui_shadow_type((cui_draw_type)c[k].kind)) {
                            cui_software_v2_shadow_descriptor d;
                            assert(cui_software_v2_shadow_query(
                                       (cui_draw_type)c[k].kind, c[k].payload.legacy.shadow, &o,
                                       (cui_software_v2_shadow_clip){clip.left, clip.top,
                                                                     clip.right, clip.bottom},
                                       CUI_SOFTWARE_V2_SHADOW_CELLS, CUI_SOFTWARE_V2_SHADOW_TAPS,
                                       &d) == CUI_OK);
                            prepared += d.bytes;
                            ++shadow_count;
                        }
                    }
                    size_t desc = l.count * sizeof(cui_software_v2_paint_descriptor);
                    size_t align = _Alignof(cui_software_v2_shadow_descriptor);
                    assert(a.requested[0] ==
                           l.count * sizeof(cui_draw_command_v2) + WIDTH * HEIGHT * 4 + prepared);
                    assert(a.requested[1] ==
                           desc + (align - desc % align) % align +
                               shadow_count * sizeof(cui_software_v2_shadow_descriptor));
                    reference(&l, &o, clears[clear], expected);
                    equal_pixels(actual, expected);
                }
}
static void hand_pixels(void) {
    unsigned char actual[CAPACITY];
    cui_render_v2_options o = options();
    o.scale_x = .5f;
    cui_draw_command_v2 c = shadow(CUI_DRAW_DROP_SHADOW);
    c.payload.legacy.shadow =
        (cui_drop_shadow_paint){.caster = {0, 0, 1, 1}, .layer = {.color = {40, 80, 120, 173}}};
    cui_draw_list_v2 l = list(&c, 1);
    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
    allocator a = {0};
    memset(actual, 0xa7, sizeof actual);
    assert(paint(&t, &l, &o, (cui_color){11, 19, 27, 0}, &a) == CUI_OK);
    const unsigned char first[] = {40, 80, 120, 87}, outside[] = {11, 19, 27, 0};
    assert(memcmp(actual, first, 4) == 0 && memcmp(actual + 4, outside, 4) == 0);
    assert(kernel_calls > 0);
    /* Source alpha0 inside support blends/canonicalizes once; outside skips. */
    o = options();
    c.payload.legacy.shadow.caster = (cui_rect){0, 0, 10, 10};
    c.payload.legacy.shadow.radii = (cui_corner_radii){5, 5, 5, 5};
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){11, 19, 27, 0}, &a) == CUI_OK);
    const unsigned char zero[] = {0, 0, 0, 0};
    assert(memcmp(actual, zero, 4) == 0 && memcmp(actual + 12 * 4, outside, 4) == 0);
    c.payload.legacy.shadow.layer.color.a = 0;
    a = (allocator){0};
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){11, 19, 27, 0}, &a) == CUI_OK && !kernel_calls);
    assert(memcmp(actual, outside, 4) == 0);
    /* Empty inset hole preserves source empty output, not a filled element. */
    c = shadow(CUI_DRAW_INSET_SHADOW);
    c.payload.legacy.shadow.layer.spread = 8;
    l = list(&c, 1);
    a = (allocator){0};
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){11, 19, 27, 0}, &a) == CUI_OK && !kernel_calls);
    assert(memcmp(actual, outside, 4) == 0);
}
static void large_position_pixels(void) {
    unsigned char actual[CAPACITY], expected[CAPACITY];
    for (int sign = -1; sign <= 1; sign += 2)
        for (unsigned kind = 0; kind < 2; ++kind)
            for (unsigned sigma = 0; sigma < 2; ++sigma) {
                cui_draw_command_v2 c = shadow(kind ? CUI_DRAW_INSET_SHADOW : CUI_DRAW_DROP_SHADOW);
                c.payload.legacy.shadow.caster = (cui_rect){sign * 16384.f, sign * 15000.f, 4, 4};
                if (!sigma)
                    c.payload.legacy.shadow.layer.sigma = 0;
                cui_render_v2_options o = options();
                o.scale_x = nextafterf(1.f / 65536, INFINITY);
                o.scale_y = 64;
                o.origin_x = .5f - o.scale_x * (sign * 16384.f + 2);
                o.origin_y = -sign * 960000.f;
                cui_draw_list_v2 l = list(&c, 1);
                cui_color clear = {11, 19, 27, 73};
                memset(actual, 0xa7, sizeof actual);
                memset(expected, 0xa7, sizeof expected);
                cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
                allocator a = {0};
                assert(paint(&t, &l, &o, clear, &a) == CUI_OK);
                reference(&l, &o, clear, expected);
                equal_pixels(actual, expected);
            }
}
static void exact_clip_centers(void) {
    /* The allocator compares the complete pre-paint buffer, including padding. */
    unsigned char actual[CAPACITY] = {0};
    cui_draw_command_v2 c[3] = {
        legacy(CUI_DRAW_CLIP_PUSH, (cui_rect){.5f, .5f, 1, 1}, (cui_color){0}),
        shadow(CUI_DRAW_DROP_SHADOW), legacy(CUI_DRAW_CLIP_POP, (cui_rect){0}, (cui_color){0})};
    c[1].payload.legacy.shadow =
        (cui_drop_shadow_paint){.caster = {0, 0, 3, 3}, .layer = {.color = {40, 80, 120, 173}}};
    cui_draw_list_v2 l = list(c, 3);
    cui_render_v2_options o = options();
    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
    const unsigned char covered[] = {40, 80, 120, 173}, outside[] = {11, 19, 27, 0};
    for (unsigned i = 0; i < 3; ++i) {
        c[0].payload.legacy.bounds.x = i == 0   ? .5f
                                       : i == 1 ? nextafterf(.5f, INFINITY)
                                                : nextafterf(.5f, 0);
        c[0].payload.legacy.bounds.w = i == 1 ? nextafterf(1, 0) : 1;
        allocator a = {0};
        kernel_calls = 0;
        assert(paint(&t, &l, &o, (cui_color){11, 19, 27, 0}, &a) == CUI_OK);
        assert(memcmp(actual, i == 1 ? outside : covered, 4) == 0 &&
               memcmp(actual + 4, outside, 4) == 0);
        assert(i == 1 ? !kernel_calls : kernel_calls == 1);
    }
}
static void frozen_unit_rgba(void) {
    unsigned char actual[CAPACITY], old[WIDTH * HEIGHT * 4];
    cui_render_v2_options o = options();
    for (unsigned kind = 0; kind < 2; ++kind)
        for (unsigned sigma = 0; sigma < 2; ++sigma)
            for (unsigned radius = 0; radius < 2; ++radius)
                for (unsigned alpha = 0; alpha < 3; ++alpha) {
                    cui_draw_command_v2 c =
                        shadow(kind ? CUI_DRAW_INSET_SHADOW : CUI_DRAW_DROP_SHADOW);
                    if (!sigma)
                        c.payload.legacy.shadow.layer.sigma = 0;
                    if (!radius)
                        c.payload.legacy.shadow.radii = (cui_corner_radii){0};
                    c.payload.legacy.shadow.layer.color.a = alpha == 0 ? 1 : alpha == 1 ? 127 : 173;
                    cui_draw_list_v2 l = list(&c, 1);
                    cui_draw_list v1 = {.commands = &c.payload.legacy,
                                        .count = 1,
                                        .width = WIDTH,
                                        .height = HEIGHT};
                    cui_color clear = {11, 19, 27, 73};
                    assert(software_v2_original_reference(&v1, clear, WIDTH, HEIGHT, old));
                    memset(actual, 0xa7, sizeof actual);
                    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
                    allocator a = {0};
                    assert(paint(&t, &l, &o, clear, &a) == CUI_OK);
                    for (unsigned y = 0; y < HEIGHT; ++y)
                        assert(memcmp(actual + y * STRIDE, old + y * WIDTH * 4, WIDTH * 4) == 0);
                }
}
static void limits_and_failures(void) {
    unsigned char actual[CAPACITY];
    memset(actual, 0x79, sizeof actual);
    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
    cui_render_v2_options o = options();
    cui_draw_command_v2 c[3] = {shadow(CUI_DRAW_DROP_SHADOW), shadow(CUI_DRAW_INSET_SHADOW)};
    cui_draw_list_v2 l = list(c, 2);
    allocator a = {0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    size_t cpu = a.requested[0], metadata = a.requested[1];
    cui_software_v2_shadow_descriptor d[2];
    cui_software_v2_shadow_clip clip = {-1e20, -1e20, 1e20, 1e20};
    size_t cells = 0, taps = 0;
    for (unsigned i = 0; i < 2; ++i) {
        kernel_calls = 0;
        assert(cui_software_v2_shadow_query((cui_draw_type)c[i].kind, c[i].payload.legacy.shadow,
                                            &o, clip, CUI_SOFTWARE_V2_SHADOW_CELLS,
                                            CUI_SOFTWARE_V2_SHADOW_TAPS, &d[i]) == CUI_OK);
        assert(!kernel_calls);
        cells += d[i].bytes;
        taps += d[i].taps;
    }
    /* Test-only hook injects an otherwise unexpected preparation error after
     * allocation, both before first byte and after the first shadow plane. */
    for (unsigned f = 0; f < 2; ++f) {
        fail_kernel_call = f ? d[0].taps + 1 : 1;
        kernel_calls = 0;
        a = (allocator){0};
        assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION);
        assert(a.calls == 2 && a.frees == 2 && !a.live);
        fail_kernel_call = 0;
        kernel_calls = 0;
        a = (allocator){0};
        assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    }
    size_t descbytes = 2 * sizeof(cui_software_v2_paint_descriptor);
    size_t align = _Alignof(cui_software_v2_shadow_descriptor);
    size_t padding = (align - descbytes % align) % align;
    assert(cpu == 2 * sizeof(cui_draw_command_v2) + WIDTH * HEIGHT * 4 + cells);
    assert(metadata == descbytes + padding + 2 * sizeof(cui_software_v2_shadow_descriptor));
    for (unsigned fail = 1; fail <= 2; ++fail) {
        a = (allocator){.fail = fail};
        kernel_calls = 0;
        assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !kernel_calls);
        a = (allocator){0};
        assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    }
    o.cpu_bytes = cpu - 1;
    a = (allocator){0};
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls &&
           !kernel_calls);
    o = options();
    o.metadata_bytes = metadata - 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls &&
           !kernel_calls);
    o.cpu_bytes = cpu;
    o.metadata_bytes = metadata;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    o = options();
    o.max_pixels = cells - 1;
    a = (allocator){0};
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls &&
           !kernel_calls);
    o.max_pixels = cells;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    /* Background and A8 cells share the transaction prepared-cell quota. */
    c[2] = (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND,
                                 .payload.background = {.bounds = {0, 0, 1, 1},
                                                        .brush = {.kind = CUI_BACKGROUND_SOLID},
                                                        .opacity = 1}};
    l.count = 3;
    o = options();
    o.max_pixels = cells;
    a = (allocator){0};
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls &&
           !kernel_calls);
    o.max_pixels = cells + 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    o = options();
    c[2] = legacy(CUI_DRAW_TEXT, (cui_rect){0}, (cui_color){0});
    l.count = 3;
    a = (allocator){0};
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION && !a.calls &&
           !kernel_calls);
    c[2] = shadow(CUI_DRAW_DROP_SHADOW);
    c[2].payload.legacy.shadow.layer.sigma = NAN;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION && !a.calls &&
           !kernel_calls);
    l.count = 2;
    /* Query cap boundaries are atomic, with no reads, including clipped retry. */
    cui_software_v2_shadow_descriptor out, before;
    memset(&out, 0xa7, sizeof out);
    memcpy(&before, &out, sizeof out);
    kernel_calls = 0;
    assert(cui_software_v2_shadow_query(CUI_DRAW_DROP_SHADOW, c[0].payload.legacy.shadow, &o, clip,
                                        d[0].bytes - 1, d[0].taps, &out) == CUI_ERROR_CAPACITY &&
           !kernel_calls);
    assert(memcmp(&out, &before, sizeof out) == 0);
    assert(cui_software_v2_shadow_query(CUI_DRAW_DROP_SHADOW, c[0].payload.legacy.shadow, &o, clip,
                                        d[0].bytes, d[0].taps - 1, &out) == CUI_ERROR_CAPACITY &&
           !kernel_calls);
    assert(cui_software_v2_shadow_query(CUI_DRAW_DROP_SHADOW, c[0].payload.legacy.shadow, &o, clip,
                                        d[0].bytes, d[0].taps, &out) == CUI_OK &&
           !kernel_calls);
    /* Invalid inputs remain invalid even when alpha0 or clipped. */
    c[0].payload.legacy.shadow.layer.color.a = 0;
    c[0].payload.legacy.shadow.caster.x = INFINITY;
    l.count = 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION && !a.calls);
    printf("shadow exact transaction: CPU=%zu metadata=%zu cells=%zu taps=%zu\n", cpu, metadata,
           cells, taps);
}
static void large_and_aggregate_caps(void) {
    unsigned char actual[CAPACITY];
    memset(actual, 0x79, sizeof actual);
    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
    cui_render_v2_options o = options();
    o.scale_x = 64;
    o.scale_y = 1.f / 65536;
    o.origin_y = .5f - o.scale_y;
    cui_draw_command_v2 c[2] = {shadow(CUI_DRAW_DROP_SHADOW)};
    c[0].payload.legacy.shadow =
        (cui_drop_shadow_paint){.caster = {0, 0, 16384, 2}, .layer = {.color = {40, 80, 120, 173}}};
    cui_draw_list_v2 l = list(c, 1);
    allocator a = {0};
    cui_drop_shadow_raster raster;
    assert(!cui_effect_drop_shadow_query(c[0].payload.legacy.shadow, 64, 0, 0, &raster));
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){11, 19, 27, 0}, &a) == CUI_OK);
    assert(kernel_calls == WIDTH * 128); /* clipped rows, no full-source bitmap */
    /* One-row profile at min-axis density has exactly524288 source taps. */
    o = options();
    o.scale_x = 1.f / 65536;
    o.scale_y = 64;
    o.origin_x = .5f;
    o.origin_y = 0;
    c[0].payload.legacy.shadow =
        (cui_drop_shadow_paint){.caster = {0, 0, 8192, 1}, .layer = {.color = {40, 80, 120, 173}}};
    cui_software_v2_shadow_descriptor d;
    cui_software_v2_shadow_clip clip = {-1e20, -1e20, 1e20, 1e20};
    assert(cui_software_v2_shadow_query(CUI_DRAW_DROP_SHADOW, c[0].payload.legacy.shadow, &o, clip,
                                        CUI_SOFTWARE_V2_SHADOW_CELLS, CUI_SOFTWARE_V2_SHADOW_TAPS,
                                        &d) == CUI_ERROR_CAPACITY);
    /* Individual sparse row exactly1M; two together must reject before eval/allocation. */
    o.viewport_height = 1;
    t.height = 1;
    c[0].payload.legacy.shadow.caster.w = 16384;
    c[0].payload.legacy.shadow.caster.h = 1.f / 64;
    assert(cui_software_v2_shadow_query(CUI_DRAW_DROP_SHADOW, c[0].payload.legacy.shadow, &o, clip,
                                        CUI_SOFTWARE_V2_SHADOW_CELLS, CUI_SOFTWARE_V2_SHADOW_TAPS,
                                        &d) == CUI_OK);
    assert(d.taps == CUI_SOFTWARE_V2_SHADOW_TAPS);
    c[1] = c[0];
    l.count = 2;
    a = (allocator){0};
    kernel_calls = 0;
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls &&
           !kernel_calls);
    l.count = 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK &&
           kernel_calls == CUI_SOFTWARE_V2_SHADOW_TAPS);
}
static void million_cell_query(void) {
    cui_render_v2_options o = options();
    o.viewport_width = 1024;
    o.viewport_height = 1024;
    cui_drop_shadow_paint p = {.caster = {0, 0, 1024, 1024}, .layer = {.color = {1, 2, 3, 4}}};
    cui_software_v2_shadow_clip clip = {-1e20, -1e20, 1e20, 1e20};
    cui_software_v2_shadow_descriptor d, before;
    memset(&d, 0xa7, sizeof d);
    memcpy(&before, &d, sizeof d);
    kernel_calls = 0;
    assert(cui_software_v2_shadow_query(CUI_DRAW_DROP_SHADOW, p, &o, clip,
                                        CUI_SOFTWARE_V2_SHADOW_CELLS - 1,
                                        CUI_SOFTWARE_V2_SHADOW_TAPS, &d) == CUI_ERROR_CAPACITY);
    assert(!kernel_calls && memcmp(&d, &before, sizeof d) == 0);
    assert(cui_software_v2_shadow_query(CUI_DRAW_DROP_SHADOW, p, &o, clip,
                                        CUI_SOFTWARE_V2_SHADOW_CELLS, CUI_SOFTWARE_V2_SHADOW_TAPS,
                                        &d) == CUI_OK);
    assert(!kernel_calls && d.bytes == 1048576 && d.taps == 1048576);
}
int main(int argc, char **argv) {
    assert(ordinary_paint_fixture_main(argc, argv) == 0);
    large_position_pixels();
    exact_clip_centers();
    frozen_unit_rgba();
    shadow_frames();
    hand_pixels();
    limits_and_failures();
    large_and_aggregate_caps();
    million_cell_query();
    return 0;
}
