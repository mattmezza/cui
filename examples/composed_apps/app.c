/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_components.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct composed_app {
    app_page page;
    int plan;
    bool logged_in, desktop_sidebar, mobile_sidebar, narrow;
    bool notifications, sync, invalid_name;
    unsigned saves;
    char display_name[80], saved_name[80], notice[160], account_label[112];
    cui_edit *name;
    cui_input_state name_state;
    cui_choice_popup account;
    cui_scroll_state scroll;
    app_callback callback;
    void *user;
};
static const cui_color ink = {25, 38, 58, 255}, muted = {101, 114, 133, 255};
static const cui_color navy = {20, 35, 55, 255}, white = {255, 255, 255, 255};
static const char *const pages[] = {"Overview", "Plans & billing", "Workspace settings"};
static const char *const plans[] = {"Starter", "Team", "Studio"};
composed_app *composed_app_create(app_callback callback, void *user) {
    composed_app *app = calloc(1, sizeof *app);
    if (!app)
        return NULL;
    app->desktop_sidebar = true;
    app->notifications = true;
    app->callback = callback;
    app->user = user;
    memcpy(app->display_name, "Ada Lovelace", sizeof "Ada Lovelace");
    memcpy(app->saved_name, "Ada Lovelace", sizeof "Ada Lovelace");
    app->name = cui_edit_create((cui_edit_config){.buffer = app->display_name,
                                                  .capacity = sizeof app->display_name,
                                                  .history_bytes = 2048,
                                                  .max_history_steps = 32});
    if (!app->name) {
        free(app);
        return NULL;
    }
    cui_choice_popup_init(&app->account, 900);
    return app;
}
void composed_app_destroy(composed_app *app) {
    if (!app)
        return;
    cui_edit_destroy(app->name);
    free(app);
}
app_snapshot composed_app_snapshot(const composed_app *a) {
    if (!a)
        return (app_snapshot){0};
    return (app_snapshot){.page = a->page,
                          .plan = a->plan,
                          .logged_in = a->logged_in,
                          .sidebar_visible = a->narrow ? a->mobile_sidebar : a->desktop_sidebar,
                          .notifications = a->notifications,
                          .sync = a->sync,
                          .invalid_name = a->invalid_name,
                          .saves = a->saves,
                          .display_name = a->display_name,
                          .saved_name = a->saved_name,
                          .notice = a->notice};
}
static void emit(composed_app *a, app_event event) {
    if (a->callback)
        a->callback(a->user, event, composed_app_snapshot(a));
}
static void navigate(composed_app *a, app_page page) {
    a->page = page;
    a->scroll = (cui_scroll_state){0};
    a->mobile_sidebar = false;
    a->notice[0] = '\0';
    emit(a, APP_NAVIGATED);
}
static void overview(void *user) {
    navigate(user, APP_OVERVIEW);
}
static void pricing(void *user) {
    navigate(user, APP_PRICING);
}
static void settings(void *user) {
    navigate(user, APP_SETTINGS);
}
static void text(cui_context *ui, const char *value, float size, cui_color color, bool bold) {
    cui_text_ex(ui, cui_string(value), size, color, size * 1.35f,
                (cui_text_style){.weight = bold ? CUI_TEXT_WEIGHT_BOLD : CUI_TEXT_WEIGHT_NORMAL});
}
static void spacer(cui_context *ui) {
    cui_begin_box(ui, 0, (cui_box){.width = -1, .height = 1});
    cui_end_box(ui);
}
static bool button(cui_context *ui, cui_id id, const cui_theme *t, const char *name,
                   cui_button_variant variant, float width) {
    return cui_button_ex(ui, id, t, cui_string(name),
                         (cui_button_options){.variant = variant, .width = width});
}
static void sidebar(cui_context *ui, composed_app *a, const cui_theme *theme, float width) {
    cui_theme t = *theme;
    t.sidebar_foreground = (cui_color){197, 207, 220, 255};
    t.sidebar_accent = (cui_color){43, 64, 90, 255};
    t.sidebar_accent_foreground = white;
    cui_begin_box(ui, 10,
                  (cui_box){.width = width,
                            .height = a->narrow ? 0 : -1,
                            .padding = 20,
                            .gap = 20,
                            .background = navy});
    if (!a->narrow) {
        text(ui, "N / NORTHSTAR", 16, white, true);
        text(ui, "YOUR TEAM WORKSPACE", 10, (cui_color){136, 159, 186, 255}, true);
    }
    cui_sidebar_menu_item items[] = {{.id = APP_NAV_OVERVIEW,
                                      .label = cui_string("Overview"),
                                      .icon = cui_string("layout-dashboard"),
                                      .active = a->page == APP_OVERVIEW,
                                      .on_activate = overview,
                                      .user = a},
                                     {.id = APP_NAV_PRICING,
                                      .label = cui_string("Plans & billing"),
                                      .icon = cui_string("building-2"),
                                      .active = a->page == APP_PRICING,
                                      .on_activate = pricing,
                                      .user = a},
                                     {.id = APP_NAV_SETTINGS,
                                      .label = cui_string("Settings"),
                                      .icon = cui_string("settings"),
                                      .active = a->page == APP_SETTINGS,
                                      .on_activate = settings,
                                      .user = a}};
    cui_sidebar_menu(ui, 11, &t, items, 3, false);
    if (!a->narrow) {
        cui_begin_box(ui, 0, (cui_box){.height = -1, .width = -1});
        cui_end_box(ui);
        cui_begin_box(ui, 12,
                      (cui_box){.width = -1,
                                .padding = 14,
                                .gap = 8,
                                .radius = 10,
                                .background = {31, 49, 73, 255}});
        text(ui, "Room to grow", 14, white, true);
        text(ui, "Find a plan that fits your team.", 12, (cui_color){176, 192, 212, 255}, false);
        if (button(ui, 113, theme, "Explore plans", CUI_BUTTON_SECONDARY, -1))
            pricing(a);
        cui_end_box(ui);
        text(ui, "Local demo / no cloud connection", 10, (cui_color){136, 159, 186, 255}, false);
    }
    cui_end_box(ui);
}
static void topbar(cui_context *ui, composed_app *a, const cui_theme *t, float w, float h) {
    cui_begin_box(ui, 20,
                  (cui_box){.width = -1,
                            .height = 72,
                            .row = true,
                            .padding = a->narrow ? 12 : 24,
                            .gap = 16,
                            .align_y = CUI_ALIGN_CENTER,
                            .background = white,
                            .border = {226, 231, 237, 255},
                            .custom_border_edges = true,
                            .border_edges = {.bottom = 1}});
    if (cui_button_ex(ui, APP_MENU, t, cui_string("Toggle navigation"),
                      (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                           .icon = cui_string("menu"),
                                           .icon_only = true})) {
        if (a->narrow)
            a->mobile_sidebar = !a->mobile_sidebar;
        else
            a->desktop_sidebar = !a->desktop_sidebar;
    }
    text(ui, a->narrow ? "Northstar" : pages[a->page], 16, ink, true);
    spacer(ui);
    cui_choice_item items[] = {
        {.id = APP_PROFILE, .label = cui_string("Workspace settings")},
        {.id = a->logged_in ? APP_LOGOUT : APP_LOGIN,
         .label = cui_string(a->logged_in ? "Sign out of demo" : "Sign in to demo")}};
    cui_id action = 0;
    snprintf(a->account_label, sizeof a->account_label, "%s", "Guest / sign in");
    if (a->logged_in) {
        if (a->narrow)
            snprintf(a->account_label, sizeof a->account_label, "%s", "Account");
        else {
            size_t length = strlen(a->saved_name);
            if (length > 24) {
                length = 24;
                while (length && ((unsigned char)a->saved_name[length] & 0xc0u) == 0x80u)
                    --length;
            }
            snprintf(a->account_label, sizeof a->account_label, "%.*s / account", (int)length,
                     a->saved_name);
        }
    }
    if (cui_menu(ui, APP_ACCOUNT, t, cui_string(a->account_label), items, 2, &action, &a->account,
                 (cui_choice_popup_options){
                     .viewport = {0, 0, w, h}, .width = 220, .align_end = true})) {
        if (action == APP_PROFILE)
            settings(a);
        else {
            a->logged_in = action == APP_LOGIN;
            snprintf(a->notice, sizeof a->notice, "%s",
                     a->logged_in ? "Signed in to the local demo. No credentials were sent."
                                  : "Signed out of the local demo.");
            emit(a, APP_SESSION_CHANGED);
        }
    }
    cui_end_box(ui);
}
static void metric(cui_context *ui, const char *label, const char *value, const char *detail) {
    cui_begin_box(ui, 0,
                  (cui_box){.width = -1,
                            .padding = 20,
                            .gap = 8,
                            .radius = 12,
                            .background = white,
                            .border = {225, 231, 238, 255},
                            .border_width = 1});
    text(ui, label, 12, muted, false);
    text(ui, value, 30, ink, true);
    text(ui, detail, 11, (cui_color){34, 127, 98, 255}, false);
    cui_end_box(ui);
}
static void render_overview(cui_context *ui, composed_app *a, const cui_theme *t, float width) {
    bool stack = width < 650;
    cui_begin_box(ui, 30,
                  (cui_box){.width = -1,
                            .padding = stack ? 24 : 32,
                            .gap = 16,
                            .radius = 16,
                            .background = {227, 237, 249, 255}});
    text(ui, "BUILD SOMETHING GREAT, TOGETHER", 11, (cui_color){56, 89, 128, 255}, true);
    text(ui, "A calmer place to do your best work.", stack ? 30 : 42, ink, true);
    text(ui, "Keep your projects, people and next steps in one thoughtful workspace.", 15, muted,
         false);
    if (button(ui, APP_HERO_CTA, t, "Find your team's plan", CUI_BUTTON_PRIMARY, 0))
        pricing(a);
    cui_end_box(ui);
    cui_begin_box(ui, 31, (cui_box){.width = -1, .row = !stack, .gap = 16});
    metric(ui, "Active projects", "12", "Sample workspace activity");
    metric(ui, "Teammates", "8", "A small team, a shared direction");
    metric(ui, "Weekly progress", "84%", "Illustrative demo metrics");
    cui_end_box(ui);
    cui_begin_box(ui, 32,
                  (cui_box){.width = -1,
                            .padding = 24,
                            .gap = 14,
                            .radius = 12,
                            .background = white,
                            .border = {225, 231, 238, 255},
                            .border_width = 1});
    text(ui, "Your next chapter", 20, ink, true);
    text(ui, "Make this workspace yours", 14, ink, true);
    text(ui, "Choose a display name and decide which updates reach you.", 13, muted, false);
    if (button(ui, 121, t, "Personalize workspace", CUI_BUTTON_OUTLINE, 0))
        settings(a);
    cui_end_box(ui);
}
static void plan_card(cui_context *ui, composed_app *a, const cui_theme *t, int plan) {
    static const char *const prices[] = {"$0", "$18", "$39"};
    static const char *const details[] = {"For a thoughtful beginning",
                                          "For teams finding their rhythm",
                                          "For ambitious work at scale"};
    static const char *const features[][3] = {
        {"3 active projects", "Personal workspace", "Community support"},
        {"Unlimited projects", "Up to 20 teammates", "Shared workspace settings"},
        {"Unlimited teammates", "Advanced project views", "Priority support"}};
    bool selected = a->plan == plan;
    cui_begin_box(ui, (cui_id)(40 + plan),
                  (cui_box){.width = -1,
                            .padding = 24,
                            .gap = 14,
                            .radius = 14,
                            .background = white,
                            .border = plan == 1 ? t->primary : t->border,
                            .border_width = plan == 1 ? 2 : 1});
    text(ui, plan == 1 ? "TEAM FAVORITE" : "FLEXIBLE BY DESIGN", 10, plan == 1 ? t->primary : muted,
         true);
    text(ui, plans[plan], 24, ink, true);
    text(ui, details[plan], 12, muted, false);
    text(ui, prices[plan], 38, ink, true);
    text(ui, plan ? "per teammate / month" : "free for personal use", 12, muted, false);
    cui_separator(ui, 0, t, -1);
    for (int i = 0; i < 3; ++i)
        text(ui, features[plan][i], 13, ink, false);
    if (button(ui, (cui_id)(APP_PLAN_STARTER + plan), t,
               selected ? "Selected plan" : "Choose this plan",
               plan == 1 ? CUI_BUTTON_PRIMARY : CUI_BUTTON_OUTLINE, -1) &&
        !selected) {
        a->plan = plan;
        snprintf(a->notice, sizeof a->notice,
                 "%s plan selected locally. No payment or subscription was created.", plans[plan]);
        emit(a, APP_PLAN_CHANGED);
    }
    cui_end_box(ui);
}
static void render_pricing(cui_context *ui, composed_app *a, const cui_theme *t, float width) {
    text(ui, "Space for your next big idea.", width < 650 ? 30 : 36, ink, true);
    text(ui, "Simple plans for teams of every size. Pick a fit and keep creating.", 14, muted,
         false);
    cui_begin_box(ui, 39, (cui_box){.width = -1, .row = width >= 720, .gap = 16});
    for (int i = 0; i < 3; ++i)
        plan_card(ui, a, t, i);
    cui_end_box(ui);
    text(ui, "Demo pricing only. Selections remain in memory and reset when you close the app.", 12,
         muted, false);
}
static void name_control(cui_context *ui, const cui_theme *t, void *user, float width) {
    composed_app *a = user;
    if (cui_input_ex(
            ui, APP_NAME, t, cui_string("Display name"), a->name, &a->name_state,
            (cui_input_options){.width = width, .required = true, .invalid = a->invalid_name})
            .changed)
        a->invalid_name = false;
}
static void render_settings(cui_context *ui, composed_app *a, const cui_theme *t, float width) {
    text(ui, "A workspace that feels like you.", width < 650 ? 28 : 36, ink, true);
    text(ui, "Tune the small details. Keep the important things close.", 14, muted, false);
    cui_begin_box(ui, 50,
                  (cui_box){.width = -1,
                            .padding = 24,
                            .gap = 20,
                            .radius = 14,
                            .background = white,
                            .border = t->border,
                            .border_width = 1});
    text(ui, "Profile & preferences", 20, ink, true);
    cui_form_field field = {
        .id = 51,
        .control = APP_NAME,
        .label = cui_string("Display name"),
        .description = cui_string("Shown in your local workspace. Maximum 79 UTF-8 bytes."),
        .required = true,
        .invalid = a->invalid_name,
        .error = cui_string(a->invalid_name ? "Enter a name before saving." : ""),
        .render_control = name_control,
        .user = a};
    cui_form(ui, 52, t, &field, 1,
             (cui_form_options){
                 .width = width - 48, .columns = 1, .name = cui_string("Workspace profile")});
    cui_separator(ui, 53, t, -1);
    cui_checkbox(ui, APP_NOTIFICATIONS, t, cui_string("Email notifications"), &a->notifications,
                 false);
    text(ui, "Preference only: this demo never sends email.", 12, muted, false);
    cui_switch(ui, APP_SYNC, t, cui_string("Sync across devices"), &a->sync, false);
    text(ui, "Preference only: no remote sync service is connected.", 12, muted, false);
    if (button(ui, APP_SAVE, t, "Save preferences", CUI_BUTTON_PRIMARY, 0)) {
        cui_str name = cui_edit_get_text(a->name);
        bool nonspace = false;
        for (size_t i = 0; i < name.length; ++i)
            if (name.data[i] != ' ' && name.data[i] != '\t' && name.data[i] != '\n')
                nonspace = true;
        a->invalid_name = !nonspace;
        if (a->invalid_name)
            snprintf(a->notice, sizeof a->notice, "Please enter a display name before saving.");
        else {
            memcpy(a->saved_name, a->display_name, sizeof a->saved_name);
            ++a->saves;
            snprintf(a->notice, sizeof a->notice, "Preferences saved for %s in this session.",
                     a->saved_name);
            emit(a, APP_SETTINGS_SAVED);
        }
    }
    cui_end_box(ui);
}
void composed_app_scene(cui_context *ui, composed_app *a, float width, float height) {
    if (!ui || !a)
        return;
    a->narrow = width < 760;
    cui_theme t = cui_theme_light();
    t.primary = (cui_color){38, 82, 132, 255};
    t.buttons[CUI_THEME_BUTTON_PRIMARY].normal = t.primary;
    t.buttons[CUI_THEME_BUTTON_PRIMARY].hover = (cui_color){31, 69, 113, 255};
    t.buttons[CUI_THEME_BUTTON_PRIMARY].active = (cui_color){23, 56, 95, 255};
    bool show = a->narrow ? a->mobile_sidebar : a->desktop_sidebar;
    cui_begin_box(
        ui, 1,
        (cui_box){
            .width = -1, .height = -1, .row = !a->narrow, .background = {245, 247, 250, 255}});
    if (show && !a->narrow)
        sidebar(ui, a, &t, 224);
    cui_begin_box(ui, 2, (cui_box){.width = -1, .height = -1});
    topbar(ui, a, &t, width, height);
    if (a->narrow && a->mobile_sidebar)
        sidebar(ui, a, &t, -1);
    float content_width = width - ((show && !a->narrow) ? 224 : 0);
    float navigation_height = a->narrow && a->mobile_sidebar ? 154 : 0;
    float viewport_height = height - 72 - navigation_height;
    if (viewport_height < 1)
        viewport_height = 1;
    float inner_width = content_width - (a->narrow ? 32 : 64);
    float content_height = a->page == APP_OVERVIEW  ? (inner_width < 650 ? 1030 : 640)
                           : a->page == APP_PRICING ? (inner_width < 720 ? 1490 : 680)
                                                    : 660;
    cui_scroll_options scroll = {.width = content_width,
                                 .height = viewport_height,
                                 .content_width = content_width,
                                 .content_height = content_height,
                                 .vertical = true,
                                 .bar_thickness = 8,
                                 .wheel_step = 48,
                                 .hide_scrollbars = content_height <= viewport_height};
    if (cui_scrollable_begin(ui, APP_CONTENT, &t, &a->scroll, scroll)) {
        cui_begin_box(ui, 3, (cui_box){.width = -1, .padding = a->narrow ? 16 : 32, .gap = 20});
        if (a->page == APP_OVERVIEW)
            render_overview(ui, a, &t, inner_width);
        else if (a->page == APP_PRICING)
            render_pricing(ui, a, &t, inner_width);
        else
            render_settings(ui, a, &t, inner_width);
        if (a->notice[0]) {
            cui_begin_box(
                ui, 60,
                (cui_box){
                    .width = -1, .padding = 16, .radius = 10, .background = {227, 239, 231, 255}});
            text(ui, a->notice, 13, ink, false);
            cui_end_box(ui);
        }
        cui_end_box(ui);
        cui_scrollable_end_ex(ui, APP_CONTENT, &t, &a->scroll, scroll);
    }
    cui_end_box(ui);
    cui_end_box(ui);
}
cui_draw_list composed_app_frame(cui_context *ui, composed_app *app, float width, float height) {
    cui_begin_frame(ui, width, height, 0.016f);
    composed_app_scene(ui, app, width, height);
    return cui_end_frame(ui);
}
