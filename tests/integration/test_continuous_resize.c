/* SPDX-License-Identifier: MIT */
#include "../../examples/responsive_dashboard/app.h"
#include "cui/cui_sdl3.h"
#include "cui/cui_theme.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
typedef struct {
    dashboard_app app;
    cui_sdl3 *host;
    cui_context *ui;
    cui_font *font;
    const char *folder, *quit;
    unsigned frames;
    bool unsupported;
} test_state;
static bool initialize(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    test_state *s = user;
    if (argc != 5)
        return false;
    s->folder = argv[2];
    s->quit = argv[3];
    s->font = cui_font_open(argv[1]);
    if (!s->font)
        return false;
    s->host = cui_sdl3_create(argv[4], 800, 760, s->font);
    if (!s->host) {
        const char *driver = cui_sdl3_video_driver();
        s->unsupported = !driver || strcmp(driver, "x11");
        return false;
    }
    const char *driver = cui_sdl3_video_driver();
    if (!driver || strcmp(driver, "x11")) {
        s->unsupported = true;
        return false;
    }
    s->ui = cui_create((cui_config){.width = 800,
                                    .height = 760,
                                    .measure = cui_font_measure,
                                    .measure_styled = cui_font_measure_styled,
                                    .text_geometry = cui_font_text_geometry,
                                    .text_geometry_line = cui_font_text_geometry_line,
                                    .font_metrics = cui_font_get_metrics,
                                    .measure_user = s->font});
    if (!s->ui)
        return false;
    dashboard_init(&s->app, NULL, NULL);
    s->app.page = 1;
    s->app.signed_in = true;
    *host = s->host;
    *ui = s->ui;
    return true;
}
static int frame(void *user, cui_sdl3 *host, cui_context *ui) {
    test_state *s = user;
    FILE *quit = fopen(s->quit, "rb");
    if (quit) {
        fclose(quit);
        return 1;
    }
    float width, height;
    cui_sdl3_size(host, &width, &height);
    if (!cui_begin_frame(ui, width, height, cui_sdl3_frame_delta(host)))
        return -1;
    dashboard_scene(ui, &s->app, width, height);
    cui_draw_list list = cui_end_frame(ui);
    char path[4096];
    if (snprintf(path, sizeof path, "%s/frame-%u.bmp", s->folder, s->frames) >= (int)sizeof path)
        return -1;
    if (!cui_sdl3_render_capture(host, &list, (cui_color){245, 247, 250, 255}, path) ||
        cui_last_error(ui) != CUI_OK)
        return -1;
    cui_rect root = cui_bounds(ui, DASH_ROOT), main = cui_bounds(ui, DASH_MAIN);
    cui_rect a = cui_bounds(ui, DASH_SUMMARY), b = cui_bounds(ui, DASH_ACTIVITY);
    bool wide = width >= 1024;
    bool row = b.x >= a.x + a.w - 1;
    if (fabsf(root.w - width) > 1 || fabsf(root.h - height) > 1 || main.w <= 0 ||
        s->app.page != 1 || !s->app.signed_in || s->app.drawer_open)
        return -1;
    if (wide) {
        cui_rect nav = cui_bounds(ui, DASH_NAV);
        if (nav.w <= 0 || main.x < nav.x + nav.w - 1)
            return -1;
    } else if (cui_bounds(ui, DASH_MENU).w <= 0 || cui_bounds(ui, DASH_NAV).w > 0)
        return -1;
    cui_color card = cui_theme_light().muted_background;
    printf("FRAME %u %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %d %d %u %d %.0f %.0f %.0f %.0f %.0f "
           "%.0f %u %u %u\n",
           s->frames, width, height, root.w, root.h, main.x, main.w, a.w, b.w, wide, row,
           s->app.page, s->app.signed_in, a.x, a.y, a.h, b.x, b.y, b.h, (unsigned)card.r,
           (unsigned)card.g, (unsigned)card.b);
    fflush(stdout);
    ++s->frames;
    return 0;
}
static void shutdown(void *user, bool success) {
    test_state *s = user;
    if (!success && !s->unsupported)
        fprintf(stderr, "resize child: %s\n", cui_sdl3_error());
    cui_destroy(s->ui);
    cui_sdl3_destroy(s->host);
    cui_font_close(s->font);
}
#endif
int main(int argc, char **argv) {
#ifndef __linux__
    (void)argc;
    (void)argv;
    return 77;
#else
    const char *isolated = getenv("CUI_TEST_VIRTUAL_DISPLAY");
    if (!isolated || strcmp(isolated, "1")) {
        fprintf(stderr, "Requires scripts/test_local.py private Xvfb; desktop access refused.\n");
        return 77;
    }
    test_state state = {0};
    int result = cui_sdl3_run(argc, argv, (cui_sdl3_app){&state, initialize, frame, shutdown});
    return state.unsupported ? 77 : result;
#endif
}
