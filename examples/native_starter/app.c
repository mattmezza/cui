/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_sidebar.h"
#include <string.h>
static void emit(starter_app *a, starter_event event) {
    if (a->callback)
        a->callback(a->user, event, a);
}
static void navigate(starter_app *a, starter_page page) {
    a->page = page;
    emit(a, STARTER_NAVIGATED);
}
static void home(void *user) {
    navigate(user, STARTER_HOME);
}
static void projects(void *user) {
    navigate(user, STARTER_PROJECTS);
}
static void settings(void *user) {
    navigate(user, STARTER_SETTINGS);
}
bool starter_app_init(starter_app *a, starter_callback callback, void *user) {
    if (!a)
        return false;
    *a = (starter_app){.notifications = true, .callback = callback, .user = user};
    strcpy(a->name, "Ada");
    strcpy(a->saved_name, "Ada");
    a->name_edit = cui_edit_create((cui_edit_config){.buffer = a->name,
                                                     .capacity = sizeof a->name,
                                                     .history_bytes = 2048,
                                                     .max_history_steps = 32});
    cui_choice_popup_init(&a->account, 900);
    return a->name_edit != NULL;
}
void starter_app_destroy(starter_app *a) {
    if (a) {
        cui_edit_destroy(a->name_edit);
        a->name_edit = NULL;
    }
}
static void label(cui_context *ui, const cui_theme *t, const char *text) {
    cui_text(ui, cui_string(text), t->font_size, t->text);
}
cui_draw_list starter_app_frame(cui_context *ui, starter_app *a, float width, float height,
                                float dt, const cui_platform *clipboard) {
    if (!cui_begin_frame(ui, width, height, dt))
        return (cui_draw_list){0};
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1,
                  (cui_box){.width = -1, .height = -1, .row = true, .background = t.background});
    cui_begin_box(
        ui, 10,
        (cui_box){.width = 220, .height = -1, .padding = 16, .gap = 16, .background = t.surface});
    label(ui, &t, "My native app");
    cui_sidebar_menu_item items[] = {{.id = STARTER_NAV_HOME,
                                      .label = cui_string("Home"),
                                      .active = a->page == STARTER_HOME,
                                      .on_activate = home,
                                      .user = a},
                                     {.id = STARTER_NAV_PROJECTS,
                                      .label = cui_string("Projects"),
                                      .active = a->page == STARTER_PROJECTS,
                                      .on_activate = projects,
                                      .user = a},
                                     {.id = STARTER_NAV_SETTINGS,
                                      .label = cui_string("Settings"),
                                      .active = a->page == STARTER_SETTINGS,
                                      .on_activate = settings,
                                      .user = a}};
    cui_sidebar_menu(ui, 11, &t, items, 3, false);
    /* A growing spacer keeps the account trigger at the sidebar bottom. */
    cui_begin_box(ui, 0, (cui_box){.width = -1, .height = -1});
    cui_end_box(ui);
    cui_choice_item account_items[] = {
        {.id = STARTER_ACCOUNT_SETTINGS, .label = cui_string("Settings")},
        {.id = STARTER_SESSION,
         .label = cui_string(a->signed_in ? "Sign out (demo)" : "Sign in (demo)")}};
    cui_id action = 0;
    if (cui_menu(ui, STARTER_ACCOUNT, &t,
                 cui_string(a->signed_in ? "Ada / account" : "Guest / account"), account_items, 2,
                 &action, &a->account,
                 (cui_choice_popup_options){.viewport = {0, 0, width, height},
                                            .width = 200,
                                            .side = CUI_POPUP_ABOVE,
                                            .dt = dt})) {
        if (action == STARTER_ACCOUNT_SETTINGS)
            settings(a);
        if (action == STARTER_SESSION) {
            a->signed_in = !a->signed_in;
            emit(a, STARTER_SESSION_CHANGED);
        }
    }
    cui_end_box(ui);
    cui_begin_box(ui, 20, (cui_box){.width = -1, .height = -1, .padding = 32, .gap = 20});
    const char *titles[] = {"Home", "Projects", "Settings"};
    cui_text(ui, cui_string(titles[a->page]), 28, t.text);
    if (a->page == STARTER_HOME) {
        label(ui, &t, "Welcome. This is a small C11 application using real CUI controls.");
        label(ui, &t, "Choose a page on the left, or open your account below.");
    } else if (a->page == STARTER_PROJECTS) {
        label(ui, &t, "Example projects");
        label(ui, &t, "Desktop app");
        label(ui, &t, "Personal tools");
    } else {
        label(ui, &t, "Display name");
        cui_input_ex(ui, STARTER_NAME, &t, cui_string("Display name"), a->name_edit, &a->name_input,
                     (cui_input_options){.width = 300, .clipboard = clipboard});
        cui_checkbox(ui, STARTER_NOTIFICATIONS, &t, cui_string("Enable notifications"),
                     &a->notifications, false);
        if (cui_button(ui, STARTER_SAVE, &t, cui_string("Save settings"), false)) {
            memcpy(a->saved_name, a->name, sizeof a->name);
            a->saves++;
            emit(a, STARTER_SAVED);
        }
        if (a->saves)
            label(ui, &t, "Settings saved locally for this session.");
    }
    cui_end_box(ui);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
