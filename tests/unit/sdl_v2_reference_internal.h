/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_SDL_V2_REFERENCE_H
#define CUI_TEST_SDL_V2_REFERENCE_H
#include "cui/cui_sdl3.h"
typedef struct {
    size_t object_bytes, object_calls;
    cui_sdl3_cache_stats cache;
} sdl_original_result;
bool sdl_original_run(cui_font *, const cui_draw_command *, size_t, cui_color,
                      sdl_original_result *);
#endif
