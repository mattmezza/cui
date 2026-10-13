/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include <assert.h>
#include <string.h>
static unsigned calls;
static void title(cui_context *ui, const cui_theme *t, cui_color fg, void *user) {
    (void)user;
    calls = calls * 10 + 1;
    cui_text(ui, cui_string("Title slot"), t->font_size, fg);
}
static void body(cui_context *ui, const cui_theme *t, cui_color fg, void *user) {
    (void)user;
    calls = calls * 10 + 2;
    cui_text(ui, cui_string("Body slot"), t->font_size, fg);
}
static void action(cui_context *ui, const cui_theme *t, cui_color fg, void *user) {
    (void)fg;
    (void)user;
    calls = calls * 10 + 3;
    assert(!cui_button(ui, 900, t, cui_string("Disabled action"), true));
}
static unsigned activations;
static void reaction_button(cui_context *ui, const cui_theme *t, cui_color fg, void *user) {
    (void)fg;
    (void)user;
    if (cui_button(ui, 901, t, cui_string("React"), false))
        activations++;
}
static void interactive_scene(cui_context *ui, const cui_theme *t, cui_bubble_state *state) {
    assert(cui_begin_frame(ui, 400, 500, .016f));
    cui_bubble(ui, 88, t,
               (cui_bubble_options){.width = 400,
                                    .message = cui_string("Message"),
                                    .reactions = reaction_button,
                                    .state = state,
                                    .reaction_side = CUI_BUBBLE_REACTIONS_TOP});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static bool same(cui_color a, cui_color b) {
    return !memcmp(&a, &b, sizeof a);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    for (unsigned dark = 0; dark < 2; dark++) {
        cui_theme t = dark ? cui_theme_dark() : cui_theme_light();
        for (int kind = CUI_ALERT_INFO; kind <= CUI_ALERT_DEFAULT; kind++) {
            calls = 0;
            assert(cui_begin_frame(ui, 400, 500, .016f));
            cui_alert(ui, 1, &t,
                      (cui_alert_options){.kind = (cui_alert_kind)kind,
                                          .width = 400,
                                          .title_content = title,
                                          .content = body,
                                          .action = action,
                                          .accessible_name = cui_string("Custom alert")});
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            assert(calls == 123);
            cui_color fg = kind == CUI_ALERT_INFO      ? t.info
                           : kind == CUI_ALERT_SUCCESS ? t.success
                           : kind == CUI_ALERT_WARNING ? t.warning
                           : kind == CUI_ALERT_ERROR   ? t.danger
                                                       : t.text;
            bool found = false;
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].text.length == 9 &&
                    !memcmp(list.commands[i].text.data, "Body slot", 9)) {
                    assert(same(list.commands[i].color, fg));
                    found = true;
                }
            assert(found);
            size_t n;
            const cui_semantic *sem = cui_semantics(ui, &n);
            assert(n == 2);
            assert(sem[0].role == CUI_ROLE_ALERT && sem[1].disabled);
        }
        for (int variant = CUI_BUBBLE_FILLED; variant <= CUI_BUBBLE_ERROR; variant++)
            for (unsigned outgoing = 0; outgoing < 2; outgoing++)
                for (unsigned top = 0; top < 2; top++) {
                    calls = 0;
                    assert(cui_begin_frame(ui, 400, 500, .016f));
                    cui_bubble(
                        ui, 2, &t,
                        (cui_bubble_options){.variant = (cui_bubble_variant)variant,
                                             .alignment = (cui_bubble_alignment)outgoing,
                                             .reaction_side = (cui_bubble_reaction_side)top,
                                             .reaction_alignment = (cui_bubble_alignment)outgoing,
                                             .width = 400,
                                             .content = body,
                                             .reactions = title,
                                             .message = cui_string("Accessible message"),
                                             .announcement = CUI_BUBBLE_ANNOUNCE_POLITE});
                    cui_draw_list list = cui_end_frame(ui);
                    assert(cui_last_error(ui) == CUI_OK);
                    assert(calls == (top ? 12 : 21));
                    size_t n;
                    const cui_semantic *sem = cui_semantics(ui, &n);
                    assert(n == 1 && sem[0].role == CUI_ROLE_STATUS && sem[0].tab_index == -1);
                    bool found = false;
                    for (size_t i = 0; i < list.count; i++)
                        if (list.commands[i].type == CUI_DRAW_TEXT &&
                            list.commands[i].text.length == 9 &&
                            !memcmp(list.commands[i].text.data, "Body slot", 9)) {
                            assert(same(list.commands[i].color,
                                        cui_bubble_foreground(&t, (cui_bubble_variant)variant)));
                            if (outgoing)
                                assert(list.commands[i].bounds.x > 150);
                            else
                                assert(list.commands[i].bounds.x < 30);
                            assert(list.commands[i].bounds.w <= 320);
                            found = true;
                        }
                    assert(found);
                }
    }
    cui_theme t = cui_theme_light();
    cui_bubble_state state = {0};
    for (unsigned frame = 0; frame < 8; frame++) {
        calls = 0;
        assert(cui_begin_frame(ui, 400, 500, .016f));
        cui_bubble(ui, 42, &t,
                   (cui_bubble_options){.width = 400,
                                        .content = body,
                                        .reactions = title,
                                        .state = &state,
                                        .alignment = CUI_BUBBLE_OUTGOING});
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        if (frame == 0)
            assert(!state.measured);
        if (frame > 3) {
            assert(state.measured && !cui_needs_redraw(ui));
            cui_rect body_bounds = {0}, reaction_bounds = {0};
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_TEXT) {
                    if (list.commands[i].text.length == 9)
                        body_bounds = list.commands[i].bounds;
                    if (list.commands[i].text.length == 10)
                        reaction_bounds = list.commands[i].bounds;
                }
            assert(reaction_bounds.y > body_bounds.y);
            assert(reaction_bounds.y < body_bounds.y + body_bounds.h + t.padding);
        }
    }
    assert(cui_begin_frame(ui, 400, 500, .016f));
    cui_bubble(ui, 42, &t,
               (cui_bubble_options){.width = 220,
                                    .content = body,
                                    .reactions = title,
                                    .state = &state,
                                    .content_revision = 1});
    cui_end_frame(ui);
    assert(!state.measured && cui_needs_redraw(ui));
    for (unsigned announcement = 0; announcement < 3; announcement++) {
        assert(cui_begin_frame(ui, 400, 500, .016f));
        cui_bubble(ui, 77, &t,
                   (cui_bubble_options){.width = 400,
                                        .message = cui_string("Notice"),
                                        .announcement = (cui_bubble_announcement)announcement});
        cui_end_frame(ui);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        assert(n == 1);
        assert(sem[0].role == (announcement == 0   ? CUI_ROLE_GROUP
                               : announcement == 1 ? CUI_ROLE_STATUS
                                                   : CUI_ROLE_ALERT));
    }
    calls = 0;
    assert(cui_begin_frame(ui, 400, 500, .016f));
    cui_bubble(ui, 77, &t,
               (cui_bubble_options){.alignment = (cui_bubble_alignment)99, .content = body});
    cui_alert(ui, 78, &t, (cui_alert_options){.kind = (cui_alert_kind)99, .content = body});
    cui_end_frame(ui);
    assert(!calls);
    calls = 0;
    assert(cui_begin_frame(ui, 400, 500, .016f));
    cui_alert(
        ui, 79, &t,
        (cui_alert_options){.banner = true, .width = 400, .title_content = title, .content = body});
    cui_draw_list banner = cui_end_frame(ui);
    assert(calls == 2);
    for (size_t i = 0; i < banner.count; i++)
        assert(!(banner.commands[i].type == CUI_DRAW_BORDER && banner.commands[i].bounds.w == 400));
    cui_bubble_state interactive = {0};
    for (unsigned frame = 0; frame < 8; frame++)
        interactive_scene(ui, &t, &interactive);
    cui_rect control = cui_bounds(ui, 901);
    assert(control.w > 0 && control.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = control.x + control.w * .5f,
                                           .y = control.y + control.h * .5f}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = control.x + control.w * .5f,
                                           .y = control.y + control.h * .5f}));
    interactive_scene(ui, &t, &interactive);
    assert(activations == 1);
    cui_focus(ui, 901);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    interactive_scene(ui, &t, &interactive);
    assert(activations == 2);
    bool visible = false;
    calls = 0;
    assert(cui_begin_frame(ui, 400, 500, .016f));
    cui_alert(ui, 1, &(cui_theme){0}, (cui_alert_options){.visible = &visible, .content = body});
    cui_end_frame(ui);
    assert(!calls);
    cui_destroy(ui);
    return 0;
}
