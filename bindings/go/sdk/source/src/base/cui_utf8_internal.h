/* SPDX-License-Identifier: MIT */
#ifndef CUI_UTF8_INTERNAL_H
#define CUI_UTF8_INTERNAL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline bool cui_utf8_decode(const unsigned char *s, size_t n, size_t *width,
                                   uint32_t *scalar) {
    if (!n)
        return false;
    unsigned char first = s[0];
    size_t count;
    uint32_t cp;
    if (first < 0x80) {
        count = 1;
        cp = first;
    } else if (first >= 0xc2 && first <= 0xdf) {
        count = 2;
        cp = first & 31u;
    } else if (first >= 0xe0 && first <= 0xef) {
        count = 3;
        cp = first & 15u;
    } else if (first >= 0xf0 && first <= 0xf4) {
        count = 4;
        cp = first & 7u;
    } else
        return false;
    if (n < count)
        return false;
    for (size_t i = 1; i < count; ++i) {
        if ((s[i] & 0xc0u) != 0x80u)
            return false;
        cp = (cp << 6) | (s[i] & 63u);
    }
    if ((count == 3 && cp < 0x800) || (count == 4 && cp < 0x10000) || cp > 0x10ffff ||
        (cp >= 0xd800 && cp <= 0xdfff))
        return false;
    *width = count;
    if (scalar)
        *scalar = cp;
    return true;
}
#endif
