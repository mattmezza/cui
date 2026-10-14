/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
float *cui_canvas_shadow_values(void);
int shadow_preset_layer(int preset, unsigned index) {
    cui_drop_shadow layers[CUI_SHADOW_PRESET_MAX];
    size_t count;
    if (!cui_shadow_preset_layers((cui_shadow_preset)preset, layers, CUI_SHADOW_PRESET_MAX, &count))
        return -1;
    if (index >= count)
        return (int)count;
    cui_drop_shadow s = layers[index];
    float *v = cui_canvas_shadow_values();
    const float copied[16] = {20,        20,        64,         64,         0,       0,
                              0,         0,         s.offset_x, s.offset_y, s.sigma, s.spread,
                              s.color.r, s.color.g, s.color.b,  s.color.a};
    for (unsigned i = 0; i < 16; i++)
        v[i] = copied[i];
    return (int)count;
}
