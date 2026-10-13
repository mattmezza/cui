/* SPDX-License-Identifier: MIT */
#include "cui/cui_speech.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_speech_state state;
    char text[256];
    unsigned char bytes[16384];
    cui_speech_message messages[32];
    cui_speech_sink sink;
    cui_speech_event_type events[64];
    size_t event_count, samples;
    unsigned capture_closed, recognition_closed, finishes;
    cui_speech_error capture_error, recognition_error;
    bool synchronous, finish_now, unavailable, restart_final;
    char order[64];
    size_t order_count;
} fixture;
static void mark(fixture *f, char c) {
    assert(f->order_count < sizeof f->order);
    f->order[f->order_count++] = c;
}
static bool available(void *user) {
    return !((fixture *)user)->unavailable;
}
static cui_speech_error recognize(void *user, cui_speech_sink sink, void **token) {
    fixture *f = user;
    f->sink = sink;
    mark(f, 'R');
    if (f->recognition_error)
        return f->recognition_error;
    *token = f;
    if (f->synchronous) {
        char temporary[] = "hello";
        assert(cui_speech_report_ready(sink));
        assert(cui_speech_report_hypothesis(sink, (cui_str){temporary, 5}));
        memset(temporary, '!', 5);
        assert(!cui_speech_pump(&f->state, 0));
    }
    return CUI_SPEECH_OK;
}
static cui_speech_error capture(void *user, cui_speech_format format, cui_speech_sink sink,
                                void **token) {
    fixture *f = user;
    assert(format.sample_rate == 16000 && format.channels == 1);
    assert(sink.generation == f->sink.generation);
    mark(f, 'C');
    if (f->capture_error)
        return f->capture_error;
    *token = f;
    return CUI_SPEECH_OK;
}
static void capture_close(void *user, void *token) {
    fixture *f = user;
    assert(token == f);
    f->capture_closed++;
    mark(f, 'c');
}
static void recognition_close(void *user, void *token) {
    fixture *f = user;
    assert(token == f);
    f->recognition_closed++;
    mark(f, 'r');
    assert(!cui_speech_report_finish(f->sink));
}
static void push(void *user, void *token, const int16_t *samples, size_t count) {
    fixture *f = user;
    assert(token == f && samples);
    f->samples += count;
    if (f->synchronous)
        assert(cui_speech_report_ready(f->sink));
}
static void finish(void *user, void *token) {
    fixture *f = user;
    assert(token == f);
    f->finishes++;
    mark(f, 'F');
    if (f->finish_now)
        assert(cui_speech_report_finish(f->sink));
}
static void event(void *user, cui_speech_event e) {
    fixture *f = user;
    assert(f->event_count < 64);
    f->events[f->event_count++] = e.type;
    if (e.type == CUI_SPEECH_FINAL && f->restart_final) {
        f->restart_final = false;
        assert(cui_speech_start(&f->state));
    }
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    assert(cui_speech_init(
        &f->state,
        (cui_speech_workspace){f->text, sizeof f->text, f->messages, 32, f->bytes, sizeof f->bytes},
        (cui_speech_recognizer){.user = f,
                                .available = available,
                                .start = recognize,
                                .push_audio = push,
                                .finish = finish,
                                .close = recognition_close},
        (cui_speech_input){f, capture, capture_close}, event, f));
}
static void transcript(fixture *f, const char *expected) {
    cui_str t = cui_speech_transcript(&f->state);
    assert(t.length == strlen(expected) && !memcmp(t.data, expected, t.length));
}
static void lifecycle(void) {
    fixture f;
    init(&f);
    f.synchronous = true;
    assert(cui_speech_start(&f.state));
    assert(f.state.status == CUI_SPEECH_CONNECTING);
    assert(f.event_count == 1 && f.events[0] == CUI_SPEECH_STARTED);
    transcript(&f, "");
    assert(cui_speech_pump(&f.state, 0));
    assert(f.state.status == CUI_SPEECH_RECORDING);
    transcript(&f, "hello");
    assert(cui_speech_report_phrase(f.sink, (cui_str){"Hello ", 6}));
    assert(cui_speech_report_hypothesis(f.sink, (cui_str){"世界", 6}));
    assert(cui_speech_pump(&f.state, .1));
    transcript(&f, "Hello 世界");
    assert(!cui_speech_report_phrase(f.sink, (cui_str){"\xc0\x80", 2}));
    int16_t audio[1280];
    for (size_t i = 0; i < 1280; i++)
        audio[i] = 8192;
    assert(cui_speech_report_audio(f.sink, audio, 1280, .08));
    memset(audio, 0, sizeof audio);
    assert(cui_speech_pump(&f.state, .1));
    assert(f.samples == 1280 && f.state.meter.length == 1);
    assert(cui_speech_meter_level(&f.state.meter, 0) > .6f);
    f.unavailable = true;
    assert(!cui_speech_available(&f.state));
    assert(cui_speech_stop(&f.state, 1));
    assert(f.state.status == CUI_SPEECH_STOPPING && f.capture_closed == 1 && f.finishes == 1);
    assert(!cui_speech_stop(&f.state, 1));
    (void)cui_speech_pump(&f.state, 3.99);
    assert(f.state.status == CUI_SPEECH_STOPPING);
    assert(cui_speech_pump(&f.state, 4));
    assert(f.state.status == CUI_SPEECH_IDLE);
    assert(f.events[f.event_count - 1] == CUI_SPEECH_FINAL && f.recognition_closed == 1 &&
           !f.state.meter.length);
    assert(f.order_count == 5 && !memcmp(f.order, "RCcFr", 5));
    transcript(&f, "Hello 世界");
    cui_speech_sink stale = f.sink;
    assert(!cui_speech_report_finish(stale));
    assert(cui_speech_start(&f.state));
    assert(!cui_speech_report_phrase(stale, (cui_str){"bad", 3}));
    assert(cui_speech_cancel(&f.state));
    transcript(&f, "");
    assert(f.events[f.event_count - 1] == CUI_SPEECH_CANCELLED);
    assert(!cui_speech_cancel(&f.state));
    cui_speech_sink retired = f.sink;
    cui_speech_workspace workspace = f.state.storage;
    cui_speech_recognizer recognizer = f.state.recognizer;
    cui_speech_input input = f.state.input;
    cui_speech_dispose(&f.state);
    assert(cui_speech_init(&f.state, workspace, recognizer, input, event, &f));
    assert(cui_speech_start(&f.state));
    assert(!cui_speech_report_finish(retired));
    assert(cui_speech_cancel(&f.state));
    cui_speech_dispose(&f.state);
}
static void failures(void) {
    fixture f;
    init(&f);
    f.capture_error = CUI_SPEECH_PERMISSION;
    f.synchronous = true;
    memcpy(f.text, "prior", 6);
    f.state.committed = 5;
    assert(!cui_speech_start(&f.state));
    transcript(&f, "prior");
    assert(f.recognition_closed == 1 && !f.capture_closed && f.state.status == CUI_SPEECH_IDLE);
    assert(f.event_count == 1 && f.events[0] == CUI_SPEECH_ERROR && !f.state.queued);
    f.capture_error = 0;
    assert(cui_speech_start(&f.state));
    (void)cui_speech_pump(&f.state, 0);
    char huge[256];
    memset(huge, 'a', sizeof huge);
    assert(cui_speech_report_phrase(f.sink, (cui_str){huge, sizeof huge}));
    assert(cui_speech_pump(&f.state, 0));
    assert(f.state.status == CUI_SPEECH_IDLE);
    transcript(&f, "hello");
    f.synchronous = false;
    assert(cui_speech_start(&f.state));
    for (size_t i = 0; i < 32; i++)
        assert(cui_speech_report_ready(f.sink));
    assert(!cui_speech_report_ready(f.sink));
    assert(cui_speech_pump(&f.state, 0));
    assert(f.state.status == CUI_SPEECH_IDLE && f.events[f.event_count - 1] == CUI_SPEECH_ERROR);
    cui_speech_dispose(&f.state);
}
static void reentrant_completion(void) {
    fixture f;
    init(&f);
    f.finish_now = true;
    f.synchronous = true;
    assert(cui_speech_start(&f.state));
    (void)cui_speech_pump(&f.state, 0);
    assert(cui_speech_stop(&f.state, 0));
    f.restart_final = true;
    assert(cui_speech_pump(&f.state, 0));
    assert(f.state.status == CUI_SPEECH_CONNECTING && f.state.queued == 2);
    assert(cui_speech_pump(&f.state, 0));
    assert(f.state.status == CUI_SPEECH_RECORDING);
    transcript(&f, "hello");
    assert(cui_speech_cancel(&f.state));
    cui_speech_dispose(&f.state);
}
static void meter(void) {
    cui_speech_meter a, b;
    cui_speech_meter_reset(&a, (cui_speech_format){1000, 2});
    cui_speech_meter_reset(&b, (cui_speech_format){1000, 2});
    int16_t audio[480];
    for (size_t i = 0; i < 480; i++)
        audio[i] = i < 160 ? 32767 : 0;
    assert(cui_speech_meter_push(&a, audio, 480, .24));
    assert(!cui_speech_meter_push(&b, audio, 79, .24));
    assert(cui_speech_meter_push(&b, audio + 79, 401, .24));
    assert(a.length == 3 && b.length == 3);
    for (size_t i = 0; i < 3; i++)
        assert(cui_speech_meter_level(&a, i) == cui_speech_meter_level(&b, i));
    assert(fabsf(cui_speech_meter_level(&a, 0) - .85f) < .0001f);
    assert(fabsf(cui_speech_meter_level(&a, 1) - .595f) < .0001f);
    assert(cui_speech_meter_lead(&a, .25) >= 0 && cui_speech_meter_lead(&a, .25) <= 3);
    assert(!cui_speech_meter_push(&a, audio, 1, NAN));
    for (size_t i = 0; i < 100; i++)
        (void)cui_speech_meter_push(&a, audio, 480, .5 + i * .24);
    assert(a.length == 256 && a.recorded == 303);
}

static void waveform_geometry(void) {
    cui_speech_meter m;
    cui_speech_meter_reset(&m, (cui_speech_format){16000, 1});
    m.length = 2;
    m.levels[0] = .2f;
    m.levels[1] = .9f;
    cui_rect r[3] = {{99, 99, 99, 99}};
    assert(cui_speech_waveform_rects(&m, 98, 20, 0, r, 1) == 2 && r[0].x == 99);
    assert(cui_speech_waveform_rects(&m, 98, 20, 0, r, 3) == 2);
    assert(r[0].x + r[0].w == 98 && r[0].w == 3 && r[0].h == 18);
    assert(r[1].x == 84 && r[1].h == 4);
    assert(cui_speech_waveform_rects(&m, 98, 20, 1.75f, r, 3) == 0);
    assert(cui_speech_waveform_rects(&m, NAN, 20, 0, r, 3) == SIZE_MAX);
}
#ifndef CUI_SPEECH_BASE_ONLY
static cui_draw_list frame(cui_context *ui, fixture *f, cui_speech_button_options options,
                           cui_speech_waveform_options wave) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 240, 120, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 240, .height = 120});
    (void)cui_speech_button(ui, 2, &theme, &f->state, options, 1);
    assert(cui_speech_waveform(ui, 3, &theme, &f->state, wave, 1));
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void waveform_motion(void) {
    fixture f;
    init(&f);
    assert(cui_speech_start(&f.state));
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    cui_theme theme = cui_theme_light();
    for (int reduced = 0; reduced < 2; reduced++) {
        cui_set_reduced_motion(ui, reduced != 0);
        for (int frame = 0; frame < 2; frame++) {
            assert(cui_begin_frame(ui, 200, 100, .016f));
            assert(
                cui_speech_waveform(ui, 55, &theme, &f.state, (cui_speech_waveform_options){0}, 1));
            (void)cui_end_frame(ui);
        }
        assert(cui_needs_redraw(ui) == (reduced == 0));
    }
    cui_set_reduced_motion(ui, false);
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_speech_waveform(ui, 55, &theme, &f.state,
                               (cui_speech_waveform_options){.reduced_motion = true}, 1));
    (void)cui_end_frame(ui);
    assert(!cui_needs_redraw(ui));
    cui_destroy(ui);
    cui_speech_dispose(&f.state);
}
static void styled(void) {
    fixture f;
    init(&f);
    cui_context *ui = cui_create((cui_config){.width = 240, .height = 120});
    assert(ui);
    static const float heights[] = {32, 24, 32, 20}, wave_heights[] = {20, 16, 24, 12};
    for (int size = 0; size < 4; size++) {
        cui_draw_list draw =
            frame(ui, &f, (cui_speech_button_options){.size = (cui_speech_size)size},
                  (cui_speech_waveform_options){.size = (cui_speech_size)size});
        assert(cui_bounds(ui, 2).h == heights[size] && cui_bounds(ui, 3).h == wave_heights[size]);
        bool icon = false;
        for (size_t i = 0; i < draw.count; i++)
            icon |= draw.commands[i].type == CUI_DRAW_IMAGE;
        assert(icon);
    }

    for (int n = CUI_DESIGN_XS; n <= CUI_DESIGN_9XL; n++) {
        cui_design_metrics m;
        assert(cui_design_measure((cui_design_size)n, 1, &m));
        frame(ui, &f, (cui_speech_button_options){.design_size = (cui_design_size)n},
              (cui_speech_waveform_options){.design_size = (cui_design_size)n});
        assert(cui_bounds(ui, 2).h == m.control_height);
        assert(cui_bounds(ui, 3).h == m.control_height);
    }
    frame(ui, &f, (cui_speech_button_options){.size = CUI_SPEECH_XSMALL},
          (cui_speech_waveform_options){0});
    cui_rect r = cui_bounds(ui, 2);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
    (void)frame(ui, &f, (cui_speech_button_options){.size = CUI_SPEECH_XSMALL},
                (cui_speech_waveform_options){0});
    assert(f.state.status == CUI_SPEECH_IDLE);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
    (void)frame(ui, &f, (cui_speech_button_options){.size = CUI_SPEECH_XSMALL},
                (cui_speech_waveform_options){0});
    assert(f.state.status == CUI_SPEECH_CONNECTING);
    f.unavailable = true;
    cui_focus(ui, 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    (void)frame(ui, &f, (cui_speech_button_options){0}, (cui_speech_waveform_options){0});
    assert(f.state.status == CUI_SPEECH_STOPPING);
    (void)frame(ui, &f, (cui_speech_button_options){0}, (cui_speech_waveform_options){0});
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    bool busy = false;
    for (size_t i = 0; i < count; i++)
        if (sem[i].id == 2)
            busy = sem[i].details.busy;
    assert(busy);
    assert(cui_speech_cancel(&f.state));
    f.state.recognizer.start = NULL;
    (void)frame(ui, &f, (cui_speech_button_options){0}, (cui_speech_waveform_options){0});
    assert(cui_bounds(ui, 2).w == 0);
    (void)frame(ui, &f, (cui_speech_button_options){.show_when_unsupported = true},
                (cui_speech_waveform_options){0});
    sem = cui_semantics(ui, &count);
    bool disabled = false;
    for (size_t i = 0; i < count; i++)
        if (sem[i].id == 2)
            disabled = sem[i].disabled;
    assert(disabled);
    cui_destroy(ui);
    cui_speech_dispose(&f.state);
}
#endif
int main(void) {
    lifecycle();
    failures();
    reentrant_completion();
    meter();
    waveform_geometry();
#ifndef CUI_SPEECH_BASE_ONLY
    styled();
    waveform_motion();
#endif
    return 0;
}
