/* SPDX-License-Identifier: MIT */
/* Same-TU native failures/owner reentry; every window is hidden at creation. */
#include "cui/cui_software.h"
#include "sdl_v2_reference_internal.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum {
    FAIL_NONE,
    FAIL_SIZE,
    FAIL_CREATE,
    FAIL_UPLOAD,
    FAIL_DRAW,
    FAIL_FLUSH,
    FAIL_PRESENT,
    FAIL_SNAPSHOT,
    FAIL_LOGICAL,
    FAIL_RESTORE
};
static int failure;
static size_t native_calls, presents, allocation_calls, fail_allocation, live, object_calls,
    object_bytes;
static bool recurse, capture_enabled, reject_format;
static void *allocated[512];
static unsigned char uploaded[32 * 24 * 4];
static cui_sdl3 *active;
static const cui_draw_list_v2 *active_list;
static cui_platform retained_platform;
static cui_window_host retained_window;
static cui_renderer retained_renderer;
static unsigned char captured[32 * 24 * 4], original_pixels[32 * 24 * 4];
static void busy_probe(void);
static bool fail(int kind) {
    ++native_calls;
    if (failure != kind)
        return false;
    failure = FAIL_NONE;
    return true;
}
bool sdl_v2_capture(SDL_Renderer *r, bool original) {
    if (!capture_enabled)
        return true;
    SDL_Surface *surface = SDL_RenderReadPixels(r, NULL);
    assert(surface && surface->w == 32 && surface->h == 24);
    unsigned char *out = original ? original_pixels : captured;
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 32; ++x)
            assert(SDL_ReadSurfacePixel(surface, x, y, out + (y * 32 + x) * 4,
                                        out + (y * 32 + x) * 4 + 1, out + (y * 32 + x) * 4 + 2,
                                        out + (y * 32 + x) * 4 + 3));
    SDL_DestroySurface(surface);
    return true;
}
static void *test_pointer_property(SDL_PropertiesID props, const char *name, void *fallback) {
    static SDL_PixelFormat refused[] = {SDL_PIXELFORMAT_INDEX8, SDL_PIXELFORMAT_UNKNOWN};
    if (reject_format && !strcmp(name, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER))
        return refused;
    return SDL_GetPointerProperty(props, name, fallback);
}
static bool test_size(SDL_Window *w, int *x, int *y) {
    if (fail(FAIL_SIZE))
        return false;
    if (recurse)
        busy_probe();
    return SDL_GetWindowSize(w, x, y);
}
static SDL_Texture *test_create(SDL_Renderer *r, SDL_PixelFormat f, SDL_TextureAccess a, int w,
                                int h) {
    if (fail(FAIL_CREATE))
        return NULL;
    return SDL_CreateTexture(r, f, a, w, h);
}
static bool test_upload(SDL_Texture *t, const SDL_Rect *r, const void *p, int pitch) {
    if (fail(FAIL_UPLOAD))
        return false;
    if (capture_enabled) {
        assert(pitch == 32 * 4);
        memcpy(uploaded, p, sizeof uploaded);
    }
    return SDL_UpdateTexture(t, r, p, pitch);
}
static bool test_draw(SDL_Renderer *r, SDL_Texture *t, const SDL_FRect *s, const SDL_FRect *d) {
    return !fail(FAIL_DRAW) && SDL_RenderTexture(r, t, s, d);
}
static bool test_flush(SDL_Renderer *r) {
    return !fail(FAIL_FLUSH) && SDL_FlushRenderer(r);
}
static bool test_present(SDL_Renderer *r) {
    ++presents;
    if (fail(FAIL_PRESENT))
        return false;
    return sdl_v2_capture(r, false) && SDL_RenderPresent(r);
}
static bool test_snapshot(SDL_Renderer *r, float *x, float *y) {
    return !fail(FAIL_SNAPSHOT) && SDL_GetRenderScale(r, x, y);
}
static bool test_logical(SDL_Renderer *r, int w, int h, SDL_RendererLogicalPresentation mode) {
    return !fail(FAIL_LOGICAL) && SDL_SetRenderLogicalPresentation(r, w, h, mode);
}
static bool test_scale(SDL_Renderer *r, float x, float y) {
    if (failure == FAIL_RESTORE && x == 1.25f) {
        failure = FAIL_NONE;
        return false;
    }
    return SDL_SetRenderScale(r, x, y);
}
static void *test_allocate(size_t bytes) {
    if (recurse)
        busy_probe();
    if (++allocation_calls == fail_allocation)
        return NULL;
    void *p = malloc(bytes);
    assert(p);
    assert(live < 512);
    allocated[live++] = p;
    return p;
}
static void test_release(void *p) {
    if (recurse)
        busy_probe();
    for (size_t i = 0; i < live; ++i)
        if (allocated[i] == p) {
            allocated[i] = allocated[--live];
            break;
        }
    free(p);
}
static void *test_calloc(size_t n, size_t bytes) {
    ++object_calls;
    object_bytes = n * bytes;
    return calloc(n, bytes);
}
#define SDL_GetPointerProperty test_pointer_property
#define SDL_GetWindowSize test_size
#define SDL_CreateTexture test_create
#define SDL_UpdateTexture test_upload
#define SDL_RenderTexture test_draw
#define SDL_FlushRenderer test_flush
#define SDL_RenderPresent test_present
#define SDL_GetRenderScale test_snapshot
#define SDL_SetRenderLogicalPresentation test_logical
#define SDL_SetRenderScale test_scale
#define malloc test_allocate
#define free test_release
#define calloc test_calloc
#include "../../backends/sdl3/cui_sdl3.c"
#undef calloc
#undef free
#undef malloc
#undef SDL_GetPointerProperty
#undef SDL_GetWindowSize
#undef SDL_CreateTexture
#undef SDL_UpdateTexture
#undef SDL_RenderTexture
#undef SDL_FlushRenderer
#undef SDL_RenderPresent
#undef SDL_GetRenderScale
#undef SDL_SetRenderLogicalPresentation
#undef SDL_SetRenderScale
static void busy_probe(void) {
    assert(active && active->v2_busy);
    bool old = recurse;
    recurse = false;
    size_t before = native_calls;
    cui_sdl3_v2_info info = {.size = sizeof(info)}, saved = info;
    assert(cui_sdl3_query_v2(active, &info) == CUI_ERROR_LIFECYCLE);
    assert(!memcmp(&saved, &info, sizeof(info)));
    assert(cui_sdl3_render_v2(active, active_list, (cui_color){0}, NULL) == CUI_ERROR_LIFECYCLE);
    assert(!cui_sdl3_render(active, (const cui_draw_list *)(uintptr_t)1, (cui_color){0}));
    assert(!retained_renderer.render(retained_renderer.user, NULL, (cui_color){0}));
    assert(!cui_sdl3_set_visible(active, true) && !cui_sdl3_is_visible(active));
    assert(!cui_sdl3_set_decorated(active, true) &&
           !cui_sdl3_set_cursor(active, CUI_CURSOR_DEFAULT));
    assert(!cui_sdl3_poll(active, NULL) && !cui_sdl3_wait_events(active, 0));
    assert(!cui_sdl3_save_bmp(active, "must-not-write.bmp"));
    float w = 7, h = 9;
    cui_sdl3_size(active, &w, &h);
    assert(w == 7 && h == 9);
    assert(cui_sdl3_frame_delta(active) == 0 && !strcmp(cui_sdl3_driver(active), "unavailable"));
    cui_platform_appearance appearance = {0};
    assert(!cui_sdl3_appearance(active, &appearance));
    cui_accessibility_window window = {0};
    assert(!cui_sdl3_accessibility_window(active, &window));
    assert(!cui_sdl3_interface(active).user && !cui_sdl3_clipboard_interface(active).user &&
           !cui_sdl3_window_interface(active).user);
    assert(!retained_platform.clipboard_set(retained_platform.user, cui_string("x")));
    assert(!retained_platform.clipboard_get(retained_platform.user).length);
    assert(!retained_window.query(retained_window.user).actions);
    assert(!retained_window.perform(retained_window.user, CUI_WINDOW_CLOSE, 0, 0));
    cui_sdl3_destroy(active);
    retained_renderer.destroy(retained_renderer.user);
    assert(native_calls == before && active->v2_busy);
    recurse = old;
}
static cui_draw_list_v2 make_list(cui_draw_command_v2 *commands, size_t count) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = 2,
                              .command_stride = sizeof(*commands),
                              .commands = commands,
                              .count = count,
                              .width = 32,
                              .height = 24,
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1};
}
static cui_draw_command_v2 legacy(cui_draw_command c) {
    return (cui_draw_command_v2){.kind = c.type, .payload.legacy = c};
}
static void state_equal(SDL_Renderer *r, const sdl_v2_state *before) {
    sdl_v2_state after;
    assert(sdl_v2_snapshot(r, &after));
    assert(before->logical_w == after.logical_w && before->logical_h == after.logical_h &&
           before->logical_mode == after.logical_mode && before->scale_x == after.scale_x &&
           before->scale_y == after.scale_y && before->color_scale == after.color_scale &&
           before->viewport_set == after.viewport_set &&
           before->clip_enabled == after.clip_enabled);
    assert(before->viewport.x == after.viewport.x && before->viewport.y == after.viewport.y &&
           before->viewport.w == after.viewport.w && before->viewport.h == after.viewport.h);
    assert(before->clip.x == after.clip.x && before->clip.y == after.clip.y &&
           before->clip.w == after.clip.w && before->clip.h == after.clip.h);
}
static void aliases(cui_sdl3 *s, cui_draw_list_v2 list) {
    unsigned char saved[sizeof(*s)];
    memcpy(saved, s, sizeof(*s));
    size_t calls = native_calls, allocations = allocation_calls;
    assert(cui_sdl3_query_v2(s, (cui_sdl3_v2_info *)s) == CUI_ERROR_INVALID_ACTION);
    assert(cui_sdl3_render_v2(s, (const cui_draw_list_v2 *)s, (cui_color){0}, NULL) ==
           CUI_ERROR_INVALID_ACTION);
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, (const cui_render_v2_options *)s) ==
           CUI_ERROR_INVALID_ACTION);
    list.commands = (const cui_draw_command_v2 *)s;
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    cui_draw_command_v2 c =
        legacy((cui_draw_command){.type = CUI_DRAW_IMAGE,
                                  .bitmap = {.bitmap = {(const unsigned char *)s, 1, 1, 4, 0},
                                             .source = {0, 0, 1, 1},
                                             .opacity = 1}});
    list.commands = &c;
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    c = legacy(
        (cui_draw_command){.type = CUI_DRAW_MESH,
                           .mesh = {.vertices = (const cui_mesh_vertex *)s, .vertex_count = 3}});
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    c = legacy((cui_draw_command){.type = CUI_DRAW_TEXT, .text = {(const char *)s, 1}});
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_source = (cui_text_line_source){{"a", 1}, 0, 1};
    c.payload.legacy.text_colors = (const cui_text_color_span *)s;
    c.payload.legacy.text_color_count = 1;
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    c.payload.legacy.text_color_count = 0;
    c.payload.legacy.text_paint.kind = CUI_TEXT_PAINT_LINEAR;
    c.payload.legacy.text_paint.stops = (const cui_text_gradient_stop *)s;
    c.payload.legacy.text_paint.stop_count = 2;
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    assert(native_calls == calls && allocation_calls == allocations &&
           !memcmp(saved, s, sizeof(*s)));
}
static void compare_upload(cui_sdl3 *s, cui_software *cpu, cui_draw_list_v2 *list,
                           cui_render_v2_options options) {
    const cui_color clear = {11, 31, 53, 71};
    active_list = list;
    assert(cui_software_render_v2(cpu, list, clear, &options) == CUI_OK);
    assert(cui_sdl3_render_v2(s, list, clear, &options) == CUI_OK);
    const unsigned char *expected = cui_software_pixels(cpu, NULL, NULL);
    assert(!memcmp(uploaded, expected, sizeof uploaded)); /* Full RGBA at native upload. */
    for (size_t i = 0; i < 32 * 24; ++i) {
        /* A window backbuffer may discard alpha; RGB must still be copied unblended. */
        assert(captured[i * 4] == expected[i * 4] && captured[i * 4 + 1] == expected[i * 4 + 1] &&
               captured[i * 4 + 2] == expected[i * 4 + 2]);
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    object_calls = object_bytes = 0;
    cui_sdl3 *s = cui_sdl3_create_with_options("SDL V2 hidden", 32, 24, font,
                                               (cui_sdl3_window_options){true});
    assert(s && object_calls == 1 && object_bytes == sizeof(*s));
    assert(SDL_GetWindowFlags(s->window) & SDL_WINDOW_HIDDEN);
    size_t native_object_bytes = object_bytes;
    active = s;
    retained_platform = cui_sdl3_clipboard_interface(s);
    retained_window = cui_sdl3_window_interface(s);
    retained_renderer = cui_sdl3_interface(s);
    cui_sdl3_v2_info info = {.size = sizeof(info)};
    assert(cui_sdl3_query_v2(s, &info) == CUI_OK);
    assert(info.defaults.viewport_width == 32 && info.defaults.viewport_height == 24 &&
           info.draw.max_commands && info.draw.max_backgrounds &&
           (info.text_features & CUI_SDL3_V2_TEXT_A8));
    assert(info.defaults.scale_x == 1 && info.defaults.scale_y == 1);
    cui_draw_command_v2 c = legacy((cui_draw_command){
        .type = CUI_DRAW_RECT, .bounds = {2, 3, 4, 5}, .color = {17, 29, 41, 255}});
    cui_draw_list_v2 list = make_list(&c, 1);
    active_list = &list;
    aliases(s, list);
    size_t allocations_before = allocation_calls, presents_before = presents;
    cui_sdl3_v2_info preserved = info;
    reject_format = true;
    assert(cui_sdl3_query_v2(s, &info) == CUI_ERROR_INVALID_ACTION);
    assert(!memcmp(&preserved, &info, sizeof info));
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    reject_format = false;
    assert(allocation_calls == allocations_before && presents == presents_before);
    SDL_Texture *target =
        SDL_CreateTexture(s->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 8, 8);
    assert(target && SDL_SetRenderTarget(s->renderer, target));
    assert(cui_sdl3_query_v2(s, &info) == CUI_ERROR_INVALID_ACTION);
    assert(SDL_SetRenderTarget(s->renderer, NULL));
    SDL_DestroyTexture(target);
    cui_draw_command_v2 invalid_late[] = {c, c};
    invalid_late[1].payload.legacy.bounds.x = NAN;
    cui_draw_list_v2 bad = make_list(invalid_late, 2);
    assert(cui_sdl3_render_v2(s, &bad, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    assert(allocation_calls == allocations_before && presents == presents_before);
    cui_render_v2_options tiny = info.defaults;
    tiny.cpu_bytes = 4;
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, &tiny) == CUI_ERROR_CAPACITY);
    assert(allocation_calls == allocations_before && presents == presents_before);
    cui_draw_command_v2 no_font = legacy((cui_draw_command){
        .type = CUI_DRAW_TEXT, .text = {"x", 1}, .font_size = 11, .color = {255, 255, 255, 255}});
    bad = make_list(&no_font, 1);
    s->font = NULL;
    assert(cui_sdl3_render_v2(s, &bad, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    s->font = font;
    assert(allocation_calls == allocations_before && presents == presents_before);
    capture_enabled = true;
    recurse = true;
    assert(cui_sdl3_render_v2(s, &list, (cui_color){1, 2, 3, 255}, NULL) == CUI_OK);
    recurse = false;
    assert(!s->v2_busy && !live);
    for (size_t y = 0; y < 24; ++y)
        for (size_t x = 0; x < 32; ++x) {
            bool inside = x >= 2 && x < 6 && y >= 3 && y < 8;
            assert(uploaded[(y * 32 + x) * 4] == (inside ? 17 : 1));
            assert(uploaded[(y * 32 + x) * 4 + 1] == (inside ? 29 : 2));
            assert(uploaded[(y * 32 + x) * 4 + 2] == (inside ? 41 : 3));
            assert(uploaded[(y * 32 + x) * 4 + 3] == 255);
        }
    cui_software *cpu = cui_software_create(32, 24, font);
    assert(cpu);
    static const unsigned char image[] = {200, 30, 70,  97, 40,  250, 60, 181,
                                          90,  20, 240, 0,  180, 100, 40, 255};
    static const cui_mesh_vertex vertices[] = {{0, 0, 0, 0}, {8, 0, 0, 0}, {0, 7, 0, 0}};
    static const cui_text_gradient_stop stops[] = {{0, {230, 40, 90, 71}},
                                                   {1, {30, 120, 240, 211}}};
    static const cui_text_color_span spans[] = {{0, 2, {240, 80, 20, 101}}};
    cui_draw_command_v2 all[16];
    size_t n = 0;
    all[n++] = legacy((cui_draw_command){
        .type = CUI_DRAW_DROP_SHADOW,
        .shadow = {
            .caster = {4.25f, 3, 20, 15},
            .radii = {2, 4, 1, 3},
            .layer = {.offset_x = -1, .offset_y = 2, .sigma = 1.25f, .color = {20, 50, 90, 127}}}});
    cui_background brush;
    assert(cui_background_make_linear(CUI_BACKGROUND_CSS, CUI_BACKGROUND_OKLAB, 71,
                                      (cui_background_stop){{0.8f, 0.1f, 0.3f, 0}, 0},
                                      (cui_background_stop){{0.1f, 0.6f, 0.8f, 0.8f}, 1}, &brush));
    all[n++] = (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND,
                                     .payload.background = {.bounds = {1.5f, 2.25f, 25, 18},
                                                            .radii = {1, 3, 2, 4},
                                                            .brush = brush,
                                                            .opacity = 0.7f}};
    all[n++] =
        legacy((cui_draw_command){.type = CUI_DRAW_CLIP_PUSH, .bounds = {2.25f, 1.75f, 27, 20}});
    all[n++] = c;
    all[n++] = legacy((cui_draw_command){
        .type = CUI_DRAW_IMAGE,
        .bounds = {7.25f, 4.5f, 9, 8},
        .radius = 2,
        .bitmap = {.bitmap = {image, 2, 2, 8, 1}, .source = {0, 0, 2, 2}, .opacity = 0.6f}});
    all[n++] = legacy((cui_draw_command){.type = CUI_DRAW_MESH,
                                         .mesh = {.vertices = vertices,
                                                  .vertex_count = 3,
                                                  .origin_x = 14.25f,
                                                  .origin_y = 6.5f,
                                                  .scale = 1,
                                                  .color = {0.1f, 0.3f, 0.05f, 0.5f}}});
    all[n++] = legacy((cui_draw_command){
        .type = CUI_DRAW_INSET_SHADOW,
        .shadow = {
            .caster = {4.25f, 3, 20, 15},
            .radii = {2, 4, 1, 3},
            .layer = {.offset_x = 1, .sigma = 0.75f, .spread = 1, .color = {70, 10, 130, 113}}}});
    all[n++] = legacy((cui_draw_command){.type = CUI_DRAW_BORDER,
                                         .bounds = {2, 2, 26, 19},
                                         .color = {230, 100, 20, 87},
                                         .corner_radii = {3, 1, 4, 2},
                                         .custom_corners = true,
                                         .custom_border_edges = true,
                                         .border_edges = {1, 2, 1.5f, 0.5f}});
    all[n++] = legacy((cui_draw_command){.type = CUI_DRAW_TEXT,
                                         .bounds = {3, 5, NAN, NAN},
                                         .text = cui_string("fi e\xcc\x81"),
                                         .font_size = 11,
                                         .color = {200, 230, 90, 151}});
    if (info.text_features & CUI_SDL3_V2_TEXT_CONTEXT) {
        all[n] = all[n - 1];
        all[n].payload.legacy.bounds.y = 12;
        all[n].payload.legacy.text_source =
            (cui_text_line_source){cui_string("fi e\xcc\x81"), 0, 6};
        all[n].payload.legacy.text = (cui_str){(const char *)(uintptr_t)1, SIZE_MAX};
        all[n].payload.legacy.text_colors = spans;
        all[n++].payload.legacy.text_color_count = 1;
    }
    all[n] = all[8];
    all[n].payload.legacy.bounds.x = 9;
    all[n].payload.legacy.text_paint = (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                                                        .x1 = 2,
                                                        .y1 = 3,
                                                        .x2 = 24,
                                                        .y2 = 18,
                                                        .stops = stops,
                                                        .stop_count = 2};
    ++n;
    all[n++] = legacy((cui_draw_command){.type = CUI_DRAW_CLIP_POP});
    list = make_list(all, n);
    list.required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS;
    compare_upload(s, cpu, &list, info.defaults);
    cui_render_v2_options anisotropic = info.defaults;
    anisotropic.scale_x = 0.75f;
    anisotropic.scale_y = 1.5f;
    anisotropic.origin_x = 1.25f;
    anisotropic.origin_y = -0.5f;
    compare_upload(s, cpu, &list, anisotropic);
    anisotropic.scale_x = 64;
    anisotropic.scale_y = 1.0f / 64;
    compare_upload(s, cpu, &list, anisotropic);
    cui_software_destroy(cpu);
    /* All preparation/native failures restore state, suppress Present and permit retry.
     * Drawing before a failed Flush may already have happened; no rollback claim. */
    list = make_list(&c, 1);
    active_list = &list;
    assert(SDL_SetRenderLogicalPresentation(s->renderer, 16, 12, SDL_LOGICAL_PRESENTATION_STRETCH));
    assert(SDL_SetRenderScale(s->renderer, 1.25f, 0.75f));
    SDL_Rect viewport = {1, 2, 20, 17}, clip = {2, 1, 10, 9};
    assert(SDL_SetRenderViewport(s->renderer, &viewport) &&
           SDL_SetRenderClipRect(s->renderer, &clip));
    assert(SDL_SetRenderColorScale(s->renderer, 0.7f));
    sdl_v2_state saved;
    assert(sdl_v2_snapshot(s->renderer, &saved));
    int failures[] = {FAIL_SIZE,  FAIL_CREATE,  FAIL_UPLOAD,   FAIL_DRAW,
                      FAIL_FLUSH, FAIL_PRESENT, FAIL_SNAPSHOT, FAIL_LOGICAL};
    for (size_t i = 0; i < sizeof failures / sizeof *failures; ++i) {
        failure = failures[i];
        assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
        assert(!failure && !s->v2_busy && !s->v2_faulted && !live);
        state_equal(s->renderer, &saved);
        assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_OK);
    }
    for (size_t k = 1; k <= 2; ++k) {
        fail_allocation = allocation_calls + k;
        assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_CAPACITY);
        assert(!s->v2_busy && !live);
        state_equal(s->renderer, &saved);
        fail_allocation = 0;
        assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_OK);
    }
    failure = FAIL_RESTORE;
    assert(cui_sdl3_render_v2(s, &list, (cui_color){0}, NULL) == CUI_ERROR_INVALID_ACTION);
    assert(s->v2_faulted && !s->v2_busy && !live);
    assert(cui_sdl3_query_v2(s, &info) == CUI_ERROR_LIFECYCLE);
    assert(!cui_sdl3_render(s, (const cui_draw_list *)(uintptr_t)1, (cui_color){0}));
    cui_sdl3_destroy(s);
    active = NULL;
    /* Frozen V1 same-runtime authority: caches, allocation count and pixels. */
    cui_draw_command old_commands[] = {c.payload.legacy,
                                       {.type = CUI_DRAW_TEXT,
                                        .bounds = {5, 8, 20, 12},
                                        .font_size = 11,
                                        .text = {"fi", 2},
                                        .color = {200, 90, 30, 191}}};
    sdl_original_result original;
    assert(sdl_original_run(font, old_commands, 2, (cui_color){4, 8, 12, 255}, &original));
    object_calls = object_bytes = 0;
    s = cui_sdl3_create_with_options("candidate SDL V1", 32, 24, font,
                                     (cui_sdl3_window_options){true});
    assert(s && object_calls == 1 && original.object_calls == 1);
    assert(object_bytes == native_object_bytes && object_bytes >= original.object_bytes &&
           object_bytes - original.object_bytes <= _Alignof(cui_sdl3));
    cui_draw_list old_list = {old_commands, 2, 32, 24};
    assert(cui_sdl3_render(s, &old_list, (cui_color){4, 8, 12, 255}));
    assert(cui_sdl3_render(s, &old_list, (cui_color){4, 8, 12, 255}));
    assert(!memcmp(original_pixels, captured, sizeof captured));
    cui_sdl3_cache_stats stats = cui_sdl3_glyph_cache_stats(s);
    assert(stats.entries == original.cache.entries &&
           stats.coverage_bytes == original.cache.coverage_bytes &&
           stats.hits == original.cache.hits && stats.misses == original.cache.misses);
    cui_sdl3_destroy(s);
    assert(!live);
    cui_font_close(font);
    puts("SDL V2 all-command transport, failure/reentry/alias and frozen V1 gates passed");
    return 0;
}
