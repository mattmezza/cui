/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "cui/cui_theme_registry.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static cui_draw_list scene(cui_context *ui, cui_theme theme) {
    assert(cui_begin_frame(ui, 240, 100, 0));
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 240, .height = 100, .background = theme.background, .padding = 8});
    cui_text(ui, cui_string("Theme projection"), theme.font_size, theme.text);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
int main(void) {
    const char *path = getenv("CUI_TEST_FONT");
    if (!path)
        path = "/usr/share/fonts/noto/NotoSans-Regular.ttf";
    cui_font *font = cui_font_open(path);
    if (!font) {
        puts("SKIP: actual native font unavailable");
        return 77;
    }
    cui_context *ui = cui_create((cui_config){
        .width = 240, .height = 100, .measure = cui_font_measure, .measure_user = font});
    assert(ui);
    cui_software *software = cui_software_create(240, 100, font);
    assert(software);
    cui_theme original = cui_theme_light(), projected;
    cui_theme_semantic_tokens tokens;
    cui_theme_import_result report;
    const char *source =
        "{\"tokens\":{\"colors\":{\"background\":\"#123456\",\"foreground\":\"#ffffff\"},"
        "\"typography\":{\"md\":{\"size\":28,\"line_height\":36}},\"radius\":{\"md\":10}}}";
    assert(cui_theme_semantic_import((cui_str){source, strlen(source)}, &original, &tokens,
                                     &projected, &report) == 0);
    cui_draw_list draw = scene(ui, projected);
    bool text = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            assert(draw.commands[i].font_size == 28);
            text = true;
        }
    assert(text && cui_software_render(software, &draw, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(software, &w, &h);
    size_t ink = 0, background = 0;
    for (size_t i = 0; i < (size_t)w * h; i++) {
        ink += pixels[i * 4] == 255 && pixels[i * 4 + 1] == 255 && pixels[i * 4 + 2] == 255;
        background += pixels[i * 4] == 18 && pixels[i * 4 + 1] == 52 && pixels[i * 4 + 2] == 86;
    }
    assert(ink > 50 && background > 10000);
    /* Theme import does not silently recolor existing specialized button skins. */
    assert(!memcmp(projected.buttons, original.buttons, sizeof original.buttons));
    cui_theme_registry_entry *entries = calloc(38, sizeof *entries);
    assert(entries);
    cui_theme_registry registry;
    assert(cui_theme_registry_init(&registry, entries, 38, true));
    cui_theme_host_state host = {0};
    cui_theme_selection select = {
        CUI_THEME_FOLLOW_SYSTEM, {"Default Light", 13}, {"Default Dark", 12}};
    assert(cui_theme_host_update(&host, &registry, select,
                                 (cui_theme_host_preferences){
                                     .high_contrast = true, .reduced_motion = true, .revision = 1},
                                 16) == 0);
    draw = scene(ui, host.theme);
    assert(cui_software_render(software, &draw, (cui_color){1, 2, 3, 255}));
    pixels = cui_software_pixels(software, &w, &h);
    ink = 0;
    background = 0;
    for (size_t i = 0; i < (size_t)w * h; i++) {
        ink += pixels[i * 4] == 255 && pixels[i * 4 + 1] == 255 && pixels[i * 4 + 2] == 255;
        background += pixels[i * 4] == 0 && pixels[i * 4 + 1] == 0 && pixels[i * 4 + 2] == 0;
    }
    assert(ink > 20 && background > 10000 && host.motion.duration_normal == 0 &&
           host.motion.distance_medium == 0);
    free(entries);
    cui_software_destroy(software);
    cui_destroy(ui);
    cui_font_close(font);
    return 0;
}
