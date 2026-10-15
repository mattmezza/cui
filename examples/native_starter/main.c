/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_sdl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void changed(void *user, starter_event event, const starter_app *app) {
    (void)user;
    printf("event=%d page=%d signed_in=%d saves=%u name=%s notifications=%d\n", event, app->page,
           app->signed_in, app->saves, app->saved_name, app->notifications);
    fflush(stdout);
}
int main(int argc, char **argv) {
    const char *font_path = NULL, *capture = NULL;
    unsigned limit = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--font") && i + 1 < argc)
            font_path = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            limit = (unsigned)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
            capture = argv[++i];
        else {
            fprintf(stderr, "Usage: native_starter --font PATH [--frames N --capture BMP]\n");
            return 1;
        }
    }
    if (!font_path || (capture && !limit)) {
        fprintf(stderr, "Supply --font; capture requires --frames.\n");
        return 1;
    }
    cui_font *font = cui_font_open(font_path);
    if (!font) {
        fprintf(stderr, "Cannot open font: %s\n", font_path);
        return 2;
    }
    cui_sdl3 *host = cui_sdl3_create("CUI native starter", 960, 640, font);
    if (!host) {
        fprintf(stderr, "SDL3: %s\n", cui_sdl3_error());
        cui_font_close(font);
        return 3;
    }
    cui_context *ui = cui_create((cui_config){.width = 960,
                                              .height = 640,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .text_geometry = cui_font_text_geometry,
                                              .text_geometry_line = cui_font_text_geometry_line,
                                              .font_metrics = cui_font_get_metrics,
                                              .measure_user = font});
    starter_app app;
    bool ready = starter_app_init(&app, changed, NULL);
    int result = 0;
    if (!ui || !ready)
        result = 4;
    else {
        cui_platform clipboard = cui_sdl3_clipboard_interface(host);
        unsigned frame = 0;
        while ((!limit || frame < limit) && cui_sdl3_poll(host, ui)) {
            if (frame && !limit && !cui_needs_redraw(ui)) {
                (void)cui_sdl3_wait_events(host, 250);
                continue;
            }
            float width, height;
            cui_sdl3_size(host, &width, &height);
            cui_draw_list list = starter_app_frame(
                ui, &app, width, height, cui_sdl3_frame_delta(host), &clipboard);
            /* poll synchronizes IME/caret state; apply this frame's cursor immediately. */
            (void)cui_sdl3_set_cursor(host, cui_pointer_cursor(ui));
            bool painted =
                capture && frame + 1 == limit
                    ? cui_sdl3_render_capture(host, &list, (cui_color){245, 245, 245, 255}, capture)
                    : cui_sdl3_render(host, &list, (cui_color){245, 245, 245, 255});
            if (!painted || cui_last_error(ui) != CUI_OK) {
                result = 5;
                break;
            }
            frame++;
        }
    }
    starter_app_destroy(&app);
    cui_destroy(ui);
    cui_sdl3_destroy(host);
    cui_font_close(font);
    return result;
}
