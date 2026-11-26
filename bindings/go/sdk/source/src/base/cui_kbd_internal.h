/* SPDX-License-Identifier: MIT */
#ifndef CUI_KBD_INTERNAL_H
#define CUI_KBD_INTERNAL_H
#include "cui/cui.h"

enum { CUI_KBD_NAME_MAX_BYTES = 1024 };

static inline bool cui_kbd_line_valid(cui_str text) {
    if (!cui_text_line_source_valid(
            (cui_text_line_source){.paragraph = text, .byte_end = text.length}))
        return false;
    for (size_t i = 0; i < text.length; ++i) {
        unsigned char c = (unsigned char)text.data[i];
        if (c < 32 || c == 127 ||
            (i + 2 < text.length && c == 0xe2 && (unsigned char)text.data[i + 1] == 0x80 &&
             (unsigned char)text.data[i + 2] == 0xa8))
            return false;
    }
    return true;
}

#endif
