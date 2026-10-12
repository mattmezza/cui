/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl_gpu.h"
#include "../software/cui_shape.h"
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
typedef struct {
    float bounds[4], viewport[4], color[4], shape[4], atlas[4], radii[4], edges[4];
} gpu_params;
typedef struct {
    gpu_params params;
    SDL_Rect clip;
} gpu_draw;
typedef struct {
    uint64_t hash;
    int x, y, w, h;
} gpu_glyph;
struct cui_sdl_gpu {
    SDL_GPUDevice *device;
    SDL_Window *window;
    cui_font *font;
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUTexture *atlas, *surface;
    SDL_GPUSampler *sampler;
    SDL_GPUTransferBuffer *upload;
    unsigned char *pixels;
    gpu_glyph glyphs[GLYPHS];
    size_t glyph_count;
    gpu_draw *draws;
    size_t draw_count;
    int shelf_x, shelf_y, shelf_h;
    Uint32 width, height;
    bool dirty, failed;
    SDL_Rect clip;
    cui_color ink;
    float logical_w, logical_h;
};
static SDL_GPUShader *shader(SDL_GPUDevice *d, bool fragment) {
    SDL_GPUShaderCreateInfo info = {
        .code_size = fragment ? sizeof(cui_gpu_frag) : sizeof(cui_gpu_vert),
        .code = fragment ? cui_gpu_frag : cui_gpu_vert,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = fragment ? SDL_GPU_SHADERSTAGE_FRAGMENT : SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = fragment ? 1 : 0,
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
    SDL_GPUShader *v = shader(device, false), *f = shader(device, true);
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
    if (!r->pixels || !r->draws || !r->pipeline || !r->atlas || !r->sampler || !r->upload) {
        cui_sdl_gpu_destroy(r);
        return NULL;
    }
    return r;
}
void cui_sdl_gpu_destroy(cui_sdl_gpu *r) {
    if (!r)
        return;
    if (r->pipeline)
        SDL_ReleaseGPUGraphicsPipeline(r->device, r->pipeline);
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
static bool collect(cui_sdl_gpu *r, const cui_draw_list *list) {
    SDL_Rect stack[CLIPS];
    size_t depth = 0;
    r->draw_count = 0;
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
        case CUI_DRAW_RECT:
        case CUI_DRAW_BORDER: {
            gpu_draw *d =
                append(r, c->bounds, c->color, c->radius, c->type == CUI_DRAW_BORDER ? 1 : 0);
            if (d)
                cui_shape_values(c, d->params.radii, d->params.edges);
        } break;
        case CUI_DRAW_TEXT:
            r->ink = c->color;
            if (!r->font ||
                !cui_font_draw(r->font, c->text, c->font_size, c->bounds.x, c->bounds.y, glyph, r))
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
            return false;
        }
        if (r->failed)
            return false;
    }
    return !depth;
}
bool cui_sdl_gpu_render(cui_sdl_gpu *r, const cui_draw_list *list, cui_color clear) {
    if (!r || !list || !isfinite(list->width) || !isfinite(list->height) || list->width <= 0 ||
        list->height <= 0 || list->width > 32768 || list->height > 32768 ||
        (!list->commands && list->count) || !collect(r, list))
        return false;
    SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(r->device);
    if (!cb)
        return false;
    SDL_GPUTexture *swap = NULL;
    Uint32 w, h;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cb, r->window, &swap, &w, &h)) {
        SDL_CancelGPUCommandBuffer(cb);
        return false;
    }
    if (!swap) {
        SDL_CancelGPUCommandBuffer(cb);
        return true;
    }
    if (!r->surface || w != r->width || h != r->height) {
        SDL_GPUTexture *surface = SDL_CreateGPUTexture(
            r->device, &(SDL_GPUTextureCreateInfo){.type = SDL_GPU_TEXTURETYPE_2D,
                                                   .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                                   .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                                                            SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                                   .width = w,
                                                   .height = h,
                                                   .layer_count_or_depth = 1,
                                                   .num_levels = 1});
        if (!surface) {
            SDL_CancelGPUCommandBuffer(cb);
            return false;
        }
        if (r->surface)
            SDL_ReleaseGPUTexture(r->device, r->surface);
        r->surface = surface;
        r->width = w;
        r->height = h;
    }
    if (r->dirty) {
        void *p = SDL_MapGPUTransferBuffer(r->device, r->upload, true);
        if (!p) {
            SDL_CancelGPUCommandBuffer(cb);
            return false;
        }
        memcpy(p, r->pixels, ATLAS * ATLAS);
        SDL_UnmapGPUTransferBuffer(r->device, r->upload);
        SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb);
        SDL_UploadToGPUTexture(
            cp,
            &(SDL_GPUTextureTransferInfo){
                .transfer_buffer = r->upload, .pixels_per_row = ATLAS, .rows_per_layer = ATLAS},
            &(SDL_GPUTextureRegion){.texture = r->atlas, .w = ATLAS, .h = ATLAS, .d = 1}, false);
        SDL_EndGPUCopyPass(cp);
        r->dirty = false;
    }
    SDL_GPUColorTargetInfo target = {
        .texture = r->surface,
        .clear_color = {clear.r / 255.f, clear.g / 255.f, clear.b / 255.f, clear.a / 255.f},
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE};
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cb, &target, 1, NULL);
    SDL_BindGPUGraphicsPipeline(pass, r->pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, &(SDL_GPUTextureSamplerBinding){r->atlas, r->sampler}, 1);
    for (size_t i = 0; i < r->draw_count; i++) {
        gpu_draw *d = &r->draws[i];
        if (d->clip.w <= 0 || d->clip.h <= 0)
            continue;
        float sx = w / list->width, sy = h / list->height;
        SDL_Rect clip = {(int)floorf(d->clip.x * sx), (int)floorf(d->clip.y * sy),
                         (int)ceilf(d->clip.w * sx), (int)ceilf(d->clip.h * sy)};
        SDL_SetGPUScissor(pass, &clip);
        SDL_PushGPUVertexUniformData(cb, 0, &d->params, sizeof(d->params));
        SDL_PushGPUFragmentUniformData(cb, 0, &d->params, sizeof(d->params));
        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    }
    SDL_EndGPURenderPass(pass);
    SDL_BlitGPUTexture(cb, &(SDL_GPUBlitInfo){.source = {.texture = r->surface, .w = w, .h = h},
                                              .destination = {.texture = swap, .w = w, .h = h},
                                              .load_op = SDL_GPU_LOADOP_DONT_CARE,
                                              .filter = SDL_GPU_FILTER_NEAREST});
    return SDL_SubmitGPUCommandBuffer(cb);
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
                              CUI_RENDER_CLIP,
                          render, destroy};
}
