/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_sidebar.h"
#include <assert.h>
#include <string.h>
static void folder_label(cui_context *ui, const cui_theme *theme, bool collapsed, void *user) {
    if (!collapsed)
        cui_text(ui, cui_string(user), 14, theme->sidebar_foreground);
}
void app_frame(cui_context *, const cui_theme *, bool *);
static void frame(cui_context *ui, const cui_theme *theme, bool *compose) {
    assert(cui_begin_frame(ui, 720, 420, .016f));
    app_frame(ui, theme, compose);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void click(cui_context *ui, cui_id id) {
    cui_rect box = cui_bounds(ui, id);
    assert(box.w > 0 && box.h > 0);
    cui_event event = {
        .type = CUI_EVENT_POINTER_DOWN, .x = box.x + box.w / 2, .y = box.y + box.h / 2};
    assert(cui_queue_event(ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, event));
}
static bool has_text(cui_draw_list list, const char *text) {
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT &&
            list.commands[i].text.length == strlen(text) &&
            !memcmp(list.commands[i].text.data, text, strlen(text)))
            return true;
    return false;
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 720, .height = 420});
    assert(ui);
    cui_sidebar_menu_item item = {.label_content = folder_label, .label_user = "Inbox"};
    assert(item.label_content == folder_label);
    cui_theme theme = cui_theme_light();
    bool compose = false;
    frame(ui, &theme, &compose);
    cui_rect folders = cui_bounds(ui, cui_id_from(ui, "folders"));
    cui_rect messages = cui_bounds(ui, cui_id_from(ui, "messages"));
    cui_rect reader = cui_bounds(ui, cui_id_from(ui, "reader"));
    assert(folders.x < messages.x && messages.x < reader.x);
    click(ui, cui_id_from(ui, "compose"));
    frame(ui, &theme, &compose);
    assert(compose);
    click(ui, cui_id_from(ui, "discard"));
    frame(ui, &theme, &compose);
    assert(!compose);
    assert(cui_begin_frame(ui, 720, 420, .016f));
    assert(cui_push_id(ui, "message-1"));
    cui_id open = cui_id_from(ui, "open");
    cui_id star = cui_id_from(ui, "star");
    cui_id row = cui_id_from(ui, "row");
    cui_pop_id(ui);
    app_frame(ui, &theme, &compose);
    (void)cui_end_frame(ui);
    click(ui, open);
    frame(ui, &theme, &compose);
    click(ui, star);
    assert(cui_begin_frame(ui, 720, 420, .016f));
    app_frame(ui, &theme, &compose);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(has_text(draw, "Message 1 (starred)"));
    cui_rect before_row = cui_bounds(ui, row);
    cui_rect before_compose = cui_bounds(ui, cui_id_from(ui, "compose"));
    assert(cui_queue_event(
        ui, (cui_event){
                .type = CUI_EVENT_SCROLL, .x = messages.x + 100, .y = messages.y + 50, .dy = -1}));
    frame(ui, &theme, &compose);
    assert(cui_bounds(ui, row).y < before_row.y);
    assert(cui_bounds(ui, cui_id_from(ui, "compose")).y == before_compose.y);
    cui_destroy(ui);
    return 0;
}
