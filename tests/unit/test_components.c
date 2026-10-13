/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_design_metrics.h"
#include "cui/cui_components.h"
#include "cui/cui_media.h"
#include "cui/cui_status.h"
#include "cui/cui_textarea.h"
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
static void test_named_themes(void) {
    assert(cui_theme_preset_count() == 38);
    unsigned light = 0, dark = 0;
    bool ayu = false, tokyo = false;
    for (size_t i = 0; i < cui_theme_preset_count(); ++i) {
        cui_theme theme;
        assert(cui_theme_preset_get(i, &theme));
        assert(theme.dark == cui_theme_preset_is_dark(i));
        assert(theme.muted_background.a > 0);
        if (i == 0)
            assert(theme.muted_background.r == 245 && theme.muted_background.g == 245 &&
                   theme.muted_background.b == 245);
        cui_str name = cui_theme_preset_name(i);
        assert(name.data && name.length && theme.background.a == 255 && theme.text.a == 255);
        if (cui_theme_preset_is_dark(i))
            dark++;
        else
            light++;
        if (name.length == 9 && !memcmp(name.data, "Ayu Light", 9)) {
            ayu = true;
            assert(theme.background.r == 252 && theme.text.r == 92 && theme.primary.r == 85);
        }
        if (name.length == 12 && !memcmp(name.data, "Default Dark", 12)) {
            cui_theme_button_colors danger = theme.buttons[CUI_THEME_BUTTON_DANGER];
            assert(danger.normal.a == 51 && danger.hover.a == 76 && danger.active.a == 102);
            assert(danger.foreground.r == theme.danger.r && danger.foreground.g == theme.danger.g);
            assert(danger.outline_normal.a == 26 && danger.outline_border.a == 102);
        }
        if (name.length == 11 && !memcmp(name.data, "Tokyo Night", 11))
            tokyo = true;
        assert(theme.buttons[CUI_THEME_BUTTON_PRIMARY].normal.a > 0);
    }
    assert(light == 12 && dark == 26 && ayu && tokyo);
    cui_theme original = cui_theme_light(), untouched = original;
    assert(!cui_theme_preset_get(38, &untouched));
    assert(!memcmp(&original, &untouched, sizeof original));
    assert(!cui_theme_preset_name(38).length && !cui_theme_preset_get(0, NULL));
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

static void test_radio_shared_redraw(void) {
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 200});
    cui_theme theme = cui_theme_light();
    int value = 0;
    for (unsigned frame = 0; frame < 3; frame++) {
        if (frame == 1) {
            cui_focus(ui, 102);
            /* Stabilize focus first, so activation itself must invalidate. */
        }
        assert(cui_begin_frame(ui, 300, 200, .016f));
        cui_begin_box(ui, 100, (cui_box){.gap = 8});
        cui_radio(ui, 101, &theme, cui_string("First"), &value, 0, false);
        cui_radio(ui, 102, &theme, cui_string("Second"), &value, 1, false);
        cui_end_box(ui);
        cui_end_frame(ui);
    }
    assert(!cui_needs_redraw(ui));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 100, (cui_box){.gap = 8});
    cui_radio(ui, 101, &theme, cui_string("First"), &value, 0, false);
    assert(cui_radio(ui, 102, &theme, cui_string("Second"), &value, 1, false));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(value == 1 && cui_needs_redraw(ui));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 100, (cui_box){.gap = 8});
    cui_radio(ui, 101, &theme, cui_string("First"), &value, 0, false);
    cui_radio(ui, 102, &theme, cui_string("Second"), &value, 1, false);
    cui_end_box(ui);
    cui_end_frame(ui);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 2 && !semantics[0].checked && semantics[1].checked);
    assert(!cui_needs_redraw(ui));
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
typedef struct {
    bool *values;
    unsigned calls;
    bool accept;
} controlled_group_owner;
static void controlled_group_single(void *user, const bool *requested, size_t count) {
    controlled_group_owner *owner = user;
    assert(count == 3);
    owner->calls++;
    if (!owner->accept)
        return;
    for (size_t i = 0; i < count; i++)
        if (requested[i] && !owner->values[i]) {
            for (size_t j = 0; j < count; j++)
                owner->values[j] = i == j;
            return;
        }
}
static void test_toggle_controlled_sibling_state(void) {
    cui_toggle_item items[] = {{.id = 921, .label = {"Bold", 4}},
                               {.id = 922, .label = {"Italic", 6}},
                               {.id = 923, .label = {"Underline", 9}}};
    for (unsigned pass = 0; pass < 2; pass++) {
        bool accept = pass == 0;
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
        assert(ui);
        cui_theme theme = cui_theme_light();
        theme.text = (cui_color){11, 22, 33, 255};
        theme.accent_foreground = (cui_color){44, 55, 66, 255};
        bool values[3] = {true, false, false}, requested[3];
        controlled_group_owner owner = {values, 0, accept != 0};
        cui_toggle_group_style_options options = {.on_change = controlled_group_single,
                                                  .change_user = &owner,
                                                  .requested_values = requested,
                                                  .requested_capacity = 3};
        group_frame(ui, &theme, items, values, options);
        group_frame(ui, &theme, items, values, options);
        cui_rect bounds = cui_bounds(ui, 922);
        cui_event touch = {.type = CUI_EVENT_POINTER_DOWN,
                           .pointer_type = CUI_POINTER_TOUCH,
                           .pointer_id = 17,
                           .pointer_button = CUI_POINTER_PRIMARY,
                           .x = bounds.x + bounds.w / 2,
                           .y = bounds.y + bounds.h / 2};
        assert(cui_queue_event(ui, touch));
        touch.type = CUI_EVENT_POINTER_UP;
        assert(cui_queue_event(ui, touch));
        group_frame(ui, &theme, items, values, options);
        assert(owner.calls == 1 && values[0] == !accept && values[1] == (accept != 0) &&
               !values[2]);
        size_t count;
        const cui_semantic *nodes = cui_semantics(ui, &count);
        assert(count == 4);
        for (size_t i = 0; i < 3; i++)
            assert(nodes[i + 1].id == items[i].id && nodes[i + 1].checked == values[i] &&
                   nodes[i + 1].value == (values[i] ? 1 : 0));
        assert(cui_needs_redraw(ui));
        cui_draw_list draw = group_frame(ui, &theme, items, values, options);
        assert(owner.calls == 1); /* Repaint must not repeat the controlled request. */
        if (accept) {
            bool bold = false, italic = false;
            for (size_t i = 0; i < draw.count; i++) {
                const cui_draw_command *command = &draw.commands[i];
                if (command->type != CUI_DRAW_TEXT)
                    continue;
                if (command->text.length == 4 && !memcmp(command->text.data, "Bold", 4)) {
                    assert(!memcmp(&command->color, &theme.text, sizeof(cui_color)));
                    bold = true;
                }
                if (command->text.length == 6 && !memcmp(command->text.data, "Italic", 6)) {
                    assert(!memcmp(&command->color, &theme.accent_foreground, sizeof(cui_color)));
                    italic = true;
                }
            }
            assert(bold && italic);
        }
        assert(!cui_needs_redraw(ui));
        cui_destroy(ui);
    }
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
static void test_button_paint_composition(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 200});
    cui_theme theme = cui_theme_light();
    cui_button_options options = {.variant = CUI_BUTTON_OUTLINE, .width = 160};
    cui_color active = theme.buttons[CUI_THEME_BUTTON_DEFAULT].outline_active;
    for (unsigned frame_index = 0; frame_index < 3; frame_index++) {
        assert(cui_begin_frame(ui, 320, 200, .016f));
        cui_interaction interaction = cui_base_button(ui, 451, cui_string("Composed"), false);
        cui_set_semantic_details(ui, 451,
                                 (cui_semantic_details){.expanded = true, .controls = 999});
        cui_button_paint(ui, 451, &theme, cui_string("Composed"), options);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        size_t count = 0;
        const cui_semantic *semantics = cui_semantics(ui, &count);
        assert(count == 1 && semantics[0].id == 451 && semantics[0].details.expanded &&
               semantics[0].details.controls == 999);
        assert(cui_bounds(ui, 451).h == 32);
        if (frame_index == 0) {
            cui_rect bounds = cui_bounds(ui, 451);
            assert(cui_queue_event(
                ui,
                (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 4, .y = bounds.y + 4}));
        } else if (frame_index == 1) {
            assert(!interaction.activated && interaction.pressed);
            bool held_color = false;
            for (size_t i = 0; i < list.count; i++)
                held_color |= list.commands[i].type == CUI_DRAW_RECT &&
                              memcmp(&list.commands[i].color, &active, sizeof active) == 0;
            assert(held_color);
            cui_rect bounds = cui_bounds(ui, 451);
            assert(cui_queue_event(
                ui,
                (cui_event){.type = CUI_EVENT_POINTER_UP, .x = bounds.x + 4, .y = bounds.y + 4}));
        } else {
            assert(interaction.activated && !interaction.pressed);
        }
    }
    /* Painting alone never adds a semantic or callback-driven activation. */
    assert(cui_begin_frame(ui, 320, 200, .016f));
    cui_button_paint(ui, 452, &theme, cui_string("Decorative"), options);
    cui_end_frame(ui);
    size_t count = 0;
    cui_semantics(ui, &count);
    assert(count == 0 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_button_hover_and_disabled_paint(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 200});
    cui_theme theme = cui_theme_light();
    cui_button_options options = {.variant = CUI_BUTTON_OUTLINE, .width = 160};
    assert(cui_begin_frame(ui, 320, 200, .016f));
    assert(!cui_button_ex(ui, 461, &theme, cui_string("Hover"), options));
    cui_end_frame(ui);
    cui_rect bounds = cui_bounds(ui, 461);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = bounds.x + 4, .y = bounds.y + 4}));
    assert(cui_begin_frame(ui, 320, 200, .016f));
    assert(!cui_button_ex(ui, 461, &theme, cui_string("Hover"), options));
    cui_draw_list list = cui_end_frame(ui);
    cui_color hover = theme.buttons[CUI_THEME_BUTTON_DEFAULT].outline_hover;
    bool hovered_color = false;
    for (size_t i = 0; i < list.count; i++)
        hovered_color |= list.commands[i].type == CUI_DRAW_RECT &&
                         memcmp(&list.commands[i].color, &hover, sizeof hover) == 0;
    assert(hovered_color);
    options.disabled = true;
    assert(cui_begin_frame(ui, 320, 200, .016f));
    assert(!cui_button_ex(ui, 461, &theme, cui_string("Hover"), options));
    list = cui_end_frame(ui);
    hovered_color = false;
    for (size_t i = 0; i < list.count; i++)
        hovered_color |= list.commands[i].type == CUI_DRAW_RECT &&
                         memcmp(&list.commands[i].color, &hover, sizeof hover) == 0;
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(!hovered_color && count == 1 && semantics[0].disabled);
    cui_destroy(ui);
}
static void badge_content(cui_context *ui, void *user) {
    (*(unsigned *)user)++;
    cui_begin_box(ui, 713, (cui_box){.width = 80, .height = 40, .background = {20, 40, 60, 255}});
    cui_text(ui, cui_string("Inbox"), 16, (cui_color){255, 255, 255, 255});
    cui_end_box(ui);
}
static void test_badge_variants(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 100});
    cui_theme theme = cui_theme_light();
    const cui_id id = 712, marker = id ^ UINT64_C(0x2780a7c4f39251db);
    unsigned calls = 0;
    cui_badge_options o = cui_badge_defaults();
    assert(o.maximum == 99 && o.variant == CUI_BADGE_COUNT);
    for (int size = CUI_CONTROL_MEDIUM; size <= CUI_CONTROL_XSMALL; size++) {
        o.size = (cui_control_size)size;
        for (int variant = CUI_BADGE_COUNT; variant <= CUI_BADGE_ICON; variant++) {
            o.variant = (cui_badge_variant)variant;
            o.count = 100;
            o.icon = cui_string("check");
            o.name = cui_string("Unread inbox");
            o.custom_color = true;
            o.color = (cui_color){120, 40, 70, 255};
            assert(cui_begin_frame(ui, 400, 100, .016f));
            assert(cui_badge_ex(ui, id, &theme, o, badge_content, &calls));
            cui_draw_list list = cui_end_frame(ui);
            cui_rect parent = cui_bounds(ui, id), content = cui_bounds(ui, 713),
                     mark = cui_bounds(ui, marker);
            assert(parent.w == 80 && parent.h == 40 && content.w == 80 && content.h == 40);
            float extent = size == CUI_CONTROL_LARGE ? 24 : size == CUI_CONTROL_MEDIUM ? 16 : 10;
            if (variant == CUI_BADGE_COUNT) {
                float x = (size == CUI_CONTROL_LARGE ? 1 : size == CUI_CONTROL_MEDIUM ? 3 : 4) * 3;
                float y = size == CUI_CONTROL_LARGE ? 2 : size == CUI_CONTROL_MEDIUM ? -3 : -4;
                assert(fabsf(mark.x + mark.w - (parent.x + parent.w + x)) < .01f);
                assert(mark.y == parent.y + y && mark.h == 14 && mark.w >= 14);
            } else {
                float diameter = variant == CUI_BADGE_DOT ? 6 : extent;
                assert(mark.w == diameter && mark.h == diameter);
                assert(mark.x + mark.w == parent.x + parent.w);
                assert(mark.y ==
                       (variant == CUI_BADGE_DOT ? parent.y : parent.y + parent.h - diameter));
            }
            size_t count;
            const cui_semantic *sem = cui_semantics(ui, &count);
            assert(count == 1 && sem[0].role == CUI_ROLE_STATUS && sem[0].details.parent == id);
            assert(sem[0].name.length == 12 && !memcmp(sem[0].name.data, "Unread inbox", 12));
            assert(sem[0].tab_index == -1 && !sem[0].focused);
            bool painted = false, overflow = false, icon = false;
            for (size_t i = 0; i < list.count; i++) {
                cui_draw_command command = list.commands[i];
                if (command.type == CUI_DRAW_RECT && command.color.r == 120 &&
                    command.color.g == 40)
                    painted = true;
                if (command.type == CUI_DRAW_TEXT && command.text.length == 3 &&
                    !memcmp(command.text.data, "99+", 3))
                    overflow = true;
                if (command.type == CUI_DRAW_IMAGE)
                    icon = true;
            }
            assert(painted && overflow == (variant == CUI_BADGE_COUNT) &&
                   icon == (variant == CUI_BADGE_ICON));
        }
    }
    o = cui_badge_defaults();
    assert(cui_begin_frame(ui, 400, 100, .016f));
    assert(cui_badge_ex(ui, id, &theme, o, badge_content, &calls));
    cui_end_frame(ui);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0 && cui_bounds(ui, id).w == 80 && cui_bounds(ui, id).h == 40);
    unsigned prior = calls;
    assert(cui_begin_frame(ui, 400, 100, .016f));
    o.variant = (cui_badge_variant)99;
    assert(!cui_badge_ex(ui, id, &theme, o, badge_content, &calls));
    cui_draw_list list = cui_end_frame(ui);
    assert(calls == prior && list.count == 0);
    cui_destroy(ui);
}
static void test_slider_semantics(void) {
    cui_context *ui = cui_create((cui_config){0});
    cui_theme theme = cui_theme_light();
    float value = 20;
    assert(cui_begin_frame(ui, 400, 200, 0));
    assert(!cui_slider(ui, 901, &theme, cui_string("Level"), &value, 10, 50, 1, false));
    cui_draw_list draw = cui_end_frame(ui);
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].role == CUI_ROLE_SLIDER && semantics[0].value == 20);
    assert(semantics[0].details.minimum == 10 && semantics[0].details.maximum == 50);
    bool fill = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.w == 60 &&
            draw.commands[i].bounds.h == 8)
            fill = true;
    assert(fill);
    cui_focus(ui, 901);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    assert(cui_begin_frame(ui, 400, 200, 0));
    assert(cui_slider(ui, 901, &theme, cui_string("Level"), &value, 10, 50, 1, false));
    cui_end_frame(ui);
    semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].value == 50 && value == 50);
    cui_destroy(ui);
}
static void circle_child(cui_context *ui, void *user) {
    unsigned *calls = user;
    ++*calls;
    cui_text(ui, cui_string("50"), 10, (cui_color){255, 255, 255, 255});
}
static void test_progress_circle(void) {
    cui_context *ui = cui_create((cui_config){0});
    cui_theme theme = cui_theme_light();
    unsigned char pixels[80 * 80 * 4];
    cui_progress_circle_state state = {pixels, sizeof pixels, 0};
    cui_progress_circle_options options = cui_progress_circle_defaults();
    const float sizes[] = {16, 12, 20, 8};
    unsigned calls = 0;
    for (unsigned i = 0; i < 4; i++) {
        options.progress.size = (cui_control_size)i;
        assert(cui_begin_frame(ui, 300, 200, 0));
        assert(!cui_progress_circle(ui, 100, &theme, .5f, &state, options, NULL, NULL));
        cui_draw_list draw = cui_end_frame(ui);
        assert(cui_bounds(ui, 100).w == sizes[i] && cui_bounds(ui, 100).h == sizes[i]);
        bool image = false;
        for (size_t j = 0; j < draw.count; j++)
            if (draw.commands[j].type == CUI_DRAW_IMAGE)
                image = true;
        assert(image && cui_last_error(ui) == CUI_OK);
        size_t count = 0;
        const cui_semantic *semantic = cui_semantics(ui, &count);
        assert(count == 1 && semantic[0].role == CUI_ROLE_PROGRESSBAR && semantic[0].value == .5f);
    }
    options.diameter = 40;
    options.progress.indeterminate = true;
    options.progress.now_seconds = .25;
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_progress_circle(ui, 100, &theme, .5f, &state, options, circle_child, &calls));
    cui_end_frame(ui);
    assert(calls == 1);
    options.progress.reduced_motion = true;
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(!cui_progress_circle(ui, 100, &theme, .5f, &state, options, NULL, NULL));
    cui_end_frame(ui);
    cui_progress_motion motion = {0};
    options.progress.indeterminate = false;
    options.progress.motion = &motion;
    options.progress.reduced_motion = false;
    state.capacity = 1;
    cui_progress_motion old = motion;
    uint64_t revision = state.revision;
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(!cui_progress_circle(ui, 100, &theme, 1, &state, options, circle_child, &calls));
    assert(cui_end_frame(ui).count == 0 && calls == 1 && state.revision == revision &&
           !memcmp(&motion, &old, sizeof old));
    cui_destroy(ui);
}
static void test_progress_source_style(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 80});
    cui_theme theme = cui_theme_light();
    assert(ui);
    const cui_id id = 90, fill = id ^ UINT64_C(0x7129d4cd19342f11);
    const float heights[] = {8, 6, 10, 4};
    for (int size = CUI_CONTROL_MEDIUM; size <= CUI_CONTROL_XSMALL; size++) {
        cui_progress_options o = cui_progress_defaults();
        o.size = (cui_control_size)size;
        o.color = (cui_color){18, 77, 133, 200};
        o.custom_color = true;
        assert(cui_begin_frame(ui, 200, 80, .016f));
        assert(!cui_progress_ex(ui, id, &theme, .5f, 100, o));
        cui_draw_list draw = cui_end_frame(ui);
        assert(cui_bounds(ui, id).h == heights[size] && cui_bounds(ui, fill).w == 50);
        bool track = false, indicator = false;
        for (size_t i = 0; i < draw.count; i++) {
            cui_draw_command c = draw.commands[i];
            if (c.type == CUI_DRAW_RECT && c.color.r == 18 && c.color.g == 77 && c.color.b == 133) {
                assert(c.radius == heights[size] / 2);
                track |= c.color.a == 40;
                indicator |= c.color.a == 200;
            }
        }
        assert(track && indicator);
    }
    cui_progress_motion motion = {0};
    cui_progress_options o = cui_progress_defaults();
    o.motion = &motion;
    for (unsigned frame = 0; frame < 4; frame++) {
        o.now_seconds = frame < 2 ? 0 : frame == 2 ? .036 : .05;
        o.reduced_motion = frame == 3;
        assert(cui_begin_frame(ui, 200, 80, .016f));
        bool moving = cui_progress_ex(ui, id, &theme, frame == 0 ? 0 : 1, 100, o);
        cui_end_frame(ui);
        assert(moving == (frame == 1 || frame == 2));
        if (frame == 2)
            assert(fabsf(cui_bounds(ui, fill).w - 50) < .01f);
        if (frame == 3)
            assert(cui_bounds(ui, fill).w == 100);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 1 && sem[0].value == (frame == 0 ? 0 : 1));
    }
    theme.radius = 0;
    o = cui_progress_defaults();
    o.indeterminate = true;
    o.now_seconds = .25;
    assert(cui_begin_frame(ui, 200, 80, .016f));
    assert(cui_progress_ex(ui, id, &theme, .8f, 100, o));
    cui_draw_list draw = cui_end_frame(ui);
    assert(fabsf(cui_bounds(ui, fill).w - 12.5f) < 1);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT)
            assert(draw.commands[i].radius == 0);
    cui_destroy(ui);
}
static void test_progress_motion(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 40});
    cui_theme theme = cui_theme_light();
    const cui_id fill = 91 ^ UINT64_C(0x7129d4cd19342f11);
    assert(ui);
    for (unsigned frame = 0; frame < 5; ++frame) {
        assert(cui_begin_frame(ui, 200, 40, .016f));
        bool reduced = frame >= 3;
        if (frame == 4)
            cui_set_reduced_motion(ui, true);
        bool moving = cui_progress_ex(ui, 91, &theme, .9f, 100,
                                      (cui_progress_options){.indeterminate = true,
                                                             .now_seconds = frame * .4,
                                                             .reduced_motion = frame == 3});
        cui_end_frame(ui);
        cui_rect track = cui_bounds(ui, 91), segment = cui_bounds(ui, fill);
        const float positions[] = {0, 35, 70, 35, 35};
        assert(moving == !reduced);
        assert(fabsf(segment.x - track.x - positions[frame]) < .01f);
        assert(fabsf(segment.w - 30) < .01f && segment.h == 8);
        assert(segment.x >= track.x && segment.x + segment.w <= track.x + track.w + .01f);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 1 && sem[0].role == CUI_ROLE_PROGRESSBAR && sem[0].details.busy);
    }
    assert(cui_begin_frame(ui, 200, 40, .016f));
    assert(!cui_progress_ex(ui, 91, &theme, 1.5f, 100, (cui_progress_options){.height = 12}));
    cui_end_frame(ui);
    assert(cui_bounds(ui, fill).w == 100 && cui_bounds(ui, fill).h == 12);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && !sem[0].details.busy && sem[0].value == 1);
    cui_destroy(ui);
}
static void design_scale(void) {
    assert(cui_space(1) == 4 && cui_space(1.5f) == 6 && cui_space(-2) == -8);
    assert(cui_space(NAN) == 0 && cui_space(INFINITY) == 0);
    assert(cui_radius(CUI_RADIUS_NONE, 1) == 0);
    assert(cui_radius(CUI_RADIUS_LG, 1) == 8);
    assert(cui_radius(CUI_RADIUS_3XL, 2) == 48);
    assert(cui_radius(CUI_RADIUS_FULL, 1) == 9999);
    cui_design_metrics untouched = {1, 2, 3, 4, 5, 6, 7}, invalid = untouched;
    assert(!cui_design_measure(CUI_DESIGN_INHERIT, 1, &invalid));
    assert(!cui_design_measure((cui_design_size)-1, 1, &invalid));
    assert(!cui_design_measure(CUI_DESIGN_XL, NAN, &invalid));
    assert(!memcmp(&invalid, &untouched, sizeof(invalid)));
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme_index = 0; theme_index < 4; theme_index++) {
        cui_theme theme = themes[theme_index];
        float previous_height = 0;
        for (int scale = CUI_DESIGN_XS; scale <= CUI_DESIGN_9XL; scale++) {
            cui_design_metrics metrics;
            assert(cui_design_measure((cui_design_size)scale, theme.font_size / 16, &metrics));
            assert(metrics.control_height >= previous_height);
            assert(metrics.line_height >= metrics.font_size);
            assert(metrics.line_height + 2 * metrics.padding_y == metrics.control_height);
            previous_height = metrics.control_height;
            cui_context *ui = cui_create((cui_config){.width = 1200, .height = 2000});
            assert(ui);
            char buffer[32] = "Hello";
            cui_edit *edit =
                cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
            assert(edit);
            char multiline_buffer[32] = "Alpha\nBeta";
            cui_edit *multiline =
                cui_edit_create((cui_edit_config){.buffer = multiline_buffer,
                                                  .capacity = sizeof(multiline_buffer),
                                                  .multiline = true});
            assert(multiline);
            cui_textarea_line lines[8];
            cui_textarea_state textarea;
            assert(cui_textarea_state_init(&textarea, lines, 8));
            cui_input_state input = {0};
            cui_check_state checked = CUI_CHECK_UNCHECKED;
            int selected = 0;
            bool enabled = false, toggled = false;
            for (unsigned frame = 0; frame < 3; frame++) {
                assert(cui_begin_frame(ui, 1200, 2000, .016f));
                cui_begin_box(ui, 100, (cui_box){.width = 1200, .height = 2000, .gap = 4});
                cui_button_ex(
                    ui, 1, &theme, cui_string("Save"),
                    (cui_button_options){.width = 800, .design_size = (cui_design_size)scale});
                cui_button_ex(ui, 2, &theme, cui_string("Close"),
                              (cui_button_options){.icon = cui_string("close"),
                                                   .icon_only = true,
                                                   .design_size = (cui_design_size)scale});
                cui_checkbox_ex(ui, 3, &theme, cui_string("Updates"), &checked,
                                (cui_checkbox_options){.design_size = (cui_design_size)scale});
                cui_radio_ex(ui, 4, &theme, cui_string("Priority"), &selected, 1,
                             (cui_radio_options){.design_size = (cui_design_size)scale});
                cui_switch_ex(ui, 5, &theme, cui_string("Enabled"), &enabled, NULL,
                              (cui_switch_options){.design_size = (cui_design_size)scale});
                cui_toggle_ex(ui, 6, &theme, cui_string("Bold"), &toggled, NULL,
                              (cui_toggle_options){.design_size = (cui_design_size)scale});
                cui_input_ex(
                    ui, 7, &theme, cui_string("Name"), edit, &input,
                    (cui_input_options){.width = 800, .design_size = (cui_design_size)scale});
                cui_textarea_result area = cui_textarea(
                    ui, 8, &theme, cui_string("Notes"), multiline, &textarea,
                    (cui_textarea_options){
                        .width = 800, .rows = 2, .design_size = (cui_design_size)scale});
                assert(area.error == CUI_TEXTAREA_OK);
                cui_end_box(ui);
                cui_draw_list draw = cui_end_frame(ui);
                assert(cui_last_error(ui) == CUI_OK);
                assert(cui_bounds(ui, 1).h == metrics.control_height);
                assert(cui_bounds(ui, 2).w == metrics.control_height &&
                       cui_bounds(ui, 2).h == metrics.control_height);
                assert(cui_bounds(ui, 6).h == metrics.control_height);
                assert(cui_bounds(ui, 7).h == metrics.control_height);
                assert(cui_bounds(ui, 8).h == 2 * metrics.line_height + 2 * metrics.padding_y + 2);
                assert(textarea.font_size == metrics.font_size &&
                       textarea.line_height == metrics.line_height && textarea.count == 2);
                bool save_text = false, close_icon = false;
                cui_rect button = cui_bounds(ui, 1);
                for (size_t j = 0; j < draw.count; j++) {
                    cui_draw_command command = draw.commands[j];
                    if (command.type == CUI_DRAW_TEXT && command.text.length == 4 &&
                        !memcmp(command.text.data, "Save", 4)) {
                        save_text = true;
                        assert(command.font_size == metrics.font_size);
                        assert(fabsf(command.bounds.y +
                                     cui_measure_text(ui, command.text, command.font_size).height *
                                         .5f -
                                     (button.y + button.h * .5f)) < .1f);
                    }
                    cui_rect icon_button = cui_bounds(ui, 2);
                    if (command.type == CUI_DRAW_IMAGE && command.bounds.x >= icon_button.x - 1 &&
                        command.bounds.y >= icon_button.y - 1 &&
                        command.bounds.x + command.bounds.w <= icon_button.x + icon_button.w + 1 &&
                        command.bounds.y + command.bounds.h <= icon_button.y + icon_button.h + 1 &&
                        fabsf(command.bounds.w - metrics.indicator_size) < 1 &&
                        fabsf(command.bounds.h - metrics.indicator_size) < 1)
                        close_icon = true;
                }
                assert(save_text && close_icon);
                if (frame == 0) {
                    cui_focus(ui, 3);
                    assert(cui_queue_event(
                        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
                    assert(cui_queue_event(
                        ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
                }
                if (frame == 1)
                    assert(checked == CUI_CHECK_CHECKED);
            }
            cui_input_metrics input_metrics;
            assert(cui_input_measure((cui_input_options){.design_size = (cui_design_size)scale},
                                     theme.font_size, &input_metrics));
            assert(input_metrics.height == metrics.control_height &&
                   input_metrics.font_size == metrics.font_size &&
                   input_metrics.line_height == metrics.line_height);
            cui_edit_destroy(multiline);
            cui_edit_destroy(edit);
            cui_destroy(ui);
        }
    }
}
static void test_source_separator(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 240});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_color color = {19, 37, 71, 211};
    for (int vertical = 0; vertical < 2; vertical++) {
        for (int dashed = 0; dashed < 2; dashed++) {
            assert(cui_begin_frame(ui, 320, 240, .016f));
            cui_separator_options o = cui_separator_defaults();
            o.length = 17;
            o.vertical = vertical;
            o.dashed = dashed;
            o.custom_color = true;
            o.color = color;
            assert(cui_separator_ex(ui, 1201, &theme, o));
            cui_draw_list list = cui_end_frame(ui);
            cui_rect bounds = cui_bounds(ui, 1201);
            assert(bounds.w == (vertical ? 1 : 17));
            assert(bounds.h == (vertical ? 17 : 1));
            size_t lines = 0;
            for (size_t i = 0; i < list.count; i++) {
                const cui_draw_command *c = list.commands + i;
                if (c->type != CUI_DRAW_RECT || memcmp(&c->color, &color, sizeof(color)))
                    continue;
                assert(c->bounds.w == (vertical ? 1 : dashed ? 4 : 17));
                assert(c->bounds.h == (vertical ? dashed ? 4 : 17 : 1));
                assert(fabsf((vertical ? c->bounds.y - bounds.y : c->bounds.x - bounds.x) -
                             (dashed ? (float)lines * 6 : 0)) < .01f);
                lines++;
            }
            assert(lines == (dashed ? 3u : 1u));
        }
    }
    for (int vertical = 0; vertical < 2; vertical++) {
        for (int empty = 0; empty < 2; empty++) {
            assert(cui_begin_frame(ui, 320, 240, .016f));
            cui_separator_options o = cui_separator_defaults();
            o.length = 160;
            o.vertical = vertical;
            o.has_label = true;
            o.label = empty ? cui_string("") : cui_string("Or");
            assert(cui_separator_ex(ui, 1202, &theme, o));
            cui_draw_list list = cui_end_frame(ui);
            cui_rect bounds = cui_bounds(ui, 1202);
            float label_width = cui_measure_text(ui, o.label, 12).width + 16;
            assert(fabsf(bounds.w - (vertical ? label_width : 160)) < 1);
            assert(bounds.h == (vertical ? 160 : 24));
            bool background = false, text = empty;
            for (size_t i = 0; i < list.count; i++) {
                const cui_draw_command *c = list.commands + i;
                if (c->type == CUI_DRAW_RECT &&
                    !memcmp(&c->color, &theme.background, sizeof(cui_color))) {
                    background = true;
                    assert(fabsf(c->bounds.x + c->bounds.w / 2 - bounds.x - bounds.w / 2) < 1);
                    assert(fabsf(c->bounds.y + c->bounds.h / 2 - bounds.y - bounds.h / 2) < 1);
                    assert(c->bounds.h == 24);
                }
                if (c->type == CUI_DRAW_TEXT && c->text.length) {
                    text = true;
                    assert(c->font_size == 12);
                    assert(!memcmp(&c->color, &theme.muted, sizeof(cui_color)));
                    assert(fabsf(c->bounds.x + c->bounds.w / 2 - bounds.x - bounds.w / 2) < 1);
                    assert(fabsf(c->bounds.y + c->bounds.h / 2 - bounds.y - bounds.h / 2) < 1);
                }
            }
            assert(background && text);
        }
    }
    assert(cui_begin_frame(ui, 320, 240, .016f));
    cui_separator_options invalid[] = {
        {.length = -1}, {.length = NAN}, {.length = 4097}, {.has_label = true, .label = {NULL, 1}}};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
        assert(!cui_separator_ex(ui, 1203, &theme, invalid[i]));
    assert(!cui_separator_ex(ui, 0, &theme, cui_separator_defaults()));
    cui_draw_list list = cui_end_frame(ui);
    assert(!list.count && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void additional_design_scale(void) {
    static unsigned char circle_pixels[1056 * 1056 * 4];
    cui_progress_circle_state circle = {circle_pixels, sizeof circle_pixels, 0};
    const float densities[] = {1, .875f, 2};
    for (size_t k = 0; k < 3; k++) {
        cui_theme theme = cui_theme_light();
        theme.font_size = 16 * densities[k];
        for (int n = CUI_DESIGN_XS; n <= CUI_DESIGN_9XL; n++) {
            cui_design_size size = (cui_design_size)n;
            cui_design_metrics expected, private_metrics;
            assert(cui_design_measure(size, densities[k], &expected));
            assert(cui_design_resolve(size, densities[k], &private_metrics));
            assert(!memcmp(&expected, &private_metrics, sizeof(expected)));
            cui_tag_style style;
            assert(cui_tag_resolve(&theme, (cui_tag_options){.design_size = size}, false, &style));
            assert(style.font_size == expected.font_size);
            assert(style.padding_y == expected.padding_y);
            cui_tag_style before = style;
            assert(!cui_tag_resolve(&theme, (cui_tag_options){.design_size = (cui_design_size)99},
                                    false, &style));
            assert(!memcmp(&before, &style, sizeof(style)));
            cui_attachment_model model = cui_attachment_model_default();
            model.design_size = size;
            model.root_font_size = theme.font_size;
            cui_attachment_plan plan;
            assert(cui_base_attachment(model, &plan));
            assert(plan.metrics.text == expected.font_size);
            assert(plan.metrics.height == expected.control_height);
            assert(plan.metrics.glyph == expected.indicator_size);
            cui_attachment_plan original = plan;
            model.design_size = (cui_design_size)99;
            assert(!cui_base_attachment(model, &plan));
            assert(!memcmp(&original, &plan, sizeof(plan)));
            cui_context *ui = cui_create((cui_config){.width = 1800, .height = 1800});
            assert(ui);
            unsigned badge_calls = 0;
            for (int frame = 0; frame < 2; frame++) {
                assert(cui_begin_frame(ui, 1800, 1800, .016f));
                cui_begin_box(ui, 800, (cui_box){.width = 1800, .height = 1800});
                cui_progress_ex(ui, 801, &theme, .5f, 1000,
                                (cui_progress_options){.design_size = size});
                cui_progress_circle(
                    ui, 803, &theme, .5f, &circle,
                    (cui_progress_circle_options){.progress = {.design_size = size}}, NULL, NULL);
                assert(cui_badge_ex(
                    ui, 804, &theme,
                    (cui_badge_options){.design_size = size, .count = 7, .maximum = 99},
                    badge_content, &badge_calls));
                assert(cui_tag(ui, 802, &theme, cui_string("Sized tag"),
                               (cui_tag_options){.design_size = size}));
                cui_end_box(ui);
                cui_draw_list draw = cui_end_frame(ui);
                assert(cui_last_error(ui) == CUI_OK);
                assert(cui_bounds(ui, 801).h == expected.control_height * .25f);
                assert(cui_bounds(ui, 803).h == expected.control_height);
                cui_rect badge = cui_bounds(ui, 804 ^ UINT64_C(0x2780a7c4f39251db));
                assert(badge.h == expected.line_height * .75f + expected.padding_y);
                bool text = false;
                for (size_t j = 0; j < draw.count; j++)
                    if (draw.commands[j].type == CUI_DRAW_TEXT &&
                        draw.commands[j].text.length == 9 &&
                        !memcmp(draw.commands[j].text.data, "Sized tag", 9)) {
                        text = true;
                        assert(draw.commands[j].font_size == expected.font_size);
                    }
                assert(text);
            }
            assert(badge_calls == 2);
            assert(cui_begin_frame(ui, 1800, 1800, .016f));
            assert(!cui_badge_ex(ui, 804, &theme,
                                 (cui_badge_options){.design_size = (cui_design_size)99},
                                 badge_content, &badge_calls));
            assert(badge_calls == 2);
            cui_progress_ex(ui, 900, &theme, .5f, 1000,
                            (cui_progress_options){.design_size = (cui_design_size)99});
            assert(!cui_tag(ui, 901, &theme, cui_string("Invalid"),
                            (cui_tag_options){.design_size = (cui_design_size)99}));
            cui_end_frame(ui);
            size_t count;
            cui_semantics(ui, &count);
            assert(count == 0 && cui_last_error(ui) == CUI_OK);
            cui_destroy(ui);
        }
    }
    cui_design_metrics sentinel = {1, 2, 3, 4, 5, 6, 7}, result = sentinel;
    assert(!cui_design_resolve(CUI_DESIGN_INHERIT, 1, &result));
    assert(!cui_design_resolve((cui_design_size)99, 1, &result));
    assert(!cui_design_resolve(CUI_DESIGN_XL, NAN, &result));
    assert(!memcmp(&result, &sentinel, sizeof(result)));
}

int main(void) {
    additional_design_scale();
    test_source_separator();
    design_scale();
    test_badge_variants();
    test_progress_source_style();
    test_progress_circle();
    test_slider_semantics();
    test_progress_motion();
    test_button_hover_and_disabled_paint();
    test_button_paint_composition();
    test_button_loading_artwork();
    test_theme_list_tokens();
    test_toggle();
    test_toggle_controlled_sibling_state();
    test_switch();
    test_checkbox_geometry();
    test_radio_geometry();
    test_radio_shared_redraw();
    test_semantic_theme_contrast();
    test_button_intent_outlines();
    test_button_palette_overrides();
    test_icon_button();
    test_named_themes();
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
    assert(count == 4);
    assert(s[3].role == CUI_ROLE_PROGRESSBAR && s[3].value == .5f);
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
