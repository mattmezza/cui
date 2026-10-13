/* SPDX-License-Identifier: MIT */
#define probe_frame inherited_probe_frame
#include "button_blocker_probe.c"
#undef probe_frame
#include <string.h>
static float available = 80;
static const char label[] = "A long label that needs automatic truncation";
void label_width(float width) {
    available = width;
}
int probe_frame(void) {
    cui_theme theme = cui_theme_light();
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(ui, 1,
                  (cui_box){.width = available, .height = 80, .background = {245, 245, 245, 255}});
    cui_button_ex(ui, 20, &theme, cui_string(label),
                  (cui_button_options){.width = mode ? -1 : available,
                                       .on_click = child_click,
                                       .action_user = &children});
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    return cui_last_error(ui);
}
int label_semantic(void) {
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (s[i].id == 20)
            return s[i].name.length == sizeof label - 1 &&
                   !memcmp(s[i].name.data, label, sizeof label - 1);
    return 0;
}
