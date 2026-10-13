/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    unsigned navigation, sessions, plans, saves;
} events;
static void callback(void *user, app_event event, app_snapshot state) {
    events *counts = user;
    assert(state.display_name && state.notice);
    switch (event) {
    case APP_NAVIGATED:
        ++counts->navigation;
        break;
    case APP_SESSION_CHANGED:
        ++counts->sessions;
        break;
    case APP_PLAN_CHANGED:
        ++counts->plans;
        break;
    case APP_SETTINGS_SAVED:
        ++counts->saves;
        break;
    }
}
static cui_context *ui;
static composed_app *app;
static float width = 1200, height = 900;
static void frame(void) {
    cui_draw_list list = composed_app_frame(ui, app, width, height);
    assert(cui_last_error(ui) == CUI_OK && list.count > 30);
    for (size_t i = 0; i < list.count; ++i) {
        cui_rect b = list.commands[i].bounds;
        assert(isfinite(b.x) && isfinite(b.y) && isfinite(b.w) && isfinite(b.h));
    }
}
static void click(cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    assert(b.w > 0 && b.h > 0);
    assert(b.x >= 0 && b.x + b.w <= width + 1);
    assert(b.y >= 0 && b.y + b.h <= height + 1);
    cui_event down = {.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w / 2, .y = b.y + b.h / 2};
    cui_event up = down;
    up.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, down) && cui_queue_event(ui, up));
    frame();
    frame();
}
static void key(cui_key keycode, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = keycode, .modifiers = modifiers}));
    frame();
}
int main(void) {
    events counts = {0};
    ui = cui_create((cui_config){.width = width, .height = height});
    app = composed_app_create(callback, &counts);
    assert(ui && app);
    frame();
    assert(composed_app_snapshot(app).page == APP_OVERVIEW);
    assert(composed_app_snapshot(app).sidebar_visible);
    click(APP_MENU);
    assert(!composed_app_snapshot(app).sidebar_visible);
    click(APP_MENU);
    assert(composed_app_snapshot(app).sidebar_visible);
    click(APP_ACCOUNT);
    key(CUI_KEY_END, 0);
    key(CUI_KEY_ENTER, 0);
    frame();
    assert(composed_app_snapshot(app).logged_in && counts.sessions == 1);
    click(APP_HERO_CTA);
    assert(composed_app_snapshot(app).page == APP_PRICING);
    click(APP_PLAN_TEAM);
    assert(composed_app_snapshot(app).plan == 1 && counts.plans == 1);
    assert(strstr(composed_app_snapshot(app).notice, "No payment"));
    click(APP_PLAN_TEAM);
    assert(counts.plans == 1); /* Selected plan CTA does not dispatch twice. */
    click(APP_PLAN_STUDIO);
    assert(composed_app_snapshot(app).plan == 2 && counts.plans == 2);
    click(APP_NAV_SETTINGS);
    click(APP_NOTIFICATIONS);
    click(APP_SYNC);
    assert(!composed_app_snapshot(app).notifications && composed_app_snapshot(app).sync);
    click(APP_NAME);
    key(CUI_KEY_A, CUI_MOD_CTRL);
    key(CUI_KEY_BACKSPACE, 0);
    assert(strlen(composed_app_snapshot(app).display_name) == 0);
    click(APP_SAVE);
    assert(composed_app_snapshot(app).invalid_name && counts.saves == 0);
    click(APP_NAME);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "   "}));
    frame();
    click(APP_SAVE);
    assert(composed_app_snapshot(app).invalid_name && counts.saves == 0);
    assert(strcmp(composed_app_snapshot(app).saved_name, "Ada Lovelace") == 0);
    click(APP_NAME);
    key(CUI_KEY_A, CUI_MOD_CTRL);
    key(CUI_KEY_BACKSPACE, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Grace Hopper"}));
    frame();
    click(APP_SAVE);
    assert(!composed_app_snapshot(app).invalid_name);
    assert(strcmp(composed_app_snapshot(app).saved_name, "Grace Hopper") == 0);
    assert(counts.saves == 1 && composed_app_snapshot(app).saves == 1);
    click(APP_ACCOUNT);
    key(CUI_KEY_END, 0);
    key(CUI_KEY_ENTER, 0);
    frame();
    assert(!composed_app_snapshot(app).logged_in && counts.sessions == 2);
    width = 390;
    height = 844;
    frame();
    frame();
    assert(!composed_app_snapshot(app).sidebar_visible);
    click(APP_ACCOUNT);
    key(CUI_KEY_END, 0);
    key(CUI_KEY_ENTER, 0);
    frame();
    assert(composed_app_snapshot(app).logged_in);
    cui_rect account = cui_bounds(ui, APP_ACCOUNT);
    assert(account.x >= 0 && account.x + account.w <= width);
    click(APP_ACCOUNT);
    key(CUI_KEY_END, 0);
    key(CUI_KEY_ENTER, 0);
    frame();
    assert(!composed_app_snapshot(app).logged_in && counts.sessions == 4);
    click(APP_MENU);
    assert(composed_app_snapshot(app).sidebar_visible);
    click(APP_NAV_OVERVIEW);
    assert(composed_app_snapshot(app).page == APP_OVERVIEW);
    assert(!composed_app_snapshot(app).sidebar_visible);
    click(APP_HERO_CTA);
    assert(composed_app_snapshot(app).page == APP_PRICING);
    /* Pricing cards stack, rather than becoming squeezed horizontal columns. */
    cui_rect first = cui_bounds(ui, APP_PLAN_STARTER), second = cui_bounds(ui, APP_PLAN_TEAM);
    assert(first.w > 250 && second.y > first.y + first.h);
    float before = first.y;
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 200, .y = 500, .dy = -8}));
    frame();
    frame();
    assert(cui_bounds(ui, APP_PLAN_STARTER).y < before);
    width = 1200;
    height = 900;
    frame();
    frame();
    assert(composed_app_snapshot(app).sidebar_visible);
    assert(counts.navigation >= 4);
    composed_app_destroy(app);
    cui_destroy(ui);
    puts("composed app: navigation, session, plans, editing, validation, save and responsive "
         "scrolling passed");
    return 0;
}
