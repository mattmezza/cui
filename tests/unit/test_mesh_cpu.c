/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "mesh_oracle.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define SIDE 16
static unsigned cases;
static unsigned char expected[SIDE * SIDE * 4];
static void compare(cui_mesh_paint paint, cui_rect clip, cui_color clear) {
    assert(cui_mesh_valid(paint));
    mesh_oracle ref = {SIDE, SIDE, expected, clip};
    for (size_t i = 0; i < SIDE * SIDE; i++) {
        expected[4 * i] = clear.r;
        expected[4 * i + 1] = clear.g;
        expected[4 * i + 2] = clear.b;
        expected[4 * i + 3] = clear.a;
    }
    mesh(&ref, &paint);
    cui_software *actual = cui_software_create(SIDE, SIDE, NULL);
    assert(actual);
    cui_draw_command commands[3] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = clip},
                                    {.type = CUI_DRAW_MESH, .mesh = paint},
                                    {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {commands, 3, SIDE, SIDE};
    assert(cui_software_render(actual, &list, clear));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(actual, &w, &h);
    assert(w == SIDE && h == SIDE);
    for (size_t i = 0; i < sizeof expected; i++) {
        if (pixels[i] != expected[i]) {
            fprintf(stderr, "case %u byte %zu: actual %u oracle %u\n", cases, i, pixels[i],
                    expected[i]);
            abort();
        }
    }
    cui_software_destroy(actual);
    cases++;
}
static void ordered_commands(cui_mesh_paint p) {
    unsigned char previous[sizeof expected];
    cui_software *actual = cui_software_create(SIDE, SIDE, NULL);
    assert(actual);
    for (unsigned order = 0; order < 2; order++) {
        memset(expected, 0, sizeof expected);
        mesh_oracle ref = {SIDE, SIDE, expected, {0, 0, SIDE, SIDE}};
        cui_draw_command commands[2] = {{.type = CUI_DRAW_MESH, .mesh = p},
                                        {.type = CUI_DRAW_MESH, .mesh = p}};
        commands[order].mesh.color = (cui_linear_color){.5f, 0, 0, .5f};
        commands[1 - order].mesh.color = (cui_linear_color){0, 0, .5f, .5f};
        mesh(&ref, &commands[0].mesh);
        mesh(&ref, &commands[1].mesh);
        cui_draw_list list = {commands, 2, SIDE, SIDE};
        assert(cui_software_render(actual, &list, (cui_color){0, 0, 0, 0}));
        assert(!memcmp(cui_software_pixels(actual, NULL, NULL), expected, sizeof expected));
        if (!order)
            memcpy(previous, expected, sizeof previous);
        else
            assert(memcmp(previous, expected, sizeof previous));
        cases++;
    }
    cui_software_destroy(actual);
}
static cui_mesh_paint paint(cui_mesh_vertex *v, size_t n) {
    return (cui_mesh_paint){v, n, 0, 0, 1, {.1f, .2f, .3f, .5f}};
}
int main(void) {
    cui_rect full = {0, 0, SIDE, SIDE};
    cui_color transparent = {0, 0, 0, 0};
    cui_mesh_vertex v[6] = {{0, 0, 0, 0}, {8, 0, 0, 0}, {0, 8, 0, 0}};
    cui_mesh_paint p = paint(v, 3);
    compare(p, full, transparent); /* solid */
    assert(expected[3] == 128);
    ordered_commands(p);
    for (size_t i = 0; i < 3; i++)
        v[i].s = v[i].x - .5f;
    for (int t = -1; t <= 1; t++) {
        for (size_t i = 0; i < 3; i++)
            v[i].t = (float)t;
        compare(p, full, transparent); /* gradient zero: outside, boundary, inside */
        assert(expected[3] == (t < 0 ? 0 : t == 0 ? 64 : 128));
    }
    v[0] = (cui_mesh_vertex){0, 0, -1, 1};
    v[1] = (cui_mesh_vertex){12, 0, 1, 1};
    v[2] = (cui_mesh_vertex){0, 12, 0, -1};
    compare(p, full, (cui_color){19, 71, 129, 113}); /* curved, transparent destination */
    cui_mesh_vertex swap = v[1];
    v[1] = v[2];
    v[2] = swap;
    unsigned char winding[sizeof expected];
    memcpy(winding, expected, sizeof winding);
    compare(p, full, (cui_color){19, 71, 129, 113});
    assert(!memcmp(winding, expected, sizeof winding));
    /* Two triangles sharing pixel-center diagonal: no hole or double coverage. */
    cui_mesh_vertex square[6] = {{0, 0, 0, 0}, {8, 0, 0, 0}, {0, 8, 0, 0},
                                 {8, 0, 0, 0}, {8, 8, 0, 0}, {0, 8, 0, 0}};
    p = paint(square, 6);
    compare(p, full, transparent);
    for (unsigned y = 0; y < 8; y++)
        for (unsigned x = 0; x < 8; x++)
            assert(expected[(y * SIDE + x) * 4 + 3] == 128);
    memcpy(square + 3, square, 3 * sizeof *square);
    compare(p, full, transparent); /* overlap must blend twice, in order */
    assert(expected[3] == 192);
    square[2] = square[1];
    square[5] = square[4];
    compare(p, full, transparent); /* degenerate empty geometry */
    for (size_t i = 0; i < sizeof expected; i++)
        assert(expected[i] == 0);
    /* Deterministic clipping, placement, SDR conversion and coverage sweep. */
    for (unsigned i = 0; i < 160; i++) {
        v[0] = (cui_mesh_vertex){-4, -2, -1, (float)(i % 3) - 1};
        v[1] = (cui_mesh_vertex){18, 1, 2, .5f};
        v[2] = (cui_mesh_vertex){2, 19, 0, 1};
        p = paint(v, 3);
        p.origin_x = (float)(i % 7) * .25f - 1;
        p.origin_y = (float)(i % 5) * .125f;
        p.scale = .25f + (float)(i % 9) * .25f;
        p.color = (cui_linear_color){-.1f, .0031308f * .5f, 2, (float)(i % 5) * .25f};
        compare(p, (cui_rect){1.25f, 2.5f, 9.5f, 10.25f},
                (cui_color){31, 73, 137, (unsigned char)i});
    }
    printf("mesh CPU: %u bit-exact oracle cases, edge/overlap/gradient assertions passed\n", cases);
    return 0;
}
