/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_context *ui;
static const cui_id id = 101;
static bool checked, disabled;
static float slider;
static cui_check_state check;
static int radio;
static bool frame(int kind) {
    assert(cui_begin_frame(ui, 400, 200, 0.016f));
    cui_begin_box(ui, id, (cui_box){.width = 200, .height = 40});
    bool changed = false;
    switch (kind) {
    case 0:
        changed = cui_base_checkbox(ui, id, cui_string("Check"), &checked, disabled);
        break;
    case 1:
        changed = cui_base_switch(ui, id, cui_string("Switch"), &checked, disabled);
        break;
    case 2:
        changed = cui_base_radio(ui, id, cui_string("Radio"), &radio, 2, disabled);
        break;
    case 3:
        changed = cui_base_check(ui, id, cui_string("Mixed"), &check, disabled);
        break;
    case 4:
        changed = cui_base_slider(ui, id, cui_string("Slider"), &slider, 0, 10, 1, disabled);
        break;
    case 5:
        changed = cui_base_toggle(ui, id, cui_string("Toggle"), &checked, disabled);
        break;
    default:
        changed = cui_base_button(ui, id, cui_string("Button"), disabled).activated;
        break;
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return changed;
}
static void key(cui_key k) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
    if (k == CUI_KEY_SPACE)
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = k}));
}
static void pointer(cui_event_type type, float x) {
    cui_rect b = cui_bounds(ui, id);
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = b.x + x, .y = b.y + 10}));
}

static void editor_navigation_consumption(void) {
    for (unsigned mode = 0; mode < 4; ++mode) {
        cui_context *ctx = cui_create((cui_config){.width = 200, .height = 100});
        char text[16] = "abcdef";
        cui_edit *edit =
            cui_edit_create((cui_edit_config){.buffer = text, .capacity = sizeof text});
        cui_input_behavior_state state = {0};
        assert(ctx && edit && cui_begin_frame(ctx, 200, 100, 0));
        cui_begin_box(ctx, 400, (cui_box){.width = 180, .height = 32});
        cui_interact(ctx, 400, CUI_ROLE_TEXTBOX, cui_string("Editor"), false, false, 0);
        cui_end_box(ctx);
        cui_end_frame(ctx);
        cui_focus(ctx, 400);
        for (unsigned end = 0; end < 2; ++end) {
            assert(cui_edit_select(edit, 3, 3) == CUI_EDIT_OK);
            assert(cui_queue_event(ctx, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                    .key = end ? CUI_KEY_END : CUI_KEY_HOME}));
            assert(cui_begin_frame(ctx, 200, 100, 0));
            cui_begin_box(ctx, 400, (cui_box){.width = 180, .height = 32});
            if (mode == 0) {
                cui_interact(ctx, 400, CUI_ROLE_TEXTBOX, cui_string("Editor"), false, false, 0);
                assert(cui_edit_apply_events(edit, ctx, 400, false) == CUI_EDIT_OK);
            } else {
                cui_input_result result =
                    cui_base_input_ex(ctx, 400, cui_string("Editor"), edit, &state,
                                      (cui_base_input_options){.readonly = mode == 2,
                                                               .delegate_home_end = mode == 3});
                assert(result.error == CUI_EDIT_OK);
            }
            assert(cui_edit_get_state(edit).cursor == (mode == 3 ? 3 : (end ? 6 : 0)));
            assert(cui_event_consumed(ctx, 0) ==
                   (mode != 3)); /* Ancestor carousel must not also navigate. */
            cui_end_box(ctx);
            cui_end_frame(ctx);
        }
        cui_edit_destroy(edit);
        cui_destroy(ctx);
    }
}
static void editor_composition_routing(void) {
    for (unsigned mode = 0; mode < 3; mode++) {
        cui_context *ctx = cui_create((cui_config){.width = 200, .height = 100});
        char buffer[32] = "abcdef";
        cui_edit *edit =
            cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
        cui_input_behavior_state state = {0};
        assert(ctx && edit && cui_begin_frame(ctx, 200, 100, 0));
        cui_begin_box(ctx, 400, (cui_box){.width = 180, .height = 32});
        cui_base_input_ex(ctx, 400, cui_string("Editor"), edit, &state,
                          (cui_base_input_options){0});
        cui_end_box(ctx);
        cui_end_frame(ctx);
        cui_focus(ctx, 400);
        assert(cui_edit_select(edit, 2, 2) == CUI_EDIT_OK);
        for (unsigned flag = 0; flag < 2; flag++) {
            const cui_key keys[] = {CUI_KEY_LEFT, CUI_KEY_HOME, CUI_KEY_ENTER, CUI_KEY_V};
            for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
                assert(cui_queue_event(ctx, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                        .key = keys[i],
                                                        .modifiers = CUI_MOD_CTRL,
                                                        .ime_composing = flag == 0,
                                                        .prefer_character_input = flag == 1}));
            assert(cui_begin_frame(ctx, 200, 100, 0));
            cui_begin_box(ctx, 400, (cui_box){.width = 180, .height = 32});
            if (mode == 0) {
                cui_interact(ctx, 400, CUI_ROLE_TEXTBOX, cui_string("Editor"), false, false, 0);
                assert(cui_edit_apply_events(edit, ctx, 400, false) == CUI_EDIT_OK);
            } else {
                cui_input_result result =
                    cui_base_input_ex(ctx, 400, cui_string("Editor"), edit, &state,
                                      (cui_base_input_options){.readonly = mode == 2});
                assert(result.error == CUI_EDIT_OK && !result.changed && !result.submitted);
            }
            assert(cui_edit_get_state(edit).cursor == 2 && !strcmp(buffer, "abcdef"));
            for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
                assert(!cui_event_consumed(ctx, i));
            cui_end_box(ctx);
            cui_end_frame(ctx);
        }
        cui_edit_destroy(edit);
        cui_destroy(ctx);
    }
}
int main(void) {
    editor_composition_routing();
    editor_navigation_consumption();
    ui = cui_create((cui_config){.width = 400, .height = 200});
    assert(ui);
    assert(!frame(0));
    pointer(CUI_EVENT_POINTER_DOWN, 10);
    pointer(CUI_EVENT_POINTER_UP, 10);
    assert(frame(0) && checked);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem[0].checked && sem[0].role == CUI_ROLE_CHECKBOX);
    assert(!frame(0) && checked);
    key(CUI_KEY_SPACE);
    assert(frame(0) && !checked);
    checked = true;
    assert(!frame(1));
    sem = cui_semantics(ui, &n);
    assert(sem[0].role == CUI_ROLE_SWITCH && sem[0].checked);
    key(CUI_KEY_ENTER);
    assert(frame(1) && !checked);
    disabled = true;
    frame(0);
    key(CUI_KEY_SPACE);
    pointer(CUI_EVENT_POINTER_DOWN, 10);
    pointer(CUI_EVENT_POINTER_UP, 10);
    assert(!frame(0) && !checked);
    assert(cui_focused(ui) != id);
    disabled = false;
    frame(3);
    cui_focus(ui, id);
    check = CUI_CHECK_MIXED;
    key(CUI_KEY_SPACE);
    assert(frame(3) && check == CUI_CHECK_CHECKED);
    key(CUI_KEY_SPACE);
    assert(frame(3) && check == CUI_CHECK_UNCHECKED);
    frame(2);
    cui_focus(ui, id);
    key(CUI_KEY_SPACE);
    assert(frame(2) && radio == 2);
    key(CUI_KEY_SPACE);
    assert(!frame(2) && radio == 2);
    checked = false;
    frame(5);
    cui_focus(ui, id);
    key(CUI_KEY_SPACE);
    assert(frame(5) && checked);
    frame(6);
    cui_focus(ui, id);
    key(CUI_KEY_ENTER);
    assert(frame(6));
    assert(!frame(6));
    slider = 5;
    frame(4);
    cui_focus(ui, id);
    key(CUI_KEY_RIGHT);
    assert(frame(4) && slider == 6);
    key(CUI_KEY_HOME);
    assert(frame(4) && slider == 0);
    key(CUI_KEY_END);
    assert(frame(4) && slider == 10);
    pointer(CUI_EVENT_POINTER_DOWN, 80);
    pointer(CUI_EVENT_POINTER_MOVE, 120);
    pointer(CUI_EVENT_POINTER_UP, 120);
    assert(frame(4) && slider == 6);
    pointer(CUI_EVENT_POINTER_DOWN, 80);
    assert(frame(4) && slider == 4);
    pointer(CUI_EVENT_POINTER_MOVE, 250);
    assert(frame(4) && slider == 10);
    pointer(CUI_EVENT_CANCEL, 0);
    assert(!frame(4));
    pointer(CUI_EVENT_POINTER_MOVE, 20);
    assert(!frame(4) && slider == 10);
    disabled = true;
    frame(4);
    key(CUI_KEY_HOME);
    assert(!frame(4) && slider == 10);
    cui_popup_result p = cui_popup_place((cui_rect){80, 90, 20, 10}, (cui_size){40, 30},
                                         (cui_rect){0, 0, 100, 100}, CUI_POPUP_BELOW, 2);
    assert(p.flipped && p.side == CUI_POPUP_ABOVE && p.bounds.x == 60 && p.bounds.y == 58);
    p = cui_popup_place((cui_rect){20, 20, 10, 10}, (cui_size){200, 200},
                        (cui_rect){0, 0, 100, 100}, CUI_POPUP_RIGHT, 0);
    assert(p.bounds.w == 100 && p.bounds.h == 100 && p.bounds.x == 0 && p.bounds.y == 0);
    float heights[] = {10, 30, 20, 40};
    cui_virtual_range vr = cui_virtualize(heights, 4, 15, 30, 0);
    assert(vr.begin == 1 && vr.end == 3 && vr.before == 10 && vr.after == 40 && vr.total == 100);
    vr = cui_virtualize(heights, 4, 1000, 30, 1);
    assert(vr.scroll == 70 && vr.begin == 2 && vr.end == 4 && vr.before == 40);
    assert(cui_date_days_in_month(2000, 2) == 29 && cui_date_days_in_month(1900, 2) == 28);
    assert(cui_date_weekday((cui_date){2024, 2, 29}) == 4 &&
           !cui_date_valid((cui_date){2023, 2, 29}));
    cui_date date = cui_date_add_months((cui_date){2024, 1, 31}, 1);
    assert(date.month == 2 && date.day == 29);
    date = cui_date_add_days(date, 1);
    assert(date.month == 3 && date.day == 1);
    date = cui_date_add_days(date, -2);
    assert(date.month == 2 && date.day == 28);
    date = cui_date_add_days((cui_date){1, 1, 1}, -1);
    assert(!cui_date_valid(date));
    cui_destroy(ui);
    return 0;
}
