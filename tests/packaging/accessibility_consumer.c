/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <cui/cui.h>
#include <cui/cui_accessibility.h>
static unsigned activated;
static double slider_value;
static void frame(cui_context *ui) {
    assert(cui_begin_frame(ui, 200, 100, .016f));
    cui_begin_box(ui, 42, (cui_box){.width = 100, .height = 30});
    activated +=
        cui_interact(ui, 42, CUI_ROLE_BUTTON, cui_string("Installed button"), false, false, 0)
            .activated;
    cui_end_box(ui);
    cui_begin_box(ui, 43, (cui_box){.width = 100, .height = 20});
    cui_interact(ui, 43, CUI_ROLE_SLIDER, cui_string("Installed slider"), false, false, 0);
    assert(cui_set_accessibility_numeric_actions(ui, 43, CUI_ACCESSIBILITY_NUMERIC_SET_VALUE));
    size_t count = 0;
    (void)cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        cui_accessibility_numeric_input_v1 value;
        if (cui_claim_accessibility_numeric_input_v1(ui, 43, i, sizeof(value), &value))
            slider_value = value.value;
    }
    cui_end_box(ui);
    cui_end_frame(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    cui_accessibility *a = cui_accessibility_create(
        (cui_accessibility_config){.max_nodes = 8, .string_bytes = 128, .max_actions = 4});
    assert(ui && a);
    frame(ui);
    assert(cui_accessibility_update(a, ui,
                                    (cui_accessibility_window){.outer = {0, 0, 200, 100},
                                                               .inner = {0, 0, 200, 100},
                                                               .title = {"Installed consumer", 18},
                                                               .focused = true}));
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
    cui_accessibility_options_v1 options = {.version = CUI_ACCESSIBILITY_OPTIONS_V1,
                                            .flags = CUI_ACCESSIBILITY_ENABLE_NUMERIC};
    a = cui_accessibility_create_with_options_v1((cui_accessibility_config){0}, sizeof(options),
                                                 &options);
    assert(a && cui_accessibility_update(a, ui, (cui_accessibility_window){0}));
    cui_accessibility_set_value_v1 value = {.version = 1, .value = .625};
    assert(cui_accessibility_request_set_value_v1(a, cui_accessibility_token(a, 43), sizeof(value),
                                                  &value));
    assert(cui_accessibility_drain(a, ui) == 1);
    frame(ui);
    assert(slider_value == .625 && cui_focused(ui) == 42 && activated == 1);
    cui_accessibility_destroy(a);
    cui_destroy(ui);
    return 0;
}
