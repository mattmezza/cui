/* SPDX-License-Identifier: MIT */
#include "cui/cui_color_entry.h"
#include "cui/cui_color_hsla_entry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool admit(void *user, cui_str text) {
    (void)user;
    if (!text.length)
        return true;
    if (text.data[0] != '#' || text.length > 9)
        return false;
    for (size_t i = 1; i < text.length; ++i)
        if (!((text.data[i] >= '0' && text.data[i] <= '9') ||
              (text.data[i] >= 'a' && text.data[i] <= 'f')))
            return false;
    return true;
}
/* Browser-style keydown/TEXT/keyup cadence, one character per frame.
 * Keydown prefers the following TEXT; it must neither duplicate nor discard it. */
static void run(unsigned kind, const char *text) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    assert(ui);
    char buffer[10] = "#abcdef";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    assert(edit && cui_edit_get_text(edit).length == 7);
    cui_input_state input = {0};
    cui_color_entry_state bytes = {0};
    cui_color_hsla_entry_state floats = {0};
    assert(cui_color_entry_init(&bytes, (cui_color_entry_value){true, {255, 0, 0, 255}}));
    assert(cui_color_hsla_entry_init(&floats, (cui_color_hsla_value){true, {0, 1, .5f, 1}}) ==
           CUI_COLOR_HSLA_ENTRY_OK);
    cui_theme theme = cui_theme_light();
    size_t count = strlen(text);
    for (size_t frame = 0; frame < count + 2; ++frame) {
        if (frame == 1) {
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = CUI_KEY_A,
                                                   .modifiers = CUI_MOD_CTRL}));
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_A}));
        }
        if (frame && frame <= count) {
            cui_key key = frame == 1 ? CUI_KEY_NONE : (cui_key)(CUI_KEY_0 + text[frame - 1] - '0');
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = key,
                                                   .prefer_character_input = true}));
            cui_event event = {.type = CUI_EVENT_TEXT};
            event.text[0] = text[frame - 1];
            assert(cui_queue_event(ui, event));
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = key}));
        }
        if (frame == count + 1)
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        assert(cui_begin_frame(ui, 400, 120, .016f));
        cui_begin_box(ui, 1, (cui_box){0});
        if (!kind)
            (void)cui_input_ex(ui, 10, &theme, cui_string("Plain hex"), edit, &input,
                               (cui_input_options){.validate = admit});
        else if (kind == 1) {
            cui_color_entry_result r =
                cui_color_entry(ui, 10, &theme, cui_string("Byte hex"), &bytes,
                                (cui_color_entry_options){0}, (cui_color_entry_handler){0});
            if (frame == count + 1) {
                assert(r.commit_count == (count == 9));
                assert(r.rejected_commit_count == (count == 3));
            }
        } else {
            cui_color_hsla_entry_result r = cui_color_hsla_entry(
                ui, 10, &theme, cui_string("Float hex"), &floats, (cui_color_hsla_entry_options){0},
                (cui_color_hsla_entry_handler){0});
            if (frame == count + 1) {
                assert(r.commit_count == (count == 9));
                assert(r.rejected_commit_count == (count == 3));
            }
        }
        cui_end_box(ui);
        (void)cui_end_frame(ui);
        if (!frame) {
            cui_focus(ui, 10);
            continue;
        }
        assert(cui_focused(ui) == 10);
        const char *actual = !kind ? buffer : kind == 1 ? bytes.draft : floats.draft;
        size_t length = frame <= count ? frame : count;
        if (strlen(actual) != length || strncmp(actual, text, length)) {
            fprintf(stderr, "kind=%u frame=%zu expected prefix length=%zu actual='%s'\n", kind,
                    frame, length, actual);
            assert(false);
        }
    }
    cui_color_hsla_entry_destroy(&floats);
    cui_color_entry_destroy(&bytes);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
int main(void) {
    for (unsigned i = 0; i < 3; ++i) {
        run(i, "#12");
        run(i, "#12345678");
    }
    return 0;
}
