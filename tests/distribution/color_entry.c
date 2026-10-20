/* SPDX-License-Identifier: MIT */
#define CUI_IMPLEMENTATION
#include "cui.h"
#define CUI_BASE_IMPLEMENTATION
#include "cui_base.h"
#define CUI_COLOR_ENTRY_IMPLEMENTATION
#include "cui_color_entry.h"
#include <assert.h>
int main(void) {
    cui_color_entry_state entry = {0};
    assert(cui_color_entry_init(&entry, (cui_color_entry_value){0}));
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    assert(ui);
    cui_theme theme = cui_theme_light();
    for (unsigned frame = 0; frame < 2; ++frame) {
        if (frame) {
            cui_focus(ui, 10);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "#abc"}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        }
        assert(cui_begin_frame(ui, 400, 120, .016f));
        cui_begin_box(ui, 1, (cui_box){0});
        cui_color_entry_result result =
            cui_color_entry(ui, 10, &theme, cui_string("Hex color"), &entry,
                            (cui_color_entry_options){0}, (cui_color_entry_handler){0});
        assert(result.input.error == CUI_EDIT_OK && result.commit_count == frame);
        cui_end_box(ui);
        assert(cui_end_frame(ui).count);
    }
    assert(entry.committed.present && entry.committed.color.r == 170);
    cui_destroy(ui);
    cui_color_entry_destroy(&entry);
    return 0;
}
