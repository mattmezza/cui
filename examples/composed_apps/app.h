/* SPDX-License-Identifier: MIT */
#ifndef CUI_COMPOSED_APP_H
#define CUI_COMPOSED_APP_H
#include "cui/cui.h"
#include "cui/cui_theme.h"
typedef struct composed_app composed_app;
typedef enum { APP_OVERVIEW, APP_PRICING, APP_SETTINGS } app_page;
typedef enum { APP_NAVIGATED, APP_SESSION_CHANGED, APP_PLAN_CHANGED, APP_SETTINGS_SAVED } app_event;
typedef struct {
    app_page page;
    int plan;
    bool logged_in, sidebar_visible, notifications, sync, invalid_name;
    unsigned saves;
    const char *display_name, *saved_name, *notice;
} app_snapshot;
typedef void (*app_callback)(void *user, app_event event, app_snapshot state);
/* Stable IDs for embedding hosts, interaction tests and screenshot automation. */
enum {
    APP_MENU = 100,
    APP_ACCOUNT = 101,
    APP_LOGIN = 102,
    APP_LOGOUT = 103,
    APP_PROFILE = 104,
    APP_NAV_OVERVIEW = 110,
    APP_NAV_PRICING = 111,
    APP_NAV_SETTINGS = 112,
    APP_HERO_CTA = 120,
    APP_PLAN_STARTER = 130,
    APP_PLAN_TEAM = 131,
    APP_PLAN_STUDIO = 132,
    APP_NAME = 140,
    APP_NOTIFICATIONS = 141,
    APP_SYNC = 142,
    APP_SAVE = 143,
    APP_CONTENT = 150
};
composed_app *composed_app_create(app_callback callback, void *user);
void composed_app_destroy(composed_app *app);
/* Copy theme values; NULL restores the original native palette. Pointer-valued
 * theme hooks/user storage remain host-owned and must outlive scene submission.
 * Set between frames on the UI thread; theme changes preserve application state. */
void composed_app_set_theme(composed_app *app, const cui_theme *theme);
app_snapshot composed_app_snapshot(const composed_app *app);
/* Submit inside an active frame; caller owns begin/end and renderer. Retain app
 * until renderer completion: draw-command text borrows application storage. */
void composed_app_scene(cui_context *ui, composed_app *app, float width, float height);
cui_draw_list composed_app_frame(cui_context *ui, composed_app *app, float width, float height);
#endif
