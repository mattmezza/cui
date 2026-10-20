/* SPDX-License-Identifier: MIT */
#include "cui/cui_color_hsla_entry.h"
#include <assert.h>
typedef struct {
    cui_color_hsla_value value;
    bool pending;
} owner;
static void accept(void *user, const cui_color_hsla_entry_result *event) {
    owner *model = user;
    model->value = event->committed;
    model->pending = true; /* Apply to entry only before the next submission. */
}
int main(void) {
    owner model = {.value = {true, {.1234567f, .2345678f, .3456789f, .5f}}};
    cui_color_hsla_entry_state entry = {0};
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    cui_theme theme = cui_theme_light();
    assert(ui && cui_color_hsla_entry_init(&entry, model.value) == CUI_COLOR_HSLA_ENTRY_OK);
    for (unsigned frame = 0; frame < 3; ++frame) {
        if (model.pending) {
            assert(cui_color_hsla_entry_sync(&entry, model.value, false) ==
                   CUI_COLOR_HSLA_ENTRY_OK);
            model.pending = false;
        }
        if (frame == 1) {
            cui_focus(ui, 10);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = CUI_KEY_A,
                                                   .modifiers = CUI_MOD_CTRL}));
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "#0f08"}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        }
        assert(cui_begin_frame(ui, 400, 120, .016f));
        cui_begin_box(ui, 1, (cui_box){0});
        cui_color_hsla_entry_result result =
            cui_color_hsla_entry(ui, 10, &theme, cui_string("Accent hex"), &entry,
                                 (cui_color_hsla_entry_options){.width = 240},
                                 (cui_color_hsla_entry_handler){accept, &model});
        assert(result.input.error == CUI_EDIT_OK);
        cui_end_box(ui);
        (void)cui_end_frame(ui); /* Render/consume draw list before next iteration. */
    }
    assert(model.value.present && model.value.color.s == 1 && model.value.color.a == 136 / 255.f);
    cui_destroy(ui);
    cui_color_hsla_entry_destroy(&entry);
    return 0;
}
