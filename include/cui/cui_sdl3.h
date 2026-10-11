/* SPDX-License-Identifier: MIT */
#ifndef CUI_SDL3_H
#define CUI_SDL3_H
#include "cui_platform.h"
#include "cui_renderer.h"
#include "cui_text.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_sdl3 cui_sdl3;
typedef struct {
    size_t entries, coverage_bytes;
    uint64_t hits, misses;
} cui_sdl3_cache_stats;
/* Optional SDL3 adapter; all operations must run on the main thread. */
cui_sdl3 *cui_sdl3_create(const char *title, int width, int height, cui_font *font);
void cui_sdl3_destroy(cui_sdl3 *backend);
bool cui_sdl3_poll(cui_sdl3 *backend, cui_context *ui);
void cui_sdl3_size(cui_sdl3 *backend, float *width, float *height);
bool cui_sdl3_render(cui_sdl3 *backend, const cui_draw_list *list, cui_color clear);
/* Capture before presenting, when the framebuffer contents are defined. */
bool cui_sdl3_render_capture(cui_sdl3 *backend, const cui_draw_list *list, cui_color clear,
                             const char *path);
bool cui_sdl3_save_bmp(cui_sdl3 *backend, const char *path);
const char *cui_sdl3_driver(cui_sdl3 *backend);
const char *cui_sdl3_video_driver(void);
const char *cui_sdl3_error(void);
cui_sdl3_cache_stats cui_sdl3_glyph_cache_stats(cui_sdl3 *backend);
/* Clipboard-only services. get() storage lasts until next get() or destruction.
 * Native input dispatch remains cui_sdl3_poll(); no IME composition capability.
 */
cui_platform cui_sdl3_clipboard_interface(cui_sdl3 *backend);
void cui_sdl3_wait(unsigned milliseconds);
cui_renderer cui_sdl3_interface(cui_sdl3 *backend);
#ifdef __cplusplus
}
#endif
#endif
