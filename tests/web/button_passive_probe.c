/* SPDX-License-Identifier: MIT */
#define probe_frame inherited_probe_frame
#define probe_init inherited_probe_init
#include "button_blocker_probe.c"
#undef probe_frame
#undef probe_init
#include <string.h>
static cui_button_state state, frozen;
static unsigned hover_count;
static bool loading, named_loading;
static void hovered(void *p, bool value) {
    (void)p;
    (void)value;
    ++hover_count;
}
int probe_init(unsigned initial) {
    int result = inherited_probe_init(0);
    state = (cui_button_state){0};
    frozen = state;
    hover_count = 0;
    loading = named_loading = false;
    mode = initial;
    return result;
}
void passive_scope(unsigned scope) {
    memcpy(&frozen, &state, sizeof frozen);
    mode = scope;
}
void passive_loading(int active, int named) {
    loading = active != 0;
    named_loading = named != 0;
}
int passive_redraw(void) {
    return cui_needs_redraw(ui);
}
int passive_unchanged(void) {
    return !memcmp(&state, &frozen, sizeof state);
}
double passive_stat(unsigned field) {
    switch (field) {
    case 0:
        return hover_count;
    case 1:
        return state.hovered;
    case 2:
        return state.tooltip.open;
    case 3:
        return state.tooltip.pending;
    case 4:
        return state.next_wake_seconds;
    default:
        return -1;
    }
}
int probe_frame(void) {
    cui_theme theme = cui_theme_light();
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 160, .height = 100, .inert = mode == 1, .layout_only = mode == 2});
    cui_button_ex(ui, 20, &theme, cui_string("A long label requiring clipping"),
                  (cui_button_options){
                      .width = -1,
                      .loading = loading,
                      .loading_icon = named_loading ? cui_string("check") : (cui_str){0},
                      .state = &state,
                      .on_hover = hovered,
                      .on_click = child_click,
                      .action_user = &children,
                      .icon = {"check", 5},
                      .tooltip = {"Actual hover tooltip", 20},
                      .tooltip_options = {
                          .viewport = {0, 0, 360, 300}, .open_delay = .03f, .close_delay = .03f}});
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    return cui_last_error(ui);
}
