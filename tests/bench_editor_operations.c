/* SPDX-License-Identifier: MIT */
/* Standalone portable C11 headless benchmark; assertions must remain enabled. */
#ifdef NDEBUG
#error "Build this benchmark with -UNDEBUG"
#endif
#include "cui/cui_editor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { LINE_BYTES = 80, SAMPLES = 11 };
typedef struct { size_t measures, highlights; } probe;
static cui_size measure(void *user, cui_str text, float size, cui_text_style style) {
    probe *p = user;
    (void)style;
    ++p->measures;
    return (cui_size){(float)text.length * 7, size};
}
static size_t highlight(void *user, cui_str language, cui_str text, size_t start,
                        size_t end, cui_color *color) {
    probe *p = user;
    (void)language; (void)text;
    assert(start < end);
    ++p->highlights;
    *color = (cui_color){80, 140, 200, 255};
    return end;
}
static void frame(const char *label, size_t rows, cui_context *ui,
                  cui_editor_state *state, probe *p) {
    cui_theme theme = cui_theme_light();
    cui_editor_options options = cui_editor_default();
    options.textarea.width = 800;
    options.textarea.height = 400;
    options.highlight = highlight;
    options.highlight_user = p;
    size_t measures = p->measures, highlights = p->highlights;
    clock_t start = clock();
    assert(start != (clock_t)-1);
    assert(cui_begin_frame(ui, 900, 500, .016f));
    cui_textarea_result result = cui_editor(ui, 10, &theme, cui_string("Benchmark"), state, options);
    cui_draw_list commands = cui_end_frame(ui);
    clock_t end = clock();
    assert(end != (clock_t)-1);
    assert(result.error == CUI_TEXTAREA_OK && cui_last_error(ui) == CUI_OK);
    assert(state->view.count == rows + 1 && commands.count > 0);
    printf("%zu,%s,%.6f,%zu,%zu,%zu\n", rows, label,
           1000.0 * (double)(end - start) / CLOCKS_PER_SEC,
           p->measures - measures, p->highlights - highlights, commands.count);
}
static void run(size_t rows) {
    const size_t bytes = LINE_BYTES * rows;
    char *buffer = malloc(bytes + 64);
    cui_textarea_line *lines = malloc((rows + 1) * sizeof(*lines));
    assert(buffer && lines);
    for (size_t i = 0; i < rows; ++i) {
        memset(buffer + i * LINE_BYTES, 'a', LINE_BYTES - 1);
        buffer[(i + 1) * LINE_BYTES - 1] = '\n';
    }
    buffer[bytes] = 0;
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
        .capacity = bytes + 64, .multiline = true});
    cui_editor_state state;
    assert(edit && cui_editor_state_init(&state, edit, lines, rows + 1));
    probe p = {0};
    cui_context *ui = cui_create((cui_config){.width = 900, .height = 500,
        .measure_styled = measure, .measure_user = &p});
    assert(ui);
    cui_focus(ui, 10);
    fprintf(stderr, "rows=%zu document_capacity=%zu row_storage=%zu\n", rows,
            bytes + 64, (rows + 1) * sizeof(*lines));
    frame("cold", rows, ui, &state, &p);
    for (size_t i = 0; i < SAMPLES; ++i) {
        frame("idle", rows, ui, &state, &p);
        assert(cui_edit_select(edit, bytes - LINE_BYTES + i, bytes - LINE_BYTES + i) == CUI_EDIT_OK);
        frame("caret_tail", rows, ui, &state, &p);
        size_t cursor = cui_edit_get_state(edit).cursor;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
        frame("key_left", rows, ui, &state, &p);
        assert(cui_edit_get_state(edit).cursor == cursor - 1);
        float old_scroll = state.view.scroll_y;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
            .x = state.view.viewport.x + 1, .y = state.view.viewport.y + 1,
            .dy = 3, .scroll_precise = true}));
        frame("scroll", rows, ui, &state, &p);
        assert(state.view.scroll_y < old_scroll);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
            .x = state.view.viewport.x + 20, .y = state.view.viewport.y + 20}));
        frame("pointer_move", rows, ui, &state, &p);
        size_t at = i % 2 ? bytes / 2 : bytes - LINE_BYTES;
        assert(cui_edit_select(edit, at, at) == CUI_EDIT_OK);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"}));
        frame(i % 2 ? "insert_middle" : "insert_tail", rows, ui, &state, &p);
        assert(buffer[at] == 'z' && cui_edit_get_state(edit).length == bytes + 1);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_BACKSPACE}));
        frame(i % 2 ? "remove_middle" : "remove_tail", rows, ui, &state, &p);
        assert(buffer[at] == 'a' && cui_edit_get_state(edit).length == bytes);
    }
    cui_destroy(ui);
    cui_edit_destroy(edit);
    free(lines);
    free(buffer);
}
int main(void) {
    printf("rows,operation,cpu_ms,measure_calls,highlight_calls,draw_commands\n");
    run(2000); run(20000); run(200000);
    return 0;
}
