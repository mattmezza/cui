/* SPDX-License-Identifier: MIT */
#include "cui/cui_alert_events.h"
#include "cui/cui_components.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { ALERT = 10, CUSTOM = 90 };
static const cui_id dismiss_id =
    ALERT ^ (UINT64_C(0x874e65fa19c80721) + UINT64_C(0x9e3779b97f4a7c15));
typedef struct {
    bool visible, hide_in_body, consume_in_body;
    unsigned order, custom_actions;
} slots;
typedef struct {
    slots *owner;
    bool restore, expect_assignment;
    unsigned calls;
    uint64_t frame;
    cui_alert_dismissal copied;
} observer;
static void title(cui_context *ui, const cui_theme *theme, cui_color fg, void *user) {
    slots *s = user;
    assert(s->order == 0);
    s->order = 1;
    cui_text(ui, cui_string("Title slot"), theme->font_size, fg);
}
static void body(cui_context *ui, const cui_theme *theme, cui_color fg, void *user) {
    slots *s = user;
    assert(s->order == 1);
    s->order = 12;
    if (s->hide_in_body)
        s->visible = false;
    if (s->consume_in_body) {
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; ++i)
            cui_consume_event(ui, i);
    }
    cui_text(ui, cui_string("Body slot"), theme->font_size, fg);
}
static void action(cui_context *ui, const cui_theme *theme, cui_color fg, void *user) {
    slots *s = user;
    (void)fg;
    assert(s->order == 12);
    s->order = 123;
    s->custom_actions += cui_button(ui, CUSTOM, theme, cui_string("Custom action"), false);
}
static void completed(void *user, const cui_alert_dismissal *dismissal) {
    observer *o = user;
    assert(o->owner->order == 123);
    assert(dismissal->visibility_assigned == o->expect_assignment);
    if (o->expect_assignment)
        assert(!o->owner->visible);
    assert(dismissal->event.kind == CUI_COMPONENT_EVENT_ACTIVATE);
    assert(dismissal->event.target == ALERT);
    assert(dismissal->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(dismissal->event.frame_sequence == o->frame);
    assert(dismissal->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(!dismissal->event.activation.target && !dismissal->event.event_index);
    o->copied = *dismissal;
    ++o->calls;
    if (o->restore)
        o->owner->visible = true;
}
static cui_context *context(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    assert(ui);
    return ui;
}
static cui_alert_options options(slots *s) {
    return (cui_alert_options){.kind = CUI_ALERT_INFO,
                               .width = 400,
                               .visible = &s->visible,
                               .dismissible = true,
                               .title_content = title,
                               .content = body,
                               .action = action,
                               .user = s,
                               .accessible_name = {"Test alert", 10}};
}
static cui_alert_dismiss_result frame(cui_context *ui, cui_alert_options opt, observer *o,
                                      unsigned passive, bool legacy, cui_draw_list *draw,
                                      size_t *accepted) {
    if (opt.user)
        ((slots *)opt.user)->order = 0;
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 600, .height = 400, .inert = passive == 1, .layout_only = passive == 2});
    if (o)
        o->frame = cui_frame_sequence(ui);
    cui_theme theme = cui_theme_light();
    cui_alert_dismiss_result result = {0};
    if (legacy)
        result.legacy_dismissed = cui_alert(ui, ALERT, &theme, opt);
    else
        result = cui_alert_events(ui, ALERT, &theme, opt,
                                  (cui_alert_dismiss_handler){o ? completed : NULL, o});
    if (accepted) {
        *accepted = 0;
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; ++i)
            *accepted += cui_event_accepted_activation(ui, i) == dismiss_id;
    }
    cui_end_box(ui);
    *draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void pointer(cui_context *ui, cui_id target, cui_pointer_type type) {
    cui_rect b = cui_bounds(ui, target);
    assert(b.w > 0 && b.h > 0);
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN,
                       .x = b.x + b.w / 2,
                       .y = b.y + b.h / 2,
                       .pointer_type = type,
                       .pointer_id = type == CUI_POINTER_TOUCH ? 7 : 0};
    assert(cui_queue_event(ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, event));
}
static void interaction(unsigned input, bool callback, bool storage) {
    cui_context *ui = context();
    slots s = {.visible = true};
    observer o = {.owner = &s, .expect_assignment = storage};
    cui_alert_options opt = options(&s);
    if (!storage)
        opt.visible = NULL;
    cui_draw_list draw;
    (void)frame(ui, opt, NULL, 0, false, &draw, NULL);
    if (input < 2)
        pointer(ui, dismiss_id, input == 0 ? CUI_POINTER_MOUSE : CUI_POINTER_TOUCH);
    else if (input == 4)
        assert(cui_queue_accessibility_action(ui, dismiss_id, CUI_ACCESSIBILITY_ACTIVATE));
    else {
        cui_focus(ui, dismiss_id);
        cui_key key = input == 2 ? CUI_KEY_ENTER : CUI_KEY_SPACE;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
        if (key == CUI_KEY_SPACE)
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = key}));
    }
    size_t accepted;
    cui_alert_dismiss_result result =
        frame(ui, opt, callback ? &o : NULL, 0, false, &draw, &accepted);
    assert(result.valid && result.submitted && result.legacy_dismissed);
    assert(result.dismissal_count == 1 && accepted == 1 && o.calls == (unsigned)callback);
    assert(s.visible == !storage);
    cui_alert_dismiss_result retained = result;
    cui_alert_dismissal copied = o.copied;
    cui_destroy(ui);
    assert(retained.dismissal_count == 1 && retained.legacy_dismissed);
    if (callback)
        assert(copied.event.target == ALERT && copied.visibility_assigned == storage);
}
static void multiple_and_slot_mutation(void) {
    for (unsigned mode = 0; mode < 6; ++mode) {
        cui_context *ui = context();
        slots s = {.visible = true};
        observer o = {.owner = &s, .expect_assignment = true, .restore = mode == 2};
        cui_alert_options opt = options(&s);
        cui_draw_list draw;
        (void)frame(ui, opt, NULL, 0, false, &draw, NULL);
        cui_focus(ui, dismiss_id);
        for (unsigned i = 0; i < 2; ++i) {
            if (mode == 3)
                assert(cui_queue_accessibility_action(ui, dismiss_id, CUI_ACCESSIBILITY_ACTIVATE));
            else
                assert(cui_queue_event(
                    ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        }
        s.hide_in_body = mode == 1;
        /* Core consumption cancels accessibility activation but does not revoke
         * an ordinary already-accepted Enter-down action. Preserve both paths. */
        s.consume_in_body = mode == 3 || mode == 4;
        size_t accepted;
        cui_alert_dismiss_result result =
            frame(ui, opt, mode == 5 ? NULL : &o, 0, false, &draw, &accepted);
        assert(result.valid && result.submitted && s.order == 123);
        if (mode == 3) {
            assert(!result.legacy_dismissed && !result.dismissal_count && !o.calls && !accepted);
            assert(s.visible);
        } else {
            assert(result.legacy_dismissed && result.dismissal_count == 1 &&
                   o.calls == (mode == 5 ? 0u : 1u));
            assert(accepted == 2 && s.visible == (mode == 2));
        }
        cui_destroy(ui);
    }
}
static void omission_and_custom_actions(void) {
    cui_context *ui = context();
    slots s = {0};
    observer o = {.owner = &s, .expect_assignment = true};
    cui_alert_options opt = options(&s);
    cui_draw_list draw;
    cui_alert_dismiss_result result = frame(ui, opt, &o, 0, false, &draw, NULL);
    assert(result.valid && !result.submitted && !result.legacy_dismissed &&
           !result.dismissal_count);
    assert(!s.order && !o.calls);
    s.visible = true;
    opt.dismissible = false;
    (void)frame(ui, opt, &o, 0, false, &draw, NULL);
    pointer(ui, CUSTOM, CUI_POINTER_MOUSE);
    result = frame(ui, opt, &o, 0, false, &draw, NULL);
    assert(result.submitted && !result.legacy_dismissed && !result.dismissal_count);
    assert(s.visible && s.custom_actions == 1 && !o.calls);
    opt.dismissible = true;
    s.hide_in_body = true;
    result = frame(ui, opt, &o, 0, false, &draw, NULL);
    assert(result.submitted && !result.legacy_dismissed && !result.dismissal_count && !s.visible);
    cui_destroy(ui);
}
static void parity_and_passive(void) {
    for (unsigned passive = 0; passive < 3; ++passive) {
        cui_context *old_ui = context(), *new_ui = context();
        slots a = {.visible = true}, b = {.visible = true};
        cui_alert_options old_opt = options(&a), new_opt = options(&b);
        cui_draw_list old_draw, new_draw;
        (void)frame(old_ui, old_opt, NULL, 0, true, &old_draw, NULL);
        (void)frame(new_ui, new_opt, NULL, 0, false, &new_draw, NULL);
        pointer(old_ui, dismiss_id, CUI_POINTER_MOUSE);
        pointer(new_ui, dismiss_id, CUI_POINTER_MOUSE);
        cui_alert_dismiss_result old_result =
            frame(old_ui, old_opt, NULL, passive, true, &old_draw, NULL);
        cui_alert_dismiss_result new_result =
            frame(new_ui, new_opt, NULL, passive, false, &new_draw, NULL);
        assert(old_result.legacy_dismissed == new_result.legacy_dismissed &&
               a.visible == b.visible);
        assert(new_result.dismissal_count == (passive ? 0u : 1u));
        assert(new_result.submitted == (passive == 0));
        size_t old_count, new_count;
        const cui_semantic *old_sem = cui_semantics(old_ui, &old_count);
        const cui_semantic *new_sem = cui_semantics(new_ui, &new_count);
        assert(old_count == new_count && old_draw.count == new_draw.count);
        for (size_t i = 0; i < old_count; ++i) {
            assert(old_sem[i].id == new_sem[i].id && old_sem[i].role == new_sem[i].role);
            assert(!memcmp(&old_sem[i].bounds, &new_sem[i].bounds, sizeof(cui_rect)));
            assert(old_sem[i].name.length == new_sem[i].name.length);
            assert(!memcmp(old_sem[i].name.data, new_sem[i].name.data, old_sem[i].name.length));
        }
        /* Portable software has no font backend: compare text commands exactly,
         * then rasterize every non-text paint/clip command, including SVG masks. */
        cui_draw_command old_shapes[256], new_shapes[256];
        cui_draw_list old_fill = {.commands = old_shapes}, new_fill = {.commands = new_shapes};
        assert(old_draw.count <= 256 && new_draw.count <= 256);
        for (size_t i = 0; i < old_draw.count; ++i) {
            const cui_draw_command *a_cmd = &old_draw.commands[i], *b_cmd = &new_draw.commands[i];
            assert(a_cmd->type == b_cmd->type);
            if (a_cmd->type == CUI_DRAW_TEXT) {
                assert(a_cmd->text.length == b_cmd->text.length);
                assert(!memcmp(a_cmd->text.data, b_cmd->text.data, a_cmd->text.length));
                assert(!memcmp(&a_cmd->bounds, &b_cmd->bounds, sizeof(cui_rect)));
                assert(!memcmp(&a_cmd->color, &b_cmd->color, sizeof(cui_color)));
                assert(a_cmd->font_size == b_cmd->font_size);
                assert(a_cmd->text_style.family == b_cmd->text_style.family);
                assert(a_cmd->text_style.weight == b_cmd->text_style.weight);
                assert(a_cmd->text_style.italic == b_cmd->text_style.italic);
                assert(a_cmd->text_style.monospace == b_cmd->text_style.monospace);
                assert(a_cmd->text_style.letter_spacing == b_cmd->text_style.letter_spacing);
            } else {
                old_shapes[old_fill.count++] = *a_cmd;
                new_shapes[new_fill.count++] = *b_cmd;
            }
        }
        cui_software *old_pixels = cui_software_create(600, 400, NULL);
        cui_software *new_pixels = cui_software_create(600, 400, NULL);
        assert(old_pixels && new_pixels);
        assert(cui_software_render(old_pixels, &old_fill, (cui_color){255, 255, 255, 255}));
        assert(cui_software_render(new_pixels, &new_fill, (cui_color){255, 255, 255, 255}));
        assert(!memcmp(cui_software_pixels(old_pixels, NULL, NULL),
                       cui_software_pixels(new_pixels, NULL, NULL), 600 * 400 * 4));
        cui_software_destroy(old_pixels);
        cui_software_destroy(new_pixels);
        cui_destroy(old_ui);
        cui_destroy(new_ui);
    }
}
static void invalid(void) {
    cui_context *ui = context();
    cui_theme theme = cui_theme_light();
    cui_alert_options opt = {0};
    assert(!cui_alert_events(NULL, ALERT, &theme, opt, (cui_alert_dismiss_handler){0}).valid);
    assert(!cui_alert_events(ui, 0, &theme, opt, (cui_alert_dismiss_handler){0}).valid);
    assert(!cui_alert_events(ui, ALERT, NULL, opt, (cui_alert_dismiss_handler){0}).valid);
    opt.kind = (cui_alert_kind)99;
    assert(!cui_alert_events(ui, ALERT, &theme, opt, (cui_alert_dismiss_handler){0}).valid);
    opt.kind = CUI_ALERT_INFO;
    opt.message = (cui_str){NULL, 1};
    assert(!cui_alert_events(ui, ALERT, &theme, opt, (cui_alert_dismiss_handler){0}).valid);
    cui_destroy(ui);
}
int main(void) {
    for (unsigned input = 0; input < 5; ++input)
        interaction(input, true, true);
    interaction(0, false, true);
    interaction(0, true, false);
    multiple_and_slot_mutation();
    omission_and_custom_actions();
    parity_and_passive();
    invalid();
    puts("Alert completion observer tests passed");
    return 0;
}
