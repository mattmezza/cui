/* SPDX-License-Identifier: MIT */
export function resizableUsage(nested) {
 return `#include "cui/cui_resizable.h"

/* Keep this application-owned model alive through frame completion. */
typedef struct {
    cui_resizable_state outer, inner;
    cui_resizable_slot outer_slots[2], inner_slots[2];
    const cui_theme *theme; /* borrowed during this submission */
    double now;
    bool disabled, reduced_motion;
} workspace_example;

bool workspace_init(workspace_example *app) {
    *app = (workspace_example){0};
    return cui_resizable_state_init(&app->outer, app->outer_slots, 2) &&
           cui_resizable_state_init(&app->inner, app->inner_slots, 2);
}
static void workspace_explorer(cui_context *ui, void *user) {
    workspace_example *app = user;
    cui_text(ui, cui_string("Explorer"), 14, app->theme->text);
}
static void workspace_editor(cui_context *ui, void *user) {
    workspace_example *app = user;
    cui_text(ui, cui_string("Code editor"), 14, app->theme->text);
}
static void workspace_console(cui_context *ui, void *user) {
    workspace_example *app = user;
    cui_text(ui, cui_string("Console: build complete"), 12, app->theme->text);
}
static void workspace_main(cui_context *ui, void *user) {
    workspace_example *app = user;
    cui_resizable_panel panels[] = {
        {.id = cui_id_from(ui, "workspace-code"),
         .accessibility_label = {"Code panel", 10},
         .has_initial_size = true, .initial_size = 180,
         .custom_range = true, .minimum = 100, .maximum = 240,
         .content = workspace_editor, .content_user = app},
        {.id = cui_id_from(ui, "workspace-console"),
         .accessibility_label = {"Console panel", 13},
         .custom_range = true, .minimum = 60, .maximum = 200,
         .content = workspace_console, .content_user = app}
    };
    cui_resizable_options options = cui_resizable_options_default();
    options.axis = CUI_RESIZABLE_VERTICAL;
    options.extent = 300;
    /* Outer input is reduced before content: this is the current pane width. */
    options.cross_size = app->outer.slots[1].size;
    options.now_seconds = app->now;
    options.disabled = app->disabled;
    options.reduced_motion = app->reduced_motion;
    (void)cui_resizable(ui, cui_id_from(ui, "workspace-inner"), app->theme,
                        &app->inner, panels, 2, options);
}
/* Initialize once with workspace_init; call inside a frame with width >= 160.
 * Use distinct namespaces/models for simultaneously rendered workspaces.
 * Switching composition should clear both states before submitting again. */
void draw_workspace(cui_context *ui, workspace_example *app, const cui_theme *theme,
                    float width, double now, bool disabled, bool reduced_motion) {
    const bool nested = ${nested ? 'true' : 'false'};
    app->theme = theme; app->now = now;
    app->disabled = disabled; app->reduced_motion = reduced_motion;
    cui_resizable_panel panels[] = {
        {.id = cui_id_from(ui, "workspace-explorer"),
         .accessibility_label = {"Explorer panel", 14},
         .has_initial_size = true, .initial_size = 140,
         .custom_range = true, .minimum = 80, .maximum = 300,
         .content = workspace_explorer, .content_user = app},
        {.id = cui_id_from(ui, "workspace-main"),
         .accessibility_label = {"Workspace panel", 15},
         .custom_range = true, .minimum = 80, .maximum = width,
         .content = nested ? workspace_main : workspace_editor, .content_user = app}
    };
    cui_resizable_options options = cui_resizable_options_default();
    options.extent = width; options.cross_size = nested ? 300 : 160;
    options.now_seconds = now; options.disabled = disabled;
    options.reduced_motion = reduced_motion;
    (void)cui_resizable(ui, cui_id_from(ui, "workspace-outer"), theme,
                        &app->outer, panels, 2, options);
}
`;
}
