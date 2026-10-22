/* SPDX-License-Identifier: MIT */
/* Independent device-space oracle derived from pre-extraction 939c5c2f. Do not use shared mesh
 * helpers here: independent arithmetic and ordered blending are intentional. */
#ifndef CUI_TEST_SDL_MESH_ORACLE_H
#define CUI_TEST_SDL_MESH_ORACLE_H
#include "cui/cui.h"
#include <math.h>
#include <string.h>
static double reference_sx, reference_sy, reference_ox, reference_oy;
typedef struct {
    unsigned width, height;
    unsigned char *pixels;
    cui_rect clip;
} mesh_oracle;
static void blend(mesh_oracle *r, int x, int y, cui_color c) {
    if (x < 0 || y < 0 || (unsigned)x >= r->width || (unsigned)y >= r->height ||
        x + 0.5f < r->clip.x || y + 0.5f < r->clip.y || x + 0.5f >= r->clip.x + r->clip.w ||
        y + 0.5f >= r->clip.y + r->clip.h)
        return;
    unsigned char *p = r->pixels + ((size_t)y * r->width + (unsigned)x) * 4;
    unsigned alpha = c.a * 255u + p[3] * (255u - c.a);
    if (!alpha) {
        memset(p, 0, 4);
        return;
    }
    unsigned src[3] = {c.r, c.g, c.b};
    for (unsigned i = 0; i < 3; i++)
        p[i] =
            (unsigned char)((src[i] * c.a * 255u + p[i] * p[3] * (255u - c.a) + alpha / 2) / alpha);
    p[3] = (unsigned char)((alpha + 127) / 255);
}
/* Screen-space edge equations use double throughout, including clipping before
 * integer conversion. Positive orientation has its interior on the left. */
typedef struct {
    double x, y, s, t;
} mesh_vertex;
static double mesh_edge(mesh_vertex a, mesh_vertex b, double x, double y) {
    return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
}
static bool mesh_top_left(mesh_vertex a, mesh_vertex b) {
    return b.y < a.y || (b.y == a.y && b.x > a.x);
}
static unsigned char mesh_channel(double premultiplied, double alpha) {
    double linear = alpha > 0 ? premultiplied / alpha : 0;
    linear = fmax(0, fmin(1, linear));
    double encoded = linear <= .0031308 ? 12.92 * linear : 1.055 * pow(linear, 1 / 2.4) - .055;
    return (unsigned char)lround(encoded * 255);
}
static void mesh(mesh_oracle *r, const cui_mesh_paint *p) {
    cui_color color = {mesh_channel(p->color.r, p->color.a), mesh_channel(p->color.g, p->color.a),
                       mesh_channel(p->color.b, p->color.a), 0};
    for (size_t i = 0; i < p->vertex_count; i += 3) {
        mesh_vertex v[3];
        for (size_t j = 0; j < 3; j++) {
            const cui_mesh_vertex *in = &p->vertices[i + j];
            v[j] = (mesh_vertex){
                (p->origin_x + (double)p->scale * in->x) * reference_sx + reference_ox,
                (p->origin_y + (double)p->scale * in->y) * reference_sy + reference_oy, in->s,
                in->t};
        }
        double area = mesh_edge(v[0], v[1], v[2].x, v[2].y);
        if (area == 0)
            continue;
        if (area < 0) {
            mesh_vertex swap = v[1];
            v[1] = v[2];
            v[2] = swap;
            area = -area;
        }
        double min_x = fmax(0, fmax(r->clip.x, fmin(v[0].x, fmin(v[1].x, v[2].x))));
        double min_y = fmax(0, fmax(r->clip.y, fmin(v[0].y, fmin(v[1].y, v[2].y))));
        double max_x =
            fmin(r->width, fmin((double)r->clip.x + r->clip.w, fmax(v[0].x, fmax(v[1].x, v[2].x))));
        double max_y = fmin(
            r->height, fmin((double)r->clip.y + r->clip.h, fmax(v[0].y, fmax(v[1].y, v[2].y))));
        if (min_x >= max_x || min_y >= max_y)
            continue;
        double sx =
            ((v[1].s - v[0].s) * (v[2].y - v[0].y) - (v[2].s - v[0].s) * (v[1].y - v[0].y)) / area;
        double sy =
            ((v[1].x - v[0].x) * (v[2].s - v[0].s) - (v[2].x - v[0].x) * (v[1].s - v[0].s)) / area;
        double tx =
            ((v[1].t - v[0].t) * (v[2].y - v[0].y) - (v[2].t - v[0].t) * (v[1].y - v[0].y)) / area;
        double ty =
            ((v[1].x - v[0].x) * (v[2].t - v[0].t) - (v[2].x - v[0].x) * (v[1].t - v[0].t)) / area;
        bool solid = hypot(sx, sy) < .001;
        for (int y = (int)floor(min_y); y < (int)ceil(max_y); y++)
            for (int x = (int)floor(min_x); x < (int)ceil(max_x); x++) {
                double e[3] = {mesh_edge(v[1], v[2], x + .5, y + .5),
                               mesh_edge(v[2], v[0], x + .5, y + .5),
                               mesh_edge(v[0], v[1], x + .5, y + .5)};
                bool inside = true;
                for (size_t j = 0; j < 3; j++)
                    if (e[j] < 0 || (e[j] == 0 && !mesh_top_left(v[(j + 1) % 3], v[(j + 2) % 3])))
                        inside = false;
                if (!inside)
                    continue;
                double coverage = 1;
                if (!solid) {
                    double ss =
                        (e[0] / area) * v[0].s + (e[1] / area) * v[1].s + (e[2] / area) * v[2].s;
                    double tt =
                        (e[0] / area) * v[0].t + (e[1] / area) * v[1].t + (e[2] / area) * v[2].t;
                    double f = ss * ss - tt, gradient = hypot(2 * ss * sx - tx, 2 * ss * sy - ty);
                    /* Deterministic extension at an undefined zero-gradient distance. */
                    coverage = gradient > 0 ? fmax(0, fmin(1, .5 - f / gradient))
                                            : (f < 0   ? 1
                                               : f > 0 ? 0
                                                       : .5);
                }
                color.a = (unsigned char)lround(p->color.a * coverage * 255);
                blend(r, x, y, color);
            }
    }
}
#endif
