/* SPDX-License-Identifier: MIT */
#define probe_init inherited_probe_init
#define probe_frame inherited_probe_frame
#include "button_blocker_probe.c"
#undef probe_init
#undef probe_frame
#include "cui/cui_software.h"
#include <string.h>
static cui_button_group_style_workspace workspace;
static cui_software *renderer;
static bool selected[3];
static unsigned requests, order[32], lengths[32], request_indices[32][3];
static cui_button_group_result result;
static cui_button_state states[3];
static void change(void *user, const cui_button_group_request *r) {
    (void)user;
    if (requests < 32) {
        order[requests] = (unsigned)r->item_index;
        lengths[requests] = (unsigned)r->selected_count;
        for (size_t i = 0; i < r->selected_count; i++)
            request_indices[requests][i] = (unsigned)r->selected_indices[i];
    }
    ++requests;
    if (mode != 2) {
        for (size_t i = 0; i < 3; i++)
            selected[i] = false;
        for (size_t i = 0; i < r->selected_count; i++)
            selected[r->selected_indices[i]] = true;
    }
}
int probe_init(unsigned state) {
    if (renderer)
        cui_software_destroy(renderer);
    renderer = cui_software_create(360, 300, NULL);
    memset(&workspace, 0, sizeof workspace);
    memset(states, 0, sizeof states);
    requests = 0;
    selected[0] = selected[2] = false;
    selected[1] = true;
    return inherited_probe_init(state) || !renderer;
}
unsigned probe_pixels(void) {
    return (unsigned)(uintptr_t)cui_software_pixels(renderer, NULL, NULL);
}
int probe_crosshair(void) {
    return CUI_CURSOR_CROSSHAIR;
}
double group_image_rect(unsigned field) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            cui_rect r = draw.commands[i].bounds;
            switch (field) {
            case 0:
                return r.x;
            case 1:
                return r.y;
            case 2:
                return r.w;
            case 3:
                return r.h;
            }
        }
    return -1;
}
double group_stat(unsigned field, unsigned index) {
    switch (field) {
    case 0:
        return requests;
    case 1:
        return selected[0] + selected[1] * 2 + selected[2] * 4;
    case 2:
        return index < 32 ? (double)order[index] : -1;
    case 3:
        return index < 32 ? (double)lengths[index] : -1;
    case 4:
        return index < 3 ? selected[index] : -1;
    case 5:
        return result.accepted_activations;
    case 6:
        return result.callbacks;
    case 7:
        return children;
    case 8:
        return cui_pointer_cursor(ui);
    default:
        return -1;
    }
}
int probe_frame(void) {
    cui_theme theme = cui_theme_light();
    cui_theme_button_colors colors = {.normal = {20, 130, 200, 255},
                                      .hover = {40, 160, 220, 255},
                                      .active = {20, 70, 100, 255},
                                      .border = {10, 40, 60, 255},
                                      .foreground = {255, 255, 255, 255}};
    cui_button_group_styled_item items[3];
    const char *labels[] = {"One", "Two", "Three"};
    for (size_t i = 0; i < 3; i++)
        items[i] = (cui_button_group_styled_item){
            .id = 10 + i,
            .label = cui_string(labels[i]),
            .options = {.width = 100,
                        .variant = CUI_BUTTON_CUSTOM,
                        .custom_colors = true,
                        .colors = colors,
                        .state = &states[i],
                        .disabled = mode == 3 && i == 0,
                        .loading = mode == 4 && i == 0,
                        .icon = i == 0 ? cui_string("check") : (cui_str){0},
                        .on_click = child_click,
                        .action_user = &children,
                        .custom_cursor = i == 0,
                        .cursor = CUI_CURSOR_CROSSHAIR}};
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 360, .height = 300, .padding = 20, .background = {245, 245, 245, 255}});
    cui_button_group_style_options options = {
        .behavior = {.multiple = mode == 1,
                     .vertical = mode == 5,
                     .on_change = mode == 6 ? NULL : change,
                     .activation_policy = mode == 0 ? CUI_BUTTON_GROUP_SOURCE_POINTER_CALLBACKS
                                                    : CUI_BUTTON_GROUP_ALL_ACTIVATIONS}};
    if (!cui_button_group_ex(ui, 100, &theme, items, selected, 3, &workspace, options, &result))
        return 2;
    cui_end_box(ui);
    cui_touch_page_dispatch(&host, ui);
    draw = cui_end_frame(ui);
    cui_draw_command shapes[512];
    size_t count = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            shapes[count++] = draw.commands[i];
    cui_draw_list shape_list = {shapes, count, 360, 300};
    return cui_last_error(ui) ||
           !cui_software_render(renderer, &shape_list, (cui_color){245, 245, 245, 255});
}
