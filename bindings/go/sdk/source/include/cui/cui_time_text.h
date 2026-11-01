/* SPDX-License-Identifier: MIT */
#ifndef CUI_TIME_TEXT_H
#define CUI_TIME_TEXT_H
#include "cui.h"
#include "cui_time_types.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Pure synchronous helpers. Records and UTF-8 tokens are borrowed for each call.
 * Exact size/version, zero reserved fields; NULL locale selects ASCII defaults.
 * Tokens are nonempty, at most16 bytes, UTF-8 without NUL, pairwise prefix-free.
 * No normalization, trimming, case folding, allocation or OS locale lookup. */
#define CUI_TIME_TEXT_VERSION_V1 UINT32_C(1)
#define CUI_TIME_TOKEN_MAX_BYTES_V1 16u
#define CUI_TIME_TEXT_MAX_BYTES_V1 160u

typedef enum {
    CUI_TIME_TEXT_OK = 0,
    CUI_TIME_TEXT_INVALID_ARGUMENT = 1,
    CUI_TIME_TEXT_INVALID_CONFIG = 2,
    CUI_TIME_TEXT_INVALID_SYNTAX = 3,
    CUI_TIME_TEXT_INVALID_VALUE = 4,
    CUI_TIME_TEXT_OUT_OF_RANGE = 5,
    CUI_TIME_TEXT_BUFFER_TOO_SMALL = 6
} cui_time_text_status_v1;
typedef enum { CUI_TIME_TEXT_H23 = 0, CUI_TIME_TEXT_H12 = 1 } cui_time_text_cycle_v1;
typedef enum { CUI_TIME_TEXT_MINUTE = 0, CUI_TIME_TEXT_SECOND = 1 } cui_time_text_precision_v1;
enum { CUI_TIME_TEXT_HAS_MINIMUM_V1 = 1u << 0, CUI_TIME_TEXT_HAS_MAXIMUM_V1 = 1u << 1 };
typedef struct {
    uint32_t struct_size, version;
    uint32_t reserved[2];
    cui_str digits[10];
    cui_str separator;
    cui_str period_separator;
    cui_str am, pm;
} cui_time_locale_v1;
typedef struct {
    uint32_t struct_size, version;
    uint32_t cycle, precision;
    uint32_t flags;
    uint32_t reserved[3];
    cui_time minimum, maximum;
    const cui_time_locale_v1 *locale;
} cui_time_text_options_v1;

cui_time_text_status_v1 cui_time_locale_init_v1(cui_time_locale_v1 *out);
cui_time_text_status_v1 cui_time_text_options_init_v1(cui_time_text_options_v1 *out);
cui_time_text_status_v1 cui_time_validate_v1(cui_time value,
                                             const cui_time_text_options_v1 *options);
cui_time_text_status_v1 cui_time_parse_v1(cui_str text, const cui_time_text_options_v1 *options,
                                          cui_time *out);
cui_time_text_status_v1 cui_time_format_v1(cui_time value, const cui_time_text_options_v1 *options,
                                           char *output, size_t capacity, size_t *required_bytes);
/* Exact grammar: two configured digits per number, configured separator; H12
 * appends period_separator and exact am/pm. No optional whitespace/case folding.
 * Minute parse produces second0. NULL text.data with length0 is empty syntax;
 * NULL with nonzero length is an invalid argument. Text exceeding160 bytes is
 * rejected after configuration validation, before scanning text.
 * Failure precedence: mandatory arguments, configuration, syntax, numeric value/
 * precision, inclusive domain, output capacity. Inactive bounds are ignored;
 * min>max is invalid configuration, not an overnight domain.
 * Parse output changes only on OK. Minute precision requires second0 for values
 * and active bounds. Bounds are inclusive, same-day; inactive bounds ignored.
 * Format is atomic: required_bytes excludes NUL and changes only on OK or
 * BUFFER_TOO_SMALL. NULL output/capacity0 queries size; otherwise capacity must
 * exceed required bytes. No partial text or failure clearing. Writable outputs
 * must not overlap input records/token/text storage or each other. */
#ifdef __cplusplus
}
#endif
#endif
