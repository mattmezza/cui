/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static bool checked;
static void scene(cui_context *ui, const cui_theme *t) {
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 640, .padding = 16, .gap = 8, .background = t->background});
    cui_label(ui, t, cui_string("Settings"));
    cui_button(ui, 2, t, cui_string("Save"), false);
    cui_checkbox(ui, 3, t, cui_string("Notifications"), &checked, false);
    cui_switch(ui, 4, t, cui_string("Sync"), &checked, true);
    cui_badge(ui, 5, t, cui_string("Preview"));
    cui_progress(ui, 6, t, .5f, 200);
    cui_separator(ui, 7, t, 300);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count > 10);
    assert(cui_last_error(ui) == CUI_OK);
    bool fill = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].bounds.w == 100)
            fill = true;
    assert(fill);
}
static void custom_button(cui_context *ui, void *user) {
    unsigned *calls = user;
    (*calls)++;
    cui_text(ui, cui_string("Custom content"), 12, (cui_color){255, 255, 255, 255});
}
static void test_button_options(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++)
        for (int variant = CUI_BUTTON_PRIMARY; variant <= CUI_BUTTON_LINK; variant++)
            for (int size = CUI_CONTROL_MEDIUM; size <= CUI_CONTROL_LARGE; size++) {
                assert(cui_begin_frame(ui, 400, 200, .016f));
                cui_button_ex(ui, 123, &themes[theme], cui_string("Action"),
                              (cui_button_options){.variant = (cui_button_variant)variant,
                                                   .size = (cui_control_size)size,
                                                   .width = 180,
                                                   .leading = cui_string("+"),
                                                   .trailing = cui_string(">")});
                cui_draw_list list = cui_end_frame(ui);
                size_t count;
                const cui_semantic *nodes = cui_semantics(ui, &count);
                assert(count == 1 && nodes[0].role == CUI_ROLE_BUTTON);
                assert(!nodes[0].disabled && nodes[0].name.length == 6);
                float scale = size == CUI_CONTROL_SMALL   ? .8f
                              : size == CUI_CONTROL_LARGE ? 1.2f
                                                          : 1;
                assert(fabsf(nodes[0].bounds.h - themes[theme].control_height * scale) < .001f);
                assert(nodes[0].bounds.w == 180);
                unsigned text = 0;
                for (size_t i = 0; i < list.count; i++)
                    text += list.commands[i].type == CUI_DRAW_TEXT;
                assert(text == 3 && cui_last_error(ui) == CUI_OK);
            }
    unsigned calls = 0;
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_button_ex(
        ui, 123, &themes[0], cui_string("Accessible action"),
        (cui_button_options){.selected = true, .content = custom_button, .content_user = &calls});
    cui_end_frame(ui);
    assert(calls == 1);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(nodes[0].checked && nodes[0].value == 1 && nodes[0].name.length == 17);
    cui_focus(ui, 123);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_button_ex(ui, 123, &themes[0], cui_string("Saving"),
                          (cui_button_options){.loading = true}));
    cui_end_frame(ui);
    nodes = cui_semantics(ui, &count);
    assert(count == 1 && nodes[0].disabled && !nodes[0].focused && nodes[0].details.busy);
    assert(nodes[0].details.description.length == 7 &&
           memcmp(nodes[0].details.description.data, "Loading", 7) == 0);
    assert(cui_focused(ui) == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_button_ex(ui, 123, &themes[0], cui_string("Saving"),
                          (cui_button_options){.loading = true}));
    cui_end_frame(ui);
    assert(cui_focused(ui) == 0 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

int main(void) {
    test_button_options();
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; i++)
        scene(ui, &themes[i]);
    cui_rect b = cui_bounds(ui, 3);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 5, .y = b.y + 5}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 5, .y = b.y + 5}));
    scene(ui, &themes[0]);
    assert(checked);
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    assert(count == 3);
    assert(s[1].role == CUI_ROLE_CHECKBOX && s[1].checked);
    assert(s[2].disabled && s[2].role == CUI_ROLE_SWITCH);
    assert(themes[0].radius != themes[3].radius && themes[0].primary.b != themes[3].primary.b);
    char buffer[64] = "Ada";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof(buffer),
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 16});
    assert(edit);
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_input_result ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, false);
    assert(!ir.changed && !ir.submitted && ir.error == CUI_EDIT_OK);
    cui_end_frame(ui);
    cui_focus(ui, 44);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_A, .modifiers = CUI_MOD_CTRL}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Eva"}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, false);
    assert(ir.changed && ir.error == CUI_EDIT_OK);
    cui_end_frame(ui);
    assert(cui_edit_get_text(edit).length == 3 && buffer[0] == 'E');
    s = cui_semantics(ui, &count);
    assert(count == 1 && s[0].role == CUI_ROLE_TEXTBOX && s[0].value == 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, false);
    assert(ir.submitted && !ir.changed);
    cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "!"}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, true);
    assert(!ir.changed);
    cui_end_frame(ui);
    assert(cui_edit_get_text(edit).length == 3);
    cui_edit_destroy(edit);
    cui_destroy(ui);
    return 0;
}
