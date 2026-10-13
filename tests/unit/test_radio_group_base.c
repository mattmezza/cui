/* SPDX-License-Identifier: MIT */
#include "cui/cui_radio_group.h"
#include <assert.h>
/* Intentionally no theme construction or styled component calls. */
static cui_context *ui;
static cui_id selected = 11;
static cui_radio_group_state state;
static cui_radio_group_item items[] = {
    {11, {"One", 3}, false}, {12, {"Two", 3}, true}, {13, {"Three", 5}, false}};
static unsigned requests;
static bool accept;
static void changed(void *user, cui_id requested) {
    assert(user == &selected);
    requests++;
    if (accept)
        selected = requested;
}
static cui_radio_group_result frame(size_t count, bool disabled) {
    assert(cui_begin_frame(ui, 320, 200, .016f));
    cui_radio_group_result result =
        cui_base_radio_group(ui, 10, items, count, &selected, &state,
                             (cui_radio_group_options){.vertical = true,
                                                       .disabled = disabled,
                                                       .name = {"Choices", 7},
                                                       .on_change = changed,
                                                       .change_user = &selected});
    assert(result.valid);
    cui_begin_box(ui, 10, (cui_box){.width = 200, .gap = 8});
    for (size_t i = 0; i < count; i++) {
        cui_begin_box(ui, items[i].id, (cui_box){.width = 200, .height = 32});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    assert(n == count + 1 && s[0].role == CUI_ROLE_RADIOGROUP);
    unsigned stops = 0;
    for (size_t i = 0; i < count; i++) {
        assert(s[i + 1].role == CUI_ROLE_RADIO);
        assert(s[i + 1].checked == (selected == items[i].id));
        assert(s[i + 1].disabled == (disabled || items[i].disabled));
        assert(s[i + 1].details.parent == 10 && s[i + 1].details.pos_in_set == i + 1 &&
               s[i + 1].details.set_size == count);
        stops += s[i + 1].tab_index == 0;
    }
    assert(stops == (result.tab_stop != 0));
    return result;
}
static void key(cui_key value) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = value}));
}
int main(void) {
    ui = cui_create((cui_config){.width = 320, .height = 200});
    assert(ui);
    frame(3, false);
    key(CUI_KEY_TAB);
    frame(3, false);
    assert(cui_focused(ui) == 11);
    key(CUI_KEY_DOWN);
    cui_radio_group_result r = frame(3, false);
    assert(r.requested && !r.changed && requests == 1 && selected == 11 &&
           cui_focused(ui) == 13); /* Controlled rejection; skip disabled Two. */
    accept = true;
    key(CUI_KEY_ENTER);
    r = frame(3, false);
    assert(r.changed && requests == 2 && selected == 13);
    key(CUI_KEY_ENTER);
    frame(3, false);
    assert(requests == 2);
    key(CUI_KEY_HOME);
    frame(3, false);
    assert(selected == 11 && requests == 3);
    cui_rect target = cui_bounds(ui, 13);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = target.x + 4, .y = target.y + 4}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = target.x + 4, .y = target.y + 4}));
    frame(3, false);
    assert(selected == 13 && requests == 4);
    frame(3, true);
    assert(!cui_focused(ui));
    key(CUI_KEY_HOME);
    frame(3, true);
    assert(requests == 4 && selected == 13);
    frame(3, false);
    cui_focus(ui, 13);
    frame(3, false);
    frame(1, false);
    assert(cui_focused(ui) == 11 && selected == 13 && requests == 4);
    frame(0, false);
    assert(!cui_focused(ui));
    cui_destroy(ui);
    return 0;
}
