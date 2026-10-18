/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static char clipboard[128];
static bool reject_copy;
static bool copy(void *user, cui_str value) {
    (void)user;
    if (reject_copy || value.length >= sizeof clipboard)
        return false;
    memcpy(clipboard, value.data, value.length);
    clipboard[value.length] = 0;
    return true;
}
static cui_str paste(void *user) {
    (void)user;
    return cui_string(clipboard);
}
static void key(cui_context *ui, cui_key code, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code, .modifiers = modifiers}));
}
static cui_input_result base_frame(cui_context *ui, cui_edit *edit, cui_input_behavior_state *state,
                                   cui_base_input_options options) {
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 40});
    cui_input_result result = cui_base_input_ex(ui, 1, cui_string("Input"), edit, state, options);
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void single_event_reduction(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char buffer[128] = "abc";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_behavior_state state = {0};
    cui_base_input_options options = {0};
    assert(ui && edit);
    base_frame(ui, edit, &state, options);
    cui_focus(ui, 1);
    key(ui, CUI_KEY_A, CUI_MOD_CTRL);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "fresh"}));
    key(ui, CUI_KEY_ENTER, 0);
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 40});
    cui_interact(ui, 1, CUI_ROLE_TEXTBOX, cui_string("Input"), false, false, 3);
    cui_input_result event = cui_base_input_reduce_event(ui, 1, edit, &state, options, 0);
    assert(!event.changed && !event.submitted && event.error == CUI_EDIT_OK);
    event = cui_base_input_reduce_event(ui, 1, edit, &state, options, 1);
    assert(event.changed && !event.submitted && !strcmp(buffer, "fresh"));
    event = cui_base_input_reduce_event(ui, 1, edit, &state, options, 2);
    assert(!event.changed && event.submitted);
    state.dragging = true;
    cui_edit_state before = cui_edit_get_state(edit);
    cui_input_result invalid[] = {
        cui_base_input_reduce_event(NULL, 1, edit, &state, options, 0),
        cui_base_input_reduce_event(ui, 0, edit, &state, options, 0),
        cui_base_input_reduce_event(ui, 1, NULL, &state, options, 0),
        cui_base_input_reduce_event(ui, 1, edit, NULL, options, 0),
        cui_base_input_reduce_event(ui, 1, edit, &state, options, 3),
        cui_base_input_reduce_event(ui, 1, edit, &state, (cui_base_input_options){.disabled = true},
                                    SIZE_MAX)};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i)
        assert(invalid[i].error == CUI_EDIT_INVALID_ARGUMENT && !invalid[i].changed &&
               !invalid[i].submitted);
    assert(state.dragging && cui_edit_get_state(edit).revision == before.revision &&
           cui_edit_get_state(edit).cursor == before.cursor && !strcmp(buffer, "fresh"));
    cui_end_box(ui);
    cui_end_frame(ui);
    /* Delegated vertical/Home/End keys remain unconsumed and leave the editor
     * unchanged, while ordinary caret movement keeps its previous ownership. */
    key(ui, CUI_KEY_UP, 0);
    key(ui, CUI_KEY_DOWN, 0);
    key(ui, CUI_KEY_HOME, 0);
    key(ui, CUI_KEY_END, 0);
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 40});
    cui_interact(ui, 1, CUI_ROLE_TEXTBOX, cui_string("Input"), false, false, 5);
    options.delegate_up_down = options.delegate_home_end = true;
    for (size_t i = 0; i < 4; ++i) {
        event = cui_base_input_reduce_event(ui, 1, edit, &state, options, i);
        assert(!event.changed && !cui_event_consumed(ui, i) &&
               cui_edit_get_state(edit).cursor == before.cursor);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    /* Existing readonly, IME and repeat guards remain in the shared reducer. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "ignored"}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .ime_composing = true}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                           .key = CUI_KEY_ENTER,
                                           .prefer_character_input = true}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    cui_input_result result =
        base_frame(ui, edit, &state, (cui_base_input_options){.readonly = true});
    assert(!result.changed && !result.submitted && !strcmp(buffer, "fresh"));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void clipboard_and_readonly(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    char buffer[128] = "caf\xc3\xa9";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 32});
    assert(ui && edit);
    cui_platform host = {
        .capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_set = copy, .clipboard_get = paste};
    cui_input_hit_geometry hit = {.ui = ui, .viewport = {10, 0, 200, 40}, .font_size = 10};
    assert(cui_base_input_hit_test(&hit, cui_string("ab"), 13, 0) == 1);
    assert(cui_base_input_hit_test(&hit, cui_string("ab"), -10, 0) == 0);
    assert(cui_base_input_hit_test(&hit, cui_string("ab"), 100, 0) == 2);
    assert(cui_base_input_hit_test(&hit, cui_string("\xc3"), 20, 0) == SIZE_MAX);
    cui_input_behavior_state state = {0};
    cui_base_input_options options = {.clipboard = &host, .required = true};
    base_frame(ui, edit, &state, options);
    cui_focus(ui, 1);
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    key(ui, CUI_KEY_X, CUI_MOD_CTRL);
    assert(base_frame(ui, edit, &state, options).changed);
    assert(!strcmp(clipboard, "caf\xc3\xa9") && !buffer[0]);
    key(ui, CUI_KEY_V, CUI_MOD_CTRL);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    assert(base_frame(ui, edit, &state, options).changed && !buffer[0]);
    key(ui, CUI_KEY_Z, CUI_MOD_META | CUI_MOD_SHIFT);
    assert(base_frame(ui, edit, &state, options).changed && !strcmp(buffer, clipboard));
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    reject_copy = true;
    key(ui, CUI_KEY_X, CUI_MOD_CTRL);
    cui_input_result result = base_frame(ui, edit, &state, options);
    assert(!result.changed && result.error == CUI_EDIT_CLIPBOARD_UNAVAILABLE &&
           !strcmp(buffer, clipboard));
    reject_copy = false;
    strcpy(clipboard, "\xc3");
    key(ui, CUI_KEY_V, CUI_MOD_CTRL);
    result = base_frame(ui, edit, &state, options);
    assert(!result.changed && result.error == CUI_EDIT_INVALID_UTF8 &&
           !strcmp(buffer, "caf\xc3\xa9"));
    options.readonly = options.invalid = true;
    key(ui, CUI_KEY_C, CUI_MOD_META);
    key(ui, CUI_KEY_BACKSPACE, 0);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "ignored"}));
    result = base_frame(ui, edit, &state, options);
    assert(!result.changed && result.error == CUI_EDIT_OK && !strcmp(buffer, clipboard));
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].details.readonly && semantics[0].details.required &&
           semantics[0].details.invalid);
    size_t readonly_cursor = cui_edit_get_state(edit).cursor;
    key(ui, CUI_KEY_UP, 0);
    result = base_frame(ui, edit, &state, options);
    /* Source single-line Input has no vertical rows, including read-only. */
    assert(!result.changed && cui_edit_get_state(edit).cursor == readonly_cursor);
    key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    result = base_frame(ui, edit, &state, options);
    assert(!result.changed && cui_edit_get_state(edit).cursor == 5 &&
           cui_edit_get_state(edit).anchor == 0);
    key(ui, CUI_KEY_LEFT, CUI_MOD_CTRL);
    result = base_frame(ui, edit, &state, options);
    assert(!result.changed && cui_edit_get_state(edit).cursor == 0);
    key(ui, CUI_KEY_RIGHT, CUI_MOD_CTRL);
    result = base_frame(ui, edit, &state, options);
    assert(!result.changed && cui_edit_get_state(edit).cursor == 5 &&
           !strcmp(buffer, "caf\xc3\xa9"));
    options.disabled = true;
    key(ui, CUI_KEY_LEFT, 0);
    assert(!base_frame(ui, edit, &state, options).changed && cui_edit_get_state(edit).cursor == 5);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static size_t token_hit(void *user, cui_str text, float x, float y) {
    (void)text;
    (void)x;
    (void)y;
    return *(size_t *)user;
}
static void token_pointer_selection(void) {
    cui_context *ui = cui_create((cui_config){0});
    char buffer[64] = "a @alice z";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_edit_token_span token = {
        2, 8, {cui_string("alice"), cui_string("@alice"), cui_string("Alice")}};
    assert(ui && edit && cui_edit_set_tokens(edit, &token, 1) == CUI_EDIT_OK);
    cui_input_behavior_state state = {0};
    size_t hit = 5;
    cui_base_input_options options = {.hit_test = token_hit, .hit_test_user = &hit};
    base_frame(ui, edit, &state, options);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 20, .y = 10}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 20, .y = 10}));
    assert(base_frame(ui, edit, &state, options).error == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 2 && cui_edit_get_state(edit).anchor == 2);
    assert(cui_edit_select_atomic(edit, 0, 0) == CUI_EDIT_OK);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 20, .y = 10, .modifiers = CUI_MOD_SHIFT}));
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 20, .y = 10, .modifiers = CUI_MOD_SHIFT}));
    assert(base_frame(ui, edit, &state, options).error == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 8);
    assert(cui_edit_get_content(edit).token_count == 1 && !strcmp(buffer, "a @alice z"));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
#ifndef CUI_INPUT_BASE_ONLY
static cui_size measure(void *user, cui_str text, float font_size) {
    (void)user;
    (void)font_size;
    float width = 0;
    for (size_t i = 0; i < text.length; ++i) {
        unsigned char c = (unsigned char)text.data[i];
        if ((c & 0xc0) == 0x80)
            continue;
        width += c == 'W' ? 20 : c == 'i' ? 4 : c >= 128 ? 12 : 10;
    }
    return (cui_size){width, 20};
}
static cui_draw_list styled_frame(cui_context *ui, cui_edit *edit, const cui_theme *theme,
                                  cui_input_state *state, cui_input_options options) {
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_input_result result = cui_input_ex(ui, 1, theme, cui_string("Name"), edit, state, options);
    assert(result.error == CUI_EDIT_OK);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static cui_rect text_bounds(cui_draw_list list, const char *text) {
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT &&
            list.commands[i].text.length == strlen(text) &&
            !memcmp(list.commands[i].text.data, text, strlen(text)))
            return list.commands[i].bounds;
    assert(false);
    return (cui_rect){0};
}
static void pointer(cui_context *ui, cui_event_type type, float x, float y, unsigned modifiers) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y, .modifiers = modifiers}));
}
static void single_line_placeholder_clipping(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    char buffer[128] = "";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    const char *placeholder = "A long placeholder with several words that must stay on one line";
    assert(ui && edit);
    for (unsigned populated = 0; populated < 3; ++populated) {
        if (populated == 1)
            assert(cui_edit_insert(edit, cui_string(placeholder)) == CUI_EDIT_OK);
        if (populated == 2) {
            assert(cui_edit_select(edit, 0, strlen(placeholder)) == CUI_EDIT_OK);
            assert(cui_edit_insert(edit, cui_string("")) == CUI_EDIT_OK);
        }
        assert(cui_begin_frame(ui, 400, 200, .016f));
        cui_begin_box(ui, 100, (cui_box){.width = 140, .fixed_width = true});
        assert(cui_input_ex(ui, 1, &theme, cui_string("Name"), edit, &state,
                            (cui_input_options){.width = populated == 2 ? -1 : 140,
                                                .placeholder = cui_string(placeholder)})
                   .error == CUI_EDIT_OK);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        cui_rect viewport = cui_bounds(ui, cui_input_viewport_id(1));
        assert(fabsf(viewport.w - 118) < .001f);
        unsigned lines = 0, clips = 0;
        bool clipped = false;
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *command = &list.commands[i];
            if (command->type == CUI_DRAW_CLIP_PUSH) {
                ++clips;
                if (fabsf(command->bounds.w - viewport.w) < .001f)
                    clipped = true;
            } else if (command->type == CUI_DRAW_CLIP_POP) {
                assert(clips);
                --clips;
            } else if (command->type == CUI_DRAW_TEXT) {
                ++lines;
                assert(clips && clipped);
                assert(command->text.length == strlen(placeholder));
                assert(!memcmp(command->text.data, placeholder, strlen(placeholder)));
                assert(command->bounds.w > 140);
                assert(fabsf(command->bounds.y - viewport.y) < .001f);
            }
        }
        assert(lines == 1 && clips == 0 && state.scroll_x == 0);
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void placeholder_alignment(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    char buffer[32] = "";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    assert(ui && edit);
    for (unsigned align = CUI_ALIGN_START; align <= CUI_ALIGN_END; ++align) {
        for (unsigned focused = 0; focused < 2; ++focused) {
            cui_focus(ui, focused ? 1 : 0);
            assert(cui_begin_frame(ui, 400, 100, .016f));
            assert(cui_input_ex(ui, 1, &theme, cui_string("Name"), edit, &state,
                                (cui_input_options){.width = 240,
                                                    .text_align = (cui_align)align,
                                                    .placeholder = cui_string("Your name")})
                       .error == CUI_EDIT_OK);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            cui_rect viewport = cui_bounds(ui, cui_input_viewport_id(1));
            cui_rect text = text_bounds(list, "Your name");
            float factor = align == CUI_ALIGN_CENTER ? .5f : align == CUI_ALIGN_END ? 1 : 0;
            assert(fabsf(text.x - (viewport.x + (viewport.w - text.w) * factor)) < .001f);
            if (focused) {
                cui_rect caret;
                assert(cui_text_input_rect(ui, &caret));
                assert(fabsf(caret.x - (viewport.x + (viewport.w - 1) * factor)) < .001f);
            }
        }
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void grow_resize_and_alignment(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    char buffer[32] = "abc";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    assert(ui && edit);
    const float widths[] = {400, 220, 500, 180};
    for (unsigned align = CUI_ALIGN_START; align <= CUI_ALIGN_END; ++align) {
        for (unsigned frame = 0; frame < 4; ++frame) {
            cui_input_options options = {.width = -1,
                                         .text_align = (cui_align)align,
                                         .custom_padding = true,
                                         .padding_x = frame * 4.0f};
            assert(cui_begin_frame(ui, widths[frame], 100, .016f));
            cui_begin_box(ui, 100, (cui_box){.width = widths[frame], .fixed_width = true});
            assert(cui_input_ex(ui, 1, &theme, cui_string("Name"), edit, &state, options).error ==
                   CUI_EDIT_OK);
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            cui_rect box = cui_bounds(ui, 1), viewport = cui_bounds(ui, cui_input_viewport_id(1));
            float inset = options.padding_x + 1;
            assert(fabsf(box.w - widths[frame]) < .001f);
            assert(fabsf(viewport.x - box.x - inset) < .001f);
            assert(fabsf(viewport.w - (box.w - 2 * inset)) < .001f);
            cui_rect bounds = text_bounds(list, buffer);
            float extra = viewport.w - bounds.w;
            float expected = viewport.x + (align == CUI_ALIGN_CENTER ? extra * .5f
                                           : align == CUI_ALIGN_END  ? extra
                                                                     : 0);
            assert(fabsf(bounds.x - expected) < .001f);
        }
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void paint_only_reduction(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200, .measure = measure});
    char buffer[32] = "ab";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    assert(ui && edit && cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    styled_frame(ui, edit, &theme, &state, (cui_input_options){0});
    cui_focus(ui, 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Q"}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_input_result reduced = cui_base_input_ex(ui, 1, cui_string("Name"), edit, &state.behavior,
                                                 (cui_base_input_options){0});
    assert(reduced.changed);
    cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &state, (cui_input_options){0});
    cui_end_frame(ui);
    assert(!strcmp(buffer, "aQb"));
    size_t count;
    assert(cui_semantics(ui, &count) && count == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "R"}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &state, (cui_input_options){0});
    cui_end_frame(ui);
    assert(!strcmp(buffer, "aQb") && cui_last_error(ui) == CUI_OK);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void styled_selection(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme_index = 0; theme_index < 4; ++theme_index) {
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 200, .measure = measure});
        char buffer[128] = "Wi\xc3\xa9x";
        cui_edit *edit =
            cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
        cui_input_state state = {0};
        cui_input_options options = {.width = 180};
        assert(ui && edit && cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
        cui_draw_list list = styled_frame(ui, edit, &themes[theme_index], &state, options);
        cui_rect bounds = text_bounds(list, buffer);
        assert(bounds.w == 46);
        pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + 21, bounds.y + 5, 0);
        pointer(ui, CUI_EVENT_POINTER_UP, bounds.x + 21, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(cui_edit_get_state(edit).cursor == 1 && cui_edit_get_state(edit).anchor == 1);
        pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + 35, bounds.y + 5, CUI_MOD_SHIFT);
        pointer(ui, CUI_EVENT_POINTER_UP, bounds.x + 35, bounds.y + 5, CUI_MOD_SHIFT);
        list = styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(cui_edit_get_state(edit).cursor == 4 && cui_edit_get_state(edit).anchor == 1);
        assert(text_bounds(list, buffer).w == 46);
        cui_input_metrics metrics;
        assert(cui_input_measure(options, themes[theme_index].font_size, &metrics));
        bool selection_paint = false, caret_paint = false;
        for (size_t i = 0; i < list.count; ++i) {
            cui_draw_command command = list.commands[i];
            if (command.type == CUI_DRAW_RECT && command.bounds.w == 16 &&
                !memcmp(&command.color, &themes[theme_index].selection, sizeof command.color))
                selection_paint = true;
            if (command.type == CUI_DRAW_RECT && command.bounds.w == 1 &&
                command.bounds.h == metrics.line_height * .85f)
                caret_paint = true;
        }
        assert(selection_paint && caret_paint);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Q"}));
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(!strcmp(buffer, "WQx"));
        pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + 1, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        pointer(ui, CUI_EVENT_POINTER_MOVE, bounds.x + 40, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        pointer(ui, CUI_EVENT_POINTER_UP, bounds.x + 40, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 3 &&
               !state.behavior.dragging);
        assert(cui_edit_insert(edit, cui_string("WWWWWWWWWWWW")) == CUI_EDIT_OK);
        options.width = 100;
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(state.scroll_x > 0 && isfinite(state.scroll_x));
        key(ui, CUI_KEY_HOME, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(state.scroll_x == 0 && cui_edit_get_state(edit).cursor == 0);
        cui_edit_destroy(edit);
        cui_destroy(ui);
    }
}
#endif
static void input_consumes_caret_keys(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    char buffer[32] = "abc";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_behavior_state state = {0};
    assert(ui && edit);
    base_frame(ui, edit, &state, (cui_base_input_options){0});
    cui_focus(ui, 1);
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    for (unsigned readonly = 0; readonly < 2; ++readonly) {
        key(ui, CUI_KEY_LEFT, 0);
        assert(cui_begin_frame(ui, 400, 200, .016f));
        assert(cui_begin_focus_scope(ui, 90, false));
        cui_base_input_ex(ui, 1, cui_string("Input"), edit, &state,
                          (cui_base_input_options){.readonly = readonly != 0});
        assert(cui_event_consumed(ui, 0));
        /* Even an arrow at the start is handled by the editor. */
        assert(cui_edit_get_state(edit).cursor == 0);
        cui_end_focus_scope(ui);
        cui_end_frame(ui);
    }
    key(ui, CUI_KEY_RIGHT, 0);
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(cui_edit_apply_events(edit, ui, 1, false) == CUI_EDIT_OK);
    assert(cui_event_consumed(ui, 0) && cui_edit_get_state(edit).cursor == 1);
    assert(cui_edit_apply_events(edit, ui, 1, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 1);
    cui_end_frame(ui);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void consumed_release_ends_selection(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    char buffer[32] = "abc";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_behavior_state state = {0};
    assert(ui && edit);
    base_frame(ui, edit, &state, (cui_base_input_options){0});
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 10, .y = 10}));
    base_frame(ui, edit, &state, (cui_base_input_options){0});
    assert(state.dragging);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 20, .y = 10}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(cui_consume_event(ui, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 40});
    cui_base_input_ex(ui, 1, cui_string("Input"), edit, &state, (cui_base_input_options){0});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(!state.dragging && !cui_pressed(ui, 1));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
#ifndef CUI_INPUT_BASE_ONLY
static void size_metrics_and_centering(void) {
    cui_theme theme = cui_theme_light();
    theme.font_size = 16;
    theme.caret = (cui_color){7, 31, 199, 255};
    theme.selection = (cui_color){99, 18, 71, 255};
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    char buffer[64] = "abc";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
    assert(edit && ui);
    cui_input_state state = {0};
    const float heights[] = {32, 24, 44, 20}, fonts[] = {14, 14, 16, 12},
                padding[] = {10, 8, 12, 4};
    for (unsigned i = 0; i < 4; i++) {
        cui_input_options options = {.size = (cui_input_size)i, .width = 180};
        cui_input_metrics metrics;
        assert(cui_input_measure(options, 16, &metrics));
        assert(metrics.height == heights[i] && metrics.font_size == fonts[i] &&
               metrics.padding_x == padding[i] && metrics.line_height == 20);
        cui_draw_list list = styled_frame(ui, edit, &theme, &state, options);
        cui_rect box = cui_bounds(ui, 1), text = text_bounds(list, buffer),
                 viewport = cui_bounds(ui, cui_input_viewport_id(1));
        assert(box.h == heights[i] &&
               fabsf(text.y + cui_measure_text(ui, cui_string(buffer), fonts[i]).height * .5f -
                     (box.y + box.h * .5f)) < .001f);
        assert(viewport.x == box.x + padding[i] + 1 && viewport.h == 20);
        assert(text.x == viewport.x && text.h == 20);
        for (size_t j = 0; j < list.count; j++)
            assert(list.commands[j].type != CUI_DRAW_TEXT || list.commands[j].text.length != 4 ||
                   memcmp(list.commands[j].text.data, "Name", 4));
        cui_focus(ui, 1);
        cui_edit_select(edit, 1, 3);
        list = styled_frame(ui, edit, &theme, &state, options);
        bool caret = false, selection = false;
        for (size_t j = 0; j < list.count; j++) {
            cui_draw_command c = list.commands[j];
            caret |=
                c.type == CUI_DRAW_RECT && c.bounds.w == 1 && c.bounds.h == 17 && c.color.b == 199;
            selection |= c.type == CUI_DRAW_RECT && c.color.r == 99 && c.bounds.h == 20;
        }
        assert(caret && selection);
        cui_edit_select(edit, 0, 0);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Q"}));
        styled_frame(ui, edit, &theme, &state, options);
        assert(buffer[0] == 'Q');
        cui_edit_select(edit, 0, 1);
        cui_edit_backspace(edit);
        assert(strcmp(buffer, "abc") == 0);
    }
    cui_input_metrics metrics;
    assert(cui_input_measure((cui_input_options){.size = CUI_INPUT_CUSTOM, .custom_size = 30}, 16,
                             &metrics) &&
           metrics.height == 30 && metrics.font_size == 26.25f);
    assert(!cui_input_measure((cui_input_options){.size = CUI_INPUT_CUSTOM}, 16, &metrics));
    assert(!cui_input_measure((cui_input_options){.density = NAN}, 16, &metrics));
    assert(!cui_input_measure((cui_input_options){.custom_padding = true, .padding_x = NAN}, 16,
                              &metrics));
    assert(!cui_input_measure((cui_input_options){.custom_padding = true, .padding_y = -1}, 16,
                              &metrics));
    cui_input_options zero_padding = {.width = 180, .no_appearance = true, .custom_padding = true};
    assert(cui_input_measure(zero_padding, 16, &metrics) && metrics.padding_x == 0 &&
           metrics.padding_y == 0);
    styled_frame(ui, edit, &theme, &state, zero_padding);
    cui_rect zero_box = cui_bounds(ui, 1);
    cui_rect zero_viewport = cui_bounds(ui, cui_input_viewport_id(1));
    assert(zero_viewport.x == zero_box.x && zero_viewport.w == zero_box.w);
    cui_input_options custom_padding = {.width = 180,
                                        .no_appearance = true,
                                        .custom_padding = true,
                                        .padding_x = 7,
                                        .padding_y = 3};
    assert(cui_input_measure(custom_padding, 16, &metrics) && metrics.padding_x == 7 &&
           metrics.padding_y == 3);
    styled_frame(ui, edit, &theme, &state, custom_padding);
    cui_rect custom_box = cui_bounds(ui, 1);
    cui_rect custom_viewport = cui_bounds(ui, cui_input_viewport_id(1));
    assert(custom_viewport.x == custom_box.x + 7 && custom_viewport.w == custom_box.w - 14);
    cui_edit_select(edit, 0, 0);
    cui_input_options center = {.width = 180, .text_align = CUI_ALIGN_CENTER};
    cui_draw_list list = styled_frame(ui, edit, &theme, &state, center);
    cui_rect bounds = text_bounds(list, buffer);
    assert(fabsf(bounds.x + bounds.w / 2 - 90) < .001f);
    pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + bounds.w, bounds.y + 2, 0);
    pointer(ui, CUI_EVENT_POINTER_UP, bounds.x + bounds.w, bounds.y + 2, 0);
    styled_frame(ui, edit, &theme, &state, center);
    assert(cui_edit_get_state(edit).cursor == 3);
    theme.input = (cui_color){17, 71, 143, 200};
    theme.input_background = (cui_color){1, 2, 3, 255};
    theme.muted = (cui_color){132, 129, 113, 255};
    list =
        styled_frame(ui, edit, &theme, &state, (cui_input_options){.width = 180, .disabled = true});
    bool disabled_background = false, disabled_foreground = false;
    for (size_t j = 0; j < list.count; j++) {
        cui_draw_command c = list.commands[j];
        disabled_background |=
            c.type == CUI_DRAW_RECT && c.bounds.w == 180 && c.color.r == 17 && c.color.a == 40;
        disabled_foreground |= c.type == CUI_DRAW_TEXT && c.color.r == 132 && c.color.a == 128;
    }
    assert(disabled_background && disabled_foreground);
    list = styled_frame(ui, edit, &theme, &state, (cui_input_options){.no_appearance = true});
    for (size_t j = 0; j < list.count; j++)
        assert(list.commands[j].type != CUI_DRAW_BORDER);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
/* Portable host contract test: a deliberately reversed ASCII geometry isolates
 * widget routing/lifetime from fonts. Native integration tests use real glyphs. */
typedef struct {
    unsigned creates, releases;
    int mode;
    const char *editor_buffer;
} visual_host;
typedef struct {
    visual_host *host;
    size_t length;
    float unit;
    char *text;
} visual_session;
static cui_text_layout_status host_carets(const void *user, size_t byte, cui_text_caret *out,
                                          size_t capacity, size_t *count) {
    const visual_session *session = user;
    if (!count || byte > session->length || (session->host->mode == 3 && byte == 1))
        return CUI_TEXT_LAYOUT_INVALID;
    size_t needed = byte && byte < session->length ? 2 : session->length ? 1 : 2;
    if (!out && !capacity) {
        *count = needed;
        return CUI_TEXT_LAYOUT_OK;
    }
    if (!out || capacity < needed)
        return CUI_TEXT_LAYOUT_CAPACITY;
    for (size_t i = 0; i < needed; ++i)
        out[i] =
            (cui_text_caret){byte,
                             0,
                             byte == session->length || i ? CUI_TEXT_UPSTREAM : CUI_TEXT_DOWNSTREAM,
                             (session->length - byte) * session->unit,
                             0,
                             20};
    *count = needed;
    return CUI_TEXT_LAYOUT_OK;
}
static cui_text_layout_status host_hit(const void *user, float x, float y,
                                       cui_text_affinity preferred, cui_text_caret *out) {
    const visual_session *session = user;
    if (!out || !isfinite(x) || !isfinite(y))
        return CUI_TEXT_LAYOUT_INVALID;
    size_t byte = x <= 0 ? session->length
                  : x >= session->length * session->unit
                      ? 0
                      : (size_t)lroundf(session->length - x / session->unit);
    *out = (cui_text_caret){byte, 0, preferred, (session->length - byte) * session->unit, 0, 20};
    if (session->host->mode == 5)
        out->x = NAN;
    return CUI_TEXT_LAYOUT_OK;
}
static cui_text_layout_status host_selection(const void *user, size_t start, size_t end,
                                             cui_text_selection_rect *out, size_t capacity,
                                             size_t *count) {
    const visual_session *session = user;
    if (!count || start > end || end > session->length ||
        (session->host->mode == 3 && (start == 1 || end == 1)))
        return CUI_TEXT_LAYOUT_INVALID;
    size_t needed = start == end ? 0 : session->host->mode == 4 ? 5000 : 1;
    if (!out && !capacity) {
        *count = needed;
        return CUI_TEXT_LAYOUT_OK;
    }
    if (capacity < needed)
        return CUI_TEXT_LAYOUT_CAPACITY;
    if (needed)
        out[0] = (cui_text_selection_rect){
            {(session->length - end) * session->unit, 0, (end - start) * session->unit, 20},
            start,
            end,
            0};
    *count = needed;
    return CUI_TEXT_LAYOUT_OK;
}
static void host_release(void *user) {
    visual_session *session = user;
    ++session->host->releases;
    free(session->text);
    free(session);
}
static cui_text_layout_status host_factory(void *user, cui_str text, float size,
                                           cui_text_style style, cui_text_geometry *out) {
    visual_host *host = user;
    ++host->creates;
    if (host->editor_buffer)
        assert(text.data != host->editor_buffer);
    assert(style.weight == CUI_TEXT_WEIGHT_NORMAL && !style.italic && !style.family);
    static const cui_text_geometry_ops ops = {host_carets, host_hit, host_selection, host_release};
    visual_session *session = calloc(1, sizeof *session);
    assert(session);
    session->host = host;
    session->length = text.length;
    session->unit = size * .5f;
    session->text = malloc(text.length + 1);
    assert(session->text);
    if (text.length)
        memcpy(session->text, text.data, text.length);
    session->text[text.length] = 0;
    *out = (cui_text_geometry){session, &ops, text.length * session->unit, 20, text.length};
    if (host->mode == 1)
        out->width = NAN;
    if (host->mode == 2)
        return CUI_TEXT_LAYOUT_CAPACITY; /* Defensive partial factory cleanup. */
    return CUI_TEXT_LAYOUT_OK;
}
static cui_size visual_measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){text.length * size * .5f, 20};
}
static void visual_host_contract(void) {
    visual_host host = {0};
    cui_context *ui = cui_create((cui_config){
        .measure = visual_measure, .measure_user = &host, .text_geometry = host_factory});
    assert(ui);
    cui_text_geometry geometry, before;
    memset(&geometry, 0x37, sizeof geometry);
    before = geometry;
    host.mode = 1;
    assert(cui_get_text_geometry(ui, cui_string("abc"), 14, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(!memcmp(&geometry, &before, sizeof geometry) && host.creates == 1 && host.releases == 1);
    host.mode = 2;
    assert(cui_get_text_geometry(ui, cui_string("abc"), 14, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_CAPACITY);
    assert(!memcmp(&geometry, &before, sizeof geometry) && host.creates == 2 && host.releases == 2);
    assert(cui_get_text_geometry(ui, cui_string("abc"), INFINITY, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(host.creates == 2 && host.releases == 2);
    host.mode = 0;
    assert(cui_get_text_geometry(ui, cui_string(""), 14, (cui_text_style){0}, &geometry) ==
           CUI_TEXT_LAYOUT_OK);
    cui_release_text_geometry(&geometry);
    cui_release_text_geometry(&geometry);
    assert(host.creates == 3 && host.releases == 3);
    char a[32] = "abc", b[32] = "wxy";
    cui_edit *first = cui_edit_create((cui_edit_config){.buffer = a, .capacity = sizeof a});
    cui_edit *second = cui_edit_create((cui_edit_config){.buffer = b, .capacity = sizeof b});
    assert(first && second);
    assert(cui_edit_select_atomic(first, 0, 0) == CUI_EDIT_OK);
    assert(cui_edit_select_atomic(second, 0, 0) == CUI_EDIT_OK);
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    for (unsigned frame = 0; frame < 4; ++frame) {
        cui_edit *edit = frame < 2 ? first : second;
        host.editor_buffer = frame < 2 ? a : b;
        if (frame)
            cui_focus(ui, 1);
        if (frame == 3)
            theme.font_size = 32;
        unsigned previous = host.creates;
        assert(cui_begin_frame(ui, 400, 100, .016f));
        assert(cui_input_ex(ui, 1, &theme, cui_string("Visual"), edit, &state,
                            (cui_input_options){.width = 200})
                   .error == CUI_EDIT_OK);
        cui_end_frame(ui);
        assert(host.creates == previous + 1 && host.releases + 1 == host.creates);
        if (frame) {
            cui_rect ime, viewport = cui_bounds(ui, cui_input_viewport_id(1));
            assert(cui_text_input_rect(ui, &ime));
            assert(fabsf(ime.x - viewport.x + state.scroll_x -
                         strlen(frame < 2 ? a : b) * 7 * theme.font_size / 16) < .01f);
        }
    }
    /* A shaped-interior failure preserves logical editing selection; the entire
     * line uses legacy geometry, rather than silently changing the byte offset. */
    host.mode = 3;
    assert(cui_edit_select_atomic(second, 1, 1) == CUI_EDIT_OK);
    assert(cui_begin_frame(ui, 400, 100, .016f));
    cui_input_ex(ui, 1, &theme, cui_string("Visual"), second, &state,
                 (cui_input_options){.width = 200});
    cui_end_frame(ui);
    assert(cui_edit_get_state(second).cursor == 1);
    cui_rect ime, viewport = cui_bounds(ui, cui_input_viewport_id(1));
    assert(cui_text_input_rect(ui, &ime));
    assert(fabsf(ime.x - viewport.x + state.scroll_x - 14) < .01f);
    host.mode = 4;
    assert(cui_edit_select_atomic(second, 0, 2) == CUI_EDIT_OK);
    assert(cui_begin_frame(ui, 400, 100, .016f));
    cui_input_ex(ui, 1, &theme, cui_string("Visual"), second, &state,
                 (cui_input_options){.width = 200});
    cui_end_frame(ui);
    assert(cui_edit_get_state(second).anchor == 0 && cui_edit_get_state(second).cursor == 2);
    host.mode = 5;
    cui_text_geometry invalid_hit;
    host.editor_buffer = NULL;
    assert(cui_get_text_geometry(ui, cui_string(b), 28, (cui_text_style){0}, &invalid_hit) ==
           CUI_TEXT_LAYOUT_OK);
    cui_text_affinity affinity = CUI_TEXT_UPSTREAM;
    cui_input_visual_hit_geometry hit = {{.ui = ui, .font_size = 28}, &invalid_hit, &affinity};
    assert(cui_base_input_visual_hit_test(&hit, cui_string(b), 0, 0) == SIZE_MAX);
    assert(affinity == CUI_TEXT_UPSTREAM);
    cui_release_text_geometry(&invalid_hit);
    host.editor_buffer = b;
    host.mode = 0;
    viewport = cui_bounds(ui, cui_input_viewport_id(1));
    pointer(ui, CUI_EVENT_POINTER_DOWN, viewport.x + .01f, viewport.y + 2, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"}));
    pointer(ui, CUI_EVENT_POINTER_UP, viewport.x + .01f, viewport.y + 2, 0);
    unsigned creates = host.creates;
    assert(cui_begin_frame(ui, 400, 100, .016f));
    assert(cui_input_ex(ui, 1, &theme, cui_string("Visual"), second, &state,
                        (cui_input_options){.width = 200})
               .error == CUI_EDIT_OK);
    cui_end_frame(ui);
    assert(!strcmp(b, "wxyz") && cui_edit_get_state(second).cursor == 4);
    assert(host.creates == creates + 2 && host.releases + 1 == host.creates);
    for (unsigned omitted = 0; omitted < 2; ++omitted) {
        assert(cui_begin_frame(ui, 400, 100, .016f));
        cui_end_frame(ui);
        assert(host.creates == host.releases);
    }
    /* Context destructor owns final session; models/state require no geometry cleanup. */
    cui_edit_destroy(first);
    cui_edit_destroy(second);
    cui_destroy(ui);
    assert(host.creates == host.releases);
}
#endif
int main(void) {
    single_event_reduction();
    token_pointer_selection();
#ifndef CUI_INPUT_BASE_ONLY
    size_metrics_and_centering();
    grow_resize_and_alignment();
    single_line_placeholder_clipping();
    placeholder_alignment();
#endif
    consumed_release_ends_selection();
    input_consumes_caret_keys();
    clipboard_and_readonly();
#ifndef CUI_INPUT_BASE_ONLY
    paint_only_reduction();
    styled_selection();
    visual_host_contract();
#endif
    return 0;
}
