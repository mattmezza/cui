/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
static unsigned instances;
static void refine(void *user, const cui_button_style_info *info,
                   const cui_button_style_paint *current, cui_button_paint_patch *patch) {
    CHECK(user == &instances && current && info->id == 2);
    CHECK(info->phase == CUI_BUTTON_STYLE_INSTANCE);
    instances++;
    patch->fields = CUI_BUTTON_PAINT_BACKGROUND;
    patch->paint.root.background = (cui_color){10, 80, 140, 255};
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 180});
    CHECK(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_tree_state menu;
    cui_menu_tree_init(&menu, 10);
    cui_id command = 0;
    const cui_menu_entry entries[] = {{.id = 12, .label = {"Run", 3}, .kind = CUI_MENU_ACTION}};
    const cui_dropdown_button_styles styles = {.paint_policy = CUI_DROPDOWN_BUTTON_PAINT_BUTTON,
                                               .action = {.instance = refine, .user = &instances}};
    for (unsigned pass = 0; pass < 2; pass++) {
        if (pass)
            CHECK(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
        CHECK(cui_begin_frame(ui, 320, 180, .016f));
        const cui_dropdown_button_metadata metadata = {.action = {.custom_name = true,
                                                                  .name = {"Save document", 13},
                                                                  .custom_tab_index = true,
                                                                  .tab_index = -3},
                                                       .trigger = {.skip_tab = true},
                                                       .toggle_semantics = true,
                                                       .toggled = true};
        cui_dropdown_button_result result = cui_dropdown_button_with_metadata_ex(
            ui, 1, 2, 3, &theme, cui_string("Save"), cui_string("More"), entries, 1, &command,
            &menu, (cui_dropdown_button_options){.width = 120}, &styles, &metadata);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        bool found = false;
        for (size_t i = 0; i < count; i++)
            if (sem[i].id == 2) {
                found = true;
                CHECK(sem[i].checked && sem[i].value == 1 && sem[i].tab_index == -3 &&
                      sem[i].name.length == 13);
            }
        CHECK(found);
        cui_draw_list draw = cui_end_frame(ui);
        CHECK(cui_last_error(ui) == CUI_OK && result.action_activated == (pass != 0));
        CHECK(instances == pass + 1);
        bool painted = false;
        for (size_t i = 0; i < draw.count; i++)
            if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == 10 &&
                draw.commands[i].color.g == 80 && draw.commands[i].color.b == 140)
                painted = true;
        CHECK(painted);
    }
    cui_destroy(ui);
    return 0;
}
