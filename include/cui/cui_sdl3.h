/* SPDX-License-Identifier: MIT */
#ifndef CUI_SDL3_H
#define CUI_SDL3_H
#include "cui_renderer.h"
#include "cui_text.h"
typedef struct cui_sdl3 cui_sdl3;
/* Optional SDL3 adapter; all operations must run on the main thread. */
cui_sdl3 *cui_sdl3_create(const char *title,int width,int height,cui_font *font);
void cui_sdl3_destroy(cui_sdl3 *backend);
bool cui_sdl3_poll(cui_sdl3 *backend,cui_context *ui);
void cui_sdl3_size(cui_sdl3 *backend,float *width,float *height);
bool cui_sdl3_render(cui_sdl3 *backend,const cui_draw_list *list,cui_color clear);
/* Capture before presenting, when the framebuffer contents are defined. */
bool cui_sdl3_render_capture(cui_sdl3 *backend,const cui_draw_list *list,cui_color clear,const char *path);
bool cui_sdl3_save_bmp(cui_sdl3 *backend,const char *path);
const char *cui_sdl3_driver(cui_sdl3 *backend);
const char *cui_sdl3_video_driver(void);
const char *cui_sdl3_error(void);
void cui_sdl3_wait(unsigned milliseconds);
cui_renderer cui_sdl3_interface(cui_sdl3 *backend);
#endif
