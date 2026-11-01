/* SPDX-License-Identifier: MIT */
/* Immutable pre-V2 SDL source compiled separately; only symbols/include paths vary. */
#include <SDL3/SDL.h>
#include <stdlib.h>
static size_t original_bytes, original_calls;
static void *original_calloc(size_t n, size_t bytes) {
    original_bytes = n * bytes;
    ++original_calls;
    return calloc(n, bytes);
}
extern bool sdl_v2_capture(SDL_Renderer *, bool);
static bool original_present(SDL_Renderer *r) {
    return sdl_v2_capture(r, true) && SDL_RenderPresent(r);
}
#define cui_sdl3 original_cui_sdl3
#define cui_sdl3_create original_cui_sdl3_create
#define cui_sdl3_create_with_options original_cui_sdl3_create_with_options
#define cui_sdl3_set_visible original_cui_sdl3_set_visible
#define cui_sdl3_is_visible original_cui_sdl3_is_visible
#define cui_sdl3_destroy original_cui_sdl3_destroy
#define cui_sdl3_appearance original_cui_sdl3_appearance
#define cui_sdl3_poll original_cui_sdl3_poll
#define cui_sdl3_run original_cui_sdl3_run
#define cui_sdl3_set_cursor original_cui_sdl3_set_cursor
#define cui_sdl3_size original_cui_sdl3_size
#define cui_sdl3_accessibility_window original_cui_sdl3_accessibility_window
#define cui_sdl3_render original_cui_sdl3_render
#define cui_sdl3_render_capture original_cui_sdl3_render_capture
#define cui_sdl3_save_bmp original_cui_sdl3_save_bmp
#define cui_sdl3_driver original_cui_sdl3_driver
#define cui_sdl3_video_driver original_cui_sdl3_video_driver
#define cui_sdl3_error original_cui_sdl3_error
#define cui_sdl3_glyph_cache_stats original_cui_sdl3_glyph_cache_stats
#define cui_sdl3_clipboard_interface original_cui_sdl3_clipboard_interface
#define cui_sdl3_window_interface original_cui_sdl3_window_interface
#define cui_sdl3_set_decorated original_cui_sdl3_set_decorated
#define cui_sdl3_frame_delta original_cui_sdl3_frame_delta
#define cui_sdl3_wait_events original_cui_sdl3_wait_events
#define cui_sdl3_wait original_cui_sdl3_wait
#define cui_sdl3_interface original_cui_sdl3_interface
#define calloc original_calloc
#define SDL_RenderPresent original_present
#include CUI_SDL_V1_FROZEN_HEADER
#undef SDL_RenderPresent
#undef calloc
#undef cui_sdl3
#undef cui_sdl3_create
#undef cui_sdl3_create_with_options
#undef cui_sdl3_set_visible
#undef cui_sdl3_is_visible
#undef cui_sdl3_destroy
#undef cui_sdl3_appearance
#undef cui_sdl3_poll
#undef cui_sdl3_run
#undef cui_sdl3_set_cursor
#undef cui_sdl3_size
#undef cui_sdl3_accessibility_window
#undef cui_sdl3_render
#undef cui_sdl3_render_capture
#undef cui_sdl3_save_bmp
#undef cui_sdl3_driver
#undef cui_sdl3_video_driver
#undef cui_sdl3_error
#undef cui_sdl3_glyph_cache_stats
#undef cui_sdl3_clipboard_interface
#undef cui_sdl3_window_interface
#undef cui_sdl3_set_decorated
#undef cui_sdl3_frame_delta
#undef cui_sdl3_wait_events
#undef cui_sdl3_wait
#undef cui_sdl3_interface
typedef struct {
    size_t object_bytes, object_calls;
    cui_sdl3_cache_stats cache;
} sdl_original_result;
bool sdl_original_run(cui_font *font, const cui_draw_command *commands, size_t count,
                      cui_color clear, sdl_original_result *out) {
    original_calls = original_bytes = 0;
    original_cui_sdl3 *s = original_cui_sdl3_create_with_options("frozen SDL V1", 32, 24, font,
                                                                 (cui_sdl3_window_options){true});
    if (!s)
        return false;
    cui_draw_list list = {commands, count, 32, 24};
    bool ok =
        original_cui_sdl3_render(s, &list, clear) && original_cui_sdl3_render(s, &list, clear);
    *out = (sdl_original_result){original_bytes, original_calls,
                                 original_cui_sdl3_glyph_cache_stats(s)};
    original_cui_sdl3_destroy(s);
    return ok;
}
