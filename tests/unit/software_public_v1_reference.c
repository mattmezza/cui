/* SPDX-License-Identifier: MIT */
/* Immutable pre-V2 software source, independent object and allocator trace. */
#include "../fixtures/text_prepare_v1/prefix.h"
#include "software_public_v1_alloc_internal.h"
#include <stdlib.h>
#define malloc sw_test_malloc
#define calloc sw_test_calloc
#define realloc sw_test_realloc
#define free sw_test_free
#include "../fixtures/text_prepare_v1/source/backends/software/cui_software.c"
#undef malloc
#undef calloc
#undef realloc
#undef free
void *sw_test_v1_create(unsigned w, unsigned h) {
    return cui_software_create(w, h, NULL);
}
bool sw_test_v1_resize(void *r, unsigned w, unsigned h) {
    return cui_software_resize(r, w, h);
}
const unsigned char *sw_test_v1_pixels(const void *r, unsigned *w, unsigned *h) {
    return cui_software_pixels(r, w, h);
}
cui_renderer sw_test_v1_interface(void *r) {
    return cui_software_interface(r);
}
void sw_test_v1_layout(size_t *out) {
    const size_t values[] = {sizeof(cui_software),
                             _Alignof(cui_software),
                             offsetof(cui_software, shadows),
                             offsetof(cui_software, width),
                             offsetof(cui_software, height),
                             offsetof(cui_software, pixels),
                             offsetof(cui_software, font),
                             offsetof(cui_software, clip),
                             offsetof(cui_software, ink),
                             offsetof(cui_software, text_paint)};
    for (size_t i = 0; i < sizeof values / sizeof *values; ++i)
        out[i] = values[i];
}
