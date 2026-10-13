/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include <assert.h>
#include <string.h>

int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 320});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (unsigned theme = 0; theme < 4; theme++) {
        cui_theme *t = &themes[theme];
        assert(cui_begin_frame(ui, 320, 320, .016f));
        cui_begin_box(ui, 1, (cui_box){.gap = 4});
        cui_label(ui, t, cui_string("Éva 小"));
        cui_kbd(ui, 2, t, cui_string("Ctrl+P"));
        cui_spinner(ui, 3, t, 32, .75f, true);
        cui_skeleton(ui, 4, t, 100, 20, 10, .5f, true, true);
        cui_end_box(ui);
        cui_draw_list draw = cui_end_frame(ui);
        bool label = false, skeleton = false;
        unsigned dots = 0;
        for (size_t i = 0; i < draw.count; i++) {
            const cui_draw_command *d = &draw.commands[i];
            if (d->type == CUI_DRAW_TEXT && d->text.length == strlen("Éva 小")) {
                assert(!memcmp(d->text.data, "Éva 小", d->text.length));
                assert(d->font_size == t->font_size);
                assert(!memcmp(&d->color, &t->text, sizeof d->color));
                label = true;
            }
            if (d->type == CUI_DRAW_RECT && d->bounds.w == 100 && d->bounds.h == 20) {
                assert(d->radius == 10 && d->color.a == t->border.a / 2);
                skeleton = true;
            }
            if (d->type == CUI_DRAW_RECT && d->bounds.w > 5 && d->bounds.w < 6)
                dots++;
        }
        assert(label && skeleton && dots == 8);
        assert(cui_bounds(ui, 3).w == 32 && cui_bounds(ui, 3).h == 32);
        size_t count;
        const cui_semantic *s = cui_semantics(ui, &count);
        assert(count == 2); /* Label and Skeleton intentionally submit no semantics. */
        assert(s[0].id == 2 && s[0].role == CUI_ROLE_LABEL && s[0].name.length == 6);
        assert(!memcmp(s[0].name.data, "Ctrl+P", 6));
        assert(s[1].id == 3 && s[1].role == CUI_ROLE_STATUS && s[1].name.length == 7);
        assert(!memcmp(s[1].name.data, "Loading", 7));
        assert(!cui_needs_redraw(ui));
    }
    assert(cui_begin_frame(ui, 320, 320, .016f));
    cui_spinner(ui, 3, &themes[0], 24, .5f, false);
    cui_end_frame(ui);
    assert(cui_needs_redraw(ui));
    assert(cui_begin_frame(ui, 320, 320, .016f));
    cui_skeleton(ui, 4, &themes[0], 100, 20, 10, .5f, false, false);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_needs_redraw(ui));
    assert(draw.count == 1 && draw.commands[0].color.a == themes[0].border.a / 2);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    return 0;
}
