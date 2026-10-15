/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"

/* Application submission is portable; the host keeps clicked alive across frames. */
void app_frame(cui_context *ui, const cui_theme *theme, bool *clicked) {
    cui_begin_box(ui, cui_id_from(ui, "root"),
                  (cui_box){.width = -1,
                            .height = -1,
                            .padding = 32,
                            .gap = 18,
                            .background = {245, 247, 250, 255}});
    cui_text_ex(ui, cui_string("Hello from CUI"), 28, (cui_color){28, 35, 48, 255}, 0,
                (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD});
    cui_text(ui, cui_string("Inter Regular and Bold, loaded before layout."), 17,
             (cui_color){55, 65, 80, 255});
    if (cui_button(ui, cui_id_from(ui, "hello"), theme, cui_string("Say hello"), false))
        *clicked = !*clicked;
    cui_text(
        ui,
        cui_string(*clicked ? "Hello! Click again to reset." : "Click the button to say hello."),
        17, (cui_color){55, 65, 80, 255});
    cui_end_box(ui);
}
