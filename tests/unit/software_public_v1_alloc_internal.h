/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_SOFTWARE_PUBLIC_V1_ALLOC_INTERNAL_H
#define CUI_TEST_SOFTWARE_PUBLIC_V1_ALLOC_INTERNAL_H
#include "cui/cui_renderer.h"
#include <stddef.h>
void *sw_test_malloc(size_t);
void *sw_test_calloc(size_t, size_t);
void *sw_test_realloc(void *, size_t);
void sw_test_free(void *);
void *sw_test_v1_create(unsigned, unsigned);
bool sw_test_v1_resize(void *, unsigned, unsigned);
const unsigned char *sw_test_v1_pixels(const void *, unsigned *, unsigned *);
cui_renderer sw_test_v1_interface(void *);
void sw_test_v1_layout(size_t *);
#endif
