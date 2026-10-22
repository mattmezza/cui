/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_form.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_size measure(void *user, cui_str text, float font_size) {
    (void)user;
    (void)font_size;
    return (cui_size){(float)text.length * 10, 20};
}
static cui_draw_list frame(cui_context *ui, cui_edit *edit, cui_input_state *state, float parent,
                           bool row, cui_input_options options) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, parent, 100, .016f));
    cui_begin_box(ui, 100, (cui_box){.width = parent, .fixed_width = true, .row = row});
    if (row) {
        cui_begin_box(ui, 101, (cui_box){.width = 64, .fixed_width = true, .height = 30});
        cui_end_box(ui);
    }
    assert(cui_input_ex(ui, 1, &theme, cui_string("Name"), edit, state, options).error ==
           CUI_EDIT_OK);
    if (row) {
        cui_begin_box(ui, 102, (cui_box){.width = 48, .fixed_width = true, .height = 30});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static bool near(float a, float b) {
    return fabsf(a - b) < .001f;
}
static void layout(cui_context *ui, cui_edit *edit, cui_input_state *state) {
    const float widths[] = {400, 220, 500, 180};
    for (unsigned row = 0; row < 2; row++)
        for (unsigned align = CUI_ALIGN_START; align <= CUI_ALIGN_END; align++)
            for (unsigned pass = 0; pass < sizeof widths / sizeof *widths; pass++) {
                cui_input_options o = {.width = -1,
                                       .text_align = (cui_align)align,
                                       .custom_padding = true,
                                       .padding_x = 3.25f + pass * 2.5f,
                                       .no_border = pass == 1,
                                       .no_appearance = pass == 2,
                                       .invalid = pass == 3};
                cui_draw_list list = frame(ui, edit, state, widths[pass], row != 0, o);
                cui_rect box = cui_bounds(ui, 1), vp = cui_bounds(ui, cui_input_viewport_id(1));
                float inset = o.padding_x + (o.no_border || o.no_appearance ? 0 : 1);
                assert(near(box.w, widths[pass] - (row ? 112 : 0)));
                assert(near(vp.w, box.w - 2 * inset) && near(vp.x, box.x + inset));
                assert(vp.x >= box.x && vp.x + vp.w <= box.x + box.w + .001f);
                unsigned depth = 0, clips = 0, text_count = 0;
                bool frame_clip = false, viewport_clip = false;
                for (size_t i = 0; i < list.count; i++) {
                    const cui_draw_command *c = &list.commands[i];
                    if (c->type == CUI_DRAW_CLIP_PUSH) {
                        depth++;
                        clips++;
                        frame_clip |= near(c->bounds.x, box.x) && near(c->bounds.w, box.w);
                        viewport_clip |= near(c->bounds.x, vp.x) && near(c->bounds.w, vp.w);
                    } else if (c->type == CUI_DRAW_CLIP_POP) {
                        assert(depth);
                        depth--;
                    } else if (c->type == CUI_DRAW_TEXT) {
                        assert(depth >= 2 && c->text.length == 255 && c->bounds.w == 2550);
                        text_count++;
                    }
                }
                assert(!depth && clips >= 2 && frame_clip && viewport_clip && text_count == 1);
                if (row) {
                    cui_rect suffix = cui_bounds(ui, 102);
                    assert(near(suffix.x, box.x + box.w) && near(suffix.w, 48));
                }
            }
    /* Existing fixed width/default width retain explicit viewport dimensions. */
    for (unsigned mode = 0; mode < 2; mode++) {
        cui_input_options o = {.width = mode ? 170 : 0};
        frame(ui, edit, state, 500, false, o);
        cui_rect box = cui_bounds(ui, 1), vp = cui_bounds(ui, cui_input_viewport_id(1));
        assert(near(box.w, mode ? 170 : 280) && near(vp.w, box.w - 22));
    }
}
static void composed(cui_context *ui, cui_edit *edit) {
    cui_input_group_state state = {0};
    cui_theme theme = cui_theme_light();
    cui_input_group_addon a[] = {{.id = 30,
                                  .kind = CUI_INPUT_GROUP_TEXT,
                                  .alignment = CUI_INPUT_GROUP_INLINE_START,
                                  .label = CUI_STR("Pre"),
                                  .width = 40},
                                 {.id = 31,
                                  .kind = CUI_INPUT_GROUP_TEXT,
                                  .alignment = CUI_INPUT_GROUP_INLINE_END,
                                  .label = CUI_STR("Post"),
                                  .width = 56}};
    const float widths[] = {400, 220, 500};
    for (unsigned i = 0; i < sizeof widths / sizeof *widths; i++) {
        assert(cui_begin_frame(ui, widths[i], 100, .016f));
        cui_input_group_result r = cui_input_group(
            ui, 10, 20, &theme, edit, &state, a, 2,
            (cui_input_group_options){.width = widths[i], .name = CUI_STR("Group")});
        assert(r.error == CUI_FORM_OK && r.input.error == CUI_EDIT_OK);
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        cui_rect group = cui_bounds(ui, 10), prefix = cui_bounds(ui, 30),
                 suffix = cui_bounds(ui, 31);
        cui_rect input = cui_bounds(ui, 20), vp = cui_bounds(ui, cui_input_viewport_id(20));
        fprintf(stderr, "group %.0f: prefix %.1f input %.1f suffix %.1f viewport %.1f\n", group.w,
                prefix.w, input.w, suffix.w, vp.w);
        assert(near(group.w, widths[i]) && near(prefix.w, 40) && near(suffix.w, 56));
        assert(prefix.x >= group.x && suffix.x + suffix.w <= group.x + group.w + .001f);
        assert(input.x >= prefix.x + prefix.w - .001f && input.x + input.w <= suffix.x + .001f);
        assert(vp.x >= input.x && vp.x + vp.w <= input.x + input.w + .001f);
        assert(near(state.viewport.w, vp.w));
    }
}
static void selection(cui_context *ui, cui_edit *edit, cui_input_state *state) {
    cui_input_options o = {.width = -1};
    cui_focus(ui, 1);
    assert(cui_edit_select(edit, 255, 255) == CUI_EDIT_OK);
    frame(ui, edit, state, 400, false, o);
    frame(ui, edit, state, 400, false, o); /* Settled geometry; first-resize probe is separate. */
    cui_rect vp = cui_bounds(ui, cui_input_viewport_id(1)), caret;
    assert(state->scroll_x > 0 && cui_text_input_rect(ui, &caret));
    assert(caret.x >= vp.x && caret.x + caret.w <= vp.x + vp.w + .001f);
    size_t expected = (size_t)floorf((25 + state->scroll_x) / 10 + .5f);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = vp.x + 25, .y = vp.y + 2}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = vp.x + 25, .y = vp.y + 2}));
    frame(ui, edit, state, 400, false, o);
    assert(cui_edit_get_state(edit).cursor == expected &&
           cui_edit_get_state(edit).anchor == expected);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME, .modifiers = CUI_MOD_SHIFT}));
    cui_draw_list list = frame(ui, edit, state, 400, false, o);
    assert(cui_edit_get_state(edit).cursor == 0 && cui_edit_get_state(edit).anchor == expected);
    assert(state->scroll_x == 0 && cui_text_input_rect(ui, &caret));
    assert(caret.x >= vp.x && caret.x + caret.w <= vp.x + vp.w + .001f);
    unsigned depth = 0;
    bool selected = false;
    cui_theme theme = cui_theme_light();
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type == CUI_DRAW_CLIP_PUSH)
            depth++;
        else if (c->type == CUI_DRAW_CLIP_POP) {
            assert(depth);
            depth--;
        } else if (c->type == CUI_DRAW_RECT && c->color.r == theme.selection.r &&
                   c->color.g == theme.selection.g && c->color.b == theme.selection.b &&
                   c->color.a == theme.selection.a && c->bounds.w > vp.w) {
            assert(depth >= 2);
            selected = true;
        }
    }
    assert(selected && !depth && cui_edit_get_state(edit).length == 255);
}
/* Optional failing probe, not a green test that codifies the remaining bug.
 * Run --probe-caret to require same-frame end-caret/scroll after grow resize. */
static int caret_probe(cui_context *ui, cui_edit *edit, cui_input_state *state) {
    cui_input_options options = {.width = -1};
    frame(ui, edit, state, 400, false, options);
    cui_focus(ui, 1);
    assert(cui_edit_select(edit, 255, 255) == CUI_EDIT_OK);
    const float widths[] = {400, 220, 500, 500};
    unsigned failures = 0;
    for (unsigned i = 0; i < sizeof widths / sizeof *widths; i++) {
        frame(ui, edit, state, widths[i], false, options);
        cui_rect vp = cui_bounds(ui, cui_input_viewport_id(1)), caret;
        assert(cui_text_input_rect(ui, &caret));
        float expected_scroll = fmaxf(0, 2551 - vp.w);
        fprintf(stderr,
                "caret width%.0f viewport%.0f caretX%.0f right%.0f scroll%.0f expected%.0f\n",
                widths[i], vp.w, caret.x, vp.x + vp.w, state->scroll_x, expected_scroll);
        failures += !near(state->scroll_x, expected_scroll) || caret.x < vp.x ||
                    caret.x + caret.w > vp.x + vp.w + .001f;
    }
    return failures ? 1 : 0;
}
int main(int argc, char **argv) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    char text[256];
    memset(text, 'W', sizeof text - 1);
    text[sizeof text - 1] = 0;
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = text, .capacity = sizeof text});
    assert(ui && edit);
    cui_input_state state = {0};
    int result = 0;
    if (argc == 2 && !strcmp(argv[1], "--probe-caret"))
        result = caret_probe(ui, edit, &state);
    else {
        layout(ui, edit, &state);
        composed(ui, edit);
        selection(ui, edit, &state);
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
    return result;
}
