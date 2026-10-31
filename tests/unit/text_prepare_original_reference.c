/* SPDX-License-Identifier: MIT */
#include "../fixtures/text_prepare_v1/prefix.h"
#include "../fixtures/text_prepare_v1/source/backends/software/cui_software.c"
#include "../fixtures/text_prepare_v1/source/src/text/cui_text.c"
#include "text_prepare_fixture_internal.h"
#include <assert.h>
bool cui_test_original_text_prepare(const char *regular, const char *bold_path, unsigned char *out,
                                    size_t capacity, size_t *out_length) {
    cui_font *font = cui_font_open(regular), *bold = cui_font_open(bold_path);
    assert(font && bold);
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
