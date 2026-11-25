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
    cui_diagnostic diagnostic;
    if (cui_first_diagnostic(ui, &diagnostic))
        fprintf(stderr, "Input frame at %.0f: %s\n", parent, diagnostic.message);
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
        if (i == 0) {
            cui_focus(ui, 20);
            assert(cui_edit_select(edit, 255, 255) == CUI_EDIT_OK);
        } else {
            cui_rect caret;
            float scroll = -1;
            assert(cui_text_input_rect(ui, &caret));
            assert(cui_input_scroll_x(ui, 20, &scroll));
            assert(near(caret.x + caret.w, vp.x + vp.w));
            assert(near(scroll, 2551 - vp.w));
        }
    }
    float scroll = -1;
    assert(cui_input_scroll_x(ui, 20, &scroll) && !near(scroll, state.scroll_x));
    size_t intended = (size_t)floorf((25 + scroll) / 10 + .5f);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = state.viewport.x + 25,
                                           .y = state.viewport.y + 2}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = state.viewport.x + 25,
                                           .y = state.viewport.y + 2}));
    assert(cui_begin_frame(ui, 400, 100, .016f));
    assert(cui_input_group(ui, 10, 20, &theme, edit, &state, a, 2,
                           (cui_input_group_options){.width = 400, .name = CUI_STR("Group")})
               .error == CUI_FORM_OK);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_edit_get_state(edit).cursor == intended);
    assert(cui_edit_get_state(edit).anchor == intended);
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
static const cui_draw_command *command_text(cui_draw_list list, size_t length) {
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].text.length == length)
            return &list.commands[i];
    return NULL;
}
static void resize_caret(cui_context *ui, cui_edit *edit, cui_input_state *state) {
    const float widths[] = {400, 220, 500, 180};
    cui_input_options options = {.width = -1};
    const cui_id viewport = cui_input_viewport_id(1);
    cui_focus(ui, 1);
    assert(cui_edit_select(edit, 240, 255) == CUI_EDIT_OK);
    for (unsigned align = CUI_ALIGN_START; align <= CUI_ALIGN_END; ++align) {
        options.text_align = (cui_align)align;
        for (unsigned i = 0; i < sizeof widths / sizeof *widths; ++i) {
            cui_draw_list list = frame(ui, edit, state, widths[i], false, options);
            cui_rect view = cui_bounds(ui, viewport), caret;
            float scroll = -1;
            assert(cui_text_input_rect(ui, &caret));
            assert(cui_input_scroll_x(ui, 1, &scroll));
            assert(near(scroll, 2551 - view.w));
            assert(near(caret.x + caret.w, view.x + view.w));
            const cui_draw_command *text_command = command_text(list, 255);
            assert(text_command && near(text_command->bounds.x, view.x - scroll));
            bool selection_visible = false;
            cui_theme theme = cui_theme_light();
            for (size_t n = 0; n < list.count; ++n) {
                const cui_draw_command *c = &list.commands[n];
                if (c->type == CUI_DRAW_RECT && c->color.r == theme.selection.r &&
                    c->color.g == theme.selection.g && c->color.b == theme.selection.b &&
                    c->color.a == theme.selection.a) {
                    assert(near(c->bounds.x, text_command->bounds.x + 2400));
                    assert(near(c->bounds.w, 150));
                    selection_visible = true;
                }
            }
            assert(selection_visible);
        }
    }
    options.text_align = CUI_ALIGN_CENTER;
    frame(ui, edit, state, 410, false, options);
    cui_rect view = cui_bounds(ui, viewport);
    float scroll = -1;
    assert(cui_input_scroll_x(ui, 1, &scroll));
    assert(!near(scroll, state->scroll_x)); /* Public state remains synchronous. */
    size_t intended = (size_t)floorf((scroll + 25) / 10 + .5f);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = view.x + 25, .y = view.y + 2}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = view.x + 25, .y = view.y + 2}));
    frame(ui, edit, state, 270, false, options);
    assert(cui_edit_get_state(edit).cursor == intended);
    assert(cui_edit_get_state(edit).anchor == intended);

    cui_focus(ui, 0);
    state->scroll_x = 17;
    frame(ui, edit, state, 340, false, options);
    assert(cui_input_scroll_x(ui, 1, &scroll) && near(scroll, 17));

    assert(cui_begin_frame(ui, 260, 100, .016f));
    {
        cui_input_state temporary = {0};
        cui_theme theme = cui_theme_light();
        cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &temporary, options);
    }
    assert(cui_set_box_scroll_offset(ui, viewport, -32, 0));
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_input_scroll_x(ui, 1, &scroll) && near(scroll, 32));
    const cui_draw_command *text_command = command_text(list, 255);
    assert(text_command && near(text_command->bounds.x, cui_bounds(ui, viewport).x - 32));
}
static void resize_placeholder(void) {
    char buffer[24] = "";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_context *ui = cui_create((cui_config){.measure = measure});
    assert(ui && edit);
    cui_input_state state = {0};
    const float widths[] = {400, 180, 500};
    const cui_str placeholder = CUI_STR("Your name");
    for (unsigned align = CUI_ALIGN_CENTER; align <= CUI_ALIGN_END; ++align) {
        cui_input_options options = {
            .width = -1, .text_align = (cui_align)align, .placeholder = placeholder};
        for (unsigned i = 0; i < sizeof widths / sizeof *widths; ++i) {
            cui_draw_list list = frame(ui, edit, &state, widths[i], true, options);
            cui_rect view = cui_bounds(ui, cui_input_viewport_id(1));
            const cui_draw_command *text_command = command_text(list, placeholder.length);
            float whitespace = fmaxf(0, view.w - placeholder.length * 10);
            assert(text_command &&
                   near(text_command->bounds.x,
                        view.x + whitespace * (align == CUI_ALIGN_CENTER ? .5f : 1)));
        }
    }
    float unchanged = -7;
    assert(cui_begin_frame(ui, 500, 100, .016f));
    assert(!cui_input_scroll_x(ui, 1, &unchanged));
    (void)cui_end_frame(ui);
    assert(!cui_input_scroll_x(ui, 1, &unchanged) && unchanged == -7);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void resize_capacity(void) {
    char buffer[24] = "Capacity";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_context *ui = cui_create((cui_config){.measure = measure, .max_resources = 1});
    assert(ui && edit);
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    cui_input_options options = {.width = -1};
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_resource_get(ui, 23, 29, 1, NULL, NULL));
    cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &state, options);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    float scroll = -1;
    assert(!cui_input_scroll_x(ui, 1, &scroll) && scroll == -1);

    /* A frame omitting the unrelated resource allows normal collection. */
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 100, 100, .016f));
    (void)cui_end_frame(ui);
    assert(cui_begin_frame(ui, 100, 100, .016f));
    cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &state, options);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_input_scroll_x(ui, 1, &scroll) && scroll == 0);
    assert(cui_begin_frame(ui, 100, 100, .016f));
    cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &state, options);
    assert(!cui_resource_get(ui, 23, 29, 1, NULL, NULL));
    (void)cui_end_frame(ui);
    scroll = -1;
    assert(!cui_input_scroll_x(ui, 1, &scroll) && scroll == -1);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    char text[256];
    memset(text, 'W', sizeof text - 1);
    text[sizeof text - 1] = 0;
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = text, .capacity = sizeof text});
    assert(ui && edit);
    cui_input_state state = {0};
    layout(ui, edit, &state);
    composed(ui, edit);
    selection(ui, edit, &state);
    resize_caret(ui, edit, &state);
    resize_placeholder();
    resize_capacity();
    cui_edit_destroy(edit);
    cui_destroy(ui);
    return 0;
}
