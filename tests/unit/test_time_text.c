/* SPDX-License-Identifier: MIT */
#if defined(CUI_TIME_TEXT_UMBRELLA)
#define CUI_IMPLEMENTATION_ALL
#include "cui_all.h"
#elif defined(CUI_TIME_TEXT_DISTRIBUTION)
#define CUI_TIME_TEXT_IMPLEMENTATION
#include "cui_time_text.h"
#else
#include "cui/cui_time_text.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void exhaustive(void) {
    cui_time_text_options_v1 o;
    assert(cui_time_text_options_init_v1(&o) == CUI_TIME_TEXT_OK);
    for (unsigned cycle = 0; cycle < 2; cycle++)
        for (unsigned precision = 0; precision < 2; precision++) {
            o.cycle = cycle;
            o.precision = precision;
            for (unsigned n = 0; n < 86400; n += precision ? 1 : 60) {
                cui_time t = {n / 3600, n / 60 % 60, n % 60}, parsed = {99, 98, 97};
                char actual[161], expected[32];
                size_t required = 999;
                unsigned hour = cycle ? (t.hour % 12 ? t.hour % 12 : 12) : t.hour;
                if (precision)
                    snprintf(expected, sizeof expected, "%02u:%02u:%02u%s%s", hour, t.minute,
                             t.second, cycle ? " " : "", cycle ? (t.hour >= 12 ? "PM" : "AM") : "");
                else
                    snprintf(expected, sizeof expected, "%02u:%02u%s%s", hour, t.minute,
                             cycle ? " " : "", cycle ? (t.hour >= 12 ? "PM" : "AM") : "");
                assert(cui_time_format_v1(t, &o, actual, sizeof actual, &required) ==
                       CUI_TIME_TEXT_OK);
                assert(required == strlen(expected) && !strcmp(actual, expected));
                assert(cui_time_parse_v1((cui_str){actual, required}, &o, &parsed) ==
                       CUI_TIME_TEXT_OK);
                assert(parsed.hour == t.hour && parsed.minute == t.minute &&
                       parsed.second == t.second);
            }
        }
}
static void negative(void) {
    cui_time_text_options_v1 o;
    cui_time_locale_v1 l;
    assert(cui_time_text_options_init_v1(&o) == 0 && cui_time_locale_init_v1(&l) == 0);
    cui_time t = {9, 8, 7};
    unsigned char saved[sizeof t];
    memcpy(saved, &t, sizeof t);
    const char *bad[] = {"", "9:00", "09:00 ", "09.00", "09:00:00", "09:0", "09:00 AM"};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        assert(cui_time_parse_v1((cui_str){bad[i], strlen(bad[i])}, &o, &t) ==
               CUI_TIME_TEXT_INVALID_SYNTAX);
        assert(!memcmp(saved, &t, sizeof t));
    }
    assert(cui_time_parse_v1((cui_str){NULL, 0}, &o, &t) == CUI_TIME_TEXT_INVALID_SYNTAX);
    assert(cui_time_parse_v1((cui_str){NULL, 1}, &o, &t) == CUI_TIME_TEXT_INVALID_ARGUMENT);
    assert(cui_time_parse_v1((cui_str){"24:00", 5}, &o, &t) == CUI_TIME_TEXT_INVALID_VALUE);
    char large[161];
    memset(large, '0', sizeof large);
    assert(cui_time_parse_v1((cui_str){large, sizeof large}, &o, &t) ==
           CUI_TIME_TEXT_INVALID_SYNTAX);
    o.version = 9;
    assert(cui_time_parse_v1((cui_str){large, sizeof large}, &o, &t) ==
           CUI_TIME_TEXT_INVALID_CONFIG);
    o.version = 1;
    o.locale = &l;
    const cui_str invalid[] = {
        {"", 0},     {"x", 17}, {"\xc0\x80", 2}, {"\xed\xa0\x80", 3}, {"\xf4\x90\x80\x80", 4},
        {"a\0b", 3}, {NULL, 1}};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        l.am = invalid[i];
        assert(cui_time_validate_v1((cui_time){0}, &o) == CUI_TIME_TEXT_INVALID_CONFIG);
    }
    cui_time_locale_init_v1(&l);
    for (unsigned i = 0; i < 10; i++) {
        l.pm = l.digits[i];
        assert(cui_time_validate_v1((cui_time){0}, &o) == CUI_TIME_TEXT_INVALID_CONFIG);
    }
    cui_time_locale_init_v1(&l);
    l.pm = (cui_str){"A", 1};
    assert(cui_time_validate_v1((cui_time){0}, &o) == CUI_TIME_TEXT_INVALID_CONFIG);
    o.locale = NULL;
    o.flags = 3;
    o.minimum = (cui_time){9, 0, 0};
    o.maximum = (cui_time){10, 0, 0};
    assert(cui_time_validate_v1(o.minimum, &o) == 0 && cui_time_validate_v1(o.maximum, &o) == 0);
    assert(cui_time_validate_v1((cui_time){8, 59, 0}, &o) == CUI_TIME_TEXT_OUT_OF_RANGE);
    assert(cui_time_validate_v1((cui_time){9, 0, 1}, &o) == CUI_TIME_TEXT_INVALID_VALUE);
    o.maximum = (cui_time){8, 0, 0};
    assert(cui_time_validate_v1((cui_time){9, 0, 0}, &o) == CUI_TIME_TEXT_INVALID_CONFIG);
    o.flags = 0;
    o.minimum = (cui_time){99, 99, 99};
    assert(cui_time_validate_v1((cui_time){9, 0, 0}, &o) == 0);
}
static void buffers(void) {
    cui_time_text_options_v1 o;
    cui_time_text_options_init_v1(&o);
    char out[16], saved[16];
    memset(out, 0x55, sizeof out);
    memcpy(saved, out, sizeof out);
    size_t n = 99;
    assert(cui_time_format_v1((cui_time){9, 30, 0}, &o, NULL, 0, &n) == 0 && n == 5);
    for (size_t cap = 0; cap <= 5; cap++) {
        assert(cui_time_format_v1((cui_time){9, 30, 0}, &o, out, cap, &n) ==
               CUI_TIME_TEXT_BUFFER_TOO_SMALL);
        assert(n == 5 && !memcmp(out, saved, sizeof out));
    }
    assert(cui_time_format_v1((cui_time){9, 30, 0}, &o, out, 6, &n) == 0 && !strcmp(out, "09:30") &&
           out[6] == 0x55);
    memcpy(out, saved, sizeof out);
    n = 99;
    assert(cui_time_format_v1((cui_time){9, 30, 1}, &o, out, sizeof out, &n) ==
           CUI_TIME_TEXT_INVALID_VALUE);
    assert(n == 99 && !memcmp(out, saved, sizeof out));
}
static void localized(void) {
    cui_time_locale_v1 l;
    cui_time_locale_init_v1(&l);
    const char *digits[] = {"٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩"};
    for (unsigned i = 0; i < 10; i++)
        l.digits[i] = (cui_str){digits[i], strlen(digits[i])};
    l.separator = (cui_str){"·", 2};
    l.am = (cui_str){"ص", 2};
    l.pm = (cui_str){"م", 2};
    cui_time_text_options_v1 o;
    cui_time_text_options_init_v1(&o);
    o.locale = &l;
    o.cycle = 1;
    o.precision = 1;
    const char expected[] = "١٢·٣٤·٥٦ م";
    char out[161];
    size_t n;
    cui_time t = {0};
    assert(cui_time_format_v1((cui_time){12, 34, 56}, &o, out, sizeof out, &n) == 0 &&
           !strcmp(out, expected));
    assert(cui_time_parse_v1((cui_str){expected, sizeof expected - 1}, &o, &t) == 0 &&
           t.hour == 12 && t.minute == 34 && t.second == 56);
    assert(cui_time_parse_v1((cui_str){"12:34:56 PM", 11}, &o, &t) == CUI_TIME_TEXT_INVALID_SYNTAX);
}
static void failure_precedence(void) {
    cui_time_text_options_v1 o;
    cui_time_text_options_init_v1(&o);
    cui_time t = {1, 2, 3};
    unsigned char before[sizeof t];
    memcpy(before, &t, sizeof t);
    size_t required = 777;
    char text[16];
    memset(text, 0x5a, sizeof text);
    o.version = 99;
    assert(cui_time_parse_v1((cui_str){NULL, 1}, &o, &t) == CUI_TIME_TEXT_INVALID_ARGUMENT);
    assert(cui_time_parse_v1((cui_str){"24:00", 5}, &o, &t) == CUI_TIME_TEXT_INVALID_CONFIG);
    assert(cui_time_format_v1((cui_time){99, 0, 0}, &o, NULL, 1, &required) ==
           CUI_TIME_TEXT_INVALID_ARGUMENT);
    assert(cui_time_format_v1((cui_time){99, 0, 0}, &o, text, 0, &required) ==
           CUI_TIME_TEXT_INVALID_CONFIG);
    o.version = 1;
    assert(cui_time_parse_v1((cui_str){"24:00x", 6}, &o, &t) == CUI_TIME_TEXT_INVALID_SYNTAX);
    assert(cui_time_parse_v1((cui_str){"\xc0\x80", 2}, &o, &t) == CUI_TIME_TEXT_INVALID_SYNTAX);
    assert(cui_time_parse_v1((cui_str){"09:0\0", 5}, &o, &t) == CUI_TIME_TEXT_INVALID_SYNTAX);
    o.flags = CUI_TIME_TEXT_HAS_MINIMUM_V1;
    o.minimum = (cui_time){10, 0, 0};
    assert(cui_time_parse_v1((cui_str){"09:00", 5}, &o, &t) == CUI_TIME_TEXT_OUT_OF_RANGE);
    assert(!memcmp(before, &t, sizeof t));
    assert(cui_time_format_v1((cui_time){9, 0, 0}, &o, text, 0, &required) ==
           CUI_TIME_TEXT_OUT_OF_RANGE);
    assert(required == 777);
    for (size_t i = 0; i < sizeof text; i++)
        assert(text[i] == 0x5a);
    assert(cui_time_locale_init_v1(NULL) == CUI_TIME_TEXT_INVALID_ARGUMENT);
    assert(cui_time_text_options_init_v1(NULL) == CUI_TIME_TEXT_INVALID_ARGUMENT);
    assert(cui_time_validate_v1(t, NULL) == CUI_TIME_TEXT_INVALID_ARGUMENT);
}
static void configuration(void) {
    cui_time_text_options_v1 o;
    cui_time_text_options_init_v1(&o);
    cui_time_text_options_v1 original = o;
    uint32_t *fields[] = {&o.struct_size, &o.version,     &o.cycle,       &o.precision,
                          &o.flags,       &o.reserved[0], &o.reserved[1], &o.reserved[2]};
    for (size_t i = 0; i < sizeof fields / sizeof fields[0]; i++) {
        o = original;
        *fields[i] = UINT32_MAX;
        assert(cui_time_validate_v1((cui_time){0}, &o) == CUI_TIME_TEXT_INVALID_CONFIG);
    }
    o = original;
    cui_time_locale_v1 l;
    cui_time_locale_init_v1(&l);
    o.locale = &l;
    char storage[14][16];
    for (unsigned i = 0; i < 14; i++) {
        memset(storage[i], 'a' + (int)i, 16);
        cui_str token = {storage[i], 16};
        if (i < 10)
            l.digits[i] = token;
        else if (i == 10)
            l.separator = token;
        else if (i == 11)
            l.period_separator = token;
        else if (i == 12)
            l.am = token;
        else
            l.pm = token;
    }
    o.cycle = 1;
    o.precision = 1;
    char text[162];
    memset(text, 0x55, sizeof text);
    size_t n = 0;
    assert(cui_time_format_v1((cui_time){23, 59, 59}, &o, text, 160, &n) ==
               CUI_TIME_TEXT_BUFFER_TOO_SMALL &&
           n == 160 && text[0] == 0x55);
    assert(cui_time_format_v1((cui_time){23, 59, 59}, &o, text, 161, &n) == 0 && n == 160 &&
           text[160] == 0 && text[161] == 0x55);
    cui_time parsed = {0};
    assert(cui_time_parse_v1((cui_str){text, 160}, &o, &parsed) == 0 && parsed.hour == 23 &&
           parsed.minute == 59 && parsed.second == 59);
    l.version = 0;
    assert(cui_time_validate_v1(parsed, &o) == CUI_TIME_TEXT_INVALID_CONFIG);
    l.version = 1;
    l.reserved[0] = 1;
    assert(cui_time_validate_v1(parsed, &o) == CUI_TIME_TEXT_INVALID_CONFIG);
}
int main(void) {
    failure_precedence();
    configuration();
    exhaustive();
    negative();
    buffers();
    localized();
    printf("time text: locale=%zu options=%zu\n", sizeof(cui_time_locale_v1),
           sizeof(cui_time_text_options_v1));
    return 0;
}
