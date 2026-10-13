/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
static unsigned calls;
static void clicked(void *user) {
    ++*(unsigned *)user;
}
static bool scene(cui_context *ui, bool disabled, bool loading) {
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_theme theme = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    bool activated = cui_button_ex(ui, 10, &theme, cui_string("Run"),
                                   (cui_button_options){.width = 100,
                                                        .height = 40,
                                                        .disabled = disabled,
                                                        .loading = loading,
                                                        .on_click = clicked,
                                                        .action_user = &calls});
    cui_end_box(ui);
    cui_end_frame(ui);
    return activated;
}
static void click(cui_context *ui) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 20, .y = 20}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 20, .y = 20}));
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    scene(ui, false, false);
    scene(ui, false, false);
    click(ui);
    click(ui);
    assert(scene(ui, false, false));
    assert(calls == 2);
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    assert(scene(ui, false, false));
    assert(calls == 3);
    scene(ui, false, true);
    click(ui);
    assert(!scene(ui, false, true));
    assert(calls == 3);
    scene(ui, true, false);
    click(ui);
    assert(!scene(ui, true, false));
    assert(calls == 3);
    scene(ui, false, false);
    click(ui);
    assert(scene(ui, false, false));
    assert(calls == 4);
    cui_destroy(ui);
}
