/* SPDX-License-Identifier: MIT */
#ifndef CUI_ALIGNMENT_INTERNAL_H
#define CUI_ALIGNMENT_INTERNAL_H
/* Private fundamental alignment for the actual CUI scalar/pointer payloads.
 * MSVC's C11 library need not provide max_align_t. No public/platform ABI.
 * Call sites assert their stored record/callback alignment fits this union;
 * explicitly over-aligned application objects are outside the arena contract. */
typedef union {
    long double long_double_value;
    double double_value;
    long long long_long_value;
    void *object_pointer;
    void (*function_pointer)(void);
} cui_fundamental_alignment_internal;
#endif
