/* SPDX-License-Identifier: MIT */
#include "../integration/tabs_source_shadows_fixture.h"
#include <string.h>
static cui_draw_command commands[64];
static cui_draw_list list;
static float values[16];
int tabs_shadow_scene(unsigned dark, unsigned policy, unsigned translucent) {
    cui_rect caster;
    list = tabs_shadow_fixture(dark, policy, translucent != 0, commands, &caster);
    return (int)list.count;
}
float *tabs_shadow_command(unsigned index) {
    if (index >= list.count)
        return NULL;
    const cui_draw_command *c = &commands[index];
    if (c->type == CUI_DRAW_DROP_SHADOW) {
        const cui_drop_shadow_paint *s = &c->shadow;
        const float next[] = {
            s->caster.x,       s->caster.y,        s->caster.w,           s->caster.h,
            s->radii.top_left, s->radii.top_right, s->radii.bottom_right, s->radii.bottom_left,
            s->layer.offset_x, s->layer.offset_y,  s->layer.sigma,        s->layer.spread,
            s->layer.color.r,  s->layer.color.g,   s->layer.color.b,      s->layer.color.a};
        memcpy(values, next, sizeof(values));
    } else {
        const float next[] = {c->bounds.x, c->bounds.y, c->bounds.w, c->bounds.h, c->radius,
                              c->color.r,  c->color.g,  c->color.b,  c->color.a};
        memset(values, 0, sizeof(values));
        memcpy(values, next, sizeof(next));
    }
    return values;
}
int tabs_shadow_type(unsigned index) {
    return index < list.count ? (int)commands[index].type : -1;
}
