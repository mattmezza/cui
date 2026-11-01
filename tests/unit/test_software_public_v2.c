/* SPDX-License-Identifier: MIT */
/* Public entry point proof with private allocator failure/reentry instrumentation. */
#ifdef CUI_HAS_TEXT
#include "../../src/text/cui_text.c"
#endif
#include "cui/cui_software.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void *public_allocate(size_t);
static void public_release(void *);
#define malloc public_allocate
#define free public_release
#include "../../backends/software/cui_software.c"
#undef malloc
#undef free
static size_t calls, fail_call, live;
static void *blocks[128];
static cui_software *active;
static const cui_draw_list_v2 *active_list;
static bool reenter;
static unsigned reentries;
static void reentry(void) {
    if (!reenter)
        return;
    ++reentries;
    assert(active->v2_busy);
    cui_software_v2_info info = {.size = sizeof(info)};
    unsigned char before[sizeof info];
    memcpy(before, &info, sizeof info);
    assert(cui_software_query_v2(active, &info) == CUI_ERROR_LIFECYCLE);
    assert(!memcmp(before, &info, sizeof(info)));
    assert(cui_software_render_v2(active, active_list, (cui_color){0}, NULL) ==
           CUI_ERROR_LIFECYCLE);
    cui_draw_list empty = {0};
    assert(!cui_software_render(active, &empty, (cui_color){0}));
    assert(!cui_software_resize(active, 7, 9));
    cui_software_destroy(active); /* Must not free an active renderer. */
    unsigned w = 0, h = 0;
    assert(cui_software_pixels(active, &w, &h) == active->pixels && w == 32 && h == 24);
}
static void *public_allocate(size_t n) {
    reentry();
    if (++calls == fail_call)
        return NULL;
    void *p = malloc(n);
    assert(p);
    assert(live < 128);
    blocks[live++] = p;
    return p;
}
static void public_release(void *p) {
    reentry();
    for (size_t i = 0; i < live; ++i)
        if (blocks[i] == p) {
            blocks[i] = blocks[--live];
            break;
        }
    free(p);
}
static cui_draw_list_v2 list(cui_draw_command_v2 *commands, size_t count) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = 2,
                              .command_stride = sizeof(*commands),
                              .commands = commands,
                              .count = count,
                              .width = 32,
                              .height = 24,
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                              .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS};
}
static cui_software_v2_info query(cui_software *r) {
    size_t before = calls;
    cui_software_v2_info info = {.size = sizeof(info)};
    assert(cui_software_query_v2(r, &info) == CUI_OK && calls == before);
    assert(info.draw.abi_version == 2 && info.draw.features == CUI_DRAW_V2_FEATURE_BACKGROUNDS);
    size_t count =
        ((size_t)32 * 1024 * 1024 - (size_t)r->width * r->height * 4) / sizeof(cui_draw_command_v2);
    if (count > (size_t)1024 * 1024 / sizeof(cui_software_v2_paint_descriptor))
        count = (size_t)1024 * 1024 / sizeof(cui_software_v2_paint_descriptor);
    if (count > 16384)
        count = 16384;
    assert(info.draw.max_commands == count);
    assert(info.draw.max_backgrounds == (count < 4096 ? count : 4096));
    return info;
}
static void aliases(cui_software *r, cui_draw_list_v2 *l, cui_software_v2_info info) {
    unsigned char before[32 * 24 * 4];
    memcpy(before, r->pixels, sizeof before);
    assert(cui_software_query_v2(r, (cui_software_v2_info *)r) == CUI_ERROR_INVALID_ACTION);
    assert(cui_software_query_v2(r, (cui_software_v2_info *)r->pixels) == CUI_ERROR_INVALID_ACTION);
    assert(cui_software_render_v2(r, (const cui_draw_list_v2 *)r->pixels, (cui_color){0}, NULL) ==
           CUI_ERROR_INVALID_ACTION);
    /* A valid explicit options value in target storage must still reject. */
    memcpy(r->pixels, &info.defaults, sizeof info.defaults);
    unsigned char aliased[sizeof before];
    memcpy(aliased, r->pixels, sizeof aliased);
    assert(cui_software_render_v2(r, l, (cui_color){0}, (const cui_render_v2_options *)r->pixels) ==
           CUI_ERROR_INVALID_ACTION);
    assert(!memcmp(aliased, r->pixels, sizeof aliased));
    memcpy(r->pixels, before, sizeof before);
    info.size--;
    unsigned char bad[sizeof info];
    memcpy(bad, &info, sizeof info);
    assert(cui_software_query_v2(r, &info) == CUI_ERROR_INVALID_ACTION);
    assert(!memcmp(bad, &info, sizeof info));
}
static void admission(cui_software *r, cui_draw_list_v2 *l, cui_software_v2_info info) {
    unsigned char before[32 * 24 * 4];
    memcpy(before, r->pixels, sizeof before);
    cui_render_v2_options o = info.defaults;
    o.cpu_bytes++;
    assert(cui_software_render_v2(r, l, (cui_color){0}, &o) == CUI_ERROR_CAPACITY);
    o = info.defaults;
    o.cpu_bytes = sizeof before;
    assert(cui_software_render_v2(r, l, (cui_color){0}, &o) == CUI_ERROR_CAPACITY);
    o = info.defaults;
    o.cpu_bytes = sizeof before + sizeof(cui_draw_command_v2);
    o.metadata_bytes = sizeof(cui_software_v2_paint_descriptor);
    assert(cui_software_render_v2(r, l, (cui_color){0}, &o) == CUI_OK);
    /* Exact one-command reservation works; one byte less refuses atomically. */
    memcpy(r->pixels, before, sizeof before);
    --o.cpu_bytes;
    assert(cui_software_render_v2(r, l, (cui_color){0}, &o) == CUI_ERROR_CAPACITY);
    o = info.defaults;
    o.viewport_width--;
    assert(cui_software_render_v2(r, l, (cui_color){0}, &o) == CUI_ERROR_INVALID_ACTION);
    o = info.defaults;
    o.scale_y = 0;
    assert(cui_software_render_v2(r, l, (cui_color){0}, &o) == CUI_ERROR_INVALID_ACTION);
    cui_draw_list_v2 broken = *l;
    broken.error = CUI_ERROR_CAPACITY;
    assert(cui_software_render_v2(r, &broken, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    assert(!memcmp(before, r->pixels, sizeof before));
    for (size_t failure = 1; failure <= 2; ++failure) {
        calls = 0;
        fail_call = failure;
        assert(cui_software_render_v2(r, l, (cui_color){0}, NULL) == CUI_ERROR_CAPACITY);
        assert(!live && !r->v2_busy && !memcmp(before, r->pixels, sizeof before));
    }
    fail_call = 0;
    calls = 0;
    reenter = true;
    assert(cui_software_render_v2(r, l, (cui_color){2, 3, 4, 50}, NULL) == CUI_OK);
    reenter = false;
    assert(reentries >= 4 && !live && !r->v2_busy);
    /* Stale query defaults cannot authorize a resized target. */
    assert(cui_software_resize(r, 33, 24));
    assert(cui_software_render_v2(r, l, (cui_color){0}, &info.defaults) ==
           CUI_ERROR_INVALID_ACTION);
    assert(cui_software_resize(r, 32, 24));
}
static void owner_aliases(cui_software *r) {
    unsigned char object_before[sizeof(*r)], pixels_before[32 * 24 * 4];
    memcpy(object_before, r, sizeof(*r));
    memcpy(pixels_before, r->pixels, sizeof pixels_before);
    cui_draw_command_v2 command = {
        .kind = CUI_DRAW_IMAGE,
        .payload.legacy = {.type = CUI_DRAW_IMAGE,
                           .bounds = {0, 0, 1, 1},
                           .bitmap = {.bitmap = {.pixels = (const unsigned char *)r,
                                                 .width = 1,
                                                 .height = 1,
                                                 .stride = 4},
                                      .source = {0, 0, 1, 1},
                                      .opacity = 1}}};
    cui_draw_list_v2 l = list(&command, 1);
    size_t before = calls;
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    l.commands = (const cui_draw_command_v2 *)r;
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    l.commands = &command;
    command =
        (cui_draw_command_v2){.kind = CUI_DRAW_MESH,
                              .payload.legacy = {.type = CUI_DRAW_MESH,
                                                 .mesh = {.vertices = (const cui_mesh_vertex *)r,
                                                          .vertex_count = 3,
                                                          .scale = 1,
                                                          .color = {.1f, .2f, .3f, .5f}}}};
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
#ifdef CUI_HAS_TEXT
    command = (cui_draw_command_v2){
        .kind = CUI_DRAW_TEXT,
        .payload.legacy = {.type = CUI_DRAW_TEXT, .font_size = 12, .text = {(const char *)r, 1}}};
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    command.payload.legacy.text = (cui_str)CUI_STR("ab");
    command.payload.legacy.text_paint = (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                                                         .stops = (const cui_text_gradient_stop *)r,
                                                         .stop_count = 2,
                                                         .x1 = 20};
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
#ifdef CUI_TEXT_BIDI
    command.payload.legacy.text_paint = (cui_text_paint){0};
    command.payload.legacy.text_source = (cui_text_line_source){{(const char *)r, 1}, 0, 1};
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    command.payload.legacy.text_source = (cui_text_line_source){CUI_STR("ab"), 0, 2};
    command.payload.legacy.text_colors = (const cui_text_color_span *)r;
    command.payload.legacy.text_color_count = 1;
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
#endif
#endif
    assert(calls == before && !r->v2_busy);
    assert(!memcmp(object_before, r, sizeof(*r)));
    assert(!memcmp(pixels_before, r->pixels, sizeof pixels_before));
}
#ifdef CUI_HAS_TEXT
static void text_allocations(cui_software *r) {
    cui_draw_command_v2 c = {.kind = CUI_DRAW_TEXT,
                             .payload.legacy = {.type = CUI_DRAW_TEXT,
                                                .bounds = {1, 1, 0, 0},
                                                .font_size = 12,
                                                .text = CUI_STR("Affi"),
                                                .color = {10, 90, 130, 157}}};
    cui_draw_list_v2 l = list(&c, 1);
    active_list = &l;
    fail_call = 0;
    calls = 0;
    reenter = true;
    assert(cui_software_render_v2(r, &l, (cui_color){1, 2, 3, 0}, NULL) == CUI_OK);
    reenter = false;
    size_t allocations = calls;
    assert(allocations > 2 && !live && !r->font->glyph_cache);
    unsigned char before[32 * 24 * 4];
    memcpy(before, r->pixels, sizeof before);
    for (size_t n = 1; n <= allocations; ++n) {
        calls = 0;
        fail_call = n;
        reenter = true;
        cui_error error = cui_software_render_v2(r, &l, (cui_color){0}, NULL);
        reenter = false;
        assert(error == CUI_ERROR_CAPACITY && !live && !r->v2_busy);
        assert(!memcmp(before, r->pixels, sizeof before));
    }
    fail_call = 0;
    assert(cui_software_render_v2(r, &l, (cui_color){1, 2, 3, 0}, NULL) == CUI_OK);
    assert(!memcmp(before, r->pixels, sizeof before));
    printf("public owned text allocation sweep: %zu requests, cleanup/reentry/retry PASS\n",
           allocations);
}
#endif
static void background_counts(cui_software *r, cui_software_v2_info info) {
    size_t count = info.draw.max_backgrounds;
    cui_draw_command_v2 *commands = calloc(count + 1, sizeof(*commands));
    assert(commands && count < info.draw.max_commands);
    for (size_t i = 0; i <= count; ++i) {
        commands[i].kind = CUI_DRAW_V2_BACKGROUND;
        commands[i].payload.background.brush.kind = CUI_BACKGROUND_SOLID;
        commands[i].payload.background.opacity = 1;
    }
    cui_draw_list_v2 l = list(commands, count);
    assert(cui_software_render_v2(r, &l, (cui_color){3, 4, 5, 6}, NULL) == CUI_OK);
    unsigned char before[32 * 24 * 4];
    memcpy(before, r->pixels, sizeof before);
    l.count++;
    size_t prior = calls;
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_CAPACITY);
    assert(calls == prior && !memcmp(before, r->pixels, sizeof before));
    l = list(commands, info.draw.max_commands + 1);
    /* Count refusal precedes any access to the undersized commands backing. */
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_CAPACITY);
    l = list(commands, 1);
    commands[0].kind = UINT32_MAX;
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    assert(!memcmp(before, r->pixels, sizeof before));
    free(commands);
}
static void composition(cui_software *r, cui_software_v2_info info) {
    cui_context *ui =
        cui_create((cui_config){.max_elements = 32, .max_commands = 64, .frame_bytes = 8192});
    assert(ui);
    cui_frame_v2_options f = {.size = sizeof(f),
                              .support = info.draw,
                              .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS};
    assert(cui_begin_frame_v2(ui, 32, 24, 0, &f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 20,
                            .height = 15,
                            .fixed_width = true,
                            .fixed_height = true,
                            .background = {100, 0, 0, 255}});
    cui_background brush = {.kind = CUI_BACKGROUND_SOLID, .solid = {0, 1, 0, .5f}};
    assert(cui_set_background(ui, &brush));
    cui_end_box(ui);
    cui_draw_list_v2 l;
    assert(cui_end_frame_v2(ui, &l));
    bool found = false;
    for (size_t i = 0; i < l.count; ++i)
        found |= l.commands[i].kind == CUI_DRAW_V2_BACKGROUND;
    assert(found);
    assert(cui_software_render_v2(r, &l, (cui_color){0}, NULL) == CUI_OK);
    assert(r->pixels[1] == 255 && r->pixels[3] == 128);
    cui_destroy(ui);
}
int main(int argc, char **argv) {
#ifndef CUI_HAS_TEXT
    (void)argc;
    (void)argv;
#endif
    cui_font *font = NULL;
#ifdef CUI_HAS_TEXT
    assert(argc == 2);
    font = cui_font_open(argv[1]);
    assert(font);
#endif
    active = cui_software_create(32, 24, font);
    assert(active);
    cui_software_v2_info info = query(active);
#ifdef CUI_HAS_TEXT
    assert(info.text_features & CUI_SOFTWARE_V2_TEXT_A8);
    assert(!!(info.text_features & CUI_SOFTWARE_V2_TEXT_CONTEXT) ==
           cui_text_prepare_context_available_internal());
#else
    assert(!info.text_features);
#endif
    cui_draw_command_v2 c = {.kind = CUI_DRAW_RECT,
                             .payload.legacy = {.type = CUI_DRAW_RECT,
                                                .bounds = {1, 2, 8, 9},
                                                .color = {7, 80, 20, 99}}};
    cui_draw_list_v2 l = list(&c, 1);
    active_list = &l;
    aliases(active, &l, info);
    owner_aliases(active);
    admission(active, &l, info);
    composition(active, info);
    background_counts(active, info);
#ifdef CUI_HAS_TEXT
    text_allocations(active);
    active_list = &l;
#endif
    /* Public forwarding must equal the proven physical private entry for both axes. */
    const float transforms[][4] = {{1, 1, 0, 0}, {2, .5f, .3f, -.25f}, {1.f / 65536, 64, 0, 0}};
    for (size_t i = 0; i < sizeof transforms / sizeof *transforms; ++i) {
        cui_render_v2_options o = info.defaults;
        o.scale_x = transforms[i][0];
        o.scale_y = transforms[i][1];
        o.origin_x = transforms[i][2];
        o.origin_y = transforms[i][3];
        unsigned char expected[32 * 24 * 4];
        cui_software_v2_target t = {.pixels = expected,
                                    .width = 32,
                                    .height = 24,
                                    .stride = 128,
                                    .capacity = sizeof expected};
        assert(cui_software_v2_paint_with_font(&t, &l, (cui_color){4, 5, 6, 7}, &o, font,
                                               software_v2_allocate, software_v2_release,
                                               NULL) == CUI_OK);
        assert(cui_software_render_v2(active, &l, (cui_color){4, 5, 6, 7}, &o) == CUI_OK);
        assert(!memcmp(expected, active->pixels, sizeof expected));
    }
    cui_draw_list old = {.commands = &c.payload.legacy, .count = 1, .width = 32, .height = 24};
    unsigned char legacy_pixels[32 * 24 * 4];
    assert(cui_software_render(active, &old, (cui_color){4, 5, 6, 7}));
    memcpy(legacy_pixels, active->pixels, sizeof legacy_pixels);
    assert(cui_software_render_v2(active, &l, (cui_color){0}, NULL) == CUI_OK);
    cui_draw_list_v2 broken = l;
    broken.error = CUI_ERROR_INVALID_ACTION;
    assert(cui_software_render_v2(active, &broken, (cui_color){0}, NULL) ==
           CUI_ERROR_INVALID_ACTION);
    assert(cui_software_render(active, &old, (cui_color){4, 5, 6, 7}));
    assert(!memcmp(legacy_pixels, active->pixels, sizeof legacy_pixels));
    /* NULL-font query never fabricates text support. */
    cui_software *plain = cui_software_create(2, 2, NULL);
    assert(plain && !query(plain).text_features);
    cui_draw_command_v2 unsupported = {.kind = CUI_DRAW_TEXT,
                                       .payload.legacy = {.type = CUI_DRAW_TEXT, .font_size = 12}};
    static volatile uintptr_t poison = 1;
    unsupported.payload.legacy.text = (cui_str){(const char *)poison, SIZE_MAX};
    cui_draw_list_v2 unsupported_list = list(&unsupported, 1);
    cui_software_v2_info plain_info = {.size = sizeof(plain_info)};
    assert(cui_software_query_v2(plain, &plain_info) == CUI_OK);
    size_t prior = calls;
    assert(cui_software_render_v2(plain, &unsupported_list, (cui_color){0}, NULL) ==
           CUI_ERROR_INVALID_ACTION);
    assert(calls == prior);
#if defined(CUI_HAS_TEXT) && !defined(CUI_TEXT_BIDI)
    unsupported.payload.legacy.text_source = (cui_text_line_source){{"", 0}, 0, 0};
    prior = calls;
    assert(cui_software_render_v2(active, &unsupported_list, (cui_color){0}, NULL) ==
           CUI_ERROR_INVALID_ACTION);
    assert(calls == prior);
#endif
    /* Real initialized target whose staging alone consumes the CPU ceiling.
     * calloc is released immediately; no giant raster or fake target storage. */
    cui_software *large = cui_software_create(4096, 2048, NULL);
    assert(large);
    cui_software_v2_info large_info = {.size = sizeof(large_info)};
    unsigned char unchanged[sizeof large_info];
    memcpy(unchanged, &large_info, sizeof large_info);
    prior = calls;
    assert(cui_software_query_v2(large, &large_info) == CUI_ERROR_CAPACITY);
    assert(calls == prior && !memcmp(unchanged, &large_info, sizeof large_info));
    cui_software_destroy(large);
    cui_software_destroy(plain);
    cui_software_destroy(active);
#ifdef CUI_HAS_TEXT
    cui_font_close(font);
#endif
    puts("public software V2 query/composition/atomicity/aliases/reentry/physical forwarding PASS");
    return 0;
}
