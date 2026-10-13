/* SPDX-License-Identifier: MIT */
#ifndef NATIVE_STARTER_APP_H
#define NATIVE_STARTER_APP_H
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
typedef enum { STARTER_HOME, STARTER_PROJECTS, STARTER_SETTINGS } starter_page;
typedef enum { STARTER_NAVIGATED, STARTER_SESSION_CHANGED, STARTER_SAVED } starter_event;
enum {
    STARTER_NAV_HOME = 101,
    STARTER_NAV_PROJECTS = 102,
    STARTER_NAV_SETTINGS = 103,
    STARTER_ACCOUNT = 110,
    STARTER_ACCOUNT_SETTINGS = 111,
    STARTER_SESSION = 112,
    STARTER_NAME = 120,
    STARTER_NOTIFICATIONS = 121,
    STARTER_SAVE = 122
};
typedef struct starter_app starter_app;
typedef void (*starter_callback)(void *user, starter_event event, const starter_app *app);
struct starter_app {
    starter_page page;
    bool signed_in, notifications;
    unsigned saves;
    char name[80], saved_name[80];
    cui_edit *name_edit;
    cui_input_state name_input;
    cui_choice_popup account;
    starter_callback callback;
    void *user;
};
bool starter_app_init(starter_app *, starter_callback, void *user);
void starter_app_destroy(starter_app *);
cui_draw_list starter_app_frame(cui_context *, starter_app *, float width, float height, float dt,
                                const cui_platform *clipboard);
#endif
