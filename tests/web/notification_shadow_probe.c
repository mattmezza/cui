/* SPDX-License-Identifier: MIT */
#include "../unit/notification_shadow_fixture.h"
static notification_fixture fixture;
static cui_draw_list list;
int notification_shadow_scene(int dark, float scale) {
    if (fixture.ui)
        cui_destroy(fixture.ui);
    notification_fixture_init(&fixture, dark != 0, CUI_NOTIFICATION_SHADOW_SOURCE);
    list = notification_fixture_frame(&fixture, 128 / scale, 128 / scale, 1, 1);
    return (int)list.count;
}
int notification_shadow_type(unsigned i) {
    return i < list.count ? (int)list.commands[i].type : -1;
}
float notification_shadow_value(unsigned i, unsigned k) {
    if (i >= list.count)
        return 0;
    cui_draw_command c = list.commands[i];
    const float values[] = {c.bounds.x,
                            c.bounds.y,
                            c.bounds.w,
                            c.bounds.h,
                            c.color.r,
                            c.color.g,
                            c.color.b,
                            c.color.a,
                            c.radius,
                            c.stroke,
                            c.corner_radii.top_left,
                            c.corner_radii.top_right,
                            c.corner_radii.bottom_left,
                            c.corner_radii.bottom_right};
    return k < 14 ? values[k] : 0;
}
float notification_shadow_paint(unsigned i, unsigned k) {
    if (i >= list.count)
        return 0;
    cui_drop_shadow_paint p = list.commands[i].shadow;
    float values[] = {
        p.caster.x,       p.caster.y,        p.caster.w,          p.caster.h,
        p.radii.top_left, p.radii.top_right, p.radii.bottom_left, p.radii.bottom_right,
        p.layer.offset_x, p.layer.offset_y,  p.layer.sigma,       p.layer.spread,
        p.layer.color.r,  p.layer.color.g,   p.layer.color.b,     p.layer.color.a};
    return k < 16 ? values[k] : 0;
}
float notification_shadow_palette(unsigned k) {
    cui_color c = fixture.theme.popover;
    float values[] = {c.r, c.g, c.b, c.a};
    return k < 4 ? values[k] : 0;
}
