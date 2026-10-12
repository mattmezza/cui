/* SPDX-License-Identifier: MIT */
#include "cui/cui_icon.h"
#include <assert.h>
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#define NANOSVG_IMPLEMENTATION
#include "../../third_party/nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../third_party/nanosvg/nanosvgrast.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
int main(void) {
    char svg[] = "<svg width=\"8\" height=\"8\"><rect width=\"8\" height=\"8\"/></svg>";
    NSVGimage *image = nsvgParse(svg, "px", 96);
    NSVGrasterizer *rasterizer = nsvgCreateRasterizer();
    unsigned char pixels[8 * 8 * 4] = {0};
    assert(image && rasterizer);
    nsvgRasterize(rasterizer, image, 0, 0, 1, pixels, 8, 8, 32);
    assert(pixels[3] == 255);
    cui_icon_state state;
    cui_icon_state_init(&state);
    assert(cui_icon_set_name(&state, cui_string("check"), NULL, NULL));
    cui_icon_state_destroy(&state);
    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(image);
    return 0;
}
