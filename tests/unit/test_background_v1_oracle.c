/* SPDX-License-Identifier: MIT */
/* Frozen pre-V2 active-field commands and exact software RGBA runs. */
#include "cui/cui.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24,
               "fixture binary32 target");
static void scalar(FILE *f, float x) {
    uint32_t bits = 0;
    if (x != 0)
        memcpy(&bits, &x, sizeof bits); /* normalize signed zero, never padding */
    fprintf(f, " %08" PRIx32, bits);
}
static void rect(FILE *f, cui_rect r) {
    scalar(f, r.x);
    scalar(f, r.y);
    scalar(f, r.w);
    scalar(f, r.h);
}
static void color(FILE *f, cui_color c) {
    fprintf(f, " %u %u %u %u", c.r, c.g, c.b, c.a);
}
static void radii(FILE *f, cui_corner_radii r) {
    scalar(f, r.top_left);
    scalar(f, r.top_right);
    scalar(f, r.bottom_left);
    scalar(f, r.bottom_right);
}
static void string(FILE *f, cui_str s) {
    fprintf(f, " %zu:", s.length);
    for (size_t i = 0; i < s.length; i++)
        fprintf(f, "%02x", (unsigned char)s.data[i]);
}
static void shape(FILE *f, const cui_draw_command *c) {
    fprintf(f, " %u", c->custom_corners);
    if (c->custom_corners)
        radii(f, c->corner_radii);
    else
        scalar(f, c->radius);
}
static void command(FILE *f, const cui_draw_command *c) {
    fprintf(f, "C %u", (unsigned)c->type);
    if (c->type != CUI_DRAW_CLIP_POP)
        rect(f, c->bounds);
    switch (c->type) {
    case CUI_DRAW_RECT:
        color(f, c->color);
        shape(f, c);
        break;
    case CUI_DRAW_BORDER:
        color(f, c->color);
        shape(f, c);
        fprintf(f, " %u", c->custom_border_edges);
        if (c->custom_border_edges) {
            scalar(f, c->border_edges.left);
            scalar(f, c->border_edges.right);
            scalar(f, c->border_edges.top);
            scalar(f, c->border_edges.bottom);
        } else
            scalar(f, c->stroke);
        break;
    case CUI_DRAW_CLIP_PUSH:
    case CUI_DRAW_CLIP_POP:
        break;
    case CUI_DRAW_TEXT:
        if (c->text_paint.kind == CUI_TEXT_PAINT_SOLID)
            color(f, c->color);
        scalar(f, c->font_size);
        fprintf(f, " %u %u %u %" PRIu32, (unsigned)c->text_style.weight, c->text_style.italic,
                c->text_style.monospace, c->text_style.family);
        scalar(f, c->text_style.letter_spacing);
        string(f, c->text);
        fprintf(f, " %u", c->text_source.paragraph.data ? 1u : 0u);
        if (c->text_source.paragraph.data) {
            string(f, c->text_source.paragraph);
            fprintf(f, " %zu %zu", c->text_source.byte_start, c->text_source.byte_end);
        }
        fprintf(f, " %zu", c->text_color_count);
        for (size_t i = 0; i < c->text_color_count; i++) {
            fprintf(f, " %zu %zu", c->text_colors[i].byte_start, c->text_colors[i].byte_end);
            color(f, c->text_colors[i].color);
        }
        fprintf(f, " %u", (unsigned)c->text_paint.kind);
        if (c->text_paint.kind == CUI_TEXT_PAINT_LINEAR) {
            scalar(f, c->text_paint.x0);
            scalar(f, c->text_paint.y0);
            scalar(f, c->text_paint.x1);
            scalar(f, c->text_paint.y1);
            fprintf(f, " %u %zu", (unsigned)c->text_paint.anchor, c->text_paint.stop_count);
            for (size_t i = 0; i < c->text_paint.stop_count; i++) {
                scalar(f, c->text_paint.stops[i].position);
                color(f, c->text_paint.stops[i].color);
            }
        }
        break;
    case CUI_DRAW_IMAGE:
        shape(f, c);
        fprintf(f, " %u %u %" PRIu64, c->bitmap.bitmap.width, c->bitmap.bitmap.height,
                c->bitmap.bitmap.revision);
        rect(f, c->bitmap.source);
        scalar(f, c->bitmap.opacity);
        fprintf(f, " %u %u", c->bitmap.grayscale, c->bitmap.custom_tint);
        if (c->bitmap.custom_tint)
            color(f, c->bitmap.tint);
        /* Row padding and addresses are representation, actual pixel bytes are owned semantics. */
        for (unsigned y = 0; y < c->bitmap.bitmap.height; y++)
            for (unsigned x = 0; x < c->bitmap.bitmap.width * 4; x++)
                fprintf(f, " %02x",
                        c->bitmap.bitmap.pixels[(size_t)y * c->bitmap.bitmap.stride + x]);
        break;
    case CUI_DRAW_MESH:
        fprintf(f, " %zu", c->mesh.vertex_count);
        scalar(f, c->mesh.origin_x);
        scalar(f, c->mesh.origin_y);
        scalar(f, c->mesh.scale);
        scalar(f, c->mesh.color.r);
        scalar(f, c->mesh.color.g);
        scalar(f, c->mesh.color.b);
        scalar(f, c->mesh.color.a);
        for (size_t i = 0; i < c->mesh.vertex_count; i++) {
            scalar(f, c->mesh.vertices[i].x);
            scalar(f, c->mesh.vertices[i].y);
            scalar(f, c->mesh.vertices[i].s);
            scalar(f, c->mesh.vertices[i].t);
        }
        break;
    case CUI_DRAW_DROP_SHADOW:
    case CUI_DRAW_INSET_SHADOW:
        rect(f, c->shadow.caster);
        radii(f, c->shadow.radii);
        scalar(f, c->shadow.layer.offset_x);
        scalar(f, c->shadow.layer.offset_y);
        scalar(f, c->shadow.layer.sigma);
        scalar(f, c->shadow.layer.spread);
        color(f, c->shadow.layer.color);
        break;
    default:
        assert(!"unknown V1 kind");
    }
    fputc('\n', f);
}
static void same_normalized(cui_draw_command a, cui_draw_command b) {
    FILE *first = tmpfile(), *second = tmpfile();
    assert(first && second);
    command(first, &a);
    command(second, &b);
    rewind(first);
    rewind(second);
    for (;;) {
        int x = fgetc(first), y = fgetc(second);
        assert(x == y);
        if (x == EOF)
            break;
    }
    fclose(first);
    fclose(second);
}
static void normalizer_contract(void) {
    cui_draw_command a = {
        .type = CUI_DRAW_RECT, .bounds = {1, 2, 3, 4}, .color = {5, 6, 7, 8}, .radius = 2};
    cui_draw_command b = a;
    b.font_size = NAN;
    b.mesh.vertices = NULL;
    b.mesh.vertex_count = 99;
    b.text = (cui_str){NULL, 123};
    b.corner_radii = (cui_corner_radii){NAN, NAN, NAN, NAN};
    b.border_edges = (cui_border_edges){NAN, NAN, NAN, NAN};
    same_normalized(a, b); /* all changed fields are inactive for RECT */
    unsigned char one[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    unsigned char two[12] = {10, 20, 30, 40, 9, 9, 9, 9, 50, 60, 70, 80};
    a = (cui_draw_command){
        .type = CUI_DRAW_IMAGE,
        .bounds = {0, 0, 1, 2},
        .bitmap = {.bitmap = {one, 1, 2, 4, 1}, .source = {0, 0, 1, 2}, .opacity = 1}};
    b = a;
    b.bitmap.bitmap.pixels = two;
    b.bitmap.bitmap.stride = 8;
    same_normalized(a, b); /* actual pixels equal; addresses/row padding differ */
}
static cui_size measure(void *u, cui_str text, float size, cui_text_style style) {
    (void)u;
    (void)style;
    return (cui_size){(float)text.length * size * .5f, size};
}
static unsigned prepaint_calls;
static bool prepaint(const cui_prepaint_info *info, const void *payload, cui_prepaint_writer *w) {
    assert(*(const unsigned *)payload == 1234);
    prepaint_calls++;
    cui_draw_command c = {.type = CUI_DRAW_RECT,
                          .bounds = {info->bounds.x + 2, info->bounds.y + 2, 13, 9},
                          .color = {20, 240, 80, 160},
                          .radius = 2};
    assert(cui_prepaint_emit(w, &c, false));
    c.bounds = (cui_rect){68, 54, 18, 10};
    c.color = (cui_color){240, 30, 180, 190};
    assert(cui_prepaint_emit(w, &c, true));
    char text[] = "copied";
    c.type = CUI_DRAW_TEXT;
    c.text = (cui_str){text, 6};
    c.font_size = 8;
    assert(cui_prepaint_emit(w, &c, false));
    memset(text, 'X', 6);
    return true;
}
static const unsigned char bitmap_bytes[24] = {240, 20, 10,  255, 10,  220, 30, 128, 9, 9, 9, 9,
                                               10,  20, 240, 200, 240, 220, 10, 80,  7, 7, 7, 7};
static const cui_mesh_vertex triangle[3] = {{0, 0, 0, 0}, {18, 0, 1, 0}, {0, 14, 0, 1}};
static cui_bitmap_paint bitmap(void) {
    return (cui_bitmap_paint){.bitmap = {bitmap_bytes, 2, 2, 12, UINT64_C(0x100000005)},
                              .source = {0, 0, 2, 2},
                              .opacity = .75f,
                              .custom_tint = true,
                              .tint = {220, 255, 240, 210}};
}
static void compose(cui_context *ui, unsigned scene, unsigned stage) {
    float shift = stage * .25f;
    cui_begin_box(ui, 1,
                  (cui_box){.padding = 3 + shift,
                            .background = {40, 50, 60, 255},
                            .custom_opacity = true,
                            .opacity = stage == 2 ? .625f : 1});
    cui_begin_box(ui, 2,
                  (cui_box){.width = 52 + stage * 2,
                            .height = 38,
                            .fixed_width = true,
                            .fixed_height = true,
                            .background = {210, 90, 30, 170},
                            .border = {20, 180, 220, 210},
                            .border_width = 2,
                            .custom_corners = true,
                            .corner_radii = {7, 3, 5, 9},
                            .clip = scene == 2 || scene == 5});
    if (scene == 1) {
        const cui_box_shadow layers[] = {{.kind = CUI_BOX_SHADOW_DROP,
                                          .layer = {.offset_x = 3,
                                                    .offset_y = 2,
                                                    .sigma = 1.5f,
                                                    .spread = 1,
                                                    .color = {30, 20, 80, 130}}},
                                         {.kind = CUI_BOX_SHADOW_INSET,
                                          .layer = {.offset_x = -2,
                                                    .offset_y = 1,
                                                    .sigma = 1,
                                                    .spread = 2,
                                                    .color = {5, 80, 30, 160}}}};
        assert(cui_set_box_shadows(ui, layers, 2, (cui_corner_radii){7, 3, 5, 9}));
        assert(
            cui_set_outline(ui, 2, (cui_color){230, 240, 10, 130}, (cui_corner_radii){7, 3, 5, 9}));
    }
    if (scene == 2) {
        assert(cui_set_underlay_ex(ui,
                                   (cui_underlay_layout){.bounds = {-2, 1, -10, -8},
                                                         .grow_width = true,
                                                         .grow_height = true,
                                                         .horizontal = CUI_ALIGN_CENTER,
                                                         .vertical = CUI_ALIGN_END},
                                   (cui_color){20, 100, 230, 150}, (cui_corner_radii){4, 2, 6, 1}));
        cui_begin_absolute_box(ui, 3, (cui_box){.background = {50, 240, 80, 120}},
                               (cui_rect){40, 25, 28, 25});
        cui_end_box(ui);
    }
    if (scene == 3) {
        assert(cui_set_bitmap_underlay(ui, (cui_underlay_layout){.bounds = {1, 1, 34, 20}},
                                       bitmap(), (cui_corner_radii){3, 5, 1, 7}));
        assert(cui_paint_mesh(
            ui, 3,
            (cui_mesh_paint){.vertices = triangle,
                             .vertex_count = 3,
                             .origin_x = 2,
                             .origin_y = 1,
                             .scale = 1,
                             .color = {.3f, .1f, .05f, .5f}},
            (cui_box){.width = 22, .height = 16, .fixed_width = true, .fixed_height = true}));
    }
    if (scene == 4) {
        unsigned value = 1234;
        assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = prepaint,
                                                           .payload = &value,
                                                           .payload_bytes = sizeof value,
                                                           .command_budget = 7,
                                                           .deferred_layer = 12}));
        value = 0;
    }
    if (scene == 5) {
        cui_begin_portal(ui, 4, (cui_box){.background = {80, 10, 240, 160}, .radius = 4},
                         (cui_rect){64 + shift, 8, 24, 20}, 9);
        cui_text(ui, (cui_str)CUI_STR("portal"), 8, (cui_color){255, 255, 255, 255});
        cui_end_box(ui);
    }
    if (scene == 6) {
        cui_text_gradient_stop stops[] = {{0, {255, 20, 30, 40}}, {1, {10, 30, 255, 210}}};
        assert(cui_text_paragraph_painted(
            ui, (cui_str)CUI_STR("gradient rows"), 8, 9,
            (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD, .italic = true, .family = 7}, true,
            CUI_TEXT_ALIGN_CENTER,
            (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                             .x1 = 1,
                             .anchor = CUI_TEXT_ANCHOR_BLOCK,
                             .stops = stops,
                             .stop_count = 2}));
        stops[0].color = (cui_color){0}; /* copied stop payload */
        cui_text_color_span spans[] = {{0, 2, {240, 30, 70, 170}}, {3, 5, {20, 240, 90, 200}}};
        assert(cui_text_paragraph_colored(ui, (cui_str)CUI_STR("ab cd"), 8,
                                          (cui_color){255, 255, 255, 255}, 9,
                                          (cui_text_style){.monospace = true}, false, spans, 2));
        spans[0].color = (cui_color){0}; /* copied span payload */
    }
    cui_text(ui, (cui_str)CUI_STR("V1 text"), 8, (cui_color){240, 230, 220, 180});
    cui_end_box(ui);
    if (scene == 7) {
        const cui_box_shadow layer = {
            .kind = CUI_BOX_SHADOW_DROP,
            .layer = {.offset_x = -2, .offset_y = 3, .sigma = 2, .color = {50, 20, 100, 140}}};
        assert(cui_element_set_box_shadows(ui, 2, &layer, 1, (cui_corner_radii){7, 3, 5, 9}));
        cui_begin_box(ui, 0,
                      (cui_box){.width = 9,
                                .height = 6,
                                .fixed_width = true,
                                .fixed_height = true,
                                .background = {190, 220, 10, 180}});
        assert(cui_set_underlay(ui, (cui_rect){1, 1, 5, 3}, (cui_color){240, 20, 50, 190},
                                (cui_corner_radii){1, 1, 1, 1}));
        cui_end_box(ui);
    }
    if (scene == 3)
        assert(cui_paint_bitmap(ui, 5, bitmap(),
                                (cui_box){.width = 18,
                                          .height = 12,
                                          .fixed_width = true,
                                          .fixed_height = true,
                                          .radius = 2}));
    cui_end_box(ui);
}
static void pixels(FILE *f, cui_software *r, cui_draw_list list) {
    cui_draw_command filtered[256];
    size_t n = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type != CUI_DRAW_TEXT) {
            assert(n < 256);
            filtered[n++] = list.commands[i];
        }
    cui_draw_list paint = {filtered, n, list.width, list.height};
    assert(cui_software_render(r, &paint, (cui_color){13, 23, 33, 255}));
    const unsigned char *p = cui_software_pixels(r, NULL, NULL);
    size_t at = 0;
    /* Exact RGBA bytes, lossless runs; no self-derived image hash expectation. */
    while (at < 96 * 72) {
        size_t end = at + 1;
        while (end < 96 * 72 && memcmp(p + at * 4, p + end * 4, 4) == 0)
            end++;
        fprintf(f, "P %zu %u %u %u %u\n", end - at, p[at * 4], p[at * 4 + 1], p[at * 4 + 2],
                p[at * 4 + 3]);
        at = end;
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    normalizer_contract();
#ifdef CUI_CAPTURE_V1
    FILE *out = fopen(argv[1], "wb");
#else
    FILE *out = tmpfile();
#endif
    assert(out);
    cui_context *ui = cui_create((cui_config){
        .max_elements = 128, .max_commands = 256, .frame_bytes = 32768, .measure_styled = measure});
    assert(ui);
    cui_software *r = cui_software_create(96, 72, NULL);
    assert(r);
    for (unsigned scene = 0; scene < 8; scene++)
        for (unsigned stage = 0; stage < 3; stage++) {
            prepaint_calls = 0;
            assert(cui_begin_frame(ui, 96, 72, .016f));
            compose(ui, scene, stage);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            assert(prepaint_calls == (scene == 4 ? 1u : 0u));
            fprintf(out, "S %u %u %zu %u\n", scene, stage, list.count, prepaint_calls);
            for (size_t i = 0; i < list.count; i++)
                command(out, &list.commands[i]);
            pixels(out, r, list);
        }
    cui_software_destroy(r);
    cui_destroy(ui);
    assert(!ferror(out));
#ifndef CUI_CAPTURE_V1
    rewind(out);
    FILE *expected = fopen(argv[1], "rb");
    assert(expected);
    size_t offset = 0;
    for (;;) {
        int a = fgetc(out), b = fgetc(expected);
        if (a != b) {
            fprintf(stderr, "V1 frozen oracle differs at byte %zu (actual %d expected %d)\n",
                    offset, a, b);
            return 1;
        }
        if (a == EOF)
            break;
        offset++;
    }
    fclose(expected);
#endif
    fclose(out);
    puts("frozen V1 command/software oracle: PASS");
    return 0;
}
