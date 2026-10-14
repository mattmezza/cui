/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_NOTIFICATION_SHADOW_FIXTURE_H
#define CUI_TEST_NOTIFICATION_SHADOW_FIXTURE_H
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_theme theme;
    cui_notification_state state;
    cui_notification_entry entry[8];
} notification_fixture;
static void notification_fixture_init(notification_fixture *f, bool dark,
                                      cui_notification_shadow_policy policy) {
    memset(f, 0, sizeof(*f));
    f->ui = cui_create((cui_config){.max_elements = 256, .max_commands = 1024});
    assert(f->ui);
    f->theme = dark ? cui_theme_dark() : cui_theme_light();
    cui_notification_settings settings = cui_notification_settings_default();
    settings.width = 32;
    settings.margin_left = 16;
    settings.margin_top = 16;
    settings.placement = CUI_NOTIFICATION_TOP_LEFT;
    settings.reduced_motion = true;
    settings.shadow_policy = policy;
    assert(cui_notification_init(&f->state, f->entry, 8, settings, (cui_notification_host){0}));
    cui_notification n = cui_notification_default();
    n.id = (cui_notification_id){1, 1};
    n.height = 24;
    n.autohide = false;
    assert(cui_notification_push(&f->state, n, 0));
}
static cui_draw_list notification_fixture_frame(notification_fixture *f, float width, float height,
                                                double now, float opacity) {
    assert(cui_begin_frame(f->ui, width, height, .016f));
    cui_begin_box(f->ui, 9, (cui_box){.opacity = opacity, .custom_opacity = true});
    cui_notification_advance advance;
    assert(cui_notifications_checked(f->ui, 10, &f->theme, &f->state,
                                     (cui_rect){0, 0, width, height}, now, &advance));
    cui_end_box(f->ui);
    return cui_end_frame(f->ui);
}
#endif
