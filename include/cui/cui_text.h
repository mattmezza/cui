/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_H
#define CUI_TEXT_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_font cui_font;
typedef void (*cui_glyph_fn)(void *user,int x,int y,int width,int height,int pitch,const unsigned char *coverage);
/* The font file remains caller-owned. Face operations are UI-thread confined. */
cui_font *cui_font_open(const char *path);
void cui_font_close(cui_font *font);
cui_size cui_font_measure(void *font,cui_str text,float size);
bool cui_font_draw(cui_font *font,cui_str text,float size,float x,float y,cui_glyph_fn emit,void *user);
#ifdef __cplusplus
}
#endif
#endif
