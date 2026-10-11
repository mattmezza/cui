/* SPDX-License-Identifier: MIT */
#include "cui/cui_sokol.h"
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
    float position[2], uv[2], color[4], shape[4], kind;
} vertex;
typedef struct {
    sg_view texture;
    cui_rect clip;
} draw;
typedef struct {
    sg_image image;
    sg_view view;
    unsigned char *coverage;
    int w, h;
    uint64_t hash, used, frame;
} cached_glyph;
struct cui_sokol {
    cui_sokol_config config;
    vertex *vertices;
    draw *draws;
    size_t quads;
    sg_buffer buffer;
    sg_shader shader;
    sg_pipeline pipeline;
    sg_sampler sampler;
    sg_image white;
    sg_view white_view;
    cached_glyph glyphs[CUI_SOKOL_GLYPHS];
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
    "layout(location=4) in float kind;\n"
    "uniform vec2 canvas; out vec2 v_uv; out vec4 v_color; out vec4 v_shape; "
    "out float v_kind;\n"
    "void main(){ "
    "gl_Position=vec4(position.x/canvas.x*2.0-1.0,1.0-position.y/"
    "canvas.y*2.0,0.0,1.0);"
    "v_uv=uv;v_color=color;v_shape=shape;v_kind=kind;}\n";
static const char *fragment_source =
    "#version 330\n"
    "in vec2 v_uv; in vec4 v_color; in vec4 v_shape; in float v_kind; uniform "
    "sampler2D glyph; out vec4 frag_color;\n"
    "void main(){float coverage; "
    "if(v_kind>1.5){coverage=texture(glyph,v_uv).a;}else{"
    "vec2 p=v_uv*v_shape.xy;float radius=v_shape.z;vec2 "
    "q=abs(p-v_shape.xy*0.5)-v_shape.xy*0.5+radius;"
    "float d=length(max(q,vec2(0.0)))+min(max(q.x,q.y),0.0)-radius;"
    "float "
    "aa=max(fwidth(d),0.001);coverage=radius<0.001?1.0:clamp(0.5-d/aa,0.0,1.0);"
    "if(v_kind>0.5){if(radius<0.001){float "
    "edge=min(min(p.x,p.y),min(v_shape.x-p.x,v_shape.y-p.y));coverage-=step(v_"
    "shape.w,edge);}"
    "else coverage-=clamp(0.5-(d+v_shape.w)/aa,0.0,1.0);}}"
    "frag_color=vec4(v_color.rgb,v_color.a*coverage);}\n";
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
                  {.glsl_name = "kind"}},
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
                             {.format = SG_VERTEXFORMAT_FLOAT}}},
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0] = {.pixel_format = SG_PIXELFORMAT_RGBA8,
                      .blend = {.enabled = true,
                                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA}},
        .sample_count = 1,
        .label = "CUI alpha pipeline"});
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
    if (sg_isvalid()) {
        if (r->listening)
            sg_remove_commit_listener((sg_commit_listener){committed, r});
        for (size_t i = 0; i < CUI_SOKOL_GLYPHS; i++)
            evict(r, &r->glyphs[i]);
        sg_destroy_view(r->white_view);
        sg_destroy_image(r->white);
        sg_destroy_sampler(r->sampler);
        sg_destroy_buffer(r->buffer);
        sg_destroy_pipeline(r->pipeline);
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
            .kind = kind};
    }
    r->draws[r->quads++] = (draw){texture, r->clip};
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
#endif
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
    cui_rect stack[64];
    size_t depth = 0;
    r->clip = (cui_rect){0, 0, list->width, list->height};
    for (size_t i = 0; i < list->count && r->ok; i++) {
        const cui_draw_command *c = &list->commands[i];
        if (!isfinite(c->bounds.x) || !isfinite(c->bounds.y) || !isfinite(c->bounds.w) ||
            !isfinite(c->bounds.h) || !isfinite(c->radius) || !isfinite(c->stroke) ||
            fabsf(c->bounds.x) > 1e6f || fabsf(c->bounds.y) > 1e6f || fabsf(c->bounds.w) > 16384 ||
            fabsf(c->bounds.h) > 16384)
            return fail(r, "Invalid CUI Sokol geometry");
        switch (c->type) {
        case CUI_DRAW_RECT:
            quad(r, c->bounds, c->color, c->radius, 0, 0, r->white_view);
            break;
        case CUI_DRAW_BORDER:
            if (c->stroke > 0)
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
            if (!r->config.font || !cui_font_draw(r->config.font, c->text, c->font_size,
                                                  c->bounds.x, c->bounds.y, glyph, r))
                return fail(r, "CUI Sokol font shaping failed");
#else
            return fail(r, "CUI Sokol text support is disabled");
#endif
            break;
        default:
            return fail(r, "Unknown CUI Sokol draw command");
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
        sg_apply_pipeline(r->pipeline);
        float canvas[2] = {list->width, list->height};
        sg_apply_uniforms(0, &(sg_range){canvas, sizeof(canvas)});
        float sx = t.width / list->width, sy = t.height / list->height;
        for (size_t i = 0; i < r->quads; i++) {
            cui_rect c = r->draws[i].clip;
            int x = (int)floorf(fmaxf(0, c.x) * sx), y = (int)floorf(fmaxf(0, c.y) * sy),
                right = (int)ceilf(fminf(list->width, c.x + c.w) * sx),
                bottom = (int)ceilf(fminf(list->height, c.y + c.h) * sy);
            if (right <= x || bottom <= y)
                continue;
            sg_apply_scissor_rect(x, y, right - x, bottom - y, true);
            sg_apply_bindings(&(sg_bindings){.vertex_buffers[0] = r->buffer,
                                             .views[0] = r->draws[i].texture,
                                             .samplers[0] = r->sampler});
            sg_draw((int)i * 6, 6, 1);
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
    unsigned capabilities = CUI_RENDER_ACCELERATED | CUI_RENDER_ROUNDED | CUI_RENDER_CLIP;
#ifdef CUI_HAS_TEXT
    if (r && r->config.font)
        capabilities |= CUI_RENDER_TEXT;
#endif
    return (cui_renderer){r, "Sokol GLCORE", capabilities, render, destroy};
}
