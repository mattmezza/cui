/* SPDX-License-Identifier: MIT */
#include "cui/cui_effects.h"
#include "cui/cui_software.h"
#include "notification_shadow_fixture.h"
static unsigned callbacks;
static void content(cui_context *ui, cui_id id, void *user, float font, cui_color color,
                    float opacity) {
    (void)user;
    (void)font;
    (void)color;
    (void)opacity;
    callbacks++;
    cui_theme theme = cui_theme_light();
    cui_spinner(ui, id ^ UINT64_C(0x123), &theme, 1, .5f, true);
    cui_begin_box(ui, id, (cui_box){.width = 1, .height = 2, .background = {255, 0, 0, 255}});
    cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("Disabled action"), true, false, 0);
    cui_end_box(ui);
}
static void emissions(void) {
    for (int dark = 0; dark < 2; dark++)
        for (int policy = 0; policy < 2; policy++) {
            notification_fixture f;
            notification_fixture_init(&f, dark, (cui_notification_shadow_policy)policy);
            cui_draw_list list = notification_fixture_frame(&f, 128, 128, 1, 1);
            size_t drops = 0, approx = 0;
            bool surface_seen = false, clip_seen = false;
            for (size_t i = 0; i < list.count; i++) {
                cui_draw_command c = list.commands[i];
                if (c.type == CUI_DRAW_RECT && c.color.r == 0 && c.color.g == 0 && c.color.b == 0 &&
                    c.color.a > 0 && c.color.a <= 4)
                    approx++;
                if (c.type == CUI_DRAW_DROP_SHADOW) {
                    assert(!surface_seen && !clip_seen);
                    assert(c.shadow.caster.x == 16 && c.shadow.caster.y == 16 &&
                           c.shadow.caster.w == 32 && c.shadow.caster.h == 24);
                    assert(c.shadow.radii.top_left == f.theme.radius_lg);
                    assert(c.shadow.layer.offset_x == 0 &&
                           c.shadow.layer.offset_y == (drops ? 4 : 10));
                    assert(c.shadow.layer.sigma == (drops ? 3 : 7.5f) &&
                           c.shadow.layer.spread == (drops ? -4 : -3));
                    assert(c.shadow.layer.color.r == 0 && c.shadow.layer.color.a == 26);
                    drops++;
                }
                if (policy && c.type == CUI_DRAW_RECT && c.color.r == f.theme.popover.r &&
                    c.color.g == f.theme.popover.g && c.color.b == f.theme.popover.b) {
                    assert(drops == 2);
                    surface_seen = true;
                }
                if (policy && c.type == CUI_DRAW_CLIP_PUSH) {
                    assert(surface_seen);
                    clip_seen = true;
                }
                if (policy && c.type == CUI_DRAW_BORDER)
                    assert(surface_seen && clip_seen);
            }
            assert(drops == (policy ? 2 : 0) && approx == (policy ? 0 : 16));
            assert(!policy || (surface_seen && clip_seen));
            if (policy) {
                list = notification_fixture_frame(&f, 128, 128, 1.016, .5f);
                for (size_t i = 0; i < list.count; i++)
                    if (list.commands[i].type == CUI_DRAW_DROP_SHADOW)
                        assert(list.commands[i].shadow.layer.color.a == 13);
            }
            cui_destroy(f.ui);
        }
}
static void software_pixels(void) {
    for (int dark = 0; dark < 2; dark++) {
        notification_fixture f;
        notification_fixture_init(&f, dark, CUI_NOTIFICATION_SHADOW_SOURCE);
        cui_draw_list list = notification_fixture_frame(&f, 128, 128, 1, 1);
        cui_software *r = cui_software_create(128, 128, NULL);
        assert(r);
        assert(cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
        const unsigned char *pixels = cui_software_pixels(r, NULL, NULL);
        assert(pixels[(28 * 128 + 32) * 4] == f.theme.popover.r);
        float expected = 255;
        const cui_drop_shadow layers[] = {
            {.offset_y = 10, .sigma = 7.5f, .spread = -3, .color = {0, 0, 0, 26}},
            {.offset_y = 4, .sigma = 3, .spread = -4, .color = {0, 0, 0, 26}}};
        for (size_t i = 0; i < 2; i++) {
            float coverage;
            assert(cui_effect_drop_shadow_sample(
                (cui_drop_shadow_paint){.caster = {16, 16, 32, 24},
                                        .radii = {f.theme.radius_lg, f.theme.radius_lg,
                                                  f.theme.radius_lg, f.theme.radius_lg},
                                        .layer = layers[i]},
                1, 0, 0, 32.5f, 42.5f, &coverage));
            expected *= 1 - coverage * 26 / 255;
        }
        assert(fabsf(pixels[(42 * 128 + 32) * 4] - expected) <= 2);
        unsigned char before[128 * 128 * 4];
        memcpy(before, pixels, sizeof before);
        cui_draw_command invalid[1024];
        memcpy(invalid, list.commands, list.count * sizeof(*invalid));
        for (size_t i = 0; i < list.count; i++)
            if (invalid[i].type == CUI_DRAW_DROP_SHADOW) {
                invalid[i].shadow.layer.sigma = NAN;
                break;
            }
        list.commands = invalid;
        assert(!cui_software_render(r, &list, (cui_color){0}));
        assert(memcmp(before, cui_software_pixels(r, NULL, NULL), sizeof before) == 0);
        f.state.settings.reduced_motion = false;
        f.entry[0].visibility = .5f;
        list = notification_fixture_frame(&f, 128, 128, 1, 1);
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].type == CUI_DRAW_DROP_SHADOW)
                assert(list.commands[i].shadow.layer.color.a == 13);
        cui_software_destroy(r);
        cui_destroy(f.ui);
    }
}
static bool system_send(void *user, const cui_notification *n) {
    (void)user;
    (void)n;
    return true;
}
static void custom_and_hidden(void) {
    notification_fixture f;
    notification_fixture_init(&f, false, CUI_NOTIFICATION_SHADOW_SOURCE);
    f.theme.popover = (cui_color){200, 120, 80, 128};
    f.theme.surface = (cui_color){1, 2, 3, 255};
    cui_draw_list list = notification_fixture_frame(&f, 128, 128, 1, .5f);
    cui_software *r = cui_software_create(128, 128, NULL);
    assert(r && cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    float under = 255;
    const cui_drop_shadow layers[] = {
        {.offset_y = 10, .sigma = 7.5f, .spread = -3, .color = {0, 0, 0, 13}},
        {.offset_y = 4, .sigma = 3, .spread = -4, .color = {0, 0, 0, 13}}};
    for (size_t i = 0; i < 2; i++) {
        float a;
        assert(cui_effect_drop_shadow_sample(
            (cui_drop_shadow_paint){.caster = {16, 16, 32, 24},
                                    .radii = {f.theme.radius_lg, f.theme.radius_lg,
                                              f.theme.radius_lg, f.theme.radius_lg},
                                    .layer = layers[i]},
            1, 0, 0, 32.5f, 28.5f, &a));
        under *= 1 - a * 13 / 255;
    }
    const float rgb[] = {200, 120, 80};
    const unsigned char *p = cui_software_pixels(r, NULL, NULL) + (28 * 128 + 32) * 4;
    for (size_t i = 0; i < 3; i++)
        assert(fabsf(p[i] - (rgb[i] * 64 / 255 + under * (1 - 64 / 255.0f))) <= 2);
    cui_software_destroy(r);
    cui_destroy(f.ui);
    notification_fixture_init(&f, false, CUI_NOTIFICATION_SHADOW_SOURCE);
    for (unsigned i = 2; i <= 4; i++) {
        cui_notification n = cui_notification_default();
        n.id = (cui_notification_id){1, i};
        n.height = 24;
        n.autohide = false;
        assert(cui_notification_push(&f.state, n, 0));
    }
    list = notification_fixture_frame(&f, 128, 128, 1, 1);
    size_t shadows = 0;
    for (size_t i = 0; i < list.count; i++)
        shadows += list.commands[i].type == CUI_DRAW_DROP_SHADOW;
    assert(shadows == 6); /* Three visible collapsed cards; the older card is source-invisible. */
    cui_notification_settings settings = f.state.settings;
    settings.delivery = CUI_NOTIFICATION_SYSTEM;
    assert(cui_notification_init(&f.state, f.entry, 8, settings,
                                 (cui_notification_host){.send = system_send}));
    cui_notification n = cui_notification_default();
    n.id = (cui_notification_id){1, 1};
    assert(cui_notification_push(&f.state, n, 0));
    list = notification_fixture_frame(&f, 128, 128, 1, 1);
    for (size_t i = 0; i < list.count; i++)
        assert(list.commands[i].type != CUI_DRAW_DROP_SHADOW);
    assert(f.state.count == 0 && f.state.system_count == 1);
    cui_destroy(f.ui);
}
static void atomic_validation(void) {
    notification_fixture f;
    notification_fixture_init(&f, false, CUI_NOTIFICATION_SHADOW_SOURCE);
    f.entry[0].notification.content = content;
    f.state.settings.shadow_policy = (cui_notification_shadow_policy)9;
    assert(!cui_notification_settings_valid(f.state.settings));
    cui_notification_state untouched = {.count = 777};
    assert(!cui_notification_init(&untouched, f.entry, 8, f.state.settings,
                                  (cui_notification_host){0}) &&
           untouched.count == 777);
    assert(!cui_notification_dismiss(&f.state, f.entry[0].notification.id, 1));
    cui_notification_advance advance = {.presented = 777};
    assert(cui_begin_frame(f.ui, 128, 128, .016f));
    assert(!cui_notifications_checked(f.ui, 10, &f.theme, &f.state, (cui_rect){0, 0, 128, 128}, 1,
                                      &advance));
    assert(advance.presented == 777 && callbacks == 0 &&
           f.entry[0].phase == CUI_NOTIFICATION_STARTING && f.entry[0].elapsed == 0);
    assert(cui_end_frame(f.ui).count == 0);
    f.state.settings.shadow_policy = CUI_NOTIFICATION_SHADOW_SOURCE;
    f.theme.radius_lg = NAN;
    assert(cui_begin_frame(f.ui, 128, 128, .016f));
    assert(!cui_notifications_checked(f.ui, 10, &f.theme, &f.state, (cui_rect){0, 0, 128, 128}, 1,
                                      &advance));
    assert(callbacks == 0 && advance.presented == 777 && cui_end_frame(f.ui).count == 0);
    f.theme.radius_lg = 8;
    f.entry[0].visibility = NAN;
    assert(cui_begin_frame(f.ui, 128, 128, .016f));
    assert(!cui_notifications_checked(f.ui, 10, &f.theme, &f.state, (cui_rect){0, 0, 128, 128}, 1,
                                      &advance));
    assert(callbacks == 0 && advance.presented == 777 && cui_end_frame(f.ui).count == 0);
    f.entry[0].visibility = 0;
    notification_fixture_frame(&f, 128, 128, 1, 1);
    assert(callbacks == 1);
    size_t semantic_count;
    const cui_semantic *semantics = cui_semantics(f.ui, &semantic_count);
    bool disabled_child = false;
    for (size_t i = 0; i < semantic_count; i++)
        disabled_child |= semantics[i].role == CUI_ROLE_BUTTON && semantics[i].disabled;
    assert(disabled_child);
    cui_destroy(f.ui);
}
static void fades(void) {
    notification_fixture f;
    notification_fixture_init(&f, false, CUI_NOTIFICATION_SHADOW_SOURCE);
    f.state.settings.reduced_motion = false;
    /* Retain a controlled PRESENT entry, then begin a controlled exit. */
    notification_fixture_frame(&f, 128, 128, 1, 1);
    assert(cui_notification_dismiss(&f.state, f.entry[0].notification.id, 1));
    cui_draw_list list = notification_fixture_frame(&f, 128, 128, 1.05, 1);
    float surface_alpha = 0;
    unsigned ink = 255;
    size_t count = 0;
    for (size_t i = 0; i < list.count; i++) {
        cui_draw_command c = list.commands[i];
        if (c.type == CUI_DRAW_DROP_SHADOW) {
            ink = c.shadow.layer.color.a;
            count++;
        }
        if (c.type == CUI_DRAW_RECT && c.color.r == f.theme.popover.r &&
            c.color.g == f.theme.popover.g && c.color.b == f.theme.popover.b)
            surface_alpha = c.color.a / 255.0f;
    }
    assert(count == 2 && ink < 26 && surface_alpha > 0 && surface_alpha < 1);
    assert(fabsf(ink - 25.5f * powf(surface_alpha, 4)) <= 1);
    notification_fixture_frame(&f, 128, 128, 1.201, 1);
    assert(f.state.count == 0);
    cui_destroy(f.ui);
}
int main(void) {
    emissions();
    atomic_validation();
    fades();
    software_pixels();
    custom_and_hidden();
    return 0;
}
