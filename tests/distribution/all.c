/* Every portable implementation, including standalone optional models. */
#define CUI_IMPLEMENTATION_ALL
#include "cui_all.h"
#include "cui_all.h"

int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    if (!ui || !cui_begin_frame(ui, 200, 100, 0))
        return 1;
    cui_theme theme = cui_theme_light();
    cui_begin_box(ui, cui_id_from(ui, "root"), (cui_box){.width = -1, .height = -1});
    cui_sidebar_menu(ui, cui_id_from(ui, "menu"), &theme, NULL, 0, false);
    cui_button(ui, cui_id_from(ui, "button"), &theme, cui_string("All layers"), false);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    int failed = !list.count || cui_last_error(ui) != CUI_OK;
    cui_destroy(ui);
    return failed;
}
