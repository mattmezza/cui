/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_sdl3.h"
#include "cui/cui_sidebar.h"
#include "cui/cui_theme_registry.h"
#ifndef CUI_SYSTEM_THEME_TEST
#include <SDL3/SDL.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Retain at a stable address: registry borrows this inline entry storage. */
typedef struct {
    cui_theme_registry_entry entries[2];
    cui_theme_registry registry;
    cui_theme_host_state resolved;
    cui_theme_host_preferences preferences;
    cui_theme_selection selection;
    uint64_t observation_revision;
    cui_system_appearance observed;
} system_theme_app;
enum { APP_FOLLOW = 101, APP_LIGHT, APP_DARK };
static bool app_resolve(system_theme_app *app) {
    return cui_theme_host_update(&app->resolved, &app->registry, app->selection, app->preferences,
                                 16) == CUI_THEME_REGISTRY_OK;
}
static bool app_init(system_theme_app *app) {
    memset(app, 0, sizeof *app);
    if (!cui_theme_registry_init(&app->registry, app->entries, 2, false))
        return false;
    /* Actual legacy ThemeSet import; strings and scratch need not survive. */
    const char *json = "{\"themes\":[{\"name\":\"Native Light\",\"mode\":\"light\","
                       "\"colors\":{\"primary.background\":\"blue-600\"}},"
                       "{\"name\":\"Native Dark\",\"mode\":\"dark\","
                       "\"colors\":{\"primary.background\":\"blue-400\"}}]}";
    cui_theme_registry_entry scratch[2];
    cui_theme_import_result report;
    if (cui_theme_registry_import(&app->registry, cui_string(json), (cui_theme_import_options){0},
                                  scratch, 2, &report) != 0)
        return false;
    app->selection =
        (cui_theme_selection){CUI_THEME_FOLLOW_SYSTEM, {"Native Light", 12}, {"Native Dark", 11}};
    app->preferences.revision = 1;
    /* Unknown initial observation deliberately falls back to light. */
    return app_resolve(app);
}
static bool app_observe(system_theme_app *app, cui_platform_appearance observation) {
    if (observation.appearance < CUI_SYSTEM_APPEARANCE_UNKNOWN ||
        observation.appearance > CUI_SYSTEM_APPEARANCE_DARK ||
        !(observation.capabilities & CUI_APPEARANCE_SYSTEM_THEME) || !observation.revision ||
        observation.revision < app->observation_revision)
        return false;
    if (observation.revision == app->observation_revision &&
        (observation.revision != UINT64_MAX || observation.appearance == app->observed))
        return true;
    if (app->preferences.revision == UINT64_MAX)
        return false;
    system_theme_app candidate = *app;
    candidate.observation_revision = observation.revision;
    candidate.observed = observation.appearance;
    /* Later UNKNOWN retains last known preference, never invents an OS answer. */
    if (observation.appearance != CUI_SYSTEM_APPEARANCE_UNKNOWN)
        candidate.preferences.dark = observation.appearance == CUI_SYSTEM_APPEARANCE_DARK;
    candidate.preferences.revision++;
    if (!app_resolve(&candidate))
        return false;
    /* Preserve registry's stable original storage binding. */
    app->observation_revision = candidate.observation_revision;
    app->observed = candidate.observed;
    app->preferences = candidate.preferences;
    app->resolved = candidate.resolved;
    return true;
}
static bool app_choose(system_theme_app *app, cui_theme_mode mode) {
    if (mode < CUI_THEME_FOLLOW_SYSTEM || mode > CUI_THEME_FORCE_DARK)
        return false;
    if (app->selection.mode == mode)
        return true;
    if (app->preferences.revision == UINT64_MAX)
        return false;
    cui_theme_selection old = app->selection;
    app->selection.mode = mode;
    app->preferences.revision++;
    if (app_resolve(app))
        return true;
    app->selection = old;
    app->preferences.revision--;
    return false;
}
static cui_draw_list app_frame(cui_context *ui, system_theme_app *app, float w, float h, float dt) {
    if (!cui_begin_frame(ui, w, h, dt))
        return (cui_draw_list){0};
    cui_theme theme = app->resolved.theme;
    cui_begin_box(
        ui, 1, (cui_box){.width = -1, .height = -1, .row = true, .background = theme.background});
    cui_begin_box(
        ui, 2,
        (cui_box){
            .width = 230, .height = -1, .padding = 20, .gap = 16, .background = theme.surface});
    cui_text(ui, cui_string("Appearance"), 22, theme.text);
    const char *names[] = {"Follow system", "Always light", "Always dark"};
    for (unsigned i = 0; i < 3; i++) {
        if (cui_button(ui, APP_FOLLOW + i, &theme, cui_string(names[i]), false)) {
            if (app_choose(app, (cui_theme_mode)i))
                cui_invalidate(ui);
        }
    }
    cui_sidebar_menu_item item = {
        .id = 201, .label = cui_string("Native theme demo"), .active = true};
    cui_sidebar_menu(ui, 200, &theme, &item, 1, false);
    cui_end_box(ui);
    cui_begin_box(ui, 3, (cui_box){.width = -1, .height = -1, .padding = 32, .gap = 20});
    cui_text(ui, cui_string(names[app->selection.mode]), 30, theme.text);
    cui_text(
        ui, cui_string(app->resolved.theme.dark ? "Active palette: dark" : "Active palette: light"),
        theme.font_size, theme.text);
    const char *observed[] = {"System appearance unknown (light initially; then retain last known)",
                              "System reports light", "System reports dark"};
    cui_text(ui, cui_string(observed[app->observed]), theme.font_size, theme.muted);
    cui_text(ui, cui_string("Explicit choices survive system notifications."), theme.font_size,
             theme.text);
    cui_end_box(ui);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
#ifndef CUI_SYSTEM_THEME_TEST
int main(int argc, char **argv) {
    const char *font_path = NULL, *capture = NULL;
    unsigned limit = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--font") && i + 1 < argc)
            font_path = argv[++i];
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
            capture = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            limit = (unsigned)strtoul(argv[++i], NULL, 10);
        else
            return 1;
    }
    if (!font_path || (capture && !limit)) {
        fputs("Usage: system_theme --font PATH [--frames N --capture BMP]\n", stderr);
        return 1;
    }
    cui_font *font = cui_font_open(font_path);
    if (!font)
        return 2;
    cui_sdl3 *host = cui_sdl3_create("CUI system appearance", 900, 540, font);
    if (!host) {
        cui_font_close(font);
        return 3;
    }
    cui_context *ui = cui_create((cui_config){.width = 900,
                                              .height = 540,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .text_geometry = cui_font_text_geometry,
                                              .text_geometry_line = cui_font_text_geometry_line,
                                              .font_metrics = cui_font_get_metrics,
                                              .measure_user = font});
    system_theme_app app;
    int result = 0;
    if (!ui || !app_init(&app))
        result = 4;
    else {
        unsigned frames = 0;
        Uint64 previous = SDL_GetTicks();
        while ((!limit || frames < limit) && cui_sdl3_poll(host, ui)) {
            cui_platform_appearance appearance;
            if (!cui_sdl3_appearance(host, &appearance) || !app_observe(&app, appearance)) {
                result = 5;
                break;
            }
            float w, h;
            cui_sdl3_size(host, &w, &h);
            Uint64 now = SDL_GetTicks();
            float dt = (float)(now - previous) / 1000;
            previous = now;
            if (dt > 1)
                dt = 1;
            cui_draw_list list = app_frame(ui, &app, w, h, dt);
            bool painted =
                capture && frames + 1 == limit
                    ? cui_sdl3_render_capture(host, &list, app.resolved.theme.background, capture)
                    : cui_sdl3_render(host, &list, app.resolved.theme.background);
            if (!painted || cui_last_error(ui) != CUI_OK) {
                result = 6;
                break;
            }
            frames++;
            /* NULL leaves the event queued for adapter dispatch; no timed polling spin.
             * This static scene has no active timed animation. */
            if (!limit && !cui_needs_redraw(ui) && !SDL_WaitEvent(NULL)) {
                result = 7;
                break;
            }
        }
    }
    cui_destroy(ui);
    cui_sdl3_destroy(host);
    cui_font_close(font);
    return result;
}
#endif
