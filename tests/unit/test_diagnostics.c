#include "cui/cui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    cui_context *ui;
    size_t calls;
    cui_diagnostic last;
} probe;
static void diagnostic(void *user, const cui_diagnostic *d) {
    probe *p = user;
    ++p->calls;
    p->last = *d;
    /* Accidental callback reentry must neither clear nor recurse. */
    cui_clear_error(p->ui);
    cui_end_box(p->ui);
}
static cui_context *make(void) {
    cui_context *ui = cui_create((cui_config){
        .width = 300, .height = 200, .max_elements = 64, .max_commands = 128, .frame_bytes = 2048});
    assert(ui);
    return ui;
}
int main(void) {
    cui_context *ui = make();
    cui_diagnostic first = {0}, latest = {0};
    assert(!cui_first_diagnostic(ui, &first));
    probe p = {.ui = ui};
    cui_set_diagnostic_callback(ui, diagnostic, &p);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 11, (cui_box){.width = 100, .height = 60, .clip = true});
    cui_begin_box(ui, 22, (cui_box){.width = 50, .height = 20});
    cui_begin_box(ui, 22, (cui_box){0});
    cui_end_box(ui);
    assert(cui_first_diagnostic(ui, &first));
    assert(first.error == CUI_ERROR_DUPLICATE_ID && first.id == 22);
    assert(first.frame_sequence == 1 && first.message[0] && first.subsystem[0]);
    assert(first.detail == CUI_DIAGNOSTIC_DUPLICATE_ID);
    assert(first.path_count == 2 && first.path[0] == 11 && first.path[1] == 22);
    assert(!strcmp(first.operation, "cui_core_begin_box"));
    cui_interact(ui, 0, CUI_ROLE_BUTTON, (cui_str)CUI_STR("zero"), false, false, 0);
    assert(cui_latest_diagnostic(ui, &latest) && latest.detail == CUI_DIAGNOSTIC_ZERO_ID);
    assert(cui_last_error(ui) == CUI_ERROR_DUPLICATE_ID);
    assert(cui_first_diagnostic(ui, &latest) && !memcmp(&latest, &first, sizeof first));
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(p.calls == 2);
    cui_set_diagnostic_callback(ui, NULL, NULL);
    cui_clear_error(ui);
    assert(!cui_first_diagnostic(ui, &first) && !cui_latest_diagnostic(ui, &latest));
    assert(cui_begin_frame(ui, 300, 200, .016f));
    for (size_t i = 0; i < 65; ++i)
        cui_begin_box(ui, i + 1, (cui_box){0});
    assert(cui_first_diagnostic(ui, &first));
    assert(first.detail == CUI_DIAGNOSTIC_ELEMENTS && first.id == 65);
    assert(first.used == 64 && first.requested == 1 && first.limit == 64);
    assert(first.path_truncated && first.path_count == CUI_DIAGNOSTIC_PATH_MAX);
    for (size_t i = 0; i < 65; ++i)
        cui_end_box(ui);
    cui_end_frame(ui);
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 91, (cui_box){0});
    cui_end_frame(ui);
    assert(cui_first_diagnostic(ui, &first));
    assert(first.detail == CUI_DIAGNOSTIC_UNBALANCED_SCOPE && first.path[0] == 91);
    cui_destroy(ui);

    ui = cui_create((cui_config){.width = 100,
                                 .height = 100,
                                 .max_resources = 1,
                                 .resource_bytes = 8,
                                 .frame_bytes = 16,
                                 .max_commands = 1});
    assert(ui && cui_begin_frame(ui, 100, 100, .016f));
    assert(!cui_resource_get(ui, 1, 1, 9, NULL, NULL));
    assert(cui_first_diagnostic(ui, &first));
    assert(!strcmp(first.subsystem, "resource bytes") && first.requested == 9 && first.limit == 8);
    cui_clear_error(ui);
    assert(cui_resource_get(ui, 1, 1, 4, NULL, NULL));
    assert(!cui_resource_get(ui, 2, 1, 4, NULL, NULL));
    assert(cui_first_diagnostic(ui, &first));
    assert(!strcmp(first.subsystem, "resource slots") && first.used == 1 && first.limit == 1);
    cui_clear_error(ui);
    cui_text(ui, (cui_str)CUI_STR("text longer than sixteen bytes"), 12, (cui_color){0});
    assert(cui_first_diagnostic(ui, &first));
    assert(!strcmp(first.subsystem, "text source bytes") && first.limit == 16);
    cui_clear_error(ui);
    cui_begin_box(ui, 1, (cui_box){.width = 10, .height = 10, .background = {255, 255, 255, 255}});
    cui_end_box(ui);
    cui_begin_box(ui, 2, (cui_box){.width = 10, .height = 10, .background = {255, 255, 255, 255}});
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0);
    assert(cui_first_diagnostic(ui, &first));
    assert(strstr(first.subsystem, "commands") && first.limit == 1);
    cui_clear_error(ui);
    cui_diagnostic custom;
    memset(&custom, 'x', sizeof custom);
    custom.error = CUI_ERROR_INVALID_ACTION;
    custom.detail = CUI_DIAGNOSTIC_INVALID_ACTION;
    cui_report_diagnostic(ui, &custom);
    assert(cui_first_diagnostic(ui, &first));
    assert(first.operation[sizeof first.operation - 1] == 0);
    assert(first.message[sizeof first.message - 1] == 0);
    assert(first.subsystem[sizeof first.subsystem - 1] == 0);
    assert(first.path_count == 0);
    cui_destroy(ui);

    ui = make();
    size_t required = 0;
    char json[4096], text[4096], tiny[4] = "abc";
    assert(!cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, sizeof json, &required));
    cui_set_layout_debug(ui, true);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(
        ui, UINT64_MAX,
        (cui_box){
            .width = 100, .height = 60, .fixed_width = true, .fixed_height = true, .clip = true});
    cui_begin_box(ui, 0,
                  (cui_box){.width = 150, .height = 20, .fixed_width = true, .fixed_height = true});
    cui_end_box(ui);
    cui_end_box(ui);
    assert(!cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, sizeof json, &required));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK); /* Anonymous layout remains legal. */
    assert(!cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, tiny, sizeof tiny, &required));
    assert(!strcmp(tiny, "abc") && required > sizeof tiny);
    assert(cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, sizeof json, &required));
    assert(required == strlen(json) + 1);
    assert(strstr(json, "\"id\":\"18446744073709551615\"") != NULL);
    assert(strstr(json, "\"parent\":null") && strstr(json, "\"parent\":0"));
    assert(strstr(json, "\"bounds\":[0,0,100,60]") != NULL);
    assert(strstr(json, "\"clip\":[0,0,100,60]") != NULL);
    assert(strstr(json, "\"requested\":[150,20]") != NULL);
    assert(cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_TEXT, text, sizeof text, &required));
    assert(strstr(text, "box 1 id=0 parent=0") != NULL);
    cui_set_layout_debug(ui, false);
    assert(!cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_TEXT, text, sizeof text, &required));
    cui_destroy(ui);
    puts("diagnostics: ok");
    return 0;
}
