/* SPDX-License-Identifier: MIT */
/* Separate TU compiles immutable8671 source, not the forwarding aliases. */
#include "cpu_v2_common_reference_internal.h"
#include <assert.h>
static size_t original_samples;
#define CUI_SOFTWARE_V2_SHADOW_SAMPLE_HOOK() (++original_samples)
#ifndef CUI_CPU_V2_FROZEN_HEADER
#error Supply the frozen header copy with ONLY relative include spelling adjusted.
#endif
#include CUI_CPU_V2_FROZEN_HEADER
size_t cpu_common_original_layout(size_t *out, size_t capacity) {
    const size_t values[] = {
        sizeof(cui_software_v2_target),
        _Alignof(cui_software_v2_target),
        sizeof(cui_software_v2_clip),
        _Alignof(cui_software_v2_clip),
        sizeof(cui_software_v2_paint_descriptor),
        _Alignof(cui_software_v2_paint_descriptor),
        sizeof(cui_software_v2_plan),
        _Alignof(cui_software_v2_plan),
        sizeof(cui_software_v2_workspace),
        _Alignof(cui_software_v2_workspace),
        sizeof(cui_software_v2_shadow_descriptor),
        _Alignof(cui_software_v2_shadow_descriptor),
#ifdef CUI_HAS_TEXT
        sizeof(cui_software_v2_text_range),
        _Alignof(cui_software_v2_text_range),
#endif
    };
    const size_t count = sizeof values / sizeof *values;
    assert(capacity >= count);
    memcpy(out, values, sizeof values);
    return count;
}
cui_error cpu_common_original_paint(unsigned char *pixels, unsigned width, unsigned height,
                                    size_t stride, size_t capacity, const cui_draw_list_v2 *list,
                                    cui_color clear, const cui_render_v2_options *options,
                                    cui_font *font, cpu_common_allocate allocate,
                                    cpu_common_release release, void *user, bool *busy) {
    cui_software_v2_target target = {pixels, width, height, stride, capacity, false};
    original_samples = 0;
    cui_error error = cui_software_v2_paint_with_font_excluding(
        &target, list, clear, options, font, allocate, release, user, NULL, 0, busy);
    assert(!target.busy);
    return error;
}
size_t cpu_common_original_samples(void) {
    return original_samples;
}
