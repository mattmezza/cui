/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "cui_bridge_internal.h"
#include <stdlib.h>
#include <string.h>

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
    bool rendered = cui_sdl3_render(native->host, list, app->theme.background);
    return rendered ? CUI_BRIDGE_OK : CUI_BRIDGE_CORE_ERROR;
}
cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *app,
                                                       const char *const *paths, uint32_t count) {
    bridge_native *native = app->native;
    if (!native || !native->font)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    return cui_font_register_fallbacks(native->font, paths, count) ? CUI_BRIDGE_OK
                                                                   : CUI_BRIDGE_CORE_ERROR;
}
cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *app, cui_str text) {
    bridge_native *native = app->native;
    if (!native || !native->host)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    cui_platform platform = cui_sdl3_clipboard_interface(native->host);
    if (!(platform.capabilities & CUI_PLATFORM_CLIPBOARD) || !platform.clipboard_set)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    return platform.clipboard_set(platform.user, text) ? CUI_BRIDGE_OK : CUI_BRIDGE_CORE_ERROR;
}
cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *app, char *out, uint32_t capacity,
                                                  uint32_t *length) {
    bridge_native *native = app->native;
    if (!native || !native->host)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    cui_platform platform = cui_sdl3_clipboard_interface(native->host);
    if (!(platform.capabilities & CUI_PLATFORM_CLIPBOARD) || !platform.clipboard_get)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    cui_str text = platform.clipboard_get(platform.user);
    if (!text.data)
        return CUI_BRIDGE_CORE_ERROR;
    if (text.length > 65536 || text.length > UINT32_MAX)
        return CUI_BRIDGE_CAPACITY;
    if (capacity <= text.length)
        return CUI_BRIDGE_CAPACITY;
    memcpy(out, text.data, text.length);
    out[text.length] = '\0';
    *length = (uint32_t)text.length;
    return CUI_BRIDGE_OK;
}

typedef struct {
    cui_bridge_app *app;
    cui_bridge_run_callback callback;
    void *user;
    cui_bridge_status status;
} bridge_run;
static bool bridge_run_init(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    (void)argc;
    (void)argv;
    bridge_run *run = user;
    bridge_native *native = run->app->native;
    *host = native ? native->host : NULL;
    *ui = run->app->ui;
    return *host && *ui;
}
static int bridge_run_frame(void *user, cui_sdl3 *host, cui_context *ui) {
    bridge_run *run = user;
    float dt = cui_sdl3_frame_delta(host);
    if (dt > 1)
        dt = 1;
    run->status = cui_bridge_step(run->app, dt);
    if (run->status == CUI_BRIDGE_OK && run->callback) {
        run->app->run_dispatching = true;
        run->status = run->callback(run->user);
        run->app->run_dispatching = false;
    }
    if (run->callback && run->status == CUI_BRIDGE_OK)
        cui_invalidate(ui); /* Preserve continuous high-level run/dispatch cadence. */
    if (run->app->closed && run->status == CUI_BRIDGE_OK)
        run->status = CUI_BRIDGE_CLOSED;
    return run->status == CUI_BRIDGE_OK ? 0 : run->status == CUI_BRIDGE_CLOSED ? 1 : -1;
}
static void bridge_run_quit(void *user, bool success) {
    bridge_run *run = user;
    if (run->status == CUI_BRIDGE_OK)
        run->status = success ? CUI_BRIDGE_CLOSED : CUI_BRIDGE_CORE_ERROR;
    run->app->closed = true;
    cui_bridge_native_destroy(run->app);
}
cui_bridge_status cui_bridge_native_run(cui_bridge_app *app, cui_bridge_run_callback callback,
                                        void *user) {
    bridge_run run = {.app = app, .callback = callback, .user = user, .status = CUI_BRIDGE_OK};
    int result = cui_sdl3_run(
        0, NULL, (cui_sdl3_app){&run, bridge_run_init, bridge_run_frame, bridge_run_quit});
    return result && run.status == CUI_BRIDGE_OK ? CUI_BRIDGE_CORE_ERROR : run.status;
}
