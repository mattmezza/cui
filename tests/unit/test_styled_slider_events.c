/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "cui/cui_styled_slider_events.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_software *renderer;
    cui_slider_state state;
    cui_slider_options options;
    cui_slider_style_options style;
    cui_theme theme;
    double owner;
    bool accept, omit;
    cui_component_slider_request events[64];
    size_t count;
    cui_draw_list draw;
} fixture;
static void request(void *user, const cui_component_slider_request *e) {
    fixture *f = user;
    assert(f->count < 64);
    f->events[f->count++] = *e;
    assert(!(e->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    if (f->accept && e->phase == CUI_SLIDER_UPDATE)
        f->owner = e->requested;
}
static cui_component_slider_result frame(fixture *f, bool handler) {
    assert(cui_begin_frame(f->ui, 400, 160, .016f));
    cui_component_slider_result r = {0};
    if (!f->omit)
        r = cui_slider_events(f->ui, 10, &f->theme, cui_string("Position"), f->owner, &f->state,
                              f->options, f->style,
                              (cui_component_slider_handler){handler ? request : NULL, f});
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    /* Pixel fixtures intentionally use no font; label-only fixture inspects
     * real Core text commands and geometry without pretending to raster glyphs. */
    if (f->style.hide_label)
        assert(cui_software_render(f->renderer, &f->draw, (cui_color){0, 0, 0, 255}));
    return r;
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    f->ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    f->renderer = cui_software_create(400, 160, NULL);
    assert(f->ui && f->renderer);
    f->theme = cui_theme_light();
    f->theme.radius = 0;
    f->style = (cui_slider_style_options){.width = 200,
                                          .hide_label = true,
                                          .custom_colors = true,
                                          .track_color = {20, 40, 200, 255},
                                          .fill_color = {240, 30, 10, 255}};
    f->options = (cui_slider_options){.minimum = 0, .maximum = 1, .step = .125};
    f->owner = .25;
    assert(frame(f, true).valid);
}
static void finish(fixture *f) {
    cui_software_destroy(f->renderer);
    cui_destroy(f->ui);
}
static void pixel(fixture *f, unsigned x, unsigned y, cui_color c) {
    unsigned w, h;
    const unsigned char *p = cui_software_pixels(f->renderer, &w, &h);
    assert(x < w && y < h);
    p += ((size_t)y * w + x) * 4;
    if (p[0] != c.r || p[1] != c.g || p[2] != c.b || p[3] != c.a)
        fprintf(stderr, "pixel %u,%u got%u,%u,%u,%u expected%u,%u,%u,%u\n", x, y, p[0], p[1], p[2],
                p[3], c.r, c.g, c.b, c.a);
    assert(p[0] == c.r && p[1] == c.g && p[2] == c.b && p[3] == c.a);
}
static void fill(fixture *f, double ratio) {
    cui_rect b = cui_bounds(f->ui, 10);
    assert(b.w == f->style.width && b.h == 24);
    unsigned y = (unsigned)(b.y + b.h / 2);
    unsigned boundary = (unsigned)(b.x + b.w * ratio);
    if (ratio > 0)
        pixel(f, boundary - 1, y, f->style.fill_color);
    if (ratio < 1)
        pixel(f, boundary, y, f->style.track_color);
    if (cui_focused(f->ui) != 10)
        pixel(f, (unsigned)b.x, (unsigned)b.y, (cui_color){0, 0, 0, 255});
}
static void pointer(fixture *f, cui_event_type type, float fraction) {
    cui_rect b = cui_bounds(f->ui, 10);
    assert(cui_queue_event(f->ui, (cui_event){.type = type,
                                              .x = b.x + b.w * fraction,
                                              .y = b.y + b.h / 2,
                                              .pointer_type = CUI_POINTER_TOUCH,
                                              .pointer_id = 7,
                                              .pointer_device = 9,
                                              .pointer_button = CUI_POINTER_PRIMARY}));
}
static void key(fixture *f, cui_key k) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void semantics(fixture *f, float value, bool readonly) {
    size_t count;
    const cui_semantic *s = cui_semantics(f->ui, &count);
    bool found = false;
    for (size_t i = 0; i < count; ++i)
        if (s[i].id == 10) {
            assert(s[i].role == CUI_ROLE_SLIDER && s[i].value == value);
            assert(s[i].details.minimum == 0 && s[i].details.maximum == 1 &&
                   s[i].details.readonly == readonly);
            found = true;
        }
    assert(found);
}
static void preview(void) {
    fixture f;
    init(&f);
    fill(&f, .25);
    semantics(&f, .25f, false);
    pointer(&f, CUI_EVENT_POINTER_DOWN, .75f);
    cui_component_slider_result r = frame(&f, true);
    assert(r.begin_count == 1 && r.request_count == 1 && f.state.dragging && f.owner == .25);
    fill(&f, .75);
    semantics(&f, .25f, false);
    assert(f.events[1].current == .25 && f.events[1].requested == .75);
    assert(f.events[1].input.pointer_id == 7 && f.events[1].input.pointer_device == 9);
    assert(f.events[1].event.event_index == 0 && f.events[1].event.frame_sequence > 0);
    pointer(&f, CUI_EVENT_POINTER_UP, .5f);
    r = frame(&f, true);
    assert(r.request_count == 1 && r.commit_count == 1 && !f.state.dragging);
    fill(&f, .25); /* Rejected final proposal does not become a confirmed paint. */
    f.accept = true;
    pointer(&f, CUI_EVENT_POINTER_DOWN, .75f);
    frame(&f, true);
    assert(f.owner == .75);
    fill(&f, .75);
    semantics(&f, .25f, false);
    pointer(&f, CUI_EVENT_POINTER_UP, .5f);
    frame(&f, true);
    assert(f.owner == .5);
    fill(&f, .75);
    semantics(&f, .75f, false);
    frame(&f, true);
    fill(&f, .5);
    semantics(&f, .5f, false);
    f.accept = false;
    cui_focus(f.ui, 10);
    key(&f, CUI_KEY_RIGHT);
    key(&f, CUI_KEY_RIGHT);
    r = frame(&f, true);
    assert(r.request_count == 2 && r.commit_count == 2);
    assert(f.events[f.count - 2].requested == .625);
    fill(&f, .5);
    pointer(&f, CUI_EVENT_POINTER_DOWN, .75f);
    frame(&f, true);
    key(&f, CUI_KEY_ESCAPE);
    r = frame(&f, true);
    assert(r.cancel_count == 1 && !f.state.dragging);
    fill(&f, .5);
    pointer(&f, CUI_EVENT_POINTER_UP, .75f);
    frame(&f, true);
    pointer(&f, CUI_EVENT_POINTER_DOWN, .75f);
    frame(&f, true);
    f.options.readonly = true;
    r = frame(&f, true);
    assert(r.cancel_count == 1 && f.events[f.count - 1].reason == CUI_SLIDER_EVENT_READONLY);
    assert(!f.events[f.count - 1].has_input);
    fill(&f, .5);
    semantics(&f, .5f, true);
    pointer(&f, CUI_EVENT_POINTER_UP, .75f);
    frame(&f, true);
    f.options.readonly = false;
    pointer(&f, CUI_EVENT_POINTER_DOWN, .75f);
    frame(&f, true);
    r = frame(&f, false);
    assert(!r.cancel_count && !f.state.dragging);
    fill(&f, .5);
    semantics(&f, .5f, true);
    f.omit = true;
    frame(&f, true);
    size_t n;
    const cui_semantic *s = cui_semantics(f.ui, &n);
    for (size_t i = 0; i < n; ++i)
        assert(s[i].id != 10);
    finish(&f);
}
static void precision_and_geometry(void) {
    fixture f;
    init(&f);
    f.options =
        (cui_slider_options){.minimum = 10000000000., .maximum = 10000000001., .step = .125};
    f.owner = 10000000000.25;
    frame(&f, true);
    fill(&f, .25);
    cui_focus(f.ui, 10);
    key(&f, CUI_KEY_RIGHT);
    frame(&f, true);
    assert(f.events[1].requested == 10000000000.375 && f.events[1].current == 10000000000.25);
    pointer(&f, CUI_EVENT_POINTER_DOWN, .75f);
    frame(&f, true);
    assert(f.state.preview == 10000000000.75);
    fill(&f, .75);
    /* Changed width is painted now; input still uses last completed width200. */
    cui_rect old = cui_bounds(f.ui, 10);
    f.style.width = 100;
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                             .x = old.x + 100,
                                             .y = old.y + 12,
                                             .pointer_type = CUI_POINTER_TOUCH,
                                             .pointer_id = 7,
                                             .pointer_device = 9}));
    frame(&f, true);
    assert(f.state.preview == 10000000000.5);
    fill(&f, .5);
    pointer(&f, CUI_EVENT_POINTER_UP, 1);
    frame(&f, true);
    assert(f.events[f.count - 1].requested == 10000000001.);
    fill(&f, .25);
    f.options = (cui_slider_options){.minimum = -DBL_MAX / 2, .maximum = DBL_MAX / 2};
    f.owner = 0;
    frame(&f, true);
    fill(&f, .5);
    f.owner = f.options.minimum;
    frame(&f, true);
    fill(&f, 0);
    f.owner = f.options.maximum;
    frame(&f, true);
    fill(&f, 1);
    f.options = (cui_slider_options){.minimum = 2, .maximum = 2};
    f.owner = 2;
    frame(&f, true);
    fill(&f, 0);
    finish(&f);
}
static void invalid(void) {
    fixture f;
    init(&f);
    cui_slider_state saved = f.state;
    const float bad[] = {NAN, INFINITY, -1, 4097};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        f.style.width = bad[i];
        cui_component_slider_result r = frame(&f, true);
        assert(!r.valid && !r.submitted && !f.count && !f.draw.count);
        assert(!memcmp(&saved, &f.state, sizeof saved));
    }
    f.style.width = 200;
    f.style.hit_height = 4;
    assert(!frame(&f, true).valid);
    f.style.hit_height = 0;
    f.options.user = &f;
    assert(!frame(&f, true).valid);
    f.options.user = NULL;
    f.options.minimum = -DBL_MAX;
    f.options.maximum = DBL_MAX;
    f.owner = 0;
    assert(!frame(&f, true).valid);
    assert(!memcmp(&saved, &f.state, sizeof saved));
    f.options = (cui_slider_options){.minimum = 0, .maximum = 1};
    f.owner = 2;
    assert(!frame(&f, true).valid);
    f.owner = .25;
    f.theme.radius = NAN;
    assert(!frame(&f, true).valid);
    f.theme.radius = 0;
    f.state.id = 11;
    assert(!frame(&f, true).valid);
    f.state = saved;
    f.state.dragging = true;
    f.state.preview = NAN;
    assert(!frame(&f, true).valid && isnan(f.state.preview));
    f.state = saved;
    assert(!cui_slider_events(f.ui, 10, &f.theme, cui_string("Position"), f.owner, &f.state,
                              f.options, f.style, (cui_component_slider_handler){request, &f})
                .valid); /* Outside frame. */
    assert(!memcmp(&saved, &f.state, sizeof saved));
    f.style = (cui_slider_style_options){.hide_label = true};
    assert(frame(&f, true).valid);
    assert(cui_bounds(f.ui, 10).w == 240 && cui_bounds(f.ui, 10).h == 24);
    finish(&f);
}
static cui_component_slider_result base_frame(fixture *f) {
    assert(cui_begin_frame(f->ui, 400, 160, .016f));
    cui_begin_box(f->ui, 10,
                  (cui_box){.width = 200, .height = 24, .fixed_width = true, .fixed_height = true});
    cui_component_slider_result r =
        cui_base_slider_events(f->ui, 10, cui_string("Position"), f->owner, &f->state, f->options,
                               (cui_component_slider_handler){request, f});
    cui_end_box(f->ui);
    cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void phase_equivalence(void) {
    fixture a, b;
    init(&a);
    init(&b);
    base_frame(&a);
    pointer(&a, CUI_EVENT_POINTER_DOWN, .25f);
    pointer(&b, CUI_EVENT_POINTER_DOWN, .25f);
    pointer(&a, CUI_EVENT_POINTER_MOVE, .75f);
    pointer(&b, CUI_EVENT_POINTER_MOVE, .75f);
    pointer(&a, CUI_EVENT_POINTER_UP, .5f);
    pointer(&b, CUI_EVENT_POINTER_UP, .5f);
    cui_component_slider_result x = base_frame(&a), y = frame(&b, true);
    assert(x.begin_count == y.begin_count && x.request_count == y.request_count &&
           x.commit_count == y.commit_count && a.count == b.count);
    for (size_t i = 0; i < a.count; ++i) {
        const cui_component_slider_request *p = a.events + i, *q = b.events + i;
        assert(p->phase == q->phase && p->current == q->current && p->requested == q->requested);
        assert(p->event.source == q->event.source && p->event.flags == q->event.flags &&
               p->event.event_index == q->event.event_index);
        assert(p->input.pointer_id == q->input.pointer_id && p->input.x == q->input.x);
    }
    finish(&a);
    finish(&b);
}
static void cancellation_and_label(void) {
    fixture f;
    init(&f);
    for (unsigned mode = 0; mode < 3; ++mode) {
        pointer(&f, CUI_EVENT_POINTER_DOWN, .75f);
        assert(frame(&f, true).begin_count == 1);
        if (mode == 0)
            f.options.disabled = true;
        else if (mode == 1)
            f.options.minimum = f.options.maximum = f.owner;
        else
            assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_CANCEL}));
        cui_component_slider_result r = frame(&f, true);
        assert(r.cancel_count == 1 && !f.state.dragging);
        const cui_component_slider_request *e = &f.events[f.count - 1];
        assert(e->phase == CUI_SLIDER_CANCEL);
        assert(e->reason == (mode == 0   ? CUI_SLIDER_EVENT_DISABLED
                             : mode == 1 ? CUI_SLIDER_EVENT_ZERO_SPAN
                                         : CUI_SLIDER_EVENT_INPUT));
        assert(e->has_input == (mode == 2));
        f.options = (cui_slider_options){.minimum = 0, .maximum = 1, .step = .125};
        pointer(&f, CUI_EVENT_POINTER_UP, .75f);
        frame(&f, true);
    }
    cui_rect b = cui_bounds(f.ui, 10);
    size_t old = f.count;
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                             .x = b.x + 100,
                                             .y = b.y + 12,
                                             .pointer_button = CUI_POINTER_SECONDARY}));
    assert(!frame(&f, true).request_count && f.count == old);
    cui_focus(f.ui, 10);
    key(&f, CUI_KEY_HOME);
    key(&f, CUI_KEY_END);
    cui_component_slider_result r = frame(&f, true);
    assert(r.commit_count == 2 && f.events[f.count - 1].requested == 1);
    f.style.hide_label = false;
    f.style.track_height = 10;
    f.style.hit_height = 30;
    frame(&f, true);
    b = cui_bounds(f.ui, 10);
    assert(b.y > 0 && b.w == 200 && b.h == 30);
    bool label = false;
    for (size_t i = 0; i < f.draw.count; ++i)
        if (f.draw.commands[i].type == CUI_DRAW_TEXT) {
            assert(f.draw.commands[i].bounds.y < b.y);
            label = true;
        }
    assert(label);
    finish(&f);
}
int main(void) {
    preview();
    precision_and_geometry();
    invalid();
    phase_equivalence();
    cancellation_and_label();
    puts("PASS Styled double Slider pixels, preview/owner separation, geometry, precision and "
         "atomicity");
    return 0;
}
