/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef APP_NATIVE
#include "cui/cui_sdl3.h"
#endif
static void event(void *user, app_event kind, app_snapshot state) {
    (void)user;
    printf("event=%d page=%d plan=%d session=%d saves=%u name=%s notifications=%d sync=%d\n", kind,
           state.page, state.plan, state.logged_in, state.saves, state.display_name,
           state.notifications, state.sync);
    fflush(stdout);
}
static bool route(cui_context *ui, composed_app *app, const char *page, float w, float h) {
    composed_app_frame(ui, app, w, h);
    cui_id id = !strcmp(page, "pricing") ? APP_NAV_PRICING : APP_NAV_SETTINGS;
    if (!strcmp(page, "overview"))
        return true;
    if (w < 760) {
        cui_rect b = cui_bounds(ui, APP_MENU);
        cui_queue_event(
            ui,
            (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w / 2, .y = b.y + b.h / 2});
        cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + b.w / 2, .y = b.y + b.h / 2});
        composed_app_frame(ui, app, w, h);
    }
    cui_rect b = cui_bounds(ui, id);
    cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w / 2, .y = b.y + b.h / 2});
    cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + b.w / 2, .y = b.y + b.h / 2});
    composed_app_frame(ui, app, w, h);
    return composed_app_snapshot(app).page ==
           (!strcmp(page, "pricing") ? APP_PRICING : APP_SETTINGS);
}
int main(int argc, char **argv) {
    const char *font_path = NULL, *capture = NULL, *page = "overview";
    unsigned frames = 0;
    int width = 1200, height = 900;
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--headless"))
            headless = true;
        else if (!strcmp(argv[i], "--font") && i + 1 < argc)
            font_path = argv[++i];
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
            capture = argv[++i];
        else if (!strcmp(argv[i], "--page") && i + 1 < argc)
            page = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            frames = (unsigned)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--width") && i + 1 < argc)
            width = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--height") && i + 1 < argc)
            height = atoi(argv[++i]);
        else {
            fprintf(
                stderr,
                "Usage: northstar [--headless] [--font PATH] [--page overview|pricing|settings] "
                "[--width N --height N] [--frames N --capture BMP]\n");
            return 1;
        }
    }
    if (width < 320 || height < 320 ||
        (strcmp(page, "overview") && strcmp(page, "pricing") && strcmp(page, "settings")) ||
        (capture && (!frames || headless))) {
        fprintf(stderr, "Invalid dimensions/page/capture options\n");
        return 1;
    }
    cui_config config = {.width = (float)width, .height = (float)height};
#ifdef APP_NATIVE
    cui_font *font = NULL;
    cui_sdl3 *backend = NULL;
    if (!headless) {
        if (!font_path) {
            fprintf(stderr, "Use --font PATH for native mode, or --headless\n");
            return 2;
        }
        font = cui_font_open(font_path);
        if (!font) {
            fprintf(stderr, "Could not open font\n");
            return 2;
        }
        config.measure = cui_font_measure;
        config.measure_styled = cui_font_measure_styled;
        config.text_geometry = cui_font_text_geometry;
        config.text_geometry_line = cui_font_text_geometry_line;
        config.font_metrics = cui_font_get_metrics;
        config.measure_user = font;
        backend = cui_sdl3_create("Northstar / CUI composed app", width, height, font);
        if (!backend) {
            fprintf(stderr, "SDL3: %s\n", cui_sdl3_error());
            cui_font_close(font);
            return 3;
        }
    }
#else
    (void)font_path;
    if (!headless) {
        fprintf(stderr, "Native adapter unavailable; use --headless\n");
        return 3;
    }
#endif
    cui_context *ui = cui_create(config);
    composed_app *app = composed_app_create(event, NULL);
    int result = 0;
    if (!ui || !app)
        result = 4;
    else if (!route(ui, app, page, (float)width, (float)height))
        result = 5;
    else if (headless) {
        cui_draw_list list = composed_app_frame(ui, app, (float)width, (float)height);
        printf("Northstar %s %dx%d: %zu real CUI commands\n", page, width, height, list.count);
        result = cui_last_error(ui) != CUI_OK || list.count < 30;
    }
#ifdef APP_NATIVE
    else {
        unsigned rendered = 0;
        while ((!frames || rendered < frames) && cui_sdl3_poll(backend, ui)) {
            float w, h;
            cui_sdl3_size(backend, &w, &h);
            cui_draw_list list = composed_app_frame(ui, app, w, h);
            bool painted = capture && rendered + 1 == frames
                               ? cui_sdl3_render_capture(backend, &list,
                                                         (cui_color){245, 247, 250, 255}, capture)
                               : cui_sdl3_render(backend, &list, (cui_color){245, 247, 250, 255});
            if (!painted || cui_last_error(ui) != CUI_OK) {
                result = 6;
                break;
            }
            ++rendered;
            cui_sdl3_wait(16);
        }
    }
#endif
    composed_app_destroy(app);
    cui_destroy(ui);
#ifdef APP_NATIVE
    cui_sdl3_destroy(backend);
    cui_font_close(font);
#endif
    return result;
}
