/* SPDX-License-Identifier: MIT */
#include "cui/cui_bidi.h"
#include <SheenBidi/SheenBidi.h>
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

static void wrapped_lines(void) {
    /* Mixed Hebrew/Arabic, numbers, paired brackets and an isolate spanning
     * caller-selected wraps; explicit embedding makes trailing-space L1 visible. */
    static const uint32_t cps[] = {0x5d0, 0x20,   0x61,  0x62,  0x20, 0x31,  0x32,
                                   0x28,  0x2067, 0x627, 0x660, 0x20, 0x5d1, 0x2069,
                                   0x29,  0x202a, 0x63,  0x64,  0x20, 0x20,  0x202c};
    static const size_t ends[] = {2, 5, 9, 12, 16, 20, 21};
    char utf8[MAX];
    size_t n = 0, start = 0;
    cui_bidi_paragraph *p = NULL, *sentinel;
    cui_bidi_result result;
    uint8_t paragraph_levels[MAX];
    SBCodepointSequence sequence = {SBStringEncodingUTF32, cps, sizeof cps / sizeof cps[0]};
    SBAlgorithmRef algorithm = SBAlgorithmCreate(&sequence);
    SBParagraphRef source =
        SBAlgorithmCreateParagraph(algorithm, 0, sequence.stringLength, SBLevelDefaultLTR);
    assert(source);
    for (size_t i = 0; i < sequence.stringLength; ++i)
        n += encode(cps[i], utf8 + n);
    assert(cui_bidi_paragraph_create(utf8, n, CUI_BIDI_AUTO_LTR, work, MAX, &p) == CUI_BIDI_OK);
    /* The reusable object owns text; corrupt both caller buffers after creation. */
    memset(work, 0, sizeof work);
    memset(utf8, 0, sizeof utf8);
    assert(cui_bidi_paragraph_levels(p, paragraph_levels, MAX, &result) == CUI_BIDI_OK);
    assert(result.scalar_count == sequence.stringLength && result.paragraph_level == 1 &&
           result.run_count == 0);
    assert(memcmp(paragraph_levels, SBParagraphGetLevelsPtr(source), sequence.stringLength) == 0);
    for (size_t l = 0; l < sizeof ends / sizeof ends[0]; ++l) {
        size_t length = ends[l] - start, visual = 0;
        SBLineRef line = SBParagraphCreateLine(source, start, length);
        assert(line);
        assert(cui_bidi_paragraph_line(p, start, length, levels, map, mirrors, runs, MAX,
                                       &result) == CUI_BIDI_OK);
        assert(result.paragraph_level == 1 && result.scalar_count == length &&
               result.run_count == SBLineGetRunCount(line));
        for (size_t r = 0; r < result.run_count; ++r) {
            const SBRun *expected = &SBLineGetRunsPtr(line)[r];
            assert(runs[r].logical_start == expected->offset &&
                   runs[r].length == expected->length && runs[r].level == expected->level);
            for (size_t j = 0; j < expected->length; ++j) {
                size_t index =
                    expected->offset + ((expected->level & 1) ? expected->length - 1 - j : j);
                uint32_t mirror = (expected->level & 1) ? SBCodepointGetMirror(cps[index]) : 0;
                assert(map[visual] == index && levels[index - start] == expected->level &&
                       mirrors[visual] == (mirror ? mirror : cps[index]));
                ++visual;
            }
        }
        assert(visual == length);
        SBLineRelease(line);
        start = ends[l];
    }
    assert(cui_bidi_paragraph_line(p, 16, 4, levels, map, mirrors, runs, MAX, &result) ==
           CUI_BIDI_OK);
    assert(paragraph_levels[18] == 2 && levels[2] == 1 && levels[3] == 1);
    /* Resolving the Latin continuation as a fresh paragraph changes the base. */
    assert(cui_bidi_resolve("ab ", 3, CUI_BIDI_AUTO_LTR, work, MAX, levels, map, mirrors, runs, MAX,
                            &result) == CUI_BIDI_OK &&
           result.paragraph_level == 0);
    assert(cui_bidi_paragraph_line(p, 2, 3, levels, map, mirrors, runs, MAX, &result) ==
               CUI_BIDI_OK &&
           result.paragraph_level == 1 && levels[2] == 1);
    {
        uint8_t before_levels[MAX];
        size_t before_map[MAX];
        uint32_t before_mirrors[MAX];
        cui_bidi_run before_runs[MAX];
        cui_bidi_result before = result;
        memcpy(before_levels, levels, sizeof levels);
        memcpy(before_map, map, sizeof map);
        memcpy(before_mirrors, mirrors, sizeof mirrors);
        memcpy(before_runs, runs, sizeof runs);
        assert(cui_bidi_paragraph_line(p, 2, 3, levels, map, mirrors, runs, 2, &result) ==
               CUI_BIDI_CAPACITY);
        assert(cui_bidi_paragraph_line(p, SIZE_MAX, 3, levels, map, mirrors, runs, MAX, &result) ==
               CUI_BIDI_INVALID);
        assert(cui_bidi_paragraph_line(p, 0, 0, levels, map, mirrors, runs, MAX, &result) ==
               CUI_BIDI_INVALID);
        assert(memcmp(before_levels, levels, sizeof levels) == 0 &&
               memcmp(before_map, map, sizeof map) == 0 &&
               memcmp(before_mirrors, mirrors, sizeof mirrors) == 0 &&
               memcmp(before_runs, runs, sizeof runs) == 0 &&
               before.scalar_count == result.scalar_count && before.run_count == result.run_count &&
               before.paragraph_level == result.paragraph_level);
        assert(cui_bidi_paragraph_levels(p, levels, 0, &result) == CUI_BIDI_CAPACITY);
        assert(memcmp(before_levels, levels, sizeof levels) == 0 &&
               before.scalar_count == result.scalar_count && before.run_count == result.run_count &&
               before.paragraph_level == result.paragraph_level);
    }
    sentinel = p;
    assert(cui_bidi_paragraph_create("a\nb", 3, CUI_BIDI_AUTO_LTR, work, MAX, &sentinel) ==
               CUI_BIDI_INVALID &&
           sentinel == p);
    assert(cui_bidi_paragraph_create("\xed\xa0\x80", 3, CUI_BIDI_AUTO_LTR, work, MAX, &sentinel) ==
               CUI_BIDI_INVALID &&
           sentinel == p);
    assert(cui_bidi_paragraph_create("abc", 3, CUI_BIDI_AUTO_LTR, work, 2, &sentinel) ==
               CUI_BIDI_CAPACITY &&
           sentinel == p);
    cui_bidi_paragraph_destroy(p);
    cui_bidi_paragraph_destroy(NULL);
    assert(cui_bidi_paragraph_create(NULL, 0, CUI_BIDI_AUTO_RTL, NULL, 0, &p) == CUI_BIDI_OK);
    assert(cui_bidi_paragraph_levels(p, NULL, 0, &result) == CUI_BIDI_OK &&
           result.scalar_count == 0 && result.paragraph_level == 1);
    cui_bidi_paragraph_destroy(p);
    SBParagraphRelease(source);
    SBAlgorithmRelease(algorithm);
}

int main(int argc, char **argv) {
    wrapped_lines();
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
