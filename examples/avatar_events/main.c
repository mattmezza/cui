/* SPDX-License-Identifier: MIT */
#include "cui/cui_avatar_events.h"
#include <assert.h>
#include <stdio.h>
/* A profile-picker owner: the handler copies evidence and requests deferred
 * dismissal. No handler frees UI state or repeats the action from frame counts. */
typedef struct {
    bool open, dismiss_pending;
    size_t profile_opens;
    cui_component_event_metadata last_activation;
} profile_picker;
static void open_profile(void *user, const cui_component_event_metadata *event) {
    profile_picker *owner = user;
    owner->last_activation = *event;
    owner->profile_opens++;
    owner->dismiss_pending = true;
}
static cui_component_action_result draw_picker(cui_context *ui, const cui_theme *theme,
                                               profile_picker *owner) {
    if (!owner->open)
        return (cui_component_action_result){0};
    return cui_avatar_events(ui, 200, theme, cui_string("Taylor Reed"),
                             (cui_avatar_options){.design_size = CUI_DESIGN_MD,
                                                  .interactive = true,
                                                  .identity_colors = true},
                             (cui_component_activation_handler){open_profile, owner});
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    assert(ui);
    cui_theme theme = cui_theme_light();
    profile_picker owner = {.open = true};
    /* This portable demonstration supplies one synthetic Core accessibility
     * action after registration. It does not claim native screen-reader proof. */
    for (unsigned frame = 0; frame < 3; frame++) {
        if (frame == 1)
            assert(cui_queue_accessibility_action(ui, 200, CUI_ACCESSIBILITY_ACTIVATE));
        assert(cui_begin_frame(ui, 200, 100, .016f));
        cui_component_action_result result = draw_picker(ui, &theme, &owner);
        cui_draw_list draw = cui_end_frame(ui);
        (void)draw;
        /* A real host must consume draw here before applying teardown. This
         * model-only program has no renderer or borrowed external image state. */
        if (owner.dismiss_pending) {
            assert(owner.open && owner.profile_opens == 1 && result.activation_count == 1);
            owner.open = false;
            owner.dismiss_pending = false;
        }
        if (frame == 2)
            assert(!result.submitted && !result.activation_count);
    }
    assert(owner.profile_opens == 1 && !owner.open);
    cui_destroy(ui);
    assert(owner.last_activation.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    puts("One profile opened; picker omitted on the next submission; evidence copied.");
    return 0;
}
