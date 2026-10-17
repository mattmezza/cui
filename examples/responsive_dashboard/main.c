/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_sdl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
typedef struct {
    dashboard_app app;
    cui_sdl3 *host;
    cui_context *ui;
    cui_font *font;
    const char *capture;
    unsigned frames, limit;
} dashboard_host;
static const char *default_font(void) {
#ifdef __APPLE__
    static char path[4096];
    uint32_t size = (uint32_t)sizeof path;
    if (_NSGetExecutablePath(path, &size) == 0) {
        char *slash = strrchr(path, '/');
        const char suffix[] = "/../Resources/inter/Inter-Regular.otf";
        if (slash && (size_t)(slash - path) + sizeof suffix <= sizeof path) {
            strcpy(slash, suffix);
            FILE *file = fopen(path, "rb");
            if (file) { fclose(file); return path; }
        }
    }
#endif
    return "third_party/inter/Inter-Regular.otf";
}
static bool initialize(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    dashboard_host *state = user;
    const char *font_path = default_font();
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--font") && i + 1 < argc) font_path = argv[++i];
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc) state->capture = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) {
            char *end;
            unsigned long limit = strtoul(argv[++i], &end, 10);
            if (*end || !limit || limit > 1000000) return false;
            state->limit = (unsigned)limit;
        } else {
            fprintf(stderr, "Usage: cui_responsive_dashboard [--font PATH] [--frames N [--capture BMP]]\n");
            return false;
        }
    }
    if (state->capture && !state->limit) return false;
    state->font = cui_font_open(font_path);
    if (!state->font) { fprintf(stderr, "Cannot open font: %s\n", font_path); return false; }
    state->host = cui_sdl3_create("CUI Responsive Dashboard", 1100, 760, state->font);
    if (!state->host) return false;
    state->ui = cui_create((cui_config){.width = 1100, .height = 760,
        .measure = cui_font_measure, .measure_styled = cui_font_measure_styled,
        .text_geometry = cui_font_text_geometry, .text_geometry_line = cui_font_text_geometry_line,
        .font_metrics = cui_font_get_metrics, .measure_user = state->font});
    if (!state->ui) return false;
    dashboard_init(&state->app, NULL, NULL);
    *host = state->host; *ui = state->ui;
    return true;
}
static int frame(void *user, cui_sdl3 *host, cui_context *ui) {
    dashboard_host *state = user;
    float width, height;
    cui_sdl3_size(host, &width, &height);
    if (!cui_begin_frame(ui, width, height, cui_sdl3_frame_delta(host))) return -1;
    dashboard_scene(ui, &state->app, width, height);
    cui_draw_list list = cui_end_frame(ui);
    bool last = state->limit && state->frames + 1 == state->limit;
    bool rendered = last && state->capture
        ? cui_sdl3_render_capture(host, &list, (cui_color){245, 247, 250, 255}, state->capture)
        : cui_sdl3_render(host, &list, (cui_color){245, 247, 250, 255});
    if (!rendered || cui_last_error(ui) != CUI_OK) return -1;
    ++state->frames;
    if (last) return 1;
    if (state->limit) cui_invalidate(ui);
    return 0;
}
static void shutdown(void *user, bool success) {
    dashboard_host *state = user;
    if (!success) fprintf(stderr, "Responsive dashboard: %s\n", cui_sdl3_error());
    cui_destroy(state->ui);
    cui_sdl3_destroy(state->host);
    cui_font_close(state->font);
}
int main(int argc, char **argv) {
    dashboard_host state = {0};
    return cui_sdl3_run(argc, argv, (cui_sdl3_app){&state, initialize, frame, shutdown});
}
