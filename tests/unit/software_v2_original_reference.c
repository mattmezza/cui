/* SPDX-License-Identifier: MIT */
/* Actual immutable V1 renderer; no font objects or V2 implementation involved. */
#include "../fixtures/text_prepare_v1/prefix.h"
#include "../fixtures/text_prepare_v1/source/backends/software/cui_software.c"
bool software_v2_original_reference(const cui_draw_list *list, cui_color clear, unsigned width,
                                    unsigned height, unsigned char *out) {
    cui_software *r = cui_software_create(width, height, NULL);
    if (!r)
        return false;
    bool ok = cui_software_render(r, list, clear);
    if (ok)
        memcpy(out, cui_software_pixels(r, NULL, NULL), (size_t)width * height * 4);
    cui_software_destroy(r);
    return ok;
}
