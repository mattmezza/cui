/* SPDX-License-Identifier: MIT */
#ifdef CUI_TEST_STARTER_RESIZE
#include "../../examples/native_starter/app.h"
#else
#include "../../examples/responsive_dashboard/app.h"
#endif
#include "cui/cui_sdl3.h"
#include "cui/cui_theme.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
typedef struct {
#ifdef CUI_TEST_STARTER_RESIZE
    starter_app app;
    bool workspace;
#else
    dashboard_app app;
#endif
    cui_sdl3 *host;
    cui_context *ui;
    cui_font *font;
    const char *folder, *quit;
    unsigned frames;
    bool unsupported;
} test_state;
static bool initialize(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    test_state *s = user;
#ifdef CUI_TEST_STARTER_RESIZE
    s->workspace = argc == 6 && !strcmp(argv[5], "starter-workspace");
    if (argc != 5 && !s->workspace)
        return false;
#else
    if (argc != 5)
        return false;
#endif
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
#ifdef CUI_TEST_STARTER_RESIZE
    if (!starter_app_init(&s->app, NULL, NULL))
        return false;
    s->app.signed_in = true;
    if (s->workspace) {
        s->app.page = STARTER_PROJECTS;
        s->app.project_index = 1;
        s->app.project_tab = STARTER_PROJECT_ACCESS;
        cui_focus(s->ui, STARTER_PROJECT_ACCESS);
    } else {
        s->app.page = STARTER_SETTINGS;
        s->app.notifications = false;
        if (cui_edit_replace_range(s->app.name_edit, 0, 3, cui_string("Unsaved")) != CUI_EDIT_OK)
            return false;
        cui_focus(s->ui, STARTER_NAME);
    }
#else
    dashboard_init(&s->app, NULL, NULL);
    s->app.page = 1;
    s->app.signed_in = true;
#endif
    *host = s->host;
    *ui = s->ui;
    return true;
}
#ifdef CUI_TEST_STARTER_RESIZE
static bool report_workspace(test_state *s, float width, float height, cui_rect root, cui_rect main,
                             bool wide) {
    cui_id group_id = wide ? STARTER_PROJECT_WIDE : STARTER_PROJECT_COMPACT;
    cui_rect group = cui_bounds(s->ui, group_id);
    cui_rect list = cui_bounds(s->ui, STARTER_PROJECT_LIST);
    cui_rect detail = cui_bounds(s->ui, STARTER_PROJECT_DETAILS);
    cui_rect divider = cui_bounds(s->ui, cui_resizable_handle_id(group_id, STARTER_PROJECT_LIST));
    cui_rect strip = cui_bounds(s->ui, s->app.project_tab_scroll.receiving_id);
    cui_rect tab = cui_bounds(s->ui, STARTER_PROJECT_ACCESS);
    if (tab.w <= 0 || tab.x < strip.x - 1 || tab.x + tab.w > strip.x + strip.w + 1)
        return false;
    bool fits = wide ? (list.y == detail.y && list.h == group.h && detail.h == group.h &&
                        fabsf(list.x + list.w - detail.x) < 1 &&
                        fabsf(detail.x + detail.w - group.x - group.w) < 1 &&
                        fabsf(divider.x + divider.w / 2 - detail.x) <= 1)
                     : (list.x == detail.x && list.w == group.w && detail.w == group.w &&
                        fabsf(list.y + list.h - detail.y) < 1 &&
                        fabsf(detail.y + detail.h - group.y - group.h) < 1 &&
                        fabsf(divider.y + divider.h / 2 - detail.y) <= 1);
    if (!fits || s->app.page != STARTER_PROJECTS || s->app.project_index != 1 ||
        s->app.project_tab != STARTER_PROJECT_ACCESS ||
        cui_focused(s->ui) != STARTER_PROJECT_ACCESS || s->app.project_overflow.menu.overlay.open)
        return false;
    cui_theme t = cui_theme_light();
    printf("FRAME %u %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f "
           "%.0f %.0f %.0f %.0f %u %u %u %u %u %u\n",
           s->frames, width, height, root.w, root.h, main.x, main.w, list.x, list.y, list.w, list.h,
           detail.x, detail.y, detail.w, detail.h, (unsigned)t.muted_background.r,
           (unsigned)t.muted_background.g, (unsigned)t.muted_background.b, (unsigned)t.surface.r,
           (unsigned)t.surface.g, (unsigned)t.surface.b);
    return true;
}
#endif
static int frame(void *user, cui_sdl3 *host, cui_context *ui) {
    test_state *s = user;
    FILE *quit = fopen(s->quit, "rb");
    if (quit) {
        fclose(quit);
        return 1;
    }
    float width, height;
    cui_sdl3_size(host, &width, &height);
    float dt = cui_sdl3_frame_delta(host);
#ifdef CUI_TEST_STARTER_RESIZE
    cui_draw_list list = starter_app_frame(ui, &s->app, width, height, dt, NULL);
#else
    if (!cui_begin_frame(ui, width, height, dt))
        return -1;
    dashboard_scene(ui, &s->app, width, height);
    cui_draw_list list = cui_end_frame(ui);
#endif
    char path[4096];
    if (snprintf(path, sizeof path, "%s/frame-%u.bmp", s->folder, s->frames) >= (int)sizeof path)
        return -1;
    if (!list.count ||
        !cui_sdl3_render_capture(host, &list, (cui_color){245, 247, 250, 255}, path) ||
        cui_last_error(ui) != CUI_OK)
        return -1;
#ifdef CUI_TEST_STARTER_RESIZE
    cui_rect root = cui_bounds(ui, STARTER_ROOT), main = cui_bounds(ui, STARTER_MAIN);
    cui_rect sidebar = cui_bounds(ui, STARTER_SIDEBAR), menu = cui_bounds(ui, STARTER_MENU);
    cui_rect account = cui_bounds(ui, STARTER_ACCOUNT);
    bool wide = width >= 768;
    if (!s->app.signed_in || s->app.drawer.overlay.open || s->app.account.overlay.open)
        return -1;
    if (wide ? (sidebar.w != 220 || account.y + account.h <= height - 20 || menu.w > 0)
             : (sidebar.w > 0 || account.w > 0 || menu.w <= 0))
        return -1;
    if (s->workspace) {
        if (!report_workspace(s, width, height, root, main, wide))
            return -1;
    } else {
        if (s->app.page != STARTER_SETTINGS || s->app.notifications || s->app.saves ||
            strcmp(s->app.name, "Unsaved") || strcmp(s->app.saved_name, "Ada") ||
            cui_focused(ui) != STARTER_NAME)
            return -1;
        cui_rect input = cui_bounds(ui, STARTER_NAME);
        cui_theme theme = cui_theme_light();
        cui_color left = wide ? theme.surface : theme.background;
        printf("FRAME %u %.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f %u %u %u %u %u %u\n", s->frames,
               width, height, root.w, root.h, main.x, main.w, input.x, input.w, (unsigned)left.r,
               (unsigned)left.g, (unsigned)left.b, (unsigned)theme.background.r,
               (unsigned)theme.background.g, (unsigned)theme.background.b);
    }
#else
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
#endif
    fflush(stdout);
    ++s->frames;
    return 0;
}
static void shutdown(void *user, bool success) {
    test_state *s = user;
    if (!success && !s->unsupported)
        fprintf(stderr, "resize child: %s\n", cui_sdl3_error());
#ifdef CUI_TEST_STARTER_RESIZE
    starter_app_destroy(&s->app);
#endif
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
