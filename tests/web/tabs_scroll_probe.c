/* SPDX-License-Identifier: MIT */
#include "../../web/touch_page_host.h"
#include "cui/cui_choice.h"
#include "cui/cui_scroll.h"
static cui_context *ui;
static cui_tab_scroll_state tabs;
static cui_scroll_state parent;
static cui_touch_page_host host;
static cui_draw_list draw;
static cui_id selected = 10;
static bool vertical, disabled;
static unsigned consumed, selections;
int probe_init(int axis, int off) {
    if (ui)
        cui_destroy(ui);
    ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    tabs = (cui_tab_scroll_state){0};
    parent = (cui_scroll_state){0};
    host = (cui_touch_page_host){0};
    selected = 10;
    selections = 0;
    vertical = axis != 0;
    disabled = off != 0;
    return ui ? 0 : 1;
}
int probe_event(int type, float x, float y, float dx, float dy, unsigned id, int touch) {
    return cui_queue_event(
        ui, (cui_event){.type = (cui_event_type)type,
                        .x = x,
                        .y = y,
                        .dx = dx,
                        .dy = dy,
                        .pointer_id = id,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                        .scroll_precise = true});
}
int probe_frame(void) {
    cui_theme theme = cui_theme_light();
    cui_choice_item items[] = {
        {10, {"First long tab", 14}, false}, {20, {"Second long tab", 15}, false},
        {30, {"Third long tab", 14}, false}, {40, {"Fourth long tab", 15}, false},
        {50, {"Fifth long tab", 14}, false}, {60, {"Sixth long tab", 14}, false}};
    cui_scroll_options outer = {.width = 320,
                                .height = 240,
                                .content_width = 800,
                                .content_height = 800,
                                .horizontal = !vertical,
                                .vertical = vertical,
                                .hide_scrollbars = true};
    if (!cui_begin_frame(ui, 360, 260, .016f))
        return 1;
    cui_begin_box(
        ui, 900,
        (cui_box){.width = 360, .height = 260, .background = {245, 245, 245, 255}, .padding = 10});
    if (!cui_scrollable_begin(ui, 700, &theme, &parent, outer))
        return 2;
    selections += cui_tabs(ui, 1, &theme, items, 6, &selected,
                           (cui_tabs_options){.vertical = vertical,
                                              .disabled = disabled,
                                              .max_width = 150,
                                              .scroll = &tabs,
                                              .bar_extent = 110},
                           NULL, NULL);
    cui_begin_box(ui, 701,
                  (cui_box){.width = 600, .height = 600, .background = {190, 220, 240, 255}});
    cui_end_box(ui);
    cui_scrollable_end_ex(ui, 700, &theme, &parent, outer);
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
    cui_rect bounds = cui_bounds(ui, tabs.receiving_id);
    switch (field) {
    case 0:
        return tabs.offset;
    case 1:
        return tabs.maximum;
    case 2:
        return vertical ? parent.y : parent.x;
    case 3:
        return consumed;
    case 4:
        return selected;
    case 5:
        return bounds.x;
    case 6:
        return bounds.y;
    case 7:
        return bounds.w;
    case 8:
        return bounds.h;
    case 9:
        return cui_bounds(ui, 10).x;
    case 10:
        return cui_bounds(ui, 10).y;
    case 11:
        return selections;
    case 12:
        return cui_bounds(ui, 20).x + cui_bounds(ui, 20).w / 2;
    case 13:
        return cui_bounds(ui, 20).y + cui_bounds(ui, 20).h / 2;
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
