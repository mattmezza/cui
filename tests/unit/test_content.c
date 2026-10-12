/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void models(void) {
    char output[64];
    assert(cui_avatar_initials(cui_string(" ada   lovelace "), output) == 2 &&
           !strcmp(output, "AL"));
    assert(cui_avatar_initials(cui_string("Jean Paul Sartre"), output) == 2 &&
           !strcmp(output, "JS"));
    assert(cui_avatar_initials(cui_string("Éva 小"), output) == 5 && !strcmp(output, "É小"));
    assert(cui_avatar_initials(cui_string(""), output) == 1 && !strcmp(output, "?"));
    assert(cui_avatar_initials((cui_str){"\xed\xa0\x80", 3}, output) == 1 && !strcmp(output, "?"));
    assert(cui_avatar_initials((cui_str){"\xf0\x80\x80\x80", 4}, output) == 1);
    assert(cui_avatar_initials((cui_str){"\xc2", 1}, output) == 1);
    assert(cui_kbd_format(CUI_MOD_CTRL | CUI_MOD_SHIFT, cui_string("P"), false, output,
                          sizeof(output)) == 12);
    assert(!strcmp(output, "Ctrl+Shift+P"));
    assert(cui_kbd_format(CUI_MOD_META | CUI_MOD_ALT, cui_string("K"), true, output,
                          sizeof(output)) == 7);
    assert(!strcmp(output, "⌥⌘K"));
    char tiny[3] = {'!', '!', '!'};
    assert(cui_kbd_format(CUI_MOD_CTRL, cui_string("P"), false, tiny, 3) == 6);
    assert(tiny[0] == 'C' && tiny[1] == 't' && tiny[2] == 0);
    assert(cui_kbd_format(0, cui_string("P"), false, NULL, 0) == 1);
    assert(cui_loading_phase(1, 2) == .5f);
    assert(cui_loading_phase(5, 2) == .5f);
    assert(cui_loading_phase(NAN, 2) == 0 && cui_loading_phase(1, 0) == 0);
    cui_hover_state s = {0};
    assert(!cui_hover_update(&s, true, false, false, false, false, .1f, .3f, .2f));
    assert(!s.open && s.pending);
    assert(cui_hover_update(&s, true, false, false, false, false, .21f, .3f, .2f) && s.open);
    assert(!cui_hover_update(&s, false, true, false, false, false, .5f, .3f, .2f) && s.open);
    assert(!cui_hover_update(&s, false, false, false, false, false, .1f, .3f, .2f) && s.open);
    assert(!cui_hover_update(&s, true, false, false, false, false, .5f, .3f, .2f) && s.open &&
           !s.pending);
    assert(cui_hover_update(&s, false, false, false, true, false, 0, .3f, .2f) && !s.open);
    assert(cui_hover_update(&s, false, false, true, false, false, 0, 0, .2f) && s.open);
    assert(cui_hover_update(&s, true, true, true, false, true, .5f, .3f, .2f) && !s.open);
    assert(!cui_hover_update(&s, true, false, false, false, false, NAN, .3f, .2f) &&
           s.elapsed == 0);
    assert(!cui_hover_update(&s, false, false, false, false, false, 0, .3f, .2f) && !s.pending);
}
static cui_draw_list scene(cui_context *ui, const cui_theme *t, bool *visible, float phase,
                           bool reduced) {
    assert(cui_begin_frame(ui, 800, 1000, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 800, .gap = 4});
    cui_alert(ui, 2, t,
              (cui_alert_options){.kind = CUI_ALERT_ERROR,
                                  .title = cui_string("Failure"),
                                  .message = cui_string("Retry the operation"),
                                  .width = 400,
                                  .visible = visible,
                                  .dismissible = true});
    cui_avatar(ui, 3, t, cui_string("Ada Lovelace"), 48);
    cui_begin_bubble(ui, 4, t, CUI_BUBBLE_OUTLINE, 320);
    cui_text(ui, cui_string("Hello"), t->font_size, t->text);
    cui_end_bubble(ui);
    cui_empty(ui, 5, t, cui_string("No files"), cui_string("Upload a file to begin"), 320);
    cui_begin_group_box(ui, 6, t, cui_string("Preferences"), 320);
    cui_kbd(ui, 7, t, cui_string("Ctrl+P"));
    cui_end_group_box(ui);
    cui_spinner(ui, 8, t, 24, phase, reduced);
    cui_skeleton(ui, 9, t, 120, 16, 8, phase, false, reduced);
    cui_divider(ui, 10, t, 100, false);
    cui_divider(ui, 11, t, 20, true);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
int main(void) {
    models();
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 1000});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; ++i) {
        bool visible = true;
        cui_draw_list list = scene(ui, &themes[i], &visible, 0, true);
        bool initials = false, skeleton = false, vertical = false, outline = false;
        for (size_t j = 0; j < list.count; ++j) {
            const cui_draw_command *c = &list.commands[j];
            if (c->type == CUI_DRAW_TEXT && c->text.length == 2 && !memcmp(c->text.data, "AL", 2))
                initials = true;
            if (c->type == CUI_DRAW_RECT && c->bounds.w == 120 && c->bounds.h == 16 &&
                c->color.a == themes[i].border.a)
                skeleton = true;
            if (c->type == CUI_DRAW_RECT && c->bounds.w == 1 && c->bounds.h == 20)
                vertical = true;
            if (c->type == CUI_DRAW_BORDER && c->bounds.w == 320 && c->stroke == 1)
                outline = true;
        }
        assert(initials && skeleton && vertical && outline);
        assert(cui_bounds(ui, 3).w == 48 && cui_bounds(ui, 3).h == 48);
        assert(!cui_focused(ui));
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 7);
        assert(sem[0].id == 2 && sem[0].name.length == 19);
        cui_rect dismiss = cui_bounds(ui, sem[1].id);
        assert(cui_queue_event(
            ui,
            (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = dismiss.x + 2, .y = dismiss.y + 2}));
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = dismiss.x + 2, .y = dismiss.y + 2}));
        scene(ui, &themes[i], &visible, .5f, true);
        assert(!visible);
        scene(ui, &themes[i], &visible, 0, true);
        sem = cui_semantics(ui, &count);
        assert(count == 5 && sem[0].id == 3);
        assert(!cui_focused(ui));
        list = scene(ui, &themes[i], &visible, .5f, true);
        bool opaque = false;
        for (size_t j = 0; j < list.count; ++j)
            if (list.commands[j].bounds.w == 120 && list.commands[j].type == CUI_DRAW_RECT)
                opaque = list.commands[j].color.a == themes[i].border.a;
        assert(opaque);
        scene(ui, &themes[i], &visible, .5f, false);
        assert(cui_needs_redraw(ui));
    }
    cui_destroy(ui);
    return 0;
}
