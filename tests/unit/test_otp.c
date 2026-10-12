/* SPDX-License-Identifier: MIT */
#include "cui/cui_otp.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_otp_result scene(cui_context *ui, cui_otp_state *s, cui_otp_options o) {
    assert(cui_begin_frame(ui, 600, 200, .016f));
    cui_otp_result r;
#ifdef CUI_OTP_BASE_ONLY
    r = cui_base_otp(ui, 10, s, o);
    cui_begin_box(ui, 10, (cui_box){.width = 300, .height = 40});
    cui_end_box(ui);
#else
    cui_theme t = cui_theme_light();
    r = cui_otp(ui, 10, &t, s, o);
#endif
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return r;
}
static void text(cui_context *ui, const char *value) {
    cui_event e = {.type = CUI_EVENT_TEXT};
    assert(strlen(value) < sizeof(e.text));
    strcpy(e.text, value);
    assert(cui_queue_event(ui, e));
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void models(void) {
    char buffer[64] = "";
    cui_otp_state s;
    assert(cui_otp_init(&s, buffer, sizeof(buffer), 4) == CUI_EDIT_OK);
    assert(cui_otp_set_value(&s, cui_string("token-over-length")) == CUI_EDIT_OK);
    assert(cui_otp_count(&s) == 17);
    assert(cui_otp_set_value(&s, (cui_str){"\xff", 1}) == CUI_EDIT_INVALID_UTF8);
    assert(cui_otp_count(&s) == 17);
    assert(cui_otp_set_value(&s, (cui_str){buffer, sizeof(buffer)}) != CUI_EDIT_OK);
    assert(cui_otp_init(&s, buffer, sizeof(buffer), 257) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_otp_groups(6, 0) == 1 && cui_otp_groups(6, SIZE_MAX) == 6 &&
           cui_otp_groups(0, 0) == 1 && cui_otp_groups(5, 2) == 2);
}
static void editing(void) {
    char buffer[64] = "";
    cui_otp_state s;
    assert(cui_otp_init(&s, buffer, sizeof(buffer), 4) == CUI_EDIT_OK);
    cui_otp_options o = cui_otp_options_default();
    o.name = cui_string("Verification code");
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 200});
    assert(ui);
    scene(ui, &s, o);
    cui_rect bounds = cui_bounds(ui, 10);
    assert(bounds.w > 0 && bounds.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 4, .y = bounds.y + 4}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = bounds.x + 4, .y = bounds.y + 4}));
    cui_otp_result r = scene(ui, &s, o);
    assert(cui_focused(ui) == 10);
    assert(r.focused && r.next_wake_seconds == .5);
    r = scene(ui, &s, o);
    assert(!r.focused && r.next_wake_seconds == .5 && !cui_needs_redraw(ui));
    text(ui, "1");
    text(ui, "２");
    text(ui, "x");
    r = scene(ui, &s, o);
    assert(r.changed && r.change_count == 2 && !r.complete && !strcmp(buffer, "12"));
    text(ui, "3");
    text(ui, "4");
    r = scene(ui, &s, o);
    assert(r.complete && r.complete_count == 1 && !strcmp(buffer, "1234"));
    text(ui, "5");
    key(ui, CUI_KEY_LEFT);
    assert(!scene(ui, &s, o).changed && !strcmp(buffer, "1234"));
    key(ui, CUI_KEY_BACKSPACE);
    text(ui, "４");
    r = scene(ui, &s, o);
    assert(r.change_count == 2 && r.complete_count == 1 && !strcmp(buffer, "1234"));
    assert(cui_otp_set_value(&s, cui_string("Aé")) == CUI_EDIT_OK);
    key(ui, CUI_KEY_BACKSPACE);
    scene(ui, &s, o);
    assert(!strcmp(buffer, "A"));
    o.disabled = true;
    text(ui, "9");
    key(ui, CUI_KEY_BACKSPACE);
    r = scene(ui, &s, o);
    assert(!r.changed && r.blurred && !strcmp(buffer, "A"));
    o.disabled = false;
    scene(ui, &s, o);
    cui_focus(ui, 10);
    scene(ui, &s, o);
    assert(cui_otp_set_value(&s, cui_string("")) == CUI_EDIT_OK);
    o.now_seconds = .6;
    r = scene(ui, &s, o);
    assert(!s.cursor_visible && r.next_wake_seconds == 1);
    assert(cui_needs_redraw(ui));
    r = scene(ui, &s, o);
    assert(r.next_wake_seconds == 1 && !cui_needs_redraw(ui));
    o.now_seconds = 1.1;
    r = scene(ui, &s, o);
    assert(s.cursor_visible && r.next_wake_seconds == 1.5);
    o.cell_size = NAN;
    assert(scene(ui, &s, o).error == CUI_EDIT_INVALID_ARGUMENT);
    cui_destroy(ui);
    char tiny[5] = "";
    assert(cui_otp_init(&s, tiny, sizeof(tiny), 6) == CUI_EDIT_OK);
    ui = cui_create((cui_config){.width = 600, .height = 200});
    assert(ui);
    o = cui_otp_options_default();
    scene(ui, &s, o);
    cui_focus(ui, 10);
    text(ui, "1");
    text(ui, "2");
    text(ui, "3");
    text(ui, "4");
    text(ui, "5");
    r = scene(ui, &s, o);
    assert(r.error == CUI_EDIT_CAPACITY && !strcmp(tiny, "1234"));
    cui_destroy(ui);
}
#ifndef CUI_OTP_BASE_ONLY
static void styled(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++)
        for (unsigned size = 0; size < 4; size++) {
            char buffer[64] = "12345";
            cui_otp_state s;
            assert(cui_otp_init(&s, buffer, sizeof(buffer), 5) == CUI_EDIT_OK);
            s.masked = true;
            cui_context *ui = cui_create((cui_config){.width = 600, .height = 200});
            assert(ui);
            cui_otp_options o = cui_otp_options_default();
            o.size = (cui_otp_size)size;
            assert(cui_begin_frame(ui, 600, 200, .016f));
            assert(cui_otp(ui, 10, &themes[theme], &s, o).error == CUI_EDIT_OK);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            size_t masks = 0;
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_TEXT) {
                    assert(list.commands[i].text.length == 1 &&
                           list.commands[i].text.data[0] == '*');
                    masks++;
                }
            assert(masks == 5);
            cui_rect a = cui_bounds(ui, cui_otp_cell_id(10, 0)),
                     b = cui_bounds(ui, cui_otp_cell_id(10, 1)),
                     c = cui_bounds(ui, cui_otp_cell_id(10, 2)),
                     d = cui_bounds(ui, cui_otp_cell_id(10, 3));
            assert(a.w == a.h && b.x - a.x - a.w == 4 && d.x - c.x - c.w == 20);
            assert(a.w == (size == CUI_OTP_LARGE ? 44 : size == CUI_OTP_MEDIUM ? 32 : 24));
            cui_destroy(ui);
        }
}
#endif
int main(void) {
    models();
    editing();
#ifndef CUI_OTP_BASE_ONLY
    styled();
#endif
    return 0;
}
