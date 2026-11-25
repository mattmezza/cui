/* SPDX-License-Identifier: MIT */
#include <cui/cui_time_field_v1.h>
#include <stdio.h>

enum { START = 10, END = 20, SAVE = 30 };

typedef struct {
    cui_time start, end;
    cui_time_field_options_v1 options;
    cui_time_field_state_v1 start_state, end_state;
    unsigned changes, saves;
} schedule;

static void changed(void *user, const cui_time_change *event) {
    schedule *form = user;
    ++form->changes;
    printf("Field %llu now %02u:%02u\n", (unsigned long long)event->event.target, event->value.hour,
           event->value.minute);
}

static bool draw(cui_context *ui, const cui_theme *theme, schedule *form) {
    cui_time_field_frame_entry_v1 fields[] = {
        {START, &form->start_state, &form->start, &form->options},
        {END, &form->end_state, &form->end, &form->options},
    };
    if (cui_begin_time_field_frame_v1(ui, 480, 180, 0, fields, 2) != CUI_TIME_TEXT_OK)
        return false;
    cui_time_handler handler = {changed, form};
    cui_begin_box(ui, 100, (cui_box){.padding = 12, .gap = 8});
    cui_time_field_result_v1 start = cui_time_field_from_frame_v1(
        ui, START, theme, cui_string("Start time"), CUI_DESIGN_BASE, handler);
    cui_time_field_result_v1 end = cui_time_field_from_frame_v1(
        ui, END, theme, cui_string("End time"), CUI_DESIGN_BASE, handler);
    cui_interaction save =
        cui_interact(ui, SAVE, CUI_ROLE_BUTTON, cui_string("Save schedule"), false, false, 0);
    cui_begin_box(ui, SAVE,
                  (cui_box){.height = 32,
                            .padding = 8,
                            .align_y = CUI_ALIGN_CENTER,
                            .background = theme->input_background});
    cui_text(ui, cui_string("Save schedule"), theme->font_size, theme->text);
    cui_end_box(ui);
    cui_end_box(ui);
    if (save.activated)
        ++form->saves;
    (void)cui_end_frame(ui);
    return start.status == CUI_TIME_TEXT_OK && end.status == CUI_TIME_TEXT_OK &&
           cui_last_error(ui) == CUI_OK;
}

int main(void) {
    schedule form = {.start = {9, 30, 0}, .end = {10, 45, 0}};
    if (cui_time_field_options_init_v1(&form.options) != CUI_TIME_TEXT_OK ||
        cui_time_field_state_init_v1(&form.start_state, form.start, &form.options) !=
            CUI_TIME_TEXT_OK ||
        cui_time_field_state_init_v1(&form.end_state, form.end, &form.options) != CUI_TIME_TEXT_OK)
        return 1;
    cui_context *ui = cui_create((cui_config){.width = 480, .height = 180});
    if (!ui)
        return 1;
    cui_theme theme = cui_theme_light();
    bool ok = draw(ui, &theme, &form); /* Publish initial geometry. */
    if (ok) {
        cui_focus(ui, START);
        ok = cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "3"}) &&
             cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}) &&
             cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "4"}) &&
             cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}) &&
             cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}) &&
             draw(ui, &theme, &form);
    }
    printf("Changes %u, saves %u\n", form.changes, form.saves);
    cui_destroy(ui);
    return ok && form.start.hour == 3 && form.end.hour == 4 && form.changes == 2 && form.saves == 1
               ? 0
               : 1;
}
