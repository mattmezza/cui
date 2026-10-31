/* SPDX-License-Identifier: MIT */
/* Exercise the Core implementation with standalone and umbrella declarations.
 * The umbrella route deliberately does not compile unrelated implementations. */
#define CUI_IMPLEMENTATION
#ifdef CUI_ELEMENT_SHADOW_UMBRELLA
#include "cui_all.h"
#else
#include "cui.h"
#endif
#include <assert.h>
int main(void) {
    cui_box_shadow layer;
    assert(cui_box_shadow_from_css(0, 2, 8, -1, (cui_color){0, 0, 0, 50}, CUI_BOX_SHADOW_DROP,
                                   &layer));
    assert(layer.layer.sigma == 4);
    cui_context *ui = cui_create((cui_config){.max_elements = 16, .max_commands = 32});
    assert(ui && cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 20, .height = 20, .background = {255, 255, 255, 255}});
    cui_end_box(ui);
    assert(cui_element_set_box_shadows(ui, 1, &layer, 1, (cui_corner_radii){0}));
    assert(cui_element_set_shadow_preset(ui, 1, CUI_SHADOW_MD, (cui_corner_radii){0}));
    cui_draw_list list = cui_end_frame(ui);
    unsigned shadows = 0;
    for (size_t i = 0; i < list.count; i++)
        shadows += list.commands[i].type == CUI_DRAW_DROP_SHADOW;
    assert(shadows == 2 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    return 0;
}
