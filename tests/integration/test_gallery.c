#define main cui_gallery_application_main
#include "../../gallery/main.c"
#undef main
#include <assert.h>
int main(void) {
    cui_font *font = cui_font_open(font_path(NULL));
    assert(font);
    cui_context *ui = cui_create((cui_config){
        .width = 680, .height = 740, .measure = cui_font_measure, .measure_user = font});
    assert(ui);
    char text[128] = "Workspace";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = text, .capacity = sizeof(text)});
    assert(edit);
    gallery_state state = {.section = 1, .volume = 65, .edit = edit};
    for (int contrast = 0; contrast < 2; contrast++) {
        state.contrast = contrast;
        for (int section = 0; section < 4; section++) {
            state.section = section;
            cui_draw_list list = frame(ui, &state, 680, 740);
            size_t count;
            const cui_semantic *semantics = cui_semantics(ui, &count);
            assert(count > 0);
            for (size_t i = 0; i < count; i++) {
                cui_rect b = semantics[i].bounds;
                assert(b.x >= 0 && b.y >= 0 && b.x + b.w <= 680.1f && b.y + b.h <= 740.1f);
            }
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_TEXT) {
                    cui_rect b = list.commands[i].bounds;
                    assert(b.x >= 0 && b.y >= 0 && b.x + b.w <= 680.1f && b.y + b.h <= 740.1f);
                }
        }
    }
    cui_focus(ui, id(ui, "volume"));
    cui_draw_list tiny = frame(ui, &state, 200, 120);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0);
    assert(!cui_focused(ui));
    unsigned texts = 0;
    for (size_t i = 0; i < tiny.count; i++)
        if (tiny.commands[i].type == CUI_DRAW_TEXT) {
            cui_rect b = tiny.commands[i].bounds;
            assert(b.x >= 0 && b.y >= 0 && b.x + b.w <= 200 && b.y + b.h <= 120);
            texts++;
        }
    assert(texts == 2);
    assert(cui_last_error(ui) == CUI_OK);
    cui_edit_destroy(edit);
    cui_destroy(ui);
    cui_font_close(font);
    puts("compact layout bounds and tiny-window focus suppression passed");
    return 0;
}
