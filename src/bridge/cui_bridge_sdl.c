/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "cui_bridge_internal.h"
#include <stdlib.h>

typedef struct {
    cui_font *font;
    cui_sdl3 *host;
} bridge_native;

cui_bridge_status cui_bridge_native_create(cui_bridge_app *app, const char *title,
                                           const char *font_path, cui_config *config) {
    if (!font_path || !font_path[0])
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_native *native = calloc(1, sizeof(*native));
    if (!native)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    app->native = native;
    native->font = cui_font_open(font_path);
    if (!native->font)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    native->host = cui_sdl3_create(title && title[0] ? title : "CUI Bridge", (int)app->width,
                                   (int)app->height, native->font);
    if (!native->host)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    config->measure = cui_font_measure;
    config->measure_styled = cui_font_measure_styled;
    config->text_geometry = cui_font_text_geometry;
    config->text_geometry_line = cui_font_text_geometry_line;
    config->font_metrics = cui_font_get_metrics;
    config->measure_user = native->font;
    return CUI_BRIDGE_OK;
}
void cui_bridge_native_destroy(cui_bridge_app *app) {
    if (!app || !app->native)
        return;
    bridge_native *native = app->native;
    cui_sdl3_destroy(native->host);
    cui_font_close(native->font);
    free(native);
    app->native = NULL;
}
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *app) {
    bridge_native *native = app->native;
    if (!native || !native->host)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    if (!cui_sdl3_poll(native->host, app->ui))
        return CUI_BRIDGE_CLOSED;
    cui_sdl3_size(native->host, &app->width, &app->height);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_native_render(cui_bridge_app *app, const cui_draw_list *list) {
    bridge_native *native = app->native;
    if (!native || !native->host)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    (void)cui_sdl3_set_cursor(native->host, cui_pointer_cursor(app->ui));
    return cui_sdl3_render(native->host, list, app->theme.background)
               ? CUI_BRIDGE_OK
               : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
