/* SPDX-License-Identifier: MIT */
#include "cui/cui_bidi.h"
#include <SheenBidi/SheenBidi.h>
#include <string.h>

static int decode(const unsigned char *s, size_t n, uint32_t *out, size_t *count) {
    size_t i = 0, k = 0;
    while (i < n) {
        uint32_t c = s[i++], min = 0;
        size_t more = 0;
        if (c < 0x80) {
        } else if (c >= 0xc2 && c <= 0xdf) {
            c &= 31;
            more = 1;
            min = 0x80;
        } else if (c >= 0xe0 && c <= 0xef) {
            c &= 15;
            more = 2;
            min = 0x800;
        } else if (c >= 0xf0 && c <= 0xf4) {
            c &= 7;
            more = 3;
            min = 0x10000;
        } else
            return 0;
        if (more > n - i)
            return 0;
        while (more--) {
            uint32_t b = s[i++];
            if ((b & 0xc0) != 0x80)
                return 0;
            c = (c << 6) | (b & 63);
        }
        if (c < min || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            return 0;
        out[k++] = c;
    }
    *count = k;
    return 1;
}

cui_bidi_status cui_bidi_resolve(const char *utf8, size_t n, cui_bidi_direction direction,
                                 uint32_t *workspace, size_t workspace_capacity, uint8_t *levels,
                                 size_t *mapping, uint32_t *mirrored, cui_bidi_run *runs,
                                 size_t capacity, cui_bidi_result *result) {
    static const SBLevel bases[] = {SBLevelDefaultLTR, SBLevelDefaultRTL, 0, 1};
    SBAlgorithmRef algorithm;
    SBParagraphRef paragraph;
    SBLineRef line;
    SBCodepointSequence sequence;
    size_t count, visual = 0;
    if (!result || (unsigned)direction > CUI_BIDI_RTL ||
        (n && (!utf8 || !workspace || !levels || !mapping || !mirrored || !runs)))
        return CUI_BIDI_INVALID;
    if (workspace_capacity < n || capacity < n)
        return CUI_BIDI_CAPACITY;
    if (!n) {
        cui_bidi_result empty = {0, 0, direction == CUI_BIDI_RTL || direction == CUI_BIDI_AUTO_RTL};
        *result = empty;
        return CUI_BIDI_OK;
    }
    if (!decode((const unsigned char *)utf8, n, workspace, &count))
        return CUI_BIDI_INVALID;
    sequence.stringEncoding = SBStringEncodingUTF32;
    sequence.stringBuffer = workspace;
    sequence.stringLength = count;
    algorithm = SBAlgorithmCreate(&sequence);
    if (!algorithm)
        return CUI_BIDI_MEMORY;
    paragraph = SBAlgorithmCreateParagraph(algorithm, 0, count, bases[direction]);
    if (!paragraph) {
        SBAlgorithmRelease(algorithm);
        return CUI_BIDI_MEMORY;
    }
    if (SBParagraphGetLength(paragraph) != count) {
        SBParagraphRelease(paragraph);
        SBAlgorithmRelease(algorithm);
        return CUI_BIDI_INVALID;
    }
    line = SBParagraphCreateLine(paragraph, 0, count);
    if (!line) {
        SBParagraphRelease(paragraph);
        SBAlgorithmRelease(algorithm);
        return CUI_BIDI_MEMORY;
    }
    /* All fallible operations precede output publication. */
    memcpy(levels, SBParagraphGetLevelsPtr(paragraph), count);
    for (size_t r = 0; r < SBLineGetRunCount(line); ++r) {
        const SBRun *run = &SBLineGetRunsPtr(line)[r];
        runs[r].logical_start = run->offset;
        runs[r].length = run->length;
        runs[r].level = run->level;
        for (size_t j = 0; j < run->length; ++j) {
            size_t logical = run->offset + ((run->level & 1) ? run->length - 1 - j : j);
            uint32_t mirror = (run->level & 1) ? SBCodepointGetMirror(workspace[logical]) : 0;
            levels[logical] = run->level;
            mapping[visual] = logical;
            mirrored[visual++] = mirror ? mirror : workspace[logical];
        }
    }
    result->scalar_count = count;
    result->run_count = SBLineGetRunCount(line);
    result->paragraph_level = SBParagraphGetBaseLevel(paragraph);
    SBLineRelease(line);
    SBParagraphRelease(paragraph);
    SBAlgorithmRelease(algorithm);
    return CUI_BIDI_OK;
}
