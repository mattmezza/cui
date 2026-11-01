/* SPDX-License-Identifier: MIT */
#include "cpu_v2_common_reference_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static size_t current_samples;
#ifdef CUI_CPU_V2_NEW_FIRST
#define CUI_CPU_V2_SHADOW_SAMPLE_HOOK() (++current_samples)
#include "../../backends/common/cui_cpu_v2_paint_internal.h"
#include "../../backends/software/cui_software_v2_paint_internal.h"
#else
#define CUI_SOFTWARE_V2_SHADOW_SAMPLE_HOOK() (++current_samples)
// clang-format off
#include "../../backends/software/cui_software_v2_paint_internal.h"
#include "../../backends/common/cui_cpu_v2_paint_internal.h"
// clang-format on
#endif
/* Repeat every selective entry after its original text-surface prerequisites. */
#include "../../backends/common/cui_cpu_v2_internal.h"
#include "../../backends/common/cui_cpu_v2_shadow_internal.h"
#include "../../backends/common/cui_cpu_v2_text_internal.h"
#include "../../backends/software/cui_software_v2_internal.h"
#include "../../backends/software/cui_software_v2_shadow_internal.h"
#include "../../backends/software/cui_software_v2_text_internal.h"
enum { W = 32, H = 24, ROW = W * 4, STRIDE = ROW + 7, BYTES = STRIDE * H + 13 };
typedef struct {
    size_t requests[256], count, freed, live, used, peak, fail;
    bool busy;
} trace;
typedef union {
    cui_fundamental_alignment_internal alignment;
    size_t bytes;
} prefix;
static void *allocate(void *user, size_t bytes) {
    trace *t = user;
    assert(t->busy && t->count < 256);
    t->requests[t->count++] = bytes;
    if (t->count == t->fail)
        return NULL;
    prefix *p = malloc(sizeof(*p) + bytes);
    assert(p);
    p->bytes = bytes;
    ++t->live;
    t->used += bytes;
    if (t->used > t->peak)
        t->peak = t->used;
    return p + 1;
}
static void release(void *user, void *block) {
    trace *t = user;
    assert(t->busy && t->live);
    prefix *p = (prefix *)block - 1;
    t->used -= p->bytes;
    --t->live;
    ++t->freed;
    free(p);
}
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
static cui_draw_list_v2 list(cui_draw_command_v2 *commands, size_t count) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = 2,
                              .command_stride = sizeof(*commands),
                              .commands = commands,
                              .count = count,
                              .width = W,
                              .height = H,
                              .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1};
}
static void layouts(void) {
    const size_t current[] = {
        sizeof(cui_cpu_v2_target),
        _Alignof(cui_cpu_v2_target),
        sizeof(cui_cpu_v2_clip),
        _Alignof(cui_cpu_v2_clip),
        sizeof(cui_cpu_v2_paint_descriptor),
        _Alignof(cui_cpu_v2_paint_descriptor),
        sizeof(cui_cpu_v2_plan),
        _Alignof(cui_cpu_v2_plan),
        sizeof(cui_cpu_v2_workspace),
        _Alignof(cui_cpu_v2_workspace),
        sizeof(cui_cpu_v2_shadow_descriptor),
        _Alignof(cui_cpu_v2_shadow_descriptor),
#ifdef CUI_HAS_TEXT
        sizeof(cui_cpu_v2_text_range),
        _Alignof(cui_cpu_v2_text_range),
#endif
    };
    size_t old[16];
    size_t count = cpu_common_original_layout(old, 16);
    assert(count == sizeof current / sizeof *current);
    for (size_t i = 0; i < count; ++i)
        assert(old[i] == current[i]);
    assert(cui_software_v2_paint == cui_cpu_v2_paint);
    assert(cui_software_v2_validate == cui_cpu_v2_validate);
    assert(CUI_SOFTWARE_V2_SHADOW_TAPS == CUI_CPU_V2_SHADOW_TAPS);
    assert(CUI_SOFTWARE_V2_SHADOW_CELLS == CUI_CPU_V2_SHADOW_CELLS);
}
static size_t mixed(cui_draw_command_v2 *c) {
    static unsigned char image[] = {180, 40, 90, 127};
    static cui_mesh_vertex triangle[] = {{0, 0, 0, 0}, {4, 0, 0, 0}, {0, 4, 0, 0}};
    c[0] = (cui_draw_command_v2){.kind = CUI_DRAW_RECT,
                                 .payload.legacy = {.type = CUI_DRAW_RECT,
                                                    .bounds = {1, 1, 25, 17},
                                                    .radius = 3,
                                                    .color = {10, 30, 80, 101}}};
    c[1] = (cui_draw_command_v2){
        .kind = CUI_DRAW_CLIP_PUSH,
        .payload.legacy = {.type = CUI_DRAW_CLIP_PUSH, .bounds = {2.25f, 1.5f, 20, 15}}};
    c[2] = (cui_draw_command_v2){
        .kind = CUI_DRAW_V2_BACKGROUND,
        .payload.background = {.bounds = {0, 0, 28, 20}, .radii = {1, 2, 3, 4}, .opacity = .7f}};
    assert(cui_background_make_linear(
        CUI_BACKGROUND_CSS, CUI_BACKGROUND_OKLAB, 37,
        (cui_background_stop){.position = 0, .color = {.7f, .1f, .2f, .2f}},
        (cui_background_stop){.position = 1, .color = {.1f, .8f, .4f, .8f}},
        &c[2].payload.background.brush));
    c[3] = (cui_draw_command_v2){
        .kind = CUI_DRAW_IMAGE,
        .payload.legacy = {
            .type = CUI_DRAW_IMAGE,
            .bounds = {3, 2, 8, 7},
            .radius = 2,
            .bitmap = {.bitmap = {.pixels = image, .width = 1, .height = 1, .stride = 4},
                       .source = {0, 0, 1, 1},
                       .opacity = .8f}}};
    c[4] = (cui_draw_command_v2){.kind = CUI_DRAW_BORDER,
                                 .payload.legacy = {.type = CUI_DRAW_BORDER,
                                                    .bounds = {2, 2, 20, 13},
                                                    .radius = 3,
                                                    .stroke = 1.25f,
                                                    .color = {250, 30, 70, 180}}};
    c[5] = (cui_draw_command_v2){.kind = CUI_DRAW_MESH,
                                 .payload.legacy = {.type = CUI_DRAW_MESH,
                                                    .mesh = {.vertices = triangle,
                                                             .vertex_count = 3,
                                                             .origin_x = 8,
                                                             .origin_y = 5,
                                                             .scale = 2,
                                                             .color = {.1f, .3f, .2f, .5f}}}};
    c[6] = (cui_draw_command_v2){.kind = CUI_DRAW_CLIP_POP,
                                 .payload.legacy = {.type = CUI_DRAW_CLIP_POP}};
    c[7] = (cui_draw_command_v2){
        .kind = CUI_DRAW_DROP_SHADOW,
        .payload.legacy = {.type = CUI_DRAW_DROP_SHADOW,
                           .shadow = {.caster = {3, 4, 12, 10},
                                      .radii = {1, 3, 2, 4},
                                      .layer = {.offset_x = .5f,
                                                .offset_y = -.25f,
                                                .sigma = 1.25f,
                                                .spread = .5f,
                                                .color = {50, 100, 150, 173}}}}};
    c[8] = c[7];
    c[8].kind = CUI_DRAW_INSET_SHADOW;
    c[8].payload.legacy.type = CUI_DRAW_INSET_SHADOW;
#ifdef CUI_HAS_TEXT
    c[9] = (cui_draw_command_v2){.kind = CUI_DRAW_TEXT,
                                 .payload.legacy = {.type = CUI_DRAW_TEXT,
                                                    .bounds = {2, 2, 0, 0},
                                                    .font_size = 12,
                                                    .text = CUI_STR("Affi"),
                                                    .color = {20, 80, 200, 157}}};
    return 10;
#else
    return 9;
#endif
}
static size_t compare(const cui_draw_list_v2 *l, cui_render_v2_options o, cui_font *font,
                      size_t failure, bool valid) {
    unsigned char old[BYTES], current[BYTES];
    memset(old, 0xa5, sizeof old);
    memset(current, 0xa5, sizeof current);
    trace a = {.fail = failure}, b = {.fail = failure};
    cui_color clear = {17, 31, 49, 0};
    cui_error x = cpu_common_original_paint(old, W, H, STRIDE, sizeof old, l, clear, &o, font,
                                            allocate, release, &a, &a.busy);
    current_samples = 0;
    cui_cpu_v2_target target = {current, W, H, STRIDE, sizeof current, false};
    cui_error y = cui_cpu_v2_paint_with_font_excluding(&target, l, clear, &o, font, allocate,
                                                       release, &b, NULL, 0, &b.busy);
    assert(x == y && a.count == b.count && a.freed == b.freed && a.peak == b.peak);
    if (valid)
        assert(x == (failure ? CUI_ERROR_CAPACITY : CUI_OK));
    else
        assert(x == CUI_ERROR_INVALID_ACTION);
    if (valid && !failure && o.scale_x == 1 && o.scale_y == 1)
        assert(current_samples > 0);
    assert(!a.live && !b.live && !a.used && !b.used && !target.busy);
    assert(!memcmp(a.requests, b.requests, a.count * sizeof(*a.requests)));
    assert(!memcmp(old, current, sizeof old));
    assert(cpu_common_original_samples() == current_samples);
    if (x != CUI_OK)
        for (size_t i = 0; i < sizeof current; ++i)
            assert(current[i] == 0xa5);
    a.busy = b.busy = false;
    return a.count;
}
int main(int argc, char **argv) {
    layouts();
    cui_font *font = NULL;
#ifdef CUI_HAS_TEXT
    assert(argc == 2);
    font = cui_font_open(argv[1]);
    assert(font);
#else
    (void)argc;
    (void)argv;
#endif
    cui_draw_command_v2 commands[10];
    cui_draw_list_v2 l = list(commands, mixed(commands));
    const float axes[][4] = {{1, 1, 0, 0},
                             {1.5f, .75f, .25f, -.5f},
                             {1.f / 65536, 64, 10, 0},
                             {64, 1.f / 65536, 0, 10},
                             {.125f, .25f, -.75f, .5f}};
    for (size_t i = 0; i < sizeof axes / sizeof *axes; ++i) {
        cui_render_v2_options o = options();
        o.scale_x = axes[i][0];
        o.scale_y = axes[i][1];
        o.origin_x = axes[i][2];
        o.origin_y = axes[i][3];
        size_t allocations = compare(&l, o, font, 0, true);
        if (i == 0)
            for (size_t n = 1; n <= allocations; ++n)
                compare(&l, o, font, n, true);
    }
    commands[l.count - 1].kind = UINT32_MAX;
    compare(&l, options(), font, 0, false);
#ifdef CUI_HAS_TEXT
    cui_font_close(font);
#endif
    puts("frozen private/common layouts, allocations, pixels, hooks and include aliases PASS");
    return 0;
}
