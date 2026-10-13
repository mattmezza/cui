/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <assert.h>
#include <string.h>
static unsigned calls[3];
static void changed(void *user, starter_event event, const starter_app *app) {
    assert(user == calls && app);
    calls[event]++;
}
static void frame(cui_context *ui, starter_app *a) {
    cui_draw_list list = starter_app_frame(ui, a, 960, 640, .016f, NULL);
    assert(list.count > 10 && cui_last_error(ui) == CUI_OK);
}
static void click(cui_context *ui, starter_app *a, cui_id id) {
    cui_rect r = cui_bounds(ui, id);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
    frame(ui, a);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
    frame(ui, a);
    frame(ui, a);
}
int main(void) {
    starter_app a;
    assert(starter_app_init(&a, changed, calls));
    cui_context *ui = cui_create((cui_config){.width = 960, .height = 640});
    assert(ui);
    frame(ui, &a);
    cui_rect account = cui_bounds(ui, STARTER_ACCOUNT);
    assert(account.y > 540 && account.x < 220); /* Bottom sidebar, not top bar. */
    click(ui, &a, STARTER_NAV_PROJECTS);
    assert(a.page == STARTER_PROJECTS && calls[0] == 1);
    click(ui, &a, STARTER_NAV_SETTINGS);
    assert(a.page == STARTER_SETTINGS && calls[0] == 2);
    click(ui, &a, STARTER_NOTIFICATIONS);
    assert(!a.notifications);
    assert(cui_edit_replace_range(a.name_edit, 0, 3, cui_string("Grace")) == CUI_EDIT_OK);
    click(ui, &a, STARTER_SAVE);
    assert(a.saves == 1 && !strcmp(a.saved_name, "Grace") && calls[2] == 1);
    click(ui, &a, STARTER_ACCOUNT);
    assert(a.account.overlay.open);
    click(ui, &a, STARTER_SESSION);
    assert(a.signed_in && !a.account.overlay.open && calls[1] == 1);
    click(ui, &a, STARTER_ACCOUNT);
    click(ui, &a, STARTER_SESSION);
    assert(!a.signed_in && calls[1] == 2);
    click(ui, &a, STARTER_NAV_HOME);
    assert(a.page == STARTER_HOME);
    click(ui, &a, STARTER_ACCOUNT);
    click(ui, &a, STARTER_ACCOUNT_SETTINGS);
    assert(a.page == STARTER_SETTINGS && !a.account.overlay.open);
    click(ui, &a, STARTER_ACCOUNT);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    frame(ui, &a);
    assert(!a.account.overlay.open);
    starter_app_destroy(&a);
    cui_destroy(ui);
    return 0;
}
