/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void press(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect r = {0};
    assert(cui_bridge_node_bounds(app, node, &r) == CUI_BRIDGE_OK);
    float x = r.x + r.width * .5f, y = r.y + r.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}
static void test_nested_group_stack(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
        .abi_version = CUI_BRIDGE_ABI_VERSION, .backend = CUI_BRIDGE_BACKEND_NONE,
        .width = 600, .height = 400, .max_nodes = 256};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node parent = {0}, root = {0};
    assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    parent = root;
    /* Each selected Item renders another Tabs group through its panel callback. */
    for (unsigned i = 0; i < 16; ++i) {
        cui_bridge_node tabs = {0}, item = {0};
        char tab_key[32], item_key[32];
        snprintf(tab_key, sizeof tab_key, "tabs-%u", i);
        snprintf(item_key, sizeof item_key, "item-%u", i);
        assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_TABS, tab_key, &tabs) == CUI_BRIDGE_OK);
        assert(cui_bridge_node_create(app, tabs, CUI_BRIDGE_ITEM, item_key, &item) == CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(app, item, "Nested") == CUI_BRIDGE_OK);
        assert(cui_bridge_set_selected(app, tabs, item) == CUI_BRIDGE_OK);
        parent = item;
    }
    cui_bridge_node leaf = {0};
    assert(cui_bridge_node_create(app, parent, CUI_BRIDGE_LABEL, "leaf", &leaf) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, leaf, "End") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
int main(void) {
    test_nested_group_stack();
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 600,
                            .height = 400};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, first = {0}, second = {0}, slider = {0}, progress = {0};
    cui_bridge_node input = {0}, area = {0}, number = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CHECKBOX, "first", &first) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_CHECKBOX, "second", &second) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SLIDER, "slider", &slider) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_PROGRESS, "progress", &progress) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_INPUT, "input", &input) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TEXTAREA, "area", &area) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_NUMBER_INPUT, "number", &number) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, first, "First \xE2\x9C\x93") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, first, "\xC0\xAF") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_text(app, second, "Second") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_range(app, slider, 10, 100, 1) == CUI_BRIDGE_OK);
    double value = -1;
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == 10);
    assert(cui_bridge_set_range(app, slider, -50, -10, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == -10);
    assert(cui_bridge_set_range(app, slider, -INFINITY, 10, 1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_range(app, slider, 0, 1e100, 1) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == -10);
    assert(cui_bridge_set_range(app, slider, 0, 10, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, slider, 5) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, progress, .5) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, slider, NAN) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_text(app, input, "hello") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, area, "first\nsecond") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, number, "Count") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_range(app, number, 0, 20, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, number, 10) == CUI_BRIDGE_OK);
    char text[32] = "preserved";
    uint32_t needed = 0;
    assert(cui_bridge_get_text(app, input, text, 3, &needed) == CUI_BRIDGE_CAPACITY);
    assert(needed == 6 && text[0] == 'p');
    assert(cui_bridge_get_text(app, input, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(needed == 6 && text[0] == 'h');
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK && value == 5);
    assert(cui_bridge_get_value(app, number, &value) == CUI_BRIDGE_OK && value == 10);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, first);
    uint32_t checked = 0;
    assert(cui_bridge_get_checked(app, first, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_get_checked(app, second, &checked) == CUI_BRIDGE_OK && checked == 0);
    cui_bridge_event_ex event = {0};
    char payload[32] = {0};
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == first.slot &&
           event.frame_sequence && event.checked == 1 && needed == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    press(app, first);
    press(app, first);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.checked == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.checked == 1);
    assert(cui_bridge_queue_text(app, "\xC0\xAF") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_disabled(app, input, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, input);
    assert(cui_bridge_queue_text(app, "X") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, input, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(needed == 6);
    assert(cui_bridge_set_disabled(app, input, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, input);
    assert(cui_bridge_queue_text(app, "Z") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, input, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(needed == 7);
    assert(cui_bridge_poll_event_ex(app, &event, payload, 2, &needed) == CUI_BRIDGE_CAPACITY);
    assert(needed == 7);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == input.slot &&
           event.text_length == 6 && needed == 7);
    assert(cui_bridge_queue_text(app, "Y") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "Q") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.text_length == 7 && needed == 8);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.text_length == 8 && needed == 9);
    assert(cui_bridge_set_disabled(app, second, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, second);
    assert(cui_bridge_get_checked(app, second, &checked) == CUI_BRIDGE_OK && checked == 0);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_node_remove(app, first) == CUI_BRIDGE_OK);
    checked = 99;
    assert(cui_bridge_get_checked(app, first, &checked) == CUI_BRIDGE_STALE_HANDLE &&
           checked == 99);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node radio = {0}, sw = {0}, toggle = {0}, badge = {0};
    cui_bridge_node collapsible = {0}, panel_label = {0}, alert = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RADIO, "radio", &radio) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SWITCH, "switch", &sw) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TOGGLE, "toggle", &toggle) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BADGE, "badge", &badge) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLLAPSIBLE, "details", &collapsible) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, collapsible, CUI_BRIDGE_LABEL, "inside", &panel_label) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ALERT, "alert", &alert) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, radio, "Radio") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, sw, "Switch") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, toggle, "Toggle") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, badge, "New") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, collapsible, "Details") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, panel_label, "Panel body") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, alert, "Heads up") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, alert, "Action is required") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, badge, &bounds) == CUI_BRIDGE_OK && bounds.width > 0);
    assert(cui_bridge_node_bounds(app, panel_label, &bounds) == CUI_BRIDGE_INVALID_STATE);
    press(app, collapsible);
    assert(cui_bridge_get_checked(app, collapsible, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == collapsible.slot && event.checked == 1);
    assert(cui_bridge_node_bounds(app, panel_label, &bounds) == CUI_BRIDGE_OK);
    press(app, radio);
    assert(cui_bridge_get_checked(app, radio, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == radio.slot && event.checked == 1);
    press(app, sw);
    assert(cui_bridge_get_checked(app, sw, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == sw.slot && event.checked == 1);
    press(app, toggle);
    assert(cui_bridge_get_checked(app, toggle, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == toggle.slot && event.checked == 1);
    assert(cui_bridge_set_disabled(app, sw, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, sw);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node select = {0}, select_a = {0}, select_b = {0};
    cui_bridge_node combo = {0}, combo_a = {0};
    cui_bridge_node dropdown = {0}, dropdown_a = {0};
    cui_bridge_node selected = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SELECT, "select", &select) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, select, CUI_BRIDGE_ITEM, "select-a", &select_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, select, CUI_BRIDGE_ITEM, "select-b", &select_b) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, select, "Select one") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, select, "Choose") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, select_a, "Apple") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, select_b, "Banana") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COMBOBOX, "combo", &combo) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, combo, CUI_BRIDGE_ITEM, "combo-a", &combo_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, combo, "Search") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, combo_a, "Cherry") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DROPDOWN_BUTTON, "dropdown", &dropdown) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, dropdown, CUI_BRIDGE_ITEM, "dropdown-a", &dropdown_a) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, dropdown, "Run") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, dropdown, "More") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, dropdown_a, "Export") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, select);
    assert(cui_bridge_node_bounds(app, select_b, &bounds) == CUI_BRIDGE_OK);
    press(app, select_b);
    assert(cui_bridge_get_selected(app, select, &selected) == CUI_BRIDGE_OK &&
           selected.slot == select_b.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.selected.slot == select_b.slot &&
           event.selected.generation == select_b.generation);
    assert(cui_bridge_set_text(app, combo, "Ch") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, combo, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 3);
    press(app, combo);
    assert(cui_bridge_queue_text(app, "e") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_text(app, combo, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 4);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == combo.slot && event.text_length == 3);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ESCAPE, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_dropdown_part_bounds(app, dropdown, 0, &bounds) == CUI_BRIDGE_OK &&
           bounds.width > 0);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width * .25f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width * .25f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event legacy = {0};
    assert(cui_bridge_poll_event(app, &legacy) == CUI_BRIDGE_OK);
    assert(legacy.type == CUI_BRIDGE_EVENT_CLICK && legacy.node.slot == dropdown.slot);
    assert(cui_bridge_dropdown_part_bounds(app, dropdown, 1, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width * .5f,
                                    bounds.y + bounds.height * .5f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, dropdown_a, &bounds) == CUI_BRIDGE_OK);
    press(app, dropdown_a);
    assert(cui_bridge_get_selected(app, dropdown, &selected) == CUI_BRIDGE_OK &&
           selected.slot == dropdown_a.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.selected.slot == dropdown_a.slot);
    assert(cui_bridge_set_disabled(app, select, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, select);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_get_selected(app, select, &selected) == CUI_BRIDGE_OK &&
           selected.slot == select_b.slot);
    assert(cui_bridge_set_text(app, combo, "ZZ") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, combo);
    assert(cui_bridge_node_bounds(app, combo_a, &bounds) == CUI_BRIDGE_INVALID_STATE);
    char too_long[4099];
    for (size_t i = 0; i < sizeof too_long - 1; i++)
        too_long[i] = 'x';
    too_long[sizeof too_long - 1] = 0;
    assert(cui_bridge_set_text(app, combo, too_long) == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_get_text(app, combo, text, sizeof text, &needed) == CUI_BRIDGE_OK &&
           needed == 3);
    cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app);
    root = (cui_bridge_node){0};
    cui_bridge_node group = {0}, option_a = {0}, option_b = {0};
    cui_bridge_node tabs = {0}, tab_a = {0}, tab_b = {0}, panel = {0};
    cui_bridge_node accordion = {0}, fold = {0}, fold_content = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_RADIO_GROUP, "group", &group) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "a", &option_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "b", &option_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, option_a, "A") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, option_b, "B") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_TABS, "tabs", &tabs) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tabs, CUI_BRIDGE_ITEM, "tab-a", &tab_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tabs, CUI_BRIDGE_ITEM, "tab-b", &tab_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, tab_b, CUI_BRIDGE_LABEL, "panel", &panel) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, tab_a, "First tab") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, tab_b, "Second tab") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, panel, "Second content") == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ACCORDION, "accordion", &accordion) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, accordion, CUI_BRIDGE_ITEM, "fold", &fold) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, fold, CUI_BRIDGE_LABEL, "fold-content", &fold_content) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, fold, "Expand") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, fold_content, "Expanded content") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, option_a);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == option_a.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == group.slot && event.value == option_a.slot &&
           event.selected.slot == option_a.slot &&
           event.selected.generation == option_a.generation);
    press(app, option_b);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == option_b.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.value == option_b.slot && event.selected.generation == option_b.generation);
    assert(cui_bridge_set_selected(app, tabs, tab_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, panel, &bounds) == CUI_BRIDGE_INVALID_STATE);
    press(app, tab_b);
    assert(cui_bridge_get_selected(app, tabs, &selected) == CUI_BRIDGE_OK &&
           selected.slot == tab_b.slot);
    assert(cui_bridge_node_bounds(app, panel, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == tabs.slot && event.value == tab_b.slot &&
           event.selected.generation == tab_b.generation);
    press(app, fold);
    assert(cui_bridge_get_checked(app, fold, &checked) == CUI_BRIDGE_OK && checked == 1);
    assert(cui_bridge_node_bounds(app, fold_content, &bounds) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.node.slot == fold.slot && event.checked == 1);
    assert(cui_bridge_node_remove(app, option_b) == CUI_BRIDGE_OK);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK && selected.slot == 0);
    cui_bridge_node replacement = {0};
    assert(cui_bridge_node_create(app, group, CUI_BRIDGE_ITEM, "replacement", &replacement) ==
           CUI_BRIDGE_OK);
    assert(replacement.slot == option_b.slot && replacement.generation != option_b.generation);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK && selected.slot == 0);
    assert(cui_bridge_set_selected(app, group, option_a) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_disabled(app, option_a, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    press(app, option_a);
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == option_a.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
    puts("bridge components ok");
}
