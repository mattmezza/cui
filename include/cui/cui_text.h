/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_H
#define CUI_TEXT_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_font cui_font;
typedef void (*cui_glyph_fn)(void *user, int x, int y, int width, int height, int pitch,
                             const unsigned char *coverage);
/* The font file remains caller-owned. Face operations are UI-thread confined.
 */
cui_font *cui_font_open(const char *path);
void cui_font_close(cui_font *font);
/* Borrow an actual face opened by the caller; pass NULL to remove a variant.
 * Keep variants alive through measurement/drawing. Unregistered styles use
 * the regular face for both operations. */
bool cui_font_set_style(cui_font *font, cui_text_style style, cui_font *variant);
/* Convenience path registration owns the opened face. Failure preserves the
 * previous registration; replacement and font_close release owned faces. */
bool cui_font_register_style(cui_font *font, cui_text_style style, const char *path);
cui_size cui_font_measure(void *font, cui_str text, float size);
cui_size cui_font_measure_styled(void *font, cui_str text, float size, cui_text_style style);
bool cui_font_draw(cui_font *font, cui_str text, float size, float x, float y, cui_glyph_fn emit,
                   void *user);
bool cui_font_draw_styled(cui_font *font, cui_str text, float size, cui_text_style style, float x,
                          float y, cui_glyph_fn emit, void *user);
#ifdef __cplusplus
}
#endif
#endif
