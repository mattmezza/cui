/* SPDX-License-Identifier: MIT */
#ifndef NATIVE_STARTER_APP_H
#define NATIVE_STARTER_APP_H
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
#include "cui/cui_resizable.h"
#include "cui/cui_scroll.h"
#include "cui/cui_sidebar.h"
#include "cui/cui_surface.h"
typedef enum { STARTER_HOME, STARTER_PROJECTS, STARTER_SETTINGS } starter_page;
typedef enum { STARTER_NAVIGATED, STARTER_SESSION_CHANGED, STARTER_SAVED } starter_event;
enum {
    STARTER_ROOT = 1,
    STARTER_SIDEBAR = 10,
    STARTER_DRAWER = 13,
    STARTER_MAIN = 20,
    STARTER_PAGE_VIEWPORT = 21,
    STARTER_PAGE_CONTENT = 22,
    STARTER_MENU = 100,
    STARTER_NAV_HOME = 101,
    STARTER_NAV_PROJECTS = 102,
    STARTER_NAV_SETTINGS = 103,
    STARTER_ACCOUNT = 110,
    STARTER_ACCOUNT_SETTINGS = 111,
    STARTER_SESSION = 112,
    STARTER_NAME = 120,
    STARTER_NOTIFICATIONS = 121,
    STARTER_SAVE = 122,
    STARTER_DRAWER_NAV_HOME = 201,
    STARTER_DRAWER_NAV_PROJECTS = 202,
    STARTER_DRAWER_NAV_SETTINGS = 203,
    STARTER_DRAWER_ACCOUNT = 210,
    STARTER_DRAWER_ACCOUNT_SETTINGS = 211,
    STARTER_DRAWER_SESSION = 212,
    STARTER_PROJECT_WIDE = 300,
    STARTER_PROJECT_COMPACT = 301,
    STARTER_PROJECT_LIST = 302,
    STARTER_PROJECT_DETAILS = 303,
    STARTER_PROJECT_DESKTOP = 304,
    STARTER_PROJECT_TOOLS = 305,
    STARTER_PROJECT_TABS = 310,
    STARTER_PROJECT_OVERVIEW = 311,
    STARTER_PROJECT_ACTIVITY = 312,
    STARTER_PROJECT_FILES = 313,
    STARTER_PROJECT_RELEASES = 314,
    STARTER_PROJECT_ACCESS = 315,
    STARTER_PROJECT_ALL_TABS = 316,
    STARTER_PROJECT_TAB_POPUP = 317,
    STARTER_PROJECT_TAB_COUNT = 5
};
typedef struct starter_app starter_app;
typedef void (*starter_callback)(void *user, starter_event event, const starter_app *app);
struct starter_app {
    starter_page page;
    bool signed_in, notifications;
    bool has_layout, wide_layout;
    unsigned saves;
    char name[80], saved_name[80];
    cui_edit *name_edit;
    cui_input_state name_input;
    cui_scroll_state page_scroll;
    cui_id page_focus;
    cui_sidebar_state sidebar;
    cui_sidebar_cache navigation_cache[1];
    cui_surface_state drawer;
    cui_choice_popup account;
    unsigned project_index;
    cui_id project_tab;
    cui_resizable_state project_split[2];
    cui_resizable_slot project_slots[2][2];
    cui_tab_scroll_state project_tab_scroll;
    cui_tab_overflow project_overflow;
    cui_menu_entry project_tab_entries[STARTER_PROJECT_TAB_COUNT];
    starter_callback callback;
    void *user;
};
bool starter_app_init(starter_app *, starter_callback, void *user);
void starter_app_destroy(starter_app *);
cui_draw_list starter_app_frame(cui_context *, starter_app *, float width, float height, float dt,
                                const cui_platform *clipboard);
#endif
