/* SPDX-License-Identifier: MIT */
/* Run the actual bridge step and software backend using a simulated native host. */
#ifndef CUI_BRIDGE_TEST_RENDER
#define CUI_BRIDGE_TEST_RENDER cui_software_render
#endif
static cui_software *software;
static cui_bridge_status poll_status, render_status;

cui_bridge_status cui_bridge_native_create(cui_bridge_app *app, const char *title, const char *font,
                                           cui_config *config, bool hidden) {
    (void)title;
    (void)font;
    (void)config;
    (void)hidden;
    software = cui_software_create((unsigned)app->width, (unsigned)app->height, NULL);
    return software ? CUI_BRIDGE_OK : CUI_BRIDGE_OUT_OF_MEMORY;
}

void cui_bridge_native_destroy(cui_bridge_app *app) {
    (void)app;
    cui_software_destroy(software);
    software = NULL;
}

cui_bridge_status cui_bridge_native_run(cui_bridge_app *app, cui_bridge_run_callback callback,
                                        void *user) {
    (void)app;
    (void)callback;
    (void)user;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_poll(cui_bridge_app *app) {
    (void)app;
    return poll_status;
}

cui_bridge_status cui_bridge_native_render(cui_bridge_app *app, const cui_draw_list *list) {
    if (render_status != CUI_BRIDGE_OK)
        return render_status;
    assert(cui_software_resize(software, (unsigned)app->width, (unsigned)app->height));
    return CUI_BRIDGE_TEST_RENDER(software, list, (cui_color){0, 0, 0, 255})
               ? CUI_BRIDGE_OK
               : CUI_BRIDGE_CORE_ERROR;
}
#undef CUI_BRIDGE_TEST_RENDER

cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *app,
                                                       const char *const *paths, uint32_t count) {
    (void)app;
    (void)paths;
    (void)count;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *app, cui_str text) {
    (void)app;
    (void)text;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *app, char *text,
                                                  uint32_t capacity, uint32_t *length) {
    (void)app;
    (void)text;
    (void)capacity;
    (void)length;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_set_window_visible(cui_bridge_app *app, uint32_t visible) {
    (void)app;
    (void)visible;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

cui_bridge_status cui_bridge_native_get_window_visible(cui_bridge_app *app, uint32_t *visible) {
    (void)app;
    (void)visible;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
