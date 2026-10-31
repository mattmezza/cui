/* SPDX-License-Identifier: MIT */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "button_surface_test_helpers.h"
#include "cui/cui_button_styles.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const cui_color red = {210, 20, 30, 255}, blue = {10, 40, 220, 255};
typedef struct {
    cui_button_surface_patch patch[6];
    cui_button_surface_paint seen[6];
    unsigned phases[8], calls;
} log_t;
static void layer(void *user, const cui_button_style_info *info,
                  const cui_button_surface_paint *current, cui_button_surface_patch *out) {
    log_t *log = user;
    assert(info->id == 10 && log->calls < 8 && out->fields == 0);
    for (size_t i = 0; i < CUI_BOX_SHADOW_MAX; ++i)
        assert(out->value.shadows[i].layer.sigma == 0);
    log->phases[log->calls++] = (unsigned)info->phase;
    log->seen[info->phase] = *current;
    *out = log->patch[info->phase];
}
static cui_button_surface_styles styles(log_t *log) {
    return (cui_button_surface_styles){layer, layer, layer, layer, layer, layer, log};
}
static cui_button_surface_patch shadow(cui_color color) {
    cui_button_surface_patch patch = {.fields = CUI_BUTTON_SURFACE_SHADOWS,
                                      .value.shadow_count = 1};
    assert(
        cui_box_shadow_from_css(3, 3, 4, 1, color, CUI_BOX_SHADOW_DROP, &patch.value.shadows[0]));
    return patch;
}
static cui_draw_list frame(cui_context *ui, cui_button_options options,
                           cui_button_surface_styles style, cui_button_surface_status *status,
                           bool passive, bool legacy, cui_box parent, bool *activated) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 320, 160, .016f));
    cui_begin_box(ui, 900, parent);
    bool result = false;
    if (passive) {
        if (legacy)
            cui_button_paint(ui, 10, &theme, cui_string("Save"), options);
        else
            cui_button_surface_paint_only(ui, 10, &theme, cui_string("Save"), options, style,
                                          status);
    } else if (legacy)
        result = cui_button_ex(ui, 10, &theme, cui_string("Save"), options);
    else
        result = cui_button_surface_ex(ui, 10, &theme, cui_string("Save"), options, style, status);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (activated)
        *activated = result;
    return draw;
}
static cui_box parent(void) {
    return (cui_box){.width = 300, .height = 140, .padding = 16, .clip = true};
}
static void pointer(cui_context *ui, cui_event_type type) {
    cui_rect b = cui_bounds(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = b.x + 5, .y = b.y + 5}));
}
static const cui_draw_command *nth(cui_draw_list draw, cui_draw_type type, unsigned index) {
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type == type && index-- == 0)
            return &draw.commands[i];
    return NULL;
}
static unsigned shadow_count(cui_draw_list draw) {
    unsigned n = 0;
    for (size_t i = 0; i < draw.count; ++i)
        n += draw.commands[i].type == CUI_DRAW_DROP_SHADOW ||
             draw.commands[i].type == CUI_DRAW_INSET_SHADOW;
    return n;
}
static void noop(void *user, const cui_button_style_info *info,
                 const cui_button_surface_paint *current, cui_button_surface_patch *out) {
    (void)user;
    (void)info;
    (void)current;
    (void)out;
}
static void empty_equivalence(void) {
    for (unsigned noops = 0; noops < 2; ++noops)
        for (int variant = CUI_BUTTON_PRIMARY; variant <= CUI_BUTTON_CUSTOM; ++variant) {
            cui_context *a = create(256, 1024), *b = create(256, 1024);
            cui_software *r = cui_software_create(320, 160, NULL);
            assert(r);
            cui_set_reduced_motion(a, true);
            cui_set_reduced_motion(b, true);
            cui_button_surface_styles style = {0};
            if (noops)
                style = (cui_button_surface_styles){noop, noop, noop, noop, noop, noop, NULL};
            for (unsigned stage = 0; stage < 8; ++stage) {
                cui_button_options o = {.width = 140,
                                        .variant = (cui_button_variant)variant,
                                        .selected = stage == 3,
                                        .open = stage == 4,
                                        .loading = stage == 5,
                                        .disabled = stage == 6};
                if (stage == 1 || stage == 2) {
                    pointer(a, stage == 1 ? CUI_EVENT_POINTER_MOVE : CUI_EVENT_POINTER_DOWN);
                    pointer(b, stage == 1 ? CUI_EVENT_POINTER_MOVE : CUI_EVENT_POINTER_DOWN);
                }
                if (stage == 7) {
                    cui_focus(a, 10);
                    cui_focus(b, 10);
                    assert(cui_queue_event(
                        a, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
                    assert(cui_queue_event(
                        b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
                }
                bool ra, rb;
                cui_draw_list da = frame(a, o, style, NULL, false, true, parent(), &ra);
                cui_button_surface_status status = {63};
                cui_draw_list db = frame(b, o, style, &status, false, false, parent(), &rb);
                assert(ra == rb);
                assert(status.rejected_phases == 0 && commands(da) == commands(db));
                assert(pixels(r, da) == pixels(r, db));
                assert(!memcmp(&(cui_rect){cui_bounds(a, 10).x, cui_bounds(a, 10).y,
                                           cui_bounds(a, 10).w, cui_bounds(a, 10).h},
                               &(cui_rect){cui_bounds(b, 10).x, cui_bounds(b, 10).y,
                                           cui_bounds(b, 10).w, cui_bounds(b, 10).h},
                               sizeof(cui_rect)));
            }
            cui_destroy(a);
            cui_destroy(b);
            cui_software_destroy(r);
        }
}
static void phase_rules(void) {
    cui_context *ui = create(256, 1024);
    cui_button_options o = {.width = 140, .hide_focus_ring = true};
    cui_button_surface_status status;
    log_t log = {0};
    cui_button_surface_styles style = styles(&log);
    log.patch[0] = shadow(red);
    log.patch[1] = shadow(blue);
    log.patch[2] = shadow((cui_color){20, 190, 60, 255});
    log.patch[3] = shadow(blue);
    log.patch[4] = shadow(blue);
    cui_draw_list d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(log.calls == 1 && nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.r == red.r);
    pointer(ui, CUI_EVENT_POINTER_MOVE);
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(log.calls == 2 && log.phases[1] == CUI_BUTTON_STYLE_HOVER);
    assert(nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.b == blue.b);
    pointer(ui, CUI_EVENT_POINTER_DOWN);
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(log.calls == 2 && log.phases[1] == CUI_BUTTON_STYLE_ACTIVE);
    assert(nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.g == 190);
    o.selected = o.open = o.disabled = o.loading = true;
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(log.calls == 3 && log.phases[1] == CUI_BUTTON_STYLE_SELECTED &&
           log.phases[2] == CUI_BUTTON_STYLE_DISABLED);
    assert(log.seen[CUI_BUTTON_STYLE_DISABLED].shadow_count == 0);
    assert(shadow_count(d) == 1 && nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.r == red.r);
    /* Captured explicit clear wins selected and disabled lists on both replays. */
    log.patch[0].value.shadow_count = 0;
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(log.calls == 3 && shadow_count(d) == 0 && status.rejected_phases == 0);
    /* Missing instance list allows a semantic list to survive; disabled clears it. */
    log.patch[0].fields = 0;
    log.patch[4].fields = 0;
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(shadow_count(d) == 0);
    o.disabled = false;
    o.loading = false;
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(shadow_count(d) == 1 && nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.b == blue.b);
    o.selected = o.open = false;
    o.loading = true;
    log.patch[0] = shadow(red);
    log.patch[0].fields |= CUI_BUTTON_PAINT_OPACITY;
    log.patch[0].value.paint.root.opacity = .5f;
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(log.calls == 1 && nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.a == 102);
    /* Keyboard modality enables the existing focused callback; it runs last. */
    o.loading = false;
    o.hide_focus_ring = false;
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    log.patch[5] = shadow(blue);
    log.calls = 0;
    d = frame(ui, o, style, &status, false, false, parent(), NULL);
    assert(log.phases[log.calls - 1] == CUI_BUTTON_STYLE_FOCUS);
    assert(log.seen[5].paint.root.border_width == 2);
    assert(nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.b == blue.b);
    cui_destroy(ui);
}
static void transactional_validation(void) {
    cui_context *ui = create(256, 1024);
    cui_button_options o = {.width = 140, .selected = true, .hide_focus_ring = true};
    cui_button_surface_status status;
    log_t log = {0};
    log.patch[3] = shadow(blue);
    for (unsigned invalid = 0; invalid < 13; ++invalid) {
        log.patch[0] = shadow(red);
        log.patch[0].fields |= CUI_BUTTON_PAINT_BACKGROUND;
        log.patch[0].value.paint.root.background = red;
        cui_drop_shadow *v = &log.patch[0].value.shadows[0].layer;
        switch (invalid) {
        case 0:
            log.patch[0].fields |= 1u << 30;
            break;
        case 1:
            log.patch[0].value.shadow_count = 9;
            break;
        case 2:
            log.patch[0].value.shadows[0].kind = (cui_box_shadow_kind)9;
            break;
        case 3:
            v->sigma = NAN;
            break;
        case 4:
            v->sigma = -1;
            break;
        case 5:
            v->sigma = 257;
            break;
        case 6:
            v->spread = INFINITY;
            break;
        case 7:
            v->offset_x = -4097;
            break;
        case 8:
            v->offset_y = 4097;
            break;
        case 9:
            v->spread = 4097;
            break;
        case 10:
            log.patch[0].fields |= CUI_BUTTON_PAINT_OPACITY;
            log.patch[0].value.paint.root.opacity = 2;
            break;
        case 11:
            log.patch[0].fields |= CUI_BUTTON_PAINT_RADIUS;
            log.patch[0].value.paint.root.radius = 16385;
            break;
        default:
            log.patch[0].fields |= CUI_BUTTON_PAINT_CORNERS;
            log.patch[0].value.paint.root.corner_radii.top_left = NAN;
            break;
        }
        log.calls = 0;
        cui_draw_list d = frame(ui, o, styles(&log), &status, false, false, parent(), NULL);
        assert(status.rejected_phases == (1u << CUI_BUTTON_STYLE_INSTANCE));
        assert(log.calls == 2 && shadow_count(d) == 1);
        assert(nth(d, CUI_DRAW_DROP_SHADOW, 0)->shadow.layer.color.b == blue.b);
        assert(log.seen[3].paint.root.background.r != red.r); /* scalar rejected too */
    }
    /* Options-derived radius can be legal in legacy API but outside shadow bounds. */
    o.rounding = CUI_BUTTON_ROUNDED_CUSTOM;
    o.radius = 20000;
    log.patch[0] = shadow(red);
    log.patch[3].fields = 0;
    log.calls = 0;
    cui_draw_list d = frame(ui, o, styles(&log), &status, false, false, parent(), NULL);
    assert(status.rejected_phases == 1 && shadow_count(d) == 0);
    /* Explicit bounded replacement fixes the effective shape transactionally. */
    log.patch[0].fields |= CUI_BUTTON_PAINT_RADIUS | CUI_BUTTON_PAINT_CORNERS;
    log.patch[0].value.paint.root.radius = 0;
    log.calls = 0;
    d = frame(ui, o, styles(&log), &status, false, false, parent(), NULL);
    assert(status.rejected_phases == 0 && shadow_count(d) == 1);
    /* Inactive fields and unused entries are never validated/copied as paint. */
    log.patch[0] = shadow(red);
    log.patch[0].value.shadows[7].layer.sigma = NAN;
    log.patch[0].value.paint.root.width = NAN;
    log.patch[0].value.paint.root.opacity = NAN;
    o.radius = 4;
    log.calls = 0;
    d = frame(ui, o, styles(&log), &status, false, false, parent(), NULL);
    assert(status.rejected_phases == 0 && shadow_count(d) == 1);
    cui_destroy(ui);
}
static unsigned slots;
static log_t *mutable_log;
static void content(cui_context *ui, void *user) {
    (void)user;
    ++slots;
    if (mutable_log)
        memset(mutable_log->patch, 0, sizeof mutable_log->patch);
    unsigned char overwrite[2048];
    memset(overwrite, 0xA5, sizeof overwrite);
    cui_begin_box(ui, 33, (cui_box){.width = 20, .height = 10, .background = {90, 10, 200, 255}});
    cui_end_box(ui);
    assert(overwrite[100] == 0xA5);
}
static void geometry_pixels_and_lifetimes(void) {
    cui_context *a = create(256, 1024), *b = create(256, 1024);
    cui_software *r = cui_software_create(320, 160, NULL);
    assert(r);
    log_t log = {0};
    cui_button_options o = {.width = 140,
                            .content = content,
                            .hide_focus_ring = true,
                            .custom_corners = true,
                            .corner_radii = {3, 7, 11, 5}};
    slots = 0;
    for (unsigned moved = 0; moved < 2; ++moved) {
        cui_box p = parent();
        p.padding = moved ? 25 : 16;
        p.height = moved ? 48 : 40;
        p.custom_opacity = true;
        p.opacity = .75f;
        o.width = moved ? 180 : 140;
        cui_draw_list baseline =
            frame(a, o, (cui_button_surface_styles){0}, NULL, false, true, p, NULL);
        uint64_t baseline_pixels = pixels(r, baseline);
        log.patch[0] = shadow(red);
        log.patch[0].value.shadow_count = CUI_BOX_SHADOW_MAX;
        for (size_t i = 1; i < CUI_BOX_SHADOW_MAX; ++i) {
            log.patch[0].value.shadows[i] = log.patch[0].value.shadows[0];
            log.patch[0].value.shadows[i].kind = i % 2 ? CUI_BOX_SHADOW_INSET : CUI_BOX_SHADOW_DROP;
            log.patch[0].value.shadows[i].layer.spread = (float)i;
        }
        log.calls = 0;
        mutable_log = &log;
        cui_draw_list d = frame(b, o, styles(&log), NULL, false, false, p, NULL);
        mutable_log = NULL;
        assert(shadow_count(d) == 8 && pixels(r, d) != baseline_pixels);
        const unsigned char *rgba = cui_software_pixels(r, NULL, NULL);
        size_t outside = (80 * 320 + 30) * 4;
        assert(rgba[outside] == 255 && rgba[outside + 1] == 255 && rgba[outside + 2] == 255);

        cui_rect bounds = cui_bounds(b, 10), expected = cui_bounds(a, 10);
        assert(!memcmp(&bounds, &expected, sizeof bounds));
        unsigned drops = 0, insets = 0;
        bool background = false;
        for (size_t i = 0; i < d.count; ++i) {
            const cui_draw_command *c = &d.commands[i];
            if (c->type == CUI_DRAW_DROP_SHADOW || c->type == CUI_DRAW_INSET_SHADOW) {
                assert(!memcmp(&c->shadow.caster, &bounds, sizeof bounds));
                assert(c->shadow.radii.top_left == 3 && c->shadow.radii.top_right == 7);
                assert(c->shadow.layer.color.a == 191);
                if (c->type == CUI_DRAW_DROP_SHADOW) {
                    assert(!background);
                    ++drops;
                } else {
                    assert(background);
                    ++insets;
                }
            } else if (c->type == CUI_DRAW_RECT && c->bounds.w == bounds.w)
                background = true;
        }
        assert(drops == 4 && insets == 4);
        /* Independent ordinary-only renderer oracle retains real clipping and opacity. */
        cui_draw_command ordinary[512];
        cui_draw_list filtered = {.commands = ordinary, .width = d.width, .height = d.height};
        for (size_t i = 0; i < d.count; ++i)
            if (d.commands[i].type != CUI_DRAW_DROP_SHADOW &&
                d.commands[i].type != CUI_DRAW_INSET_SHADOW)
                ordinary[filtered.count++] = d.commands[i];
        assert(pixels(r, filtered) == baseline_pixels);
        /* Public element assignment replaces complete facade list before end_frame. */
        assert(cui_begin_frame(b, 320, 160, .016f));
        cui_theme theme = cui_theme_light();
        log.patch[0] = shadow(red);
        log.calls = 0;
        (void)cui_button_surface_ex(b, 10, &theme, cui_string("Save"), o, styles(&log), NULL);
        assert(cui_element_set_shadow_preset(b, 10, CUI_SHADOW_NONE, (cui_corner_radii){0}));
        d = cui_end_frame(b);
        assert(cui_last_error(b) == CUI_OK && shadow_count(d) == 0);
    }
    assert(slots == 6);
    /* Independently retain text command semantics around the fontless pixel oracle. */
    o.content = NULL;
    cui_draw_list ordinary_text =
        frame(a, o, (cui_button_surface_styles){0}, NULL, false, true, parent(), NULL);
    log.patch[0] = shadow(red);
    log.calls = 0;
    cui_draw_list shadow_text = frame(b, o, styles(&log), NULL, false, false, parent(), NULL);
    cui_draw_command text_a[8], text_b[8];
    cui_draw_list ta = {.commands = text_a}, tb = {.commands = text_b};
    for (size_t i = 0; i < ordinary_text.count; ++i)
        if (ordinary_text.commands[i].type == CUI_DRAW_TEXT) {
            assert(ta.count < 8);
            text_a[ta.count++] = ordinary_text.commands[i];
        }
    for (size_t i = 0; i < shadow_text.count; ++i)
        if (shadow_text.commands[i].type == CUI_DRAW_TEXT) {
            assert(tb.count < 8);
            text_b[tb.count++] = shadow_text.commands[i];
        }
    assert(ta.count != 0 && commands(ta) == commands(tb));
    log.patch[0] = shadow(red);
    log.calls = 0;
    cui_box p = parent();
    p.layout_only = true;
    cui_draw_list d = frame(b, o, styles(&log), NULL, false, false, p, NULL);
    size_t n;
    cui_semantics(b, &n);
    assert(n == 0 && shadow_count(d) == 0);
    /* No retained decoration on the following ordinary frame. */
    d = frame(b, o, (cui_button_surface_styles){0}, NULL, false, false, parent(), NULL);
    assert(shadow_count(d) == 0);
    cui_software_destroy(r);
    cui_destroy(a);
    cui_destroy(b);
}
typedef struct {
    unsigned order[16], count;
    cui_button_surface_styles descriptor;
    log_t paint;
} owner_t;
static void click(void *user) {
    owner_t *owner = user;
    assert(owner->count < 16);
    owner->order[owner->count++] = 1;
    owner->descriptor = (cui_button_surface_styles){0};
}
static void typed(void *user, const cui_button_click_event *event) {
    owner_t *owner = user;
    assert(event->target == 10 && owner->count < 16);
    owner->order[owner->count++] = 2;
}
static void input_and_passive(void) {
    cui_context *ui = create(256, 1024);
    owner_t owner = {0};
    owner.paint.patch[0] = shadow(red);
    owner.descriptor = styles(&owner.paint);
    cui_button_options o = {
        .width = 140, .on_click = click, .on_click_event = typed, .action_user = &owner};
    cui_button_surface_status status = {63};
    frame(ui, o, owner.descriptor, &status, false, false, parent(), NULL);
    for (unsigned i = 0; i < 2; ++i)
        assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    owner.paint.calls = 0;
    bool activated;
    cui_draw_list d = frame(ui, o, owner.descriptor, &status, false, false, parent(), &activated);
    assert(activated && status.rejected_phases == 0 && shadow_count(d) == 1);
    assert(owner.count == 4 && owner.paint.calls == 1);
    for (unsigned i = 0; i < 4; ++i)
        assert(owner.order[i] == (i % 2 ? 2u : 1u));
    d = frame(ui, o, owner.descriptor, &status, false, false, parent(), NULL);
    assert(shadow_count(d) == 0); /* original descriptor mutation affects only next call */
    owner.descriptor = styles(&owner.paint);
    owner.paint.calls = 0;
    pointer(ui, CUI_EVENT_POINTER_DOWN);
    frame(ui, o, owner.descriptor, &status, false, false, parent(), &activated);
    assert(!activated && owner.count == 4);
    owner.paint.calls = 0;
    pointer(ui, CUI_EVENT_POINTER_UP);
    d = frame(ui, o, owner.descriptor, &status, false, false, parent(), &activated);
    assert(activated && owner.count == 6 && shadow_count(d) == 1);
    owner.descriptor = styles(&owner.paint);
    owner.paint.calls = 0;
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
    d = frame(ui, o, owner.descriptor, &status, false, false, parent(), &activated);
    assert(activated && owner.count == 8 && shadow_count(d) == 1);
    for (unsigned i = 0; i < owner.count; ++i)
        assert(owner.order[i] == (i % 2 ? 2u : 1u));
    /* Authoring rejection reports independently; accepted actions still run once. */
    owner.descriptor = styles(&owner.paint);
    owner.paint.patch[0].value.shadow_count = 9;
    owner.paint.calls = 0;
    assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    d = frame(ui, o, owner.descriptor, &status, false, false, parent(), &activated);
    assert(activated && owner.count == 10 && status.rejected_phases == 1 && shadow_count(d) == 0);
    owner.paint.patch[0] = shadow(red);
    owner.descriptor = styles(&owner.paint);
    owner.paint.calls = 0;
    assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    d = frame(ui, o, owner.descriptor, &status, true, false, parent(), &activated);
    size_t n;
    cui_semantics(ui, &n);
    assert(!activated && !n && owner.count == 10 && shadow_count(d) == 1);
    assert(status.rejected_phases == 0);
    /* Rejected initial submission still clears stale caller diagnostics. */
    status.rejected_phases = 63;
    assert(
        !cui_button_surface_ex(NULL, 10, NULL, cui_string("Save"), o, owner.descriptor, &status));
    assert(status.rejected_phases == 0);
    cui_destroy(ui);
}
static void resource_failure(void) {
    cui_context *ui = create(64, 4);
    cui_theme theme = cui_theme_light();
    log_t log = {0};
    log.patch[0] = shadow(red);
    log.patch[0].value.shadow_count = CUI_BOX_SHADOW_MAX;
    for (size_t i = 1; i < CUI_BOX_SHADOW_MAX; ++i)
        log.patch[0].value.shadows[i] = log.patch[0].value.shadows[0];
    cui_button_surface_status status = {63};
    assert(cui_begin_frame(ui, 320, 160, .016f));
    (void)cui_button_surface_ex(ui, 10, &theme, cui_string("Save"),
                                (cui_button_options){.width = 140}, styles(&log), &status);
    (void)cui_end_frame(ui);
    assert(status.rejected_phases == 0 && cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
}
int main(void) {
    empty_equivalence();
    phase_rules();
    transactional_validation();
    geometry_pixels_and_lifetimes();
    input_and_passive();
    resource_failure();
    puts("Button surface: bounded owned shadows, phase/reset/input/software checks pass");
    return 0;
}
