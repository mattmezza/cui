/* SPDX-License-Identifier: MIT */
/* Private boundary proof; no testing setter is added to the public ABI. */
#include "../../src/core/cui.c"
#include <assert.h>
static void frame(cui_context *ui, bool secure, bool modal) {
    assert(cui_begin_frame(ui, 300, 100, .016f));
    cui_begin_box(ui, 10, (cui_box){.width = 100, .height = 40});
    cui_interact(ui, 10, CUI_ROLE_TEXTBOX, cui_string("Field"), false, false, 0);
    if (secure)
        assert(cui_set_text_input_config(ui, 10,
                                         (cui_text_input_config){CUI_TEXT_INPUT_PASSWORD_HIDDEN,
                                                                 CUI_TEXT_INPUT_PURPOSE_PASSWORD}));
    cui_end_box(ui);
    if (modal) {
        assert(cui_begin_focus_scope(ui, 50, true));
        cui_begin_box(ui, 20, (cui_box){.width = 100, .height = 40});
        cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("Modal"), false, false, 0);
        cui_end_box(ui);
        cui_end_focus_scope(ui);
    }
    cui_end_frame(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    frame(ui, false, false);
    cui_focus(ui, 10);
    frame(ui, false, false);
    cui_text_input_state s;
    assert(cui_get_text_input_config(ui, &s));
    frame(ui, true, true);
    assert(!cui_get_text_input_config(ui, &s));
    frame(ui, false, false);
    cui_focus(ui, 10);
    frame(ui, false, false);
    assert(cui_get_text_input_config(ui, &s));
    ui->text_input_state.generation = UINT64_MAX;
    frame(ui, true, false);
    s = (cui_text_input_state){.owner = 999, .generation = 42};
    assert(!cui_get_text_input_config(ui, &s) && s.owner == 999 && s.generation == 42 &&
           ui->text_input_exhausted);
    frame(ui, false, false);
    assert(!cui_get_text_input_config(ui, &s));
    cui_focus(ui, 0);
    cui_focus(ui, 10);
    frame(ui, true, false);
    assert(!cui_get_text_input_config(ui, &s));
    cui_destroy(ui);
    return 0;
}
