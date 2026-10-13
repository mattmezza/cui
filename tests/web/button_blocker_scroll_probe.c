/* SPDX-License-Identifier: MIT */
/* Reuse the independent host fixture's bounded exports and draw-list access. */
#define probe_frame nested_probe_frame
#include "button_blocker_probe.c"
#undef probe_frame
static bool scroll_scope_hit, overlay_scope_hit;
int probe_scope(unsigned field) {
    return field ? overlay_scope_hit : scroll_scope_hit;
}
int probe_frame(void) {
    cui_theme theme = cui_theme_light();
    cui_scroll_options options = {.width = 300,
                                  .height = 240,
                                  .content_height = 700,
                                  .vertical = true,
                                  .hide_scrollbars = true};
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(ui, 1, (cui_box){.width = 360, .height = 300, .padding = 10});
    cui_begin_focus_scope(ui, 100, false);
    if (!cui_scrollable_begin(ui, 700, &theme, &scroll, options))
        return 2;
    cui_begin_box(ui, 701,
                  (cui_box){.width = 280, .height = 700, .background = {200, 220, 240, 255}});
    cui_text(ui, cui_string("Underlying scroll content"), 14, theme.text);
    cui_end_box(ui);
    cui_scrollable_end_ex(ui, 700, &theme, &scroll, options);
    cui_end_focus_scope(ui);
    cui_begin_focus_scope(ui, 200, false);
    cui_begin_absolute_box(ui, 21, (cui_box){0}, (cui_rect){20, 40, 120, 32});
    cui_button_ex(ui, 20, &theme, cui_string("Overlay"),
                  (cui_button_options){.width = 120,
                                       .height = 32,
                                       .disabled = mode == 1,
                                       .loading = mode == 2,
                                       .on_click = child_click,
                                       .action_user = &children});
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    scroll_scope_hit = cui_event_hit_within_scope(ui, 0, 100);
    overlay_scope_hit = cui_event_hit_within_scope(ui, 0, 200);
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
