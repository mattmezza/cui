#include "cui/cui.h"
#include <assert.h>
#include <locale.h>
#include <math.h>
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
    cui_destroy(p->ui);
    assert(!cui_begin_frame(p->ui, 10, 10, 0));
    cui_diagnostic recursive = {.error = CUI_ERROR_LAYOUT};
    cui_report_diagnostic(p->ui, &recursive);
    assert(cui_last_error(p->ui) != CUI_OK);
    cui_diagnostic observed;
    assert(cui_latest_diagnostic(p->ui, &observed));
    assert(observed.error == d->error && observed.frame_sequence == d->frame_sequence);
}
static cui_context *make(void) {
    cui_context *ui = cui_create((cui_config){
        .width = 300, .height = 200, .max_elements = 64, .max_commands = 128, .frame_bytes = 2048});
    assert(ui);
    return ui;
}
static cui_size hostile_measure(void *user, cui_str text, float size) {
    cui_context *ui = *(cui_context **)user;
    (void)text;
    cui_destroy(ui); /* Must reject destruction while the host callback is active. */
    assert(!cui_begin_frame(ui, 100, 100, 0));
    cui_diagnostic d = {.error = CUI_ERROR_INVALID_ACTION, .operation = "hostile_measure"};
    cui_report_diagnostic(ui, &d);
    return (cui_size){size, size};
}
static bool hostile_metrics(void *user, float size, cui_text_style style, cui_font_metrics *out) {
    (void)style;
    hostile_measure(user, (cui_str){0}, size);
    *out = (cui_font_metrics){.ascent = 8, .descent = 2, .line_height = 12};
    return true;
}
static cui_text_layout_status hostile_geometry(void *user, cui_str text, float size,
                                               cui_text_style style, cui_text_geometry *out) {
    (void)style;
    (void)out;
    hostile_measure(user, text, size);
    return CUI_TEXT_LAYOUT_UNAVAILABLE;
}
static cui_text_layout_status hostile_geometry_line(void *user, cui_text_line_source source,
                                                    float size, cui_text_style style,
                                                    cui_text_geometry *out) {
    return hostile_geometry(user, source.paragraph, size, style, out);
}
static bool report_prepaint(const cui_prepaint_info *info, const void *payload,
                            cui_prepaint_writer *writer) {
    (void)info;
    (void)writer;
    cui_context *ui = *(cui_context *const *)payload;
    cui_diagnostic d = {.error = CUI_ERROR_INVALID_ACTION, .operation = "report_prepaint"};
    cui_report_diagnostic(ui, &d);
    cui_destroy(ui);
    assert(!cui_begin_frame(ui, 100, 100, 0));
    return true;
}
static void test_callback_boundaries(void) {
    cui_context *ui = NULL;
    ui = cui_create((cui_config){.width = 100,
                                 .height = 100,
                                 .measure = hostile_measure,
                                 .measure_user = &ui,
                                 .font_metrics = hostile_metrics,
                                 .text_geometry = hostile_geometry,
                                 .text_geometry_line = hostile_geometry_line});
    assert(ui);
    probe p = {.ui = ui};
    cui_set_diagnostic_callback(ui, diagnostic, &p);
    cui_size size = cui_measure_text(ui, (cui_str)CUI_STR("measure"), 12);
    assert(size.width == 12 && p.calls == 3);
    cui_diagnostic first, latest;
    assert(cui_first_diagnostic(ui, &first) && first.error == CUI_ERROR_LIFECYCLE);
    assert(cui_latest_diagnostic(ui, &latest) && latest.error == CUI_ERROR_INVALID_ACTION);
    assert(first.frame_sequence == 0 && latest.frame_sequence == 0);
    cui_font_metrics metrics;
    assert(cui_measure_font_metrics(ui, 12, (cui_text_style){0}, &metrics));
    cui_text_geometry geometry = {0};
    assert(cui_get_text_geometry(ui, (cui_str)CUI_STR("text"), 12, (cui_text_style){0},
                                 &geometry) == CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(cui_get_text_geometry_line(
               ui, (cui_text_line_source){.paragraph = CUI_STR("text"), .byte_end = 4}, 12,
               (cui_text_style){0}, &geometry) == CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(p.calls == 12);
    assert(cui_first_diagnostic(ui, &latest) && latest.error == first.error &&
           !strcmp(latest.operation, first.operation));
    cui_set_diagnostic_callback(ui, NULL, NULL);
    cui_destroy(ui);
    ui = make();
    p = (probe){.ui = ui};
    cui_set_diagnostic_callback(ui, diagnostic, &p);
    assert(cui_begin_frame(ui, 100, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 10, .height = 10});
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = report_prepaint,
                                                       .payload = &ui,
                                                       .payload_bytes = sizeof ui,
                                                       .command_budget = 4}));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_first_diagnostic(ui, &first));
    assert(first.error == CUI_ERROR_INVALID_ACTION && !strcmp(first.operation, "report_prepaint"));
    assert(cui_last_error(ui) == first.error && first.frame_sequence == 1 && p.calls > 0);
    cui_set_diagnostic_callback(ui, NULL, NULL);
    cui_destroy(ui);
    ui = make();
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(cui_begin_focus_scope(ui, 77, false));
    assert(cui_push_id(ui, "unclosed"));
    assert(cui_begin_tab_group(ui, 1));
    cui_end_frame(ui);
    assert(cui_first_diagnostic(ui, &first) && !strcmp(first.subsystem, "ID scopes"));
    assert(cui_latest_diagnostic(ui, &latest) && !strcmp(latest.subsystem, "tab groups"));
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(cui_begin_focus_scope(ui, 88, false));
    cui_end_frame(ui);
    assert(cui_first_diagnostic(ui, &first) && !strcmp(first.subsystem, "focus scopes"));
    assert(first.id == 88 && first.used == 1 && first.path_count == 0);
    cui_destroy(ui);
}
int main(void) {
    test_callback_boundaries();
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
    assert(strstr(json, "\"fixed\":[true,true]") && strstr(json, "\"percent\":[false,false]"));
    assert(strstr(json, "\"max\":[0,0]"));
    /* A new frame must never expose the previous frame's geometry. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    assert(!cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, sizeof json, &required));
    cui_begin_box(ui, 123,
                  (cui_box){.width = .375f,
                            .height = 0,
                            .width_percent = true,
                            .fixed_height = true,
                            .max_width = 99.5f,
                            .max_height = INFINITY});
    cui_end_box(ui);
    cui_begin_box(ui, 124, (cui_box){.width = NAN, .height = -INFINITY});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, sizeof json, &required));
    assert(strstr(json, "\"requested\":[0.375,0]") && strstr(json, "\"max\":[99.5,null]"));
    assert(strstr(json, "\"fixed\":[false,true]") && strstr(json, "\"percent\":[true,false]"));
    assert(strstr(json, "\"requested\":[null,null]") != NULL);
    assert(!strstr(json, "nan") && !strstr(json, "inf") && !strstr(json, "0,375"));
    /* Decimal-comma environments are tested when the host has one installed. */
    const char *locales[] = {"de_DE.UTF-8", "de_DE.utf8", "fr_FR.UTF-8", "it_IT.UTF-8"};
    bool tested_comma = false;
    for (size_t i = 0; i < sizeof locales / sizeof locales[0]; ++i) {
        if (!setlocale(LC_NUMERIC, locales[i]))
            continue;
        if (!strcmp(localeconv()->decimal_point, ",")) {
            assert(
                cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, sizeof json, &required));
            assert(strstr(json, "\"requested\":[0.375,0]") && strstr(json, "\"max\":[99.5,null]"));
            assert(!strstr(json, "0,375"));
            tested_comma = true;
            break;
        }
    }
    assert(setlocale(LC_NUMERIC, "C"));
    if (!tested_comma)
        puts("diagnostics: decimal-comma locale unavailable (conditional test skipped)");
    cui_set_layout_debug(ui, false);
    assert(!cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_TEXT, text, sizeof text, &required));
    cui_destroy(ui);
    puts("diagnostics: ok");
    return 0;
}
