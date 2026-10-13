/* SPDX-License-Identifier: MIT */
/* Headless viewport benchmark. Compile with -O2 and the portable CUI library. */
#include "cui/cui_textarea.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    size_t calls;
} measure_probe;

static cui_size measure(void *user, cui_str text, float size, cui_text_style style) {
    measure_probe *probe = user;
    (void)style;
    ++probe->calls;
    return (cui_size){text.length * 7.0f, size};
}

static double elapsed_ms(clock_t start) {
    return 1000.0 * (double)(clock() - start) / CLOCKS_PER_SEC;
}

static void sample(const char *label, cui_context *ui, cui_edit *edit, cui_textarea_state *view,
                   measure_probe *probe) {
    cui_theme theme = cui_theme_light();
    cui_textarea_options options = {.width = 200, .height = 80};
    size_t before = probe->calls;
    clock_t start = clock();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_textarea_result result =
        cui_textarea(ui, 10, &theme, cui_string("200K editor"), edit, view, options);
    cui_draw_list commands = cui_end_frame(ui);
    assert(result.error == CUI_TEXTAREA_OK && cui_last_error(ui) == CUI_OK);
    printf("%s: cpu_ms=%.3f measure_calls=%zu draw_commands=%zu rows=%zu\n", label,
           elapsed_ms(start), probe->calls - before, commands.count, view->count);
}

int main(void) {
    const size_t rows = 200000, bytes = 2 * rows;
    printf("workspace_bytes=%zu document_bytes=%zu\n", (rows + 1) * sizeof(cui_textarea_line),
           bytes);
    char *buffer = malloc(bytes + 64);
    cui_textarea_line *lines = malloc((rows + 1) * sizeof(*lines));
    assert(buffer && lines);
    for (size_t i = 0; i < rows; ++i) {
        buffer[2 * i] = 'a';
        buffer[2 * i + 1] = '\n';
    }
    buffer[bytes] = 0;
    cui_edit *edit = cui_edit_create((cui_edit_config){
        .buffer = buffer, .capacity = bytes + 64, .multiline = true});
    cui_textarea_state view;
    assert(edit && cui_textarea_state_init(&view, lines, rows + 1));
    measure_probe probe = {0};
    cui_context *ui = cui_create((cui_config){
        .width = 400, .height = 300, .measure_styled = measure, .measure_user = &probe});
    assert(ui);
    cui_focus(ui, 10);
    sample("cold", ui, edit, &view, &probe);
    sample("steady", ui, edit, &view, &probe);
    assert(cui_edit_select(edit, bytes - 2, bytes - 2) == CUI_EDIT_OK);
    sample("caret-tail", ui, edit, &view, &probe);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"}));
    sample("edit-tail", ui, edit, &view, &probe);
    assert(buffer[bytes - 2] == 'z');
    sample("steady-after-edit", ui, edit, &view, &probe);
    cui_destroy(ui);
    cui_edit_destroy(edit);
    free(lines);
    free(buffer);
    return 0;
}
