/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl_gpu.h"
#include "../software/cui_bitmap.h"
#include "../software/cui_shadow.h"
#include "shaders/generated.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ATLAS 1024
#define GLYPHS 4096
#define DRAWS 32768
#define CLIPS 256
#define MESH_VERTICES (DRAWS * 6u)
typedef struct {
    float bounds[4], viewport[4], color[4], shape[4], atlas[4], radii[4], edges[4];
} gpu_params;
typedef struct {
    gpu_params params;
    SDL_Rect clip;
    bool shadow;
    SDL_GPUTexture *texture;
    Uint32 mesh_first, mesh_count;
} gpu_draw;
typedef struct {
    uint64_t hash;
    int x, y, w, h;
} gpu_glyph;
#define BITMAPS 64
#define BITMAP_BUDGET (64u * 1024u * 1024u)
typedef struct {
    SDL_GPUTexture *texture;
    cui_bitmap_key key;
    bool shadow;
    uint64_t used, frame;
} gpu_bitmap;
struct cui_sdl_gpu {
    cui_shadow_cache shadows;
    SDL_GPUDevice *device;
    SDL_Window *window;
    cui_font *font;
    SDL_GPUGraphicsPipeline *pipeline, *mesh_pipeline;
    SDL_GPUBuffer *mesh_buffer;
    SDL_GPUTransferBuffer *mesh_upload;
    cui_mesh_vertex *mesh_vertices;
    size_t mesh_count;
    SDL_GPUTexture *atlas, *surface;
    SDL_GPUSampler *sampler;
    SDL_GPUTransferBuffer *upload;
    unsigned char *pixels;
    gpu_glyph glyphs[GLYPHS];
    size_t glyph_count;
    gpu_bitmap bitmaps[BITMAPS];
    size_t bitmap_bytes, shadow_bytes;
    uint64_t frame, clock;
    gpu_draw *draws;
    size_t draw_count;
    int shelf_x, shelf_y, shelf_h;
    Uint32 width, height;
    bool dirty, failed;
    SDL_Rect clip;
    cui_color ink;
    float logical_w, logical_h;
};
static SDL_GPUShader *shader(SDL_GPUDevice *d, bool fragment, bool mesh) {
    SDL_GPUShaderCreateInfo info = {
        .code_size = mesh ? (fragment ? sizeof(cui_gpu_mesh_frag) : sizeof(cui_gpu_mesh_vert))
                          : (fragment ? sizeof(cui_gpu_frag) : sizeof(cui_gpu_vert)),
        .code = mesh ? (fragment ? cui_gpu_mesh_frag : cui_gpu_mesh_vert)
                     : (fragment ? cui_gpu_frag : cui_gpu_vert),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = fragment ? SDL_GPU_SHADERSTAGE_FRAGMENT : SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = fragment && !mesh ? 1 : 0,
        .num_uniform_buffers = 1};
    return SDL_CreateGPUShader(d, &info);
}
cui_sdl_gpu *cui_sdl_gpu_create(void *device, void *window, cui_font *font) {
    if (!device || !window)
        return NULL;
    if (!(SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV)) {
        SDL_SetError("CUI SDL_GPU adapter currently requires SPIR-V (Vulkan)");
        return NULL;
    }
    cui_sdl_gpu *r = calloc(1, sizeof(*r));
    if (!r)
        return NULL;
    r->device = device;
    r->window = window;
    r->font = font;
    r->pixels = calloc(ATLAS, ATLAS);
    r->draws = calloc(DRAWS, sizeof(*r->draws));
    SDL_GPUShader *v = shader(device, false, false), *f = shader(device, true, false);
    SDL_GPUColorTargetDescription color = {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .blend_state = {.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .color_blend_op = SDL_GPU_BLENDOP_ADD,
                        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                        .enable_blend = true}};
    if (v && f)
        r->pipeline = SDL_CreateGPUGraphicsPipeline(
            device,
            &(SDL_GPUGraphicsPipelineCreateInfo){
                .vertex_shader = v,
                .fragment_shader = f,
                .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                .rasterizer_state = {.fill_mode = SDL_GPU_FILLMODE_FILL,
                                     .cull_mode = SDL_GPU_CULLMODE_NONE},
                .multisample_state = {.sample_count = SDL_GPU_SAMPLECOUNT_1},
                .target_info = {.color_target_descriptions = &color, .num_color_targets = 1}});
    if (v)
        SDL_ReleaseGPUShader(device, v);
    if (f)
        SDL_ReleaseGPUShader(device, f);
    v = shader(device, false, true);
    f = shader(device, true, true);
    color.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    SDL_GPUVertexBufferDescription mesh_buffer = {
        .slot = 0, .pitch = sizeof(cui_mesh_vertex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX};
    SDL_GPUVertexAttribute mesh_attrs[2] = {{.location = 0,
                                             .buffer_slot = 0,
                                             .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                                             .offset = 0},
                                            {.location = 1,
                                             .buffer_slot = 0,
                                             .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                                             .offset = 2 * sizeof(float)}};
    if (v && f)
        r->mesh_pipeline = SDL_CreateGPUGraphicsPipeline(
            device,
            &(SDL_GPUGraphicsPipelineCreateInfo){
                .vertex_shader = v,
                .fragment_shader = f,
                .vertex_input_state = {.vertex_buffer_descriptions = &mesh_buffer,
                                       .num_vertex_buffers = 1,
                                       .vertex_attributes = mesh_attrs,
                                       .num_vertex_attributes = 2},
                .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                .rasterizer_state = {.fill_mode = SDL_GPU_FILLMODE_FILL,
                                     .cull_mode = SDL_GPU_CULLMODE_NONE},
                .multisample_state = {.sample_count = SDL_GPU_SAMPLECOUNT_1},
                .target_info = {.color_target_descriptions = &color, .num_color_targets = 1}});
    if (v)
        SDL_ReleaseGPUShader(device, v);
    if (f)
        SDL_ReleaseGPUShader(device, f);
    r->mesh_vertices = calloc(MESH_VERTICES, sizeof(cui_mesh_vertex));
    r->mesh_buffer = SDL_CreateGPUBuffer(
        device, &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                           .size = MESH_VERTICES * sizeof(cui_mesh_vertex)});
    r->mesh_upload = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                           .size = MESH_VERTICES * sizeof(cui_mesh_vertex)});
    r->atlas = SDL_CreateGPUTexture(
        device, &(SDL_GPUTextureCreateInfo){.type = SDL_GPU_TEXTURETYPE_2D,
                                            .format = SDL_GPU_TEXTUREFORMAT_R8_UNORM,
                                            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                            .width = ATLAS,
                                            .height = ATLAS,
                                            .layer_count_or_depth = 1,
                                            .num_levels = 1});
    r->sampler = SDL_CreateGPUSampler(
        device,
        &(SDL_GPUSamplerCreateInfo){.min_filter = SDL_GPU_FILTER_LINEAR,
                                    .mag_filter = SDL_GPU_FILTER_LINEAR,
                                    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
                                    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE});
    r->upload = SDL_CreateGPUTransferBuffer(
        device, &(SDL_GPUTransferBufferCreateInfo){.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                   .size = ATLAS * ATLAS});
    r->shelf_x = 1;
    r->shelf_y = 1;
    r->dirty = true;
    if (!r->mesh_vertices || !r->mesh_buffer || !r->mesh_upload || !r->mesh_pipeline ||
        !r->pixels || !r->draws || !r->pipeline || !r->atlas || !r->sampler || !r->upload) {
        cui_sdl_gpu_destroy(r);
        return NULL;
    }
    return r;
}
void cui_sdl_gpu_destroy(cui_sdl_gpu *r) {
    if (!r)
        return;
    cui_shadow_destroy(&r->shadows);
    if (r->mesh_pipeline)
        SDL_ReleaseGPUGraphicsPipeline(r->device, r->mesh_pipeline);
    if (r->mesh_buffer)
        SDL_ReleaseGPUBuffer(r->device, r->mesh_buffer);
    if (r->mesh_upload)
        SDL_ReleaseGPUTransferBuffer(r->device, r->mesh_upload);
    free(r->mesh_vertices);
    if (r->pipeline)
        SDL_ReleaseGPUGraphicsPipeline(r->device, r->pipeline);
    for (size_t i = 0; i < BITMAPS; i++)
        if (r->bitmaps[i].texture)
            SDL_ReleaseGPUTexture(r->device, r->bitmaps[i].texture);
    if (r->atlas)
        SDL_ReleaseGPUTexture(r->device, r->atlas);
    if (r->surface)
        SDL_ReleaseGPUTexture(r->device, r->surface);
    if (r->sampler)
        SDL_ReleaseGPUSampler(r->device, r->sampler);
    if (r->upload)
        SDL_ReleaseGPUTransferBuffer(r->device, r->upload);
    free(r->pixels);
    free(r->draws);
    free(r);
}
static gpu_draw *append(cui_sdl_gpu *r, cui_rect rect, cui_color color, float radius,
                        float stroke) {
    if (r->draw_count == DRAWS) {
        r->failed = true;
        SDL_SetError("CUI GPU draw capacity exceeded");
        return NULL;
    }
    gpu_draw *d = &r->draws[r->draw_count++];
    *d = (gpu_draw){.clip = r->clip};
    memcpy(d->params.bounds, (float[]){rect.x, rect.y, rect.w, rect.h}, 16);
    memcpy(d->params.viewport, (float[]){r->logical_w, r->logical_h, 0, 0}, 16);
    memcpy(d->params.color,
           (float[]){color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f}, 16);
    memcpy(d->params.shape, (float[]){rect.w, rect.h, radius, stroke}, 16);
    return d;
}
static SDL_GPUTexture *bitmap_texture(cui_sdl_gpu *r, const cui_draw_command *c) {
    cui_bitmap_key key;
    if (!cui_bitmap_key_make(c, &key))
        return NULL;
    size_t bytes = (size_t)key.width * key.height * 4;
    if (bytes > BITMAP_BUDGET)
        return NULL;
    bool shadow = c->type == CUI_DRAW_DROP_SHADOW;
    gpu_bitmap *entry = NULL;
    for (size_t i = 0; i < BITMAPS; i++)
        if (r->bitmaps[i].texture && cui_bitmap_key_equal(&key, &r->bitmaps[i].key)) {
            entry = &r->bitmaps[i];
            break;
        }
    if (!entry) {
        for (;;) {
            gpu_bitmap *oldest = NULL, *empty = NULL;
            for (size_t i = 0; i < BITMAPS; i++) {
                gpu_bitmap *b = &r->bitmaps[i];
                if (!b->texture)
                    empty = b;
                else if (b->frame != r->frame && (!oldest || b->used < oldest->used))
                    oldest = b;
            }
            if (empty && r->bitmap_bytes + bytes <= BITMAP_BUDGET &&
                (!shadow || r->shadow_bytes + bytes <= CUI_SHADOW_CACHE_BYTES)) {
                entry = empty;
                break;
            }
            if (!oldest)
                return NULL;
            if (oldest->shadow)
                r->shadow_bytes -= (size_t)oldest->key.width * oldest->key.height * 4;
            r->bitmap_bytes -= (size_t)oldest->key.width * oldest->key.height * 4;
            SDL_ReleaseGPUTexture(r->device, oldest->texture);
            *oldest = (gpu_bitmap){0};
        }
        unsigned char *rgba = cui_bitmap_raster(&key, BITMAP_BUDGET);
        if (!rgba)
            return NULL;
        SDL_GPUTexture *texture = SDL_CreateGPUTexture(
            r->device, &(SDL_GPUTextureCreateInfo){.type = SDL_GPU_TEXTURETYPE_2D,
                                                   .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                                   .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                                   .width = key.width,
                                                   .height = key.height,
                                                   .layer_count_or_depth = 1,
                                                   .num_levels = 1});
        SDL_GPUTransferBuffer *upload = SDL_CreateGPUTransferBuffer(
            r->device, &(SDL_GPUTransferBufferCreateInfo){
                           .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = (Uint32)bytes});
        void *mapped = upload ? SDL_MapGPUTransferBuffer(r->device, upload, false) : NULL;
        if (!texture || !mapped) {
            if (mapped)
                SDL_UnmapGPUTransferBuffer(r->device, upload);
            if (upload)
                SDL_ReleaseGPUTransferBuffer(r->device, upload);
            if (texture)
                SDL_ReleaseGPUTexture(r->device, texture);
            free(rgba);
            return NULL;
        }
        memcpy(mapped, rgba, bytes);
        free(rgba);
        SDL_UnmapGPUTransferBuffer(r->device, upload);
        SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(r->device);
        if (!cb) {
            SDL_ReleaseGPUTransferBuffer(r->device, upload);
            SDL_ReleaseGPUTexture(r->device, texture);
            return NULL;
        }
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cb);
        SDL_UploadToGPUTexture(
            copy,
            &(SDL_GPUTextureTransferInfo){.transfer_buffer = upload,
                                          .pixels_per_row = key.width,
                                          .rows_per_layer = key.height},
            &(SDL_GPUTextureRegion){.texture = texture, .w = key.width, .h = key.height, .d = 1},
            false);
        SDL_EndGPUCopyPass(copy);
        bool submitted = SDL_SubmitGPUCommandBuffer(cb);
        SDL_ReleaseGPUTransferBuffer(r->device, upload);
        if (!submitted) {
            SDL_ReleaseGPUTexture(r->device, texture);
            return NULL;
        }
        *entry = (gpu_bitmap){.texture = texture, .key = key, .shadow = shadow};
        r->bitmap_bytes += bytes;
        if (shadow)
            r->shadow_bytes += bytes;
    }
    entry->used = ++r->clock;
    entry->frame = r->frame;
    return entry->texture;
}
static bool matches(cui_sdl_gpu *r, gpu_glyph g, int pitch, const unsigned char *p) {
    for (int j = 0; j < g.h; j++)
        if (memcmp(r->pixels + (g.y + j) * ATLAS + g.x, p + j * pitch, (size_t)g.w))
            return false;
    return true;
}
static void glyph(void *user, int x, int y, int w, int h, int pitch,
                  const unsigned char *coverage) {
    cui_sdl_gpu *r = user;
    if (!w || !h || r->failed)
        return;
    if (!coverage || w < 0 || h < 0 || w > ATLAS - 2 || h > ATLAS - 2 || pitch < w) {
        r->failed = true;
        return;
    }
    uint64_t hash = 14695981039346656037ULL;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            hash ^= coverage[j * pitch + i];
            hash *= 1099511628211ULL;
        }
    gpu_glyph *g = NULL;
    for (size_t i = 0; i < r->glyph_count; i++)
        if (r->glyphs[i].hash == hash && r->glyphs[i].w == w && r->glyphs[i].h == h &&
            matches(r, r->glyphs[i], pitch, coverage)) {
            g = &r->glyphs[i];
            break;
        }
    if (!g) {
        if (r->shelf_x + w + 1 > ATLAS) {
            r->shelf_x = 1;
            r->shelf_y += r->shelf_h + 1;
            r->shelf_h = 0;
        }
        if (r->glyph_count == GLYPHS || r->shelf_y + h + 1 > ATLAS) {
            r->failed = true;
            SDL_SetError("CUI GPU glyph atlas exhausted");
            return;
        }
        g = &r->glyphs[r->glyph_count++];
        *g = (gpu_glyph){hash, r->shelf_x, r->shelf_y, w, h};
        for (int j = 0; j < h; j++)
            memcpy(r->pixels + (g->y + j) * ATLAS + g->x, coverage + j * pitch, (size_t)w);
        r->shelf_x += w + 1;
        r->shelf_h = r->shelf_h > h ? r->shelf_h : h;
        r->dirty = true;
    }
    gpu_draw *d = append(r, (cui_rect){(float)x, (float)y, (float)w, (float)h}, r->ink, 0, 0);
    if (d)
        memcpy(
            d->params.atlas,
            (float[]){g->x / (float)ATLAS, g->y / (float)ATLAS, w / (float)ATLAS, h / (float)ATLAS},
            16);
}
static void color_glyph(void *user, int x, int y, int w, int h, int pitch,
                        const unsigned char *data, cui_color color) {
    cui_sdl_gpu *r = user;
    r->ink = color;
    glyph(user, x, y, w, h, pitch, data);
}
static bool text_draw(cui_sdl_gpu *r, const cui_draw_command *c) {
    if (c->text_color_count)
        return cui_font_draw_context_line_colored(
            r->font, c->text_source, c->font_size, c->text_style, c->bounds.x, c->bounds.y,
            c->color, c->text_colors, c->text_color_count, color_glyph, r);
    return cui_font_draw_context_line(r->font, c->text, c->text_source, c->font_size, c->text_style,
                                      c->bounds.x, c->bounds.y, glyph, r);
}

static bool collect(cui_sdl_gpu *r, const cui_draw_list *list) {
    SDL_Rect stack[CLIPS];
    size_t depth = 0;
    r->draw_count = 0;
    r->mesh_count = 0;
    r->frame++;
    r->failed = false;
    r->logical_w = list->width;
    r->logical_h = list->height;
    r->clip = (SDL_Rect){0, 0, (int)ceilf(list->width), (int)ceilf(list->height)};
    for (size_t i = 0; i < list->count; i++) {
        const cui_draw_command *c = &list->commands[i];
        if (!isfinite(c->bounds.x) || !isfinite(c->bounds.y) || !isfinite(c->bounds.w) ||
            !isfinite(c->bounds.h) || !cui_shape_valid(c) || !isfinite(c->radius) ||
            !isfinite(c->stroke) || !isfinite(c->font_size) || fabsf(c->bounds.x) > 1048576 ||
            fabsf(c->bounds.y) > 1048576 || c->bounds.w < 0 || c->bounds.h < 0 ||
            c->bounds.w > 1048576 || c->bounds.h > 1048576) {
            SDL_SetError("CUI GPU invalid command geometry");
            return false;
        }
        switch (c->type) {
        case CUI_DRAW_MESH: {
            cui_mesh_paint m = c->mesh;
            if (!cui_mesh_valid(m) || m.vertex_count > MESH_VERTICES - r->mesh_count) {
                SDL_SetError("CUI GPU invalid mesh or mesh capacity exceeded");
                return false;
            }
            gpu_draw *d = append(r, c->bounds, (cui_color){0}, 0, 0);
            if (!d)
                return false;
            d->mesh_first = (Uint32)r->mesh_count;
            d->mesh_count = (Uint32)m.vertex_count;
            memcpy(d->params.color, (float[]){m.color.r, m.color.g, m.color.b, m.color.a}, 16);
            for (size_t j = 0; j < m.vertex_count; j++) {
                cui_mesh_vertex v = m.vertices[j];
                v.x = m.origin_x + v.x * m.scale;
                v.y = m.origin_y + v.y * m.scale;
                r->mesh_vertices[r->mesh_count++] = v;
            }
            break;
        }
        case CUI_DRAW_DROP_SHADOW:
        case CUI_DRAW_IMAGE: {
            if (c->bounds.w <= 0 || c->bounds.h <= 0)
                break;
            SDL_GPUTexture *texture = bitmap_texture(r, c);
            if (!texture)
                return false;
            gpu_draw *d =
                append(r, c->bounds,
                       (cui_color){255, 255, 255, (uint8_t)lroundf(c->bitmap.opacity * 255)}, 0, 0);
            if (d) {
                d->texture = texture;
                d->shadow = c->type == CUI_DRAW_DROP_SHADOW;
                d->params.shape[2] = -1;
            }
            break;
        }
        case CUI_DRAW_RECT:
        case CUI_DRAW_BORDER: {
            gpu_draw *d =
                append(r, c->bounds, c->color, c->radius, c->type == CUI_DRAW_BORDER ? 1 : 0);
            if (d)
                cui_shape_values(c, d->params.radii, d->params.edges);
        } break;
        case CUI_DRAW_TEXT:
            r->ink = c->color;
            if (!r->font || !text_draw(r, c))
                return false;
            break;
        case CUI_DRAW_CLIP_PUSH: {
            if (depth == CLIPS)
                return false;
            stack[depth++] = r->clip;
            SDL_Rect next = {(int)floorf(c->bounds.x), (int)floorf(c->bounds.y),
                             (int)ceilf(c->bounds.w), (int)ceilf(c->bounds.h)};
            if (!SDL_GetRectIntersection(&r->clip, &next, &r->clip))
                r->clip = (SDL_Rect){0};
            break;
        }
        case CUI_DRAW_CLIP_POP:
            if (!depth)
                return false;
            r->clip = stack[--depth];
            break;
        default:
            SDL_SetError("Unknown CUI GPU draw command");
            return false;
        }
        if (r->failed)
            return false;
    }
    return !depth;
}
/* An acquired swapchain buffer cannot be cancelled. On a late failure retain
 * the previous completed offscreen pixels when available, then legally submit. */
static bool failed_acquired(cui_sdl_gpu *r, SDL_GPUCommandBuffer *cb, SDL_GPUTexture *swap,
                            Uint32 w, Uint32 h) {
    if (r->surface)
        SDL_BlitGPUTexture(
            cb, &(SDL_GPUBlitInfo){.source = {.texture = r->surface, .w = r->width, .h = r->height},
                                   .destination = {.texture = swap, .w = w, .h = h},
                                   .load_op = SDL_GPU_LOADOP_DONT_CARE,
                                   .filter = SDL_GPU_FILTER_NEAREST});
    if (SDL_SubmitGPUCommandBuffer(cb))
        r->dirty = false;
    return false;
}
static SDL_GPUTexture *frame_surface(cui_sdl_gpu *r, Uint32 w, Uint32 h) {
    return SDL_CreateGPUTexture(
        r->device, &(SDL_GPUTextureCreateInfo){.type = SDL_GPU_TEXTURETYPE_2D,
                                               .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                               .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                                                        SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                               .width = w,
                                               .height = h,
                                               .layer_count_or_depth = 1,
                                               .num_levels = 1});
}
bool cui_sdl_gpu_render(cui_sdl_gpu *r, const cui_draw_list *list, cui_color clear) {
    if (!r || !list || !isfinite(list->width) || !isfinite(list->height) || list->width <= 0 ||
        list->height <= 0 || list->width > 32768 || list->height > 32768 ||
        (!list->commands && list->count))
        return false;
    int physical_w, physical_h;
    if (!SDL_GetWindowSizeInPixels(r->window, &physical_w, &physical_h) || physical_w <= 0 ||
        physical_h <= 0)
        return false;
    bool shadow_frame = cui_shadow_present(list);
    cui_draw_list prepared;
    if (!cui_shadow_prepare(&r->shadows, list, physical_w / list->width, physical_h / list->height,
                            0, 0, &prepared) ||
        !collect(r, &prepared))
        return false;
    list = &prepared;
    Uint32 w = (Uint32)physical_w, h = (Uint32)physical_h;
    SDL_GPUTexture *candidate = NULL;
    if (!r->surface || w != r->width || h != r->height) {
        candidate = frame_surface(r, w, h);
        if (!candidate)
            return false;
    }
    if (r->dirty) {
        void *mapped = SDL_MapGPUTransferBuffer(r->device, r->upload, true);
        if (!mapped) {
            if (candidate)
                SDL_ReleaseGPUTexture(r->device, candidate);
            return false;
        }
        memcpy(mapped, r->pixels, ATLAS * ATLAS);
        SDL_UnmapGPUTransferBuffer(r->device, r->upload);
    }
    if (r->mesh_count) {
        void *mapped = SDL_MapGPUTransferBuffer(r->device, r->mesh_upload, true);
        if (!mapped) {
            if (candidate)
                SDL_ReleaseGPUTexture(r->device, candidate);
            return false;
        }
        memcpy(mapped, r->mesh_vertices, r->mesh_count * sizeof(cui_mesh_vertex));
        SDL_UnmapGPUTransferBuffer(r->device, r->mesh_upload);
    }
    SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(r->device);
    if (!cb) {
        if (candidate)
            SDL_ReleaseGPUTexture(r->device, candidate);
        return false;
    }
    if (r->dirty || r->mesh_count) {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cb);
        if (!copy) {
            SDL_CancelGPUCommandBuffer(cb);
            if (candidate)
                SDL_ReleaseGPUTexture(r->device, candidate);
            return false;
        }
        if (r->dirty)
            SDL_UploadToGPUTexture(
                copy,
                &(SDL_GPUTextureTransferInfo){
                    .transfer_buffer = r->upload, .pixels_per_row = ATLAS, .rows_per_layer = ATLAS},
                &(SDL_GPUTextureRegion){.texture = r->atlas, .w = ATLAS, .h = ATLAS, .d = 1},
                false);
        if (r->mesh_count)
            SDL_UploadToGPUBuffer(
                copy, &(SDL_GPUTransferBufferLocation){.transfer_buffer = r->mesh_upload},
                &(SDL_GPUBufferRegion){.buffer = r->mesh_buffer,
                                       .size = (Uint32)(r->mesh_count * sizeof(cui_mesh_vertex))},
                true);
        SDL_EndGPUCopyPass(copy);
    }
    SDL_GPUTexture *swap = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cb, r->window, &swap, &w, &h)) {
        if (candidate)
            SDL_ReleaseGPUTexture(r->device, candidate);
        if (swap)
            return failed_acquired(r, cb, swap, w, h);
        SDL_CancelGPUCommandBuffer(cb);
        return false;
    }
    if (!swap) {
        SDL_CancelGPUCommandBuffer(cb);
        if (candidate)
            SDL_ReleaseGPUTexture(r->device, candidate);
        return true;
    }
    if (shadow_frame && (w != (Uint32)physical_w || h != (Uint32)physical_h)) {
        if (!cui_shadow_uniform(w / list->width, h / list->height)) {
            if (candidate)
                SDL_ReleaseGPUTexture(r->device, candidate);
            return failed_acquired(r, cb, swap, w, h);
        }
        /* A changed scale needs fresh rasterization/upload before clear. */
        if (!cui_shadow_prepare(&r->shadows, list, w / list->width, h / list->height, 0, 0,
                                &prepared) ||
            !collect(r, &prepared)) {
            if (candidate)
                SDL_ReleaseGPUTexture(r->device, candidate);
            return failed_acquired(r, cb, swap, w, h);
        }
    }
    /* Drawable size can change between preflight and acquisition. */
    if (w != (Uint32)physical_w || h != (Uint32)physical_h) {
        if (candidate)
            SDL_ReleaseGPUTexture(r->device, candidate);
        candidate = frame_surface(r, w, h);
        if (!candidate)
            return failed_acquired(r, cb, swap, w, h);
    }
    SDL_GPUTexture *surface = candidate ? candidate : r->surface;
    SDL_GPUColorTargetInfo target = {
        .texture = surface,
        .clear_color = {clear.r / 255.f, clear.g / 255.f, clear.b / 255.f, clear.a / 255.f},
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE};
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cb, &target, 1, NULL);
    if (!pass) {
        if (candidate)
            SDL_ReleaseGPUTexture(r->device, candidate);
        return failed_acquired(r, cb, swap, w, h);
    }
    if (candidate) {
        if (r->surface)
            SDL_ReleaseGPUTexture(r->device, r->surface);
        r->surface = candidate;
        r->width = w;
        r->height = h;
    }
    SDL_BindGPUGraphicsPipeline(pass, r->pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, &(SDL_GPUTextureSamplerBinding){r->atlas, r->sampler}, 1);
    for (size_t i = 0; i < r->draw_count; i++) {
        gpu_draw *d = &r->draws[i];
        if (d->clip.w <= 0 || d->clip.h <= 0)
            continue;
        float sx = w / list->width, sy = h / list->height;
        SDL_Rect clip = {(int)floorf(d->clip.x * sx), (int)floorf(d->clip.y * sy),
                         (int)ceilf(d->clip.w * sx), (int)ceilf(d->clip.h * sy)};
        if (d->shadow)
            clip = (SDL_Rect){0, 0, (int)w, (int)h};
        if (shadow_frame && !d->shadow) {
            int left = (int)fmaxf(0, fminf((float)w, floorf(d->clip.x * sx)));
            int top = (int)fmaxf(0, fminf((float)h, floorf(d->clip.y * sy)));
            int right =
                (int)fmaxf((float)left, fminf((float)w, ceilf((d->clip.x + d->clip.w) * sx)));
            int bottom =
                (int)fmaxf((float)top, fminf((float)h, ceilf((d->clip.y + d->clip.h) * sy)));
            clip = (SDL_Rect){left, top, right - left, bottom - top};
        }
        SDL_BindGPUGraphicsPipeline(pass, d->mesh_count ? r->mesh_pipeline : r->pipeline);
        if (d->mesh_count)
            SDL_BindGPUVertexBuffers(pass, 0, &(SDL_GPUBufferBinding){.buffer = r->mesh_buffer}, 1);
        else
            SDL_BindGPUFragmentSamplers(
                pass, 0,
                &(SDL_GPUTextureSamplerBinding){d->texture ? d->texture : r->atlas, r->sampler}, 1);
        SDL_SetGPUScissor(pass, &clip);
        SDL_PushGPUVertexUniformData(cb, 0, &d->params, sizeof(d->params));
        SDL_PushGPUFragmentUniformData(cb, 0, &d->params, sizeof(d->params));
        SDL_DrawGPUPrimitives(pass, d->mesh_count ? d->mesh_count : 6, 1, d->mesh_first, 0);
    }
    SDL_EndGPURenderPass(pass);
    SDL_BlitGPUTexture(cb, &(SDL_GPUBlitInfo){.source = {.texture = r->surface, .w = w, .h = h},
                                              .destination = {.texture = swap, .w = w, .h = h},
                                              .load_op = SDL_GPU_LOADOP_DONT_CARE,
                                              .filter = SDL_GPU_FILTER_NEAREST});
    bool submitted = SDL_SubmitGPUCommandBuffer(cb);
    if (submitted)
        r->dirty = false;
    return submitted;
}
bool cui_sdl_gpu_save_ppm(cui_sdl_gpu *r, const char *path) {
    if (!r || !r->surface || !path || r->width > UINT32_MAX / 4 / r->height)
        return false;
    Uint32 bytes = r->width * r->height * 4;
    SDL_GPUTransferBuffer *download = SDL_CreateGPUTransferBuffer(
        r->device, &(SDL_GPUTransferBufferCreateInfo){.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                                                      .size = bytes});
    if (!download)
        return false;
    SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(r->device);
    bool ok = false;
    if (cb) {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cb);
        SDL_DownloadFromGPUTexture(
            copy,
            &(SDL_GPUTextureRegion){.texture = r->surface, .w = r->width, .h = r->height, .d = 1},
            &(SDL_GPUTextureTransferInfo){.transfer_buffer = download,
                                          .pixels_per_row = r->width,
                                          .rows_per_layer = r->height});
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb);
        if (fence) {
            if (SDL_WaitForGPUFences(r->device, true, &fence, 1)) {
                const unsigned char *pixels = SDL_MapGPUTransferBuffer(r->device, download, false);
                if (pixels) {
                    FILE *f = fopen(path, "wb");
                    if (f) {
                        ok = fprintf(f, "P6\n%u %u\n255\n", r->width, r->height) > 0;
                        for (size_t i = 0; ok && i < (size_t)r->width * r->height; i++)
                            ok = fwrite(pixels + i * 4, 1, 3, f) == 3;
                        if (fclose(f))
                            ok = false;
                    }
                    SDL_UnmapGPUTransferBuffer(r->device, download);
                }
            }
            SDL_ReleaseGPUFence(r->device, fence);
        }
    }
    SDL_ReleaseGPUTransferBuffer(r->device, download);
    return ok;
}
const char *cui_sdl_gpu_driver(cui_sdl_gpu *r) {
    return r ? SDL_GetGPUDeviceDriver(r->device) : "unavailable";
}
static bool render(void *r, const cui_draw_list *l, cui_color c) {
    return cui_sdl_gpu_render(r, l, c);
}
static void destroy(void *r) {
    cui_sdl_gpu_destroy(r);
}
cui_renderer cui_sdl_gpu_interface(cui_sdl_gpu *r) {
    return (cui_renderer){r, cui_sdl_gpu_driver(r),
                          (r && r->font ? CUI_RENDER_TEXT : 0) | CUI_RENDER_ROUNDED |
                              CUI_RENDER_CLIP | CUI_RENDER_IMAGE | CUI_RENDER_MESH |
                              CUI_RENDER_DROP_SHADOW,
                          render, destroy};
}
