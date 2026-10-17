/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_components.h"
#include "cui/cui_responsive.h"
void dashboard_init(dashboard_app *app, dashboard_callback callback, void *user) {
    if (app)
        *app = (dashboard_app){.callback = callback, .user = user};
}
static void changed(dashboard_app *app, dashboard_event event) {
    if (app->callback)
        app->callback(app->user, event, app->page, app->signed_in);
}
static void navigation(cui_context *ui, dashboard_app *app, const cui_theme *theme, float width,
                       bool persistent) {
    cui_begin_box(
        ui, DASH_NAV,
        (cui_box){.width = width, .padding = 12, .gap = 8, .background = theme->muted_background});
    cui_label(ui, theme, cui_string("Workspace"));
    if (cui_button(ui, DASH_HOME, theme, cui_string("Overview"), false)) {
        app->page = 0;
        if (!persistent)
            app->drawer_open = false;
        changed(app, DASH_NAVIGATED);
    }
    if (cui_button(ui, DASH_REPORTS, theme, cui_string("Reports"), false)) {
        app->page = 1;
        if (!persistent)
            app->drawer_open = false;
        changed(app, DASH_NAVIGATED);
    }
    cui_end_box(ui);
}
void dashboard_scene(cui_context *ui, dashboard_app *app, float width, float height) {
    if (!ui || !app || !isfinite(height) || height <= 0)
        return;
    cui_responsive container = cui_responsive_query(width, NULL);
    if (!container.valid || width <= 0)
        return;
    cui_theme theme = cui_theme_light();
    bool persistent = cui_responsive_at_least(container, CUI_BREAKPOINT_LG);
    float padding = cui_responsive_value(container, CUI_BREAKPOINT_MD, 12, 24);
    float inner = fmaxf(0, width - padding * 2);
    cui_begin_box(ui, DASH_ROOT,
                  (cui_box){.width = width,
                            .height = height,
                            .padding = padding,
                            .gap = 12,
                            .clip = true,
                            .background = theme.background});
    cui_begin_box(ui, 8200, (cui_box){.width = -1, .height = 40, .row = true, .gap = 8});
    if (!persistent && cui_button(ui, DASH_MENU, &theme, cui_string("Menu"), false))
        app->drawer_open = !app->drawer_open;
    if (cui_button(ui, DASH_ACCOUNT, &theme, cui_string(app->signed_in ? "Sign out" : "Sign in"),
                   false)) {
        app->signed_in = !app->signed_in;
        changed(app, DASH_ACCOUNT_CHANGED);
    }
    cui_end_box(ui);
    cui_begin_box(ui, 8201, (cui_box){.width = -1, .height = -1, .row = persistent, .gap = 12});
    if (persistent || app->drawer_open)
        navigation(ui, app, &theme, persistent ? 220 : inner, persistent);
    float main_width = fmaxf(0, inner - (persistent ? 232 : 0));
    cui_responsive main = cui_responsive_query(main_width, NULL);
    cui_begin_box(ui, DASH_MAIN, (cui_box){.width = -1, .height = -1, .gap = 12, .clip = true});
    cui_label(ui, &theme, cui_string(app->page ? "Reports" : "Overview"));
    cui_label(ui, &theme, cui_string(app->signed_in ? "Your workspace" : "Public workspace"));
    bool row = cui_responsive_fits(main, 260, 260, 12);
    cui_begin_box(ui, 8202, (cui_box){.width = -1, .row = row, .gap = 12});
    cui_begin_box(
        ui, DASH_SUMMARY,
        (cui_box){.width = -1, .height = 100, .padding = 12, .background = theme.muted_background});
    cui_label(ui, &theme, cui_string("Projects: 12"));
    cui_end_box(ui);
    cui_begin_box(
        ui, DASH_ACTIVITY,
        (cui_box){.width = -1, .height = 100, .padding = 12, .background = theme.muted_background});
    cui_label(ui, &theme, cui_string("Activity: all systems ready"));
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
}
