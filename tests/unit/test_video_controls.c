/* SPDX-License-Identifier: MIT */
#include "cui/cui_video_controls.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_video_request requests[64];
    unsigned count;
    cui_video_view *mutate;
} log_t;
static void request(void *user, const cui_video_request *r) {
    log_t *log = user;
    assert(log->count < 64);
    log->requests[log->count++] = *r;
    if (log->mutate)
        log->mutate->source_id = 999;
}
static unsigned icons;
static bool icon(void *user, cui_str path, cui_str *svg) {
    (void)user;
    if (path.length == 14 && memcmp(path.data, "icons/play.svg", 14) == 0) {
        ++icons;
        *svg = cui_string("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'><path d='M0 "
                          "0H16V16H0Z'/></svg>");
        return true;
    }
    return false;
}
static void frame(cui_context *ui, cui_video_controls_state *state, cui_video_view v,
                  cui_video_controls_options o) {
    cui_theme theme = cui_theme_light();
    theme.icon_loader = icon;
    assert(cui_begin_frame(ui, 500, 200, .016f));
    assert(cui_video_controls(ui, 10, &theme, state, v, o));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static const cui_semantic *semantic(cui_context *ui, cui_video_part part) {
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (s[i].id == cui_video_part_id(10, part))
            return &s[i];
    return NULL;
}
static void key(cui_context *ui, cui_video_part part, cui_key k) {
    cui_focus(ui, cui_video_part_id(10, part));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void sound_frame(cui_context *ui, cui_video_controls_state *state, cui_video_view view,
                        cui_video_controls_options o, const cui_video_audio_view *audio,
                        cui_video_audio_state *sound, bool expected) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 500, 250, .016f));
    assert(cui_video_controls_with_audio(ui, 10, &theme, state, view, o, audio, sound) == expected);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void volume_touch(cui_context *ui, cui_event_type type, double fraction) {
    cui_rect track = cui_bounds(ui, cui_video_part_id(10, CUI_VIDEO_PART_VOLUME));
    assert(track.w >= 44 && track.h >= 44);
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .x = track.x + track.w * (float)fraction,
                                           .y = track.y + track.h / 2,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 5}));
}
static void test_audio(cui_context *ui) {
    cui_video_view view = {.source_id = 92, .capabilities = CUI_VIDEO_CAP_PLAY};
    assert(cui_video_init(&view.timeline, (cui_video_config){0}));
    cui_video_controls_state state = {0};
    cui_video_audio_state sound = {0};
    cui_video_audio_view audio = {
        .capabilities = CUI_VIDEO_AUDIO_CAP_MUTE | CUI_VIDEO_AUDIO_CAP_VOLUME, .volume = .4};
    log_t log = {0};
    cui_video_controls_options o = cui_video_controls_defaults();
    o.compact = true;
    o.width = 160;
    o.on_request = request;
    o.user = &log;
    sound_frame(ui, &state, view, o, &audio, NULL, false);
    assert(!semantic(ui, CUI_VIDEO_PART_MUTE));
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    const cui_semantic *volume = semantic(ui, CUI_VIDEO_PART_VOLUME);
    const cui_semantic *mute = semantic(ui, CUI_VIDEO_PART_MUTE);
    assert(volume && mute && volume->role == CUI_ROLE_SLIDER && volume->value == .4f);
    assert(volume->details.description.length == 3 &&
           !memcmp(volume->details.description.data, "40%", 3));
    cui_rect v = cui_bounds(ui, volume->id), m = cui_bounds(ui, mute->id);
    assert(v.y == m.y && m.w >= 44 && m.h >= 44 && v.x >= m.x + m.w && v.x + v.w <= 160);
    assert(cui_queue_accessibility_action(ui, mute->id, CUI_ACCESSIBILITY_ACTIVATE));
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(log.count == 1 && log.requests[0].kind == CUI_VIDEO_MUTE &&
           log.requests[0].phase == CUI_SLIDER_COMMIT && log.requests[0].value.enabled);
    assert(semantic(ui, CUI_VIDEO_PART_MUTE)->name.length == 4); /* Owner has not accepted. */
    audio.muted = true;
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(semantic(ui, CUI_VIDEO_PART_MUTE)->name.length == 6);
    key(ui, CUI_VIDEO_PART_VOLUME, CUI_KEY_RIGHT);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(log.count == 2 && log.requests[1].kind == CUI_VIDEO_VOLUME &&
           log.requests[1].phase == CUI_SLIDER_COMMIT &&
           fabs(log.requests[1].value.volume - .45) < 1e-9 && log.requests[1].sequence == 2 &&
           log.requests[1].generation == 1 && log.requests[1].source_id == 92);
    assert(audio.muted && semantic(ui, CUI_VIDEO_PART_VOLUME)->value == .4f);
    volume_touch(ui, CUI_EVENT_POINTER_DOWN, .25);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(sound.volume_slider.dragging && log.count == 2);
    volume_touch(ui, CUI_EVENT_POINTER_MOVE, .75);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(sound.volume_slider.preview == .75 && log.count == 2);
    volume_touch(ui, CUI_EVENT_POINTER_UP, .75);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(!sound.volume_slider.dragging && log.count == 3 && log.requests[2].value.volume == .75);
    volume_touch(ui, CUI_EVENT_POINTER_DOWN, .25);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(sound.volume_slider.dragging);
    view.timeline.generation++;
    volume_touch(ui, CUI_EVENT_POINTER_UP, .75);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(!sound.volume_slider.dragging && log.count == 3);
    volume_touch(ui, CUI_EVENT_POINTER_DOWN, .25);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(sound.volume_slider.dragging);
    volume_touch(ui, CUI_EVENT_CANCEL, .25);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(!sound.volume_slider.dragging && log.count == 3);
    volume_touch(ui, CUI_EVENT_POINTER_DOWN, .25);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(sound.volume_slider.dragging);
    volume_touch(ui, CUI_EVENT_POINTER_UP, .75);
    sound_frame(ui, &state, view, o, NULL, NULL, true);
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(!sound.volume_slider.dragging && log.count == 3);
    uint64_t last_active_frame = sound.frame;
    key(ui, CUI_VIDEO_PART_VOLUME, CUI_KEY_END);
    assert(cui_begin_frame(ui, 500, 250, .016f));
    cui_begin_box(ui, 88, (cui_box){.inert = true});
    cui_theme theme = cui_theme_light();
    assert(cui_video_controls_with_audio(ui, 10, &theme, &state, view, o, &audio, &sound));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(log.count == 3 && sound.frame == last_active_frame);
    audio.volume = NAN;
    sound_frame(ui, &state, view, o, &audio, &sound, false);
    assert(!semantic(ui, CUI_VIDEO_PART_PLAY));
    audio.capabilities = CUI_VIDEO_AUDIO_CAP_MUTE;
    o.disabled = true;
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(!semantic(ui, CUI_VIDEO_PART_VOLUME) && semantic(ui, CUI_VIDEO_PART_MUTE)->disabled);
    o.disabled = false;
    view.status = CUI_VIDEO_LOADING;
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(semantic(ui, CUI_VIDEO_PART_MUTE)->disabled);
    view.status = CUI_VIDEO_BUFFERING;
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(!semantic(ui, CUI_VIDEO_PART_MUTE)->disabled);
    audio.capabilities = CUI_VIDEO_AUDIO_CAP_VOLUME;
    audio.volume = .25;
    o.on_request = NULL;
    sound_frame(ui, &state, view, o, &audio, &sound, true);
    assert(!semantic(ui, CUI_VIDEO_PART_MUTE) &&
           semantic(ui, CUI_VIDEO_PART_VOLUME)->details.readonly);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 200});
    assert(ui);
    cui_video_view v = {.source_id = 44, .capabilities = 15};
    assert(cui_video_init(
        &v.timeline,
        (cui_video_config){.duration = 120, .duration_known = true, .seekable = true}));
    log_t log = {0};
    cui_video_controls_state state = {0};
    cui_video_controls_options o = cui_video_controls_defaults();
    o.visible = 31;
    o.on_request = request;
    o.user = &log;
    o.play_icon = cui_string("play");
    frame(ui, &state, v, o);
    assert(icons > 0);
    key(ui, CUI_VIDEO_PART_PLAY, CUI_KEY_ENTER);
    frame(ui, &state, v, o);
    assert(log.count == 1 && log.requests[0].kind == CUI_VIDEO_PLAYING &&
           log.requests[0].value.enabled);
    assert(!v.timeline.playing); /* owner rejects */
    assert(semantic(ui, CUI_VIDEO_PART_PLAY)->name.length == 4);
    assert(cui_queue_accessibility_action(ui, cui_video_part_id(10, CUI_VIDEO_PART_PLAY),
                                          CUI_ACCESSIBILITY_ACTIVATE));
    frame(ui, &state, v, o);
    assert(log.count == 2 && log.requests[1].kind == CUI_VIDEO_PLAYING);
    log.count = 1; /* Keep subsequent lifecycle indexes independent. */
    key(ui, CUI_VIDEO_PART_TIMELINE, CUI_KEY_RIGHT);
    frame(ui, &state, v, o);
    assert(log.count == 4 && log.requests[1].phase == CUI_SLIDER_BEGIN &&
           log.requests[3].phase == CUI_SLIDER_COMMIT && log.requests[3].value.seconds == 1);
    assert(log.requests[3].source_id == 44 && log.requests[3].generation == 1);
    assert(semantic(ui, CUI_VIDEO_PART_TIMELINE)->details.maximum == 1);
    cui_rect track = cui_bounds(ui, cui_video_part_id(10, CUI_VIDEO_PART_TIMELINE));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = track.x + 80,
                                           .y = track.y + 16,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 3}));
    frame(ui, &state, v, o);
    assert(state.scrub.dragging);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_CANCEL, .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 3}));
    frame(ui, &state, v, o);
    assert(!state.scrub.dragging && log.requests[log.count - 1].phase == CUI_SLIDER_CANCEL);
    key(ui, CUI_VIDEO_PART_FULLSCREEN, CUI_KEY_ENTER);
    frame(ui, &state, v, o);
    assert(log.requests[log.count - 1].kind == CUI_VIDEO_FULLSCREEN &&
           log.requests[log.count - 1].value.enabled);
    key(ui, CUI_VIDEO_PART_BACK, CUI_KEY_ENTER);
    frame(ui, &state, v, o);
    assert(log.requests[log.count - 1].kind == CUI_VIDEO_SKIP &&
           log.requests[log.count - 1].value.seconds == -10);
    unsigned before_skips = log.count;
    key(ui, CUI_VIDEO_PART_FORWARD, CUI_KEY_ENTER);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame(ui, &state, v, o);
    assert(log.count == before_skips + 2);
    key(ui, CUI_VIDEO_PART_RATE, CUI_KEY_ENTER);
    frame(ui, &state, v, o);
    assert(log.requests[log.count - 1].kind == CUI_VIDEO_RATE);
    /* Snapshot remains stable even when callback changes application source. */
    log.mutate = &v;
    key(ui, CUI_VIDEO_PART_TIMELINE, CUI_KEY_END);
    frame(ui, &state, v, o);
    assert(log.requests[log.count - 1].source_id == 44);
    log.mutate = NULL;
    frame(ui, &state, v, o);
    assert(state.source_id == 999 && !state.scrub.dragging);
    unsigned count = log.count;
    key(ui, CUI_VIDEO_PART_PLAY, CUI_KEY_ENTER);
    cui_theme inert_theme = cui_theme_light();
    assert(cui_begin_frame(ui, 500, 200, .016f));
    cui_begin_box(ui, 77, (cui_box){.inert = true});
    assert(cui_video_controls(ui, 10, &inert_theme, &state, v, o));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(log.count == count);
    frame(ui, &state, v, o);

    o.on_request = NULL;
    key(ui, CUI_VIDEO_PART_PLAY, CUI_KEY_ENTER);
    frame(ui, &state, v, o);
    assert(log.count == count && semantic(ui, CUI_VIDEO_PART_PLAY)->disabled);
    assert(semantic(ui, CUI_VIDEO_PART_TIMELINE)->details.readonly);
    state.sequence = UINT64_MAX;
    o.on_request = request;
    frame(ui, &state, v, o);
    assert(semantic(ui, CUI_VIDEO_PART_PLAY)->disabled);
    assert(semantic(ui, CUI_VIDEO_PART_TIMELINE)->details.readonly);
    state.sequence = log.requests[log.count - 1].sequence;
    o.visible = CUI_VIDEO_CONTROL_PLAY;
    frame(ui, &state, v, o);
    assert(!semantic(ui, CUI_VIDEO_PART_TIMELINE) && !semantic(ui, CUI_VIDEO_PART_FULLSCREEN));
    o.on_request = request;
    v.capabilities = 0;
    key(ui, CUI_VIDEO_PART_PLAY, CUI_KEY_ENTER);
    frame(ui, &state, v, o);
    assert(log.count == count && semantic(ui, CUI_VIDEO_PART_PLAY)->disabled);
    /* Narrow rows retain44-unit targets without overlapping/offscreen buttons. */
    o.visible = 31;
    o.width = 160;
    v.capabilities = 15;
    frame(ui, &state, v, o);
    for (int part = CUI_VIDEO_PART_PLAY; part <= CUI_VIDEO_PART_RATE; ++part) {
        const cui_semantic *a = semantic(ui, (cui_video_part)part);
        assert(a && a->bounds.x >= 0 && a->bounds.x + a->bounds.w <= 160.01f);
        for (int other = part + 1; other <= CUI_VIDEO_PART_RATE; ++other) {
            const cui_semantic *b = semantic(ui, (cui_video_part)other);
            assert(b);
            assert(a->bounds.x + a->bounds.w <= b->bounds.x ||
                   b->bounds.x + b->bounds.w <= a->bounds.x ||
                   a->bounds.y + a->bounds.h <= b->bounds.y ||
                   b->bounds.y + b->bounds.h <= a->bounds.y);
        }
    }
    /* Unsupported stale frames reject before submitting any semantics. */
    unsigned char pixels[4] = {255, 0, 0, 255};
    cui_video_player_frame f = {.source_id = 44,
                                .frame = {.bitmap = {pixels, 1, 1, 4, 1}, .generation = 1}};
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 500, 200, .016f));
    assert(!cui_video_player(
        ui, 50, &theme, &state, v, &f,
        (cui_video_player_options){
            .surface = {.geometry = {.width = 200, .height = 100}, .opacity = 1}}));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 0);
    o.width = NAN;
    assert(!cui_video_controls(ui, 10, &theme, &state, v, o));
    test_audio(ui);
    cui_destroy(ui);
    return 0;
}
