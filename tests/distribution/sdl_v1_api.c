/* SPDX-License-Identifier: MIT */
/* Frozen public SDL V1 record/function ABI; no SDL headers or native calls. */
#include "cui/cui_sdl3.h"
#include <stddef.h>
typedef struct {
    size_t entries, coverage_bytes;
    uint64_t hits, misses;
} frozen_cache_stats;
typedef struct {
    bool initially_hidden;
} frozen_window_options;
typedef struct {
    void *user;
    bool (*init)(void *, int, char **, cui_sdl3 **, cui_context **);
    int (*frame)(void *, cui_sdl3 *, cui_context *);
    void (*quit)(void *, bool);
} frozen_app;
typedef struct {
    void *user;
    const char *name;
    unsigned capabilities;
    bool (*render)(void *, const cui_draw_list *, cui_color);
    void (*destroy)(void *);
} frozen_renderer;
#define RECORD(actual, old)                                                                        \
    _Static_assert(sizeof(actual) == sizeof(old), #actual " V1 size changed");                     \
    _Static_assert(_Alignof(actual) == _Alignof(old), #actual " V1 alignment changed")
#define FIELD(actual, old, field)                                                                  \
    _Static_assert(offsetof(actual, field) == offsetof(old, field), #actual "." #field " moved")
RECORD(cui_sdl3_cache_stats, frozen_cache_stats);
FIELD(cui_sdl3_cache_stats, frozen_cache_stats, entries);
FIELD(cui_sdl3_cache_stats, frozen_cache_stats, coverage_bytes);
FIELD(cui_sdl3_cache_stats, frozen_cache_stats, hits);
FIELD(cui_sdl3_cache_stats, frozen_cache_stats, misses);
RECORD(cui_sdl3_window_options, frozen_window_options);
FIELD(cui_sdl3_window_options, frozen_window_options, initially_hidden);
RECORD(cui_sdl3_app, frozen_app);
FIELD(cui_sdl3_app, frozen_app, user);
FIELD(cui_sdl3_app, frozen_app, init);
FIELD(cui_sdl3_app, frozen_app, frame);
FIELD(cui_sdl3_app, frozen_app, quit);
RECORD(cui_renderer, frozen_renderer);
FIELD(cui_renderer, frozen_renderer, user);
FIELD(cui_renderer, frozen_renderer, name);
FIELD(cui_renderer, frozen_renderer, capabilities);
FIELD(cui_renderer, frozen_renderer, render);
FIELD(cui_renderer, frozen_renderer, destroy);
#define SIGNATURE(function, ...)                                                                   \
    _Static_assert(_Generic(&(function), __VA_ARGS__: 1, default: 0),                              \
                   #function " V1 signature changed")
SIGNATURE(cui_sdl3_create, cui_sdl3 *(*)(const char *, int, int, cui_font *));
SIGNATURE(cui_sdl3_create_with_options,
          cui_sdl3 *(*)(const char *, int, int, cui_font *, cui_sdl3_window_options));
SIGNATURE(cui_sdl3_destroy, void (*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_set_visible, bool (*)(cui_sdl3 *, bool));
SIGNATURE(cui_sdl3_is_visible, bool (*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_appearance, bool (*)(cui_sdl3 *, cui_platform_appearance *));
SIGNATURE(cui_sdl3_poll, bool (*)(cui_sdl3 *, cui_context *));
SIGNATURE(cui_sdl3_set_cursor, bool (*)(cui_sdl3 *, cui_cursor));
SIGNATURE(cui_sdl3_size, void (*)(cui_sdl3 *, float *, float *));
SIGNATURE(cui_sdl3_accessibility_window, bool (*)(cui_sdl3 *, cui_accessibility_window *));
SIGNATURE(cui_sdl3_render, bool (*)(cui_sdl3 *, const cui_draw_list *, cui_color));
SIGNATURE(cui_sdl3_render_capture,
          bool (*)(cui_sdl3 *, const cui_draw_list *, cui_color, const char *));
SIGNATURE(cui_sdl3_save_bmp, bool (*)(cui_sdl3 *, const char *));
SIGNATURE(cui_sdl3_driver, const char *(*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_video_driver, const char *(*)(void));
SIGNATURE(cui_sdl3_error, const char *(*)(void));
SIGNATURE(cui_sdl3_glyph_cache_stats, cui_sdl3_cache_stats (*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_clipboard_interface, cui_platform (*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_window_interface, cui_window_host (*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_set_decorated, bool (*)(cui_sdl3 *, bool));
SIGNATURE(cui_sdl3_frame_delta, float (*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_wait_events, bool (*)(cui_sdl3 *, int));
SIGNATURE(cui_sdl3_wait, void (*)(unsigned));
SIGNATURE(cui_sdl3_interface, cui_renderer (*)(cui_sdl3 *));
SIGNATURE(cui_sdl3_run, int (*)(int, char **, cui_sdl3_app));
int main(void) {
    return 0;
}
