/* SPDX-License-Identifier: MIT */
/* Compile the implementation here to count the actual raster calls without
 * exposing test-only counters in the public font API. */
#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static int fail_bitmap_allocation;
static void *cache_malloc(size_t bytes) {
    return fail_bitmap_allocation ? NULL : malloc(bytes);
}
static unsigned loads, renders;
static FT_Error counted_load(FT_Face face, FT_UInt glyph, FT_Int32 flags) {
    ++loads;
    return FT_Load_Glyph(face, glyph, flags);
}
static FT_Error counted_render(FT_GlyphSlot slot, FT_Render_Mode mode) {
    ++renders;
    return FT_Render_Glyph(slot, mode);
}
#define FT_Load_Glyph counted_load
#define FT_Render_Glyph counted_render
#define malloc cache_malloc
#include "../../src/text/cui_text.c"
#undef malloc
#undef FT_Load_Glyph
#undef FT_Render_Glyph
static uint64_t bitmap_hash(FT_GlyphSlot slot) {
    uint64_t hash = (uint64_t)slot->bitmap_left + 31u * (uint64_t)slot->bitmap_top;
    hash = hash * 33u + slot->bitmap.width;
    hash = hash * 33u + slot->bitmap.rows;
    for (unsigned y = 0; y < slot->bitmap.rows; ++y)
        for (unsigned x = 0; x < slot->bitmap.width; ++x)
            hash = hash * 33u + slot->bitmap.buffer[y * (unsigned)slot->bitmap.pitch + x];
    return hash;
}
static FT_GlyphSlot raster(cui_font *font, FT_UInt glyph, float size) {
    assert(!FT_Set_Char_Size(font->face, 0, (FT_F26Dot6)lroundf(size * 64), 72, 72));
    FT_GlyphSlot slot = raster_glyph(font, glyph, size);
    assert(slot);
    return slot;
}
static uint64_t emitted_hash;
static void emit(void *user, int x, int y, int w, int h, int pitch, const unsigned char *pixels) {
    emitted_hash = emitted_hash * 33u + (unsigned)x;
    emitted_hash = emitted_hash * 33u + (unsigned)y;
    emitted_hash = emitted_hash * 33u + (unsigned)w;
    emitted_hash = emitted_hash * 33u + (unsigned)h;
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
            emitted_hash = emitted_hash * 33u + pixels[row * pitch + col];
    ++*(unsigned *)user;
}
#ifdef CUI_TEXT_BIDI
static void colored_emit(void *user, int x, int y, int w, int h, int pitch,
                         const unsigned char *pixels, cui_color color) {
    (void)color;
    emit(user, x, y, w, h, pitch, pixels);
}
#endif
int main(void) {
    const char *path = getenv("CUI_TEST_FONT");
    cui_font *font = path ? cui_font_open(path) : NULL;
    const char *const defaults[] = {"/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
    if (!path)
        for (size_t i = 0; i < sizeof defaults / sizeof defaults[0]; ++i)
            if ((font = cui_font_open(defaults[i]))) {
                path = defaults[i];
                break;
            }
    cui_font *other = cui_font_open(path);
    if (!font || !other) {
        cui_font_close(font);
        cui_font_close(other);
        return 77;
    }
    FT_UInt glyph = FT_Get_Char_Index(font->face, 'W');
    uint64_t original = bitmap_hash(raster(font, glyph, 24));
    assert(loads == 1 && renders == 1);
    assert(bitmap_hash(raster(font, glyph, 24)) == original);
    assert(loads == 1 && renders == 1);
    /* A different face must never share bitmap storage or identity. */
    assert(bitmap_hash(raster(other, glyph, 24)) == original);
    assert(loads == 2 && renders == 2);
    assert(bitmap_hash(raster(font, glyph, 25)) != original);
    assert(loads == 3 && renders == 3);
    /* 2 pixels add 128 in 26.6 and collide with the 24px entry. */
    raster(font, glyph, 26);
    assert(loads == 4 && renders == 4);
    assert(bitmap_hash(raster(font, glyph, 24)) == original);
    assert(loads == 5 && renders == 5);
    fail_bitmap_allocation = 1;
    uint64_t failed_allocation = bitmap_hash(raster(font, glyph, 27));
    unsigned uncached = renders;
    assert(bitmap_hash(raster(font, glyph, 27)) == failed_allocation);
    assert(renders == uncached + 1);
    fail_bitmap_allocation = 0;
    assert(bitmap_hash(raster(font, glyph, 27)) == failed_allocation);
    uncached = renders;
    assert(bitmap_hash(raster(font, glyph, 27)) == failed_allocation);
    assert(renders == uncached);
    /* A measurement can change the face size; drawing restores its own size. */
    unsigned emitted = 0;
    assert(cui_font_draw(font, cui_string("WWWW"), 24, 0, 0, emit, &emitted));
    unsigned warm = renders;
    unsigned warm_loads = loads;
    cui_font_measure(font, cui_string("W"), 35);
    assert(cui_font_draw(font, cui_string("WWWW"), 24, 2, 4, emit, &emitted));
    assert(renders == warm && loads == warm_loads && emitted == 8);
    printf("warm four-glyph draw: %u FT_Load_Glyph and %u FT_Render_Glyph calls\n",
           loads - warm_loads, renders - warm);
    /* Style selection uses the selected face's cache, even for equal glyph IDs. */
    cui_text_style bold = {.weight = CUI_TEXT_WEIGHT_BOLD};
    assert(cui_font_set_style(font, bold, other));
    assert(cui_font_draw_styled(font, cui_string("W"), 24, bold, 0, 0, emit, &emitted));
    assert(renders == warm);
#ifdef CUI_TEXT_BIDI
    cui_text_layout *public_layout = NULL;
    assert(cui_font_layout_line(font, cui_string("WWWW"), 24, (cui_text_style){0},
                                CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL,
                                &public_layout) == CUI_TEXT_LAYOUT_OK);
    warm = renders;
    emitted_hash = 0;
    assert(cui_text_layout_draw(public_layout, 0, 0, emit, &emitted));
    uint64_t layout_hash = emitted_hash;
    emitted_hash = 0;
    assert(cui_text_layout_draw(public_layout, 0, 0, emit, &emitted));
    assert(emitted_hash == layout_hash);
    assert(renders == warm);
    cui_text_layout_destroy(public_layout);
    cui_text_line_source source = {cui_string("WWWW"), 0, 4};
    emitted_hash = 0;
    assert(cui_font_draw_context_line_colored(font, source, 24, (cui_text_style){0}, 0, 0,
                                              (cui_color){0}, NULL, 0, colored_emit, &emitted));
    assert(renders == warm);
    assert(emitted_hash == layout_hash);
    /* Synthetic fallback runs isolate face switching from installed coverage. */
    cui_text_layout *layout = calloc(1, sizeof *layout);
    assert(layout);
    layout->size = 24;
    layout->faces = malloc(2 * sizeof *layout->faces);
    layout->runs = calloc(2, sizeof *layout->runs);
    layout->glyphs = calloc(2, sizeof *layout->glyphs);
    assert(layout->faces && layout->runs && layout->glyphs);
    layout->faces[0] = font;
    layout->faces[1] = other;
    ++font->references;
    ++other->references;
    layout->view.face_count = layout->view.run_count = layout->view.glyph_count = 2;
    layout->runs[0].glyph_count = 1;
    layout->runs[1].glyph_start = 1;
    layout->runs[1].glyph_count = 1;
    layout->runs[1].face_index = 1;
    layout->glyphs[0].glyph_id = layout->glyphs[1].glyph_id = glyph;
    warm = renders;
    assert(cui_text_layout_draw(layout, 0, 0, emit, &emitted));
    assert(renders == warm);
    cui_text_layout_destroy(layout);
#endif
    /* Large glyphs bypass the memory bound while preserving raster output. */
    uint64_t large = bitmap_hash(raster(font, glyph, 512));
    warm = renders;
    assert(bitmap_hash(raster(font, glyph, 512)) == large);
    assert(renders == warm + 1);
    size_t bytes = 0;
    for (size_t i = 0; i < GLYPH_CACHE_SLOTS; ++i)
        if (font->glyph_cache[i].valid)
            bytes += (size_t)font->glyph_cache[i].slot.bitmap.pitch *
                     font->glyph_cache[i].slot.bitmap.rows;
    assert(bytes <= GLYPH_CACHE_SLOTS * GLYPH_CACHE_BITMAP_MAX);
    cui_font_close(font);
    cui_font_close(other);
    puts("raster cache reuse, size/face isolation, collision and oversized bypass passed");
    return 0;
}
