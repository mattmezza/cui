/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_questionnaire_input_state input_view;
static void frame(cui_context *ui, cui_questionnaire_state *s, cui_questionnaire_options o) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_theme t = cui_theme_light();
    cui_id answer_id = 12;
    assert(cui_questionnaire_begin(ui, 10, &t, s, o));
    (void)cui_questionnaire_answer_keys(ui, 10, s, &answer_id, 1, 19);
    (void)cui_questionnaire_shortcut_keys(ui, 10, s, &answer_id, 1, 19, o.numeric_shortcuts);
    cui_questionnaire_progress(ui, 11, &t, s, o);
    cui_questionnaire_heading(ui, 13, &t, s, 0, o);
    (void)cui_questionnaire_choice_control(ui, 12, &t, s, 0, 0, o);
    if (s->items[0].input)
        assert(cui_questionnaire_input_control(ui, 19, &t, s, 0, &input_view, NULL, o) ==
               CUI_QUESTIONNAIRE_OK);
    cui_questionnaire_error_control(ui, 14, &t, s, 0, o);
    cui_begin_box(ui, 15, (cui_box){.row = true, .gap = 8});
    (void)cui_questionnaire_action_control(ui, 16, &t, s, CUI_QUESTIONNAIRE_PREVIOUS, (cui_str){0},
                                           o);
    (void)cui_questionnaire_action_control(ui, 17, &t, s, CUI_QUESTIONNAIRE_SKIP, (cui_str){0}, o);
    (void)cui_questionnaire_action_control(ui, 18, &t, s, CUI_QUESTIONNAIRE_FINISH, (cui_str){0},
                                           o);
    cui_end_box(ui);
    cui_questionnaire_end(ui);
    cui_draw_list draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && draws.count > 0);
}
static void shortcut_frame(cui_context *ui, cui_questionnaire_state *s, bool numbers) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_theme t = cui_theme_light();
    cui_id ids[] = {22, 23, 24};
    (void)cui_questionnaire_radio_keys(ui, s, ids, 3);
    assert(cui_questionnaire_begin(ui, 20, &t, s, (cui_questionnaire_options){.width = 300}));
    (void)cui_questionnaire_answer_keys(ui, 20, s, ids, 3, 0);
    (void)cui_questionnaire_confirm_keys(ui, 20, s, ids, 3, 0, CUI_MOD_CTRL);
    (void)cui_questionnaire_shortcut_keys(ui, 20, s, ids, 3, 0, numbers);
    for (size_t i = 0; i < 3; i++)
        (void)cui_questionnaire_choice_control(ui, ids[i], &t, s, 0, i,
                                               (cui_questionnaire_options){0});
    cui_begin_box(ui, 26, (cui_box){.width = 60, .height = 20});
    cui_interact(ui, 26, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    cui_end_box(ui);
    cui_questionnaire_end(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void radio_frame(cui_context *ui, cui_questionnaire_state *s) {
    shortcut_frame(ui, s, false);
}
static void radio_keyboard(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_questionnaire_choice choices[] = {{.value = cui_string("a")},
                                          {.value = cui_string("b"), .disabled = true},
                                          {.value = cui_string("c")}};
    cui_questionnaire_item item = {
        .name = cui_string("radio"), .choices = choices, .choice_count = 3};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected[3];
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = selected,
                                                                .choice_capacity = 3},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    radio_frame(ui, &s);
    cui_focus(ui, 22);
    const cui_event ignored[] = {
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .repeat = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .modifiers = CUI_MOD_CTRL},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .ime_composing = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B, .prefer_character_input = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT, .ime_composing = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .ime_composing = true},
        {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_C}};
    for (size_t i = 0; i < sizeof ignored / sizeof ignored[0]; i++) {
        assert(cui_queue_event(ui, ignored[i]));
        radio_frame(ui, &s);
        assert(!cui_questionnaire_choice_selected(&s, 0, 0));
        assert(!cui_questionnaire_choice_selected(&s, 0, 2));
        assert(!s.pending_confirm && !s.complete && !cui_event_consumed(ui, 0));
    }
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B}));
    radio_frame(ui, &s);
    assert(cui_questionnaire_choice_selected(&s, 0, 2));
    assert(cui_focused(ui) == 24 && s.pending_confirm && cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_B}));
    radio_frame(ui, &s);
    assert(s.complete && cui_event_consumed(ui, 0));
    assert(cui_questionnaire_reset(&s));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_2}));
    shortcut_frame(ui, &s, true);
    assert(cui_questionnaire_choice_selected(&s, 0, 2) && cui_focused(ui) == 24);
    assert(cui_questionnaire_reset(&s));
    cui_focus(ui, 22);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 24 && cui_questionnaire_choice_selected(&s, 0, 2));
    assert(!s.pending_confirm && !s.complete && cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 22 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT, .repeat = true}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 22 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 24 && cui_questionnaire_choice_selected(&s, 0, 2));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    radio_frame(ui, &s);
    assert(s.complete && cui_event_consumed(ui, 0));
    assert(cui_questionnaire_reset(&s));
    cui_focus(ui, 22);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .modifiers = CUI_MOD_CTRL}));
    radio_frame(ui, &s);
    assert(!s.complete && cui_questionnaire_item_error(&s, 0).kind == CUI_QUESTIONNAIRE_UNFILLED);
    assert(cui_questionnaire_activate(&s, 0, 2) == CUI_QUESTIONNAIRE_OK);
    cui_focus(ui, 26);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 24 && cui_questionnaire_choice_selected(&s, 0, 2));
    assert(!cui_questionnaire_choice_selected(&s, 0, 0) && !s.pending_confirm);
    assert(cui_questionnaire_activate(&s, 0, 0) == CUI_QUESTIONNAIRE_OK);
    cui_focus(ui, 26);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    radio_frame(ui, &s);
    assert(cui_focused(ui) == 22 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(!cui_questionnaire_choice_selected(&s, 0, 2) && !s.pending_confirm);

    cui_destroy(ui);
}
static bool same_color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static cui_draw_list card_frame(cui_context *ui, cui_questionnaire_state *s, const cui_theme *t,
                                cui_questionnaire_options o) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_questionnaire_begin(ui, 10, t, s, o));
    cui_questionnaire_choice_control(ui, 12, t, s, 0, 0, o);
    cui_questionnaire_end(ui);
    return cui_end_frame(ui);
}
static void card_paint(cui_context *ui, cui_draw_list list, cui_color fill, cui_color border,
                       float radius, float shortcut_font, const cui_theme *theme, int size) {
    cui_rect bounds = cui_bounds(ui, 12);
    bool found_fill = false, found_border = false, found_shortcut = false;
    bool found_indicator = false, found_badge = false;
    const float indicators[] = {16, 14, 18, 12}, badges[] = {20, 18, 24, 16};
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *d = &list.commands[i];
        if (d->type == CUI_DRAW_TEXT && d->text.length == 1 && d->text.data[0] == 'A') {
            assert(d->font_size == shortcut_font);
            found_shortcut = true;
        }
        if (d->type == CUI_DRAW_BORDER && d->bounds.w == indicators[size] &&
            d->bounds.h == indicators[size]) {
            assert(d->corner_radii.top_left == theme->radius / 2);
            found_indicator = true;
        }
        if (d->type == CUI_DRAW_BORDER && d->bounds.w == badges[size] &&
            d->bounds.h == badges[size]) {
            float badge_radius = size == CUI_QUESTIONNAIRE_XSMALL  ? theme->radius / 2
                                 : size == CUI_QUESTIONNAIRE_LARGE ? theme->radius_lg
                                                                   : theme->radius;
            assert(d->corner_radii.top_left == badge_radius);
            found_badge = true;
        }
        if (d->bounds.x != bounds.x || d->bounds.y != bounds.y || d->bounds.w != bounds.w ||
            d->bounds.h != bounds.h)
            continue;
        if (d->type == CUI_DRAW_RECT) {
            assert(same_color(d->color, fill));
            found_fill = true;
        } else if (d->type == CUI_DRAW_BORDER) {
            assert(same_color(d->color, border) && d->corner_radii.top_left == radius);
            found_border = true;
        }
    }
    assert(found_fill == (fill.a != 0) && found_border && found_shortcut && found_indicator &&
           found_badge);
}
static void source_card_styles(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_questionnaire_choice choice = {.value = cui_string("choice")};
    cui_questionnaire_item item = {
        .name = cui_string("item"), .choices = &choice, .choice_count = 1, .multiple = true};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected;
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = &selected,
                                                                .choice_capacity = 1},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    for (size_t palette = 0; palette < cui_theme_preset_count(); palette++) {
        cui_theme t;
        assert(cui_theme_preset_get(palette, &t));
        for (int size = 0; size < 4; size++) {
            cui_questionnaire_options o = {.width = 300, .size = (cui_questionnaire_size)size};
            float radius = size == CUI_QUESTIONNAIRE_XSMALL  ? t.radius
                           : size == CUI_QUESTIONNAIRE_LARGE ? t.radius * 2
                                                             : t.radius_lg;
            float font = size == CUI_QUESTIONNAIRE_XSMALL  ? 8
                         : size == CUI_QUESTIONNAIRE_SMALL ? 9
                         : size == CUI_QUESTIONNAIRE_LARGE ? 12
                                                           : 10;
            assert(cui_questionnaire_reset(&s));
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 390, .y = 290}));
            cui_draw_list list = card_frame(ui, &s, &t, o);
            cui_color fill = t.dark ? t.input : t.background;
            fill.a = t.dark ? (uint8_t)(fill.a * .2f + .5f) : 0;
            card_paint(ui, list, fill, t.input, radius, font, &t, size);
            assert(cui_questionnaire_activate(&s, 0, 0) == CUI_QUESTIONNAIRE_OK);
            list = card_frame(ui, &s, &t, o);
            cui_color border = t.primary;
            border.a = (uint8_t)(border.a * .4f + .5f);
            card_paint(ui, list, t.muted_background, border, radius, font, &t, size);
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20}));
            list = card_frame(ui, &s, &t, o);
            fill = t.muted_background;
            fill.a = (uint8_t)(fill.a * .5f + .5f);
            card_paint(ui, list, fill, border, radius, font, &t, size);
            o.disabled = true;
            list = card_frame(ui, &s, &t, o);
            fill = t.muted_background;
            fill.a = (uint8_t)(fill.a * .5f);
            border.a = (uint8_t)(border.a * .5f);
            card_paint(ui, list, fill, border, radius, font, &t, size);
        }
    }
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
typedef struct {
    unsigned calls[3];
    cui_id ids[3];
    cui_questionnaire_choice_view views[3];
} slot_probe;
static void custom_slot(cui_context *ui, cui_id id, const cui_questionnaire_choice_view *view,
                        slot_probe *probe, unsigned slot) {
    probe->calls[slot]++;
    probe->ids[slot] = id;
    probe->views[slot] = *view;
    cui_begin_box(ui, id,
                  (cui_box){.width = slot == 1 ? -1 : 13,
                            .height = slot == 0   ? 30
                                      : slot == 1 ? 40
                                                  : 7,
                            .background = {(uint8_t)(61 + slot), 73, 91, 255}});
    if (slot == 1)
        cui_text(ui, cui_string("custom choice"), 10, (cui_color){1, 2, 3, 255});
    cui_end_box(ui);
}
static void indicator_slot(cui_context *ui, cui_id id, const cui_questionnaire_choice_view *view,
                           void *user) {
    custom_slot(ui, id, view, user, 0);
}
static void content_slot(cui_context *ui, cui_id id, const cui_questionnaire_choice_view *view,
                         void *user) {
    custom_slot(ui, id, view, user, 1);
}
static void shortcut_slot(cui_context *ui, cui_id id, const cui_questionnaire_choice_view *view,
                          void *user) {
    custom_slot(ui, id, view, user, 2);
}
static void custom_choice_slots(void) {
    const float padding[] = {10, 8, 12, 4};
    const float indicator_offset[] = {4, 3, 5, 2};
    const float shortcut_offset[] = {2, 1, 2, 0};
    for (unsigned multiple = 0; multiple < 2; multiple++) {
        cui_context *ui = cui_create((cui_config){.max_elements = 256});
        cui_questionnaire_choice choice = {.value = cui_string("yes"),
                                           .label = cui_string("Default label"),
                                           .description = cui_string("Default description")};
        cui_questionnaire_item item = {.name = cui_string("question"),
                                       .multiple = multiple != 0,
                                       .choices = &choice,
                                       .choice_count = 1};
        cui_questionnaire_item_runtime runtime;
        cui_questionnaire_choice_runtime selected;
        cui_questionnaire_state state = {0};
        assert(cui_questionnaire_init(&state, &item, 1,
                                      (cui_questionnaire_workspace){.items = &runtime,
                                                                    .item_capacity = 1,
                                                                    .choices = &selected,
                                                                    .choice_capacity = 1},
                                      NULL, NULL) == CUI_QUESTIONNAIRE_OK);
        cui_theme theme = cui_theme_light();
        for (unsigned size = 0; size < 4; size++) {
            assert(cui_questionnaire_reset(&state));
            assert(cui_questionnaire_disable_choice(&state, 0, 0, false) == CUI_QUESTIONNAIRE_OK);
            assert(cui_questionnaire_external_error(&state, 0, NULL) == CUI_QUESTIONNAIRE_OK);
            slot_probe probe = {0};
            cui_questionnaire_options options = {.width = 300,
                                                 .size = (cui_questionnaire_size)size,
                                                 .render_indicator = indicator_slot,
                                                 .render_content = content_slot,
                                                 .render_shortcut = shortcut_slot,
                                                 .render_user = &probe};
            cui_draw_list draw = card_frame(ui, &state, &theme, options);
            cui_rect card = cui_bounds(ui, 12);
            for (unsigned slot = 0; slot < 3; slot++) {
                assert(probe.calls[slot] == 1 && probe.views[slot].item == 0 &&
                       probe.views[slot].choice == 0 &&
                       probe.views[slot].size == (cui_questionnaire_size)size &&
                       probe.views[slot].multiple == (multiple != 0) &&
                       !probe.views[slot].selected && !probe.views[slot].disabled &&
                       probe.views[slot].shortcut == 'A');
                cui_rect bounds = cui_bounds(ui, probe.ids[slot]);
                float offset = slot == 0   ? indicator_offset[size]
                               : slot == 2 ? shortcut_offset[size]
                                           : 0;
                assert(fabsf(bounds.y - card.y - padding[size] - offset) < .01f);
                assert(bounds.x >= card.x && bounds.x + bounds.w <= card.x + card.w);
            }
            assert(probe.ids[0] != probe.ids[1] && probe.ids[1] != probe.ids[2]);
            unsigned custom_text = 0;
            for (size_t i = 0; i < draw.count; i++) {
                if (draw.commands[i].type == CUI_DRAW_TEXT) {
                    assert(draw.commands[i].text.length == 13 &&
                           !memcmp(draw.commands[i].text.data, "custom choice", 13));
                    custom_text++;
                }
            }
            assert(custom_text == 1);
            /* Custom content preserves parent hit testing and native semantics. */
            cui_focus(ui, 12);
            assert(cui_queue_event(
                ui,
                (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = card.x + 20, .y = card.y + 20}));
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = card.x + 20, .y = card.y + 20}));
            card_frame(ui, &state, &theme, options);
            assert(cui_questionnaire_choice_selected(&state, 0, 0));
            assert(probe.views[0].selected && probe.views[0].focused);
            size_t count;
            const cui_semantic *semantics = cui_semantics(ui, &count);
            bool found = false;
            for (size_t i = 0; i < count; i++)
                if (semantics[i].id == 12) {
                    assert(semantics[i].role == (multiple ? CUI_ROLE_CHECKBOX : CUI_ROLE_RADIO));
                    assert(semantics[i].checked && semantics[i].name.length == 13);
                    found = true;
                }
            assert(found);
            cui_str error = cui_string("Try again");
            assert(cui_questionnaire_external_error(&state, 0, &error) == CUI_QUESTIONNAIRE_OK);
            assert(cui_questionnaire_disable_choice(&state, 0, 0, true) == CUI_QUESTIONNAIRE_OK);
            draw = card_frame(ui, &state, &theme, options);
            assert(probe.views[0].invalid && probe.views[0].disabled &&
                   probe.views[2].shortcut == 0);
            assert(probe.calls[2] ==
                   3); /* Custom shortcut remains available without a default key. */
            unsigned colored = 0;
            for (size_t i = 0; i < draw.count; i++)
                if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.g == 73 &&
                    draw.commands[i].color.b == 91) {
                    assert(draw.commands[i].color.a == 127);
                    colored++;
                }
            assert(colored == 3 && cui_last_error(ui) == CUI_OK);
        }
        cui_destroy(ui);
    }
}
static void progress_semantics(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_questionnaire_item items[3] = {
        {.name = cui_string("one")}, {.name = cui_string("two")}, {.name = cui_string("three")}};
    cui_questionnaire_item_runtime runtime[3];
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(
               &s, items, 3, (cui_questionnaire_workspace){.items = runtime, .item_capacity = 3},
               NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    cui_theme theme = cui_theme_light();
    for (size_t item = 0; item < 3; item++) {
        assert(cui_questionnaire_set_current(&s, item, false) == CUI_QUESTIONNAIRE_OK);
        assert(cui_begin_frame(ui, 400, 300, .016f));
        cui_questionnaire_progress(ui, 31, &theme, &s, (cui_questionnaire_options){0});
        cui_end_frame(ui);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 1 && sem[0].role == CUI_ROLE_PROGRESSBAR);
        assert(sem[0].value == (float)(item + 1));
        assert(sem[0].details.minimum == 0 && sem[0].details.maximum == 3);
    }
    cui_destroy(ui);
}
int main(void) {
    custom_choice_slots();
    source_card_styles();
    progress_semantics();
    radio_keyboard();
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_questionnaire_choice choice = {.value = cui_string("yes"),
                                       .label = cui_string("Yes"),
                                       .description = cui_string("A detailed answer")};
    cui_questionnaire_item item = {
        .name = cui_string("question"), .multiple = true, .choices = &choice, .choice_count = 1};
    cui_questionnaire_item_runtime runtime;
    cui_questionnaire_choice_runtime selected;
    cui_questionnaire_state s = {0};
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = &selected,
                                                                .choice_capacity = 1},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    for (int size = 0; size < 4; size++) {
        cui_questionnaire_options o = {.size = (cui_questionnaire_size)size, .width = 300};
        cui_str description = choice.description;
        choice.description = (cui_str){0};
        frame(ui, &s, o);
        const float minimum[] = {44, 36, 52, 28};
        assert(cui_bounds(ui, 12).h == minimum[size]);
        choice.description = description;
        frame(ui, &s, o);
        cui_rect bounds = cui_bounds(ui, 12);
        assert(bounds.w == 300 && bounds.h > 28);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                               .x = bounds.x + 20,
                                               .y = bounds.y + bounds.h / 2}));
        frame(ui, &s, o);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                               .x = bounds.x + 20,
                                               .y = bounds.y + bounds.h / 2}));
        frame(ui, &s, o);
        assert(cui_questionnaire_choice_selected(&s, 0, 0));
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        bool found = false;
        for (size_t i = 0; i < count; i++)
            if (sem[i].id == 12) {
                assert(sem[i].role == CUI_ROLE_CHECKBOX && sem[i].checked);
                found = true;
            }
        assert(found);
        assert(cui_questionnaire_reset(&s));
    }
    cui_questionnaire_options o = {.width = 300};
    assert(!cui_questionnaire_submit(&s));
    frame(ui, &s, o);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    bool error = false, skip = false, submit = false;
    for (size_t i = 0; i < count; i++) {
        assert(sem[i].id != 16); /* Previous hidden at the first item. */
        error |= sem[i].id == 14 && sem[i].role == CUI_ROLE_ALERT;
        skip |= sem[i].id == 17;
        submit |= sem[i].id == 18;
    }
    assert(error && skip && submit);
    cui_rect bounds = cui_bounds(ui, 17);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 4, .y = bounds.y + 4}));
    frame(ui, &s, o);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = bounds.x + 4, .y = bounds.y + 4}));
    frame(ui, &s, o);
    assert(s.complete && cui_questionnaire_item_status(&s, 0) == CUI_QUESTIONNAIRE_SKIPPED);
    char buffer[64] = {0}, storage[3 * 64];
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    assert(edit);
    item.input = edit;
    item.multiple = false;
    s = (cui_questionnaire_state){0};
    assert(cui_questionnaire_init(&s, &item, 1,
                                  (cui_questionnaire_workspace){.items = &runtime,
                                                                .item_capacity = 1,
                                                                .choices = &selected,
                                                                .choice_capacity = 1,
                                                                .text = storage,
                                                                .text_capacity = sizeof storage,
                                                                .max_answer_bytes = 63},
                                  NULL, NULL) == CUI_QUESTIONNAIRE_OK);
    assert(cui_questionnaire_activate(&s, 0, 0) == CUI_QUESTIONNAIRE_OK);
    frame(ui, &s, o);
    cui_focus(ui, 19);
    frame(ui, &s, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "custom"}));
    frame(ui, &s, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_A}));
    frame(ui, &s, o);
    assert(!cui_event_consumed(ui, 0) && cui_focused(ui) == 19);
    assert(cui_questionnaire_freeform(&s, 0).length == 6);
    assert(!cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    frame(ui, &s, o);
    assert(!s.complete);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame(ui, &s, o);
    assert(s.complete && cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    frame(ui, &s, o);
    assert(cui_focused(ui) == 19); /* Draft text retains cursor keys. */
    assert(cui_questionnaire_set_input(&s, 0, (cui_str){0}) == CUI_QUESTIONNAIRE_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    frame(ui, &s, o);
    assert(cui_focused(ui) == 12 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(!s.pending_confirm);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    frame(ui, &s, o);
    assert(cui_focused(ui) == 19 && cui_questionnaire_choice_selected(&s, 0, 0));
    assert(cui_questionnaire_set_input(&s, 0, cui_string("custom")) == CUI_QUESTIONNAIRE_OK);
    o.disabled = true;
    frame(ui, &s, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "ignored"}));
    frame(ui, &s, o);
    assert(cui_questionnaire_freeform(&s, 0).length == 6);
    cui_edit_destroy(edit);
    cui_destroy(ui);
    puts("questionnaire styled tests passed");
    return 0;
}
