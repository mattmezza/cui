/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "decoder.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct {
    cui_context *ui;
    cui_sdl3 *host;
    cui_font *font;
    video_decoder *decoder;
    cui_video_view view;
    cui_video_controls_state controls;
    video_lease current;
    bool have_frame, ready, failed, awaiting_seek;
    const char *captures;
    unsigned stage, paused_frames;
    double paused_position;
    uint64_t paused_revision;
    struct timespec started;
} application;
static void release_current(application *app) {
    if (app->have_frame) {
        if (!video_decoder_release(app->decoder, &app->current))
            app->failed = true;
        app->have_frame = false;
    }
}
static void requested(void *user, const cui_video_request *r) {
    application *app = user;
    if (r->source_id != app->view.source_id || r->generation != app->view.timeline.generation)
        return;
    cui_video next = app->view.timeline;
    bool ok = false;
    if (r->kind == CUI_VIDEO_PLAYING)
        ok = cui_video_set_playing(&next, r->value.enabled);
    else if (r->kind == CUI_VIDEO_RATE)
        ok = cui_video_set_rate(&next, r->value.rate);
    else if (r->kind == CUI_VIDEO_SKIP ||
             (r->kind == CUI_VIDEO_SEEK && r->phase == CUI_SLIDER_COMMIT)) {
        double target =
            r->kind == CUI_VIDEO_SKIP ? next.position + r->value.seconds : r->value.seconds;
        target = fmax(0, target);
        if (next.duration_known)
            target = fmin(next.duration, target);
        ok = cui_video_seek(&next, target);
    }
    if (ok && next.generation != app->view.timeline.generation) {
        ok = video_decoder_seek(app->decoder, next.generation, next.position);
        if (ok)
            app->awaiting_seek = true;
    }
    if (ok)
        app->view.timeline = next;
    cui_invalidate(app->ui);
}
static void command(application *app, cui_video_request_kind kind, double number, bool enabled) {
    cui_video_request r = {.kind = kind,
                           .source_id = app->view.source_id,
                           .generation = app->view.timeline.generation,
                           .phase = CUI_SLIDER_COMMIT};
    if (kind == CUI_VIDEO_PLAYING)
        r.value.enabled = enabled;
    else
        r.value.seconds = number;
    requested(app, &r);
}
static bool initialize(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    application *app = user;
    if (argc != 3 && argc != 5) {
        fprintf(stderr, "Usage: player FONT VIDEO [--smoke CAPTURE_DIR]\n");
        return false;
    }
    if (argc == 5) {
        const char *virtual_display = getenv("CUI_TEST_VIRTUAL_DISPLAY");
        if (strcmp(argv[3], "--smoke") || !virtual_display || strcmp(virtual_display, "1"))
            return false;
#if defined(__linux__)
        const char *display = getenv("DISPLAY"), *wayland = getenv("WAYLAND_DISPLAY");
        if (!display || !*display || strcmp(display, ":0") == 0 ||
            strncmp(display, ":0.", 3) == 0 || (wayland && *wayland))
            return false;
#else
        return false;
#endif
        app->captures = argv[4];
    }
    app->font = cui_font_open(argv[1]);
    if (!app->font)
        return false;
    app->host = cui_sdl3_create("CUI silent decoded video", 640, 440, app->font);
    if (!app->host)
        return false;
    if (app->captures && strcmp(cui_sdl3_video_driver(), "x11"))
        return false;
    app->ui = cui_create((cui_config){.width = 640,
                                      .height = 440,
                                      .measure = cui_font_measure,
                                      .measure_styled = cui_font_measure_styled,
                                      .font_metrics = cui_font_get_metrics,
                                      .text_geometry = cui_font_text_geometry,
                                      .text_geometry_line = cui_font_text_geometry_line,
                                      .measure_user = app->font});
    if (!app->ui)
        return false;
    app->view = (cui_video_view){.source_id = 1,
                                 .status = CUI_VIDEO_LOADING,
                                 .capabilities =
                                     CUI_VIDEO_CAP_PLAY | CUI_VIDEO_CAP_SEEK | CUI_VIDEO_CAP_RATE,
                                 .status_description = cui_string("Loading silent local video")};
    if (!cui_video_init(&app->view.timeline, (cui_video_config){.seekable = true}))
        return false;
    app->decoder = video_decoder_open(argv[2], app->view.source_id, app->view.timeline.generation);
    if (!app->decoder)
        return false;
    timespec_get(&app->started, TIME_UTC);
    *host = app->host;
    *ui = app->ui;
    return true;
}
static int iterate(void *user, cui_sdl3 *host, cui_context *ui) {
    application *app = user;
    if (!cui_sdl3_poll(host, ui))
        return 1;
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    if (app->captures && (now.tv_sec - app->started.tv_sec) > 20)
        return -1;
    video_decoder_status status = video_decoder_poll(app->decoder);
    if (status.failed || app->failed)
        return -1;
    if (status.ready && !app->ready) {
        app->ready = true;
        app->view.status = CUI_VIDEO_READY;
        app->view.timeline.duration = status.duration;
        app->view.timeline.duration_known = status.duration_known;
        app->view.status_description = cui_string("Silent local video; no audio output");
        if (!app->captures)
            command(app, CUI_VIDEO_PLAYING, 0, true);
    }
    if (!cui_video_advance(&app->view.timeline, cui_sdl3_frame_delta(host)))
        return -1;
    if (app->have_frame && app->current.frame.frame.generation != app->view.timeline.generation)
        release_current(app);
    video_lease next = {0};
    if ((!app->have_frame || app->view.timeline.playing || app->view.timeline.ended ||
         app->awaiting_seek) &&
        video_decoder_acquire(app->decoder, app->view.timeline.generation,
                              app->view.timeline.position, &next)) {
        release_current(app);
        app->current = next;
        app->have_frame = true;
        if (app->awaiting_seek) {
            /* Adapter seek acknowledgement: first frame at/after target. */
            app->view.timeline.position = next.frame.frame.timestamp;
            app->awaiting_seek = false;
        }
    }
    status = video_decoder_poll(app->decoder);
    if (status.eof && !status.queued &&
        (!app->view.timeline.duration_known ||
         (app->awaiting_seek && app->view.timeline.position == app->view.timeline.duration))) {
        cui_video_finish(&app->view.timeline);
        app->awaiting_seek = false;
    }
    float width, height;
    cui_sdl3_size(host, &width, &height);
    if (!cui_begin_frame(ui, width, height, .016f))
        return -1;
    cui_theme theme = cui_theme_dark();
    cui_video_controls_options options = cui_video_controls_defaults();
    options.width = fmaxf(160, width);
    options.on_request = requested;
    options.user = app;
    options.visible |= CUI_VIDEO_CONTROL_RATE;
    options.requested_rate = app->view.timeline.rate == 1 ? 2 : 1;
    bool ok = cui_video_player(
        ui, 40, &theme, &app->controls, app->view, app->have_frame ? &app->current.frame : NULL,
        (cui_video_player_options){.surface = {.geometry = {.width = width,
                                                            .height = fmaxf(1, height - 130),
                                                            .fit = CUI_IMAGE_CONTAIN},
                                               .opacity = 1,
                                               .description = cui_string("Decoded video")},
                                   .controls = true,
                                   .controls_options = options});
    cui_draw_list list = cui_end_frame(ui);
    char capture[1024];
    const char *path = NULL;
    if (app->captures && app->have_frame) {
        const char *name = app->stage == 0                          ? "red.bmp"
                           : app->stage == 2                        ? "pause.bmp"
                           : app->stage == 3 && !app->awaiting_seek ? "seek.bmp"
                                                                    : NULL;
        if (name) {
            int n = snprintf(capture, sizeof capture, "%s/%s", app->captures, name);
            if (n < 0 || (size_t)n >= sizeof capture)
                return -1;
            path = capture;
        }
    }
    if (!ok || cui_last_error(ui) != CUI_OK ||
        !cui_sdl3_render_capture(host, &list, theme.background, path))
        return -1;
    if (app->captures && app->have_frame) {
        if (app->stage == 0) {
            app->stage = 1;
            command(app, CUI_VIDEO_PLAYING, 0, true);
        } else if (app->stage == 1 && app->view.timeline.position >= .3) {
            command(app, CUI_VIDEO_PLAYING, 0, false);
            app->stage = 2;
            app->paused_position = app->view.timeline.position;
            app->paused_revision = app->current.frame.frame.bitmap.revision;
        } else if (app->stage == 2) {
            if (app->view.timeline.position != app->paused_position ||
                app->current.frame.frame.bitmap.revision != app->paused_revision)
                return -1;
            if (++app->paused_frames == 4) {
                command(app, CUI_VIDEO_SEEK, 1.2, false);
                app->stage = 3;
            }
        } else if (app->stage == 3 && !app->awaiting_seek) {
            if (app->current.frame.frame.generation != 2 ||
                app->current.frame.frame.timestamp < 1.2)
                return -1;
            app->stage = 4;
            command(app, CUI_VIDEO_PLAYING, 0, true);
        } else if (app->stage == 4 && app->view.timeline.ended && status.eof && !status.queued) {
            printf("PASS: actual decoded presentations, pause, seek generation, EOF\n");
            return 1;
        }
    }
    /* Active rendering is driven by SDL; no nested pump, wait, decode or sleep.
     * Paused playback idles with a retained lease; loading/seek waits repaint. */
    if (!app->ready || app->view.timeline.playing || app->awaiting_seek || app->captures ||
        (app->view.timeline.ended && (!status.eof || status.queued)))
        cui_invalidate(ui);
    return 0;
}
static void shutdown(void *user, bool success) {
    application *app = user;
    (void)success;
    release_current(app);
    if (app->decoder && !video_decoder_close(app->decoder))
        abort();
    cui_destroy(app->ui);
    cui_sdl3_destroy(app->host);
    cui_font_close(app->font);
}
int main(int argc, char **argv) {
    application app = {0};
    return cui_sdl3_run(
        argc, argv,
        (cui_sdl3_app){.user = &app, .init = initialize, .frame = iterate, .quit = shutdown});
}
