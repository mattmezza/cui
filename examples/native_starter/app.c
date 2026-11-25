/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_responsive.h"
#include <string.h>

static const float sidebar_width = 220;
static const float drawer_width = 280;

static void emit(starter_app *a, starter_event event) {
    if (a->callback)
        a->callback(a->user, event, a);
}
static void navigate(starter_app *a, starter_page page) {
    a->page = page;
    emit(a, STARTER_NAVIGATED);
}

bool starter_app_init(starter_app *a, starter_callback callback, void *user) {
    if (!a)
        return false;
    *a = (starter_app){.notifications = true, .callback = callback, .user = user};
    if (!cui_sidebar_state_init(&a->sidebar, a->navigation_cache, 1))
        return false;
    strcpy(a->name, "Ada");
    strcpy(a->saved_name, "Ada");
    a->name_edit = cui_edit_create((cui_edit_config){.buffer = a->name,
                                                     .capacity = sizeof a->name,
                                                     .history_bytes = 2048,
                                                     .max_history_steps = 32});
    cui_surface_init(&a->drawer, STARTER_DRAWER, CUI_SURFACE_SHEET);
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
    (void)cui_text_paragraph_colored(ui, cui_string(text), t->font_size, t->text, 0,
                                     (cui_text_style){0}, true, NULL, 0);
}

typedef struct {
    starter_app *app;
    starter_page page;
    bool *selected;
} starter_link;

static void activate_page(void *user) {
    starter_link *link = user;
    *link->selected = true;
    navigate(link->app, link->page);
}

static bool navigation(cui_context *ui, const cui_theme *t, starter_app *a) {
    const char *labels[] = {"Home", "Projects", "Settings"};
    const cui_id ids[] = {STARTER_NAV_HOME, STARTER_NAV_PROJECTS, STARTER_NAV_SETTINGS};
    starter_link links[3];
    cui_sidebar_menu_item items[3];
    bool selected = false;
    for (size_t i = 0; i < 3; ++i) {
        links[i] = (starter_link){a, (starter_page)i, &selected};
        items[i] = (cui_sidebar_menu_item){.id = ids[i],
                                           .label = cui_string(labels[i]),
                                           .active = a->page == links[i].page,
                                           .on_activate = activate_page,
                                           .user = &links[i]};
    }
    cui_sidebar_menu(ui, 11, t, items, 3, false);
    return selected;
}

static bool account_menu(cui_context *ui, const cui_theme *t, starter_app *a, cui_rect viewport,
                         float dt) {
    cui_choice_item items[] = {
        {.id = STARTER_ACCOUNT_SETTINGS, .label = cui_string("Settings")},
        {.id = STARTER_SESSION,
         .label = cui_string(a->signed_in ? "Sign out (demo)" : "Sign in (demo)")}};
    cui_id action = 0;
    if (!cui_menu(ui, STARTER_ACCOUNT, t,
                  cui_string(a->signed_in ? "Ada / account" : "Guest / account"), items, 2, &action,
                  &a->account,
                  (cui_choice_popup_options){
                      .viewport = viewport, .width = 200, .side = CUI_POPUP_ABOVE, .dt = dt}))
        return false;
    if (action == STARTER_ACCOUNT_SETTINGS) {
        navigate(a, STARTER_SETTINGS);
        return true;
    }
    if (action == STARTER_SESSION) {
        a->signed_in = !a->signed_in;
        emit(a, STARTER_SESSION_CHANGED);
    }
    return false;
}

typedef struct {
    starter_app *app;
    cui_rect viewport;
    float dt;
} starter_sidebar_content;

static void sidebar_header(cui_context *ui, const cui_theme *t, bool collapsed, void *user) {
    (void)collapsed;
    (void)user;
    label(ui, t, "My native app");
}

static void sidebar_links(cui_context *ui, const cui_theme *t, bool collapsed, void *user) {
    (void)collapsed;
    (void)navigation(ui, t, user);
}

static void sidebar_account(cui_context *ui, const cui_theme *t, bool collapsed, void *user) {
    (void)collapsed;
    const starter_sidebar_content *content = user;
    (void)account_menu(ui, t, content->app, content->viewport, content->dt);
}

static void sidebar(cui_context *ui, const cui_theme *t, starter_app *a, cui_rect viewport,
                    float dt) {
    /* Sidebar owns body scrolling and keeps the footer outside the clip. */
    starter_sidebar_content content = {a, viewport, dt};
    cui_sidebar_entry links = {.id = 12,
                               .render = sidebar_links,
                               .user = a,
                               .estimated_height = 3 * t->control_height + 16};
    cui_sidebar(ui, STARTER_SIDEBAR, t, &a->sidebar, &links, 1,
                (cui_sidebar_options){.width = sidebar_width,
                                      .height = viewport.h,
                                      .label = cui_string("Workspace"),
                                      .collapse = CUI_SIDEBAR_NONE,
                                      .header = sidebar_header,
                                      .footer = sidebar_account,
                                      .footer_user = &content,
                                      .custom_box = true,
                                      .box = {.background = t->surface, .border = t->border}});
}

static void close_drawer(cui_context *ui, starter_app *a) {
    cui_overlay_close(ui, &a->account.overlay);
    cui_surface_close(ui, &a->drawer, CUI_SURFACE_CLOSED);
}

static void navigation_drawer(cui_context *ui, const cui_theme *t, starter_app *a,
                              cui_rect viewport, float dt) {
    float width = fminf(drawer_width, viewport.w);
    /* Choice popups use layer 100, above the sheet's panel at layer 81. */
    bool open = cui_surface_begin(ui, t, &a->drawer, cui_string("Workspace"), (cui_str){0},
                                  (cui_surface_options){.viewport = viewport,
                                                        .width = width,
                                                        .edge = CUI_SHEET_LEFT,
                                                        .layer = 80,
                                                        .no_sheet_resize = true});
    if (!open) {
        if (a->account.overlay.open && !a->drawer.overlay.open) {
            cui_overlay_close(ui, &a->account.overlay);
            cui_focus_restore(ui, STARTER_MENU);
        }
        return;
    }
    bool selected = navigation(ui, t, a);
    cui_surface_footer_begin(ui, t);
    /* Keep the account control in the fixed footer even when links scroll. */
    selected |= account_menu(ui, t, a, (cui_rect){0, 0, width, viewport.h}, dt);
    cui_surface_end(ui);
    if (selected)
        close_drawer(ui, a);
}

static void main_content(cui_context *ui, const cui_theme *t, starter_app *a, cui_rect viewport,
                         cui_responsive window, bool wide, const cui_platform *clipboard) {
    float width = viewport.w - (wide ? sidebar_width : 0);
    float padding = fminf(width * .125f, cui_responsive_value(window, CUI_BREAKPOINT_MD, 16, 32));
    cui_begin_box(ui, STARTER_MAIN,
                  (cui_box){.width = width,
                            .fixed_width = true,
                            .height = -1,
                            .padding = padding,
                            .gap = wide ? 20 : 12,
                            .clip = true});
    if (!wide) {
        if (cui_button_ex(
                ui, STARTER_MENU, t, cui_string("Menu"),
                (cui_button_options){.variant = CUI_BUTTON_OUTLINE, .icon = cui_string("menu")}))
            cui_surface_open(ui, &a->drawer, STARTER_MENU);
        cui_set_semantic_details(
            ui, STARTER_MENU,
            (cui_semantic_details){.expanded = a->drawer.overlay.open, .controls = STARTER_DRAWER});
    }
    const char *titles[] = {"Home", "Projects", "Settings"};
    cui_text(ui, cui_string(titles[a->page]), 28, t->text);
    if (a->page == STARTER_HOME) {
        label(ui, t, "Welcome. This is a small C11 application using real CUI controls.");
        label(ui, t,
              wide ? "Choose a page on the left, or open your account below."
                   : "Open Menu to switch pages or manage your account.");
    } else if (a->page == STARTER_PROJECTS) {
        label(ui, t, "Example projects");
        label(ui, t, "Desktop app");
        label(ui, t, "Personal tools");
    } else {
        label(ui, t, "Display name");
        cui_input_ex(
            ui, STARTER_NAME, t, cui_string("Display name"), a->name_edit, &a->name_input,
            (cui_input_options){.width = fminf(300, width - 2 * padding), .clipboard = clipboard});
        cui_checkbox(ui, STARTER_NOTIFICATIONS, t, cui_string("Enable notifications"),
                     &a->notifications, false);
        if (cui_button(ui, STARTER_SAVE, t, cui_string("Save settings"), false)) {
            memcpy(a->saved_name, a->name, sizeof a->name);
            a->saves++;
            emit(a, STARTER_SAVED);
        }
        if (a->saves)
            label(ui, t, "Settings saved locally for this session.");
    }
    cui_end_box(ui);
}

cui_draw_list starter_app_frame(cui_context *ui, starter_app *a, float width, float height,
                                float dt, const cui_platform *clipboard) {
    cui_responsive window = cui_responsive_query(width, NULL);
    if (!a || !window.valid || !cui_begin_frame(ui, width, height, dt))
        return (cui_draw_list){0};
    bool wide = cui_responsive_at_least(window, CUI_BREAKPOINT_MD);
    if (wide && a->drawer.overlay.open)
        close_drawer(ui, a);
    if (!wide && !a->drawer.overlay.open)
        cui_overlay_close(ui, &a->account.overlay);
    /* Process nested dismissal before the drawer or its background controls. */
    cui_overlay_update(ui, &a->account.overlay);
    starter_page previous_page = a->page;
    bool previous_signed_in = a->signed_in;
    cui_theme t = cui_theme_light();
    cui_rect viewport = {0, 0, width, height};
    cui_begin_box(
        ui, STARTER_ROOT,
        (cui_box){
            .width = -1, .height = -1, .row = wide, .clip = true, .background = t.background});
    if (wide)
        sidebar(ui, &t, a, viewport, dt);
    main_content(ui, &t, a, viewport, window, wide, clipboard);
    cui_end_box(ui);
    if (!wide)
        navigation_drawer(ui, &t, a, viewport, dt);
    cui_draw_list list = cui_end_frame(ui);
    /* Some labels were submitted before navigation or account callbacks. */
    if (previous_page != a->page || previous_signed_in != a->signed_in)
        cui_invalidate(ui);
    return list;
}
