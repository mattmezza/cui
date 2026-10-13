/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
static unsigned parent_clicks, child_clicks;
static void child_click(void *user) {
    ++*(unsigned *)user;
}
static void scene(cui_context *ui, unsigned mode, bool base) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 320, 240, .016f));
    cui_begin_box(ui, 30, (cui_box){.width = 100, .height = 24});
    cui_interact(ui, 30, CUI_ROLE_TEXTBOX, cui_string("Editor"), false, false, 0);
    cui_end_box(ui);
    cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 100, .padding = 10});
    parent_clicks += cui_base_button(ui, 10, cui_string("Parent"), false).activated;
    if (base) {
        child_clicks += cui_base_button(ui, 20, cui_string("Child"), mode != 0).activated;
        cui_begin_box(ui, 20, (cui_box){.width = 120, .height = 32});
        cui_end_box(ui);
    } else
        cui_button_ex(ui, 20, &theme, cui_string("Child"),
                      (cui_button_options){.width = 120,
                                           .disabled = mode == 1,
                                           .loading = mode == 2,
                                           .on_click = child_click,
                                           .action_user = &child_clicks});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void pointer(cui_context *ui, cui_event_type type, cui_rect b, bool touch) {
    assert(cui_queue_event(
        ui, (cui_event){.type = type,
                        .x = b.x + 5,
                        .y = b.y + 5,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                        .pointer_id = touch ? 79 : 0}));
}
static void scrolling_scene(cui_context *ui, unsigned mode, cui_scroll_state *scroll) {
    cui_theme theme = cui_theme_light();
    cui_scroll_options options = {.width = 260,
                                  .height = 200,
                                  .content_height = 600,
                                  .vertical = true,
                                  .hide_scrollbars = true};
    assert(cui_begin_frame(ui, 320, 240, .016f));
    assert(cui_scrollable_begin(ui, 700, &theme, scroll, options));
    cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 100, .padding = 10});
    parent_clicks += cui_base_button(ui, 10, cui_string("Parent"), false).activated;
    cui_button_ex(ui, 20, &theme, cui_string("Child"),
                  (cui_button_options){.width = 120,
                                       .disabled = mode == 1,
                                       .loading = mode == 2,
                                       .on_click = child_click,
                                       .action_user = &child_clicks});
    cui_end_box(ui);
    cui_scrollable_end_ex(ui, 700, &theme, scroll, options);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void scroll_through_barrier(void) {
    for (unsigned mode = 1; mode < 3; mode++)
        for (unsigned touch = 0; touch < 2; touch++) {
            cui_context *ui = cui_create((cui_config){0});
            assert(ui);
            cui_scroll_state scroll = {0};
            parent_clicks = child_clicks = 0;
            scrolling_scene(ui, mode, &scroll);
            cui_rect b = cui_bounds(ui, 20);
            float x = b.x + 5, y = b.y + b.h / 2;
            if (touch) {
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                       .x = x,
                                                       .y = y,
                                                       .pointer_type = CUI_POINTER_TOUCH,
                                                       .pointer_id = 80}));
                scrolling_scene(ui, mode, &scroll);
                assert(cui_event_consumed(ui, 0) && !cui_pressed(ui, 10));
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                                       .x = x,
                                                       .y = y - 20,
                                                       .pointer_type = CUI_POINTER_TOUCH,
                                                       .pointer_id = 80}));
            } else
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                                       .x = x,
                                                       .y = y,
                                                       .dy = -20,
                                                       .scroll_precise = true}));
            scrolling_scene(ui, mode, &scroll);
            assert(scroll.y == 20 && cui_event_target(ui, 0) == 700 && cui_event_consumed(ui, 0));
            if (touch) {
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                                       .x = x,
                                                       .y = y - 20,
                                                       .pointer_type = CUI_POINTER_TOUCH,
                                                       .pointer_id = 80}));
                scrolling_scene(ui, mode, &scroll);
            }
            assert(parent_clicks == 0 && child_clicks == 0);
            cui_destroy(ui);
        }
}
int main(void) {
    scroll_through_barrier();
    for (unsigned base = 0; base < 2; base++)
        for (unsigned mode = 0; mode < (base ? 2u : 3u); mode++)
            for (unsigned touch = 0; touch < 2; touch++) {
                cui_context *ui = cui_create((cui_config){0});
                assert(ui);
                parent_clicks = child_clicks = 0;
                scene(ui, mode, base != 0);
                cui_rect b = cui_bounds(ui, 20);
                cui_focus(ui, 30);
                pointer(ui, CUI_EVENT_POINTER_DOWN, b, touch != 0);
                scene(ui, mode, base != 0);
                if (mode) {
                    assert(cui_focused(ui) == 30 && !cui_pressed(ui, 10) && !cui_pressed(ui, 20));
                    assert(cui_event_target(ui, 0) == 20 && cui_event_consumed(ui, 0));
                }
                pointer(ui, CUI_EVENT_POINTER_UP, b, touch != 0);
                scene(ui, mode, base != 0);
                assert(parent_clicks == 0 && child_clicks == (mode ? 0u : 1u));
                if (mode)
                    assert(cui_event_activation(ui, 0) == 0);
                /* A cancelled contact cannot become a parent click. */
                pointer(ui, CUI_EVENT_POINTER_DOWN, b, touch != 0);
                scene(ui, mode, base != 0);
                pointer(ui, CUI_EVENT_CANCEL, b, touch != 0);
                pointer(ui, CUI_EVENT_POINTER_UP, b, touch != 0);
                scene(ui, mode, base != 0);
                assert(parent_clicks == 0 && child_clicks == (mode ? 0u : 1u));
                /* Keyboard activation of the separate parent is not blocked. */
                cui_focus(ui, 10);
                assert(cui_queue_event(
                    ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
                scene(ui, mode, base != 0);
                assert(parent_clicks == 1);
                cui_destroy(ui);
            }
}
