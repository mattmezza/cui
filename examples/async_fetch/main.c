/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "cui/cui_components.h"
#include "cui/cui_sdl3.h"
#include "fetch.h"
#ifdef AF_WITH_CURL
#include "curl_transport.h"
#endif
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
typedef struct {
    cui_font *font;
    cui_sdl3 *host;
    cui_context *ui;
    af_worker *worker;
    af_model model;
    char url[AF_URL_MAX + 1];
    const char *capture;
    bool offline, curl_ready, idle_observed, close_pending, quit_error, fail_first_wake;
    atomic_bool release_offline;
    atomic_uint wake_calls, wake_attempts, offline_started, offline_finished;
    atomic_bool offline_cancelled;
    Uint32 wake_type;
    unsigned stage, clicks;
    af_start_result last_start;
    cui_button_state fetch_button, cancel_button;
} application;
static bool wake_ui(void *user) {
    application *a = user;
    unsigned attempt = atomic_fetch_add(&a->wake_attempts, 1);
    if (a->fail_first_wake && !attempt)
        return false;                         /* Offline failure injection. */
    SDL_Event event = {.type = a->wake_type}; /* No pointer payload or CUI call. */
    /* Publish the observation before the event: UI dispatch may beat the
     * return from SDL_PushEvent on the producer thread. */
    atomic_fetch_add(&a->wake_calls, 1);
    bool sent = SDL_PushEvent(&event);
    if (!sent)
        atomic_fetch_sub(&a->wake_calls, 1);
    return sent;
}
static void offline_transport(void *user, const char *url, af_cancel_fn cancel, void *token,
                              af_result *r) {
    application *a = user;
    (void)url;
    atomic_fetch_add(&a->offline_started, 1);
    while (!atomic_load(&a->release_offline) && !cancel(token)) {
        const struct timespec pause = {0, 1000000};
        nanosleep(&pause, NULL);
    }
    bool cancelled = cancel(token);
    atomic_store(&a->offline_cancelled, cancelled);
    r->status = cancelled ? AF_CANCELLED : AF_SUCCESS;
    r->http_status = 200;
    r->size = 16;
    memcpy(r->body, "offline response", 16);
    atomic_fetch_add(&a->offline_finished, 1);
}
static void fetch_clicked(void *user) {
    application *a = user;
    assert(SDL_IsMainThread());
    a->last_start = af_model_start(&a->model, a->worker, a->url);
    if (a->last_start == AF_STARTED)
        ++a->clicks;
    cui_invalidate(a->ui);
}
static void cancel_clicked(void *user) {
    application *a = user;
    assert(SDL_IsMainThread());
    (void)af_model_cancel(&a->model, a->worker);
    cui_invalidate(a->ui);
}
static bool initialize(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    application *a = user;
    const char *font = NULL, *url = "https://example.com/";
    atomic_init(&a->release_offline, false);
    atomic_init(&a->wake_calls, 0);
    atomic_init(&a->wake_attempts, 0);
    atomic_init(&a->offline_started, 0);
    atomic_init(&a->offline_finished, 0);
    atomic_init(&a->offline_cancelled, false);
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--font") && i + 1 < argc)
            font = argv[++i];
        else if (!strcmp(argv[i], "--url") && i + 1 < argc)
            url = argv[++i];
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
            a->capture = argv[++i];
        else if (!strcmp(argv[i], "--offline-smoke"))
            a->offline = true;
        else if (!strcmp(argv[i], "--offline-wake-failure")) {
            a->offline = true;
            a->fail_first_wake = true;
        } else if (!strcmp(argv[i], "--offline-close-pending")) {
            a->offline = true;
            a->close_pending = true;
        } else {
            fprintf(stderr, "Usage: async_fetch --font PATH [--url HTTPS_URL] [--offline-smoke "
                            "--capture BMP]\n");
            return false;
        }
    }
    size_t length = strlen(url);
    if (!font || length > AF_URL_MAX || (a->capture && !a->offline))
        return false;
    memcpy(a->url, url, length + 1);
    af_model_init(&a->model);
    a->font = cui_font_open(font);
    if (!a->font)
        return false;
    a->host = cui_sdl3_create("CUI async URL fetch", 720, 320, a->font);
    if (!a->host)
        return false;
    a->ui = cui_create((cui_config){.max_elements = 128,
                                    .max_commands = 256,
                                    .frame_bytes = 65536,
                                    .measure = cui_font_measure,
                                    .measure_styled = cui_font_measure_styled,
                                    .measure_user = a->font,
                                    .text_geometry = cui_font_text_geometry,
                                    .text_geometry_line = cui_font_text_geometry_line});
    if (!a->ui)
        return false;
    a->wake_type = SDL_RegisterEvents(1);
    if (!a->wake_type)
        return false;
    af_transport_fn transport = offline_transport;
    if (!a->offline) {
#ifdef AF_WITH_CURL
        a->curl_ready = af_curl_initialize();
        if (!a->curl_ready) {
            fprintf(stderr, "libcurl requires TLS and asynchronous DNS support\n");
            return false;
        }
        transport = af_curl_transport;
#else
        fprintf(stderr, "Built without libcurl; use --offline-smoke\n");
        return false;
#endif
    }
    a->worker = af_worker_create(transport, a, wake_ui, a);
    if (!a->worker)
        return false;
    *host = a->host;
    *ui = a->ui;
    return true;
}
static bool capture_pixels(const char *path) {
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface)
        return false;
    size_t ink = 0;
    for (int y = 0; y < surface->h; ++y)
        for (int x = 0; x < surface->w; ++x) {
            Uint8 r, g, b, alpha;
            if (!SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &alpha)) {
                SDL_DestroySurface(surface);
                return false;
            }
            ink += r < 160 && g < 160 && b < 160;
        }
    SDL_DestroySurface(surface);
    return ink > 100;
}
static int frame(void *user, cui_sdl3 *host, cui_context *ui) {
    application *a = user;
    assert(SDL_IsMainThread());
    (void)af_model_drain(&a->model, a->worker); /* Only the UI thread applies results. */
    float width, height;
    cui_sdl3_size(host, &width, &height);
    if (!cui_begin_frame(ui, width, height, cui_sdl3_frame_delta(host)))
        return -1;
    cui_theme theme = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 24, .gap = 16});
    cui_text(ui, cui_string("Fetch without blocking the UI"), 24, theme.text);
    cui_text(ui, cui_string(a->url), 14, theme.text);
    cui_begin_box(ui, 2, (cui_box){.row = true, .gap = 12});
    const char *button =
        (a->model.result.status == AF_FAILURE || a->model.result.status == AF_TOO_LARGE)
            ? "Retry fetch"
            : "Fetch URL";
    (void)cui_button_ex(ui, 10, &theme, cui_string(button),
                        (cui_button_options){.disabled = a->model.loading,
                                             .on_click = fetch_clicked,
                                             .action_user = a,
                                             .state = &a->fetch_button});
    (void)cui_button_ex(ui, 11, &theme, cui_string("Cancel"),
                        (cui_button_options){.disabled = !a->model.loading,
                                             .on_click = cancel_clicked,
                                             .action_user = a,
                                             .state = &a->cancel_button});
    cui_end_box(ui);
    char status[256];
    if (a->model.loading)
        snprintf(status, sizeof status, "Loading... duplicate fetches are disabled.");
    else if (a->model.result.status == AF_CANCELLED)
        snprintf(status, sizeof status, "Cancelled locally. Late results are ignored.");
    else if (a->model.result.status == AF_FAILURE || a->model.result.status == AF_TOO_LARGE)
        snprintf(status, sizeof status, "Failed: %s. Choose Retry fetch.", a->model.result.error);
    else if (a->model.applied)
        snprintf(status, sizeof status, "Received %zu bytes (HTTP %ld).", a->model.result.size,
                 a->model.result.http_status);
    else
        snprintf(status, sizeof status, "Ready. Fetch queues one bounded request and returns.");
    cui_text(ui, cui_string(status), 16, theme.text);
    if (a->last_start == AF_BUSY)
        cui_text(ui, cui_string("Worker still busy; retry shortly."), 14, theme.text);
    if (a->last_start == AF_INVALID)
        cui_text(ui, cui_string("Use an ASCII https:// URL of at most 2048 bytes."), 14,
                 theme.text);
    cui_text(ui, cui_string("Close safely joins pending work before releasing the window."), 14,
             theme.text);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    bool done = a->offline && a->model.applied == 1;
    bool rendered = done && a->capture
                        ? cui_sdl3_render_capture(host, &list, theme.background, a->capture)
                        : cui_sdl3_render(host, &list, theme.background);
    if (!rendered || cui_last_error(ui) != CUI_OK)
        return -1;
    if (!a->offline)
        return 0;
    if (done) {
        if (!a->idle_observed || a->clicks != 1 || atomic_load(&a->wake_calls) != 1 ||
            af_worker_wake_failures(a->worker) != (a->fail_first_wake ? 1u : 0u) ||
            atomic_load(&a->wake_attempts) != (a->fail_first_wake ? 2u : 1u) ||
            a->model.result.status != AF_SUCCESS || (a->capture && !capture_pixels(a->capture)))
            return -1;
        puts("PASS actual button enqueue, visible loading, idle SDL wake, UI-thread completion and "
             "pixels");
        return 1;
    }
    if (!a->stage) {
        cui_rect b = cui_bounds(ui, 10);
        if (b.w <= 0 || b.h <= 0 ||
            !cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 2, .y = b.y + 2}) ||
            !cui_queue_event(ui,
                             (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 2, .y = b.y + 2}))
            return -1;
        a->stage = 1;
        cui_invalidate(ui);
    } else if (a->stage == 1 && a->model.loading) {
        if (a->close_pending) {
            if (atomic_load(&a->offline_started))
                return 1;
            cui_invalidate(ui);
            return 0;
        }
        /* Release completion only after a loading frame reached Core idle. The
         * callback runner can then sleep until the worker pushes its user event. */
        if (!cui_needs_redraw(ui)) {
            a->idle_observed = true;
            a->stage = 2;
            atomic_store(&a->release_offline, true);
        }
    }
    return 0;
}
static void shutdown(void *user, bool success) {
    application *a = user;
    af_worker_destroy(a->worker); /* No callback may access a/SDL after this join. */
    a->worker = NULL;
    if (a->close_pending) {
        a->quit_error = atomic_load(&a->offline_started) != 1 ||
                        atomic_load(&a->offline_finished) != 1 ||
                        !atomic_load(&a->offline_cancelled);
        if (!a->quit_error)
            puts("PASS native close with active pending transport: cancelled and joined before "
                 "host teardown");
    }
#ifdef AF_WITH_CURL
    if (a->curl_ready)
        af_curl_shutdown();
#endif
    if (!success)
        fprintf(stderr, "Async fetch stopped: %s\n", cui_sdl3_error());
    cui_destroy(a->ui);
    cui_sdl3_destroy(a->host);
    cui_font_close(a->font);
}
int main(int argc, char **argv) {
    application app = {0};
    int result = cui_sdl3_run(argc, argv, (cui_sdl3_app){&app, initialize, frame, shutdown});
    return result || app.quit_error ? 1 : 0;
}
