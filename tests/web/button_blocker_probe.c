/* SPDX-License-Identifier: MIT */
#include "../../web/touch_page_host.h"
#include "cui/cui_components.h"
static cui_context *ui;
static cui_scroll_state scroll;
static cui_touch_page_host host;
static cui_draw_list draw;
static unsigned parents, children, mode, consumed;
static void child_click(void *user) {
    ++*(unsigned *)user;
}
int probe_init(unsigned state) {
    if (ui)
        cui_destroy(ui);
    ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    mode = state;
    parents = children = 0;
    scroll = (cui_scroll_state){0};
    host = (cui_touch_page_host){0};
    return ui ? 0 : 1;
}
int probe_event(int type, float x, float y, unsigned id, int touch, float dy) {
    return cui_queue_event(
        ui, (cui_event){.type = (cui_event_type)type,
                        .x = x,
                        .y = y,
                        .dy = dy,
                        .scroll_precise = true,
                        .pointer_id = id,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE});
}
int probe_key(int key, int up, int repeat) {
    return cui_queue_event(ui, (cui_event){.type = up ? CUI_EVENT_KEY_UP : CUI_EVENT_KEY_DOWN,
                                           .key = (cui_key)key,
                                           .repeat = repeat != 0});
}
int probe_frame(void) {
    cui_theme theme = cui_theme_light();
    cui_scroll_options outer = {.width = 300,
                                .height = 240,
                                .content_height = 700,
                                .vertical = true,
                                .hide_scrollbars = true};
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 360, .height = 300, .padding = 10, .background = {245, 245, 245, 255}});
    cui_begin_box(ui, 90,
                  (cui_box){.width = 100, .height = 24, .background = {220, 230, 240, 255}});
    cui_interact(ui, 90, CUI_ROLE_TEXTBOX, cui_string("Editor"), false, false, 0);
    cui_text(ui, cui_string("Editor"), 14, theme.text);
    cui_end_box(ui);
    if (!cui_scrollable_begin(ui, 700, &theme, &scroll, outer))
        return 2;
    cui_begin_box(
        ui, 10,
        (cui_box){.width = 260, .height = 130, .padding = 10, .background = {200, 220, 240, 255}});
    parents += cui_base_button(ui, 10, cui_string("Parent"), false).activated;
    cui_button_ex(ui, 20, &theme, cui_string("Child"),
                  (cui_button_options){.width = 120,
                                       .disabled = mode == 1,
                                       .loading = mode == 2,
                                       .on_click = child_click,
                                       .action_user = &children});
    cui_end_box(ui);
    cui_begin_box(ui, 701,
                  (cui_box){.width = 260, .height = 500, .background = {230, 235, 240, 255}});
    cui_end_box(ui);
    cui_scrollable_end_ex(ui, 700, &theme, &scroll, outer);
    cui_end_box(ui);
    cui_touch_page_dispatch(&host, ui);
    size_t count;
    cui_frame_events(ui, &count);
    consumed = 0;
    for (size_t i = 0; i < count; i++)
        consumed += cui_event_consumed(ui, i);
    draw = cui_end_frame(ui);
    return cui_last_error(ui);
}
int probe_page_available(unsigned id) {
    return cui_touch_page_available(&host, id);
}
void probe_page_end(unsigned id) {
    cui_touch_page_end(&host, id);
}
double probe_stat(unsigned field) {
    switch (field) {
    case 0:
        return parents;
    case 1:
        return children;
    case 2:
        return (double)cui_focused(ui);
    case 3:
        return scroll.y;
    case 4:
        return consumed;
    case 5:
        return cui_pressed(ui, 10);
    default:
        return -1;
    }
}
double probe_rect(unsigned id, unsigned field) {
    cui_rect b = cui_bounds(ui, id);
    switch (field) {
    case 0:
        return b.x;
    case 1:
        return b.y;
    case 2:
        return b.w;
    case 3:
        return b.h;
    default:
        return -1;
    }
}
unsigned probe_count(void) {
    return (unsigned)draw.count;
}
double probe_command(unsigned i, unsigned field) {
    if (i >= draw.count)
        return -1;
    const cui_draw_command *c = &draw.commands[i];
    switch (field) {
    case 0:
        return c->type;
    case 1:
        return c->bounds.x;
    case 2:
        return c->bounds.y;
    case 3:
        return c->bounds.w;
    case 4:
        return c->bounds.h;
    case 5:
        return c->color.r;
    case 6:
        return c->color.g;
    case 7:
        return c->color.b;
    case 8:
        return c->color.a;
    case 9:
        return c->font_size;
    default:
        return -1;
    }
}
const char *probe_text(unsigned i) {
    return i < draw.count ? draw.commands[i].text.data : 0;
}
unsigned probe_text_length(unsigned i) {
    return i < draw.count ? (unsigned)draw.commands[i].text.length : 0;
}
double probe_bitmap(unsigned i, unsigned field) {
    if (i >= draw.count || draw.commands[i].type != CUI_DRAW_IMAGE)
        return 0;
    cui_bitmap_paint b = draw.commands[i].bitmap;
    cui_color tint = b.custom_tint ? b.tint : (cui_color){255, 255, 255, 255};
    switch (field) {
    case 0:
        return (double)(uintptr_t)b.bitmap.pixels;
    case 1:
        return b.bitmap.width;
    case 2:
        return b.bitmap.height;
    case 3:
        return b.bitmap.stride;
    case 4:
        return tint.r;
    case 5:
        return tint.g;
    case 6:
        return tint.b;
    case 7:
        return tint.a;
    case 8:
        return b.opacity;
    default:
        return 0;
    }
}
