/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <assert.h>
#include <string.h>
static unsigned calls[3];
static float width = 960, height = 640;
static void changed(void *user, starter_event event, const starter_app *app) {
    assert(user == calls && app);
    calls[event]++;
}
static void frame(cui_context *ui, starter_app *a) {
    cui_draw_list list = starter_app_frame(ui, a, width, height, .016f, NULL);
    assert(list.count > 10 && cui_last_error(ui) == CUI_OK);
}
static void tap(cui_context *ui, starter_app *a, float x, float y, cui_pointer_type pointer) {
    starter_page previous_page = a->page;
    bool previous_signed_in = a->signed_in;
    cui_event event = {
        .type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y, .pointer_type = pointer, .pointer_id = 1};
    assert(cui_queue_event(ui, event));
    frame(ui, a);
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, event));
    frame(ui, a);
    if (previous_page != a->page || previous_signed_in != a->signed_in)
        assert(cui_needs_redraw(ui)); /* Repaint labels submitted before callbacks. */
    frame(ui, a);
}
static void click(cui_context *ui, starter_app *a, cui_id id) {
    cui_rect r = cui_bounds(ui, id);
    assert(r.w > 0 && r.h > 0);
    tap(ui, a, r.x + r.w / 2, r.y + r.h / 2, CUI_POINTER_MOUSE);
}
static void key(cui_context *ui, starter_app *a, cui_key code) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code}));
    frame(ui, a);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = code}));
    frame(ui, a);
}
static void assert_fits(cui_rect bounds) {
    assert(bounds.w > 0 && bounds.h > 0 && bounds.x >= 0 && bounds.y >= 0 &&
           bounds.x + bounds.w <= width && bounds.y + bounds.h <= height);
}
static void responsive(cui_context *ui, starter_app *a) {
    assert(a->page == STARTER_SETTINGS && !a->signed_in);
    assert(cui_edit_replace_range(a->name_edit, 0, 5, cui_string("Uncommitted")) == CUI_EDIT_OK);
    /* Cross the boundary with an open desktop popup. */
    click(ui, a, STARTER_ACCOUNT);
    assert(a->account.overlay.open);
    cui_focus(ui, STARTER_SESSION);
    assert(cui_focus_within(ui, a->account.overlay.id));
    const float widths[] = {320, 767, 768, 1280, 360, 960, 224, 360};
    for (size_t i = 0; i < sizeof widths / sizeof widths[0]; ++i) {
        width = widths[i];
        frame(ui, a);
        if (i == 0)
            assert(cui_focused(ui) == STARTER_MENU);
        cui_rect root = cui_bounds(ui, STARTER_ROOT), main = cui_bounds(ui, STARTER_MAIN);
        bool wide = width >= 768;
        assert(root.w == width && root.h == height);
        assert(main.x == (wide ? 220 : 0) && main.x + main.w == width);
        assert_fits(cui_bounds(ui, STARTER_NAME));
        assert(cui_bounds(ui, STARTER_NAME).w <= 300);
        if (wide) {
            cui_rect account = cui_bounds(ui, STARTER_ACCOUNT);
            assert(account.y + account.h > height - 20 && account.x + account.w <= 220);
            assert(cui_bounds(ui, STARTER_MENU).w == 0);
        } else {
            assert_fits(cui_bounds(ui, STARTER_MENU));
            assert(cui_bounds(ui, STARTER_ACCOUNT).w == 0);
            assert(cui_bounds(ui, STARTER_NAV_HOME).w == 0);
        }
        assert(a->page == STARTER_SETTINGS && !a->signed_in && !a->notifications &&
               !a->drawer.overlay.open && !a->account.overlay.open);
        assert(!strcmp(a->name, "Uncommitted") && !strcmp(a->saved_name, "Grace") && a->saves == 1);
    }

    cui_focus(ui, STARTER_MENU);
    key(ui, a, CUI_KEY_ENTER);
    assert(a->drawer.overlay.open);
    assert_fits(cui_bounds(ui, STARTER_DRAWER));
    cui_rect account = cui_bounds(ui, STARTER_DRAWER_ACCOUNT);
    assert_fits(account);
    assert(account.y + account.h > height - 20);
    click(ui, a, STARTER_DRAWER_ACCOUNT);
    assert(a->account.overlay.open && a->drawer.overlay.open);
    key(ui, a, CUI_KEY_ESCAPE);
    assert(!a->account.overlay.open && a->drawer.overlay.open);
    assert(cui_focused(ui) == STARTER_DRAWER_ACCOUNT);
    click(ui, a, STARTER_DRAWER_ACCOUNT);
    click(ui, a, STARTER_DRAWER_SESSION);
    assert(a->signed_in && !a->account.overlay.open && a->drawer.overlay.open);
    key(ui, a, CUI_KEY_ESCAPE);
    assert(!a->drawer.overlay.open && cui_focused(ui) == STARTER_MENU);
    assert(cui_bounds(ui, STARTER_DRAWER_ACCOUNT).w == 0);

    cui_rect menu = cui_bounds(ui, STARTER_MENU);
    tap(ui, a, menu.x + menu.w / 2, menu.y + menu.h / 2, CUI_POINTER_TOUCH);
    assert(a->drawer.overlay.open);
    cui_rect projects = cui_bounds(ui, STARTER_DRAWER_NAV_PROJECTS);
    assert_fits(projects);
    tap(ui, a, projects.x + projects.w / 2, projects.y + projects.h / 2, CUI_POINTER_TOUCH);
    assert(!a->drawer.overlay.open && a->page == STARTER_PROJECTS);
    unsigned navigations = calls[STARTER_NAVIGATED];
    tap(ui, a, projects.x + projects.w / 2, projects.y + projects.h / 2, CUI_POINTER_MOUSE);
    assert(a->page == STARTER_PROJECTS && calls[STARTER_NAVIGATED] == navigations);
    click(ui, a, STARTER_MENU);
    tap(ui, a, 350, 480, CUI_POINTER_MOUSE);
    assert(!a->drawer.overlay.open && cui_focused(ui) == STARTER_MENU);
    click(ui, a, STARTER_MENU);
    click(ui, a, STARTER_DRAWER_ACCOUNT);
    cui_focus(ui, STARTER_DRAWER_SESSION);
    assert(cui_focus_within(ui, a->account.overlay.id));
    width = 960;
    height = 190;
    frame(ui, a);
    assert(!a->drawer.overlay.open && !a->account.overlay.open && a->signed_in);
    assert(cui_focused(ui) == STARTER_ACCOUNT);
    assert(cui_bounds(ui, STARTER_DRAWER).w == 0 && cui_bounds(ui, STARTER_MENU).w == 0);
    account = cui_bounds(ui, STARTER_ACCOUNT);
    assert_fits(account);
    assert(account.y + account.h > height - 20);
    height = 640;
    frame(ui, a);
    click(ui, a, STARTER_NAV_SETTINGS);
    assert(!strcmp(a->name, "Uncommitted") && !strcmp(a->saved_name, "Grace") &&
           a->page == STARTER_SETTINGS);
}

static void breakpoint_input(cui_context *ui, starter_app *a) {
    cui_focus(ui, STARTER_NAME);
    width = 360;
    frame(ui, a);
    assert(cui_focused(ui) == STARTER_NAME);
    width = 960;
    frame(ui, a);
    assert(cui_focused(ui) == STARTER_NAME);

    cui_focus(ui, STARTER_NAV_SETTINGS);
    width = 360;
    frame(ui, a);
    assert(cui_focused(ui) == STARTER_MENU);
    width = 960;
    frame(ui, a);
    assert(cui_focused(ui) == STARTER_NAV_SETTINGS);

    unsigned navigations = calls[STARTER_NAVIGATED];
    const cui_pointer_type pointers[] = {CUI_POINTER_MOUSE, CUI_POINTER_TOUCH};
    for (size_t i = 0; i < sizeof pointers / sizeof pointers[0]; ++i) {
        width = 360;
        frame(ui, a);
        key(ui, a, CUI_KEY_ENTER);
        cui_rect link = cui_bounds(ui, STARTER_DRAWER_NAV_HOME);
        assert_fits(link);
        cui_event event = {.type = CUI_EVENT_POINTER_DOWN,
                           .x = link.x + link.w / 2,
                           .y = link.y + link.h / 2,
                           .pointer_type = pointers[i],
                           .pointer_id = 1};
        assert(cui_queue_event(ui, event));
        frame(ui, a);
        cui_focus(ui, STARTER_DRAWER_NAV_HOME);
        width = 960;
        frame(ui, a);
        assert(!a->drawer.overlay.open && cui_focused(ui) == STARTER_NAV_HOME);
        link = cui_bounds(ui, STARTER_NAV_HOME);
        assert_fits(link);
        event.type = CUI_EVENT_POINTER_UP;
        event.x = link.x + link.w / 2;
        event.y = link.y + link.h / 2;
        assert(cui_queue_event(ui, event));
        frame(ui, a);
        assert(a->page == STARTER_SETTINGS && calls[STARTER_NAVIGATED] == navigations);
    }
    click(ui, a, STARTER_NAV_HOME);
    assert(a->page == STARTER_HOME && calls[STARTER_NAVIGATED] == navigations + 1);
}

static void scroll_at(cui_context *ui, starter_app *a, cui_rect region) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = region.x + region.w / 2,
                                           .y = region.y + region.h / 2,
                                           .dy = -2}));
    frame(ui, a);
}

static void navigation_scroll(cui_context *ui, starter_app *a, bool drawer) {
    width = drawer ? 360 : 960;
    height = 132;
    frame(ui, a);
    if (drawer)
        click(ui, a, STARTER_MENU);
    frame(ui, a);
    const float *scroll = drawer ? &a->drawer.body_scroll : &a->sidebar.scroll.y;
    cui_rect viewport = drawer ? a->drawer.body_viewport : a->sidebar.viewport;
    cui_id account_id = drawer ? STARTER_DRAWER_ACCOUNT : STARTER_ACCOUNT;
    cui_id home_id = drawer ? STARTER_DRAWER_NAV_HOME : STARTER_NAV_HOME;
    cui_rect account = cui_bounds(ui, account_id), home = cui_bounds(ui, home_id);
    assert_fits(viewport);
    assert(viewport.y > 0 && account.y >= viewport.y + viewport.h);
    assert_fits(account);
    assert(home.y >= viewport.y && home.y < viewport.y + viewport.h && *scroll == 0);

    scroll_at(ui, a, account);
    scroll_at(ui, a, (cui_rect){viewport.x, 0, viewport.w, viewport.y});
    assert(*scroll == 0);
    scroll_at(ui, a, viewport);
    assert(*scroll > 0 && cui_bounds(ui, home_id).y == home.y - *scroll);
    assert(cui_bounds(ui, account_id).y == account.y);
    assert(cui_bounds(ui, account_id).h == account.h);

    /* Part of the scrolled menu sits behind the header, outside the hit clip. */
    cui_rect projects = cui_bounds(ui, drawer ? STARTER_DRAWER_NAV_PROJECTS : STARTER_NAV_PROJECTS);
    float clipped_top = projects.y > 0 ? projects.y : 0;
    float clipped_bottom =
        projects.y + projects.h < viewport.y ? projects.y + projects.h : viewport.y;
    assert(clipped_bottom > clipped_top);
    unsigned navigations = calls[STARTER_NAVIGATED];
    tap(ui, a, projects.x + projects.w / 2, (clipped_top + clipped_bottom) / 2, CUI_POINTER_MOUSE);
    assert(a->page == STARTER_HOME && calls[STARTER_NAVIGATED] == navigations);
    click(ui, a, account_id);
    assert(a->account.overlay.open);
}

static void short_settings(bool wide) {
    starter_app a;
    assert(starter_app_init(&a, changed, calls));
    a.page = STARTER_SETTINGS;
    width = wide ? 960 : 360;
    height = 148;
    cui_context *ui = cui_create((cui_config){.width = width, .height = height});
    assert(ui);
    frame(ui, &a);
    frame(ui, &a);
    cui_rect save = cui_bounds(ui, STARTER_SAVE);
    cui_rect menu = cui_bounds(ui, STARTER_MENU);
    assert(save.y + save.h > height);
    scroll_at(ui, &a, (cui_rect){width - 72, height - 62, 40, 40});
    scroll_at(ui, &a, (cui_rect){width - 72, height - 62, 40, 40});
    assert_fits(cui_bounds(ui, STARTER_SAVE));
    assert(cui_bounds(ui, STARTER_SAVE).y < save.y);
    if (!wide)
        assert_fits(cui_bounds(ui, STARTER_MENU));
    assert(cui_bounds(ui, STARTER_MENU).y == menu.y);
    unsigned saves = calls[STARTER_SAVED];
    click(ui, &a, STARTER_SAVE);
    assert(a.saves == 1 && calls[STARTER_SAVED] == saves + 1);

    height = 640;
    frame(ui, &a);
    assert(cui_bounds(ui, STARTER_SAVE).y == save.y);
    assert(cui_bounds(ui, cui_scrollbar_id(STARTER_PAGE_VIEWPORT, true)).w == 0);
    cui_focus(ui, STARTER_NAME);
    height = 148;
    frame(ui, &a);
    key(ui, &a, CUI_KEY_TAB);
    assert(cui_focused(ui) == STARTER_NOTIFICATIONS);
    key(ui, &a, CUI_KEY_TAB);
    assert(cui_focused(ui) == STARTER_SAVE);
    assert_fits(cui_bounds(ui, STARTER_SAVE));
    cui_rect viewport = cui_bounds(ui, STARTER_PAGE_VIEWPORT);
    cui_rect revealed = cui_bounds(ui, STARTER_SAVE);
    assert(revealed.y >= viewport.y && revealed.y + revealed.h <= viewport.y + viewport.h);
    key(ui, &a, CUI_KEY_ENTER);
    assert(a.saves == 2 && calls[STARTER_SAVED] == saves + 2);

    height = 640;
    frame(ui, &a);
    cui_focus(ui, 0);
    height = 148;
    frame(ui, &a);
    viewport = cui_bounds(ui, STARTER_PAGE_VIEWPORT);
    cui_event swipe = {.type = CUI_EVENT_POINTER_DOWN,
                       .x = viewport.x + viewport.w - 24,
                       .y = viewport.y + viewport.h - 8,
                       .pointer_type = CUI_POINTER_TOUCH,
                       .pointer_id = 5};
    assert(a.page_scroll.y == 0);
    assert(cui_queue_event(ui, swipe));
    frame(ui, &a);
    swipe.type = CUI_EVENT_POINTER_MOVE;
    swipe.y = viewport.y + 8;
    assert(cui_queue_event(ui, swipe));
    frame(ui, &a);
    swipe.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, swipe));
    frame(ui, &a);
    assert(a.page_scroll.y > 0 && a.saves == 2);
    assert(cui_bounds(ui, STARTER_MENU).y == menu.y);
    starter_app_destroy(&a);
    cui_destroy(ui);
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
    responsive(ui, &a);
    breakpoint_input(ui, &a);
    navigation_scroll(ui, &a, false);
    navigation_scroll(ui, &a, true);
    starter_app_destroy(&a);
    cui_destroy(ui);
    short_settings(true);
    short_settings(false);
    return 0;
}
