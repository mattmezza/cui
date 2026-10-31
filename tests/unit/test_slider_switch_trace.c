/* SPDX-License-Identifier: MIT */
/* Local before/after trace compares the actual public Switch on identical Core.
 * CI additionally executes the independent old-source numeric oracle. */
#include "cui/cui_components.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static uint64_t hash;
static void number(uint64_t v) {
    for (unsigned i = 0; i < 8; ++i) {
        hash = (hash ^ (v & 255)) * UINT64_C(1099511628211);
        v >>= 8;
    }
}
static void real(double d) {
    uint64_t bits;
    memcpy(&bits, &d, 8);
    number(bits);
}
static void rectangle(cui_rect r) {
    real(r.x);
    real(r.y);
    real(r.w);
    real(r.h);
}
int main(void) {
    const double clocks[] = {0, .01, .1, .2, .15, .2, .5, 2, 2, 100};
    for (unsigned mode = 0; mode < 6; ++mode) {
        cui_context *ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
        assert(ui);
        cui_theme theme = cui_theme_light();
        cui_switch_state state = {0};
        bool value = false;
        theme.spring_move.damping = mode == 0 ? 0 : mode == 1 ? .5f : mode == 2 ? 1 : 2;
        if (mode == 4)
            theme.spring_move.travel = false;
        if (mode == 5)
            theme.spring_move.response = 0;
        hash = UINT64_C(14695981039346656037);
        for (unsigned frame = 0; frame < 10; ++frame) {
            if (frame == 1 || frame == 3 || frame == 5)
                value = !value;
            if (frame == 6)
                cui_focus(ui, 100);
            assert(cui_begin_frame(ui, 200, 100, .016f));
            cui_switch_options o = cui_switch_options_default();
            o.now_seconds = clocks[frame];
            o.reduced_motion = frame == 8;
            o.disabled = frame == 9;
            (void)cui_switch_ex(ui, 100, &theme, (cui_str){0}, &value, &state, o);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            real(state.position);
            real(state.velocity);
            real(state.target);
            real(state.updated_at);
            real(state.next_wake_seconds);
            number(state.initialized);
            number(list.count);
            for (size_t i = 0; i < list.count; ++i) {
                const cui_draw_command *c = &list.commands[i];
                number(c->type);
                rectangle(c->bounds);
                number(c->color.r);
                number(c->color.g);
                number(c->color.b);
                number(c->color.a);
                real(c->radius);
                real(c->stroke);
            }
            rectangle(cui_bounds(ui, 100));
            rectangle(cui_bounds(ui, cui_switch_track_id(100)));
            rectangle(cui_bounds(ui, cui_switch_thumb_id(100)));
            size_t count;
            const cui_semantic *sem = cui_semantics(ui, &count);
            number(count);
            for (size_t i = 0; i < count; ++i) {
                number(sem[i].id);
                number(sem[i].role);
                rectangle(sem[i].bounds);
                number(sem[i].checked);
                number(sem[i].disabled);
                number(sem[i].focused);
            }
        }
        printf("Switch mode%u %016" PRIx64 "\n", mode, hash);
        cui_destroy(ui);
    }
    return 0;
}
