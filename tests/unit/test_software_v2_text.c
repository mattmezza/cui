/* SPDX-License-Identifier: MIT */
#ifdef CUI_HAS_TEXT
#include "../../src/text/cui_text.c" /* Private ownership/cache/dependency proof. */
#endif
#include "../../backends/software/cui_software_v2_paint_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
bool cui_test_original_render(const char *, const char *, const cui_draw_list *, cui_color,
                              unsigned, unsigned, unsigned char *);
bool cui_test_original_fallback_render(const char *, const char *, const cui_draw_list *, cui_color,
                                       unsigned, unsigned, unsigned char *);
bool cui_test_original_scaled_render(const char *, const char *, const cui_draw_command *, float,
                                     int, int, cui_color, unsigned, unsigned, unsigned char *);
#ifdef CUI_HAS_TEXT
#include "text_prepare_fixture_internal.h"
#endif
enum { W = 96, H = 72, ROW = W * 4, STRIDE = ROW + 11, CAPACITY = STRIDE * H + 17 };
static cui_render_v2_options options(void) {
    return (cui_render_v2_options){.size = sizeof(cui_render_v2_options),
                                   .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                                   .viewport_width = W,
                                   .viewport_height = H,
                                   .scale_x = 1,
                                   .scale_y = 1,
                                   .cpu_bytes = 32 * 1024 * 1024,
                                   .metadata_bytes = 1024 * 1024,
                                   .texture_bytes = 32 * 1024 * 1024,
                                   .transfer_bytes = 32 * 1024 * 1024,
                                   .max_pixels = 8 * 1024 * 1024,
                                   .max_pixels_per_background = 4 * 1024 * 1024};
}
static cui_draw_list_v2 list(cui_draw_command_v2 *c, size_t n) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = CUI_DRAW_ABI_V2,
                              .command_stride = sizeof(*c),
                              .commands = c,
                              .count = n,
                              .width = W,
                              .height = H,
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                              .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS};
}
typedef struct {
    size_t calls, fail, live, peak, used;
    cui_software_v2_target *target;
    const cui_draw_list_v2 *list;
    const cui_render_v2_options *options;
    unsigned char before[CAPACITY];
    bool reentrant;
} allocator;
typedef union {
    cui_fundamental_alignment_internal alignment;
    size_t bytes;
} prefix;
static void *allocate(void *user, size_t n);
static void release(void *user, void *p) {
    allocator *a = user;
    assert(a->target->busy);
    if (a->reentrant)
        assert(cui_software_v2_paint(a->target, a->list, (cui_color){0}, a->options, allocate,
                                     release, a) == CUI_ERROR_INVALID_ACTION);
    prefix *h = (prefix *)p - 1;
    a->used -= h->bytes;
    --a->live;
    free(h);
}
static void *allocate(void *user, size_t n) {
    allocator *a = user;
    assert(a->target->busy);
    assert(!memcmp(a->target->pixels, a->before, CAPACITY));
    if (a->reentrant)
        assert(cui_software_v2_paint(a->target, a->list, (cui_color){0}, a->options, allocate,
                                     release, a) == CUI_ERROR_INVALID_ACTION);
    if (++a->calls == a->fail)
        return NULL;
    prefix *p = malloc(sizeof(*p) + n);
    assert(p);
    p->bytes = n;
    ++a->live;
    a->used += n;
    if (a->used > a->peak)
        a->peak = a->used;
    return p + 1;
}
static void unchanged_padding(const unsigned char *p) {
    for (unsigned y = 0; y < H; ++y)
        for (unsigned x = ROW; x < STRIDE; ++x)
            assert(p[y * STRIDE + x] == 0xa5);
    for (size_t i = STRIDE * H; i < CAPACITY; ++i)
        assert(p[i] == 0xa5);
}
#ifdef CUI_HAS_TEXT
/* Independent integer source-over oracle, never calls painter blending. */
static void reference_blend(unsigned char *p, cui_color c) {
    unsigned d = p[3], a = c.a;
    unsigned den = 255 * a + d * (255 - a);
    if (!den) {
        memset(p, 0, 4);
        return;
    }
    unsigned rgb[] = {c.r, c.g, c.b};
    for (unsigned i = 0; i < 3; ++i)
        p[i] = (unsigned char)((255 * rgb[i] * a + p[i] * d * (255 - a) + den / 2) / den);
    p[3] = (unsigned char)((den + 127) / 255);
}
static void tiny_samples(void) {
    unsigned char a8[] = {0, 64, 128, 255};
    cui_text_recorded_glyph_internal g = {
        .width = 2, .height = 2, .coverage = a8, .source_density = 1};
    const struct {
        double x, y;
        unsigned byte;
        bool occupied;
    } cases[] = {{.5, .5, 0, true},   {1, .5, 32, true},     {.5, 1, 64, true},
                 {1, 1, 112, true},   {1.5, 1.5, 255, true}, {2, 1.5, 128, true},
                 {-.5, .5, 0, false}, {2.5, 1.5, 0, false}};
    for (size_t i = 0; i < sizeof cases / sizeof *cases; ++i) {
        bool occupied = false;
        assert(cui_software_v2_glyph_sample(&g, cases[i].x, cases[i].y, &occupied) ==
               cases[i].byte);
        assert(occupied == cases[i].occupied);
    }
    /* Axis transform and zero-weight halo: unit cells only, including zero alpha. */
    unsigned char p[3 * 3 * 4];
    for (unsigned i = 0; i < 9; ++i) {
        p[i * 4] = 99;
        p[i * 4 + 1] = 88;
        p[i * 4 + 2] = 77;
        p[i * 4 + 3] = 0;
    }
    cui_render_v2_options o = options();
    o.viewport_width = o.viewport_height = 3;
    cui_software_v2_surface s = {p, &o, cui_software_v2_clip_all()};
    g.color = (cui_color){200, 30, 90, 127};
    cui_software_v2_text_paint(&s, (cui_software_v2_text_range){&g, &g});
    assert(p[0] == 0 && p[3] == 0);
    assert(p[2 * 4] == 99 && p[2 * 4 + 3] == 0);
    unsigned char expected[4] = {99, 88, 77, 0};
    reference_blend(expected, (cui_color){200, 30, 90, (unsigned char)(127 * 255 / 255)});
    assert(!memcmp(p + (3 + 1) * 4, expected, 4));
    for (unsigned i = 0; i < 9; ++i) {
        p[4 * i] = 99;
        p[4 * i + 1] = 88;
        p[4 * i + 2] = 77;
        p[4 * i + 3] = 0;
    }
    o.scale_x = 2;
    o.scale_y = .5f;
    cui_software_v2_text_paint(&s, (cui_software_v2_text_range){&g, &g});
    const unsigned char coverage[] = {48, 88, 136};
    for (unsigned x = 0; x < 3; ++x) {
        unsigned char e[4] = {99, 88, 77, 0};
        reference_blend(e, (cui_color){200, 30, 90, (unsigned char)(127 * coverage[x] / 255)});
        assert(!memcmp(p + x * 4, e, 4));
    }
    for (unsigned i = 3; i < 9; ++i)
        assert(p[4 * i] == 99 && p[4 * i + 3] == 0);
    g.x = INT_MAX;
    g.y = INT_MIN;
    g.source_density = 64;
    cui_software_v2_glyph_cover cover;
    assert(cui_software_v2_glyph_query(&g, &s, &cover));
    assert(cover.x0 == cover.x1 && cover.y0 == cover.y1);
}
/* Independent physical reference uses recorded source bytes, separable weights
 * at each target center, then preserves glyph source order. Tiny table above
 * independently fixes the interpolation/rounding/occupancy convention. */
static void reference_text(unsigned char *out, cui_font *font, const cui_draw_command *c,
                           const cui_render_v2_options *o, cui_color clear) {
    for (unsigned i = 0; i < W * H; ++i) {
        out[4 * i] = clear.r;
        out[4 * i + 1] = clear.g;
        out[4 * i + 2] = clear.b;
        out[4 * i + 3] = clear.a;
    }
    cui_text_prepare_session session = {0};
    /* Test malloc callbacks below have no transaction state. */
    extern void *reference_allocate(void *, size_t);
    extern void reference_release(void *, void *);
    cui_text_owner_internal owner = {.allocate = reference_allocate,
                                     .release = reference_release,
                                     .cpu_limit = 32 * 1024 * 1024,
                                     .metadata_limit = 1024 * 1024};
    session.owner = &owner;
    assert(cui_text_prepare_line(&session, font, c, fmaxf(1, fmaxf(o->scale_x, o->scale_y))));
    for (cui_text_recorded_glyph_internal *g = session.first; g; g = g->next)
        for (unsigned y = 0; y < H; ++y)
            for (unsigned x = 0; x < W; ++x) {
                double lx = (x + .5 - o->origin_x) / o->scale_x,
                       ly = (y + .5 - o->origin_y) / o->scale_y;
                double sx = lx * g->source_density - .5 - g->x,
                       sy = ly * g->source_density - .5 - g->y;
                if (o->scale_x == g->source_density && o->scale_y == g->source_density &&
                    o->origin_x == floorf(o->origin_x) && o->origin_y == floorf(o->origin_y)) {
                    sx = x - o->origin_x - g->x;
                    sy = y - o->origin_y - g->y;
                }
                double bx = floor(sx), by = floor(sy), sum = 0;
                bool occupied = false;
                for (unsigned j = 0; j < 2; ++j)
                    for (unsigned i = 0; i < 2; ++i) {
                        double xx = bx + i, yy = by + j;
                        double wx = 1 - fabs(sx - xx), wy = 1 - fabs(sy - yy);
                        if (wx > 0 && wy > 0 && xx >= 0 && yy >= 0 && xx < g->width &&
                            yy < g->height) {
                            occupied = true;
                            sum += wx * wy * g->coverage[(size_t)yy * g->width + (size_t)xx];
                        }
                    }
                if (occupied) {
                    cui_color ink = g->paint.kind == CUI_TEXT_PAINT_LINEAR
                                        ? cui_text_paint_sample(g->paint, lx, ly)
                                        : g->color;
                    ink.a = (unsigned char)(ink.a * (unsigned)floor(sum + .5) / 255);
                    reference_blend(out + (y * W + x) * 4, ink);
                }
            }
    cui_text_prepare_session_end(&session);
    assert(!owner.cpu_used && !owner.metadata_used);
}
void *reference_allocate(void *u, size_t n) {
    (void)u;
    return malloc(n);
}
void reference_release(void *u, void *p) {
    (void)u;
    free(p);
}
static void negative_frames(cui_font *font) {
    unsigned char pixels[CAPACITY];
    cui_render_v2_options o = options();
    cui_draw_command_v2 c[2] = {
        {.kind = CUI_DRAW_TEXT, .payload.legacy = fixture(0)},
        {.kind = CUI_DRAW_RECT, .payload.legacy = {.type = CUI_DRAW_RECT, .bounds = {0, 0, 2, 2}}}};
    cui_draw_list_v2 l = list(c, 2);
    cui_software_v2_target t = {pixels, W, H, STRIDE, sizeof pixels, false};
    for (unsigned i = 0; i < 8; ++i) {
        c[0].payload.legacy = fixture(0);
        c[1].payload.legacy.radius = 0;
        o = options();
        if (i == 0)
            c[0].payload.legacy.font_size = nextafterf(4096, INFINITY);
        if (i == 1)
            c[0].payload.legacy.bounds.x = nextafterf(1e6f, INFINITY);
        if (i == 2)
            c[0].payload.legacy.text = (cui_str){(const char *)pixels, 3};
        if (i == 3)
            c[1].payload.legacy.radius = NAN;
        if (i == 4)
            o.max_pixels = 1;
        if (i == 5)
            o.cpu_bytes = 1;
        if (i == 6)
            o.metadata_bytes = 1;
        if (i == 7) {
            c[0].payload.legacy.text = (cui_str){NULL, 1};
        }
        memset(pixels, 0xa5, sizeof pixels);
        allocator a = {.target = &t, .list = &l, .options = &o};
        memcpy(a.before, pixels, sizeof pixels);
        assert(cui_software_v2_paint_with_font(&t, &l, (cui_color){0}, &o, font, allocate, release,
                                               &a) != CUI_OK);
        assert(!a.live && !a.used && !t.busy && !memcmp(pixels, a.before, sizeof pixels));
        if (i != 4 && i != 7)
            assert(!a.calls);
    }
#ifndef CUI_TEXT_BIDI
    static volatile uintptr_t bad = 1;
    for (unsigned i = 0; i < 2; ++i) {
        c[0].payload.legacy = fixture(0);
        c[0].payload.legacy.text_source =
            (cui_text_line_source){{(const char *)bad, i ? 1 : 0}, 0, 0};
        c[0].payload.legacy.text = (cui_str){(const char *)bad, SIZE_MAX};
        l = list(c, 1);
        o = options();
        memset(pixels, 0xa5, sizeof pixels);
        allocator a = {.target = &t, .list = &l, .options = &o};
        memcpy(a.before, pixels, sizeof pixels);
        assert(cui_software_v2_paint_with_font(&t, &l, (cui_color){0}, &o, font, allocate, release,
                                               &a) == CUI_ERROR_INVALID_ACTION);
        assert(!a.calls && !t.busy && !memcmp(pixels, a.before, sizeof pixels));
    }
#endif
}
static void text_resource_aliases(cui_font *font) {
    union {
        cui_fundamental_alignment_internal alignment;
        unsigned char bytes[CAPACITY];
        cui_text_gradient_stop stops[2];
        cui_text_color_span spans[2];
    } storage;
    for (unsigned i = 0; i < 2; ++i) {
#ifndef CUI_TEXT_BIDI
        if (i == 1)
            continue;
#endif
        memset(storage.bytes, 0xa5, sizeof storage.bytes);
        cui_draw_command c = fixture(7);
        if (i == 0) {
            storage.stops[0] = (cui_text_gradient_stop){0, {200, 30, 50, 127}};
            storage.stops[1] = (cui_text_gradient_stop){1, {20, 100, 240, 230}};
            c.text_paint = (cui_text_paint){
                .kind = CUI_TEXT_PAINT_LINEAR, .stops = storage.stops, .stop_count = 2, .x1 = 1};
        } else {
            storage.spans[0] = (cui_text_color_span){0, 1, {20, 100, 240, 127}};
            c.text_source = (cui_text_line_source){{"g", 1}, 0, 1};
            c.text_colors = storage.spans;
            c.text_color_count = 1;
        }
        cui_draw_command_v2 command = {.kind = CUI_DRAW_TEXT, .payload.legacy = c};
        cui_draw_list_v2 l = list(&command, 1);
        cui_render_v2_options o = options();
        cui_software_v2_target t = {storage.bytes, W, H, STRIDE, CAPACITY, false};
        allocator a = {.target = &t, .list = &l, .options = &o};
        memcpy(a.before, storage.bytes, CAPACITY);
        assert(cui_software_v2_paint_with_font(&t, &l, (cui_color){0}, &o, font, allocate, release,
                                               &a) == CUI_ERROR_INVALID_ACTION);
        assert(!a.calls && !t.busy && !memcmp(storage.bytes, a.before, CAPACITY));
    }
}
static void mixed_frame(cui_font *font, const char *regular, const char *bold) {
    unsigned char pixels[CAPACITY], expected[W * H * 4];
    cui_color clear = {21, 17, 35, 63};
    cui_draw_command old[14] = {
        {.type = CUI_DRAW_RECT, .bounds = {1, 1, 80, 50}, .color = {10, 40, 80, 120}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {4, 3, 52, 39}},
        {0},
        {.type = CUI_DRAW_BORDER,
         .bounds = {3, 2, 62, 43},
         .radius = 5,
         .stroke = 2,
         .color = {255, 30, 0, 90}},
        {0},
        {.type = CUI_DRAW_CLIP_POP},
        {.type = CUI_DRAW_DROP_SHADOW,
         .shadow = {.caster = {4, 5, 12, 15},
                    .radii = {2, 3, 1, 4},
                    .layer = {.offset_x = 1,
                              .offset_y = -2,
                              .sigma = 1.5f,
                              .spread = .25f,
                              .color = {30, 80, 130, 173}}}},
        {0}};
    old[2] = fixture(0);
    old[4] = fixture(6);
    old[4].bounds.y = 20;
    old[7] = fixture(7);
    old[7].bounds.y = 5;
    static unsigned char image[] = {200, 30, 90, 127};
    static cui_mesh_vertex triangle[] = {{0, 0, 0, 0}, {4, 0, 0, 0}, {0, 4, 0, 0}};
    old[8] = (cui_draw_command){
        .type = CUI_DRAW_IMAGE,
        .bounds = {4, 4, 4, 4},
        .bitmap = {.bitmap = {.pixels = image, .width = 1, .height = 1, .stride = 4},
                   .source = {0, 0, 1, 1},
                   .opacity = .5f}};
    old[9] = fixture(0);
    old[9].bounds.y = 8;
    static const cui_text_gradient_stop stops[] = {{0, {255, 0, 0, 0}}, {1, {20, 160, 240, 200}}};
    old[9].text_paint = (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                                         .stops = stops,
                                         .stop_count = 2,
                                         .x0 = 3,
                                         .y0 = 8,
                                         .x1 = 70,
                                         .y1 = 8};
    old[10] = (cui_draw_command){.type = CUI_DRAW_MESH,
                                 .mesh = {.vertices = triangle,
                                          .vertex_count = 3,
                                          .origin_x = 10,
                                          .origin_y = 10,
                                          .scale = 2,
                                          .color = {.1f, .2f, .05f, .5f}}};
    old[11] = old[6];
    old[11].type = CUI_DRAW_INSET_SHADOW;
    old[11].shadow.layer.offset_x = -.5f;
    old[12] = (cui_draw_command){
        .type = CUI_DRAW_RECT, .bounds = {5, 5, 20, 15}, .color = {120, 30, 170, 127}};
    old[13] = fixture(7);
    old[13].bounds = (cui_rect){6, 6, 0, 0};
    cui_draw_command_v2 commands[14];
    for (unsigned i = 0; i < 14; ++i)
        commands[i] = (cui_draw_command_v2){.kind = old[i].type, .payload.legacy = old[i]};
    /* Solid BACKGROUND has an independent equivalent V1 RECT at unit scale. */
    commands[12] =
        (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND,
                              .payload.background = {.bounds = old[12].bounds, .opacity = 1}};
    assert(cui_background_make_solid(cui_background_color_from_srgba8(old[12].color),
                                     &commands[12].payload.background.brush));
    cui_draw_list_v2 l = list(commands, 14);
    cui_render_v2_options o = options();
    memset(pixels, 0xa5, sizeof pixels);
    cui_software_v2_target t = {pixels, W, H, STRIDE, sizeof pixels, false};
    allocator a = {.target = &t, .list = &l, .options = &o};
    memcpy(a.before, pixels, sizeof pixels);
    assert(cui_software_v2_paint_with_font(&t, &l, clear, &o, font, allocate, release, &a) ==
           CUI_OK);
    cui_draw_list v1 = {.commands = old, .count = 14, .width = W, .height = H};
    assert(cui_test_original_render(regular, bold, &v1, clear, W, H, expected));
    for (unsigned y = 0; y < H; ++y)
        assert(!memcmp(pixels + y * STRIDE, expected + y * ROW, ROW));
    assert(!a.live && !a.used && !t.busy);
    unchanged_padding(pixels);
}
#ifdef CUI_TEXT_BIDI
static void fallback_fonts(cui_font *font, const char *regular, const char *bold) {
    unsigned char pixels[CAPACITY], expected[W * H * 4];
    cui_render_v2_options o = options();
    cui_color clear = {13, 17, 29, 0};
    cui_draw_command c = fixture(0);
    c.text = (cui_str)CUI_STR("abc \xd7\x90\xd7\x91\xd7\x92");
    cui_draw_command_v2 command = {.kind = CUI_DRAW_TEXT, .payload.legacy = c};
    cui_draw_list_v2 l = list(&command, 1);
    cui_software_v2_target t = {pixels, W, H, STRIDE, sizeof pixels, false};
    memset(pixels, 0xa5, sizeof pixels);
    allocator a = {.target = &t, .list = &l, .options = &o};
    memcpy(a.before, pixels, sizeof pixels);
    assert(!FT_Get_Char_Index(font->face, 0x5d0));
    assert(cui_software_v2_paint_with_font(&t, &l, clear, &o, font, allocate, release, &a) ==
           CUI_ERROR_INVALID_ACTION);
    cui_draw_list old = {.commands = &c, .count = 1, .width = W, .height = H};
    assert(!cui_test_original_render(regular, bold, &old, clear, W, H, expected));
    assert(!a.live && !a.used && !t.busy && !memcmp(pixels, a.before, sizeof pixels));
    puts("Actual missing Inter Hebrew face: old rejects, V2 rejects atomically.");
    const char *paths[] = {"/usr/share/fonts/noto/NotoSansHebrew-Regular.ttf",
                           "/usr/share/fonts/noto/NotoSansArabic-Regular.ttf"};
    const cui_str strings[] = {
        CUI_STR("abc \xd7\x90\xd7\x91\xd7\x92"),
        CUI_STR("abc \xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9")};
    const float axes[][2] = {{1, 1}, {2, .75f}, {64, 1.f / 65536}};
    for (unsigned i = 0; i < 2; ++i) {
        FILE *f = fopen(paths[i], "rb");
        if (!f) {
            printf("Optional RTL/fallback font unavailable: %s; explicit coverage deferred.\n",
                   paths[i]);
            continue;
        }
        fclose(f);
        cui_font *fallback = cui_font_open(paths[i]);
        assert(fallback);
        cui_font *chain[] = {fallback};
        assert(cui_font_set_fallbacks(font, chain, 1));
        c.text = strings[i];
        command.payload.legacy = c;
        for (unsigned j = 0; j < 3; ++j) {
            o = options();
            o.scale_x = axes[j][0];
            o.scale_y = axes[j][1];
            memset(pixels, 0xa5, sizeof pixels);
            a = (allocator){.target = &t, .list = &l, .options = &o};
            memcpy(a.before, pixels, sizeof pixels);
            assert(cui_software_v2_paint_with_font(&t, &l, clear, &o, font, allocate, release,
                                                   &a) == CUI_OK);
            reference_text(expected, font, &c, &o, clear);
            for (unsigned y = 0; y < H; ++y)
                assert(!memcmp(pixels + y * STRIDE, expected + y * ROW, ROW));
            if (j == 0) {
                old.commands = &c;
                assert(cui_test_original_fallback_render(regular, paths[i], &old, clear, W, H,
                                                         expected));
                for (unsigned y = 0; y < H; ++y)
                    assert(!memcmp(pixels + y * STRIDE, expected + y * ROW, ROW));
            }
            assert(!a.live && !a.used && !t.busy && !fallback->glyph_cache);
            unchanged_padding(pixels);
        }
        assert(cui_font_set_fallbacks(font, NULL, 0));
        cui_font_close(fallback);
        printf("Actual old/new RTL fallback graph passed: %s (3transforms).\n", paths[i]);
    }
}
#endif
static void real_fonts(cui_font *font, const char *regular, const char *bold) {
    unsigned char pixels[CAPACITY], expected[W * H * 4];
    cui_color clear = {31, 55, 91, 0};
    const float axes[][4] = {{1, 1, 0, 0},
                             {1, 1, 2, -1},
                             {1.5f, .75f, .25f, -.5f},
                             {1.f / 65536, 64, 40, 0},
                             {64, 1.f / 65536, 0, 20},
                             {64, 64, -192, -128},
                             {.125f, .25f, -.75f, .5f},
                             {3, 2, 0, 0},
                             {3, 3, 2, -1},
                             {16, 16, 0, 0},
                             {1, 1, -999900, -999900},
                             {64, 2, -1000000, -1000000}};
    for (unsigned f = 0; f < 14; ++f) {
#ifndef CUI_TEXT_BIDI
        if (f == 3 || f == 4 || f == 5 || f == 9 || f == 12)
            continue;
#endif
        for (unsigned a = 0; a < sizeof axes / sizeof *axes; ++a) {
            cui_render_v2_options o = options();
            o.scale_x = axes[a][0];
            o.scale_y = axes[a][1];
            o.origin_x = axes[a][2];
            o.origin_y = axes[a][3];
            cui_draw_command c = fixture(f);
            if (f == 8)
                c.text = (cui_str)CUI_STR("a\xcc\x81 ffi");
            if (f == 9) {
                c.text_source =
                    (cui_text_line_source){(cui_str)CUI_STR("xx a\xcc\x81 ffi yy"), 3, 10};
                c.text = (cui_str){NULL, 0};
            }
            if (f == 10) {
                static const cui_text_gradient_stop stops[] = {{0, {255, 0, 0, 0}},
                                                               {.5f, {0, 255, 0, 127}},
                                                               {.5f, {0, 0, 255, 180}},
                                                               {1, {255, 255, 0, 255}}};
                c.text_paint = (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                                                .stops = stops,
                                                .stop_count = 4,
                                                .x0 = 3,
                                                .y0 = 2,
                                                .x1 = 70,
                                                .y1 = 2};
            }
            if (f == 11) {
                c.text = (cui_str){"", 0};
                c.bounds.w = NAN;
                c.bounds.h = NAN;
            }
            if (f == 12) {
                c.text_source = (cui_text_line_source){{"", 0}, 0, 0};
                static volatile uintptr_t inactive = 1;
                c.text = (cui_str){(const char *)inactive, SIZE_MAX};
            }
            if (f == 13)
                c.text_style = (cui_text_style){.family = 7,
                                                .weight = CUI_TEXT_WEIGHT_MEDIUM,
                                                .italic = true,
                                                .monospace = true};
            if (a == 10) {
                c.bounds.x = 999900;
                c.bounds.y = 999900;
            }
            if (a == 11) {
                c.bounds.x = 15625;
                c.bounds.y = 500000;
            }
            cui_draw_command_v2 command = {.kind = CUI_DRAW_TEXT, .payload.legacy = c};
            cui_draw_list_v2 l = list(&command, 1);
            memset(pixels, 0xa5, sizeof pixels);
            cui_software_v2_target t = {pixels, W, H, STRIDE, sizeof pixels, false};
            allocator state = {.target = &t, .list = &l, .options = &o, .reentrant = true};
            memcpy(state.before, pixels, sizeof pixels);
            cui_error painted =
                cui_software_v2_paint_with_font(&t, &l, clear, &o, font, allocate, release, &state);
            if (painted != CUI_OK)
                fprintf(stderr, "paint failed form=%u transform=%u error=%d calls=%zu peak=%zu\n",
                        f, a, painted, state.calls, state.peak);
            assert(painted == CUI_OK);
            assert(!state.live && !state.used && !t.busy);
            unchanged_padding(pixels);
            reference_text(expected, font, &c, &o, clear);
            for (unsigned y = 0; y < H; ++y)
                assert(!memcmp(pixels + y * STRIDE, expected + y * ROW, ROW));
            if (a <= 1) {
                cui_draw_command translated = c;
                /* Frozen V1 blanket validation treats TEXT w/h as finite;
                 * V2 leaves these inactive and deliberately keeps NaN input. */
                translated.bounds.w = 0;
                translated.bounds.h = 0;
                translated.bounds.x += o.origin_x;
                translated.bounds.y += o.origin_y;
                if (translated.text_paint.kind == CUI_TEXT_PAINT_LINEAR) {
                    translated.text_paint.x0 += o.origin_x;
                    translated.text_paint.x1 += o.origin_x;
                    translated.text_paint.y0 += o.origin_y;
                    translated.text_paint.y1 += o.origin_y;
                }
                cui_draw_list old = {.commands = &translated, .count = 1, .width = W, .height = H};
                bool original_ok =
                    cui_test_original_render(regular, bold, &old, clear, W, H, expected);
                if (!original_ok)
                    fprintf(stderr, "original render failed form=%u transform=%u\n", f, a);
                assert(original_ok);
                for (unsigned y = 0; y < H; ++y)
                    assert(!memcmp(pixels + y * STRIDE, expected + y * ROW, ROW));
            }
            if (a < 10 && o.scale_x == o.scale_y && o.scale_x <= 16 &&
                o.origin_x == floorf(o.origin_x) && o.origin_y == floorf(o.origin_y)) {
                assert(cui_test_original_scaled_render(regular, bold, &c, o.scale_x,
                                                       (int)o.origin_x, (int)o.origin_y, clear, W,
                                                       H, expected));
                for (unsigned y = 0; y < H; ++y)
                    assert(!memcmp(pixels + y * STRIDE, expected + y * ROW, ROW));
            }
            /* Sweep each actual counted allocation request; every failure is atomic/retryable. */
            if (a == 0 && f == 0) {
                size_t calls = state.calls;
                for (size_t fail = 1; fail <= calls; ++fail) {
                    memset(pixels, 0xa5, sizeof pixels);
                    state = (allocator){.target = &t, .list = &l, .options = &o, .fail = fail};
                    memcpy(state.before, pixels, sizeof pixels);
                    assert(cui_software_v2_paint_with_font(&t, &l, clear, &o, font, allocate,
                                                           release, &state) != CUI_OK);
                    assert(!memcmp(pixels, state.before, sizeof pixels) && !state.live && !t.busy);
                }
                state = (allocator){.target = &t, .list = &l, .options = &o};
                memcpy(state.before, pixels, sizeof pixels);
                assert(cui_software_v2_paint_with_font(&t, &l, clear, &o, font, allocate, release,
                                                       &state) == CUI_OK);
                assert(!state.live && !state.used && !t.busy);
                for (unsigned y = 0; y < H; ++y)
                    assert(!memcmp(pixels + y * STRIDE, expected + y * ROW, ROW));
            }
        }
    }
}
#endif
int main(int argc, char **argv) {
#ifdef CUI_HAS_TEXT
    assert(argc == 3);
    cui_font *font = cui_font_open(argv[1]), *bold = cui_font_open(argv[2]);
    assert(font && bold);
    assert(cui_font_set_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD}, bold));
    assert(cui_font_set_style(
        font,
        (cui_text_style){
            .family = 7, .weight = CUI_TEXT_WEIGHT_MEDIUM, .italic = true, .monospace = true},
        bold));
    (void)baseline;
    tiny_samples();
    negative_frames(font);
    text_resource_aliases(font);
    mixed_frame(font, argv[1], argv[2]);
    real_fonts(font, argv[1], argv[2]);
#ifdef CUI_TEXT_BIDI
    fallback_fonts(font, argv[1], argv[2]);
#endif
    assert(!font->glyph_cache && !bold->glyph_cache);
    cui_font_close(font);
    cui_font_close(bold);
#else
    (void)argc;
    (void)argv;
    unsigned char pixels[CAPACITY];
    memset(pixels, 0xa5, sizeof pixels);
    cui_render_v2_options o = options();
    cui_draw_command_v2 c = {
        .kind = CUI_DRAW_TEXT,
        .payload.legacy = {.type = CUI_DRAW_TEXT, .font_size = 12, .text = CUI_STR("hi")}};
    static volatile uintptr_t poisoned = 1;
    c.payload.legacy.text = (cui_str){(const char *)poisoned, SIZE_MAX};
    cui_draw_list_v2 l = list(&c, 1);
    cui_software_v2_target t = {pixels, W, H, STRIDE, sizeof pixels, false};
    allocator a = {.target = &t, .list = &l, .options = &o};
    memcpy(a.before, pixels, sizeof pixels);
    assert(cui_software_v2_paint_with_font(&t, &l, (cui_color){0}, &o, (cui_font *)(uintptr_t)1,
                                           allocate, release, &a) == CUI_ERROR_INVALID_ACTION);
    assert(!a.calls && !t.busy && !memcmp(pixels, a.before, sizeof pixels));
    unchanged_padding(pixels);
#endif
    puts("private V2 text transaction passed");
    return 0;
}
