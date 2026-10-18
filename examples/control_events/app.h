/* SPDX-License-Identifier: MIT */
#ifndef CONTROL_EVENTS_EXAMPLE_APP_H
#define CONTROL_EVENTS_EXAMPLE_APP_H
#include "cui/cui_components.h"
enum { CE_SETTING = 10, CE_ALLOW = 11, CE_APPLY = 12, CE_CLOSE = 13, CE_REOPEN = 14 };
typedef struct {
    bool allow_toggle, visible, close_requested, open_requested;
    cui_check_state setting;
    unsigned requests, rejected, actions, closes;
    cui_component_check_request last_request;
    cui_component_event_metadata last_activation;
    cui_button_state apply_button, close_button, reopen_button;
} ce_app;
typedef struct {
    cui_component_action_result setting, allow, apply, close, reopen;
} ce_frame_result;
void ce_init(ce_app *);
void ce_setting_request(void *, const cui_component_check_request *);
void ce_allow_request(void *, const cui_component_check_request *);
void ce_apply(void *, const cui_component_event_metadata *);
void ce_close(void *, const cui_component_event_metadata *);
void ce_reopen(void *, const cui_component_event_metadata *);
/* UI thread: callbacks update app-owned values, never destroy active resources. */
ce_frame_result ce_scene(cui_context *, ce_app *, const cui_theme *);
/* Call after end_frame and rendering consumed its draw list. */
bool ce_finish_frame(ce_app *);
#endif
