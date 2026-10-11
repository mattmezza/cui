#include "cui.h"
#ifdef CUI_DISTRIBUTION_CONTROLS
#include "cui_components.h"
#endif
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    if (!ui)
        return 1;
    if (!cui_begin_frame(ui, 200, 100, 0))
        return 2;
    cui_begin_box(ui, cui_id_from(ui, "root"), (cui_box){.width = -1, .height = -1});
#ifdef CUI_DISTRIBUTION_CONTROLS
    cui_theme theme = cui_theme_light();
    cui_button(ui, cui_id_from(ui, "button"), &theme, cui_string("Vendored button"), false);
#else
    cui_text(ui, cui_string("Standalone C11 header"), 14, (cui_color){255, 255, 255, 255});
#endif
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    int result = !list.count || cui_last_error(ui) != CUI_OK;
    cui_destroy(ui);
    return result;
}
