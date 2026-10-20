/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "session.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_sdl3 *host;
    cui_context *ui;
    cui_font *font;
    video_sound_session *session;
    cui_video_view view;
    cui_video_controls_state controls;
    const char *captures, *media_path;
    unsigned stage, paused_frames;
    uint64_t paused_position, paused_revision;
    Uint64 started;
    bool printed_error, audio_initialized;
} application;
static void requested(void *u, const cui_video_request *r) {
    application *a = u;
    video_sound_status s = video_sound_snapshot(a->session);
    if (s.failed || r->source_id != 1 || r->generation != s.generation)
        return;
    bool ok = true;
    if (r->kind == CUI_VIDEO_PLAYING)
        ok = video_sound_play(a->session, r->value.enabled);
    else if (r->kind == CUI_VIDEO_SKIP ||
             (r->kind == CUI_VIDEO_SEEK && r->phase == CUI_SLIDER_COMMIT)) {
        double target = r->kind == CUI_VIDEO_SKIP ? s.position_frames / 48000.0 + r->value.seconds
                                                  : r->value.seconds;
        ok = video_sound_seek(a->session, target);
    }
    if (!ok)
        fprintf(stderr, "Playback request failed: %s\n",
                video_sound_snapshot(a->session).error ? video_sound_snapshot(a->session).error
                                                       : "unavailable");
    cui_invalidate(a->ui);
}
static bool initialize(void *u, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    application *a = u;
    if (argc != 3 && argc != 5) {
        fprintf(
            stderr,
            "Usage: video_player_audio FONT LOCAL_FFV1_PCM_MKV [--smoke CAPTURE_DIR]\nSupported: "
            "<=10s FFV1 + PCM, fixed48k mixer software clock; no AAC/Opus/MP4/fullscreen/rate.\n");
        return false;
    }
    if (argc == 5) {
        const char *virtual = getenv("CUI_TEST_VIRTUAL_DISPLAY"), *display = getenv("DISPLAY"),
                   *wayland = getenv("WAYLAND_DISPLAY");
#if defined(__linux__)
        if (strcmp(argv[3], "--smoke") || !virtual || strcmp(virtual, "1") || !display ||
            !*display || !strcmp(display, ":0") || !strncmp(display, ":0.", 3) ||
            (wayland && *wayland))
            return false;
#else
        (void)virtual;
        (void)display;
        (void)wayland;
        return false;
#endif
        if (SDL_setenv_unsafe("SDL_AUDIODRIVER", "dummy", 1) != 0)
            return false;
        a->captures = argv[4];
    }
    a->font = cui_font_open(argv[1]);
    if (!a->font)
        return false;
    a->host = cui_sdl3_create("CUI decoded video with PCM sound", 640, 440, a->font);
    if (!a->host)
        return false;
    if (a->captures && strcmp(cui_sdl3_video_driver(), "x11"))
        return false;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        return false;
    a->audio_initialized = true;
    if (a->captures && strcmp(SDL_GetCurrentAudioDriver(), "dummy"))
        return false;
    a->ui = cui_create((cui_config){.width = 640,
                                    .height = 440,
                                    .measure = cui_font_measure,
                                    .measure_styled = cui_font_measure_styled,
                                    .font_metrics = cui_font_get_metrics,
                                    .text_geometry = cui_font_text_geometry,
                                    .text_geometry_line = cui_font_text_geometry_line,
                                    .measure_user = a->font});
    if (!a->ui)
        return false;
    a->media_path = argv[2];
    a->view = (cui_video_view){.source_id = 1,
                               .status = CUI_VIDEO_LOADING,
                               .capabilities = CUI_VIDEO_CAP_PLAY | CUI_VIDEO_CAP_SEEK};
    if (!cui_video_init(&a->view.timeline, (cui_video_config){.seekable = true}))
        return false;
    a->started = SDL_GetTicks();
    *host = a->host;
    *ui = a->ui;
    return true;
}
static int failed(application *a, int line) {
    video_sound_status s = a->session ? video_sound_snapshot(a->session) : (video_sound_status){0};
    fprintf(stderr, "Player failure line %d stage %u frame %llu generation %llu UI error %d\n",
            line, a->stage, (unsigned long long)s.position_frames, (unsigned long long)s.generation,
            (int)cui_last_error(a->ui));
    return -1;
}
static int iterate(void *u, cui_sdl3 *host, cui_context *ui) {
    application *a = u;
    if (!cui_sdl3_poll(host, ui))
        return 1;
    /* Normal runner polling first publishes initial SDL pending audio-device
     * enumeration before the guarded generation exists. Never reset a sticky
     * invalidation flag or nest an event pump to hide a later device change. */
    if (!a->session) {
        a->session = video_sound_open(a->media_path, (video_sound_hooks){0});
        if (!a->session) {
            fprintf(stderr, "Cannot open supported fixed48k SDL output/media\n");
            return failed(a, __LINE__);
        }
        if (!a->captures)
            video_sound_play(a->session, true);
    }
    if (a->captures && SDL_GetTicks() - a->started > 15000)
        return failed(a, __LINE__);
    bool healthy = video_sound_poll(a->session);
    video_sound_status s = video_sound_snapshot(a->session);
    if (!healthy && !a->printed_error) {
        fprintf(stderr, "Sound playback stopped: %s\n", s.error ? s.error : "session closed");
        a->printed_error = true;
    }
    if (a->captures && !healthy)
        return failed(a, __LINE__);
    a->view.timeline = (cui_video){.position = s.position_frames / 48000.0,
                                   .duration = s.duration,
                                   .rate = 1,
                                   .generation = s.generation,
                                   .duration_known = s.ready,
                                   .seekable = s.ready,
                                   .playing = s.playing,
                                   .ended = s.ended};
    a->view.status = s.failed ? CUI_VIDEO_FAILED : s.ready ? CUI_VIDEO_READY : CUI_VIDEO_LOADING;
    a->view.status_description =
        cui_string(s.failed    ? s.error
                   : s.seeking ? "Seeking PCM audio and video"
                   : s.ready   ? "PCM sound; SDL software-mixer clock (not DAC)"
                               : "Loading supported FFV1 + PCM file");
    a->view.capabilities = s.ready && !s.failed ? CUI_VIDEO_CAP_PLAY | CUI_VIDEO_CAP_SEEK : 0;
    const cui_video_player_frame *frame = video_sound_frame(a->session);
    float width, height;
    cui_sdl3_size(host, &width, &height);
    if (!cui_begin_frame(ui, width, height, .016f))
        return failed(a, __LINE__);
    cui_theme theme = cui_theme_dark();
    cui_video_controls_options options = cui_video_controls_defaults();
    options.width = fmaxf(160, width);
    options.on_request = requested;
    options.user = a;
    bool ok =
        cui_video_player(ui, 40, &theme, &a->controls, a->view, frame,
                         (cui_video_player_options){
                             .surface = {.geometry = {.width = width,
                                                      .height = fmaxf(1, height - 130),
                                                      .fit = CUI_IMAGE_CONTAIN},
                                         .opacity = 1,
                                         .description = cui_string("Decoded video with PCM sound")},
                             .controls = true,
                             .controls_options = options});
    cui_draw_list list = cui_end_frame(ui);
    char capture[4096];
    const char *path = NULL;
    if (a->captures && frame) {
        const char *name = a->stage == 0                 ? "red.bmp"
                           : a->stage == 2               ? "pause.bmp"
                           : a->stage == 3 && !s.seeking ? "seek.bmp"
                                                         : NULL;
        if (name) {
            int n = snprintf(capture, sizeof(capture), "%s/%s", a->captures, name);
            if (n < 0 || (size_t)n >= sizeof(capture))
                return failed(a, __LINE__);
            path = capture;
        }
    }
    if (!ok || cui_last_error(ui) != CUI_OK ||
        !cui_sdl3_render_capture(host, &list, theme.background, path))
        return failed(a, __LINE__);
    if (a->captures && frame) {
        if (a->stage == 0) {
            a->stage = 1;
            if (!video_sound_play(a->session, true))
                return failed(a, __LINE__);
        } else if (a->stage == 1 && s.position_frames >= 14400) {
            if (!video_sound_play(a->session, false))
                return failed(a, __LINE__);
            a->paused_position = video_sound_snapshot(a->session).position_frames;
            a->paused_revision = frame->frame.bitmap.revision;
            a->stage = 2;
        } else if (a->stage == 2) {
            if (s.position_frames != a->paused_position ||
                frame->frame.bitmap.revision != a->paused_revision)
                return failed(a, __LINE__);
            if (++a->paused_frames == 4) {
                if (!video_sound_seek(a->session, 1.25))
                    return failed(a, __LINE__);
                a->stage = 3;
            }
        } else if (a->stage == 3 && !s.seeking) {
            if (s.generation != 2 || s.position_frames != 60000 || frame->frame.timestamp < 1.25)
                return failed(a, __LINE__);
            a->stage = 4;
            if (!video_sound_play(a->session, true))
                return failed(a, __LINE__);
        } else if (a->stage == 4 && s.ended) {
            if (s.position_frames != 96000)
                return failed(a, __LINE__);
            printf("PASS: presented real decoded red/pause/blue, PCM dummy mixer EOF96000, seek "
                   "target60000 retained\n");
            return 1;
        }
    }
    if (!s.ready || s.playing || s.seeking || a->captures)
        cui_invalidate(ui);
    return 0;
}
static void shutdown(void *u, bool success) {
    application *a = u;
    (void)success;
    if (!video_sound_close(a->session)) {
        fprintf(stderr,
                "Session teardown did not quiesce; preserving resources until process exit\n");
        abort();
    }
    if (a->audio_initialized)
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    cui_destroy(a->ui);
    cui_sdl3_destroy(a->host);
    cui_font_close(a->font);
}
int main(int argc, char **argv) {
    application a = {0};
    return cui_sdl3_run(
        argc, argv,
        (cui_sdl3_app){.user = &a, .init = initialize, .frame = iterate, .quit = shutdown});
}
