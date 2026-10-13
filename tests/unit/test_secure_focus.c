/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
static void frame(cui_context *ui, bool present, bool disabled, bool readonly, bool multiline,
                  bool secure) {
    assert(cui_begin_frame(ui, 300, 100, .016f));
    cui_text_input_state untouched = {.owner = 99, .generation = 71};
    assert(!cui_get_text_input_config(ui, &untouched) && untouched.owner == 99 &&
           untouched.generation == 71);
    if (present) {
        cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 40, .fixed_height = true});
        cui_interact(ui, 10, CUI_ROLE_TEXTBOX, cui_string("Field"), disabled, false, 0);
        cui_set_semantic_details(
            ui, 10, (cui_semantic_details){.readonly = readonly, .multiline = multiline});
        if (secure)
            assert(cui_set_text_input_config(
                       ui, 10,
                       (cui_text_input_config){CUI_TEXT_INPUT_PASSWORD_HIDDEN,
                                               CUI_TEXT_INPUT_PURPOSE_PASSWORD}) == !multiline);
        assert(!cui_set_text_input_config(
            ui, 10, (cui_text_input_config){.type = (cui_text_input_type)99}));
        cui_end_box(ui);
    }
    cui_end_frame(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_text_input_state s = {.owner = 99, .generation = 71};
    assert(!cui_get_text_input_config(ui, &s) && s.owner == 99 && s.generation == 71);
    frame(ui, true, false, false, false, false);
    cui_focus(ui, 10);
    frame(ui, true, false, false, false, false);
    assert(cui_get_text_input_config(ui, &s) && s.owner == 10 && !s.explicit_config &&
           s.config.type == CUI_TEXT_INPUT_TEXT);
    uint64_t generation = s.generation;
    frame(ui, true, false, false, true, false);
    assert(cui_get_text_input_config(ui, &s) && s.generation > generation && !s.explicit_config &&
           s.multiline);
    frame(ui, true, false, false, true, true);
    assert(cui_get_text_input_config(ui, &s) && s.config.type == CUI_TEXT_INPUT_TEXT);
    frame(ui, true, false, false, false, true);
    assert(cui_get_text_input_config(ui, &s) && s.explicit_config && s.generation > generation);
    generation = s.generation;
    frame(ui, true, false, false, false, true);
    assert(cui_get_text_input_config(ui, &s) && s.generation == generation);
    frame(ui, true, false, false, false, false);
    assert(cui_get_text_input_config(ui, &s) && !s.explicit_config && s.generation > generation);
    frame(ui, true, false, true, false, false);
    assert(!cui_get_text_input_config(ui, &s));
    frame(ui, true, true, false, false, false);
    assert(!cui_get_text_input_config(ui, &s));
    frame(ui, false, false, false, false, false);
    assert(!cui_get_text_input_config(ui, &s));
    frame(ui, true, false, false, false, true);
    cui_focus(ui, 10);
    frame(ui, true, false, false, false, true);
    assert(cui_get_text_input_config(ui, &s));
    cui_focus(ui, 0);
    assert(!cui_get_text_input_config(ui, &s));
    cui_destroy(ui);
    return 0;
}
