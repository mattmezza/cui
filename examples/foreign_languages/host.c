/* SPDX-License-Identifier: MIT */
#include "host.h"
#include "cui/cui_components.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef CUI_EXAMPLE_NATIVE
#include "cui/cui_sdl3.h"
#endif
static cui_draw_list frame(cui_context *ui, float w, float h, int *count,
                           cui_example_changed callback, void *user, char label[64]) {
    cui_theme theme = cui_theme_light();
    cui_begin_frame(ui, w, h, 0.016f);
    cui_begin_box(ui, 1,
                  (cui_box){.width = -1,
                            .height = -1,
                            .padding = 24,
                            .gap = 12,
                            .background = {245, 247, 250, 255}});
    cui_text(ui, cui_string("CUI from your language"), 24, (cui_color){20, 30, 40, 255});
    if (cui_button(ui, 2, &theme, cui_string("Increment"), false)) {
        ++*count;
        if (callback)
            callback(user, *count);
    }
    snprintf(label, 64, "Count: %d", *count);
    cui_text(ui, cui_string(label), 20, (cui_color){20, 30, 40, 255});
    cui_end_box(ui);
    return cui_end_frame(ui);
}
int cui_example_run(const char *font_path, unsigned frames, cui_example_changed callback,
                    void *user) {
    cui_config config = {.width = 480, .height = 240};
#ifdef CUI_EXAMPLE_NATIVE
    cui_font *font = NULL;
    cui_sdl3 *backend = NULL;
    if (font_path) {
        font = cui_font_open(font_path);
        if (!font)
            return 2;
        config.measure = cui_font_measure;
        config.measure_user = font;
        backend = cui_sdl3_create("CUI language example", 480, 240, font);
        if (!backend) {
            cui_font_close(font);
            return 3;
        }
    }
#else
    (void)frames;
    if (font_path)
        return 4;
#endif
    cui_context *ui = cui_create(config);
    if (!ui) {
#ifdef CUI_EXAMPLE_NATIVE
        cui_sdl3_destroy(backend);
        cui_font_close(font);
#endif
        return 5;
    }
    int count = 0, result = 0;
    char label[64]; /* Remains alive while the returned draw list is rendered. */
    cui_draw_list list = frame(ui, 480, 240, &count, callback, user, label);
    if (!font_path) {
        cui_rect button = cui_bounds(ui, 2);
        if (list.count < 3 || button.w <= 0)
            result = 6;
        for (int i = 0; i < 2 && !result; ++i) {
            cui_event down = {.type = CUI_EVENT_POINTER_DOWN,
                              .x = button.x + button.w / 2,
                              .y = button.y + button.h / 2};
            cui_event up = down;
            up.type = CUI_EVENT_POINTER_UP;
            if (!cui_queue_event(ui, down) || !cui_queue_event(ui, up))
                result = 7;
            list = frame(ui, 480, 240, &count, callback, user, label);
            if (count != i + 1 || list.count < 3 || cui_last_error(ui) != CUI_OK)
                result = 8;
        }
    }
#ifdef CUI_EXAMPLE_NATIVE
    else {
        unsigned rendered = 0;
        while (cui_sdl3_poll(backend, ui) && (!frames || rendered < frames)) {
            float w, h;
            cui_sdl3_size(backend, &w, &h);
            list = frame(ui, w, h, &count, callback, user, label);
            const char *capture = getenv("CUI_EXAMPLE_CAPTURE");
            bool painted = capture && frames && rendered + 1 == frames
                               ? cui_sdl3_render_capture(backend, &list,
                                                         (cui_color){245, 247, 250, 255}, capture)
                               : cui_sdl3_render(backend, &list, (cui_color){245, 247, 250, 255});
            if (cui_last_error(ui) != CUI_OK || !painted) {
                result = 9;
                break;
            }
            ++rendered;
            cui_sdl3_wait(16);
        }
        cui_sdl3_destroy(backend);
        cui_font_close(font);
    }
#endif
    cui_destroy(ui);
    return result;
}
