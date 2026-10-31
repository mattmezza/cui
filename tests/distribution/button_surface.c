/* SPDX-License-Identifier: MIT */
/* Compile the scoped generated implementation itself, never the full all target. */
#define CUI_BUTTON_STYLES_IMPLEMENTATION
#ifdef CUI_BUTTON_SURFACE_UMBRELLA
#include "cui_all.h"
#include "cui_button_styles.h"
#else
#include "cui_button_styles.h"
#endif
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
static void shadow(void *user, const cui_button_style_info *info,
                   const cui_button_surface_paint *current, cui_button_surface_patch *out) {
    (void)current;
    assert(info->phase == CUI_BUTTON_STYLE_INSTANCE);
    out->fields = CUI_BUTTON_SURFACE_SHADOWS;
    out->value.shadow_count = 1;
    out->value.shadows[0] = *(const cui_box_shadow *)user;
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
    assert(ui && cui_begin_frame(ui, 320, 160, .016f));
    cui_theme theme = cui_theme_light();
    cui_box_shadow value;
    assert(
        cui_box_shadow_from_css(0, 2, 4, 0, (cui_color){0, 0, 0, 80}, CUI_BOX_SHADOW_DROP, &value));
    cui_button_surface_status status = {63};
    assert(!cui_button_surface_ex(ui, 10, &theme, cui_string("Save"), (cui_button_options){0},
                                  (cui_button_surface_styles){.instance = shadow, .user = &value},
                                  &status));
    cui_draw_list list = cui_end_frame(ui);
    unsigned shadows = 0;
    for (size_t i = 0; i < list.count; ++i)
        shadows += list.commands[i].type == CUI_DRAW_DROP_SHADOW;
    assert(cui_last_error(ui) == CUI_OK && status.rejected_phases == 0 && shadows == 1);
    cui_destroy(ui);
    return 0;
}
