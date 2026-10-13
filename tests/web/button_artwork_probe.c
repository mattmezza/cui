/* SPDX-License-Identifier: MIT */
#define probe_frame inherited_probe_frame
#include "button_blocker_probe.c"
#undef probe_frame
static cui_icon_state artwork_icon;
static bool artwork_initialized;
static unsigned artwork_mode;
static unsigned char progress_pixels[32 * 32 * 4];
static cui_progress_circle_state artwork_progress;
static cui_button_artwork_result artwork_result;
void artwork_config(unsigned value) {
    artwork_mode = value;
}
int probe_frame(void) {
    if (!artwork_initialized) {
        cui_icon_state_init(&artwork_icon);
        if (!cui_icon_set_data(
                &artwork_icon,
                cui_string("<svg viewBox='0 0 16 16'><path d='M2 2h12v12H2z'/></svg>")))
            return 1;
        artwork_progress = (cui_progress_circle_state){.pixels = progress_pixels,
                                                       .capacity = sizeof(progress_pixels)};
        artwork_initialized = true;
    }
    cui_theme t = cui_theme_light();
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(ui, 1,
                  (cui_box){.width = 360,
                            .height = 300,
                            .padding = 10,
                            .background = {245, 245, 245, 255},
                            .layout_only = artwork_mode == 6,
                            .inert = artwork_mode == 7});
    cui_button_artwork icon = {.kind = CUI_BUTTON_ARTWORK_ICON,
                               .icon = &artwork_icon,
                               .icon_options = {.custom_color = true, .color = {231, 24, 91, 255}},
                               .result = &artwork_result};
    cui_button_artwork progress = {
        .kind = CUI_BUTTON_ARTWORK_PROGRESS_CIRCLE,
        .progress = &artwork_progress,
        .fraction = .75f,
        .result = &artwork_result,
        .progress_options = {.raster_scale = 1,
                             .progress = {.custom_color = true, .color = {10, 180, 30, 255}}}};
    if (artwork_mode == 4)
        icon.kind = CUI_BUTTON_ARTWORK_SPINNER;
    artwork_progress.capacity = artwork_mode == 5 ? 1 : sizeof(progress_pixels);
    (void)cui_button_ex(
        ui, 20, &t, cui_string("Artwork"),
        (cui_button_options){
            .leading_artwork = artwork_mode == 3 || artwork_mode == 5 ? &progress : &icon,
            .loading_artwork = artwork_mode == 2 || artwork_mode == 3 ? &icon : NULL,
            .loading = artwork_mode > 0 && artwork_mode < 5,
            .icon_size = 24,
            .on_click = child_click,
            .action_user = &children});
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    return 0;
}
unsigned artwork_stat(unsigned field) {
    return field == 0   ? artwork_result.status
           : field == 1 ? artwork_icon.raster_count
           : field == 2 ? (unsigned)artwork_progress.revision
                        : cui_needs_redraw(ui);
}
