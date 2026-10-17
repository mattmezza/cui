/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #x);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static cui_context *ui;
static cui_theme theme;
static cui_check_state value;
static cui_checkbox_options options;
static bool accept, base_mode, inert, layout_only, skip_middle = true;
static unsigned calls, parent_clicks, slot_calls, style_order;
static cui_activation_snapshot copied;
static cui_check_state requested;
static void request(void *user, cui_check_state next, const cui_activation_snapshot *event) {
    CHECK(user == &value && event);
    ++calls;
    requested = next;
    copied = *event;
    if (accept)
        value = next;
}
static cui_draw_list frame(void) {
    CHECK(cui_begin_frame(ui, 500, 400, .016f));
    cui_begin_box(
        ui, 900,
        (cui_box){.width = 300, .height = 160, .inert = inert, .layout_only = layout_only});
    parent_clicks += cui_base_button(ui, 900, cui_string("Parent"), false).activated;
    if (base_mode) {
        cui_begin_box(ui, 11, (cui_box){.width = 150, .height = 40});
        cui_base_check_ex(ui, 11, cui_string("Base"), &value,
                          (cui_check_options){.disabled = options.disabled,
                                              .on_change = request,
                                              .change_user = &value});
        cui_end_box(ui);
    } else {
        cui_checkbox_source(ui, 11, &theme, cui_string("Visible label"), &value, options);
    }
    cui_end_box(ui);
    cui_button(ui, 99, &theme, cui_string("Focus anchor"), false);
    return cui_end_frame(ui);
}
static void pointer(cui_event_type type, cui_pointer_type pointer_type, unsigned modifiers) {
    cui_rect b = cui_bounds(ui, 11);
    CHECK(b.w > 0 && b.h > 0);
    CHECK(cui_queue_event(ui, (cui_event){.type = type,
                                          .x = b.x + b.w / 2,
                                          .y = b.y + b.h / 2,
                                          .pointer_type = pointer_type,
                                          .pointer_id = 7,
                                          .modifiers = modifiers}));
}
static void click(cui_pointer_type type) {
    pointer(CUI_EVENT_POINTER_DOWN, type, CUI_MOD_SHIFT);
    pointer(CUI_EVENT_POINTER_UP, type, CUI_MOD_SHIFT);
    frame();
    frame();
}
static void key(cui_key k) {
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = k}));
    frame();
    frame();
}
static const cui_semantic *semantic(cui_id id) {
    size_t n;
    const cui_semantic *items = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; ++i)
        if (items[i].id == id)
            return &items[i];
    CHECK(false);
    return NULL;
}
static void instance(cui_context *context, void *user, cui_check_state state, bool disabled,
                     cui_checkbox_paint *paint) {
    (void)context;
    (void)user;
    (void)state;
    (void)disabled;
    CHECK(style_order == 0);
    style_order = 1;
    paint->root.width = 170;
    paint->root.fixed_width = true;
    paint->indicator.width = 24;
    paint->indicator.height = 20;
    paint->label_font_size = 19;
    paint->label_line_height = 25;
    paint->indicator.background = (cui_color){1, 2, 3, 255};
}
static void checked(cui_context *context, void *user, cui_check_state state, bool disabled,
                    cui_checkbox_paint *paint) {
    (void)context;
    (void)user;
    (void)disabled;
    CHECK(state == CUI_CHECK_CHECKED && style_order == 1);
    style_order = 2;
    paint->indicator.background = (cui_color){4, 5, 6, 255};
}
static void mixed(cui_context *context, void *user, cui_check_state state, bool disabled,
                  cui_checkbox_paint *paint) {
    (void)context;
    (void)user;
    (void)disabled;
    CHECK(state == CUI_CHECK_MIXED && style_order == 1);
    style_order = 2;
    paint->indicator.background = (cui_color){4, 5, 6, 255};
}
static void disabled_style(cui_context *context, void *user, cui_check_state state, bool disabled,
                           cui_checkbox_paint *paint) {
    (void)context;
    (void)user;
    (void)state;
    CHECK(disabled && style_order == 2);
    style_order = 3;
    paint->indicator.background = (cui_color){7, 8, 9, 255};
}
static void indicator(cui_context *context, void *user, cui_checkbox_indicator_info info) {
    CHECK(user == &slot_calls && info.value == CUI_CHECK_CHECKED && info.disabled);
    CHECK(info.width == 24 && info.height == 20 && info.checked_progress == 1);
    ++slot_calls;
    cui_begin_box(context, 77,
                  (cui_box){.width = 5, .height = 7, .background = {200, 10, 20, 255}});
    cui_end_box(context);
}
static void box_instance(void *user, cui_box *box) {
    CHECK(user == &style_order);
    style_order = 1;
    box->width = 30;
}
static void box_checked(void *user, cui_box *box) {
    CHECK(user == &style_order && style_order == 1);
    style_order = 2;
    box->height = 20;
}
static void box_disabled(void *user, cui_box *box) {
    CHECK(user == &style_order && style_order == 2);
    style_order = 3;
    box->background = (cui_color){13, 14, 15, 255};
}
static void ordering(void) {
    CHECK(cui_begin_frame(ui, 500, 400, .016f));
    for (unsigned i = 0; i < 3; ++i) {
        cui_id id = 20 + i;
        cui_begin_box(ui, id, (cui_box){.width = 100, .height = 30});
        cui_base_check_ex(ui, id, cui_string("Ordered"), &value,
                          (cui_check_options){.tab_index = i == 0   ? 2
                                                           : i == 1 ? -1
                                                                    : 1,
                                              .skip_tab = i == 1 && skip_middle});
        cui_end_box(ui);
    }
    cui_end_frame(ui);
}
int main(void) {
    ui = cui_create((cui_config){.width = 500, .height = 400});
    CHECK(ui);
    theme = cui_theme_light();
    options = (cui_checkbox_options){.width = 180,
                                     .on_change = request,
                                     .change_user = &value,
                                     .required = true,
                                     .invalid = true,
                                     .accessibility_label = {"Accessible name", 15}};
    value = CUI_CHECK_MIXED;
    frame();
    frame();
    cui_focus(ui, 99);
    frame();
    click(CUI_POINTER_MOUSE);
    CHECK(calls == 1 && requested == CUI_CHECK_CHECKED && value == CUI_CHECK_MIXED);
    CHECK(cui_focused(ui) == 99 && copied.modifiers == CUI_MOD_SHIFT && copied.has_release);
    const cui_semantic *s = semantic(11);
    CHECK(s->role == CUI_ROLE_CHECKBOX && s->value == CUI_CHECK_MIXED && !s->checked);
    CHECK(s->details.required && s->details.invalid && s->name.length == 15);
    CHECK(!memcmp(s->name.data, "Accessible name", 15));
    accept = true;
    click(CUI_POINTER_TOUCH);
    CHECK(calls == 2 && value == CUI_CHECK_CHECKED && cui_focused(ui) == 99);
    cui_focus(ui, 11);
    frame();
    key(CUI_KEY_ENTER);
    CHECK(calls == 3 && value == CUI_CHECK_UNCHECKED);
    key(CUI_KEY_SPACE);
    CHECK(calls == 4 && value == CUI_CHECK_CHECKED);
    accept = false;
    for (unsigned i = 0; i < 2; ++i) {
        CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
    }
    frame();
    frame();
    CHECK(calls == 6 && value == CUI_CHECK_CHECKED && requested == CUI_CHECK_UNCHECKED);
    accept = true;
    unsigned guard_calls = calls;
    pointer(CUI_EVENT_POINTER_DOWN, CUI_POINTER_TOUCH, 0);
    frame();
    CHECK(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_CANCEL, .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 7}));
    frame();
    pointer(CUI_EVENT_POINTER_UP, CUI_POINTER_TOUCH, 0);
    frame();
    CHECK(calls == guard_calls);
    pointer(CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 0);
    pointer(CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 0);
    inert = true;
    frame();
    CHECK(calls == guard_calls);
    inert = false;
    frame();
    frame();
    pointer(CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 0);
    pointer(CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 0);
    layout_only = true;
    frame();
    CHECK(calls == guard_calls);
    layout_only = false;
    frame();
    frame();
    options.disabled = true;
    frame();
    frame();
    unsigned before = calls, parents = parent_clicks;
    click(CUI_POINTER_MOUSE);
    CHECK(calls == before && parent_clicks == parents + 1 && cui_focused(ui) != 11);
    key(CUI_KEY_SPACE);
    CHECK(calls == before);
    options.disabled = false;
    options.custom_role = true;
    options.role = CUI_ROLE_SWITCH;
    frame();
    CHECK(semantic(11)->role == CUI_ROLE_SWITCH);
    options.custom_role = false;
    base_mode = true;
    frame();
    frame();
    click(CUI_POINTER_MOUSE);
    CHECK(cui_focused(ui) == 11); /* Base focuses; source Styled preserves focus. */
    base_mode = false;
    value = CUI_CHECK_CHECKED;
    options.disabled = true;
    options.styles = (cui_checkbox_styles){instance, checked, NULL, disabled_style, NULL};
    options.indicator = indicator;
    options.indicator_user = &slot_calls;
    style_order = 0;
    cui_draw_list drawing = frame();
    CHECK(style_order == 3 && slot_calls == 1 && cui_bounds(ui, 11).w == 170);
    CHECK(cui_bounds(ui, 77).w == 5 && cui_bounds(ui, 77).h == 7);
    bool color = false, text = false;
    for (size_t i = 0; i < drawing.count; ++i) {
        if (drawing.commands[i].type == CUI_DRAW_TEXT && drawing.commands[i].font_size == 19)
            text = true;
        if (drawing.commands[i].type == CUI_DRAW_RECT && drawing.commands[i].color.r == 7 &&
            drawing.commands[i].bounds.w == 24 && drawing.commands[i].bounds.h == 20)
            color = true;
    }
    CHECK(color && text);
    value = CUI_CHECK_MIXED;
    options.styles.mixed = mixed;
    options.indicator = NULL;
    style_order = 0;
    frame();
    CHECK(style_order == 3);
    CHECK(cui_begin_frame(ui, 500, 400, .016f));
    style_order = 0;
    CHECK(cui_base_check_indicator(
        ui, 88, CUI_CHECK_CHECKED, true, (cui_box){0},
        (cui_check_box_styles){box_instance, box_checked, NULL, box_disabled, &style_order}, NULL,
        NULL));
    cui_end_frame(ui);
    CHECK(style_order == 3 && cui_bounds(ui, 88).w == 30 && cui_bounds(ui, 88).h == 20);
    options = (cui_checkbox_options){.width = 120};
    for (int scale = 0; scale < 2; ++scale) {
        theme.font_size = scale ? 24 : 16;
        for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; ++size) {
            options.design_size = (cui_design_size)size;
            value = CUI_CHECK_MIXED;
            frame();
            CHECK(cui_bounds(ui, 11).w >= 120 && semantic(11)->value == CUI_CHECK_MIXED);
        }
    }
    unsigned readonly_calls = calls;
    click(CUI_POINTER_MOUSE);
    CHECK(value == CUI_CHECK_MIXED &&
          calls == readonly_calls); /* No handler: source is controlled. */
    ordering();
    ordering();
    cui_focus(ui, 21);
    ordering();
    CHECK(cui_focused(ui) == 21);
    cui_focus(ui, 0);
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    ordering();
    CHECK(cui_focused(ui) == 22);
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    ordering();
    CHECK(cui_focused(ui) == 20);
    skip_middle = false;
    ordering();
    ordering();
    cui_focus(ui, 0);
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    ordering();
    CHECK(cui_focused(ui) == 21); /* Explicit negative signed order is included. */
    CHECK(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    return 0;
}
