/* SPDX-License-Identifier: MIT */
#define probe_frame inherited_probe_frame
#include "button_blocker_probe.c"
#undef probe_frame
static unsigned numeric_mode;
void numeric_config(unsigned value) {
    numeric_mode = value;
}
int probe_frame(void) {
    cui_theme t = cui_theme_light();
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 360, .height = 300, .padding = 10, .background = {245, 245, 245, 255}});
    cui_button_ex(ui, 20, &t, cui_string("Open"),
                  (cui_button_options){.pixel_size = 80,
                                       .dropdown_caret = true,
                                       .icon_only = numeric_mode == 1,
                                       .open = numeric_mode == 2,
                                       .disabled = numeric_mode == 3,
                                       .loading = numeric_mode == 4,
                                       .on_click = child_click,
                                       .action_user = &children});
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    return 0;
}
