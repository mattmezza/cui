/* SPDX-License-Identifier: MIT */
#define notification_shadow_scene notification_motion_unused_shadow_scene
#include "notification_shadow_probe.c"
#undef notification_shadow_scene
int notification_source_motion_scene(int anchor, int closing, double elapsed, float scale,
                                     int legacy, int reduced) {
    if (fixture.ui)
        cui_destroy(fixture.ui);
    notification_fixture_init(&fixture, true, CUI_NOTIFICATION_SHADOW_SOURCE);
    fixture.state.motion_policy =
        legacy ? CUI_NOTIFICATION_MOTION_LEGACY : CUI_NOTIFICATION_MOTION_SOURCE;
    fixture.state.settings.reduced_motion = reduced != 0;
    fixture.state.settings.placement = (cui_notification_anchor)anchor;
    fixture.state.settings.margin_top = fixture.state.settings.margin_bottom = 112;
    if (closing) {
        notification_fixture_frame(&fixture, 384 / scale, 384 / scale, .4, 1);
        assert(cui_notification_dismiss(&fixture.state, fixture.entry[0].notification.id, .4));
    }
    list = notification_fixture_frame(&fixture, 384 / scale, 384 / scale,
                                      (closing ? .4 : 0) + elapsed, 1);
    return (int)list.count;
}
float notification_source_motion_bound(unsigned field) {
    cui_rect b = fixture.entry[0].bounds;
    float values[] = {b.x, b.y, b.w, b.h,
                      fixture.state.stack_bounds[fixture.state.settings.placement].y};
    return field < 5 ? values[field] : 0;
}
