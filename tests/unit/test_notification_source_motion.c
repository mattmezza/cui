/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "notification_shadow_fixture.h"
static unsigned closed, content_calls;
static void close_note(void *user, cui_notification_id id) {
    (void)user;
    (void)id;
    closed++;
}
static void custom_content(cui_context *ui, cui_id id, void *user, float size, cui_color ink,
                           float opacity) {
    (void)id;
    (void)user;
    (void)size;
    (void)ink;
    (void)opacity;
    content_calls++;
    cui_begin_box(ui, id, (cui_box){.width = 1, .height = 1});
    cui_end_box(ui);
}
static double reference_ease(double progress) {
    double lo = 0, hi = 1;
    for (unsigned i = 0; i < 60; i++) {
        double u = (lo + hi) / 2;
        double x = 3 * (1 - u) * (1 - u) * u * .25 + 3 * (1 - u) * u * u * .25 + u * u * u;
        if (x < progress)
            lo = u;
        else
            hi = u;
    }
    double u = (lo + hi) / 2;
    return 3 * (1 - u) * (1 - u) * u * .1 + 3 * (1 - u) * u * u + u * u * u;
}
static cui_notification_advance frame(notification_fixture *f, double now) {
    assert(cui_begin_frame(f->ui, 384, 384, .016f));
    cui_notification_advance result;
    assert(cui_notifications_checked(f->ui, 10, &f->theme, &f->state, (cui_rect){0, 0, 384, 384},
                                     now, &result));
    cui_end_frame(f->ui);
    return result;
}
static void geometry_and_fade(void) {
    for (int anchor = 0; anchor < 8; anchor++)
        for (int policy = 0; policy < 2; policy++) {
            notification_fixture f;
            notification_fixture_init(&f, false, CUI_NOTIFICATION_SHADOW_SOURCE);
            f.state.motion_policy = (cui_notification_motion_policy)policy;
            f.state.settings.reduced_motion = false;
            f.state.settings.placement = (cui_notification_anchor)anchor;
            f.state.settings.margin_top = f.state.settings.margin_bottom = 112;
            const double times[] = {0, .04, .1, .2, .399};
            for (size_t j = 0; j < sizeof(times) / sizeof(times[0]); j++) {
                frame(&f, times[j]);
                double ease = reference_ease(times[j] / .4);
                int direction = anchor >= CUI_NOTIFICATION_BOTTOM_LEFT &&
                                        anchor <= CUI_NOTIFICATION_BOTTOM_RIGHT
                                    ? 1
                                    : -1;
                if (anchor == CUI_NOTIFICATION_LEFT_CENTER ||
                    anchor == CUI_NOTIFICATION_RIGHT_CENTER)
                    direction = 0;
                double expected =
                    f.state.stack_bounds[anchor].y + direction * (1 - ease) * (policy ? 96 : 16);
                assert(fabs(f.entry[0].bounds.y - expected) < .002);
                assert(f.entry[0].phase == CUI_NOTIFICATION_STARTING);
            }
            cui_notification_advance done = frame(&f, .4);
            assert(done.presented == 1 && f.entry[0].phase == CUI_NOTIFICATION_PRESENT);
            assert(f.entry[0].bounds.y == f.state.stack_bounds[anchor].y);
            assert(cui_notification_dismiss(&f.state, f.entry[0].notification.id, .4));
            frame(&f, .45);
            double ease = reference_ease(.05 / .2);
            int direction =
                anchor >= CUI_NOTIFICATION_BOTTOM_LEFT && anchor <= CUI_NOTIFICATION_BOTTOM_RIGHT
                    ? 1
                    : -1;
            if (anchor == CUI_NOTIFICATION_LEFT_CENTER || anchor == CUI_NOTIFICATION_RIGHT_CENTER)
                direction = 0;
            assert(fabs(f.entry[0].bounds.y - (f.state.stack_bounds[anchor].y +
                                               direction * ease * (policy ? 96 : 16))) < .002);
            frame(&f, .599);
            assert(f.state.count == 1);
            assert(frame(&f, .6).removed == 1 && f.state.count == 0);
            frame(&f, .7);
            assert(!cui_needs_redraw(f.ui));
            cui_destroy(f.ui);
        }
}
static void reduced_deadlines(void) {
    notification_fixture f;
    notification_fixture_init(&f, false, CUI_NOTIFICATION_SHADOW_SOURCE);
    f.state.motion_policy = CUI_NOTIFICATION_MOTION_SOURCE;
    f.entry[0].notification.on_close = close_note;
    cui_notification_advance r = frame(&f, 0);
    assert(r.next_wake_seconds == .4 && r.presented == 0);
    r = frame(&f, .1);
    assert(fabs(r.next_wake_seconds - .4) < 1e-12 && r.presented == 0);
    assert(f.entry[0].bounds.y == f.state.stack_bounds[0].y && !cui_needs_redraw(f.ui));
    r = frame(&f, .4);
    assert(r.presented == 1 && r.next_wake_seconds == 0);
    frame(&f, .4);
    assert(!cui_needs_redraw(f.ui));
    assert(cui_notification_dismiss(&f.state, f.entry[0].notification.id, .4));
    r = frame(&f, .4);
    assert(fabs(r.next_wake_seconds - .6) < 1e-12 && closed == 0);
    assert(f.entry[0].bounds.y == f.state.stack_bounds[0].y - 96);
    r = frame(&f, .599);
    assert(closed == 0 && f.state.count == 1 && fabs(r.next_wake_seconds - .6) < 1e-12);
    assert(frame(&f, .6).removed == 1 && closed == 1);
    notification_fixture_frame(&f, 384, 384, .7, 1);
    assert(!cui_needs_redraw(f.ui));
    cui_destroy(f.ui);
}
static void independent_policies_and_pixels(void) {
    for (int shadow = 0; shadow < 2; shadow++) {
        notification_fixture f;
        notification_fixture_init(&f, true, (cui_notification_shadow_policy)shadow);
        f.state.motion_policy = CUI_NOTIFICATION_MOTION_SOURCE;
        f.state.settings.reduced_motion = false;
        f.state.settings.margin_top = 112;
        cui_draw_list list = notification_fixture_frame(&f, 384, 384, .1, 1);
        const double y = 112 - (1 - reference_ease(.25)) * 96;
        assert(fabs(f.entry[0].bounds.y - y) < .002);
        size_t drops = 0;
        for (size_t i = 0; i < list.count; i++)
            drops += list.commands[i].type == CUI_DRAW_DROP_SHADOW;
        assert(drops == (shadow ? 2 : 0));
        cui_software *renderer = cui_software_create(384, 384, NULL);
        assert(renderer && cui_software_render(renderer, &list, (cui_color){255, 255, 255, 255}));
        const unsigned char *pixels = cui_software_pixels(renderer, NULL, NULL);
        unsigned row = (unsigned)floor(y + 12);
        assert(pixels[(row * 384 + 32) * 4] < 200); /* Card has moved into source position. */
        assert(pixels[(112 * 384 + 32) * 4] > 250); /* Legacy16px card would cover this point. */
        cui_software_destroy(renderer);
        cui_destroy(f.ui);
    }
}
static void invalid_atomic(void) {
    notification_fixture f;
    notification_fixture_init(&f, false, CUI_NOTIFICATION_SHADOW_LEGACY);
    f.state.motion_policy = (cui_notification_motion_policy)99;
    f.entry[0].notification.content = custom_content;
    cui_notification_advance r = {.presented = 77};
    assert(cui_begin_frame(f.ui, 384, 384, .016f));
    assert(!cui_notifications_checked(f.ui, 10, &f.theme, &f.state, (cui_rect){0, 0, 384, 384}, .1,
                                      &r));
    assert(r.presented == 77 && content_calls == 0 && f.entry[0].elapsed == 0 &&
           f.state.last_render == 0);
    assert(cui_end_frame(f.ui).count == 0);
    f.state.motion_policy = CUI_NOTIFICATION_MOTION_SOURCE;
    f.entry[0].elapsed = NAN;
    assert(cui_begin_frame(f.ui, 384, 384, .016f));
    assert(!cui_notifications_checked(f.ui, 10, &f.theme, &f.state, (cui_rect){0, 0, 384, 384}, .1,
                                      &r));
    assert(r.presented == 77 && content_calls == 0 && cui_end_frame(f.ui).count == 0);
    cui_destroy(f.ui);
}
int main(void) {
    geometry_and_fade();
    reduced_deadlines();
    invalid_atomic();
    independent_policies_and_pixels();
    return 0;
}
