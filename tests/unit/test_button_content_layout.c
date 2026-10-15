/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>

typedef struct {
    bool paragraph, nested, decorated;
    unsigned calls;
} content_state;

static cui_measure_fn fixture_measure;
static void *fixture_measure_user;

static void content(cui_context *ui, void *user) {
    content_state *state = user;
    ++state->calls;
    if (state->nested) {
        cui_begin_box(ui, 30, (cui_box){.width = -1, .row = true, .gap = 8});
        cui_begin_box(ui, 31, (cui_box){.width = 32, .height = 32, .fixed_width = true});
        cui_end_box(ui);
        cui_begin_box(ui, 32, (cui_box){.width = -1});
    }
    cui_str preview = cui_string("A preview of the message with enough words to wrap across "
                                 "several lines in a narrow mail list row.");
    if (state->paragraph)
        assert(cui_text_paragraph_colored(ui, preview, 14, (cui_color){1, 2, 3, 255}, 18,
                                          (cui_text_style){0}, true, NULL, 0));
    else
        cui_text_with_line_height(ui, preview, 14, (cui_color){1, 2, 3, 255}, 18);
    if (state->nested) {
        cui_end_box(ui);
        cui_end_box(ui);
    }
}

static unsigned frame(cui_context *ui, content_state *state, float width, bool fixed,
                      bool overflow) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 800, 600, 0));
    cui_begin_box(ui, 1, (cui_box){.width = width, .height = 400});
    unsigned before = state->calls;
    cui_button_ex(
        ui, 10, &theme, cui_string("Message"),
        (cui_button_options){.variant = CUI_BUTTON_GHOST,
                             .width = fixed ? width : -1,
                             .height = 200,
                             .leading = state->decorated ? cui_string("L") : (cui_str){0},
                             .trailing = state->decorated ? cui_string("R") : (cui_str){0},
                             .content = content,
                             .content_user = state,
                             .allow_content_overflow = overflow});
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(state->calls == before + 1);
    cui_rect button = cui_bounds(ui, 10);
    assert(fabsf(button.w - width) < .01f);
    unsigned lines = 0, clips = 0;
    float last_y = -1;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *command = &list.commands[i];
        if (command->type == CUI_DRAW_CLIP_PUSH)
            ++clips;
        else if (command->type == CUI_DRAW_CLIP_POP) {
            assert(clips);
            --clips;
        }
        if (command->type != CUI_DRAW_TEXT || command->color.r != 1 || command->color.g != 2 ||
            command->color.b != 3)
            continue;
        ++lines;
        assert(command->bounds.y > last_y);
        last_y = command->bounds.y;
        if (!overflow) {
            assert(clips);
            assert(command->bounds.x >= button.x + 10 - .01f);
            assert(command->bounds.x + command->bounds.w <= button.x + button.w - 10 + .01f);
        }
    }
    assert(lines && !clips);
    if (state->nested && !overflow) {
        cui_rect row = cui_bounds(ui, 30), avatar = cui_bounds(ui, 31), text = cui_bounds(ui, 32);
        assert(row.x >= button.x + 10 - .01f);
        assert(row.x + row.w <= button.x + button.w - 10 + .01f);
        assert(fabsf(avatar.w - 32) < .01f);
        assert(text.x >= avatar.x + avatar.w + 8 - .01f);
        assert(text.x + text.w <= row.x + row.w + .01f);
    }
    return lines;
}

static void intrinsic(cui_context *ui, content_state *state) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 2000, 400, 0));
    unsigned before = state->calls;
    cui_button_ex(ui, 10, &theme, cui_string("Message"),
                  (cui_button_options){.height = 200, .content = content, .content_user = state});
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && state->calls == before + 1);
    assert(cui_bounds(ui, 10).w > 358);
    unsigned lines = 0;
    for (size_t i = 0; i < list.count; ++i)
        lines += list.commands[i].type == CUI_DRAW_TEXT;
    assert(lines == 1);
}

static unsigned reference(cui_context *ui, content_state *state, float width) {
    assert(cui_begin_frame(ui, 800, 600, 0));
    cui_begin_box(ui, 1, (cui_box){.width = width - 20, .height = 200, .row = true, .gap = 8});
    if (state->decorated)
        cui_text(ui, cui_string("L"), 14, (cui_color){0});
    content(ui, state);
    if (state->decorated)
        cui_text(ui, cui_string("R"), 14, (cui_color){0});
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    unsigned lines = 0;
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].color.r == 1)
            ++lines;
    return lines;
}

int main(void) {
    for (unsigned paragraph = 0; paragraph < 2; ++paragraph)
        for (unsigned nested = 0; nested < 2; ++nested)
            for (unsigned mode = 0; mode < 4; ++mode) {
                bool fixed = (mode & 1) != 0;
                cui_context *ui = cui_create(
                    (cui_config){.measure = fixture_measure, .measure_user = fixture_measure_user});
                assert(ui);
                content_state state = {.paragraph = paragraph != 0,
                                       .nested = nested != 0,
                                       .decorated = (mode & 2) != 0};
                /* First frame and immediate resize must use current layout, without replay. */
                unsigned narrow = frame(ui, &state, 220, fixed != 0, false);
                unsigned wide = frame(ui, &state, 358, fixed != 0, false);
                assert(narrow > wide && wide > 1);
                assert(frame(ui, &state, 220, fixed != 0, false) == narrow);
                assert(reference(ui, &state, 220) == narrow);
                assert(reference(ui, &state, 358) == wide);
                if (!state.nested) {
                    (void)frame(ui, &state, 220, fixed, true);
                    intrinsic(ui, &state);
                }
                cui_destroy(ui);
            }
    return 0;
}
