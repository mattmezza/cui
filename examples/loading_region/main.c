/* SPDX-License-Identifier: MIT */
/* Compiling, no-window example. Emits real Core snapshots for the DOM fixture. */
#include "scene.h"
#include <stdio.h>
static void string_json(cui_str value) {
    putchar('"');
    for (size_t i = 0; i < value.length; i++) {
        unsigned char c = (unsigned char)value.data[i];
        if (c == '"' || c == '\\')
            putchar('\\');
        if (c < 32)
            printf("\\u%04x", c);
        else
            putchar(c);
    }
    putchar('"');
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    if (!ui)
        return 1;
    cui_theme theme = cui_theme_light();
    loading_scene scene = {.alert_visible = true, .alert_name = "Records unavailable"};
    puts("[");
    for (unsigned frame = 0; frame < 8; frame++) {
        scene.phase = frame < 2   ? LOADING_REGION_LOADING
                      : frame < 3 ? LOADING_REGION_READY
                                  : LOADING_REGION_FAILED;
        scene.alert_name = frame < 5 ? "Records unavailable" : "Retry failed";
        scene.alert_visible = frame != 6;
        if (!cui_begin_frame(ui, 400, 400, 0))
            return 2;
        loading_scene_frame(ui, &theme, &scene, frame * .1);
        cui_end_frame(ui);
        if (cui_last_error(ui) != CUI_OK)
            return 3;
        size_t count;
        const cui_semantic *nodes = cui_semantics(ui, &count);
        printf("%s[", frame ? "," : "");
        for (size_t i = 0; i < count; i++) {
            printf("%s{\"id\":\"%llu\",\"parent\":\"%llu\",\"role\":%d,\"busy\":%s,\"name\":",
                   i ? "," : "", (unsigned long long)nodes[i].id,
                   (unsigned long long)cui_semantic_parent(ui, nodes[i].id), (int)nodes[i].role,
                   nodes[i].details.busy ? "true" : "false");
            string_json(nodes[i].name);
            putchar('}');
        }
        puts("]");
    }
    puts("]");
    cui_destroy(ui);
    return 0;
}
