/* SPDX-License-Identifier: MIT */
#include <cui/cui.h>
#include <cui/cui_accessibility.h>
#include <assert.h>
static unsigned activated;
static void frame(cui_context *ui) {
    assert(cui_begin_frame(ui, 200, 100, .016f));
    cui_begin_box(ui, 42, (cui_box){.width = 100, .height = 30});
    activated += cui_interact(ui, 42, CUI_ROLE_BUTTON, cui_string("Installed button"), false, false, 0).activated;
    cui_end_box(ui);
    cui_end_frame(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    cui_accessibility *a = cui_accessibility_create((cui_accessibility_config){.max_nodes = 8, .string_bytes = 128, .max_actions = 4});
    assert(ui && a);
    frame(ui);
    assert(cui_accessibility_update(a, ui, (cui_accessibility_window){
        .outer = {0, 0, 200, 100}, .inner = {0, 0, 200, 100}, .title = {"Installed consumer", 18}, .focused = true}));
    uint64_t token = cui_accessibility_token(a, 42);
    assert(token && cui_accessibility_request(a, token, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_accessibility_drain(a, ui) == 1);
    frame(ui);
    assert(activated == 1);
    frame(ui);
    assert(activated == 1);
    assert(cui_accessibility_request(a, token, CUI_ACCESSIBILITY_FOCUS));
    assert(cui_accessibility_drain(a, ui) == 1);
    frame(ui);
    assert(cui_focused(ui) == 42 && activated == 1);
    cui_accessibility_destroy(a);
    cui_destroy(ui);
    return 0;
}
