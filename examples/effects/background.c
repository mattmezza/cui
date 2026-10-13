/* SPDX-License-Identifier: MIT */
#include "background.h"
bool cui_example_background_init(cui_example_background *b) {
    if (!b)
        return false;
    const cui_gradient_stop stops[] = {{0, {12, 23, 43, 255}},
                                       {.45f, {29, 49, 77, 255}},
                                       {.78f, {36, 76, 89, 255}},
                                       {1, {62, 103, 112, 255}}};
    cui_effect_buffer output = {b->pixels, 128, 128, 128 * 4, sizeof(b->pixels)};
    if (!cui_effect_linear_gradient(output, stops, 4, 35))
        return false;
    b->revision = 1;
    return true;
}
bool cui_example_background_paint(cui_context *ui, cui_id id, const cui_example_background *b,
                                  float w, float h) {
    if (!b)
        return false;
    cui_bitmap_paint paint = {.bitmap = {b->pixels, 128, 128, 128 * 4, b->revision}, .opacity = 1};
    return cui_paint_bitmap(ui, id, paint, (cui_box){.width = w, .height = h, .radius = 18});
}
