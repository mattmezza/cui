/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
static cui_context *ui;
static dashboard_app app;
static float width;
static unsigned navigations, accounts;
static void callback(void *user, dashboard_event event, unsigned page, bool signed_in) {
    CHECK(user == &app && page == app.page && signed_in == app.signed_in);
    if (event == DASH_NAVIGATED) ++navigations;
    else ++accounts;
}
static void frame(void) {
    CHECK(cui_begin_frame(ui, 1500, 800, .016f));
    dashboard_scene(ui, &app, width, 760);
    cui_draw_list list = cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK && list.count > 0);
    cui_rect root = cui_bounds(ui, DASH_ROOT), main = cui_bounds(ui, DASH_MAIN);
    CHECK(fabsf(root.w - width) < 1 && main.w > 0);
    CHECK(main.x >= root.x && main.x + main.w <= root.x + root.w + 1);
    for (size_t i = 0; i < list.count; ++i)
        CHECK(isfinite(list.commands[i].bounds.x) && isfinite(list.commands[i].bounds.y));
}
static void click_at(float x, float y) {
    cui_event down = {.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y};
    cui_event up = down;
    up.type = CUI_EVENT_POINTER_UP;
    CHECK(cui_queue_event(ui, down) && cui_queue_event(ui, up));
    frame(); frame();
}
static void click(cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    CHECK(b.w > 0 && b.h > 0 && b.x + b.w <= width + 1);
    click_at(b.x + b.w / 2, b.y + b.h / 2);
}
int main(void) {
    ui = cui_create((cui_config){.width = 1500, .height = 800});
    CHECK(ui);
    dashboard_init(&app, callback, &app);
    const float widths[] = {360, 800, 1400, 360, 1024, 767, 1280, 800, 360};
    for (size_t i = 0; i < sizeof widths / sizeof widths[0]; ++i) {
        width = widths[i]; frame(); frame();
        cui_rect main = cui_bounds(ui, DASH_MAIN), a = cui_bounds(ui, DASH_SUMMARY);
        cui_rect b = cui_bounds(ui, DASH_ACTIVITY);
        CHECK(a.w > 0 && b.w > 0);
        CHECK(a.x >= main.x && b.x + b.w <= main.x + main.w + 1);
        CHECK(a.y + a.h <= b.y + 1 || a.x + a.w <= b.x + 1);
        if (width < 532 + 24) CHECK(b.y >= a.y + a.h);
        else CHECK(b.x >= a.x + a.w && fabsf(a.y - b.y) < 1);
        if (width >= 1024) {
            cui_rect nav = cui_bounds(ui, DASH_NAV);
            CHECK(main.x >= nav.x + nav.w);
        }
    }
    width = 360; frame(); frame();
    click(DASH_MENU);
    CHECK(app.drawer_open);
    cui_rect hidden = cui_bounds(ui, DASH_REPORTS);
    click(DASH_REPORTS);
    CHECK(app.page == 1 && navigations == 1 && !app.drawer_open);
    click_at(hidden.x + hidden.w / 2, hidden.y + hidden.h / 2);
    CHECK(navigations == 1);
    click(DASH_ACCOUNT);
    CHECK(app.signed_in && accounts == 1);
    width = 1400; frame(); frame();
    click(DASH_HOME);
    CHECK(app.page == 0 && navigations == 2 && app.signed_in);
    width = 800; frame(); frame();
    click(DASH_MENU);
    CHECK(app.drawer_open);
    frame();
    cui_rect nav = cui_bounds(ui, DASH_NAV), main = cui_bounds(ui, DASH_MAIN);
    CHECK(main.y >= nav.y + nav.h);
    /* Crossing while open preserves the same disclosure/account/page state. */
    width = 1400; frame(); frame();
    CHECK(app.drawer_open && app.signed_in && app.page == 0);
    width = 360; frame(); frame();
    CHECK(app.drawer_open && app.signed_in && app.page == 0);
    nav = cui_bounds(ui, DASH_NAV); main = cui_bounds(ui, DASH_MAIN);
    CHECK(main.y >= nav.y + nav.h);
    click(DASH_MENU);
    CHECK(!app.drawer_open && accounts == 1 && navigations == 2);
    cui_destroy(ui);
    return 0;
}
