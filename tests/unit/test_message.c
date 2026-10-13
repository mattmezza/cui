/* SPDX-License-Identifier: MIT */
#include "cui/cui_message.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#ifndef CUI_MESSAGE_BASE_ONLY
#include "cui/cui_components.h"
#endif
static void models(void) {
    cui_message_options message = cui_message_options_default();
    cui_message_plan plan;
    assert(cui_base_message(message, &plan) && !plan.avatar && plan.column_width == 480 &&
           plan.header_inset);
    cui_message_body body = {.typed_bubble = true, .bubble = CUI_BUBBLE_GHOST};
    message.body = &body;
    message.body_count = 1;
    message.avatar.present = true;
    assert(cui_base_message(message, &plan) && plan.ghost && !plan.header_inset &&
           !plan.footer_inset && plan.column_width == 440 && plan.footer_offset == 40);
    message.header.inset = CUI_MESSAGE_INSET_ON;
    assert(cui_base_message(message, &plan) && plan.header_inset);
    message.width = NAN;
    assert(!cui_base_message(message, &plan));
    message = cui_message_options_default();
    message.max_bubble_width = 220;
    message.avatar = (cui_message_slot){.present = true, .width = 40, .height = 40};
    assert(cui_base_message(message, &plan) && plan.column_width == 220 &&
           plan.footer_offset == 48);
    message.width = 180;
    assert(cui_base_message(message, &plan) && plan.column_width == 132);
    message.max_bubble_width = NAN;
    assert(!cui_base_message(message, &plan));
    message.max_bubble_width = -1;
    assert(!cui_base_message(message, &plan));
    cui_message_scroller_state *s =
        cui_message_scroller_create((cui_message_scroller_config){100, 20, 50});
    assert(s);
    assert(cui_message_scroller_count(s) == 20 && cui_message_scroller_following(s));
    size_t bytes = cui_message_scroller_memory_bytes(s);
    cui_message_scroller_view v;
    cui_message_scroller_metrics m = {300, 8, 8, 32};
    assert(cui_message_scroller_layout(s, m, &v) && v.total == 1624 && v.scroll == 1324 &&
           !v.scrolled_up);
    assert(cui_message_scroller_to_item(s, 5));
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == 418 && !v.following_tail &&
           v.scrolled_up);
    double old = v.scroll;
    assert(cui_message_scroller_prepend(s, 2));
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == old + 164);
    old = v.scroll;
    assert(cui_message_scroller_measure(s, 0, 150));
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == old + 100);
    m.row_gap = 16;
    m.padding_top = 12;
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == 574);
    assert(cui_message_scroller_remeasure_items(s, 0, 2));
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == 574);
    assert(cui_message_scroller_measure(s, 0, 50));
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == 474);
    m = (cui_message_scroller_metrics){300, 8, 8, 32};
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == old);
    size_t count = cui_message_scroller_count(s);
    assert(!cui_message_scroller_splice(s, 10, 30, 2) && !cui_message_scroller_append(s, 100) &&
           cui_message_scroller_count(s) == count);
    assert(!cui_message_scroller_to_item(s, 100));
    assert(!cui_message_scroller_measure(s, 0, NAN));
    assert(!cui_message_scroller_remeasure_items(s, 1, 30));
    assert(cui_message_scroller_append(s, 1));
    assert(cui_message_scroller_layout(s, m, &v) && v.scrolled_up && !v.following_tail);
    cui_message_scroller_to_end(s);
    assert(cui_message_scroller_append(s, 1));
    assert(cui_message_scroller_layout(s, m, &v) && v.following_tail && v.scroll == v.total - 300);
    assert(cui_message_scroller_scroll(s, 0));
    assert(cui_message_scroller_layout(s, m, &v) && v.scrolled_up);
    assert(cui_message_scroller_scroll(s, 1e9));
    assert(cui_message_scroller_following(s));
    assert(cui_message_scroller_reset(s, 2));
    assert(cui_message_scroller_layout(s, m, &v) && v.scroll == 0 && v.following_tail &&
           !v.scrolled_up && v.total == 148);
    assert(cui_message_scroller_to_item(s, 1));
    assert(!cui_message_scroller_following(s) && !cui_message_scroller_scrolled_up(s));
    assert(cui_message_scroller_remeasure(s));
    assert(cui_message_scroller_memory_bytes(s) == bytes);
    cui_message_scroller_destroy(s);
    assert(!cui_message_scroller_create((cui_message_scroller_config){2, 3, 50}));
    assert(!cui_message_scroller_create((cui_message_scroller_config){2, 1, NAN}));
}
#ifndef CUI_MESSAGE_BASE_ONLY
static void metadata_button(cui_context *ui, cui_id id, void *user, cui_message_style style) {
    (void)style;
    bool *clicked = user;
    cui_theme t = cui_theme_light();
    if (cui_button(ui, id, &t, cui_string("Reply"), false))
        *clicked = true;
}
static void compact_body(cui_context *ui, cui_id id, void *user, cui_message_style style) {
    float *width = user;
    *width = style.width;
    cui_begin_box(
        ui, id,
        (cui_box){.width = style.width, .height = 30, .fixed_width = true, .fixed_height = true});
    cui_interact(ui, id, CUI_ROLE_LABEL, cui_string("Compact body"), false, false, 0);
    cui_end_box(ui);
}
static void compact_messages(void) {
    cui_theme t = cui_theme_light();
    for (unsigned mobile = 0; mobile < 2; mobile++)
        for (unsigned end = 0; end < 2; end++) {
            float row = mobile ? 240 : 600;
            cui_context *ui = cui_create((cui_config){.width = row, .height = 300});
            assert(ui);
            float content = 0;
            cui_message_body body = {
                .typed_bubble = true,
                .bubble = CUI_BUBBLE_SECONDARY,
                .slot = {.present = true, .render = compact_body, .user = &content}};
            cui_message_options o = cui_message_options_default();
            o.width = row;
            o.max_bubble_width = 220;
            o.alignment = end ? CUI_MESSAGE_END : CUI_MESSAGE_START;
            o.body = &body;
            o.body_count = 1;
            o.avatar = (cui_message_slot){
                .present = true, .text = cui_string("AM"), .width = 40, .height = 40};
            o.footer = (cui_message_slot){.present = true,
                                          .text = cui_string("09:30 · Delivered"),
                                          .inset = CUI_MESSAGE_INSET_OFF};
            assert(cui_begin_frame(ui, row, 300, .016f));
            assert(cui_message(ui, 500, &t, o));
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            float column = fminf(220, row - 48);
            assert(fabsf(content - (column - 2 * t.padding)) < .01f);
            size_t count;
            const cui_semantic *sem = cui_semantics(ui, &count);
            assert(count == 1);
            assert(sem[0].bounds.w <= column && sem[0].bounds.x >= 0 &&
                   sem[0].bounds.x + sem[0].bounds.w <= row);
            if (end)
                assert(sem[0].bounds.x > row - column - 60);
            else
                assert(sem[0].bounds.x >= 48);
            bool metadata = false;
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].text.length == 18) {
                    metadata = true;
                    if (!end)
                        assert(fabsf(list.commands[i].bounds.x - 48) < .01f);
                    assert(list.commands[i].bounds.x + list.commands[i].bounds.w <= row + .01f);
                }
            assert(metadata);
            o.max_bubble_width = NAN;
            assert(cui_begin_frame(ui, row, 300, .016f));
            assert(!cui_message(ui, 500, &t, o));
            cui_end_frame(ui);
            cui_destroy(ui);
        }
}
static void composition(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++)
        for (int alignment = 0; alignment < 2; alignment++)
            for (int ghost = 0; ghost < 2; ghost++) {
                cui_context *ui = cui_create((cui_config){.width = 600, .height = 300});
                assert(ui);
                cui_message_options o = cui_message_options_default();
                o.alignment = (cui_message_alignment)alignment;
                o.role = CUI_ROLE_LISTITEM;
                o.name = cui_string("Alice message");
                o.avatar = (cui_message_slot){.present = true, .height = 32};
                o.header.text = cui_string("Alice");
                o.footer.text = cui_string("Read");
                cui_message_body body = {.typed_bubble = true,
                                         .bubble = ghost ? CUI_BUBBLE_GHOST : CUI_BUBBLE_SECONDARY,
                                         .slot = {.text = {"Hello", 5}}};
                o.body = &body;
                o.body_count = 1;
                assert(cui_begin_frame(ui, 600, 300, .016f));
                assert(cui_message(ui, 11, &themes[theme], o));
                cui_draw_list list = cui_end_frame(ui);
                assert(cui_last_error(ui) == CUI_OK);
                float header_x = -1, footer_x = -1, body_y = -1, body_h = 0, avatar_y = -1;
                for (size_t i = 0; i < list.count; i++) {
                    cui_draw_command c = list.commands[i];
                    if (c.type == CUI_DRAW_TEXT && c.text.length == 5 &&
                        !memcmp(c.text.data, "Alice", 5))
                        header_x = c.bounds.x;
                    if (c.type == CUI_DRAW_TEXT && c.text.length == 4 &&
                        !memcmp(c.text.data, "Read", 4))
                        footer_x = c.bounds.x;
                    if (c.type == CUI_DRAW_TEXT && c.text.length == 5 &&
                        !memcmp(c.text.data, "Hello", 5)) {
                        body_y = c.bounds.y;
                        body_h = c.bounds.h;
                    }
                    if (c.type == CUI_DRAW_RECT && c.bounds.w == 32 && c.bounds.h == 32)
                        avatar_y = c.bounds.y;
                }
                assert(header_x >= 0 && footer_x >= 0 && avatar_y >= 0);
                if (!alignment) {
                    assert(header_x == (ghost ? 40 : 52));
                    assert(footer_x == (ghost ? 40 : 52));
                } else
                    assert(header_x > 300 && footer_x > 300);
                assert(avatar_y + 32 >= body_y + body_h);
                size_t count;
                const cui_semantic *sem = cui_semantics(ui, &count);
                assert(count == 1 && sem[0].role == CUI_ROLE_LISTITEM);
                bool clicked = false;
                o.footer =
                    (cui_message_slot){.render = metadata_button, .user = &clicked, .width = 100};
                assert(cui_begin_frame(ui, 600, 300, .016f));
                assert(cui_message(ui, 11, &themes[theme], o));
                cui_end_frame(ui);
                sem = cui_semantics(ui, &count);
                cui_id button = 0;
                for (size_t i = 0; i < count; i++)
                    if (sem[i].role == CUI_ROLE_BUTTON)
                        button = sem[i].id;
                assert(button);
                cui_focus(ui, button);
                assert(cui_queue_event(
                    ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
                assert(cui_begin_frame(ui, 600, 300, .016f));
                assert(cui_message(ui, 11, &themes[theme], o));
                cui_end_frame(ui);
                assert(clicked);
                cui_destroy(ui);
            }
}
#endif
typedef struct {
    size_t calls;
    float height;
} rows;
#ifndef CUI_MESSAGE_BASE_ONLY
static void row(cui_context *ui, cui_id id, size_t index, void *user, float width) {
    rows *r = user;
    r->calls++;
    cui_begin_box(ui, id,
                  (cui_box){.width = width, .height = r->height, .background = {40, 50, 60, 255}});
    cui_interact(ui, id, CUI_ROLE_LISTITEM, cui_string("Message"), false, false, (float)index);
    cui_end_box(ui);
}
#endif
static cui_message_scroller_result scene(cui_context *ui, cui_message_scroller_state *s,
                                         cui_message_scroller_options o, rows *r) {
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_message_scroller_result result;
#ifdef CUI_MESSAGE_BASE_ONLY
    (void)r;
    result = cui_base_message_scroller(ui, 22, s, o);
    cui_begin_box(ui, 22, (cui_box){.width = o.width, .height = o.height});
    cui_end_box(ui);
    if (result.jump_opacity > 0) {
        cui_begin_box(ui, cui_message_scroller_jump_id(22), (cui_box){.width = 32, .height = 32});
        cui_end_box(ui);
    }
#else
    cui_theme t = cui_theme_light();
    result = cui_message_scroller(ui, 22, &t, s, row, r, o);
#endif
    cui_draw_list drawing = cui_end_frame(ui);
#ifndef CUI_MESSAGE_BASE_ONLY
    if (result.jump_opacity > 0 && cui_bounds(ui, cui_message_scroller_jump_id(22)).w > 0) {
        bool icon = false;
        for (size_t i = 0; i < drawing.count; ++i) {
            const cui_draw_command *c = &drawing.commands[i];
            if (c->type == CUI_DRAW_IMAGE) {
                icon = true;
                assert(c->bitmap.bitmap.pixels && c->bounds.w == 16 && c->bounds.h == 16);
            }
            assert(c->type != CUI_DRAW_TEXT || c->text.length != 1 || c->text.data[0] != 'v');
        }
        assert(icon);
    }
#else
    (void)drawing;
#endif
    assert(cui_last_error(ui) == CUI_OK);
    assert(result.valid);
    return result;
}
static void interactions(void) {
    cui_message_scroller_state *s =
        cui_message_scroller_create((cui_message_scroller_config){10000, 10000, 50});
    assert(s);
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    assert(ui);
    cui_message_scroller_options o = cui_message_scroller_options_default();
    o.scrollbar = false;
    o.reduced_motion = true;
    rows r = {.height = 50};
    scene(ui, s, o, &r);
    r.calls = 0;
    scene(ui, s, o, &r);
#ifndef CUI_MESSAGE_BASE_ONLY
    assert(r.calls < 30 && r.calls > 0);
#endif
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 100, .y = 100, .dy = 2}));
    cui_message_scroller_result result = scene(ui, s, o, &r);
    assert(result.view.scrolled_up && !result.view.following_tail);
    double before_precise = result.view.scroll;
    assert(cui_queue_event(
        ui, (cui_event){
                .type = CUI_EVENT_SCROLL, .x = 100, .y = 100, .dy = 3.5f, .scroll_precise = true}));
    result = scene(ui, s, o, &r);
    assert(result.view.scroll == before_precise - 3.5);
    assert(cui_event_consumed(ui, 0));
    cui_focus(ui, 22);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    result = scene(ui, s, o, &r);
    assert(result.view.scroll == 0 && result.view.scrolled_up);
    assert(cui_queue_event(
        ui, (cui_event){
                .type = CUI_EVENT_SCROLL, .x = 100, .y = 100, .dy = 3.5f, .scroll_precise = true}));
    result = scene(ui, s, o, &r);
    assert(result.view.scroll == 0 && !cui_event_consumed(ui, 0));
    cui_focus(ui, cui_message_scroller_jump_id(22));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    result = scene(ui, s, o, &r);
    assert(result.jumped && result.view.following_tail && !result.view.scrolled_up);
    assert(result.jump_opacity == 0 && result.next_wake_seconds == 0);
    o.reduced_motion = false;
    o.bottom_fade = true;
    o.now_seconds = 1;
    assert(cui_message_scroller_scroll(s, 0));
    result = scene(ui, s, o, &r);
    assert(result.jump_opacity == 0 && result.next_wake_seconds > 1);
    o.now_seconds = 1.1;
    result = scene(ui, s, o, &r);
    assert(fabsf(result.jump_opacity - .875f) < .001f);
    assert(fabsf(result.fade_opacity - .875f) < .001f);
    cui_message_scroller_to_end(s);
    result = scene(ui, s, o, &r);
    assert(result.jump_disabled && result.jump_opacity > 0);
    o.now_seconds = 1.3;
    result = scene(ui, s, o, &r);
    assert(result.jump_opacity < .001f && result.fade_opacity < .001f);
    assert(cui_message_scroller_scroll(s, 0));
    o.jump_transition = 0;
    result = scene(ui, s, o, &r);
    assert(result.jump_opacity == 1);
    cui_message_scroller_options invalid = o;
    invalid.width = NAN;
    assert(!cui_message_scroller_validate_options(invalid));
    cui_message_scroller_destroy(s);
    cui_destroy(ui);
}
static void test_measure_preserves_top_padding(void) {
    cui_message_scroller_state *s = cui_message_scroller_create(
        (cui_message_scroller_config){.capacity = 20, .item_count = 10, .estimated_height = 50});
    assert(s);
    cui_message_scroller_metrics metrics = {
        .viewport_height = 100, .padding_top = 8, .padding_bottom = 8, .row_gap = 32};
    cui_message_scroller_view view;
    assert(cui_message_scroller_layout(s, metrics, &view));
    assert(cui_message_scroller_scroll(s, 0));
    assert(cui_message_scroller_measure(s, 0, 80));
    assert(cui_message_scroller_layout(s, metrics, &view) && view.scroll == 0);
    assert(cui_message_scroller_scroll(s, 4));
    assert(cui_message_scroller_measure(s, 1, 75));
    assert(cui_message_scroller_layout(s, metrics, &view) && view.scroll == 4);
    cui_message_scroller_destroy(s);
}
int main(void) {
    test_measure_preserves_top_padding();
    models();
#ifndef CUI_MESSAGE_BASE_ONLY
    compact_messages();
    composition();
#endif
    interactions();
    return 0;
}
