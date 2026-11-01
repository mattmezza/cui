/* SPDX-License-Identifier: MIT */
#ifdef CUI_TEST_SOURCE_STANDALONE
#include "../../backends/common/cui_cpu_v2_sources_internal.h"
#else
#include "../../backends/common/cui_cpu_v2_internal.h"
#define CPU_RANGE_RESULT cpu_range_current
#include "cpu_v2_range_case_internal.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned native_queries, comparisons;
static volatile uintptr_t poison_address = 1;
static void *poison(void) {
    return (void *)poison_address;
}
static cui_draw_list_v2 list(cui_draw_command_v2 *c, size_t count) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = CUI_DRAW_ABI_V2,
                              .command_stride = sizeof(*c),
                              .commands = c,
                              .count = count,
                              .width = 8,
                              .height = 8,
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1};
}
static cui_draw_command_v2 rect(void) {
    return (cui_draw_command_v2){
        .kind = CUI_DRAW_RECT,
        .payload.legacy = {.type = CUI_DRAW_RECT, .bounds = {0, 0, 4, 4}, .color = {1, 2, 3, 255}}};
}
static cui_draw_command_v2 text(void) {
    return (cui_draw_command_v2){
        .kind = CUI_DRAW_TEXT,
        .payload.legacy = {.type = CUI_DRAW_TEXT, .text = {"a", 1}, .font_size = 12}};
}
static cui_draw_command_v2 image(void) {
    static const unsigned char pixel[4] = {1, 2, 3, 4};
    return (cui_draw_command_v2){
        .kind = CUI_DRAW_IMAGE,
        .payload.legacy = {
            .type = CUI_DRAW_IMAGE,
            .bounds = {0, 0, 1, 1},
            .bitmap = {.bitmap = {.pixels = pixel, .width = 1, .height = 1, .stride = 4},
                       .source = {0, 0, 1, 1},
                       .opacity = 1}}};
}
static cui_draw_command_v2 mesh(void) {
    static const cui_mesh_vertex vertices[3] = {{0, 0, 0, 0}, {1, 0, 1, 0}, {0, 1, 0, 1}};
    return (cui_draw_command_v2){
        .kind = CUI_DRAW_MESH,
        .payload.legacy = {
            .type = CUI_DRAW_MESH,
            .mesh = {.vertices = vertices, .vertex_count = 3, .scale = 1, .color = {1, 1, 1, 1}}}};
}
static cui_draw_command_v2 contextual(void) {
    static const cui_text_color_span span = {0, 1, {1, 2, 3, 4}};
    cui_draw_command_v2 c = text();
    c.payload.legacy.text_source = (cui_text_line_source){{"a", 1}, 0, 1};
    c.payload.legacy.text_colors = &span;
    c.payload.legacy.text_color_count = 1;
    c.payload.legacy.text = (cui_str){poison(), SIZE_MAX};
    return c;
}
static cui_draw_command_v2 gradient(void) {
    static const cui_text_gradient_stop stops[2] = {{0, {1, 2, 3, 4}}, {1, {5, 6, 7, 8}}};
    cui_draw_command_v2 c = text();
    c.payload.legacy.text_paint =
        (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR, .stops = stops, .stop_count = 2, .x1 = 1};
    return c;
}
/* Model only the reviewed caller ordering. No SDL is linked or initialized. */
static cui_error before_native(const cui_draw_list_v2 *l, const cui_render_v2_options *o,
                               const cui_cpu_v2_exclusion *ranges, size_t count, bool *busy) {
    cui_error error = cui_cpu_v2_sources_exclude(l, o, ranges, count);
    if (error != CUI_OK)
        return error;
    assert(!*busy);
    *busy = true;
    ++native_queries;
    *busy = false;
    return CUI_OK;
}
static void prequery_tests(void) {
    cui_draw_command_v2 c = rect();
    cui_draw_list_v2 l = list(&c, 1);
    _Alignas(cui_draw_list_v2) _Alignas(cui_render_v2_options) _Alignas(
        cui_draw_command_v2) unsigned char owner[256];
    memset(owner, 0x6b, sizeof owner);
    unsigned char saved[sizeof owner];
    memcpy(saved, owner, sizeof owner);
    cui_cpu_v2_exclusion range = {owner, sizeof owner};
    bool busy = false;
    assert(before_native(&l, NULL, &range, 1, &busy) == CUI_OK && native_queries == 1 && !busy);
    for (unsigned part = 0; part <= CUI_CPU_SOURCE_TEXT_STOPS; ++part) {
        cui_cpu_v2_exclusion out = {poison(), SIZE_MAX};
        c = rect();
        c.payload.legacy.text = (cui_str){poison(), SIZE_MAX};
        c.payload.legacy.text_colors = poison();
        c.payload.legacy.text_color_count = SIZE_MAX;
        c.payload.legacy.text_paint.stops = poison();
        c.payload.legacy.text_paint.stop_count = SIZE_MAX;
        assert(cui_cpu_v2_source_range(&c, (cui_cpu_v2_source_part)part, &out) == CUI_OK);
        assert(!out.pointer && !out.bytes);
    }
    c = text();
    c.payload.legacy.text_colors = poison();
    c.payload.legacy.text_paint.stops = poison();
    assert(cui_cpu_v2_sources_exclude(&l, NULL, &range, 1) == CUI_OK);
    c.payload.legacy.text_paint.stop_count = 1;
    cui_cpu_v2_exclusion out = {poison(), SIZE_MAX};
    assert(cui_cpu_v2_source_range(&c, CUI_CPU_SOURCE_TEXT_STOPS, &out) == CUI_OK && !out.pointer &&
           !out.bytes);
    assert(cui_cpu_v2_sources_exclude(&l, NULL, &range, 1) == CUI_ERROR_INVALID_ACTION);
    c = contextual();
    c.payload.legacy.text_source = (cui_text_line_source){{"", 0}, 0, 0};
    c.payload.legacy.text_color_count = 0;
    c.payload.legacy.text_colors = poison();
    assert(cui_cpu_v2_source_range(&c, CUI_CPU_SOURCE_TEXT_BYTES, &out) == CUI_OK);
    assert(out.pointer == c.payload.legacy.text_source.paragraph.data && !out.bytes);
    assert(cui_cpu_v2_sources_exclude(&l, NULL, &range, 1) == CUI_OK);
    native_queries = 0;
    /* Every active borrowed pointer must reject before the fake native hook. */
    for (unsigned part = 0; part <= CUI_CPU_SOURCE_TEXT_STOPS; ++part) {
        if (part == CUI_CPU_SOURCE_IMAGE) {
            c = image();
            c.payload.legacy.bitmap.bitmap.pixels = owner;
        } else if (part == CUI_CPU_SOURCE_MESH) {
            c = mesh();
            c.payload.legacy.mesh.vertices = (const cui_mesh_vertex *)owner;
        } else if (part == CUI_CPU_SOURCE_TEXT_BYTES) {
            c = text();
            c.payload.legacy.text = (cui_str){(const char *)owner, 1};
        } else if (part == CUI_CPU_SOURCE_TEXT_SPANS) {
            c = contextual();
            c.payload.legacy.text_colors = (const cui_text_color_span *)owner;
        } else {
            c = gradient();
            c.payload.legacy.text_paint.stops = (const cui_text_gradient_stop *)owner;
        }
        assert(before_native(&l, NULL, &range, 1, &busy) == CUI_ERROR_INVALID_ACTION);
        assert(!native_queries && !busy && !memcmp(saved, owner, sizeof owner));
    }
    c = text();
    c.payload.legacy.text = (cui_str){(const char *)&busy, sizeof busy};
    cui_cpu_v2_exclusion flag_range = {&busy, sizeof busy};
    assert(before_native(&l, NULL, &flag_range, 1, &busy) == CUI_ERROR_INVALID_ACTION);
    assert(!busy && !native_queries);
    c = rect();
    assert(before_native((const cui_draw_list_v2 *)owner, NULL, &range, 1, &busy) ==
           CUI_ERROR_INVALID_ACTION);
    assert(before_native(&l, (const cui_render_v2_options *)owner, &range, 1, &busy) ==
           CUI_ERROR_INVALID_ACTION);
    l.commands = (const cui_draw_command_v2 *)owner;
    assert(before_native(&l, NULL, &range, 1, &busy) == CUI_ERROR_INVALID_ACTION);
    l.commands = poison();
    l.count = 16385;
    assert(before_native(&l, NULL, &range, 1, &busy) == CUI_ERROR_CAPACITY);
    l.count = 0;
    assert(before_native(&l, NULL, &range, 1, &busy) == CUI_OK);
    assert(native_queries == 1);
    native_queries = 0;
    l = list(&c, 1);
    c = image();
    c.payload.legacy.bitmap.bitmap.pixels = (const unsigned char *)(UINTPTR_MAX - 2);
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.bitmap.bitmap.height = 0;
    assert(cui_cpu_v2_source_range(&c, CUI_CPU_SOURCE_IMAGE, &out) == CUI_OK && !out.bytes);
    assert(out.pointer == (const void *)(UINTPTR_MAX - 2));
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_OK);
    native_queries = 0;
    c = mesh();
    c.payload.legacy.mesh.vertex_count = SIZE_MAX;
    cui_cpu_v2_exclusion untouched = {owner, 73};
    out = untouched;
    assert(cui_cpu_v2_source_range(&c, CUI_CPU_SOURCE_MESH, &out) == CUI_ERROR_CAPACITY);
    assert(out.pointer == untouched.pointer && out.bytes == untouched.bytes);
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_ERROR_CAPACITY && !native_queries);
    c = image();
    c.payload.legacy.bitmap.bitmap.stride = SIZE_MAX;
    c.payload.legacy.bitmap.bitmap.height = 2;
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_ERROR_CAPACITY);
    c = contextual();
    c.payload.legacy.text_color_count = 4097;
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_ERROR_CAPACITY);
    c = gradient();
    c.payload.legacy.text_paint.stop_count = 9;
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_ERROR_CAPACITY);
    c = text();
    c.payload.legacy.text.length = 65537;
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_ERROR_CAPACITY);
    c.payload.legacy.text_source.paragraph.length = 1;
    assert(before_native(&l, NULL, NULL, 0, &busy) == CUI_ERROR_INVALID_ACTION);
    c = rect();
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 1) == CUI_ERROR_INVALID_ACTION);
    assert(cui_cpu_v2_sources_exclude(&l, NULL, &range, 8) == CUI_ERROR_INVALID_ACTION);
    assert(cui_cpu_v2_source_extent((cui_cpu_v2_exclusion){(const void *)(UINTPTR_MAX - 1), 1}));
    assert(!cui_cpu_v2_source_extent((cui_cpu_v2_exclusion){(const void *)UINTPTR_MAX, 1}));
    assert(cui_cpu_v2_source_extent((cui_cpu_v2_exclusion){(const void *)UINTPTR_MAX, 0}));
    for (unsigned part = 0; part <= CUI_CPU_SOURCE_TEXT_STOPS; ++part) {
        if (part == CUI_CPU_SOURCE_IMAGE) {
            c = image();
            c.payload.legacy.bitmap.bitmap.height = 0;
            c.payload.legacy.bitmap.bitmap.pixels = poison();
        } else if (part == CUI_CPU_SOURCE_MESH) {
            c = mesh();
            c.payload.legacy.mesh.vertex_count = 0;
            c.payload.legacy.mesh.vertices = poison();
        } else if (part == CUI_CPU_SOURCE_TEXT_BYTES) {
            c = text();
            c.payload.legacy.text = (cui_str){poison(), 0};
        } else if (part == CUI_CPU_SOURCE_TEXT_SPANS) {
            c = contextual();
            c.payload.legacy.text_color_count = 0;
            c.payload.legacy.text_colors = poison();
        } else {
            c = gradient();
            c.payload.legacy.text_paint.stop_count = 0;
            c.payload.legacy.text_paint.stops = poison();
        }
        out = (cui_cpu_v2_exclusion){owner, 73};
        assert(cui_cpu_v2_source_range(&c, (cui_cpu_v2_source_part)part, &out) == CUI_OK);
        assert(out.pointer == poison() && !out.bytes);
    }
    c = rect();
    out = (cui_cpu_v2_exclusion){owner, 73};
    assert(cui_cpu_v2_source_range(&c, (cui_cpu_v2_source_part)99, &out) ==
           CUI_ERROR_INVALID_ACTION);
    assert(out.pointer == owner && out.bytes == 73);
    c.kind = UINT32_MAX;
    assert(cui_cpu_v2_source_range(&c, CUI_CPU_SOURCE_IMAGE, &out) == CUI_ERROR_INVALID_ACTION);
    assert(out.pointer == owner && out.bytes == 73);
    c.kind = CUI_DRAW_RECT;
    c.payload.legacy.type = CUI_DRAW_TEXT;
    assert(cui_cpu_v2_source_range(&c, CUI_CPU_SOURCE_IMAGE, &out) == CUI_ERROR_INVALID_ACTION);
    c.kind = CUI_DRAW_V2_BACKGROUND;
    memset(&c.payload, 0xa5, sizeof c.payload);
    for (unsigned part = 0; part <= CUI_CPU_SOURCE_TEXT_STOPS; ++part) {
        assert(cui_cpu_v2_source_range(&c, (cui_cpu_v2_source_part)part, &out) == CUI_OK);
        assert(!out.pointer && !out.bytes);
    }
    c = rect();
    cui_render_v2_options unread_options;
    memset(&unread_options, 0xa5, sizeof unread_options);
    assert(cui_cpu_v2_sources_exclude(&l, &unread_options, &range, 1) == CUI_OK);
    assert(!native_queries && !busy && !memcmp(saved, owner, sizeof owner));
}
static void boundary_tests(void) {
    /* Fixed BSS only: no malloc/dependency calls. The walker never reads these
     * pointed arrays; the independent old validator may read the bounded text. */
    static cui_draw_command_v2 commands[16384];
    static char paragraph[65536];
    static cui_text_color_span spans[CUI_TEXT_COLOR_SPAN_MAX];
    static cui_text_gradient_stop stops[8];
    memset(paragraph, 'a', sizeof paragraph);
    for (size_t i = 0; i < 16384; ++i)
        commands[i] = rect();
    cui_draw_list_v2 l = list(commands, 16384);
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_OK);
    l.count = 16385;
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_ERROR_CAPACITY);
    commands[0] = contextual();
    commands[0].payload.legacy.text_source = (cui_text_line_source){{"", 0}, 0, 0};
    commands[0].payload.legacy.text_colors = spans;
    commands[0].payload.legacy.text_color_count = CUI_TEXT_COLOR_SPAN_MAX;
    l.count = 1;
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_OK);
    commands[0] = gradient();
    commands[0].payload.legacy.text_paint.stops = stops;
    commands[0].payload.legacy.text_paint.stop_count = 8;
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_OK);
    for (size_t i = 0; i < 5; ++i) {
        commands[i] = text();
        commands[i].payload.legacy.text = (cui_str){paragraph, sizeof paragraph};
    }
    l.count = 4;
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_OK);
    l.count = 5;
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_ERROR_CAPACITY);
    commands[0] = contextual();
    commands[0].payload.legacy.text_source =
        (cui_text_line_source){{paragraph, sizeof paragraph}, 0, 0};
    commands[0].payload.legacy.text_colors = spans;
    commands[0].payload.legacy.text_color_count = 510;
    commands[1] = text();
    commands[1].payload.legacy.text = (cui_str){paragraph, 65488};
    commands[2] = mesh();
    l.count = 3;
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_OK); /* inspected exactly32 MiB */
    ++commands[1].payload.legacy.text.length;
    assert(cui_cpu_v2_sources_exclude(&l, NULL, NULL, 0) == CUI_ERROR_CAPACITY);
}
#ifndef CUI_TEST_SOURCE_STANDALONE
static cui_render_v2_options options(void) {
    return (cui_render_v2_options){.size = sizeof(cui_render_v2_options),
                                   .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                                   .viewport_width = 8,
                                   .viewport_height = 8,
                                   .scale_x = 1,
                                   .scale_y = 1,
                                   .cpu_bytes = 32 * 1024 * 1024,
                                   .metadata_bytes = 1024 * 1024,
                                   .texture_bytes = 32 * 1024 * 1024,
                                   .transfer_bytes = 32 * 1024 * 1024,
                                   .max_pixels = 8 * 1024 * 1024,
                                   .max_pixels_per_background = 4 * 1024 * 1024};
}
static void compare(cui_draw_list_v2 *l, cui_render_v2_options *o, const void *owner, size_t bytes,
                    unsigned alias, cui_error expected) {
    for (unsigned flags = 0; flags < 8; ++flags) {
        cpu_range_result a = cpu_range_original(l, o, flags, owner, bytes, alias);
        cpu_range_result b = cpu_range_current(l, o, flags, owner, bytes, alias);
        assert(a.error == b.error && a.unchanged == b.unchanged);
        if (flags == 7)
            assert(a.error == expected);
        if (a.error != CUI_OK)
            assert(a.unchanged);
        else {
            for (unsigned i = 0; i < 9; ++i)
                assert(a.fields[i] == b.fields[i]);
            assert(a.text == b.text && a.shadow == b.shadow);
        }
        ++comparisons;
    }
}
static void differential_tests(void) {
    cui_draw_command_v2 c = rect();
    cui_draw_list_v2 l = list(&c, 1);
    cui_render_v2_options o = options();
    _Alignas(cui_draw_list_v2) _Alignas(cui_render_v2_options) _Alignas(
        cui_draw_command_v2) unsigned char owner[256] = {0};
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
    for (unsigned alias = RANGE_OUT_LIST; alias <= RANGE_OUT_COMMANDS; ++alias)
        compare(&l, &o, owner, sizeof owner, alias, CUI_ERROR_INVALID_ACTION);
    c = image();
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
    compare(&l, &o, owner, sizeof owner, RANGE_OUT_IMAGE, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.bounds.x = NAN;
    c.payload.legacy.bitmap.bitmap.stride = SIZE_MAX;
    c.payload.legacy.bitmap.bitmap.height = 2;
    compare(&l, &o, owner, sizeof owner, 0, CUI_ERROR_INVALID_ACTION);
    c = mesh();
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
    compare(&l, &o, owner, sizeof owner, RANGE_OUT_MESH, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.mesh.vertex_count = SIZE_MAX;
    c.payload.legacy.mesh.scale = NAN;
    compare(&l, &o, owner, sizeof owner, 0, CUI_ERROR_CAPACITY);
    c = text();
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
    compare(&l, &o, owner, sizeof owner, RANGE_OUT_TEXT, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.font_size = NAN;
    c.payload.legacy.text.length = SIZE_MAX;
    c.payload.legacy.text.data = poison();
    compare(&l, &o, owner, sizeof owner, 0, CUI_ERROR_INVALID_ACTION);
    c = contextual();
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
    compare(&l, &o, owner, sizeof owner, RANGE_OUT_SPANS, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_source = (cui_text_line_source){{"", 0}, 0, 0};
    c.payload.legacy.text_color_count = 0;
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
    c = text();
    c.payload.legacy.text_source.paragraph.length = 1;
    c.payload.legacy.text = (cui_str){poison(), 1};
    compare(&l, &o, owner, sizeof owner, 0, CUI_ERROR_INVALID_ACTION);
    c = gradient();
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
    compare(&l, &o, owner, sizeof owner, RANGE_OUT_STOPS, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_style.weight = (cui_text_weight)999;
    c.payload.legacy.text_paint.stop_count = SIZE_MAX;
    compare(&l, &o, owner, sizeof owner, 0, CUI_ERROR_INVALID_ACTION);
    for (unsigned part = 0; part <= CUI_CPU_SOURCE_TEXT_STOPS; ++part) {
        if (part == CUI_CPU_SOURCE_IMAGE) {
            c = image();
            c.payload.legacy.bitmap.bitmap.pixels = owner;
        } else if (part == CUI_CPU_SOURCE_MESH) {
            c = mesh();
            c.payload.legacy.mesh.vertices = (const cui_mesh_vertex *)owner;
        } else if (part == CUI_CPU_SOURCE_TEXT_BYTES) {
            c = text();
            c.payload.legacy.text = (cui_str){(const char *)owner, 1};
        } else if (part == CUI_CPU_SOURCE_TEXT_SPANS) {
            c = contextual();
            c.payload.legacy.text_colors = (const cui_text_color_span *)owner;
        } else {
            c = gradient();
            c.payload.legacy.text_paint.stops = (const cui_text_gradient_stop *)owner;
        }
        compare(&l, &o, owner, sizeof owner, 0, CUI_ERROR_INVALID_ACTION);
    }
    l.count = 16385;
    l.commands = poison();
    compare(&l, &o, owner, sizeof owner, 0, CUI_ERROR_CAPACITY);
    l.count = 0;
    compare(&l, &o, owner, sizeof owner, 0, CUI_OK);
}
#endif
int main(void) {
    prequery_tests();
    boundary_tests();
#ifndef CUI_TEST_SOURCE_STANDALONE
    differential_tests();
    assert(comparisons >= 200);
    printf("%u independent original-validator error/active-output comparisons PASS\n", comparisons);
#else
    (void)comparisons;
#endif
    puts("pure source ranges/prequery ordering PASS");
    return 0;
}
