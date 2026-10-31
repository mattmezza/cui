/* SPDX-License-Identifier: MIT */
#include "styled_slider_fixture.h"
#include <float.h>
static void interaction(void) {
    machine *m = create();
    okay(frame(m, 0, NULL));
    assert(!m->motion.owner);
    over(m, 1);
    okay(frame(m, 0, NULL));
    assert(m->motion.owner == 100 && m->motion.thumbs[1].target == 1 &&
           m->motion.thumbs[1].position == 0);
    assert(m->motion.next_wake_seconds > cui_frame_time(m->ui));
    okay(frame(m, .05f, NULL));
    double old = m->motion.thumbs[1].position;
    assert(old > 0 && old < 1);
    pointer(m, CUI_EVENT_POINTER_MOVE, 350, 250, false);
    okay(frame(m, 0, NULL));
    assert(m->motion.thumbs[1].target == 0 && m->motion.thumbs[1].position == old);
    okay(frame(m, .05f, NULL));
    assert(isfinite(m->motion.thumbs[1].position));
    m->style.flags = CUI_STYLED_SLIDER_REDUCED_MOTION;
    over(m, 1);
    okay(frame(m, 0, NULL));
    assert(m->motion.thumbs[1].position == 1 && !m->motion.next_wake_seconds);
    cui_focus(m->ui, part(CUI_SLIDER_MODEL_END_THUMB));
    m->accept = true;
    m->count = 0;
    key(m, CUI_KEY_LEFT);
    key(m, CUI_KEY_LEFT);
    okay(frame(m, 0, NULL));
    assert(m->count == 6 && m->value.end == .5 && m->requests[1].current.end == .75 &&
           m->requests[4].current.end == .75);
    assert(m->requests[1].requested.end == .625 && m->requests[4].requested.end == .5);
    assert(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_FOCUS_RING)).w == 24);
    m->style.flags |= CUI_STYLED_SLIDER_HIDE_FOCUS;
    okay(frame(m, 0, NULL));
    assert(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_FOCUS_RING)).w == 0);
    for (unsigned touch = 0; touch < 2; ++touch) {
        m->style.flags = CUI_STYLED_SLIDER_REDUCED_MOTION;
        m->count = 0;
        pointer(m, CUI_EVENT_POINTER_DOWN, 100, 12, touch != 0);
        pointer(m, CUI_EVENT_POINTER_MOVE, 180, 12, touch != 0);
        pointer(m, CUI_EVENT_POINTER_UP, 120, 12, touch != 0);
        okay(frame(m, 0, NULL));
        assert(!(m->model.flags & CUI_SLIDER_MODEL_DRAGGING));
        assert(m->count >= 3);
        for (size_t i = 0; i < m->count; ++i)
            assert(m->requests[i].input.pointer_type ==
                   (touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE));
    }
    /* Track/root capture chooses an endpoint, ring follows that logical owner. */
    m->accept = false;
    m->value = (cui_slider_model_value_v1){.kind = CUI_SLIDER_MODEL_SINGLE, .end = .75};
    okay(frame(m, 0, NULL));
    pointer(m, CUI_EVENT_POINTER_DOWN, 80, 12, false);
    okay(frame(m, 0, NULL));
    assert(m->model.flags & CUI_SLIDER_MODEL_DRAGGING);
    assert(m->model.active_thumb == 1 && m->motion.thumbs[1].target == 1);
    m->options.flags = CUI_SLIDER_MODEL_DISABLED_BIT;
    m->count = 0;
    okay(frame(m, 0, NULL));
    assert(m->count == 1 && m->requests[0].phase == CUI_SLIDER_MODEL_CANCEL);
    assert(!m->motion.initialized_mask && !m->motion.next_wake_seconds &&
           cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_SURFACE)).w == 0);
    m->options.flags = CUI_SLIDER_MODEL_READONLY_BIT;
    okay(frame(m, 0, NULL));
    assert(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_SURFACE)).w == 14);
    m->options.flags = 0;
    m->handler = false;
    m->count = 0;
    key(m, CUI_KEY_RIGHT);
    okay(frame(m, 0, NULL));
    assert(!m->count);
    destroy(m);
}
static void suppression(void) {
    machine *m = create();
    okay(frame(m, 0, NULL));
    okay(frame(m, .01f, NULL));
    for (unsigned mode = 0; mode < 2; ++mode) {
        unsigned char before[sizeof m->motion];
        memcpy(before, &m->motion, sizeof before);
        unsigned char model[sizeof m->model];
        memcpy(model, &m->model, sizeof model);
        m->inert = mode == 0;
        m->layout_only = mode == 1;
        cui_styled_slider_model_result_v1 r = frame(m, .05f, NULL);
        okay(r);
        assert(!r.next_wake_seconds);
        assert(!memcmp(before, &m->motion, sizeof before) &&
               !memcmp(model, &m->model, sizeof model));
    }
    m->inert = m->layout_only = false;
    okay(frame(m, 0, NULL));
    assert(m->motion.thumbs[1].position == 0);
    unsigned char before[sizeof m->motion];
    memcpy(before, &m->motion, sizeof before);
    assert(cui_begin_frame(m->ui, 400, 300, 0));
    (void)cui_end_frame(m->ui); /* omission */
    assert(!memcmp(before, &m->motion, sizeof before));
    okay(frame(m, 0, NULL));
    assert(m->motion.thumbs[1].position == 0);
    cui_focus(m->ui, part(CUI_SLIDER_MODEL_END_THUMB));
    m->count = 0;
    m->invalidate = true;
    key(m, CUI_KEY_RIGHT);
    key(m, CUI_KEY_RIGHT);
    memcpy(before, &m->motion, sizeof before);
    cui_styled_slider_model_result_v1 r = frame(m, 0, NULL);
    assert(r.render_status == CUI_STYLED_SLIDER_CORE_FAILURE && !r.next_wake_seconds);
    assert(m->count == 1 && !memcmp(before, &m->motion, sizeof before));
    m->invalidate = false;
    memcpy(before, &m->motion, sizeof before);
    r = frame(m, 0, NULL);
    okay(r);
    assert(!r.next_wake_seconds && !memcmp(before, &m->motion, sizeof before));
    okay(frame(m, 0, NULL));
    destroy(m);
}
static void atomic(void) {
    machine *m = create();
    okay(frame(m, 0, NULL));
    okay(frame(m, 0, NULL));
    size_t aurora = 0;
    bool found = false;
    for (size_t i = 0; i < cui_slider_palette_preset_count(); ++i) {
        cui_slider_palette p;
        assert(cui_slider_palette_preset_get(i, &p));
        if (p.thumb.kind == CUI_SLIDER_BRUSH_LINEAR) {
            aurora = i;
            found = true;
            break;
        }
    }
    assert(found && cui_slider_palette_preset_get(aurora, &m->style.style.palette));
    cui_focus(m->ui, part(CUI_SLIDER_MODEL_END_THUMB));
    key(m, CUI_KEY_LEFT);
    pointer(m, CUI_EVENT_POINTER_DOWN, 150, 12, true);
    pointer(m, CUI_EVENT_POINTER_UP, 150, 12, true);
    cui_accessibility_set_value_v1 numeric = {1, 0, .5};
    assert(cui_queue_accessibility_set_value_v1(m->ui, part(CUI_SLIDER_MODEL_END_THUMB),
                                                sizeof numeric, &numeric));
    assert(cui_begin_frame(m->ui, 400, 300, 0));
    unsigned char model[sizeof m->model];
    memcpy(model, &m->model, sizeof model);
    unsigned char motion[sizeof m->motion];
    memcpy(motion, &m->motion, sizeof motion);
    cui_styled_slider_model_result_v1 r = submit(m);
    assert(r.render_status == CUI_STYLED_SLIDER_UNSUPPORTED && !r.base.flags && !m->count);
    assert(!memcmp(model, &m->model, sizeof model) && !memcmp(motion, &m->motion, sizeof motion));
    size_t count;
    const cui_event *events = cui_frame_events(m->ui, &count);
    assert(events && count == 4);
    for (size_t i = 0; i < count; ++i)
        assert(!cui_event_consumed(m->ui, i));
    cui_draw_list list = cui_end_frame(m->ui);
    assert(!list.count); /* parent transparent */
    m->options.flags = CUI_SLIDER_MODEL_DISABLED_BIT;
    okay(frame(m, 0, NULL));
    m->options.flags = 0;
    m->style.style.rem = 1;
    okay(frame(m, 0, NULL));
    assert(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_SURFACE)).w == 0);
    m->style.style.rem = 16;
    m->layout_only = true;
    okay(frame(m, 0, NULL));
    m->layout_only = false;
    m->style.style.color_overrides = CUI_SLIDER_STYLE_THUMB_OVERRIDE;
    m->style.style.thumb_override = (cui_slider_palette_color){.7f, .2f, .3f, 0};
    okay(frame(m, 0, NULL));
    assert(cui_styled_slider_model_part_id_v1(0, 0, 0) == 0 &&
           cui_styled_slider_model_part_id_v1(100, 2, 0) == 0 &&
           cui_styled_slider_model_part_id_v1(100, 0, 3) == 0);
    cui_styled_slider_model_state_v1 out;
    unsigned char before[sizeof out];
    memset(&out, 0xa5, sizeof out);
    memcpy(before, &out, sizeof before);
    assert(!cui_styled_slider_model_state_init_v1(sizeof out - 1, &out) &&
           !memcmp(&out, before, sizeof out));
    cui_theme t = cui_theme_light();
    cui_styled_slider_model_options_v1 options;
    unsigned char prior[sizeof options];
    memset(&options, 0xa5, sizeof options);
    memcpy(prior, &options, sizeof prior);
    t.radius = NAN;
    assert(!cui_styled_slider_model_options_from_theme_v1(&t, 16, 0, &options) &&
           !memcmp(prior, &options, sizeof options));
    t = cui_theme_light();
    t.radius = 0;
    assert(cui_styled_slider_model_options_from_theme_v1(&t, 24, 1, &options));
    assert(options.style.layout == CUI_SLIDER_STYLE_FIXED && options.style.length == 120 &&
           options.style.full_radius == 0);
    destroy(m);
}
static cui_draw_v2_support support(void) {
    return (cui_draw_v2_support){.size = sizeof(cui_draw_v2_support),
                                 .abi_version = CUI_DRAW_ABI_V2,
                                 .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                                 .features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                                 .max_commands = 256,
                                 .max_backgrounds = 8,
                                 .max_pixels = 8192,
                                 .max_pixels_per_background = 4096,
                                 .cpu_bytes = 65536,
                                 .texture_bytes = 65536,
                                 .transfer_bytes = 65536,
                                 .metadata_bytes = 65536};
}
static void transport(void) {
    machine *m = create();
    m->style.render_mode = CUI_STYLED_SLIDER_BACKGROUND_V2;
    m->style.support = support();
    for (size_t i = 0; i < cui_slider_palette_preset_count(); ++i) {
        cui_slider_palette p;
        assert(cui_slider_palette_preset_get(i, &p));
        if (p.thumb.kind == CUI_SLIDER_BRUSH_LINEAR) {
            m->style.style.palette = p;
            break;
        }
    }
    assert(m->style.style.palette.thumb.kind == CUI_SLIDER_BRUSH_LINEAR);
    cui_frame_v2_options options = {.size = sizeof options,
                                    .support = support(),
                                    .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                                    .command_bytes = 256 * sizeof(cui_draw_command_v2),
                                    .background_bytes = 65536};
    assert(cui_begin_frame_v2(m->ui, 400, 300, 0, &options));
    okay(submit(m));
    cui_draw_list_v2 output;
    assert(cui_end_frame_v2(m->ui, &output));
    unsigned count = 0;
    for (size_t i = 0; i < output.count; ++i)
        if (output.commands[i].kind == CUI_DRAW_V2_BACKGROUND) {
            const cui_background_draw_v2 *b = &output.commands[i].payload.background;
            assert(b->bounds.w == 14 && b->bounds.h == 14 &&
                   b->brush.kind == CUI_BACKGROUND_LINEAR &&
                   b->brush.model == CUI_BACKGROUND_SOURCE_038);
            for (unsigned stop = 0; stop < 2; ++stop) {
                const cui_slider_palette_stop *p = &m->style.style.palette.thumb.stops[stop];
                assert(b->brush.stops[stop].position == p->position &&
                       b->brush.stops[stop].color.r == p->color.r &&
                       b->brush.stops[stop].color.a == p->color.a);
            }
            ++count;
        }
    assert(count == 2);
    destroy(m); /* fabricated declaration: Core transport proof ONLY */
    m = create();
    okay(frame(m, 0, NULL));
    okay(frame(m, 0, NULL));
    m->accept = true;
    cui_focus(m->ui, part(CUI_SLIDER_MODEL_END_THUMB));
    key(m, CUI_KEY_LEFT);
    m->style.render_mode = CUI_STYLED_SLIDER_BACKGROUND_V2;
    m->style.support = support();
    unsigned char before[sizeof m->motion];
    memcpy(before, &m->motion, sizeof before);
    cui_styled_slider_model_result_v1 r = frame(m, 0, NULL); /* Host lied: V1 frame. */
    assert(r.render_status == CUI_STYLED_SLIDER_CORE_FAILURE && m->count == 3 &&
           m->value.end == .625);
    assert(!r.next_wake_seconds && !memcmp(before, &m->motion, sizeof before));
    assert(cui_last_error(m->ui) == CUI_ERROR_LIFECYCLE);
    destroy(m);
}
static void modes_and_modal(void) {
    machine *m = create();
    m->style.flags = CUI_STYLED_SLIDER_REDUCED_MOTION;
    for (unsigned axis = 0; axis < 2; ++axis)
        for (unsigned mode = 0; mode < 3; ++mode)
            for (unsigned size = 0; size < 3; ++size) {
                m->options.orientation = axis;
                m->options.flags = mode == 2 ? CUI_SLIDER_MODEL_REVERSE_FILL_BIT : 0;
                m->value.kind = mode == 0 ? CUI_SLIDER_MODEL_RANGE : CUI_SLIDER_MODEL_SINGLE;
                m->style.style.rem = size == 0 ? 12 : size == 1 ? 16 : 24;
                m->style.style.layout = CUI_SLIDER_STYLE_PARENT;
                m->length = size == 0 ? 80 : size == 1 ? 300 : 120;
                okay(frame(m, 0, NULL));
                cui_rect track = cui_bounds(m->ui, part(CUI_SLIDER_MODEL_TRACK));
                assert((axis ? track.h : track.w) == m->length);
                assert(cui_bounds(m->ui, part(CUI_SLIDER_MODEL_END_THUMB)).w == m->style.style.rem);
                if (mode != 0) {
                    assert(!(m->motion.initialized_mask & 1));
                    assert(cui_bounds(m->ui, decor(0, CUI_STYLED_SLIDER_SURFACE)).w == 0);
                }
                m->length += 37;
                okay(frame(m, 0, NULL));
                track = cui_bounds(m->ui, part(CUI_SLIDER_MODEL_TRACK));
                assert((axis ? track.h : track.w) == m->length);
            }
    unsigned char before[sizeof m->motion];
    memcpy(before, &m->motion, sizeof before);
    assert(cui_begin_frame(m->ui, 400, 300, 0));
    assert(cui_begin_focus_scope(m->ui, 888, true));
    cui_end_focus_scope(m->ui);
    cui_styled_slider_model_result_v1 r = submit(m);
    okay(r);
    assert(!r.next_wake_seconds && !memcmp(before, &m->motion, sizeof before));
    (void)cui_end_frame(m->ui);
    r = frame(m, 0, NULL);
    okay(r);
    assert(!r.next_wake_seconds && !memcmp(before, &m->motion, sizeof before));
    okay(frame(m, 0, NULL));
    m->motion.last_clock = DBL_MAX;
    m->motion.thumbs[1].updated_at = DBL_MAX;
    m->motion.thumbs[1].position = .5;
    m->motion.thumbs[1].velocity = 0;
    m->motion.thumbs[1].target = 1;
    m->style.flags = 0;
    over(m, 1);
    r = frame(m, 0, NULL);
    okay(r);
    assert(!r.next_wake_seconds && isfinite(m->motion.thumbs[1].position));
    destroy(m);
}
static void invalid_records(void) {
    for (unsigned variant = 0; variant < 14; ++variant) {
        machine *m = create();
        okay(frame(m, 0, NULL));
        okay(frame(m, 0, NULL));
        cui_focus(m->ui, part(CUI_SLIDER_MODEL_END_THUMB));
        key(m, CUI_KEY_LEFT);
        switch (variant) {
        case 0:
            m->style.version = 2;
            break;
        case 1:
            --m->style.struct_size;
            break;
        case 2:
            m->style.reserved[1] = 1;
            break;
        case 3:
            m->style.flags = 4;
            break;
        case 4:
            m->style.render_mode = 2;
            break;
        case 5:
            m->style.style.flags = CUI_SLIDER_STYLE_ACTIVE;
            break;
        case 6:
            m->style.spring_control.epsilon = 0;
            break;
        case 7:
            m->style.style.rem = NAN;
            break;
        case 8:
            m->motion.version = 2;
            break;
        case 9:
            m->motion.initialized_mask = 4;
            break;
        case 10:
            m->motion.thumbs[1].velocity = NAN;
            break;
        case 11:
            m->model.flags = 2;
            break;
        case 12:
            m->options.domain.maximum = -1;
            break;
        case 13:
            m->style.render_mode = CUI_STYLED_SLIDER_BACKGROUND_V2;
            break;
        }
        unsigned char model[sizeof m->model];
        memcpy(model, &m->model, sizeof model);
        unsigned char motion[sizeof m->motion];
        memcpy(motion, &m->motion, sizeof motion);
        assert(cui_begin_frame(m->ui, 400, 300, 0));
        cui_styled_slider_model_result_v1 r = submit(m);
        assert(r.render_status != CUI_STYLED_SLIDER_RENDER_OK && !r.next_wake_seconds &&
               !m->count && !r.base.begin_count && !r.base.request_count);
        assert(!cui_event_consumed(m->ui, 0) && !memcmp(model, &m->model, sizeof model) &&
               !memcmp(motion, &m->motion, sizeof motion));
        cui_draw_list list = cui_end_frame(m->ui);
        assert(!list.count && cui_last_error(m->ui) == CUI_OK);
        destroy(m);
    }
}
static void color_quantization(void) {
    machine *m = create();
    m->value.kind = CUI_SLIDER_MODEL_SINGLE;
    m->value.end = .75;
    for (unsigned value = 0; value < 256; ++value) {
        cui_color color = {(unsigned char)value, (unsigned char)(255 - value), 127, 255};
        assert(cui_slider_palette_color_from_srgba8(color, &m->style.style.palette.bar));
        cui_draw_list list;
        okay(frame(m, 0, &list));
        cui_rect fill = cui_bounds(m->ui, part(CUI_SLIDER_MODEL_FILL));
        unsigned found = 0;
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_RECT && c->bounds.x == fill.x && c->bounds.y == fill.y &&
                c->bounds.w == fill.w && c->bounds.h == fill.h) {
                assert(c->color.r == color.r && c->color.g == color.g && c->color.b == color.b &&
                       c->color.a == 255);
                ++found;
            }
        }
        assert(found == 1);
    }
    destroy(m);
}
static void palettes(void) {
    const cui_theme themes[4] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                                 cui_theme_terminal()};
    for (unsigned i = 0; i < 4; ++i) {
        machine *m = create();
        assert(cui_styled_slider_model_options_from_theme_v1(&themes[i], 16, 0, &m->style));
        assert((m->style.style.full_radius == 0) == (themes[i].radius == 0));
        okay(frame(m, 0, NULL));
        assert(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_SURFACE)).w == 14);
        destroy(m);
    }
    unsigned solid = 0, gradient = 0;
    for (size_t i = 0; i < cui_slider_palette_preset_count(); ++i) {
        machine *m = create();
        assert(cui_slider_palette_preset_get(i, &m->style.style.palette));
        cui_styled_slider_model_result_v1 r = frame(m, 0, NULL);
        if (m->style.style.palette.thumb.kind == CUI_SLIDER_BRUSH_LINEAR) {
            assert(r.render_status == CUI_STYLED_SLIDER_UNSUPPORTED && !m->model.owner &&
                   !m->motion.owner);
            ++gradient;
        } else {
            okay(r);
            assert(cui_bounds(m->ui, decor(1, CUI_STYLED_SLIDER_SURFACE)).w == 14);
            ++solid;
        }
        destroy(m);
    }
    assert(solid == 37 && gradient == 1);
    puts("PASS 4 builtin +37 named solid styles; 1 named gradient rejects V1 atomically");
}
int main(void) {
    printf("record sizes options=%zu presentation=%zu motion=%zu result=%zu\n",
           sizeof(cui_styled_slider_model_options_v1), sizeof(cui_styled_slider_model_state_v1),
           sizeof(cui_styled_slider_model_motion_v1), sizeof(cui_styled_slider_model_result_v1));
    palettes();
    invalid_records();
    color_quantization();
    transport();
    modes_and_modal();
    interaction();
    suppression();
    atomic();
    puts("PASS Styled interaction/acceptance/touch, motion/suppression/recovery, gradient atomic "
         "admission and constructors");
    return 0;
}
