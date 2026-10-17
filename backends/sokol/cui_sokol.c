/* SPDX-License-Identifier: MIT */
#include "cui/cui_sokol.h"
#include "../../src/core/cui_text_paint_internal.h"
#include "../software/cui_bitmap.h"
#include "../software/cui_shadow.h"
#define SOKOL_GLCORE
#define SOKOL_VALIDATE_NON_FATAL
#ifndef CUI_SOKOL_EXTERNAL_RUNTIME
#define SOKOL_IMPL
#endif
#include "sokol_gfx.h"
#ifdef CUI_SOKOL_EXTERNAL_RUNTIME
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#endif
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CUI_SOKOL_GLYPHS 256
#define CUI_SOKOL_GLYPH_BYTES (4u * 1024u * 1024u)
typedef struct {
    float position[2], uv[2], color[4], shape[4], kind, radii[4], edges[4];
} vertex;
typedef struct {
    sg_view texture;
    cui_rect clip;
    bool mesh;
    unsigned count;
} draw;
typedef struct {
    sg_image image;
    sg_view view;
    unsigned char *coverage;
    int w, h;
    uint64_t hash, used, frame;
} cached_glyph;
#define CUI_SOKOL_BITMAPS 64
#define CUI_SOKOL_BITMAP_BYTES (64u * 1024u * 1024u)
typedef struct {
    sg_image image;
    sg_view view;
    cui_bitmap_key key;
    bool shadow;
    uint64_t used, frame;
} cached_bitmap;
struct cui_sokol {
    cui_shadow_cache shadows;
    cui_sokol_config config;
    vertex *vertices;
    draw *draws;
    size_t quads;
    sg_buffer buffer;
    sg_shader shader;
    sg_pipeline pipeline, mesh_pipeline;
    sg_sampler sampler;
    sg_image white;
    sg_view white_view;
    cached_glyph glyphs[CUI_SOKOL_GLYPHS];
    cached_bitmap bitmaps[CUI_SOKOL_BITMAPS];
    size_t bitmap_bytes, shadow_bytes;
    size_t glyph_bytes;
    uint64_t clock, frame, hits, misses;
    bool owns_runtime, ok, rendered, listening;
    cui_rect clip;
    cui_color ink;
    char error[256];
};
static bool fail(cui_sokol *r, const char *message) {
    if (r) {
        snprintf(r->error, sizeof(r->error), "%s", message);
        r->ok = false;
    }
    return false;
}
static void log_message(const char *tag, uint32_t level, uint32_t item, const char *message,
                        uint32_t line, const char *file, void *user) {
    (void)tag;
    (void)item;
    (void)line;
    (void)file;
    if (level <= 1)
        fail(user, message ? message : "Sokol resource or validation error");
}
static const char *vertex_source =
    "#version 330\n"
    "layout(location=0) in vec2 position; layout(location=1) in vec2 uv;\n"
    "layout(location=2) in vec4 color; layout(location=3) in vec4 shape; "
    "layout(location=4) in float kind; layout(location=5) in vec4 radii; layout(location=6) in "
    "vec4 edges;\n"
    "uniform vec2 canvas; out vec2 v_uv; out vec4 v_color; out vec4 v_shape; "
    "out float v_kind; out vec4 v_radii; out vec4 v_edges;\n"
    "void main(){ "
    "gl_Position=vec4(position.x/canvas.x*2.0-1.0,1.0-position.y/"
    "canvas.y*2.0,0.0,1.0);"
    "v_uv=uv;v_color=color;v_shape=shape;v_kind=kind;v_radii=radii;v_edges=edges;}\n";
static const char *fragment_source =
    "#version 330\n"
    "in vec2 v_uv; in vec4 v_color; in vec4 v_shape; in float v_kind; in vec4 v_radii; in vec4 "
    "v_edges; uniform sampler2D glyph; out vec4 frag_color;\n"
    "float arcDistance(vec2 p,vec2 r) {\n"
    "    if(r.x<=0.0||r.y<=0.0||p.x>=r.x||p.y>=r.y) return -min(p.x,p.y);\n"
    "    return (length((p-r)/r)-1.0)*min(r.x,r.y);\n"
    "}\n"
    "float outline(vec2 p,vec2 size,vec4 rx,vec4 ry) {\n"
    "    float d=-1e6;\n"
    "    d=max(d,arcDistance(p,vec2(rx.x,ry.x)));\n"
    "    d=max(d,arcDistance(vec2(size.x-p.x,p.y),vec2(rx.y,ry.y)));\n"
    "    d=max(d,arcDistance(vec2(p.x,size.y-p.y),vec2(rx.z,ry.z)));\n"
    "    return max(d,arcDistance(size-p,vec2(rx.w,ry.w)));\n"
    "}\n"
    "float shapeCoverage(vec2 p,vec2 size,vec4 radii,vec4 edges,float border) {\n"
    "    float d=outline(p,size,radii,radii);\n"
    "    float aa=max(fwidth(d),0.5);\n"
    "    float alpha=1.0-smoothstep(-aa*0.5,aa*0.5,d); bool "
    "curved=(p.x<radii.x&&p.y<radii.x)||(size.x-p.x<radii.y&&p.y<radii.y)||(p.x<radii.z&&size.y-p."
    "y<radii.z)||(size.x-p.x<radii.w&&size.y-p.y<radii.w); if(!curved) alpha=1.0;\n"
    "    if(border>0.5) { if(dot(edges,vec4(1))<=0.0) return 0.0;\n"
    "        vec2 inner=size-vec2(edges.x+edges.y,edges.z+edges.w);\n"
    "        if(inner.x>0.0&&inner.y>0.0) {\n"
    "            vec4 rx=max(radii-vec4(edges.x,edges.y,edges.x,edges.y),vec4(0));\n"
    "            vec4 ry=max(radii-vec4(edges.z,edges.z,edges.w,edges.w),vec4(0));\n"
    "            vec2 ip=p-vec2(edges.x,edges.z); float "
    "di=max(outline(ip,inner,rx,ry),max(max(-ip.x,-ip.y),max(ip.x-inner.x,ip.y-inner.y)));\n"
    "            float ai=max(fwidth(di),0.5);\n"
    "            alpha=max(0.0,alpha-(1.0-smoothstep(-ai*0.5,ai*0.5,di)));\n"
    "        }\n"
    "    }\n"
    "    return alpha;\n"
    "}\n"
    "vec3 encodeSRGB(vec3 v){return "
    "mix(12.92*v,1.055*pow(v,vec3(1.0/2.4))-0.055,greaterThan(v,vec3(0.0031308)));}\n"
    "void main(){if(v_kind>3.5){vec2 ds=vec2(dFdx(v_uv.x),dFdy(v_uv.x));"
    "vec2 dt=vec2(dFdx(v_uv.y),dFdy(v_uv.y));float a=1.0;"
    "if(length(ds)>=0.001){float f=v_uv.x*v_uv.x-v_uv.y;"
    "a=clamp(0.5-f/length(2.0*v_uv.x*ds-dt),0.0,1.0);}"
    "vec3 rgb=v_color.a>0.0?encodeSRGB(clamp(v_color.rgb/v_color.a,0.0,1.0)):vec3(0);"
    "a*=v_color.a;frag_color=vec4(rgb*a,a);return;}if(v_kind>2.5){frag_color=texture(glyph,v_uv)*v_"
    "color;return;}float "
    "coverage=v_kind>1.5?texture(glyph,v_uv).a:shapeCoverage(v_uv*v_shape.xy,v_shape.xy,v_radii,v_"
    "edges,v_kind);frag_color=vec4(v_color.rgb,v_color.a*coverage);}\n";
static bool valid_target(cui_sokol_target t) {
    return t.width > 0 && t.height > 0 && t.width <= 16384 && t.height <= 16384;
}
static void committed(void *user) {
    ((cui_sokol *)user)->rendered = false;
}
cui_sokol *cui_sokol_create(cui_sokol_config config) {
    if (!valid_target(config.target) || !glGetString(GL_VERSION))
        return NULL;
    int major = 0, minor = 0;
    if (sscanf((const char *)glGetString(GL_VERSION), "%d.%d", &major, &minor) != 2 || major < 3 ||
        (major == 3 && minor < 3))
        return NULL;
    if (!config.max_quads)
        config.max_quads = 8192;
    if (config.max_quads > 100000 || config.max_quads > INT_MAX / 6)
        return NULL;
    if ((config.borrow_runtime && (!sg_isvalid() || sg_query_backend() != SG_BACKEND_GLCORE)) ||
        (!config.borrow_runtime && sg_isvalid()))
        return NULL;
    cui_sokol *r = calloc(1, sizeof(*r));
    if (!r)
        return NULL;
    r->config = config;
    r->ok = true;
    r->vertices = calloc(config.max_quads * 6, sizeof(vertex));
    r->draws = calloc(config.max_quads, sizeof(draw));
    if (!r->vertices || !r->draws) {
        cui_sokol_destroy(r);
        return NULL;
    }
    if (!config.borrow_runtime) {
        sg_setup(&(sg_desc){.image_pool_size = 512,
                            .view_pool_size = 512,
                            .logger = {log_message, r},
                            .environment.defaults = {.color_format = SG_PIXELFORMAT_RGBA8,
                                                     .depth_format = SG_PIXELFORMAT_NONE,
                                                     .sample_count = 1}});
        r->owns_runtime = true;
    }
    if (!sg_isvalid()) {
        cui_sokol_destroy(r);
        return NULL;
    }
    sg_shader_desc shader = {
        .vertex_func.source = vertex_source,
        .fragment_func.source = fragment_source,
        .attrs = {{.glsl_name = "position"},
                  {.glsl_name = "uv"},
                  {.glsl_name = "color"},
                  {.glsl_name = "shape"},
                  {.glsl_name = "kind"},
                  {.glsl_name = "radii"},
                  {.glsl_name = "edges"}},
        .uniform_blocks[0] = {.stage = SG_SHADERSTAGE_VERTEX,
                              .size = 2 * sizeof(float),
                              .layout = SG_UNIFORMLAYOUT_NATIVE,
                              .glsl_uniforms[0] = {.type = SG_UNIFORMTYPE_FLOAT2,
                                                   .glsl_name = "canvas"}},
        .views[0].texture = {.stage = SG_SHADERSTAGE_FRAGMENT,
                             .image_type = SG_IMAGETYPE_2D,
                             .sample_type = SG_IMAGESAMPLETYPE_FLOAT},
        .samplers[0] = {.stage = SG_SHADERSTAGE_FRAGMENT, .sampler_type = SG_SAMPLERTYPE_FILTERING},
        .texture_sampler_pairs[0] = {.stage = SG_SHADERSTAGE_FRAGMENT,
                                     .view_slot = 0,
                                     .sampler_slot = 0,
                                     .glsl_name = "glyph"},
        .label = "CUI solid/rounded/glyph shader"};
    r->shader = sg_make_shader(&shader);
    if (sg_query_shader_state(r->shader) != SG_RESOURCESTATE_VALID) {
        cui_sokol_destroy(r);
        return NULL;
    }
    r->pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = r->shader,
        .layout = {.buffers[0].stride = sizeof(vertex),
                   .attrs = {{.format = SG_VERTEXFORMAT_FLOAT2},
                             {.format = SG_VERTEXFORMAT_FLOAT2},
                             {.format = SG_VERTEXFORMAT_FLOAT4},
                             {.format = SG_VERTEXFORMAT_FLOAT4},
                             {.format = SG_VERTEXFORMAT_FLOAT},
                             {.format = SG_VERTEXFORMAT_FLOAT4},
                             {.format = SG_VERTEXFORMAT_FLOAT4}}},
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0] = {.pixel_format = SG_PIXELFORMAT_RGBA8,
                      .blend = {.enabled = true,
                                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA}},
        .sample_count = 1,
        .label = "CUI alpha pipeline"});
    sg_pipeline_desc mesh_desc = sg_query_pipeline_desc(r->pipeline);
    mesh_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
    mesh_desc.label = "CUI analytic premultiplied mesh pipeline";
    r->mesh_pipeline = sg_make_pipeline(&mesh_desc);
    r->buffer =
        sg_make_buffer(&(sg_buffer_desc){.size = config.max_quads * 6 * sizeof(vertex),
                                         .usage = {.vertex_buffer = true, .write_transient = true},
                                         .label = "CUI bounded frame vertices"});
    r->sampler = sg_make_sampler(&(sg_sampler_desc){.min_filter = SG_FILTER_LINEAR,
                                                    .mag_filter = SG_FILTER_LINEAR,
                                                    .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                                                    .wrap_v = SG_WRAP_CLAMP_TO_EDGE});
    const unsigned char white[] = {255, 255, 255, 255};
    r->white = sg_make_image(&(sg_image_desc){.width = 1,
                                              .height = 1,
                                              .pixel_format = SG_PIXELFORMAT_RGBA8,
                                              .data.mip_levels[0] = {white, sizeof(white)}});
    r->white_view = sg_make_view(&(sg_view_desc){.texture.image = r->white});
    if (!r->ok || sg_query_shader_state(r->shader) != SG_RESOURCESTATE_VALID ||
        sg_query_pipeline_state(r->pipeline) != SG_RESOURCESTATE_VALID ||
        sg_query_pipeline_state(r->mesh_pipeline) != SG_RESOURCESTATE_VALID ||
        sg_query_buffer_state(r->buffer) != SG_RESOURCESTATE_VALID ||
        sg_query_sampler_state(r->sampler) != SG_RESOURCESTATE_VALID ||
        sg_query_image_state(r->white) != SG_RESOURCESTATE_VALID ||
        sg_query_view_state(r->white_view) != SG_RESOURCESTATE_VALID) {
        cui_sokol_destroy(r);
        return NULL;
    }
    if (!sg_add_commit_listener((sg_commit_listener){committed, r})) {
        cui_sokol_destroy(r);
        return NULL;
    }
    r->listening = true;
    return r;
}
static void evict(cui_sokol *r, cached_glyph *g) {
    if (g->coverage)
        r->glyph_bytes -= (size_t)g->w * g->h;
    sg_destroy_view(g->view);
    sg_destroy_image(g->image);
    free(g->coverage);
    memset(g, 0, sizeof(*g));
}
void cui_sokol_destroy(cui_sokol *r) {
    if (!r)
        return;
    cui_shadow_destroy(&r->shadows);
    if (sg_isvalid()) {
        if (r->listening)
            sg_remove_commit_listener((sg_commit_listener){committed, r});
        for (size_t i = 0; i < CUI_SOKOL_GLYPHS; i++)
            evict(r, &r->glyphs[i]);
        for (size_t i = 0; i < CUI_SOKOL_BITMAPS; i++) {
            sg_destroy_view(r->bitmaps[i].view);
            sg_destroy_image(r->bitmaps[i].image);
        }
        sg_destroy_view(r->white_view);
        sg_destroy_image(r->white);
        sg_destroy_sampler(r->sampler);
        sg_destroy_buffer(r->buffer);
        sg_destroy_pipeline(r->pipeline);
        sg_destroy_pipeline(r->mesh_pipeline);
        sg_destroy_shader(r->shader);
        if (r->owns_runtime)
            sg_shutdown();
    } else
        for (size_t i = 0; i < CUI_SOKOL_GLYPHS; i++)
            free(r->glyphs[i].coverage);
    free(r->vertices);
    free(r->draws);
    free(r);
}
bool cui_sokol_set_target(cui_sokol *r, cui_sokol_target t) {
    if (!r || !valid_target(t))
        return false;
    r->config.target = t;
    return true;
}
static cui_rect intersection(cui_rect a, cui_rect b) {
    float x = fmaxf(a.x, b.x), y = fmaxf(a.y, b.y), right = fminf(a.x + a.w, b.x + b.w),
          bottom = fminf(a.y + a.h, b.y + b.h);
    return (cui_rect){x, y, fmaxf(0, right - x), fmaxf(0, bottom - y)};
}
static bool quad(cui_sokol *r, cui_rect bounds, cui_color color, float radius, float stroke,
                 float kind, sg_view texture) {
    if (bounds.w <= 0 || bounds.h <= 0 || r->clip.w <= 0 || r->clip.h <= 0)
        return true;
    if (r->quads == r->config.max_quads)
        return fail(r, "CUI Sokol quad capacity exceeded");
    float corners[6][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}};
    for (unsigned i = 0; i < 6; i++) {
        float u = corners[i][0], v = corners[i][1];
        r->vertices[r->quads * 6 + i] = (vertex){
            .position = {bounds.x + u * bounds.w, bounds.y + v * bounds.h},
            .uv = {u, v},
            .color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
            .shape = {bounds.w, bounds.h, fminf(fmaxf(radius, 0), fminf(bounds.w, bounds.h) / 2),
                      fmaxf(stroke, 0)},
            .kind = kind,
            .radii = {radius, radius, radius, radius},
            .edges = {stroke, stroke, stroke, stroke}};
    }
    r->draws[r->quads++] = (draw){.texture = texture, .clip = r->clip, .count = 6};
    return true;
}
static bool mesh(cui_sokol *r, cui_mesh_paint paint) {
    if (!cui_mesh_valid(paint))
        return fail(r, "Invalid CUI Sokol mesh");
    size_t chunks = paint.vertex_count / 6 + (paint.vertex_count % 6 != 0);
    if (chunks > r->config.max_quads - r->quads)
        return fail(r, "CUI Sokol mesh capacity exceeded");
    for (size_t start = 0; start < paint.vertex_count; start += 6) {
        unsigned count = paint.vertex_count - start >= 6 ? 6 : 3;
        size_t slot = r->quads++;
        for (unsigned j = 0; j < count; j++) {
            cui_mesh_vertex v = paint.vertices[start + j];
            r->vertices[slot * 6 + j] =
                (vertex){.position = {paint.origin_x + v.x * paint.scale,
                                      paint.origin_y + v.y * paint.scale},
                         .uv = {v.s, v.t},
                         .color = {paint.color.r, paint.color.g, paint.color.b, paint.color.a},
                         .kind = 4};
        }
        r->draws[slot] =
            (draw){.texture = r->white_view, .clip = r->clip, .mesh = true, .count = count};
    }
    return true;
}
#ifdef CUI_HAS_TEXT
static cached_glyph *oldest(cui_sokol *r, bool populated) {
    cached_glyph *best = NULL;
    for (size_t i = 0; i < CUI_SOKOL_GLYPHS; i++) {
        cached_glyph *g = &r->glyphs[i];
        if (!g->coverage) {
            if (!populated)
                return g;
            continue;
        }
        if (g->frame != r->frame && (!best || g->used < best->used))
            best = g;
    }
    return best;
}
static sg_view glyph_texture(cui_sokol *r, int w, int h, int pitch, const unsigned char *data) {
    sg_view none = {0};
    if (w <= 0 || h <= 0 || (size_t)w > CUI_SOKOL_GLYPH_BYTES / (size_t)h) {
        fail(r, "CUI Sokol glyph exceeds cache budget");
        return none;
    }
    size_t bytes = (size_t)w * h;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            hash ^= data[j * pitch + i];
            hash *= UINT64_C(1099511628211);
        }
    for (size_t i = 0; i < CUI_SOKOL_GLYPHS; i++) {
        cached_glyph *g = &r->glyphs[i];
        if (g->coverage && g->w == w && g->h == h && g->hash == hash) {
            bool same = true;
            for (int j = 0; j < h && same; j++)
                same = !memcmp(g->coverage + (size_t)j * w, data + j * pitch, (size_t)w);
            if (same) {
                g->used = ++r->clock;
                g->frame = r->frame;
                r->hits++;
                return g->view;
            }
        }
    }
    r->misses++;
    while (r->glyph_bytes + bytes > CUI_SOKOL_GLYPH_BYTES) {
        cached_glyph *g = oldest(r, true);
        if (!g) {
            fail(r, "CUI Sokol glyphs exceed a single-frame cache budget");
            return none;
        }
        evict(r, g);
    }
    cached_glyph *g = oldest(r, false);
    if (!g) {
        fail(r, "CUI Sokol single frame exceeds 256 distinct cached glyphs");
        return none;
    }
    if (g->coverage)
        evict(r, g);
    unsigned char *rgba = malloc(bytes * 4), *copy = malloc(bytes);
    if (!rgba || !copy) {
        free(rgba);
        free(copy);
        fail(r, "CUI Sokol glyph allocation failed");
        return none;
    }
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            size_t at = (size_t)j * w + i;
            copy[at] = data[j * pitch + i];
            rgba[at * 4] = 255;
            rgba[at * 4 + 1] = 255;
            rgba[at * 4 + 2] = 255;
            rgba[at * 4 + 3] = copy[at];
        }
    sg_image image = sg_make_image(&(sg_image_desc){.width = w,
                                                    .height = h,
                                                    .pixel_format = SG_PIXELFORMAT_RGBA8,
                                                    .data.mip_levels[0] = {rgba, bytes * 4},
                                                    .label = "CUI immutable glyph coverage"});
    free(rgba);
    if (sg_query_image_state(image) != SG_RESOURCESTATE_VALID) {
        sg_destroy_image(image);
        free(copy);
        fail(r, "CUI Sokol glyph GPU image creation failed");
        return none;
    }
    sg_view view = sg_make_view(&(sg_view_desc){.texture.image = image});
    if (sg_query_image_state(image) != SG_RESOURCESTATE_VALID ||
        sg_query_view_state(view) != SG_RESOURCESTATE_VALID) {
        sg_destroy_view(view);
        sg_destroy_image(image);
        free(copy);
        fail(r, "CUI Sokol glyph GPU resource creation failed");
        return none;
    }
    *g = (cached_glyph){image, view, copy, w, h, hash, ++r->clock, r->frame};
    r->glyph_bytes += bytes;
    return view;
}
static void glyph(void *user, int x, int y, int w, int h, int pitch,
                  const unsigned char *coverage) {
    cui_sokol *r = user;
    if (!r->ok || !w || !h || !coverage)
        return;
    sg_view texture = glyph_texture(r, w, h, pitch, coverage);
    if (texture.id)
        quad(r, (cui_rect){(float)x, (float)y, (float)w, (float)h}, r->ink, 0, 0, 2, texture);
}
static void color_glyph(void *user, int x, int y, int w, int h, int pitch,
                        const unsigned char *data, cui_color color) {
    cui_sokol *r = user;
    r->ink = color;
    glyph(user, x, y, w, h, pitch, data);
}
static bool text_draw(cui_sokol *r, const cui_draw_command *c) {
    if (c->text_color_count)
        return cui_font_draw_context_line_colored(
            r->config.font, c->text_source, c->font_size, c->text_style, c->bounds.x, c->bounds.y,
            c->color, c->text_colors, c->text_color_count, color_glyph, r);
    return cui_font_draw_context_line(r->config.font, c->text, c->text_source, c->font_size,
                                      c->text_style, c->bounds.x, c->bounds.y, glyph, r);
}

#endif
static sg_view bitmap_view(cui_sokol *r, const cui_draw_command *c) {
    cui_bitmap_key key;
    if (!cui_bitmap_key_make(c, &key)) {
        fail(r, "Invalid CUI Sokol bitmap");
        return (sg_view){0};
    }
    size_t bytes = (size_t)key.width * key.height * 4;
    if (bytes > CUI_SOKOL_BITMAP_BYTES) {
        fail(r, "CUI Sokol bitmap exceeds cache budget");
        return (sg_view){0};
    }
    bool shadow = cui_shadow_type(c->type);
    cached_bitmap *entry = NULL;
    for (size_t i = 0; i < CUI_SOKOL_BITMAPS; i++)
        if (r->bitmaps[i].image.id && cui_bitmap_key_equal(&key, &r->bitmaps[i].key)) {
            entry = &r->bitmaps[i];
            break;
        }
    if (!entry) {
        for (;;) {
            cached_bitmap *oldest = NULL, *empty = NULL;
            for (size_t i = 0; i < CUI_SOKOL_BITMAPS; i++) {
                cached_bitmap *b = &r->bitmaps[i];
                if (!b->image.id)
                    empty = b;
                else if (b->frame != r->frame && (!oldest || b->used < oldest->used))
                    oldest = b;
            }
            if (empty && r->bitmap_bytes + bytes <= CUI_SOKOL_BITMAP_BYTES &&
                (!shadow || r->shadow_bytes + bytes <= CUI_SHADOW_CACHE_BYTES)) {
                entry = empty;
                break;
            }
            if (!oldest) {
                fail(r, "CUI Sokol frame exceeds bitmap cache capacity");
                return (sg_view){0};
            }
            if (oldest->shadow)
                r->shadow_bytes -= (size_t)oldest->key.width * oldest->key.height * 4;
            r->bitmap_bytes -= (size_t)oldest->key.width * oldest->key.height * 4;
            sg_destroy_view(oldest->view);
            sg_destroy_image(oldest->image);
            *oldest = (cached_bitmap){0};
        }
        unsigned char *rgba = cui_bitmap_raster(&key, CUI_SOKOL_BITMAP_BYTES);
        if (!rgba) {
            fail(r, "CUI Sokol bitmap allocation failed");
            return (sg_view){0};
        }
        sg_image image = sg_make_image(&(sg_image_desc){.width = (int)key.width,
                                                        .height = (int)key.height,
                                                        .pixel_format = SG_PIXELFORMAT_RGBA8,
                                                        .data.mip_levels[0] = {rgba, bytes},
                                                        .label = "CUI RGBA bitmap"});
        free(rgba);
        sg_view view = sg_make_view(&(sg_view_desc){.texture.image = image});
        if (sg_query_image_state(image) != SG_RESOURCESTATE_VALID ||
            sg_query_view_state(view) != SG_RESOURCESTATE_VALID) {
            sg_destroy_view(view);
            sg_destroy_image(image);
            fail(r, "CUI Sokol bitmap upload failed");
            return (sg_view){0};
        }
        *entry = (cached_bitmap){.image = image, .view = view, .key = key, .shadow = shadow};
        r->bitmap_bytes += bytes;
        if (shadow)
            r->shadow_bytes += bytes;
    }
    entry->used = ++r->clock;
    entry->frame = r->frame;
    return entry->view;
}
bool cui_sokol_render(cui_sokol *r, const cui_draw_list *list, cui_color clear) {
    if (!r || !list)
        return false;
    r->error[0] = 0;
    r->ok = true;
    r->quads = 0;
    r->frame++;
    if (!sg_isvalid() || (!list->commands && list->count) || !isfinite(list->width) ||
        !isfinite(list->height) || list->width <= 0 || list->height <= 0 || list->width > 16384 ||
        list->height > 16384)
        return fail(r, "Invalid CUI Sokol frame");
    if (sg_query_pass_state() != SG_PASSSTATE_NONE)
        return fail(r, "CUI Sokol render cannot run inside an active host pass");
    if (r->rendered)
        return fail(r, "Host must commit before rendering this CUI adapter again");
    if (!cui_text_tracking_supported(list, false) || !cui_text_paints_supported(list, false))
        return false;
    cui_draw_list prepared;
    if (!cui_shadow_prepare(&r->shadows, list, r->config.target.width / list->width,
                            r->config.target.height / list->height, 0, 0, &prepared))
        return fail(r, "Invalid or over-budget CUI shadow transform");
    list = &prepared;
    cui_rect stack[64];
    size_t depth = 0;
    r->clip = (cui_rect){0, 0, list->width, list->height};
    for (size_t i = 0; i < list->count && r->ok; i++) {
        const cui_draw_command *c = &list->commands[i];
        if (!isfinite(c->bounds.x) || !isfinite(c->bounds.y) || !isfinite(c->bounds.w) ||
            !isfinite(c->bounds.h) || !cui_shape_valid(c) || !isfinite(c->radius) ||
            !isfinite(c->stroke) || fabsf(c->bounds.x) > 1e6f || fabsf(c->bounds.y) > 1e6f ||
            fabsf(c->bounds.w) > 16384 || fabsf(c->bounds.h) > 16384)
            return fail(r, "Invalid CUI Sokol geometry");
        size_t before = r->quads;
        switch (c->type) {
        case CUI_DRAW_MESH:
            if (!mesh(r, c->mesh))
                return false;
            break;
        case CUI_DRAW_INSET_SHADOW:
        case CUI_DRAW_DROP_SHADOW:
        case CUI_DRAW_IMAGE:
            if (c->bounds.w > 0 && c->bounds.h > 0) {
                sg_view view = bitmap_view(r, c);
                if (!view.id)
                    return false;
                quad(r, c->bounds,
                     (cui_color){255, 255, 255, (uint8_t)lroundf(c->bitmap.opacity * 255)}, 0, 0, 3,
                     view);
            }
            break;
        case CUI_DRAW_RECT:
            quad(r, c->bounds, c->color, c->radius, 0, 0, r->white_view);
            break;
        case CUI_DRAW_BORDER:
            quad(r, c->bounds, c->color, c->radius, c->stroke, 1, r->white_view);
            break;
        case CUI_DRAW_CLIP_PUSH:
            if (depth == 64)
                return fail(r, "CUI Sokol clip stack overflow");
            stack[depth++] = r->clip;
            r->clip = intersection(r->clip, c->bounds);
            break;
        case CUI_DRAW_CLIP_POP:
            if (!depth)
                return fail(r, "CUI Sokol clip stack underflow");
            r->clip = stack[--depth];
            break;
        case CUI_DRAW_TEXT:
#ifdef CUI_HAS_TEXT
            r->ink = c->color;
            if (!r->config.font || !text_draw(r, c))
                return fail(r, "CUI Sokol font shaping failed");
#else
            return fail(r, "CUI Sokol text support is disabled");
#endif
            break;
        default:
            return fail(r, "Unknown CUI Sokol draw command");
        }
        if ((c->type == CUI_DRAW_RECT || c->type == CUI_DRAW_BORDER) && r->quads > before) {
            float radii[4], edges[4];
            cui_shape_values(c, radii, edges);
            for (unsigned v = 0; v < 6; v++) {
                memcpy(r->vertices[before * 6 + v].radii, radii, sizeof(radii));
                memcpy(r->vertices[before * 6 + v].edges, edges, sizeof(edges));
            }
        }
    }
    if (depth)
        return fail(r, "Unbalanced CUI Sokol clipping");
    if (!r->ok)
        return false;
    sg_reset_state_cache();
    if (r->quads)
        sg_write_buffer_transient(&(sg_write_buffer_desc){
            .src.data = {r->vertices, r->quads * 6 * sizeof(vertex)}, .dst.buffer = r->buffer});
    cui_sokol_target t = r->config.target;
    sg_begin_pass(
        &(sg_pass){.action.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                                        .store_action = SG_STOREACTION_STORE,
                                        .clear_value = {clear.r / 255.0f, clear.g / 255.0f,
                                                        clear.b / 255.0f, clear.a / 255.0f}},
                   .swapchain = {.width = (int)t.width,
                                 .height = (int)t.height,
                                 .sample_count = 1,
                                 .color_format = SG_PIXELFORMAT_RGBA8,
                                 .depth_format = SG_PIXELFORMAT_NONE,
                                 .gl.framebuffer = t.framebuffer}});
    if (r->quads) {
        float canvas[2] = {list->width, list->height};
        float sx = t.width / list->width, sy = t.height / list->height;
        for (size_t i = 0; i < r->quads; i++) {
            cui_rect c = r->draws[i].clip;
            int x = (int)floorf(fmaxf(0, c.x) * sx), y = (int)floorf(fmaxf(0, c.y) * sy),
                right = (int)ceilf(fminf(list->width, c.x + c.w) * sx),
                bottom = (int)ceilf(fminf(list->height, c.y + c.h) * sy);
            if (right <= x || bottom <= y)
                continue;
            sg_apply_pipeline(r->draws[i].mesh ? r->mesh_pipeline : r->pipeline);
            sg_apply_uniforms(0, &(sg_range){canvas, sizeof(canvas)});
            sg_apply_scissor_rect(x, y, right - x, bottom - y, true);
            sg_apply_bindings(&(sg_bindings){.vertex_buffers[0] = r->buffer,
                                             .views[0] = r->draws[i].texture,
                                             .samplers[0] = r->sampler});
            sg_draw((int)i * 6, (int)r->draws[i].count, 1);
        }
    }
    sg_end_pass();
    r->rendered = true;
    if (r->owns_runtime)
        sg_commit();
    if (glGetError() != GL_NO_ERROR)
        return fail(r, "OpenGL reported an error during CUI Sokol rendering");
    return r->ok;
}
cui_sokol_stats cui_sokol_get_stats(const cui_sokol *r) {
    cui_sokol_stats s = {0};
    if (!r)
        return s;
    s.coverage_bytes = r->glyph_bytes;
    s.hits = r->hits;
    s.misses = r->misses;
    s.quads = r->quads;
    for (size_t i = 0; i < CUI_SOKOL_GLYPHS; i++)
        s.entries += r->glyphs[i].coverage != NULL;
    return s;
}
const char *cui_sokol_error(const cui_sokol *r) {
    return r ? r->error : "Invalid CUI Sokol renderer";
}
static bool render(void *r, const cui_draw_list *list, cui_color clear) {
    return cui_sokol_render(r, list, clear);
}
static void destroy(void *r) {
    cui_sokol_destroy(r);
}
cui_renderer cui_sokol_interface(cui_sokol *r) {
    unsigned capabilities = CUI_RENDER_ACCELERATED | CUI_RENDER_ROUNDED | CUI_RENDER_CLIP |
                            CUI_RENDER_IMAGE | CUI_RENDER_MESH | CUI_RENDER_DROP_SHADOW |
                            CUI_RENDER_INSET_SHADOW;
#ifdef CUI_HAS_TEXT
    if (r && r->config.font)
        capabilities |= CUI_RENDER_TEXT;
#endif
    return (cui_renderer){r, "Sokol GLCORE", capabilities, render, destroy};
}
