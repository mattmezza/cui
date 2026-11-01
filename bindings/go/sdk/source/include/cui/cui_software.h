/* SPDX-License-Identifier: MIT */
#ifndef CUI_SOFTWARE_H
#define CUI_SOFTWARE_H
#include "cui_draw_v2.h"
#include "cui_renderer.h"
#include "cui_text.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_software cui_software;
cui_software *cui_software_create(unsigned width, unsigned height, cui_font *font);
void cui_software_destroy(cui_software *renderer);
bool cui_software_resize(cui_software *renderer, unsigned width, unsigned height);
bool cui_software_render(cui_software *renderer, const cui_draw_list *list, cui_color clear);
/* Unreleased, software-only V2 opt-in. V1 interface/layout remain unchanged.
 * Query before Core V2 composition; requery after resize. Exact out->size is
 * required; errors leave output unchanged. Output must be disjoint from renderer,
 * pixels and the opaque borrowed font graph. Query allocates nothing. */
enum {
    CUI_SOFTWARE_V2_TEXT_A8 = 1u << 0,
    CUI_SOFTWARE_V2_TEXT_CONTEXT = 1u << 1,
    CUI_SOFTWARE_V2_TEXT_TRACKING = 1u << 2
};
typedef struct {
    uint32_t size;
    cui_draw_v2_support draw;
    uint32_t text_features;
    cui_render_v2_options defaults;
} cui_software_v2_info;
cui_error cui_software_query_v2(const cui_software *, cui_software_v2_info *out);
/* NULL options selects current query defaults. Explicit options must match the
 * target and use positive ceilings <= query ceilings. All failures preserve
 * published pixels; provider/font side effects cannot be rolled back. Borrowed
 * resources/font graph remain stable/disjoint until synchronous return; no
 * concurrent/nested render of the same font. Transaction CPU ceiling excludes
 * existing font state and opaque FT/HB/Bidi/provider allocations. During V2,
 * query/render return LIFECYCLE, V1 render/resize return false, void destroy is a
 * no-op and must be retried afterwards. No foreign bindings/other adapter support
 * is implied. See docs/SOFTWARE_V2.md for bounds and coverage semantics. */
cui_error cui_software_render_v2(cui_software *, const cui_draw_list_v2 *, cui_color clear,
                                 const cui_render_v2_options *options);
const unsigned char *cui_software_pixels(const cui_software *renderer, unsigned *width,
                                         unsigned *height);
bool cui_software_save_ppm(const cui_software *renderer, const char *path);
cui_renderer cui_software_interface(cui_software *renderer);
#ifdef __cplusplus
}
#endif
#endif
