/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <stdio.h>
#include <string.h>
void ce_init(ce_app *a) {
    memset(a, 0, sizeof *a);
    a->visible = true;
}
void ce_setting_request(void *user, const cui_component_check_request *request) {
    ce_app *a = user;
    ++a->requests;
    a->last_request = *request;
    if (a->allow_toggle)
        a->setting = request->requested;
    else
        ++a->rejected;
}
void ce_allow_request(void *user, const cui_component_check_request *request) {
    ce_app *a = user;
    a->allow_toggle = request->requested == CUI_CHECK_CHECKED;
}
void ce_apply(void *user, const cui_component_event_metadata *event) {
    ce_app *a = user;
    ++a->actions;
    a->last_activation = *event;
}
void ce_close(void *user, const cui_component_event_metadata *event) {
    ce_app *a = user;
    (void)event;
    ++a->closes;
    a->close_requested = true;
}
void ce_reopen(void *user, const cui_component_event_metadata *event) {
    ce_app *a = user;
    (void)event;
    a->open_requested = true;
}
bool ce_finish_frame(ce_app *a) {
    bool changed = a->close_requested || a->open_requested;
    if (a->close_requested)
        a->visible = false;
    else if (a->open_requested)
        a->visible = true;
    a->close_requested = a->open_requested = false;
    return changed;
}
ce_frame_result ce_scene(cui_context *ui, ce_app *a, const cui_theme *theme) {
    ce_frame_result r = {0};
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 24, .gap = 14});
    cui_text(ui, cui_string("Settings: requests and owner decisions"), 23, theme->text);
    if (a->visible) {
        r.allow = cui_checkbox_events(ui, CE_ALLOW, theme, cui_string("Allow setting changes"),
                                      a->allow_toggle ? CUI_CHECK_CHECKED : CUI_CHECK_UNCHECKED,
                                      (cui_checkbox_options){0},
                                      (cui_component_check_handler){ce_allow_request, a});
        r.setting = cui_checkbox_events(ui, CE_SETTING, theme, cui_string("Enable notifications"),
                                        a->setting, (cui_checkbox_options){0},
                                        (cui_component_check_handler){ce_setting_request, a});
        cui_text(ui, cui_string("Owner rejects requests until changes are allowed."), 15,
                 theme->text);
        r.apply = cui_button_events(ui, CE_APPLY, theme, cui_string("Apply settings"),
                                    (cui_button_options){.state = &a->apply_button},
                                    (cui_component_activation_handler){ce_apply, a});
        r.close = cui_button_events(ui, CE_CLOSE, theme, cui_string("Close settings panel"),
                                    (cui_button_options){.state = &a->close_button},
                                    (cui_component_activation_handler){ce_close, a});
    } else {
        cui_text(ui, cui_string("Panel omitted after the previous frame completed."), 16,
                 theme->text);
        r.reopen = cui_button_events(ui, CE_REOPEN, theme, cui_string("Reopen settings"),
                                     (cui_button_options){.state = &a->reopen_button},
                                     (cui_component_activation_handler){ce_reopen, a});
    }
    char status[192];
    snprintf(status, sizeof status, "Owner: %s | requests: %u | rejected: %u | apply actions: %u",
             a->setting == CUI_CHECK_CHECKED ? "enabled" : "disabled", a->requests, a->rejected,
             a->actions);
    cui_text(ui, cui_string(status), 15, theme->text);
    snprintf(status, sizeof status, "This call: apply activations %zu, checkbox requests %zu",
             r.apply.activation_count, r.setting.request_count);
    cui_text(ui, cui_string(status), 14, theme->muted);
    cui_end_box(ui);
    return r; /* Inspection only: never replay callbacks from these counts. */
}
