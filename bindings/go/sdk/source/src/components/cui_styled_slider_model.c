/* SPDX-License-Identifier: MIT */
#include "cui/cui_styled_slider_model.h"
#include "cui_slider_model_presentation_internal.h"
#include "cui_spring_internal.h"
#include <math.h>
#include <string.h>
static bool cui_styled_slider_spring_valid(cui_spring_settings p) {
    return isfinite(p.response) && p.response >= 0 && isfinite(p.damping) && p.damping >= 0 &&
           isfinite(p.epsilon) && p.epsilon > 0;
}
bool cui_styled_slider_model_options_from_theme_v1(const cui_theme *theme, float rem,
                                                   uint32_t orientation,
                                                   cui_styled_slider_model_options_v1 *out) {
    if (!theme || !out || !isfinite(theme->radius) || theme->radius < 0 || !isfinite(rem) ||
        rem <= 0 || !isfinite(rem * 1.5f) || orientation > CUI_SLIDER_MODEL_VERTICAL ||
        !cui_styled_slider_spring_valid(theme->spring_control))
        return false;
    cui_styled_slider_model_options_v1 o;
    memset(&o, 0, sizeof o);
    o.version = 1;
    o.struct_size = sizeof o;
    o.style.version = 1;
    o.style.struct_size = sizeof o.style;
    o.style.layout =
        orientation == CUI_SLIDER_MODEL_VERTICAL ? CUI_SLIDER_STYLE_FIXED : CUI_SLIDER_STYLE_PARENT;
    o.style.length = orientation == CUI_SLIDER_MODEL_VERTICAL ? 120 : 0;
    o.style.rem = rem;
    o.style.full_radius = theme->radius == 0 ? 0 : cui_radius(CUI_RADIUS_FULL, 1);
    o.spring_control = theme->spring_control;
    if (!cui_slider_palette_from_theme(theme, &o.style.palette))
        return false;
    *out = o;
    return true;
}
bool cui_styled_slider_model_state_init_v1(uint32_t bytes, cui_styled_slider_model_state_v1 *out) {
    if (!out || bytes != sizeof(*out))
        return false;
    memset(out, 0, sizeof(*out));
    out->version = 1;
    out->struct_size = sizeof(*out);
    return true;
}
cui_id cui_styled_slider_model_part_id_v1(cui_id owner, uint32_t thumb, uint32_t part) {
    static const cui_id salts[3] = {UINT64_C(0x791cf239d64a805b), UINT64_C(0x829df10aeb65074c),
                                    UINT64_C(0x93be081bf473165d)};
    if (!owner || thumb > CUI_SLIDER_MODEL_END || part > CUI_STYLED_SLIDER_FOCUS_RING)
        return 0;
    cui_id parent = cui_base_slider_model_part_id_v1(owner, CUI_SLIDER_MODEL_START_THUMB + thumb);
    cui_id id = parent ^ salts[part];
    return id ? id : ~salts[part];
}
static bool cui_styled_slider_state_valid(const cui_styled_slider_model_state_v1 *s) {
    if (!s || s->version != 1 || s->struct_size != sizeof(*s) ||
        (s->initialized_mask & ~UINT32_C(3)) || !isfinite(s->last_clock) || s->last_clock < 0 ||
        !isfinite(s->next_wake_seconds) || s->next_wake_seconds < 0)
        return false;
    if (!s->owner && (s->initialized_mask || s->last_admitted_frame || s->next_wake_seconds))
        return false;
    if (s->owner && (!s->last_admitted_frame || s->previous_kind > CUI_SLIDER_MODEL_RANGE ||
                     (s->previous_kind == CUI_SLIDER_MODEL_SINGLE && (s->initialized_mask & 1))))
        return false;
    for (unsigned i = 0; i < 2; ++i)
        if (s->initialized_mask & (1u << i)) {
            const cui_styled_slider_model_motion_v1 *m = &s->thumbs[i];
            if (!isfinite(m->position) || !isfinite(m->velocity) ||
                (m->target != 0 && m->target != 1) || !isfinite(m->updated_at) || m->updated_at < 0)
                return false;
        }
    return true;
}
static unsigned char cui_styled_slider_byte(double value) {
    return (unsigned char)floor(fmin(1, fmax(0, value)) * 255 + .5);
}
static unsigned char cui_styled_slider_channel(float value) {
    double v = value;
    return cui_styled_slider_byte(v <= .0031308 ? 12.92 * v : 1.055 * pow(v, 1 / 2.4) - .055);
}
static cui_color cui_styled_slider_color(cui_slider_palette_color c) {
    return (cui_color){cui_styled_slider_channel(c.r), cui_styled_slider_channel(c.g),
                       cui_styled_slider_channel(c.b), cui_styled_slider_byte(c.a)};
}
typedef struct {
    float width, radius;
    cui_color color;
} cui_styled_slider_ring_payload;
static bool cui_styled_slider_ring_paint(const cui_prepaint_info *info, const void *payload,
                                         cui_prepaint_writer *writer) {
    const cui_styled_slider_ring_payload *p = payload;
    return cui_prepaint_emit(writer,
                             &(cui_draw_command){.type = CUI_DRAW_BORDER,
                                                 .bounds = info->bounds,
                                                 .color = p->color,
                                                 .stroke = p->width,
                                                 .radius = p->radius},
                             false);
}
typedef struct {
    cui_id owner;
    cui_styled_slider_model_options_v1 options;
    cui_styled_slider_model_state_v1 staged;
    const cui_slider_model_state_v1 *model;
    bool eligible, admitted;
    bool focus[2];
    float progress[2];
} cui_styled_slider_submission;
static void cui_styled_slider_ring(cui_context *ui, cui_id id, float rem, float inset, float stroke,
                                   float radius, cui_color color) {
    if (stroke <= 0)
        return;
    cui_begin_absolute_box(ui, id, (cui_box){0},
                           (cui_rect){-inset, -inset, rem + 2 * inset, rem + 2 * inset});
    cui_styled_slider_ring_payload payload = {stroke, radius, color};
    (void)cui_set_prepaint(ui, (cui_prepaint_options){.callback = cui_styled_slider_ring_paint,
                                                      .payload = &payload,
                                                      .payload_bytes = sizeof payload,
                                                      .command_budget = 3});
    cui_end_box(ui);
}
static void cui_styled_slider_decorate(cui_context *ui, cui_id outer, uint32_t thumb,
                                       const cui_slider_style_plan_v1 *p, void *user) {
    (void)outer;
    cui_styled_slider_submission *s = user;
    bool square = p->full_radius == 0;
    float progress = s->progress[thumb], width = 3 * progress;
    cui_slider_palette_color ring = p->ring;
    ring.a = .5f * progress;
    if (cui_frame_paint_enabled(ui)) {
        cui_styled_slider_ring(
            ui, cui_styled_slider_model_part_id_v1(s->owner, thumb, CUI_STYLED_SLIDER_POINTER_RING),
            p->thumb_extent, width, width, square ? 0 : p->thumb_extent * .5f + width,
            cui_styled_slider_color(ring));
        if (s->focus[thumb])
            cui_styled_slider_ring(
                ui,
                cui_styled_slider_model_part_id_v1(s->owner, thumb, CUI_STYLED_SLIDER_FOCUS_RING),
                p->thumb_extent, 4, 2, square ? 0 : p->thumb_extent * .5f + 4,
                cui_styled_slider_color(p->ring));
    }
    if (p->surface_extent <= 0)
        return;
    cui_color color = {0};
    if (s->options.render_mode == CUI_STYLED_SLIDER_SOLID_V1 &&
        p->thumb.kind == CUI_BACKGROUND_SOLID)
        color = cui_styled_slider_color((cui_slider_palette_color){
            p->thumb.solid.r, p->thumb.solid.g, p->thumb.solid.b, p->thumb.solid.a});
    cui_begin_absolute_box(
        ui, cui_styled_slider_model_part_id_v1(s->owner, thumb, CUI_STYLED_SLIDER_SURFACE),
        (cui_box){.background = color, .radius = p->full_radius},
        (cui_rect){p->padding, p->padding, p->surface_extent, p->surface_extent});
    if (s->options.render_mode == CUI_STYLED_SLIDER_BACKGROUND_V2 && cui_frame_paint_enabled(ui))
        (void)cui_set_background(ui, &p->thumb);
    cui_end_box(ui);
}
static void cui_styled_slider_motion(cui_context *ui, cui_styled_slider_submission *s,
                                     const cui_slider_model_options_v1 *o, uint32_t kind) {
    s->admitted = s->eligible && cui_frame_input_snapshot_ready(ui) && cui_last_error(ui) == CUI_OK;
    if (!s->admitted)
        return;
    uint64_t frame = cui_frame_sequence(ui);
    double now = cui_frame_time(ui);
    cui_styled_slider_model_state_v1 *state = &s->staged;
    bool adjacent =
        state->last_admitted_frame &&
        frame == (state->last_admitted_frame == UINT64_MAX ? 1 : state->last_admitted_frame + 1);
    if (state->owner != s->owner || !adjacent) {
        (void)cui_styled_slider_model_state_init_v1(sizeof(*state), state);
        state->last_clock = now;
    }
    state->owner = s->owner;
    state->last_admitted_frame = frame;
    state->previous_kind = kind;
    state->last_clock = fmax(state->last_clock, now);
    state->next_wake_seconds = 0;
    if (kind == CUI_SLIDER_MODEL_SINGLE) {
        memset(&state->thumbs[0], 0, sizeof state->thumbs[0]);
        state->initialized_mask &= ~UINT32_C(1);
    }
    bool disabled = (o->flags & CUI_SLIDER_MODEL_DISABLED_BIT) != 0;
    if (disabled) {
        memset(state->thumbs, 0, sizeof state->thumbs);
        state->initialized_mask = 0;
        return;
    }
    bool reduced =
        (s->options.flags & CUI_STYLED_SLIDER_REDUCED_MOTION) != 0 || cui_reduced_motion(ui);
    for (unsigned i = kind == CUI_SLIDER_MODEL_SINGLE ? 1u : 0u; i < 2; ++i) {
        cui_id id = cui_base_slider_model_part_id_v1(s->owner, CUI_SLIDER_MODEL_START_THUMB + i);
        cui_interaction interaction = cui_interaction_state(ui, id);
        bool pressed = (s->model->flags & CUI_SLIDER_MODEL_DRAGGING) && s->model->active_thumb == i;
        double target = interaction.hovered || pressed ? 1 : 0;
        cui_styled_slider_model_motion_v1 *m = &state->thumbs[i];
        if (!(state->initialized_mask & (1u << i)))
            *m = (cui_styled_slider_model_motion_v1){.updated_at = state->last_clock};
        cui_spring_sample_internal sample = {m->position,   m->velocity, m->target,
                                             m->updated_at, 0,           true};
        double clock = fmax(state->last_clock, m->updated_at);
        cui_spring_step_internal(&sample, target, s->options.spring_control, clock, reduced);
        if (sample.next_wake_seconds && sample.next_wake_seconds <= clock)
            sample.next_wake_seconds = nextafter(clock, INFINITY);
        if (!isfinite(sample.next_wake_seconds)) {
            sample.position = sample.target = target;
            sample.velocity = 0;
            sample.next_wake_seconds = 0;
        }
        *m = (cui_styled_slider_model_motion_v1){sample.position, sample.velocity, sample.target,
                                                 sample.updated_at};
        state->initialized_mask |= 1u << i;
        if (sample.next_wake_seconds &&
            (!state->next_wake_seconds || sample.next_wake_seconds < state->next_wake_seconds))
            state->next_wake_seconds = sample.next_wake_seconds;
        s->progress[i] = (float)fmin(1, fmax(0, m->position));
        s->focus[i] = !(s->options.flags & CUI_STYLED_SLIDER_HIDE_FOCUS) && interaction.focused &&
                      interaction.focus_visible;
    }
}
static void cui_styled_slider_compose(cui_context *ui, const cui_id ids[5],
                                      cui_slider_model_value_v1 paint,
                                      const cui_slider_model_options_v1 *o,
                                      const cui_base_slider_model_layout_v1 *l, void *user) {
    cui_styled_slider_submission *s = user;
    cui_styled_slider_motion(ui, s, o, paint.kind);
    cui_slider_style_options_v1 style = s->options.style;
    style.flags = (o->flags & CUI_SLIDER_MODEL_DISABLED_BIT) ? CUI_SLIDER_STYLE_DISABLED : 0;
    if (s->admitted && (s->model->flags & CUI_SLIDER_MODEL_DRAGGING))
        style.flags |= CUI_SLIDER_STYLE_ACTIVE;
    cui_slider_source_presentation_internal source = {.decorate = cui_styled_slider_decorate,
                                                      .user = s};
    if (!cui_slider_style_resolve_v1(&style, &source.plan))
        return; /* Preflight already checked all inputs; only derived known flags change. */
    source.track = (cui_box){.background = cui_styled_slider_color(source.plan.track),
                             .custom_corners = true,
                             .corner_radii = source.plan.track_corners};
    source.fill = (cui_box){.background = cui_styled_slider_color(source.plan.fill),
                            .radius = source.plan.full_radius};
    source.outer = (cui_box){.background = cui_styled_slider_color(source.plan.outer),
                             .radius = source.plan.full_radius};
    cui_slider_source_compose_internal(ui, ids, paint, o, l, &source);
}
static bool cui_styled_slider_support(cui_draw_v2_support s, size_t backgrounds) {
    return s.size == sizeof s && s.abi_version == CUI_DRAW_ABI_V2 &&
           s.background_output == CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1 &&
           s.features == CUI_DRAW_V2_FEATURE_BACKGROUNDS && s.max_commands && s.max_backgrounds &&
           s.max_backgrounds >= backgrounds && s.max_pixels && s.max_pixels_per_background &&
           s.cpu_bytes && s.texture_bytes && s.transfer_bytes && s.metadata_bytes;
}
cui_styled_slider_model_result_v1 cui_styled_slider_model_events_v1(
    cui_context *ui, cui_id owner, cui_slider_model_value_v1 confirmed,
    cui_slider_model_state_v1 *model, const cui_slider_model_options_v1 *options,
    cui_styled_slider_model_state_v1 *presentation, const cui_styled_slider_model_options_v1 *style,
    cui_slider_model_handler_v1 handler) {
    cui_styled_slider_model_result_v1 result = {
        .version = 1,
        .struct_size = sizeof result,
        .render_status = CUI_STYLED_SLIDER_INVALID_ARGUMENT,
        .base = {.version = 1,
                 .struct_size = sizeof(cui_slider_model_result_v1),
                 .status = CUI_SLIDER_MODEL_INVALID_ARGUMENT}};
    if (!ui || !owner || !options || !model || !style || !presentation || style->version != 1 ||
        style->struct_size != sizeof(*style) || style->reserved[0] || style->reserved[1] ||
        (style->flags & ~UINT32_C(3)) || style->render_mode > CUI_STYLED_SLIDER_BACKGROUND_V2 ||
        style->style.flags || !cui_styled_slider_spring_valid(style->spring_control))
        return result;
    cui_slider_style_plan_v1 plan;
    if (!cui_slider_style_resolve_v1(&style->style, &plan))
        return result;
    if (!cui_styled_slider_state_valid(presentation)) {
        result.render_status = CUI_STYLED_SLIDER_INVALID_STATE;
        return result;
    }
    cui_base_slider_model_layout_v1 l = {
        .version = 1,
        .struct_size = sizeof l,
        .length = plan.layout == CUI_SLIDER_STYLE_FIXED ? plan.length : 1,
        .cross_extent = plan.cross_extent,
        .track_extent = plan.track_extent,
        .thumb_extent = plan.thumb_extent};
    cui_id ids[11];
    result.base = cui_base_slider_model_validate_internal(ui, owner, confirmed, model, options, &l,
                                                          handler, ids);
    if (result.base.status != CUI_SLIDER_MODEL_OK) {
        result.render_status = CUI_STYLED_SLIDER_BASE_REJECTED;
        return result;
    }
    unsigned count = 5;
    for (unsigned thumb = confirmed.kind == CUI_SLIDER_MODEL_SINGLE ? 1u : 0u; thumb < 2; ++thumb)
        for (unsigned part = 0; part < 3; ++part) {
            cui_id id = cui_styled_slider_model_part_id_v1(owner, thumb, part);
            for (unsigned i = 0; i < count; ++i)
                if (!id || ids[i] == id) {
                    result.base =
                        (cui_slider_model_result_v1){.version = 1,
                                                     .struct_size = sizeof result.base,
                                                     .status = CUI_SLIDER_MODEL_INVALID_ARGUMENT};
                    return result;
                }
            ids[count++] = id;
        }
    bool visible = cui_frame_paint_enabled(ui) &&
                   !(options->flags & CUI_SLIDER_MODEL_DISABLED_BIT) && plan.surface_extent > 0;
    if ((style->render_mode == CUI_STYLED_SLIDER_SOLID_V1 && visible &&
         plan.thumb.kind == CUI_BACKGROUND_LINEAR) ||
        (style->render_mode == CUI_STYLED_SLIDER_BACKGROUND_V2 &&
         !cui_styled_slider_support(style->support, !visible ? 0
                                                    : confirmed.kind == CUI_SLIDER_MODEL_SINGLE
                                                        ? 1
                                                        : 2))) {
        result.render_status = CUI_STYLED_SLIDER_UNSUPPORTED;
        result.base = (cui_slider_model_result_v1){.version = 1,
                                                   .struct_size = sizeof result.base,
                                                   .status = CUI_SLIDER_MODEL_INVALID_ARGUMENT};
        return result;
    }
    cui_styled_slider_submission submission = {.owner = owner,
                                               .options = *style,
                                               .staged = *presentation,
                                               .model = model,
                                               .eligible = cui_frame_sequence(ui) != 0 &&
                                                           cui_frame_input_snapshot_ready(ui) &&
                                                           cui_pointer_scope_allowed(ui, 0)};
    cui_slider_model_presentation_internal policy = {
        cui_slider_source_root_internal(&plan, options->orientation), cui_styled_slider_compose,
        &submission};
    result.base = cui_base_slider_model_present_internal(ui, owner, confirmed, model, options, &l,
                                                         handler, &policy);
    if (result.base.status != CUI_SLIDER_MODEL_OK) {
        result.render_status = result.base.status == CUI_SLIDER_MODEL_CORE_FAILURE
                                   ? CUI_STYLED_SLIDER_CORE_FAILURE
                                   : CUI_STYLED_SLIDER_BASE_REJECTED;
        return result;
    }
    result.render_status = CUI_STYLED_SLIDER_RENDER_OK;
    if (submission.admitted && cui_frame_input_snapshot_ready(ui) && cui_last_error(ui) == CUI_OK) {
        *presentation = submission.staged;
        result.next_wake_seconds = presentation->next_wake_seconds;
    }
    return result;
}
