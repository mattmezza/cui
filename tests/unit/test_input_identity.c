/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_numeric.h"
#include <assert.h>
#include <string.h>

static void pair(bool number, cui_id second, cui_id focused) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char first_bytes[128] = "12", second_bytes[128] = "second";
    cui_edit *first = cui_edit_create((cui_edit_config){.buffer = first_bytes,
                                                       .capacity = sizeof first_bytes});
    cui_edit *other = cui_edit_create((cui_edit_config){.buffer = second_bytes,
                                                       .capacity = sizeof second_bytes});
    cui_input_state inputs[2];
    memset(inputs, 0, sizeof inputs);
    cui_number_state numeric = {.edit = first};
    double value = 12;
    cui_theme theme = cui_theme_light();
    assert(ui && first && other);
    for (unsigned frame = 0; frame < 3; ++frame) {
        if (frame == 1) cui_focus(ui, focused);
        if (frame == 2)
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "7"}));
        assert(cui_begin_frame(ui, 500, 240, .016f));
        cui_begin_box(ui, 2, (cui_box){.width = 300, .height = 200});
        if (number)
            assert(cui_number_input(ui, 3, &theme, cui_string("Number"), &numeric, &value,
                                    (cui_number_options){.width = 200, .step = 1}).error == CUI_EDIT_OK);
        else
            assert(cui_input_ex(ui, 3, &theme, cui_string("First"), first, &inputs[0],
                                (cui_input_options){.width = 200}).error == CUI_EDIT_OK);
        assert(cui_input_ex(ui, second, &theme, cui_string("Second"), other, &inputs[1],
                            (cui_input_options){.width = 200}).error == CUI_EDIT_OK);
        cui_end_box(ui);
        assert(cui_end_frame(ui).count > 0);
        assert(cui_last_error(ui) == CUI_OK);
        assert(cui_bounds(ui, cui_input_viewport_id(3)).w > 0);
        assert(cui_bounds(ui, cui_input_viewport_id(second)).w > 0);
        if (frame) {
            cui_rect caret;
            assert(cui_text_input_rect(ui, &caret) && caret.h > 0);
            assert(cui_focused(ui) == focused);
        }
    }
    if (focused == second) {
        assert(strchr(second_bytes, '7'));
        assert(!strchr(first_bytes, '7'));
    } else {
        assert(strchr(first_bytes, '7'));
        assert(!strchr(second_bytes, '7'));
    }
    cui_edit_destroy(other);
    cui_edit_destroy(first);
    cui_destroy(ui);
}
int main(void) {
    pair(false, 4, 4); /* Old text wrapper of 3 collided with viewport of 4. */
    pair(true, 4, 4);  /* NumberInput embeds the same Input painter. */
    pair(false, 5, 3); /* Old caret of 3 collided with viewport of 5. */
    pair(true, 5, 3);
    return 0;
}
