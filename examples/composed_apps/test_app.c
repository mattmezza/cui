/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_scroll.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef APP_THEME_NATIVE
#include "cui/cui_sdl3.h"
#endif
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
static cui_draw_list last_list;
static void frame(void) {
    cui_draw_list list = composed_app_frame(ui, app, width, height);
    last_list = list;
    assert(cui_last_error(ui) == CUI_OK && list.count > (height >= 800 ? 30u : 10u));
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
static void workflow(const cui_theme *theme) {
    width = 1200;
    height = 900;
    events counts = {0};
    ui = cui_create((cui_config){.width = width, .height = height});
    app = composed_app_create(callback, &counts);
    assert(ui && app);
    composed_app_set_theme(app, theme);
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
}
static bool same_color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static const cui_draw_command *paint(cui_draw_type type, cui_rect bounds) {
    for (size_t i = 0; i < last_list.count; ++i) {
        const cui_draw_command *cmd = &last_list.commands[i];
        if (cmd->type == type && fabsf(cmd->bounds.x - bounds.x) < .1f &&
            fabsf(cmd->bounds.y - bounds.y) < .1f && fabsf(cmd->bounds.w - bounds.w) < .1f &&
            fabsf(cmd->bounds.h - bounds.h) < .1f)
            return cmd;
    }
    return NULL;
}
static const cui_draw_command *label(const char *name) {
    size_t length = strlen(name);
    for (size_t i = last_list.count; i > 0; --i) {
        const cui_draw_command *cmd = &last_list.commands[i - 1];
        if (cmd->type == CUI_DRAW_TEXT && cmd->text.length >= length &&
            !memcmp(cmd->text.data, name, length))
            return cmd;
    }
    return NULL;
}
static double luminance(cui_color c) {
    double channels[] = {c.r / 255.0, c.g / 255.0, c.b / 255.0};
    for (unsigned i = 0; i < 3; ++i)
        channels[i] =
            channels[i] <= .04045 ? channels[i] / 12.92 : pow((channels[i] + .055) / 1.055, 2.4);
    return .2126 * channels[0] + .7152 * channels[1] + .0722 * channels[2];
}
static void readable(cui_color ink, cui_color background) {
    double a = luminance(ink), b = luminance(background);
    assert((fmax(a, b) + .05) / (fmin(a, b) + .05) >= 4.5);
}
static void theme_render(const cui_theme *theme) {
    width = 1200;
    height = 900;
    ui = cui_create((cui_config){.width = width, .height = height});
    app = composed_app_create(NULL, NULL);
    assert(ui && app);
    cui_theme supplied = *theme;
    composed_app_set_theme(app, &supplied);
    supplied.surface = (cui_color){255, 0, 255, 255};
    supplied.text = (cui_color){0, 255, 0, 255}; /* Setter must copy, not borrow this struct. */
    frame();
    const cui_draw_command *header = paint(CUI_DRAW_RECT, cui_bounds(ui, 20));
    const cui_draw_command *hero = paint(CUI_DRAW_RECT, cui_bounds(ui, 30));
    const cui_draw_command *heading = label("Overview");
    assert(header && hero && heading);
    assert(same_color(header->color, theme->surface));
    assert(same_color(heading->color, theme->text));
    readable(heading->color, header->color);
    const cui_draw_command *hero_heading = label("A calmer place");
    assert(hero_heading && same_color(hero_heading->color, theme->text));
    readable(hero_heading->color, hero->color);
    cui_focus(ui, APP_HERO_CTA);
    frame();
    assert(cui_focused(ui) == APP_HERO_CTA);
    const cui_draw_command *focus = paint(CUI_DRAW_BORDER, cui_bounds(ui, APP_HERO_CTA));
    assert(focus && same_color(focus->color, theme->focus));
    click(APP_HERO_CTA);
    const cui_draw_command *card = paint(CUI_DRAW_RECT, cui_bounds(ui, 40));
    const cui_draw_command *name = label("Starter");
    assert(card && name && same_color(card->color, theme->surface) &&
           same_color(name->color, theme->text));
    readable(name->color, card->color);
    click(APP_PLAN_TEAM);
    cui_focus(ui, APP_PLAN_TEAM);
    frame();
    composed_app_set_theme(app, NULL);
    frame();
    assert(composed_app_snapshot(app).plan == 1 && cui_focused(ui) == APP_PLAN_TEAM);
    card = paint(CUI_DRAW_RECT, cui_bounds(ui, 40));
    assert(card && same_color(card->color, cui_theme_light().surface));
    composed_app_set_theme(app, theme);
    frame();
    assert(composed_app_snapshot(app).plan == 1 && cui_focused(ui) == APP_PLAN_TEAM);
    click(APP_NAV_SETTINGS);
    card = paint(CUI_DRAW_RECT, cui_bounds(ui, 50));
    name = label("Profile & preferences");
    assert(card && name && same_color(card->color, theme->surface) &&
           same_color(name->color, theme->text));
    readable(name->color, card->color);
    composed_app_destroy(app);
    cui_destroy(ui);
}
static const cui_semantic *semantic(cui_id id) {
    size_t count = 0;
    const cui_semantic *items = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (items[i].id == id)
            return &items[i];
    return NULL;
}
static bool fully_visible(cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    return b.w > 0 && b.h > 0 && b.x >= 0 && b.x + b.w <= width && b.y >= 72 && b.y + b.h <= height;
}
static void narrow_pricing_scroll(const cui_theme *theme, float w, float h) {
    width = w;
    height = h;
    ui = cui_create((cui_config){.width = width, .height = height});
    app = composed_app_create(NULL, NULL);
    assert(ui && app);
    composed_app_set_theme(app, theme);
    frame();
    click(APP_MENU);
    click(APP_NAV_PRICING);
    assert(composed_app_snapshot(app).page == APP_PRICING);
    assert(semantic(APP_PLAN_TEAM)); /* Raw semantic exists even below the visible viewport. */
    assert(!fully_visible(APP_PLAN_TEAM));
    unsigned steps = 0;
    while (!fully_visible(APP_PLAN_TEAM) && steps++ < 20) {
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = width / 2,
                                               .y = height / 2,
                                               .dy = -96,
                                               .scroll_precise = true}));
        frame();
        frame();
    }
    assert(fully_visible(APP_PLAN_TEAM));
    click(APP_PLAN_TEAM);
    assert(composed_app_snapshot(app).plan == 1);
    /* Keyboard End on the actual scrollbar must reveal the final Studio CTA. */
    cui_id bar = cui_scrollbar_id(APP_CONTENT, true);
    const cui_semantic *scrollbar = semantic(bar);
    assert(scrollbar && scrollbar->role == CUI_ROLE_SCROLLBAR && !scrollbar->disabled &&
           scrollbar->details.maximum > 0);
    cui_focus(ui, bar);
    key(CUI_KEY_END, 0);
    frame();
    assert(fully_visible(APP_PLAN_STUDIO));
    click(APP_PLAN_STUDIO);
    assert(composed_app_snapshot(app).plan == 2);
    /* Inverse pixel scrolling returns to the top, and wheel units work as well. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = width / 2,
                                           .y = height / 2,
                                           .dy = 10000,
                                           .scroll_precise = true}));
    frame();
    frame();
    float top = cui_bounds(ui, APP_PLAN_STARTER).y;
    assert(!fully_visible(APP_PLAN_TEAM));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = width / 2, .y = height / 2, .dy = -1}));
    frame();
    frame();
    assert(fabsf(cui_bounds(ui, APP_PLAN_STARTER).y - (top - 48)) < .1f);
    composed_app_destroy(app);
    cui_destroy(ui);
}
#ifdef APP_THEME_NATIVE
static void native_callback(void *user, app_event event, app_snapshot state) {
    callback(user, event, state);
    printf("native event=%d page=%d plan=%d session=%d saves=%u name=%s notifications=%d sync=%d\n",
           event, state.page, state.plan, state.logged_in, state.saves, state.display_name,
           state.notifications, state.sync);
    fflush(stdout);
}
/* Optional native audit driver. Its only platform-facing types stay in this test. */
static int native_audit(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: test_theme_native --native FONT light|dark|contrast|terminal "
                        "OUTPUT_DIRECTORY\n");
        return 2;
    }
    cui_theme theme;
    if (!strcmp(argv[1], "light"))
        theme = cui_theme_light();
    else if (!strcmp(argv[1], "dark"))
        theme = cui_theme_dark();
    else if (!strcmp(argv[1], "contrast"))
        theme = cui_theme_high_contrast();
    else if (!strcmp(argv[1], "terminal"))
        theme = cui_theme_terminal();
    else
        return 2;
    cui_font *font = cui_font_open(argv[0]);
    if (!font)
        return 3;
    cui_sdl3 *backend = cui_sdl3_create("Northstar / theme audit", 1200, 900, font);
    if (!backend) {
        cui_font_close(font);
        return 4;
    }
    ui = cui_create((cui_config){.width = 1200,
                                 .height = 900,
                                 .measure = cui_font_measure,
                                 .measure_styled = cui_font_measure_styled,
                                 .font_metrics = cui_font_get_metrics,
                                 .measure_user = font});
    events counts = {0};
    app = composed_app_create(native_callback, &counts);
    if (!ui || !app) {
        composed_app_destroy(app);
        cui_destroy(ui);
        cui_sdl3_destroy(backend);
        cui_font_close(font);
        return 5;
    }
    composed_app_set_theme(app, &theme);
    int result = 0;
    app_page previous = APP_OVERVIEW;
    unsigned age = 0, completed_frames = 0;
    bool captured[3] = {false, false, false};
    const char *pages[] = {"overview", "pricing", "settings"};
    for (unsigned tick = 0; tick < 1200 && cui_sdl3_poll(backend, ui); ++tick) {
        float w, h;
        cui_sdl3_size(backend, &w, &h);
        cui_draw_list list = composed_app_frame(ui, app, w, h);
        app_snapshot state = composed_app_snapshot(app);
        if (state.page != previous) {
            previous = state.page;
            age = 0;
        } else
            ++age;
        char path[1024];
        bool capture = age >= 3 && !captured[state.page];
        bool complete = state.page == APP_SETTINGS && state.plan == 1 && !state.logged_in &&
                        state.saves == 1 && !strcmp(state.saved_name, "Grace Hopper") &&
                        !state.notifications && state.sync && counts.plans == 1 &&
                        counts.sessions == 2 && counts.saves == 1 && captured[0] && captured[1] &&
                        captured[2];
        completed_frames = complete ? completed_frames + 1 : 0;
        bool final = completed_frames >= 3;
        snprintf(path, sizeof path, "%s/%s-%s.bmp", argv[2], argv[1],
                 final ? "settings-saved" : pages[state.page]);
        bool painted = capture || final
                           ? cui_sdl3_render_capture(backend, &list, theme.background, path)
                           : cui_sdl3_render(backend, &list, theme.background);
        if (!painted || cui_last_error(ui) != CUI_OK) {
            result = 6;
            break;
        }
        if (capture)
            captured[state.page] = true;
        if (tick % 8 == 0) {
            const cui_id ids[] = {APP_MENU,          APP_ACCOUNT,      APP_HERO_CTA,
                                  APP_PLAN_TEAM,     APP_NAV_SETTINGS, APP_NAME,
                                  APP_NOTIFICATIONS, APP_SYNC,         APP_SAVE};
            for (size_t i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
                cui_rect b = cui_bounds(ui, ids[i]);
                if (b.w > 0 && b.h > 0)
                    printf("bound id=%u x=%.0f y=%.0f\n", (unsigned)ids[i], b.x + b.w / 2,
                           b.y + b.h / 2);
            }
            fflush(stdout);
        }
        if (final)
            break;
        cui_sdl3_wait(16);
    }
    app_snapshot state = composed_app_snapshot(app);
    if (state.plan != 1 || state.logged_in || state.saves != 1 ||
        strcmp(state.saved_name, "Grace Hopper") || state.notifications || !state.sync ||
        counts.plans != 1 || counts.sessions != 2 || counts.saves != 1 || !captured[0] ||
        !captured[1] || !captured[2])
        result = 7;
    composed_app_destroy(app);
    cui_destroy(ui);
    cui_sdl3_destroy(backend);
    cui_font_close(font);
    return result;
}
#endif
int main(int argc, char **argv) {
#ifdef APP_THEME_NATIVE
    if (argc > 1 && !strcmp(argv[1], "--native"))
        return native_audit(argc - 2, argv + 2);
#else
    (void)argc;
    (void)argv;
#endif
    workflow(NULL);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; ++i) {
        theme_render(&themes[i]);
        workflow(&themes[i]);
        narrow_pricing_scroll(&themes[i], 390, 844);
        narrow_pricing_scroll(&themes[i], 360, 420);
    }
    puts("composed app: default and four themes passed geometry, readable ink, copied palette, "
         "focus, navigation, session, plans, editing, validation, save and responsive scrolling");
    return 0;
}
