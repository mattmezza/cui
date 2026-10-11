/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include <stdio.h>
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    if (!ui)
        return 1;
    bool value = false;
    for (int frame = 0; frame < 2; frame++) {
        if (frame == 1) {
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 10, .y = 10});
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 10, .y = 10});
        }
        cui_begin_frame(ui, 400, 200, .016f);
        cui_begin_box(ui, 1,
                      (cui_box){.width = 180,
                                .height = 40,
                                .padding = 8,
                                .background = {240, 218, 165, 255},
                                .border = {80, 60, 30, 255},
                                .border_width = 2});
        cui_base_checkbox(ui, 1, cui_string("Custom checkbox"), &value, false);
        cui_text(ui, cui_string(value ? "Enabled" : "Enable feature"), 16,
                 (cui_color){40, 30, 10, 255});
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        printf("unstyled frame %d: %zu commands, checked=%d\n", frame, list.count, value);
    }
    cui_destroy(ui);
    return value ? 0 : 1;
}
