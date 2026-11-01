/* SPDX-License-Identifier: MIT */
#include "cui/cui_time_text.h"
#include <string.h>

static const cui_time_locale_v1 cui_tt_ascii = {.struct_size = sizeof(cui_time_locale_v1),
                                                .version = 1,
                                                .digits = {{"0", 1},
                                                           {"1", 1},
                                                           {"2", 1},
                                                           {"3", 1},
                                                           {"4", 1},
                                                           {"5", 1},
                                                           {"6", 1},
                                                           {"7", 1},
                                                           {"8", 1},
                                                           {"9", 1}},
                                                .separator = {":", 1},
                                                .period_separator = {" ", 1},
                                                .am = {"AM", 2},
                                                .pm = {"PM", 2}};
static bool cui_tt_utf8(cui_str s) {
    size_t i = 0;
    while (i < s.length) {
        unsigned c = (unsigned char)s.data[i++], n, value, minimum;
        if (!c)
            return false;
        if (c < 128)
            continue;
        if (c >= 0xc2 && c <= 0xdf) {
            n = 1;
            value = c & 31;
            minimum = 0x80;
        } else if (c >= 0xe0 && c <= 0xef) {
            n = 2;
            value = c & 15;
            minimum = 0x800;
        } else if (c >= 0xf0 && c <= 0xf4) {
            n = 3;
            value = c & 7;
            minimum = 0x10000;
        } else
            return false;
        if (n > s.length - i)
            return false;
        while (n--) {
            c = (unsigned char)s.data[i++];
            if ((c & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (c & 63);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
static cui_str cui_tt_token(const cui_time_locale_v1 *l, unsigned i) {
    if (i < 10)
        return l->digits[i];
    if (i == 10)
        return l->separator;
    if (i == 11)
        return l->period_separator;
    return i == 12 ? l->am : l->pm;
}
static bool cui_tt_valid(cui_time t) {
    return t.hour < 24 && t.minute < 60 && t.second < 60;
}
static uint32_t cui_tt_seconds(cui_time t) {
    return (uint32_t)t.hour * UINT32_C(3600) + (uint32_t)t.minute * UINT32_C(60) + t.second;
}
static bool cui_tt_config(const cui_time_text_options_v1 *o, const cui_time_locale_v1 **locale) {
    if (o->struct_size != sizeof(*o) || o->version != 1 || o->cycle > 1 || o->precision > 1 ||
        (o->flags & ~3u) || o->reserved[0] || o->reserved[1] || o->reserved[2])
        return false;
    const cui_time_locale_v1 *l = o->locale ? o->locale : &cui_tt_ascii;
    if (l->struct_size != sizeof(*l) || l->version != 1 || l->reserved[0] || l->reserved[1])
        return false;
    for (unsigned i = 0; i < 14; i++) {
        cui_str a = cui_tt_token(l, i);
        if (!a.data || !a.length || a.length > CUI_TIME_TOKEN_MAX_BYTES_V1 || !cui_tt_utf8(a))
            return false;
        for (unsigned j = 0; j < i; j++) {
            cui_str b = cui_tt_token(l, j);
            size_t n = a.length < b.length ? a.length : b.length;
            if (!memcmp(a.data, b.data, n))
                return false;
        }
    }
    if ((o->flags & 1) && (!cui_tt_valid(o->minimum) || (!o->precision && o->minimum.second)))
        return false;
    if ((o->flags & 2) && (!cui_tt_valid(o->maximum) || (!o->precision && o->maximum.second)))
        return false;
    if ((o->flags & 3) == 3 && cui_tt_seconds(o->minimum) > cui_tt_seconds(o->maximum))
        return false;
    *locale = l;
    return true;
}
static cui_time_text_status_v1 cui_tt_value(cui_time t, const cui_time_text_options_v1 *o) {
    if (!cui_tt_valid(t) || (!o->precision && t.second))
        return CUI_TIME_TEXT_INVALID_VALUE;
    uint32_t n = cui_tt_seconds(t);
    if (((o->flags & 1) && n < cui_tt_seconds(o->minimum)) ||
        ((o->flags & 2) && n > cui_tt_seconds(o->maximum)))
        return CUI_TIME_TEXT_OUT_OF_RANGE;
    return CUI_TIME_TEXT_OK;
}
cui_time_text_status_v1 cui_time_locale_init_v1(cui_time_locale_v1 *out) {
    if (!out)
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    *out = cui_tt_ascii;
    return CUI_TIME_TEXT_OK;
}
cui_time_text_status_v1 cui_time_text_options_init_v1(cui_time_text_options_v1 *out) {
    if (!out)
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    *out = (cui_time_text_options_v1){.struct_size = sizeof(*out), .version = 1};
    return CUI_TIME_TEXT_OK;
}
cui_time_text_status_v1 cui_time_validate_v1(cui_time value, const cui_time_text_options_v1 *o) {
    const cui_time_locale_v1 *l;
    if (!o)
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    if (!cui_tt_config(o, &l))
        return CUI_TIME_TEXT_INVALID_CONFIG;
    return cui_tt_value(value, o);
}
static bool cui_tt_take(cui_str text, size_t *at, cui_str token) {
    if (token.length > text.length - *at || memcmp(text.data + *at, token.data, token.length))
        return false;
    *at += token.length;
    return true;
}
static bool cui_tt_number(cui_str text, size_t *at, const cui_time_locale_v1 *l, unsigned *out) {
    unsigned value = 0;
    for (unsigned j = 0; j < 2; j++) {
        unsigned i;
        for (i = 0; i < 10; i++)
            if (cui_tt_take(text, at, l->digits[i]))
                break;
        if (i == 10)
            return false;
        value = value * 10 + i;
    }
    *out = value;
    return true;
}
cui_time_text_status_v1 cui_time_parse_v1(cui_str text, const cui_time_text_options_v1 *o,
                                          cui_time *out) {
    const cui_time_locale_v1 *l;
    if (!o || !out || (!text.data && text.length))
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    if (!cui_tt_config(o, &l))
        return CUI_TIME_TEXT_INVALID_CONFIG;
    if (!text.length || text.length > CUI_TIME_TEXT_MAX_BYTES_V1 || !cui_tt_utf8(text))
        return CUI_TIME_TEXT_INVALID_SYNTAX;
    size_t at = 0;
    cui_time t = {0};
    bool pm = false;
    if (!cui_tt_number(text, &at, l, &t.hour) || !cui_tt_take(text, &at, l->separator) ||
        !cui_tt_number(text, &at, l, &t.minute))
        return CUI_TIME_TEXT_INVALID_SYNTAX;
    if (o->precision &&
        (!cui_tt_take(text, &at, l->separator) || !cui_tt_number(text, &at, l, &t.second)))
        return CUI_TIME_TEXT_INVALID_SYNTAX;
    if (o->cycle) {
        if (!cui_tt_take(text, &at, l->period_separator))
            return CUI_TIME_TEXT_INVALID_SYNTAX;
        if (!cui_tt_take(text, &at, l->am)) {
            if (!cui_tt_take(text, &at, l->pm))
                return CUI_TIME_TEXT_INVALID_SYNTAX;
            pm = true;
        }
    }
    if (at != text.length)
        return CUI_TIME_TEXT_INVALID_SYNTAX;
    if (o->cycle) {
        if (!t.hour || t.hour > 12)
            return CUI_TIME_TEXT_INVALID_VALUE;
        t.hour = t.hour % 12 + (pm ? 12 : 0);
    }
    cui_time_text_status_v1 status = cui_tt_value(t, o);
    if (status == CUI_TIME_TEXT_OK)
        *out = t;
    return status;
}
cui_time_text_status_v1 cui_time_format_v1(cui_time t, const cui_time_text_options_v1 *o,
                                           char *output, size_t capacity, size_t *required) {
    const cui_time_locale_v1 *l;
    if (!o || !required || (!output && capacity))
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    if (!cui_tt_config(o, &l))
        return CUI_TIME_TEXT_INVALID_CONFIG;
    cui_time_text_status_v1 status = cui_tt_value(t, o);
    if (status != CUI_TIME_TEXT_OK)
        return status;
    cui_str tokens[10];
    size_t count = 0, length = 0;
    unsigned hour = o->cycle ? (t.hour % 12 ? t.hour % 12 : 12) : t.hour;
    tokens[count++] = l->digits[hour / 10];
    tokens[count++] = l->digits[hour % 10];
    tokens[count++] = l->separator;
    tokens[count++] = l->digits[t.minute / 10];
    tokens[count++] = l->digits[t.minute % 10];
    if (o->precision) {
        tokens[count++] = l->separator;
        tokens[count++] = l->digits[t.second / 10];
        tokens[count++] = l->digits[t.second % 10];
    }
    if (o->cycle) {
        tokens[count++] = l->period_separator;
        tokens[count++] = t.hour >= 12 ? l->pm : l->am;
    }
    for (size_t i = 0; i < count; i++)
        length += tokens[i].length;
    *required = length;
    if (!output && !capacity)
        return CUI_TIME_TEXT_OK;
    if (capacity <= length)
        return CUI_TIME_TEXT_BUFFER_TOO_SMALL;
    size_t at = 0;
    for (size_t i = 0; i < count; i++) {
        memcpy(output + at, tokens[i].data, tokens[i].length);
        at += tokens[i].length;
    }
    output[at] = 0;
    return CUI_TIME_TEXT_OK;
}
