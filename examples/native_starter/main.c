/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_sdl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <stdint.h>
#endif

/* Finder supplies no --font argument. Resolve resources from the executable,
 * independent of the working directory; command-line overrides still win. */
static const char *bundle_font(void) {
#ifdef __APPLE__
    static char path[4096];
    uint32_t size = (uint32_t)sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        char *slash = strrchr(path, '/');
        const char suffix[] = "/../Resources/inter/Inter-Regular.otf";
        if (slash && (size_t)(slash - path) + sizeof(suffix) <= sizeof(path)) {
            strcpy(slash, suffix);
            FILE *file = fopen(path, "rb");
            if (file) {
                fclose(file);
                return path;
            }
        }
    }
#endif
    return NULL;
}
static void changed(void *user, starter_event event, const starter_app *app) {
    (void)user;
    printf("event=%d page=%d signed_in=%d saves=%u name=%s notifications=%d\n", event, app->page,
           app->signed_in, app->saves, app->saved_name, app->notifications);
    fflush(stdout);
}
typedef struct {
    cui_font *font;
    cui_sdl3 *host;
    cui_context *ui;
    starter_app app;
    const char *capture;
    unsigned limit, frame;
} starter_host;
static bool initialize(void *user, int argc, char **argv, cui_sdl3 **out_host,
                       cui_context **out_ui) {
    starter_host *state = user;
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
            return false;
        }
    }
    if (!font_path)
        font_path = bundle_font();
    if (!font_path || (capture && !limit)) {
        fprintf(stderr, "Supply --font; capture requires --frames.\n");
        return false;
    }
    cui_font *font = cui_font_open(font_path);
    if (!font) {
        fprintf(stderr, "Cannot open font: %s\n", font_path);
        return false;
    }
    cui_sdl3 *host = cui_sdl3_create("CUI native starter", 960, 640, font);
    if (!host) {
        fprintf(stderr, "SDL3: %s\n", cui_sdl3_error());
        cui_font_close(font);
        return false;
    }
    state->font = font;
    state->host = host;
    cui_context *ui = cui_create((cui_config){.width = 960,
                                              .height = 640,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .text_geometry = cui_font_text_geometry,
                                              .text_geometry_line = cui_font_text_geometry_line,
                                              .font_metrics = cui_font_get_metrics,
                                              .measure_user = font});
    state->ui = ui;
    state->capture = capture;
    state->limit = limit;
    *out_host = host;
    *out_ui = ui;
    return ui && starter_app_init(&state->app, changed, NULL);
}
static int iterate(void *user, cui_sdl3 *host, cui_context *ui) {
    starter_host *state = user;
    if (!cui_sdl3_poll(host, ui))
        return 1;
    if (state->frame && !state->limit && !cui_needs_redraw(ui))
        return 0;
    float width, height;
    cui_sdl3_size(host, &width, &height);
    cui_platform clipboard = cui_sdl3_clipboard_interface(host);
    cui_draw_list list =
        starter_app_frame(ui, &state->app, width, height, cui_sdl3_frame_delta(host), &clipboard);
    (void)cui_sdl3_set_cursor(host, cui_pointer_cursor(ui));
    const char *shot = state->capture && state->frame + 1 == state->limit ? state->capture : NULL;
    if (!cui_sdl3_render_capture(host, &list, (cui_color){245, 245, 245, 255}, shot) ||
        cui_last_error(ui) != CUI_OK)
        return -1;
    state->frame++;
    if (state->limit && state->frame < state->limit)
        cui_invalidate(ui);
    return state->limit && state->frame >= state->limit ? 1 : 0;
}
static void shutdown(void *user, bool success) {
    (void)success;
    starter_host *state = user;
    starter_app_destroy(&state->app);
    cui_destroy(state->ui);
    cui_sdl3_destroy(state->host);
    cui_font_close(state->font);
}
int main(int argc, char **argv) {
    starter_host state = {0};
    return cui_sdl3_run(argc, argv, (cui_sdl3_app){&state, initialize, iterate, shutdown});
}
