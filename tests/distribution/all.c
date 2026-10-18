/* Every portable implementation, including standalone optional models. */
#define CUI_IMPLEMENTATION_ALL
#include "cui_all.h"
#include "cui_component_events.h"

int main(void) {
    char storage[8];
    cui_str copied;
    cui_component_event_metadata metadata;
    if (!cui_component_event_init(&metadata, CUI_COMPONENT_EVENT_REQUEST, 7,
                                  CUI_COMPONENT_EVENT_SOURCE_PROGRAMMATIC) ||
        cui_component_event_copy_utf8(cui_string("owned"), 7, storage, sizeof storage, &copied) !=
            CUI_COMPONENT_EVENT_COPY_OK ||
        copied.length != 5 || copied.data != storage || metadata.target != 7)
        return 1;
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
