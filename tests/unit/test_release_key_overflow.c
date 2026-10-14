/* SPDX-License-Identifier: MIT
 * Private Core fixture: compile this file alone, with -Iinclude -Ithird_party/clay.
 * No production test hook or exported ABI is needed to drive generation overflow. */
#include "../../src/core/cui.c"
#include <assert.h>
#include <stdio.h>
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){text.length * size / 2, size};
}
static bool scene(cui_context *ui, bool opt) {
    assert(cui_begin_frame(ui, 100, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 100, .height = 100});
    cui_interaction result =
        cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0);
    cui_set_semantic_details(ui, 10, (cui_semantic_details){.release_key_activation = opt});
    cui_end_box(ui);
    assert(cui_end_frame(ui).commands);
    return result.activated;
}
static bool key(cui_context *ui, cui_event_type type, bool opt) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .key = CUI_KEY_ENTER}));
    return scene(ui, opt);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    assert(ui);
    assert(!scene(ui, true));
    cui_focus(ui, 10);
    ui->focus_generation = UINT64_MAX;
    cui_focus(ui, 0);
    assert(ui->release_generation_exhausted);
    cui_focus(ui, 10);
    assert(!key(ui, CUI_EVENT_KEY_DOWN, true));
    assert(!key(ui, CUI_EVENT_KEY_UP, true));
    assert(!scene(ui, false));
    assert(key(ui, CUI_EVENT_KEY_DOWN, false)); /* legacy unaffected */
    cui_destroy(ui);
    puts("release key overflow ok");
}
