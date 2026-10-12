/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void test_theme_list_tokens(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; i++) {
        cui_theme t = themes[i];
        assert(t.list_active_highlight);
        assert(memcmp(&t.popover_foreground, &t.text, sizeof(cui_color)) == 0);
        assert(t.list_hover.r == t.accent.r && t.list_hover.a == 153);
        assert(t.list_active.r == (9u * t.background.r + t.primary.r) / 10u);
        assert(t.list_active.a == 51);
        assert(t.selection.r == t.primary.r && t.selection.a == 76);
        assert(memcmp(&t.caret, &t.text, sizeof(cui_color)) == 0);
        assert(memcmp(&t.group_box_foreground, &t.text, sizeof(cui_color)) == 0);
        assert(t.group_box.a == 255);
    }
}
static bool checked;
static void scene(cui_context *ui, const cui_theme *t) {
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 640, .padding = 16, .gap = 8, .background = t->background});
    cui_label(ui, t, cui_string("Settings"));
    cui_button(ui, 2, t, cui_string("Save"), false);
    cui_checkbox(ui, 3, t, cui_string("Notifications"), &checked, false);
    cui_switch(ui, 4, t, cui_string("Sync"), &checked, true);
    cui_badge(ui, 5, t, cui_string("Preview"));
    cui_progress(ui, 6, t, .5f, 200);
    cui_separator(ui, 7, t, 300);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count > 10);
    assert(cui_last_error(ui) == CUI_OK);
    bool fill = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].bounds.w == 100)
            fill = true;
    assert(fill);
}
static void custom_button(cui_context *ui, void *user) {
    unsigned *calls = user;
    (*calls)++;
    cui_text(ui, cui_string("Custom content"), 12, (cui_color){255, 255, 255, 255});
}
static void test_icon_button(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    cui_theme theme = cui_theme_light();
    assert(ui && cui_begin_frame(ui, 200, 100, 0));
    cui_button_ex(ui, 99, &theme, cui_string("Close"),
                  (cui_button_options){.size = CUI_CONTROL_SMALL,
                                       .variant = CUI_BUTTON_GHOST,
                                       .icon = {"close", 5},
                                       .icon_only = true});
    cui_draw_list draw = cui_end_frame(ui);
    cui_rect bounds = cui_bounds(ui, 99);
    assert(bounds.w == 24 && bounds.h == 24);
    unsigned images = 0;
    for (size_t i = 0; i < draw.count; ++i) {
        assert(draw.commands[i].type != CUI_DRAW_TEXT);
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            cui_rect icon = draw.commands[i].bounds;
            assert(icon.w == 14 && icon.h == 14);
            assert(fabsf(icon.x + 7 - bounds.x - 12) < .01f);
            assert(fabsf(icon.y + 7 - bounds.y - 12) < .01f);
            images++;
        }
    }
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(images == 1 && count == 1 && nodes[0].name.length == 5);
    cui_focus(ui, 99);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_button_ex(
        ui, 99, &theme, cui_string("Close"),
        (cui_button_options){.size = CUI_CONTROL_SMALL, .icon = {"close", 5}, .icon_only = true}));
    cui_end_frame(ui);
    cui_destroy(ui);
}
static void test_button_options(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++)
        for (int variant = CUI_BUTTON_PRIMARY; variant <= CUI_BUTTON_INFO; variant++)
            for (int size = CUI_CONTROL_MEDIUM; size <= CUI_CONTROL_XSMALL; size++) {
                assert(cui_begin_frame(ui, 400, 200, .016f));
                cui_button_ex(ui, 123, &themes[theme], cui_string("Action"),
                              (cui_button_options){.variant = (cui_button_variant)variant,
                                                   .size = (cui_control_size)size,
                                                   .width = 180,
                                                   .leading = cui_string("+"),
                                                   .trailing = cui_string(">")});
                cui_draw_list list = cui_end_frame(ui);
                size_t count;
                const cui_semantic *nodes = cui_semantics(ui, &count);
                assert(count == 1 &&
                       nodes[0].role ==
                           (variant == CUI_BUTTON_LINK ? CUI_ROLE_LINK : CUI_ROLE_BUTTON));
                assert(!nodes[0].disabled && nodes[0].name.length == 6);
                float density = themes[theme].font_size / 16;
                float height = (size == CUI_CONTROL_XSMALL  ? 20
                                : size == CUI_CONTROL_SMALL ? 24
                                                            : 32) *
                               density;
                assert(fabsf(nodes[0].bounds.h - height) < .001f);
                assert(nodes[0].bounds.w == 180);
                unsigned text = 0;
                for (size_t i = 0; i < list.count; i++)
                    if (list.commands[i].type == CUI_DRAW_TEXT) {
                        const cui_draw_command *command = &list.commands[i];
                        float natural =
                            cui_measure_text(ui, command->text, command->font_size).height;
                        float center = command->bounds.y + natural * .5f;
                        assert(fabsf(center - (nodes[0].bounds.y + height * .5f)) < .6f);
                        assert(command->bounds.x >= nodes[0].bounds.x);
                        assert(command->bounds.x + command->bounds.w <=
                               nodes[0].bounds.x + nodes[0].bounds.w);
                        text++;
                    }
                assert(text == 3 && cui_last_error(ui) == CUI_OK);
            }
    unsigned calls = 0;
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_button_ex(
        ui, 123, &themes[0], cui_string("Accessible action"),
        (cui_button_options){.selected = true, .content = custom_button, .content_user = &calls});
    cui_end_frame(ui);
    assert(calls == 1);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(!nodes[0].checked && nodes[0].value == 0 && nodes[0].name.length == 17);
    cui_focus(ui, 123);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_button_ex(ui, 123, &themes[0], cui_string("Saving"),
                          (cui_button_options){.loading = true}));
    cui_end_frame(ui);
    nodes = cui_semantics(ui, &count);
    assert(count == 1 && nodes[0].disabled && !nodes[0].focused && nodes[0].details.busy);
    assert(nodes[0].details.description.length == 7 &&
           memcmp(nodes[0].details.description.data, "Loading", 7) == 0);
    assert(cui_focused(ui) == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_button_ex(ui, 123, &themes[0], cui_string("Saving"),
                          (cui_button_options){.loading = true}));
    cui_end_frame(ui);
    assert(cui_focused(ui) == 0 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void test_button_intent_outlines(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_button_variant variants[] = {CUI_BUTTON_DANGER, CUI_BUTTON_WARNING, CUI_BUTTON_SUCCESS,
                                     CUI_BUTTON_INFO};
    for (size_t t = 0; t < 4; t++)
        for (size_t v = 0; v < 4; v++) {
            cui_context *ui = cui_create((cui_config){.width = 300, .height = 200});
            assert(ui);
            cui_theme theme = themes[t];
            cui_color colors[] = {theme.danger, theme.warning, theme.success, theme.info};
            cui_button_options o = {.variant = variants[v], .outline = true, .width = 160};
            assert(cui_begin_frame(ui, 300, 200, .016f));
            assert(!cui_button_ex(ui, 18, &theme, cui_string("Intent"), o));
            cui_draw_list list = cui_end_frame(ui);
            bool text = false, border = false;
            for (size_t n = 0; n < list.count; n++) {
                const cui_draw_command *d = &list.commands[n];
                if (d->type == CUI_DRAW_TEXT) {
                    assert(memcmp(&d->color, &colors[v], sizeof(cui_color)) == 0);
                    text = true;
                }
                if (d->type == CUI_DRAW_BORDER) {
                    assert(memcmp(&d->color, &colors[v], sizeof(cui_color)) == 0);
                    border = true;
                }
            }
            assert(text && border);
            cui_rect r = cui_bounds(ui, 18);
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 10, .y = r.y + 10}));
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 10, .y = r.y + 10}));
            assert(cui_begin_frame(ui, 300, 200, .016f));
            assert(cui_button_ex(ui, 18, &theme, cui_string("Intent"), o));
            cui_end_frame(ui);
            o.disabled = true;
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
            assert(cui_begin_frame(ui, 300, 200, .016f));
            assert(!cui_button_ex(ui, 18, &theme, cui_string("Intent"), o));
            cui_end_frame(ui);
            size_t count;
            const cui_semantic *sem = cui_semantics(ui, &count);
            assert(count == 1 && sem[0].disabled && !sem[0].checked);
            assert(cui_last_error(ui) == CUI_OK);
            cui_destroy(ui);
        }
}

static void test_button_palette_overrides(void) {
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 200});
    assert(ui);
    cui_theme t = cui_theme_light();
    t.buttons[CUI_THEME_BUTTON_PRIMARY] =
        (cui_theme_button_colors){.normal = {10, 20, 30, 255},
                                  .hover = {40, 50, 60, 255},
                                  .active = {70, 80, 90, 255},
                                  .foreground = {255, 255, 255, 255},
                                  .outline_normal = {1, 2, 3, 255},
                                  .outline_foreground = {4, 5, 6, 255}};
    for (int state = 0; state < 4; state++) {
        if (state == 1)
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20}));
        if (state == 2)
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 20, .y = 20}));
        if (state == 3)
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
        assert(cui_begin_frame(ui, 300, 200, .016f));
        cui_button_ex(ui, 1, &t, cui_string("Palette"),
                      (cui_button_options){.width = 160, .selected = state == 3});
        cui_draw_list list = cui_end_frame(ui);
        unsigned expected = state == 0 ? 10 : state == 1 ? 40 : 70;
        bool found = false;
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].type == CUI_DRAW_RECT) {
                assert(list.commands[i].color.r == expected);
                found = true;
            }
        assert(found);
    }
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static double theme_luminance(cui_color c) {
    double channels[] = {c.r / 255.0, c.g / 255.0, c.b / 255.0};
    for (size_t i = 0; i < 3; i++)
        channels[i] =
            channels[i] <= .04045 ? channels[i] / 12.92 : pow((channels[i] + .055) / 1.055, 2.4);
    return .2126 * channels[0] + .7152 * channels[1] + .0722 * channels[2];
}
static void test_semantic_theme_contrast(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < sizeof(themes) / sizeof(*themes); i++) {
        cui_theme t = themes[i];
        cui_color pairs[][2] = {{t.warning, t.on_warning},
                                {t.success, t.on_success},
                                {t.info, t.on_info},
                                {t.danger, t.on_danger}};
        for (size_t p = 0; p < 4; p++) {
            double a = theme_luminance(pairs[p][0]), b = theme_luminance(pairs[p][1]);
            assert(pairs[p][0].a == 255 && pairs[p][1].a == 255);
            assert((fmax(a, b) + .05) / (fmin(a, b) + .05) >= 4.5);
            double surface = theme_luminance(t.surface);
            assert((fmax(a, surface) + .05) / (fmin(a, surface) + .05) >= 4.5);
        }
    }
}
static void test_checkbox_geometry(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_check_state value = CUI_CHECK_CHECKED;
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(!cui_checkbox_ex(ui, 99, &theme, cui_string("Choice"), &value,
                            (cui_checkbox_options){.required = true}));
    cui_draw_list list = cui_end_frame(ui);
    unsigned images = 0, labels = 0;
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type == CUI_DRAW_IMAGE) {
            images++;
            assert(c->bounds.w == 12 && c->bounds.h == 12 && c->bitmap.custom_tint);
            unsigned partial = 0, opaque = 0;
            for (unsigned j = 0; j < 256; j++) {
                unsigned a = c->bitmap.bitmap.pixels[j * 4 + 3];
                partial += a > 0 && a < 255;
                opaque += a == 255;
            }
            assert(partial && opaque);
        }
        if (c->type == CUI_DRAW_TEXT) {
            labels++;
            assert(c->text.length == 6 && !memcmp(c->text.data, "Choice", 6));
        }
    }
    assert(images == 1 && labels == 1);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].checked && sem[0].details.required);
    cui_focus(ui, 99);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(
        cui_checkbox_ex(ui, 99, &theme, cui_string("Choice"), &value, (cui_checkbox_options){0}));
    assert(value == CUI_CHECK_UNCHECKED);
    cui_end_frame(ui);
    value = CUI_CHECK_MIXED;
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(!cui_checkbox_ex(
        ui, 99, &theme, cui_string("Choice"), &value,
        (cui_checkbox_options){.disabled = true, .invalid = true, .size = CUI_CONTROL_LARGE}));
    list = cui_end_frame(ui);
    for (size_t i = 0; i < list.count; i++)
        assert(list.commands[i].type != CUI_DRAW_IMAGE);
    sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].value == CUI_CHECK_MIXED && sem[0].disabled &&
           sem[0].details.invalid);
    cui_destroy(ui);
}
static void test_radio_geometry(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    cui_theme theme = cui_theme_light();
    int selected = 1;
    for (int size = CUI_CONTROL_MEDIUM; size <= CUI_CONTROL_XSMALL; size++) {
        assert(cui_begin_frame(ui, 200, 100, .016f));
        assert(
            !cui_radio_ex(ui, 91, &theme, cui_string("Radio"), &selected, 1,
                          (cui_radio_options){.size = (cui_control_size)size, .disabled = true}));
        cui_draw_list list = cui_end_frame(ui);
        unsigned images = 0, labels = 0, circles = 0;
        float side = size == CUI_CONTROL_XSMALL  ? 12
                     : size == CUI_CONTROL_SMALL ? 14
                     : size == CUI_CONTROL_LARGE ? 18
                                                 : 16;
        for (size_t i = 0; i < list.count; i++) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_IMAGE) {
                images++;
                assert(c->bounds.w == side - 4 && c->bitmap.tint.a == theme.on_primary.a / 2);
            }
            if (c->type == CUI_DRAW_RECT && c->bounds.w == side && c->bounds.h == side) {
                circles++;
                assert(c->corner_radii.top_left == side / 2 && c->color.a == theme.primary.a / 2);
            }
            if (c->type == CUI_DRAW_TEXT) {
                labels++;
                assert(c->text.length == 5 && !memcmp(c->text.data, "Radio", 5));
            }
        }
        assert(images == 1 && labels == 1 && circles == 1);
    }
    assert(cui_begin_frame(ui, 200, 100, .016f));
    cui_radio(ui, 91, &theme, cui_string("Radio"), &selected, 2, false);
    cui_end_frame(ui);
    cui_focus(ui, 91);
    cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER});
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_radio(ui, 91, &theme, cui_string("Radio"), &selected, 2, false));
    assert(selected == 2);
    cui_end_frame(ui);
    cui_destroy(ui);
}

static cui_draw_list switch_frame(cui_context *ui, cui_theme *t, bool *value,
                                  cui_switch_state *state, cui_switch_options o) {
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_switch_ex(ui, 900, t, cui_string("Automatic sync"), value, state, o);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static unsigned switch_requests;
static bool switch_requested;
static void switch_request(void *user, bool requested) {
    assert(user == &switch_requests);
    switch_requests++;
    switch_requested = requested;
}
static void test_switch(void) {
    switch_requests = 0;
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t ti = 0; ti < 4; ti++)
        for (int size = CUI_CONTROL_MEDIUM; size <= CUI_CONTROL_XSMALL; size++)
            for (int enabled = 0; enabled < 2; enabled++)
                for (int on = 0; on < 2; on++) {
                    bool value = on;
                    cui_switch_options o = {.size = (cui_control_size)size, .disabled = !enabled};
                    cui_draw_list list = switch_frame(ui, &themes[ti], &value, NULL, o);
                    bool small = size == CUI_CONTROL_SMALL || size == CUI_CONTROL_XSMALL;
                    float w = small ? 28 : size == CUI_CONTROL_LARGE ? 44 : 36;
                    float h = small ? 16 : size == CUI_CONTROL_LARGE ? 24 : 20;
                    cui_rect track = cui_bounds(ui, cui_switch_track_id(900));
                    cui_rect thumb = cui_bounds(ui, cui_switch_thumb_id(900));
                    assert(track.w == w && track.h == h && thumb.w == h - 4 && thumb.h == h - 4);
                    assert(fabsf(thumb.x - track.x - (on ? w - h + 2 : 2)) < .01f);
                    assert(thumb.y == track.y + 2);
                    bool track_found = false, thumb_found = false;
                    unsigned texts = 0;
                    for (size_t i = 0; i < list.count; i++) {
                        const cui_draw_command *c = &list.commands[i];
                        if (c->type == CUI_DRAW_RECT && c->bounds.w == w && c->bounds.h == h) {
                            cui_color expected =
                                on ? themes[ti].primary : themes[ti].switch_background;
                            if (!enabled)
                                expected.a /= 2;
                            assert(memcmp(&expected, &c->color, sizeof(expected)) == 0);
                            assert(c->radius == (themes[ti].radius >= 4 ? h : themes[ti].radius));
                            track_found = true;
                        }
                        if (c->type == CUI_DRAW_RECT && c->bounds.w == h - 4) {
                            assert(memcmp(&c->color, &themes[ti].switch_thumb, sizeof(cui_color)) ==
                                   0);
                            thumb_found = true;
                        }
                        if (c->type == CUI_DRAW_TEXT) {
                            texts++;
                            assert(c->text.length == 14); /* No ON/OFF marker. */
                        }
                    }
                    assert(track_found && thumb_found && texts == 1);
                    size_t count;
                    const cui_semantic *nodes = cui_semantics(ui, &count);
                    assert(count == 1 && nodes[0].role == CUI_ROLE_SWITCH &&
                           nodes[0].checked == value);
                    assert(nodes[0].disabled == !enabled);
                }
    cui_theme t = themes[0];
    bool value = false;
    cui_switch_options o = {0};
    switch_frame(ui, &t, &value, NULL, o);
    cui_rect b = cui_bounds(ui, 900);
    /* Visible label belongs to the same control and receives pointer focus. */
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w - 2, .y = b.y + 5}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + b.w - 2, .y = b.y + 5}));
    switch_frame(ui, &t, &value, NULL, o);
    assert(value && cui_focused(ui) == 900);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    switch_frame(ui, &t, &value, NULL, o);
    assert(!value);
    o.on_change = switch_request;
    o.change_user = &switch_requests;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    switch_frame(ui, &t, &value, NULL, o);
    assert(switch_requests == 1 && switch_requested && !value);
    o.disabled = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    switch_frame(ui, &t, &value, NULL, o);
    assert(switch_requests == 1 && !value);
    o = (cui_switch_options){0};
    cui_switch_state state = {0};
    switch_frame(ui, &t, &value, &state, o);
    assert(state.position == 0 && state.next_wake_seconds == 0);
    value = true;
    switch_frame(ui, &t, &value, &state, o);
    assert(state.position == 0 && state.target == 16 && state.next_wake_seconds > 0);
    o.now_seconds = .07;
    switch_frame(ui, &t, &value, &state, o);
    assert(state.position > 0 && state.position < 16 && state.velocity > 0);
    double position = state.position, velocity = state.velocity;
    value = false;
    switch_frame(ui, &t, &value, &state, o);
    assert(state.position == position && state.velocity == velocity && state.target == 0);
    o.now_seconds = 3;
    switch_frame(ui, &t, &value, &state, o);
    assert(state.position == 0 && state.velocity == 0 && state.next_wake_seconds == 0);
    value = true;
    o.reduced_motion = true;
    switch_frame(ui, &t, &value, &state, o);
    assert(state.position == 16 && state.next_wake_seconds == 0);
    o = (cui_switch_options){.width = 136, .reduced_motion = true};
    cui_draw_list wrapped = switch_frame(ui, &t, &value, &state, o);
    assert(cui_bounds(ui, 900).h == 40);
    assert(cui_bounds(ui, cui_switch_track_id(900)).w == 36);
    unsigned lines = 0;
    for (size_t i = 0; i < wrapped.count; i++)
        if (wrapped.commands[i].type == CUI_DRAW_TEXT)
            lines++;
    assert(lines == 2);
    o = (cui_switch_options){.skip_tab = true};
    switch_frame(ui, &t, &value, NULL, o);
    cui_focus(ui, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    switch_frame(ui, &t, &value, NULL, o);
    assert(cui_focused(ui) == 0);
    o.now_seconds = NAN;
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_switch_ex(ui, 900, &t, cui_string("Sync"), &value, &state, o));
    cui_end_frame(ui);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0);
    cui_destroy(ui);
}

static void toggle_icon(cui_context *ui, void *user) {
    unsigned *calls = user;
    (*calls)++;
    cui_begin_box(ui, 0, (cui_box){.width = 8, .height = 8, .background = {200, 10, 30, 255}});
    cui_end_box(ui);
}
static cui_draw_list toggle_frame(cui_context *ui, cui_theme *t, bool *value,
                                  cui_toggle_options o) {
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_toggle_ex(ui, 910, t, cui_string("Bold"), value, NULL, o);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static bool group_requested[3];
static unsigned group_calls;
static void group_request(void *user, const bool *values, size_t count) {
    assert(user == &group_calls && count == 3);
    group_calls++;
    memcpy(group_requested, values, sizeof(group_requested));
}
static cui_draw_list group_frame(cui_context *ui, cui_theme *t, const cui_toggle_item *items,
                                 bool *values, cui_toggle_group_style_options o) {
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_toggle_group_ex(ui, 920, t, items, values, 3, o);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static void test_toggle(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t ti = 0; ti < 4; ti++)
        for (int size = 0; size <= CUI_CONTROL_XSMALL; size++)
            for (int variant = 0; variant <= CUI_TOGGLE_OUTLINE; variant++)
                for (int disabled = 0; disabled < 2; disabled++) {
                    bool value = true;
                    cui_toggle_options o = {.size = (cui_control_size)size,
                                            .variant = (cui_toggle_variant)variant,
                                            .disabled = disabled};
                    cui_draw_list list = toggle_frame(ui, &themes[ti], &value, o);
                    float side = size == CUI_CONTROL_XSMALL  ? 20
                                 : size == CUI_CONTROL_SMALL ? 24
                                 : size == CUI_CONTROL_LARGE ? 36
                                                             : 32;
                    cui_rect b = cui_bounds(ui, 910);
                    assert(b.h == side && b.w >= side);
                    unsigned text = 0, fill = 0, border = 0;
                    for (size_t i = 0; i < list.count; i++) {
                        const cui_draw_command *c = &list.commands[i];
                        if (c->type == CUI_DRAW_TEXT) {
                            text++;
                            assert(c->text.length == 4);
                            assert(memcmp(&c->color, &themes[ti].accent_foreground,
                                          sizeof(cui_color)) == 0);
                        }
                        if (c->type == CUI_DRAW_RECT && c->bounds.h == side) {
                            fill++;
                            assert(memcmp(&c->color, &themes[ti].accent, sizeof(cui_color)) == 0);
                        }
                        border += c->type == CUI_DRAW_BORDER;
                    }
                    assert(text == 1 && fill == 1 && border == (unsigned)variant);
                    size_t count;
                    const cui_semantic *nodes = cui_semantics(ui, &count);
                    assert(count == 1 && nodes[0].role == CUI_ROLE_BUTTON && nodes[0].checked);
                }
    cui_theme t = themes[0];
    bool value = false;
    unsigned content_calls = 0;
    cui_toggle_options o = {
        .content = toggle_icon, .content_user = &content_calls, .tooltip = {"Bold style", 10}};
    cui_draw_list list = toggle_frame(ui, &t, &value, o);
    assert(cui_bounds(ui, 910).w == 32 && content_calls == 1);
    for (size_t i = 0; i < list.count; i++)
        assert(list.commands[i].type != CUI_DRAW_TEXT);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(nodes[0].name.length == 10);
    cui_focus(ui, 910);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    toggle_frame(ui, &t, &value, o);
    assert(value);
    o.disabled = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    toggle_frame(ui, &t, &value, o);
    assert(value);

    o = (cui_toggle_options){0};
    value = false;
    toggle_frame(ui, &t, &value, o);
    cui_rect hover = cui_bounds(ui, 910);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = hover.x + 4, .y = hover.y + 4}));
    list = toggle_frame(ui, &t, &value, o);
    bool accent = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].bounds.h == 32)
            accent = memcmp(&list.commands[i].color, &t.accent, sizeof(cui_color)) == 0;
    assert(accent && !value);
    o.disabled = true;
    list = toggle_frame(ui, &t, &value, o);
    for (size_t i = 0; i < list.count; i++)
        assert(list.commands[i].type != CUI_DRAW_BORDER);
    o = (cui_toggle_options){.on_change = switch_request, .change_user = &switch_requests};
    unsigned requests = switch_requests;
    cui_focus(ui, 910);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    toggle_frame(ui, &t, &value, o);
    assert(switch_requests == requests + 1 && switch_requested && !value);
    o = (cui_toggle_options){.custom_corners = true, .corner_radii = {.top_left = NAN}};
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_toggle_ex(ui, 910, &t, cui_string("Bold"), &value, NULL, o));
    cui_end_frame(ui);
    cui_semantics(ui, &count);
    assert(count == 0);
    cui_toggle_item items[] = {{.id = 921, .label = {"A", 1}},
                               {.id = 922, .label = {"B", 1}},
                               {.id = 923, .label = {"C", 1}}};
    bool values[3] = {true, false, true}, scratch[3];
    cui_toggle_group_style_options group = {.variant = CUI_TOGGLE_OUTLINE, .segmented = true};
    list = group_frame(ui, &t, items, values, group);
    nodes = cui_semantics(ui, &count);
    assert(count == 4 && nodes[0].role == CUI_ROLE_TOOLBAR &&
           nodes[0].details.orientation == CUI_ORIENTATION_HORIZONTAL);
    unsigned borders = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_BORDER) {
            const cui_draw_command *c = &list.commands[i];
            assert(c->custom_corners && c->custom_border_edges);
            assert(c->border_edges.left == (borders == 0 ? 1 : 0));
            assert(c->corner_radii.top_left == (borders == 0 ? t.radius : 0));
            assert(c->corner_radii.top_right == (borders == 2 ? t.radius : 0));
            borders++;
        }
    assert(borders == 3);
    assert(cui_bounds(ui, 922).x == cui_bounds(ui, 921).x + cui_bounds(ui, 921).w);
    group.variant = CUI_TOGGLE_GHOST;
    for (size_t ti = 0; ti < 4; ti++) {
        list = group_frame(ui, &themes[ti], items, values, group);
        for (size_t i = 0; i < list.count; i++)
            assert(list.commands[i].type != CUI_DRAW_BORDER);
    }
    group.variant = CUI_TOGGLE_OUTLINE;
    group.on_change = group_request;
    group.change_user = &group_calls;
    group.requested_values = scratch;
    group.requested_capacity = 3;
    cui_focus(ui, 922);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    group_frame(ui, &t, items, values, group);
    assert(group_calls == 1 && group_requested[0] && group_requested[1] && group_requested[2]);
    assert(!values[1]); /* Controlled group never changes owner state. */
    nodes = cui_semantics(ui, &count);
    assert(count == 4 && !nodes[2].checked);
    cui_rect b = cui_bounds(ui, 923);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 4, .y = b.y + 4}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 4, .y = b.y + 4}));
    group_frame(ui, &t, items, values, group);
    assert(group_calls == 2 && group_requested[0] && !group_requested[1] && !group_requested[2]);
    group.disabled = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    group_frame(ui, &t, items, values, group);
    assert(group_calls == 2);
    group.on_change = NULL;
    group.disabled = false;
    group.segmented = false;
    group_frame(ui, &t, items, values, group);
    assert(cui_bounds(ui, 922).x - cui_bounds(ui, 921).x - cui_bounds(ui, 921).w == 8);
    cui_focus(ui, 922);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    group_frame(ui, &t, items, values, group);
    assert(values[0] && values[1] && values[2]);
    group.gap = NAN;
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_toggle_group_ex(ui, 920, &t, items, values, 3, group));
    cui_end_frame(ui);
    cui_semantics(ui, &count);
    assert(count == 0);
    group.gap = 0;
    items[2].id = 922;
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(!cui_toggle_group_ex(ui, 920, &t, items, values, 3, group));
    cui_end_frame(ui);
    cui_semantics(ui, &count);
    assert(count == 0);
    cui_destroy(ui);
}
static void test_button_loading_artwork(void) {
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 100});
    cui_theme t = cui_theme_light();
    assert(ui);
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 300, 100, .016f));
    assert(!cui_button_ex(ui, 990, &t, cui_string("Save"),
                          (cui_button_options){.loading = true, .size = CUI_CONTROL_SMALL}));
    cui_draw_list draw = cui_end_frame(ui);
    unsigned images = 0;
    for (size_t i = 0; i < draw.count; i++) {
        cui_draw_command c = draw.commands[i];
        if (c.type == CUI_DRAW_IMAGE) {
            images++;
            assert(c.bounds.w == 14 && c.bounds.h == 14 && cui_bitmap_valid(c.bitmap.bitmap));
            unsigned coverage = 0;
            for (unsigned y = 0; y < c.bitmap.bitmap.height; y++)
                for (unsigned x = 0; x < c.bitmap.bitmap.width; x++)
                    coverage += c.bitmap.bitmap.pixels[y * c.bitmap.bitmap.stride + x * 4 + 3] > 0;
            assert(coverage > 20);
        }
        assert(c.type != CUI_DRAW_TEXT || c.text.length != 3 || memcmp(c.text.data, "...", 3));
    }
    assert(images == 1 && !cui_needs_redraw(ui) && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
int main(void) {
    test_button_loading_artwork();
    test_theme_list_tokens();
    test_toggle();
    test_switch();
    test_checkbox_geometry();
    test_radio_geometry();
    test_semantic_theme_contrast();
    test_button_intent_outlines();
    test_button_palette_overrides();
    test_icon_button();
    test_button_options();
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 480});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; i++)
        scene(ui, &themes[i]);
    cui_rect b = cui_bounds(ui, 3);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 5, .y = b.y + 5}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 5, .y = b.y + 5}));
    scene(ui, &themes[0]);
    assert(checked);
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    assert(count == 3);
    assert(s[1].role == CUI_ROLE_CHECKBOX && s[1].checked);
    assert(s[2].disabled && s[2].role == CUI_ROLE_SWITCH);
    assert(themes[0].radius != themes[3].radius && themes[0].primary.b != themes[3].primary.b);
    char buffer[64] = "Ada";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof(buffer),
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 16});
    assert(edit);
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_input_result ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, false);
    assert(!ir.changed && !ir.submitted && ir.error == CUI_EDIT_OK);
    cui_end_frame(ui);
    cui_focus(ui, 44);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_A, .modifiers = CUI_MOD_CTRL}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Eva"}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, false);
    assert(ir.changed && ir.error == CUI_EDIT_OK);
    cui_end_frame(ui);
    assert(cui_edit_get_text(edit).length == 3 && buffer[0] == 'E');
    s = cui_semantics(ui, &count);
    assert(count == 1 && s[0].role == CUI_ROLE_TEXTBOX && s[0].value == 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, false);
    assert(ir.submitted && !ir.changed);
    cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "!"}));
    assert(cui_begin_frame(ui, 640, 480, .016f));
    ir = cui_input(ui, 44, &themes[0], cui_string("Name"), edit, true);
    assert(!ir.changed);
    cui_end_frame(ui);
    assert(cui_edit_get_text(edit).length == 3);
    cui_edit_destroy(edit);
    cui_destroy(ui);
    return 0;
}
