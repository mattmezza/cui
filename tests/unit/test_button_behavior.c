/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_context *ui;
static cui_theme theme;
static cui_button_options options;
static cui_button_state state;
static cui_str name;
static unsigned clicks, enters, leaves, results;
static void click(void *user) {
    assert(user == &clicks);
    clicks++;
}
static void hover(void *user, bool entered) {
    assert(user == &clicks);
    if (entered)
        enters++;
    else
        leaves++;
}
static void slot(cui_context *context, void *user) {
    float size = *(float *)user;
    cui_begin_box(context, 0,
                  (cui_box){.width = size, .height = size, .background = {212, 19, 83, 255}});
    cui_end_box(context);
}
static cui_draw_list frame(void) {
    assert(cui_begin_frame(ui, 6000, 6000, .016f));
    cui_begin_box(ui, 10, (cui_box){.width = 180, .height = 24});
    cui_interact(ui, 10, CUI_ROLE_TEXTBOX, cui_string("Editor"), false, false, 0);
    cui_end_box(ui);
    results += cui_button_ex(ui, 20, &theme, name, options);
    return cui_end_frame(ui);
}
static const cui_semantic *button(void) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        if (s[i].id == 20)
            return &s[i];
    assert(false);
    return NULL;
}
static void key(cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = key}));
}
static void pointer(cui_event_type kind, bool touch, uint64_t contact) {
    cui_rect b = cui_bounds(ui, 20);
    assert(cui_queue_event(
        ui, (cui_event){.type = kind,
                        .x = b.x + 5,
                        .y = b.y + 5,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                        .pointer_id = contact}));
}
static bool color(cui_draw_list list, cui_draw_type type, cui_color expected) {
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == type &&
            !memcmp(&list.commands[i].color, &expected, sizeof expected))
            return true;
    return false;
}
int main(void) {
    ui = cui_create((cui_config){.width = 6000, .height = 6000});
    assert(ui);
    theme = cui_theme_light();
    name = cui_string("Action");
    options = (cui_button_options){.width = 180,
                                   .state = &state,
                                   .on_click = click,
                                   .on_hover = hover,
                                   .action_user = &clicks,
                                   .prevent_pointer_focus = true,
                                   .custom_cursor = true,
                                   .cursor = CUI_CURSOR_CROSSHAIR};
    frame();
    cui_focus(ui, 10);
    pointer(CUI_EVENT_POINTER_MOVE, false, 0);
    frame();
    assert(enters == 1 && leaves == 0);
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_CROSSHAIR);
    pointer(CUI_EVENT_POINTER_DOWN, false, 0);
    frame();
    assert(cui_pressed(ui, 20) && cui_focused(ui) == 10);
    pointer(CUI_EVENT_POINTER_UP, false, 0);
    frame();
    assert(clicks == 1 && results == 1 && cui_focused(ui) == 10);
    frame();
    assert(clicks == 1 && enters == 1);
    pointer(CUI_EVENT_POINTER_DOWN, true, 71);
    frame();
    pointer(CUI_EVENT_CANCEL, true, 71);
    frame();
    pointer(CUI_EVENT_POINTER_UP, true, 71);
    frame();
    assert(clicks == 1);
    pointer(CUI_EVENT_POINTER_DOWN, true, 72);
    frame();
    pointer(CUI_EVENT_POINTER_UP, true, 72);
    frame();
    assert(clicks == 2 && cui_focused(ui) == 10);
    key(CUI_KEY_TAB);
    frame();
    assert(cui_focused(ui) == 20 && cui_interaction_state(ui, 20).focus_visible);
    key(CUI_KEY_ENTER);
    frame();
    key(CUI_KEY_SPACE);
    frame();
    assert(clicks == 4 && results == 4);
    options.skip_tab = true;
    frame();
    cui_focus(ui, 10);
    key(CUI_KEY_TAB);
    frame();
    assert(cui_focused(ui) == 10);
    options.skip_tab = false;
    options.disabled = true;
    frame();
    pointer(CUI_EVENT_POINTER_DOWN, false, 0);
    pointer(CUI_EVENT_POINTER_UP, false, 0);
    key(CUI_KEY_ENTER);
    frame();
    assert(clicks == 4);
    options.disabled = false;
    options.loading = true;
    options.variant = CUI_BUTTON_PRIMARY;
    cui_draw_list draw = frame();
    cui_color faded = theme.buttons[CUI_THEME_BUTTON_PRIMARY].normal;
    faded.a = (uint8_t)(faded.a * .8f);
    assert(button()->disabled && button()->details.busy && color(draw, CUI_DRAW_RECT, faded));
    pointer(CUI_EVENT_POINTER_DOWN, true, 73);
    pointer(CUI_EVENT_POINTER_UP, true, 73);
    frame();
    assert(clicks == 4);
    options.loading_icon = cui_string("loader");
    draw = frame();
    unsigned loading_images = 0;
    for (size_t i = 0; i < draw.count; i++)
        loading_images += draw.commands[i].type == CUI_DRAW_IMAGE;
    assert(loading_images == 1 && cui_needs_redraw(ui));
    cui_set_reduced_motion(ui, true);
    frame();
    frame();
    assert(!cui_needs_redraw(ui) && button()->details.busy);
    cui_set_reduced_motion(ui, false);
    options.loading_icon = (cui_str){0};
    options.loading = false;
    options.toggle_semantics = true;
    options.toggled = true;
    frame();
    assert(button()->checked && button()->value == 1);
    options.toggle_semantics = false;
    options.selected = true;
    frame();
    assert(!button()->checked && button()->value == 0);
    options.selected = false;
    options.prevent_pointer_focus = false;
    frame();
    cui_focus(ui, 10);
    pointer(CUI_EVENT_POINTER_DOWN, false, 0);
    frame();
    assert(cui_focused(ui) == 20 && !cui_interaction_state(ui, 20).focus_visible);
    pointer(CUI_EVENT_CANCEL, false, 0);
    frame();
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 5900, .y = 5900}));
    frame();
    assert(leaves >= 1);
    options.on_click = NULL;
    options.on_hover = NULL;
    options.custom_cursor = false;
    cui_set_reduced_motion(ui, true);
    for (unsigned density = 0; density < 2; density++)
        for (int design = 0; design < 18; design++)
            for (int variant = CUI_BUTTON_PRIMARY; variant <= CUI_BUTTON_CUSTOM; variant++) {
                theme.font_size = density ? 14 : 16;
                options = (cui_button_options){
                    .state = &state,
                    .variant = (cui_button_variant)variant,
                    .size = design < 4 ? (cui_control_size)design : CUI_CONTROL_MEDIUM,
                    .design_size = design < 4 ? CUI_DESIGN_INHERIT : (cui_design_size)(design - 3),
                    .icon = {"check", 5},
                    .trailing_icon = {"arrow-right", 11}};
                float font = (options.size == CUI_CONTROL_XSMALL  ? 12
                              : options.size == CUI_CONTROL_LARGE ? 16
                                                                  : 14) *
                             theme.font_size / 16;
                cui_design_metrics metrics;
                if (options.design_size) {
                    assert(cui_design_measure(options.design_size, theme.font_size / 16, &metrics));
                    font = metrics.font_size;
                }
                options.width = 16 * font;
                name = cui_string("Run");
                draw = frame();
                cui_rect b = button()->bounds;
                assert(button()->value == 0 && !button()->checked && !button()->disabled);
                unsigned icon_count = 0;
                for (size_t i = 0; i < draw.count; i++)
                    icon_count += draw.commands[i].type == CUI_DRAW_IMAGE;
                assert(icon_count == 2);
                for (size_t i = 0; i < draw.count; i++)
                    if (draw.commands[i].type == CUI_DRAW_TEXT ||
                        draw.commands[i].type == CUI_DRAW_IMAGE) {
                        cui_draw_command d = draw.commands[i];
                        assert(d.bounds.x >= b.x - .1f &&
                               d.bounds.x + d.bounds.w <= b.x + b.w + .1f);
                        assert(d.bounds.y >= b.y - .1f &&
                               d.bounds.y + d.bounds.h <= b.y + b.h + .1f);
                        float natural = d.type == CUI_DRAW_TEXT
                                            ? cui_measure_text(ui, d.text, d.font_size).height
                                            : d.bounds.h;
                        assert(fabsf(d.bounds.y + natural * .5f - (b.y + b.h * .5f)) < .7f);
                    }
            }
    theme = cui_theme_light();
    name = cui_string("A very long label with emoji e\xcc\x81 and "
                      "\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa9 family");
    options = (cui_button_options){
        .state = &state, .width = 180, .label_width = 70, .accessibility_label = name};
    draw = frame();
    assert(button()->name.length == name.length &&
           !memcmp(button()->name.data, name.data, name.length));
    assert(cui_edit_valid_utf8(cui_string(state.label)));
    size_t prefix = strlen(state.label) - 3;
    assert(cui_grapheme_boundary(name, prefix));
    assert(cui_measure_text(ui, cui_string(state.label), 14).width <= 70);
    options.label_width = 1;
    frame();
    assert(!state.label[0] && button()->name.length == name.length);
    char long_name[700];
    memset(long_name, 'a', sizeof(long_name));
    name = (cui_str){long_name, sizeof(long_name)};
    options.accessibility_label = name;
    options.label_width = 4000;
    options.width = 5000;
    options.font_size = 16;
    draw = frame();
    assert(strlen(state.label) <= 255 && button()->name.length == 700 &&
           cui_edit_valid_utf8(cui_string(state.label)));
    float square = 16;
    name = cui_string("Custom slots");
    options = (cui_button_options){.width = 260,
                                   .leading_content = slot,
                                   .leading_user = &square,
                                   .trailing_content = slot,
                                   .trailing_user = &square,
                                   .children = slot,
                                   .children_user = &square,
                                   .dropdown_caret = true,
                                   .height = 40,
                                   .font_size = 16,
                                   .icon_size = 18,
                                   .rounding = CUI_BUTTON_ROUNDED_CUSTOM,
                                   .radius = 9};
    draw = frame();
    assert(button()->bounds.h == 40 && button()->bounds.w == 260);
    unsigned images = 0, slots = 0;
    for (size_t i = 0; i < draw.count; i++) {
        images += draw.commands[i].type == CUI_DRAW_IMAGE;
        slots += draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == 212;
    }
    assert(images == 1 && slots == 3);
    options = (cui_button_options){.variant = CUI_BUTTON_CUSTOM,
                                   .custom_colors = true,
                                   .colors = {.normal = {21, 32, 43, 255},
                                              .hover = {54, 65, 76, 255},
                                              .active = {87, 98, 109, 255},
                                              .foreground = {240, 241, 242, 255}}};
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 5900, .y = 5900}));
    draw = frame();
    assert(color(draw, CUI_DRAW_RECT, options.colors.normal));
    pointer(CUI_EVENT_POINTER_MOVE, false, 0);
    draw = frame();
    assert(color(draw, CUI_DRAW_RECT, options.colors.hover));
    pointer(CUI_EVENT_POINTER_DOWN, false, 0);
    draw = frame();
    assert(color(draw, CUI_DRAW_RECT, options.colors.active));
    pointer(CUI_EVENT_CANCEL, false, 0);
    frame();
    options.selected = true;
    draw = frame();
    assert(color(draw, CUI_DRAW_RECT, options.colors.active));
    assert(!button()->checked && button()->value == 0);
    name = cui_string("I");
    options = (cui_button_options){.compact = true, .height = 60};
    frame();
    assert(button()->bounds.w >= 60 && button()->bounds.h == 60);
    options = (cui_button_options){.icon_only = true, .icon = {"check", 5}, .height = 44};
    frame();
    assert(button()->bounds.w == 44 && button()->bounds.h == 44);
    options = (cui_button_options){
        .state = &state,
        .width = 180,
        .tooltip = {"Action help", 11},
        .tooltip_options = {.viewport = {0, 0, 6000, 6000}, .open_delay = .05f}};
    name = cui_string("Action");
    frame();
    cui_focus(ui, 20);
    for (unsigned i = 0; i < 8; i++)
        frame();
    assert(state.tooltip.open && button()->details.description.length == 11);
    key(CUI_KEY_ESCAPE);
    frame();
    assert(!state.tooltip.open);
    pointer(CUI_EVENT_POINTER_DOWN, true, 99);
    frame();
    pointer(CUI_EVENT_CANCEL, true, 99);
    frame();
    assert(!state.tooltip.open && state.tooltip.suppressed);
    options.disabled = true;
    frame();
    assert(!state.tooltip.pending && state.next_wake_seconds == 0);
    cui_destroy(ui);
    puts("button behavior: ok");
}
