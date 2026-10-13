/* SPDX-License-Identifier: MIT */
#include "cui/cui_bidi.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX 8192
static uint32_t work[MAX], mirrors[MAX];
static uint8_t levels[MAX];
static size_t map[MAX];
static cui_bidi_run runs[MAX];
static size_t encode(uint32_t cp, char *s) {
    if (cp < 128) {
        s[0] = (char)cp;
        return 1;
    }
    if (cp < 2048) {
        s[0] = (char)(192 | (cp >> 6));
        s[1] = (char)(128 | (cp & 63));
        return 2;
    }
    if (cp < 65536) {
        s[0] = (char)(224 | (cp >> 12));
        s[1] = (char)(128 | ((cp >> 6) & 63));
        s[2] = (char)(128 | (cp & 63));
        return 3;
    }
    s[0] = (char)(240 | (cp >> 18));
    s[1] = (char)(128 | ((cp >> 12) & 63));
    s[2] = (char)(128 | ((cp >> 6) & 63));
    s[3] = (char)(128 | (cp & 63));
    return 4;
}
static size_t conformance(const char *path) {
    FILE *f = fopen(path, "r");
    char line[65536], utf8[MAX];
    size_t tested = 0;
    assert(f);
    while (fgets(line, sizeof line, f)) {
        char *fields[5], *p = line;
        size_t n = 0, scalar = 0, v = 0;
        unsigned char removed[MAX] = {0};
        cui_bidi_result result;
        cui_bidi_direction direction;
        if (*p == '#' || *p == '\n' || *p == '\r')
            continue;
        fields[0] = p;
        for (size_t i = 1; i < 5; ++i) {
            p = strchr(p, ';');
            assert(p);
            *p++ = 0;
            fields[i] = p;
        }
        p = fields[0];
        while (*p) {
            char *end;
            unsigned long cp = strtoul(p, &end, 16);
            if (end == p)
                break;
            assert(n + 4 < MAX);
            n += encode((uint32_t)cp, utf8 + n);
            scalar++;
            p = end;
        }
        direction = atoi(fields[1]) == 2   ? CUI_BIDI_AUTO_LTR
                    : atoi(fields[1]) == 1 ? CUI_BIDI_RTL
                                           : CUI_BIDI_LTR;
        assert(cui_bidi_resolve(utf8, n, direction, work, MAX, levels, map, mirrors, runs, MAX,
                                &result) == CUI_BIDI_OK);
        assert(result.scalar_count == scalar && result.paragraph_level == atoi(fields[2]));
        p = fields[3];
        for (size_t i = 0; i < scalar; ++i) {
            while (isspace((unsigned char)*p))
                ++p;
            if (*p == 'x') {
                removed[i] = 1;
                ++p;
            } else {
                char *end;
                unsigned long expected = strtoul(p, &end, 10);
                if (levels[i] != expected) {
                    fprintf(stderr, "level failure case %zu scalar %zu: %u expected %lu\n", tested,
                            i, levels[i], expected);
                    abort();
                }
                p = end;
            }
        }
        p = fields[4];
        while (*p) {
            char *end;
            unsigned long expected = strtoul(p, &end, 10);
            if (end == p)
                break;
            while (v < scalar && removed[map[v]])
                ++v;
            assert(v < scalar && map[v] == expected);
            ++v;
            p = end;
        }
        while (v < scalar && removed[map[v]])
            ++v;
        assert(v == scalar);
        tested++;
    }
    fclose(f);
    return tested;
}

static size_t property_conformance(const char *path) {
    static const char *names[] = {"L",   "R",   "AL",  "EN",  "ES",  "ET",  "AN",  "CS",
                                  "NSM", "BN",  "B",   "S",   "WS",  "ON",  "LRE", "LRO",
                                  "RLE", "RLO", "PDF", "LRI", "RLI", "FSI", "PDI"};
    static const uint32_t cps[] = {0x61,   0x5d0,  0x627,  0x30,   0x2b,   0x24,   0x660,  0x2c,
                                   0x300,  0xad,   0x2029, 0x9,    0x20,   0x21,   0x202a, 0x202d,
                                   0x202b, 0x202e, 0x202c, 0x2066, 0x2067, 0x2068, 0x2069};
    FILE *f = fopen(path, "r");
    char line[MAX], expected_levels[MAX] = "", expected_order[MAX] = "";
    size_t tested = 0;
    assert(f);
    while (fgets(line, sizeof line, f)) {
        char utf8[MAX], *p = line, *semi;
        size_t n = 0, scalar = 0;
        unsigned long bits;
        if (strncmp(line, "@Levels:", 8) == 0) {
            strcpy(expected_levels, line + 8);
            continue;
        }
        if (strncmp(line, "@Reorder:", 9) == 0) {
            strcpy(expected_order, line + 9);
            continue;
        }
        if (*p == '#' || *p == '@' || *p == '\n' || *p == '\r')
            continue;
        semi = strchr(p, ';');
        assert(semi);
        *semi++ = 0;
        bits = strtoul(semi, NULL, 16);
        while (*p) {
            char name[8];
            size_t k = 0, found = 0;
            while (isspace((unsigned char)*p))
                ++p;
            if (!*p)
                break;
            while (*p && *p != ' ') {
                assert(k < 7);
                name[k++] = *p++;
            }
            name[k] = 0;
            for (size_t i = 0; i < sizeof cps / sizeof cps[0]; ++i)
                if (strcmp(name, names[i]) == 0) {
                    n += encode(cps[i], utf8 + n);
                    found = 1;
                    break;
                }
            assert(found);
            scalar++;
        }
        for (unsigned bit = 1; bit <= 4; bit <<= 1)
            if (bits & bit) {
                cui_bidi_result result;
                unsigned char removed[MAX] = {0};
                size_t v = 0;
                cui_bidi_direction direction = bit == 1   ? CUI_BIDI_AUTO_LTR
                                               : bit == 2 ? CUI_BIDI_LTR
                                                          : CUI_BIDI_RTL;
                assert(cui_bidi_resolve(utf8, n, direction, work, MAX, levels, map, mirrors, runs,
                                        MAX, &result) == CUI_BIDI_OK);
                p = expected_levels;
                for (size_t i = 0; i < scalar; ++i) {
                    while (isspace((unsigned char)*p))
                        ++p;
                    if (*p == 'x') {
                        removed[i] = 1;
                        ++p;
                    } else {
                        char *end;
                        unsigned long value = strtoul(p, &end, 10);
                        if (p == end || levels[i] != value) {
                            fprintf(stderr,
                                    "property case %zu bit %u input %s index %zu level %u expected "
                                    "%lu\n",
                                    tested, bit, line, i, levels[i], value);
                            abort();
                        }
                        p = end;
                    }
                }
                p = expected_order;
                while (*p) {
                    char *end;
                    unsigned long value = strtoul(p, &end, 10);
                    if (p == end)
                        break;
                    while (v < scalar && removed[map[v]])
                        ++v;
                    assert(v < scalar && map[v] == value);
                    ++v;
                    p = end;
                }
                while (v < scalar && removed[map[v]])
                    ++v;
                assert(v == scalar);
                tested++;
            }
    }
    fclose(f);
    return tested;
}

int main(int argc, char **argv) {
    cui_bidi_result result = {17, 19, 23};
    memset(levels, 42, sizeof levels);
    assert(cui_bidi_resolve("abc", 3, CUI_BIDI_LTR, work, 2, levels, map, mirrors, runs, MAX,
                            &result) == CUI_BIDI_CAPACITY);
    assert(result.scalar_count == 17 && levels[0] == 42);
    assert(cui_bidi_resolve("\xc0\xaf", 2, CUI_BIDI_LTR, work, MAX, levels, map, mirrors, runs, MAX,
                            &result) == CUI_BIDI_INVALID);
    assert(result.scalar_count == 17 && levels[0] == 42);
    assert(cui_bidi_resolve("a\nb", 3, CUI_BIDI_LTR, work, MAX, levels, map, mirrors, runs, MAX,
                            &result) == CUI_BIDI_INVALID);
    assert(result.scalar_count == 17 && levels[0] == 42);
    assert(cui_bidi_resolve("(\xd7\x90)", 4, CUI_BIDI_RTL, work, MAX, levels, map, mirrors, runs,
                            MAX, &result) == CUI_BIDI_OK);
    assert(result.paragraph_level == 1 && map[0] == 2 && mirrors[0] == '(' && mirrors[2] == ')');
    assert(cui_bidi_resolve(NULL, 0, CUI_BIDI_AUTO_RTL, NULL, 0, NULL, NULL, NULL, NULL, 0,
                            &result) == CUI_BIDI_OK &&
           result.paragraph_level == 1);
    if (argc > 1)
        printf("BidiCharacterTest: %zu cases passed\n", conformance(argv[1]));
    if (argc > 2)
        printf("BidiTest: %zu direction cases passed\n", property_conformance(argv[2]));
    return 0;
}
