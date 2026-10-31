/* SPDX-License-Identifier: MIT */
#include "../../backends/software/cui_software_v2_internal.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static cui_render_v2_options options(void) {
    return (cui_render_v2_options){.size = sizeof(cui_render_v2_options),
                                   .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                                   .viewport_width = 16,
                                   .viewport_height = 12,
                                   .scale_x = 1,
                                   .scale_y = 1,
                                   .cpu_bytes = 32768,
                                   .texture_bytes = 32768,
                                   .transfer_bytes = 32768,
                                   .metadata_bytes = 16384,
                                   .max_pixels = 4096,
                                   .max_pixels_per_background = 4096};
}
static cui_draw_list_v2 list(cui_draw_command_v2 *commands, size_t count) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = CUI_DRAW_ABI_V2,
                              .command_stride = sizeof(cui_draw_command_v2),
                              .commands = commands,
                              .count = count,
                              .width = 16,
                              .height = 12,
                              .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1};
}
static cui_draw_command_v2 legacy(cui_draw_type type) {
    return (cui_draw_command_v2){.kind = (uint32_t)type,
                                 .payload.legacy = {.type = type, .bounds = {1, 1, 8, 8}}};
}
static void rejects(cui_draw_list_v2 l, cui_render_v2_options o, cui_error error) {
    cui_software_v2_plan out, previous;
    memset(&out, 0xa7, sizeof(out));
    memcpy(&previous, &out, sizeof(out));
    assert(cui_software_v2_validate(&l, &o, true, true, &out) == error);
    assert(memcmp(&out, &previous, sizeof(out)) == 0);
}
static void commands(void) {
    unsigned char pixels[16] = {0};
    cui_mesh_vertex vertices[3] = {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}};
    cui_draw_command_v2 c[12];
    c[0] = legacy(CUI_DRAW_DROP_SHADOW);
    c[0].payload.legacy.shadow =
        (cui_drop_shadow_paint){.caster = {1, 1, 4, 4}, .layer = {.color = {0, 0, 0, 128}}};
    c[1] = legacy(CUI_DRAW_CLIP_PUSH);
    c[1].payload.legacy.bounds = (cui_rect){2, 2, 4, 4};
    c[2] = (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND,
                                 .payload.background = {.bounds = {0, 0, 16, 12},
                                                        .brush = {.kind = CUI_BACKGROUND_SOLID},
                                                        .opacity = 1}};
    c[3] = legacy(CUI_DRAW_RECT);
    c[4] = legacy(CUI_DRAW_INSET_SHADOW);
    c[4].payload.legacy.shadow = c[0].payload.legacy.shadow;
    c[5] = legacy(CUI_DRAW_BORDER);
    c[6] = legacy(CUI_DRAW_TEXT);
    c[6].payload.legacy.font_size = 12;
    c[6].payload.legacy.text = (cui_str)CUI_STR("abc");
    c[7] = legacy(CUI_DRAW_IMAGE);
    c[7].payload.legacy.bitmap =
        (cui_bitmap_paint){.bitmap = {pixels, 2, 2, 8, 1}, .source = {0, 0, 2, 2}, .opacity = 1};
    c[8] = legacy(CUI_DRAW_MESH);
    c[8].payload.legacy.mesh = (cui_mesh_paint){
        .vertices = vertices, .vertex_count = 3, .scale = 1, .color = {1, 0, 0, 1}};
    c[9] = legacy(CUI_DRAW_CLIP_POP);
    cui_render_v2_options o = options();
    cui_draw_list_v2 l = list(c, 10);
    cui_software_v2_plan plan;
    assert(cui_software_v2_validate(&l, &o, true, true, &plan) == CUI_OK);
    assert(plan.commands == 10 && plan.backgrounds == 1 && plan.background_pixels == 16);
    assert(plan.background_bytes == 64 && plan.scratch_bytes == 768);
    assert(plan.cpu_bytes == plan.scratch_bytes + plan.background_bytes + plan.shadow_bytes);
    assert(plan.metadata_bytes == 10 * sizeof(cui_software_v2_descriptor));
    assert(plan.needs_text_preparation && plan.needs_shadow_preparation);
    /* Inactive resource/shape/geometry fields must never be dereferenced or validated. */
    c[3].payload.legacy.text.data = (const char *)(uintptr_t)1;
    c[3].payload.legacy.text.length = SIZE_MAX;
    c[3].payload.legacy.font_size = NAN;
    c[3].payload.legacy.stroke = NAN;
    c[3].payload.legacy.corner_radii.top_left = NAN;
    c[8].payload.legacy.bounds = (cui_rect){NAN, NAN, NAN, NAN};
    c[9].payload.legacy.bounds = (cui_rect){NAN, NAN, NAN, NAN};
    assert(cui_software_v2_validate(&l, &o, true, true, &plan) == CUI_OK);
    c[7].payload.legacy.bitmap.source.w = 3;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c[7].payload.legacy.bitmap.source.w = 2;
    vertices[2].t = NAN;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    vertices[2].t = 0;
    c[6].payload.legacy.text_style.letter_spacing = NAN;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c[6].payload.legacy.text_style.letter_spacing = 0;
    cui_software_v2_plan untouched = {0};
    assert(cui_software_v2_validate(&l, &o, false, true, &untouched) == CUI_ERROR_INVALID_ACTION);
    c[6].payload.legacy.text_style.letter_spacing = 1;
    assert(cui_software_v2_validate(&l, &o, true, false, &untouched) == CUI_ERROR_INVALID_ACTION);
    c[6].payload.legacy.text_style.letter_spacing = 0;
    o.cpu_bytes = plan.cpu_bytes - 1;
    rejects(l, o, CUI_ERROR_CAPACITY);
    o = options();
    o.texture_bytes = 63;
    rejects(l, o, CUI_ERROR_CAPACITY);
    o = options();
    o.metadata_bytes = plan.metadata_bytes - 1;
    rejects(l, o, CUI_ERROR_CAPACITY);
    o = options();
    l.required_features = 0;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 10);
    c[9].kind = 123;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c[9] = legacy(CUI_DRAW_CLIP_POP);
    c[9].payload.legacy.type = CUI_DRAW_RECT;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
}
static void clips_and_headers(void) {
    cui_draw_command_v2 c[130];
    cui_render_v2_options o = options();
    o.metadata_bytes = 32768;
    cui_draw_list_v2 l = list(c, 0);
    cui_software_v2_plan p;
    assert(cui_software_v2_validate(&l, &o, false, false, &p) == CUI_OK);
    assert(!p.commands && !p.backgrounds && p.cpu_bytes == 768);
    for (size_t i = 0; i < 64; i++)
        c[i] = legacy(CUI_DRAW_CLIP_PUSH);
    for (size_t i = 64; i < 128; i++)
        c[i] = legacy(CUI_DRAW_CLIP_POP);
    l.count = 128;
    assert(cui_software_v2_validate(&l, &o, false, false, &p) == CUI_OK);
    l.count = 127;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l.count = 129;
    c[128] = legacy(CUI_DRAW_CLIP_POP);
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c[64] = legacy(CUI_DRAW_CLIP_PUSH);
    l.count = 65;
    rejects(l, o, CUI_ERROR_CAPACITY);
    l = list(c, 0);
    l.command_stride--;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 0);
    l.header_size--;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 0);
    l.abi_version = 1;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 0);
    l.background_output = 123;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 0);
    l.required_features = 2;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 0);
    l.error = CUI_ERROR_CAPACITY;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 0);
    l.width = NAN;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 16385);
    rejects(l, o, CUI_ERROR_CAPACITY);
    l = list(NULL, 1);
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    l = list(c, 0);
    o.size--;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
}
static void aggregate_and_alias(void) {
    cui_render_v2_options o = options();
    cui_draw_command_v2 c[2] = {{.kind = CUI_DRAW_V2_BACKGROUND,
                                 .payload.background = {.bounds = {0, 0, 16, 12},
                                                        .brush = {.kind = CUI_BACKGROUND_SOLID},
                                                        .opacity = 1}}};
    c[1] = c[0];
    cui_draw_list_v2 l = list(c, 2);
    cui_software_v2_plan p;
    o.max_pixels = 383;
    rejects(l, o, CUI_ERROR_CAPACITY);
    o = options();
    o.transfer_bytes = 1535;
    rejects(l, o, CUI_ERROR_CAPACITY);
    o = options();
    o.cpu_bytes = 2303;
    rejects(l, o, CUI_ERROR_CAPACITY);
    o.cpu_bytes = 2304;
    assert(cui_software_v2_validate(&l, &o, false, false, &p) == CUI_OK);
    assert(p.cpu_bytes == 2304 && p.background_pixels == 384);
    unsigned char before[sizeof(c)];
    memcpy(before, c, sizeof(c));
    assert(cui_software_v2_validate(&l, &o, false, false, (cui_software_v2_plan *)(void *)c) ==
           CUI_ERROR_INVALID_ACTION);
    assert(!memcmp(before, c, sizeof(c)));
    o = options();
    o.scale_x = 0.75f;
    o.scale_y = 1.25f;
    o.origin_x = 0.125f;
    assert(cui_software_v2_validate(&l, &o, false, false, &p) == CUI_OK);
    cui_background_prepared_query q;
    cui_rect broad = {-FLT_MAX / 4, -FLT_MAX / 4, FLT_MAX / 2, FLT_MAX / 2};
    assert(cui_background_prepare_query(c[0].payload.background, &o, broad, &q) == CUI_OK);
    assert(p.background_pixels == 2 * (size_t)q.width * q.height);
}
static void text_resources(void) {
    cui_draw_command_v2 c = legacy(CUI_DRAW_TEXT);
    cui_render_v2_options o = options();
    cui_draw_list_v2 l = list(&c, 1);
    cui_software_v2_plan p;
    c.payload.legacy.font_size = 12;
    c.payload.legacy.text_source = (cui_text_line_source){(cui_str)CUI_STR("a\xc3\xa9z"), 0, 4};
    cui_text_color_span spans[2] = {{0, 1, {1, 2, 3, 4}}, {1, 3, {5, 6, 7, 8}}};
    c.payload.legacy.text_colors = spans;
    c.payload.legacy.text_color_count = 2;
    assert(cui_software_v2_validate(&l, &o, true, true, &p) == CUI_OK);
    assert(p.inspected_bytes == 12 && p.needs_text_preparation);
    spans[1].byte_start = 2;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    spans[1].byte_start = 1;
    spans[1].byte_end = 5;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    spans[1].byte_end = 3;
    cui_text_gradient_stop stops[2] = {{0, {0, 0, 0, 0}}, {1, {1, 2, 3, 4}}};
    c.payload.legacy.text_paint =
        (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR, .stops = stops, .stop_count = 2};
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_color_count = 0;
    assert(cui_software_v2_validate(&l, &o, true, true, &p) == CUI_OK);
    stops[1].position = -1;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    stops[1].position = 1;
    c.payload.legacy.text_source.byte_end = 2;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_source = (cui_text_line_source){0};
    c.payload.legacy.text = (cui_str){NULL, 1};
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    /* Context presence, including empty context, makes raw legacy text inactive. */
    c.payload.legacy.text_paint = (cui_text_paint){0};
    c.payload.legacy.text = (cui_str){(const char *)(uintptr_t)1, SIZE_MAX};
    c.payload.legacy.text_source = (cui_text_line_source){{"", 0}, 0, 0};
    assert(cui_software_v2_validate(&l, &o, true, true, &p) == CUI_OK);
    assert(p.inspected_bytes == 0);
    c.payload.legacy.text_source = (cui_text_line_source){{"ok", 2}, 0, 2};
    assert(cui_software_v2_validate(&l, &o, true, true, &p) == CUI_OK);
    assert(p.inspected_bytes == 2);
    c.payload.legacy.text = (cui_str)CUI_STR("valid inactive text");
    c.payload.legacy.text_source.byte_end = 3;
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_source = (cui_text_line_source){{"\xff", 1}, 0, 1};
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_source = (cui_text_line_source){{NULL, 1}, 0, 0};
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_source = (cui_text_line_source){{NULL, 0}, 0, 1};
    rejects(l, o, CUI_ERROR_INVALID_ACTION);
}
typedef struct {
    size_t allocations, frees, live, peak, fail;
    cui_software_v2_workspace *w;
} allocator;
static void release(void *user, void *p) {
    allocator *a = user;
    assert(p);
    a->frees++;
    a->live--;
    free(p);
    if (a->w) {
        cui_software_v2_workspace_end(a->w);
        assert(a->w->busy);
    }
}
static void *allocate(void *user, size_t n) {
    allocator *a = user;
    a->allocations++;
    if (a->w) {
        assert(cui_software_v2_workspace_begin(a->w, 1, 1, allocate, release, a) ==
               CUI_ERROR_INVALID_ACTION);
        cui_software_v2_workspace_end(a->w);
        assert(a->w->busy);
    }
    if (a->allocations == a->fail)
        return NULL;
    void *p = malloc(n);
    assert(p);
    a->live++;
    if (a->live > a->peak)
        a->peak = a->live;
    return p;
}
static void workspace(void) {
    cui_software_v2_workspace w = {0}, previous = w;
    allocator a = {.w = &w, .fail = 1};
    assert(cui_software_v2_workspace_begin(&w, 256, 128, allocate, release, &a) ==
           CUI_ERROR_CAPACITY);
    assert(!a.live && !memcmp(&w, &previous, sizeof(w)));
    a.fail = 3;
    assert(cui_software_v2_workspace_begin(&w, 256, 128, allocate, release, &a) ==
           CUI_ERROR_CAPACITY);
    assert(!a.live && a.frees == 1 && !memcmp(&w, &previous, sizeof(w)));
    a.fail = 0;
    assert(cui_software_v2_workspace_begin(&w, 1024, 128, allocate, release, &a) == CUI_OK);
    assert(a.live == 2 && a.peak == 2);
    cui_draw_command_v2 *command =
        cui_software_v2_workspace_take(&w, false, sizeof(*command), _Alignof(cui_draw_command_v2));
    assert(command && (uintptr_t)command % _Alignof(cui_draw_command_v2) == 0);
    *command = (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND};
    assert(command->kind == CUI_DRAW_V2_BACKGROUND);
    assert(cui_software_v2_workspace_take(&w, false, 1, 1));
    void *p = cui_software_v2_workspace_take(&w, false, sizeof(cui_fundamental_alignment_internal),
                                             _Alignof(cui_fundamental_alignment_internal));
    assert(p && (uintptr_t)p % _Alignof(cui_fundamental_alignment_internal) == 0);
    size_t used = w.cpu_used;
    assert(!cui_software_v2_workspace_take(&w, false, SIZE_MAX, 1) && w.cpu_used == used);
    assert(!cui_software_v2_workspace_take(&w, false, 1, 3));
    assert(cui_software_v2_workspace_take(&w, true, 128, 1));
    assert(!cui_software_v2_workspace_take(&w, true, 1, 1));
    assert(cui_software_v2_workspace_begin(&w, 1, 1, allocate, release, &a) ==
           CUI_ERROR_INVALID_ACTION);
    cui_software_v2_workspace_end(&w);
    assert(!a.live && !w.active && !w.busy);
    cui_software_v2_workspace_end(&w);
    assert(cui_software_v2_workspace_begin(&w, (size_t)32 * 1024 * 1024 + 1, 0, allocate, release,
                                           &a) == CUI_ERROR_CAPACITY);
    assert(cui_software_v2_workspace_begin(&w, 0, 0, allocate, release, &a) == CUI_OK);
    assert(!cui_software_v2_workspace_take(&w, false, 1, 1));
    cui_software_v2_workspace_end(&w);
}
int main(void) {
    commands();
    clips_and_headers();
    aggregate_and_alias();
    text_resources();
    workspace();
    return 0;
}
