/* SPDX-License-Identifier: MIT
 * Diagnostic fixture for the existing geometry timing, not a prepaint API test. */
#include "cui/cui.h"
#include <assert.h>
#include <stdio.h>
static cui_rect tracked;
static cui_rect submit(cui_context *ui, float spacer, float width, bool *hovered) {
    cui_begin_box(ui, 100, (cui_box){.width = 300, .height = 200});
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = spacer});
    cui_end_box(ui);
    cui_begin_box(ui, 2, (cui_box){.width = width, .height = 80, .clip = true});
    cui_begin_box(ui, 20, (cui_box){.width = -1, .height = 60});
    cui_interaction interaction =
        cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("Plot"), false, false, 0);
    *hovered = interaction.hovered;
    cui_rect observed = cui_bounds(ui, 20);
    assert(cui_track_bounds(ui, 20, &tracked));
    assert(cui_set_underlay_ex(ui, (cui_underlay_layout){.grow_width = true, .grow_height = true},
                               (cui_color){17, 29, 43, 255}, (cui_corner_radii){0}));
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    return observed;
}
static cui_rect underlay(cui_draw_list draw) {
    for (size_t i = 0; i < draw.count; i++) {
        cui_draw_command c = draw.commands[i];
        if (c.type == CUI_DRAW_RECT && c.color.r == 17 && c.color.g == 29 && c.color.b == 43)
            return c.bounds;
    }
    assert(false);
    return (cui_rect){0};
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    bool hovered;
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_rect initial = submit(ui, 20, 200, &hovered);
    assert(initial.w == 0 && tracked.w == 0);
    cui_draw_list first = cui_end_frame(ui);
    cui_rect paint = underlay(first);
    assert(paint.x == 0 && paint.y == 20 && paint.w == 200 && paint.h == 60);
    assert(tracked.y == 20 && tracked.w == 200);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 50, .y = 35}));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_rect old = submit(ui, 60, 100, &hovered);
    float x, y;
    assert(cui_pointer_position(ui, &x, &y) && x == 50 && y == 35);
    assert(old.y == 20 && old.w == 200 && tracked.y == 20);
    assert(hovered); /* Previous-frame hit; current layout will move away. */
    cui_draw_list second = cui_end_frame(ui);
    paint = underlay(second);
    assert(paint.y == 60 && paint.w == 100 && paint.h == 60);
    assert(tracked.y == 60 && tracked.w == 100);
    assert(y < paint.y); /* Live pointer lies outside the current painted plot. */
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    puts("existing geometry timing and current-frame underlay seam: ok");
}
