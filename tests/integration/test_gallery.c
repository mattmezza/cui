#define main cui_gallery_application_main
#include "../../gallery/main.c"
#undef main
#include <assert.h>
#include <math.h>
int main(void) {
    const char *path = font_path(NULL);
    cui_font *font = cui_font_open(path);
    assert(font);
    register_matching_font_styles(font, path);
    cui_context *ui = cui_create((cui_config){.width = 680,
                                              .height = 740,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .font_metrics = cui_font_get_metrics,
                                              .measure_user = font});
    assert(ui);
    cui_font_metrics via_context, via_font;
    assert(cui_measure_font_metrics(ui, 16, (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD},
                                    &via_context));
    assert(cui_font_get_metrics(font, 16, (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD},
                                &via_font));
    assert(via_context.ascent == via_font.ascent && via_context.descent == via_font.descent &&
           via_context.line_height == via_font.line_height);
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
            bool styled_heading = false;
            for (size_t i = 0; i < list.count; i++) {
                cui_draw_command command = list.commands[i];
                if (command.type == CUI_DRAW_TEXT &&
                    command.text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD &&
                    command.text.length == strlen("CUI / component workbench") &&
                    !memcmp(command.text.data, "CUI / component workbench", command.text.length)) {
                    cui_size measured = cui_font_measure_styled(
                        font, command.text, command.font_size, command.text_style);
                    assert(fabsf(command.bounds.w - measured.width) < .1f);
                    styled_heading = true;
                }
            }
            assert(styled_heading);
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
