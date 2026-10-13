/* SPDX-License-Identifier: MIT */
#define CUI_SYSTEM_THEME_TEST
#include "../../examples/system_theme/main.c"
#include <assert.h>
static cui_platform_appearance observation(cui_system_appearance value, uint64_t revision) {
    return (cui_platform_appearance){value, CUI_APPEARANCE_SYSTEM_THEME, revision};
}
int main(void) {
    system_theme_app app;
    assert(app_init(&app) && !app.resolved.theme.dark && app.registry.count == 2);
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_UNKNOWN, 1)) &&
           !app.resolved.theme.dark);
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_DARK, 2)) &&
           app.resolved.theme.dark);
    uint64_t revision = app.preferences.revision;
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_DARK, 2)) &&
           app.preferences.revision == revision);
    assert(app_choose(&app, CUI_THEME_FORCE_LIGHT) && !app.resolved.theme.dark);
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_DARK, 3)) &&
           !app.resolved.theme.dark && app.selection.mode == CUI_THEME_FORCE_LIGHT);
    assert(app_choose(&app, CUI_THEME_FOLLOW_SYSTEM) && app.resolved.theme.dark);
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_UNKNOWN, 4)) &&
           app.resolved.theme.dark);
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_LIGHT, 5)) &&
           !app.resolved.theme.dark);
    assert(app_choose(&app, CUI_THEME_FORCE_DARK) && app.resolved.theme.dark);
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_LIGHT, 6)) &&
           app.resolved.theme.dark);
    revision = app.preferences.revision;
    assert(!app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_DARK, 5)) &&
           app.preferences.revision == revision);
    assert(!app_observe(&app, observation((cui_system_appearance)-1, 7)) &&
           app.preferences.revision == revision);
    assert(!app_choose(&app, (cui_theme_mode)99) && app.selection.mode == CUI_THEME_FORCE_DARK);
    cui_context *ui = cui_create((cui_config){.width = 900, .height = 540});
    assert(ui);
    cui_draw_list list = app_frame(ui, &app, 900, 540, 0);
    assert(list.count && cui_last_error(ui) == CUI_OK);
    assert(cui_queue_accessibility_action(ui, APP_LIGHT, CUI_ACCESSIBILITY_ACTIVATE));
    (void)app_frame(ui, &app, 900, 540, 0);
    assert(app.selection.mode == CUI_THEME_FORCE_LIGHT && !app.resolved.theme.dark);
    assert(cui_queue_accessibility_action(ui, APP_FOLLOW, CUI_ACCESSIBILITY_ACTIVATE));
    (void)app_frame(ui, &app, 900, 540, 0);
    assert(app.selection.mode == CUI_THEME_FOLLOW_SYSTEM && !app.resolved.theme.dark);
    assert(app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_DARK, 7)));
    list = app_frame(ui, &app, 900, 540, 0);
    assert(app.resolved.theme.dark && list.count && cui_last_error(ui) == CUI_OK);
    app.preferences.revision = UINT64_MAX;
    assert(!app_choose(&app, CUI_THEME_FORCE_LIGHT) &&
           app.selection.mode == CUI_THEME_FOLLOW_SYSTEM);
    assert(!app_observe(&app, observation(CUI_SYSTEM_APPEARANCE_LIGHT, 8)) &&
           app.observation_revision == 7);
    cui_destroy(ui);
    return 0;
}
