/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
static cui_context *ui;
static unsigned activations, frames;
static bool opt = true;
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){text.length * size / 2, size};
}
int probe_init(void) {
    ui = cui_create((cui_config){.max_elements = 32, .max_commands = 32,
                                 .frame_bytes = 4096, .measure = measure});
    return ui ? 0 : 1;
}
int probe_event(int down, int key, unsigned modifiers, int repeat) {
    return cui_queue_event(ui, (cui_event){.type = down ? CUI_EVENT_KEY_DOWN : CUI_EVENT_KEY_UP,
                                           .key = (cui_key)key, .modifiers = modifiers,
                                           .repeat = repeat != 0}) ? 0 : 1;
}
void probe_focus(int id) { cui_focus(ui, (cui_id)id); }
void probe_option(int enabled) { opt = enabled != 0; }
int probe_frame(void) {
    if (!cui_begin_frame(ui, 200, 100, 0)) return 1;
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 80, 30});
    cui_interaction result = cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Button"),
                                          false, false, 0);
    activations += result.activated;
    cui_set_semantic_details(ui, 10,
                             (cui_semantic_details){.release_key_activation = opt});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    frames++;
    return cui_last_error(ui);
}
unsigned probe_stat(int kind) {
    return kind == 0 ? activations : kind == 1 ? frames : (unsigned)cui_focused(ui);
}
