/* SPDX-License-Identifier: MIT */
#include "../fixtures/text_prepare_v1/prefix.h"
#include "../fixtures/text_prepare_v1/source/backends/software/cui_software.c"
#include "../fixtures/text_prepare_v1/source/src/text/cui_text.c"
#include "text_prepare_fixture_internal.h"
#include <assert.h>
#ifdef CUI_TEXT_BIDI
/* Runtime-derived address remains unreadable without O3 bounds constant propagation. */
static uintptr_t poison_address(void) {
    static volatile uintptr_t address = 1;
    return address;
}
#endif
bool cui_test_original_text_prepare(const char *regular, const char *bold_path, unsigned char *out,
                                    size_t capacity, size_t *out_length) {
    cui_font *font = cui_font_open(regular), *bold = cui_font_open(bold_path);
    assert(font && bold);
#ifdef CUI_TEXT_BIDI
    /* Immutable original source selects context by pointer, even when empty. */
    length = 0;
    assert(cui_font_draw_context_line_scaled(
        font, (cui_str){(const char *)poison_address(), SIZE_MAX},
        (cui_text_line_source){{"", 0}, 0, 0}, 12, (cui_text_style){0}, 0, 0, 1, record, NULL));
    assert(length == 0);
#endif
    assert(cui_font_set_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD}, bold));
    baseline(font);
    assert(length <= capacity);
    memcpy(out, result, length);
    *out_length = length;
    cui_font_close(font);
    cui_font_close(bold);
    return true;
}
bool cui_test_original_fallback(const char *regular, const char *fallback_path,
                                const cui_draw_command *command, unsigned char *out,
                                size_t capacity, size_t *out_length) {
    cui_font *font = cui_font_open(regular), *fallback = cui_font_open(fallback_path);
    assert(font && fallback);
    cui_font *chain[] = {fallback};
    assert(cui_font_set_fallbacks(font, chain, 1));
    length = 0;
    bool ok = cui_font_draw_context_line_scaled(
        font, command->text, command->text_source, command->font_size, command->text_style,
        command->bounds.x, command->bounds.y, 1, record, NULL);
    if (ok) {
        assert(length <= capacity);
        memcpy(out, result, length);
        *out_length = length;
    }
    cui_font_close(font);
    cui_font_close(fallback);
    return ok;
}
/* Arbitrary <=16 density oracle uses immutable original source above. This does
 * not regenerate any durable expected fixture from the candidate. */
bool cui_test_original_density(const char *regular, const char *bold_path,
                               const cui_draw_command *c, float density, unsigned char *out,
                               size_t capacity, size_t *out_length) {
    cui_font *font = cui_font_open(regular), *bold = cui_font_open(bold_path);
    assert(font && bold);
    assert(cui_font_set_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD}, bold));
    length = 0;
    bool ok = c->text_color_count
                  ? cui_font_draw_context_line_colored_scaled(
                        font, c->text_source, c->font_size, c->text_style, c->bounds.x, c->bounds.y,
                        c->color, c->text_colors, c->text_color_count, density, colored, NULL)
                  : cui_font_draw_context_line_scaled(font, c->text, c->text_source, c->font_size,
                                                      c->text_style, c->bounds.x, c->bounds.y,
                                                      density, record, NULL);
    if (ok) {
        assert(length <= capacity);
        memcpy(out, result, length);
        *out_length = length;
    } else
        *out_length = 0;
    cui_font_close(font);
    cui_font_close(bold);
    return ok;
}
