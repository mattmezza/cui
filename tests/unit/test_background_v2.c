/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
static bool fail_allocation;
static size_t fail_after;
static void *v2_malloc(size_t n) {
    if (fail_allocation && fail_after-- == 0)
        return NULL;
    return malloc(n);
}
static void *v2_calloc(size_t n, size_t s) {
    if (fail_allocation && fail_after-- == 0)
        return NULL;
    return calloc(n, s);
}
#define malloc v2_malloc
#define calloc v2_calloc
#include "../../src/core/cui.c"
#undef malloc
#undef calloc
/* Public synthetic consumer contract, ONLY for Core tests, not a real adapter. */
static cui_frame_v2_options options(void) {
    return (cui_frame_v2_options){
        .size = sizeof(cui_frame_v2_options),
        .support = {.size = sizeof(cui_draw_v2_support),
                    .abi_version = CUI_DRAW_ABI_V2,
                    .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                    .features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                    .max_commands = 256,
                    .max_backgrounds = 128,
                    .max_pixels = 65536,
                    .max_pixels_per_background = 65536,
                    .cpu_bytes = 262144,
                    .texture_bytes = 262144,
                    .transfer_bytes = 262144,
                    .metadata_bytes = 65536},
        .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS};
}
static bool oracle_legacy_mode;
static bool oracle_begin(cui_context *ui, float w, float h, float dt) {
    if (oracle_legacy_mode)
        return cui_begin_frame(ui, w, h, dt);
    cui_frame_v2_options o = options();
    return cui_begin_frame_v2(ui, w, h, dt, &o);
}
static cui_draw_list oracle_end(cui_context *ui) {
    if (oracle_legacy_mode)
        return cui_end_frame(ui);
    cui_draw_list_v2 out;
    assert(cui_end_frame_v2(ui, &out));
    assert(out.header_size == sizeof(out) && out.abi_version == 2);
    assert(out.command_stride == sizeof(cui_draw_command_v2));
    static cui_draw_command legacy[256];
    assert(out.count <= 256);
    for (size_t i = 0; i < out.count; i++) {
        assert(out.commands[i].kind <= CUI_DRAW_INSET_SHADOW);
        assert(out.commands[i].kind == (uint32_t)out.commands[i].payload.legacy.type);
        legacy[i] = out.commands[i].payload.legacy;
    }
    return (cui_draw_list){legacy, out.count, out.width, out.height};
}
#include "cui/cui_software.h"
static bool v2_oracle_render(cui_software *, const cui_draw_list *, cui_color);
#define cui_software_render v2_oracle_render
#define cui_begin_frame oracle_begin
#define cui_end_frame oracle_end
#define main frozen_v2_oracle
#include "test_background_v1_oracle.c"
#undef cui_software_render
#undef main
#undef cui_begin_frame
#undef cui_end_frame
static bool v2_oracle_render(cui_software *r, const cui_draw_list *list, cui_color clear) {
    static unsigned case_index;
    bool ok = cui_software_render(r, list, clear);
    if (!ok) {
        fprintf(stderr, "V2 legacy oracle render failed case=%u count=%zu\n", case_index,
                list->count);
        for (size_t i = 0; i < list->count; i++)
            command(stderr, &list->commands[i]);
    }
    case_index++;
    return ok;
}
static cui_context *context(void) {
    cui_context *ui = cui_create((cui_config){
        .max_elements = 128, .max_commands = 256, .frame_bytes = 32768, .measure_styled = measure});
    assert(ui && !ui->v2);
    return ui;
}
static const cui_background red = {.kind = CUI_BACKGROUND_SOLID, .solid = {1, 0, 0, .5f}};
static void box(cui_context *ui, cui_id id) {
    cui_begin_box(ui, id,
                  (cui_box){.width = 40,
                            .height = 24,
                            .fixed_width = true,
                            .fixed_height = true,
                            .background = {30, 40, 50, 200},
                            .radius = 100});
}
static cui_draw_list_v2 finish(cui_context *ui) {
    cui_draw_list_v2 list;
    assert(cui_end_frame_v2(ui, &list));
    assert(!cui_frame_active(ui) && list.error == CUI_OK);
    return list;
}
static void admission(void) {
    cui_context *ui = context();
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 17, .y = 19}));
    cui_frame_v2_options good = options(), bad = good;
    for (unsigned i = 0; i < 7; i++) {
        bad = good;
        switch (i) {
        case 0:
            bad.size--;
            break;
        case 1:
            bad.support.size--;
            break;
        case 2:
            bad.support.abi_version = 1;
            break;
        case 3:
            bad.support.background_output = 0;
            break;
        case 4:
            bad.required_features |= 2;
            break;
        case 5:
            bad.support.features = 0;
            break;
        case 6:
            bad.support.features |= 2;
            break;
        }
        assert(!cui_begin_frame_v2(ui, 96, 72, 0, &bad));
        assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION && !ui->v2 && ui->queued_count == 1);
        cui_clear_error(ui);
    }
    for (unsigned i = 0; i < 6; i++) {
        bad = good;
        switch (i) {
        case 0:
            bad.command_bytes = CUI_DRAW_V2_MAX_BYTES + 1;
            break;
        case 1:
            bad.command_bytes = sizeof(cui_draw_command_v2) - 1;
            break;
        case 2:
            bad.background_bytes = sizeof(uint32_t);
            break;
        case 3:
            bad.support.max_commands = 0;
            break;
        case 4:
            bad.support.max_backgrounds = 0;
            break;
        case 5:
            bad.support.cpu_bytes = 0;
            break;
        }
        assert(!cui_begin_frame_v2(ui, 96, 72, 0, &bad));
        assert(cui_last_error(ui) == CUI_ERROR_CAPACITY && !ui->v2 && ui->queued_count == 1);
        cui_clear_error(ui);
    }
    assert(cui_begin_frame(ui, 96, 72, 0));
    size_t n;
    const cui_event *events = cui_frame_events(ui, &n);
    assert(n == 1 && events[0].x == 17 && !ui->v2);
    assert(!cui_set_background(ui, &red));
    cui_clear_error(ui);
    cui_draw_list_v2 wrong;
    assert(!cui_end_frame_v2(ui, &wrong) && wrong.error == CUI_ERROR_LIFECYCLE);
    assert(cui_frame_active(ui));
    cui_clear_error(ui);
    (void)cui_end_frame(ui);
    assert(!ui->v2);
    cui_destroy(ui);
    for (size_t nth = 0; nth < 3; nth++) {
        ui = context();
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 9}));
        fail_after = nth;
        fail_allocation = true;
        assert(!cui_begin_frame_v2(ui, 96, 72, 0, &good));
        fail_allocation = false;
        assert(!ui->v2 && !ui->in_frame && ui->queued_count == 1);
        assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
        cui_clear_error(ui);
        assert(oracle_begin(ui, 96, 72, 0));
        assert(ui->event_count == 1);
        (void)finish(ui);
        cui_destroy(ui);
    }
}
static void ownership_and_order(void) {
    cui_context *ui = context();
    cui_frame_v2_options o = options();
    o.support.max_backgrounds = 1;
    assert(cui_begin_frame_v2(ui, 96, 72, 0, &o));
    cui_begin_box(ui, 1, (cui_box){.custom_opacity = true, .opacity = .5f, .clip = true});
    box(ui, 2);
    cui_box_shadow shadows[] = {
        {.kind = CUI_BOX_SHADOW_DROP, .layer = {.sigma = 1, .color = {0, 0, 0, 200}}},
        {.kind = CUI_BOX_SHADOW_INSET, .layer = {.sigma = 1, .color = {0, 0, 0, 200}}}};
    assert(cui_set_box_shadows(ui, shadows, 2, (cui_corner_radii){3, 3, 3, 3}));
    assert(cui_set_underlay(ui, (cui_rect){1, 1, 5, 6}, (cui_color){20, 30, 40, 255},
                            (cui_corner_radii){0}));
    cui_background copy = red;
    copy.model = (cui_background_model)999; /* inactive ignored */
    assert(cui_set_background(ui, &copy));
    copy.solid.r = 0;
    cui_end_box(ui);
    assert(cui_element_set_background(ui, 2, &red));
    cui_background bad = red;
    bad.solid.r = NAN;
    assert(!cui_element_set_background(ui, 2, &bad));
    assert(ui->v2->owner_count == 1 && ui->v2->owners[0].brush.solid.r == 1);
    cui_clear_error(ui);
    box(ui, 3);
    assert(!cui_set_background(ui, &red)); /* distinct touched owner budget */
    cui_clear_error(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list_v2 list = finish(ui);
    size_t drop = SIZE_MAX, fill = SIZE_MAX, inset = SIZE_MAX, underlay = SIZE_MAX;
    for (size_t i = 0; i < list.count; i++) {
        if (list.commands[i].kind == CUI_DRAW_DROP_SHADOW)
            drop = i;
        if (list.commands[i].kind == CUI_DRAW_V2_BACKGROUND) {
            fill = i;
            cui_background_draw_v2 b = list.commands[i].payload.background;
            assert(b.bounds.w == 40 && b.bounds.h == 24 && b.radii.top_left == 12);
            assert(b.opacity == .5f && b.brush.solid.a == .5f && b.brush.solid.r == 1);
            assert(b.brush.model == 0 && b.brush.space == 0 && b.brush.stops[0].position == 0);
        }
        if (list.commands[i].kind == CUI_DRAW_INSET_SHADOW)
            inset = i;
        if (list.commands[i].kind == CUI_DRAW_RECT && list.commands[i].payload.legacy.bounds.w == 5)
            underlay = i;
    }
    assert(drop < fill && fill < inset && inset < underlay);
    assert(ui->v2->command_capacity == 256 && ui->v2->owner_capacity == 1);
    cui_draw_command_v2 *retained = ui->v2->commands;
    assert(cui_begin_frame_v2(ui, 96, 72, 0, &o));
    box(ui, 2);
    assert(cui_set_box_shadows(ui, shadows, 2, (cui_corner_radii){0}));
    assert(cui_set_background(ui, &red));
    assert(cui_set_background(ui, NULL));
    assert(ui->v2->owner_count == 1 && !ui->v2->owners[0].active);
    cui_end_box(ui);
    list = finish(ui);
    unsigned drops = 0, insets = 0, rects = 0;
    for (size_t i = 0; i < list.count; i++) {
        assert(list.commands[i].kind != CUI_DRAW_V2_BACKGROUND);
        drops += list.commands[i].kind == CUI_DRAW_DROP_SHADOW;
        insets += list.commands[i].kind == CUI_DRAW_INSET_SHADOW;
        rects += list.commands[i].kind == CUI_DRAW_RECT;
    }
    assert(drops == 1 && insets == 1 && rects == 1 && ui->v2->commands == retained);
    assert(cui_begin_frame(ui, 96, 72, 0));
    box(ui, 2);
    cui_end_box(ui);
    cui_draw_list old = cui_end_frame(ui);
    assert(old.count && ui->v2->commands == retained && ui->v2->owner_count == 0);
    cui_destroy(ui);
}
static unsigned rejection_callbacks;
static bool reject_prepaint(const cui_prepaint_info *info, const void *payload,
                            cui_prepaint_writer *w) {
    (void)info;
    (void)w;
    cui_context *ui = *(cui_context *const *)payload;
    rejection_callbacks++;
    assert(!cui_set_background(ui, &red));
    return true;
}
static void failures_and_geometry(void) {
    cui_context *ui = context();
    assert(oracle_begin(ui, 96, 72, 0));
    box(ui, 1);
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = reject_prepaint,
                                                       .payload = &ui,
                                                       .payload_bytes = sizeof(ui),
                                                       .command_budget = 1}));
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0 && cui_frame_active(ui) && rejection_callbacks == 0);
    cui_clear_error(ui);
    cui_draw_list_v2 failed;
    assert(!cui_end_frame_v2(ui, NULL) && cui_frame_active(ui));
    cui_clear_error(ui);
    assert(!cui_end_frame_v2(ui, &failed));
    assert(!failed.commands && failed.count == 0 && rejection_callbacks == 1 && !ui->in_frame);
    cui_clear_error(ui);
    cui_frame_v2_options o = options();
    o.command_bytes = sizeof(cui_draw_command_v2);
    assert(cui_begin_frame_v2(ui, 96, 72, 0, &o));
    box(ui, 1);
    assert(cui_set_background(ui, &red));
    cui_end_box(ui);
    box(ui, 2);
    assert(cui_set_background(ui, &red));
    cui_end_box(ui);
    assert(!cui_end_frame_v2(ui, &failed) && failed.error == CUI_ERROR_CAPACITY && !failed.count);
    cui_clear_error(ui);
    for (unsigned frame = 0; frame < 2; frame++) {
        assert(oracle_begin(ui, 96, 72, 0));
        assert(!cui_element_set_background(ui, 99, &red));
        cui_clear_error(ui);
        cui_begin_portal(ui, 99, (cui_box){.radius = 4},
                         (cui_rect){10 + frame * 7, 8, 24 + frame * 3, 20}, 9);
        cui_end_box(ui);
        assert(cui_element_set_background(ui, 99, &red));
        cui_draw_list_v2 list = finish(ui);
        unsigned found = 0;
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].kind == CUI_DRAW_V2_BACKGROUND) {
                cui_rect b = list.commands[i].payload.background.bounds;
                assert(b.x == 10 + frame * 7 && b.w == 24 + frame * 3 && b.y == 8);
                found++;
            }
        assert(found == 1);
    }
    assert(oracle_begin(ui, 96, 72, 0));
    cui_begin_box(ui, 1, (cui_box){.layout_only = true});
    assert(!cui_set_background(ui, &red));
    cui_clear_error(ui);
    cui_end_box(ui);
    assert(!cui_element_set_background(ui, 1, &red));
    cui_clear_error(ui);
    box(ui, 2);
    cui_end_box(ui);
    box(ui, 2);
    cui_end_box(ui);
    assert(!cui_element_set_background(ui, 2, &red));
    cui_clear_error(ui);
    assert(!cui_end_frame_v2(ui, &failed) && !failed.commands && !failed.count);
    cui_destroy(ui);
}
static cui_context *measure_context;
static unsigned measure_rejections, diagnostic_rejections;
static cui_size guarded_measure(void *u, cui_str text, float size, cui_text_style style) {
    if (measure_context) {
        assert(!cui_set_background(measure_context, &red));
        measure_rejections++;
    }
    return measure(u, text, size, style);
}
static void guarded_diagnostic(void *u, const cui_diagnostic *d) {
    (void)d;
    cui_context *ui = u;
    assert(!cui_set_background(ui, &red));
    diagnostic_rejections++;
}
static void extra_guards_and_storage(void) {
    cui_context *ui = context();
    cui_frame_v2_options o = options();
    o.required_features = 0;
    assert(cui_begin_frame_v2(ui, 96, 72, 0, &o));
    assert(!ui->v2->map && !ui->v2->owners && ui->v2->owner_capacity == 0);
    assert(!cui_set_background(ui, &red));
    cui_clear_error(ui);
    (void)finish(ui);
    o = options();
    o.support.max_commands = 4;
    o.support.max_backgrounds = 1;
    assert(cui_begin_frame_v2(ui, 96, 72, 0, &o));
    box(ui, 0);
    assert(cui_set_background(ui, &red));
    cui_end_box(ui);
    cui_draw_list_v2 list = finish(ui);
    bool anonymous = false;
    for (size_t i = 0; i < list.count; i++)
        anonymous |= list.commands[i].kind == CUI_DRAW_V2_BACKGROUND;
    assert(anonymous && ui->v2->owner_count == 1);
    cui_draw_command_v2 *retained_commands = ui->v2->commands;
    cui_core_background_owner *retained_owners = ui->v2->owners;
    /* Growth frees expired blocks before replacements; a later allocation
     * failure leaves no active mode/frame, preserves input and remains usable. */
    o = options();
    o.support.max_commands = 512;
    cui_context *growth = context();
    cui_frame_v2_options small = options();
    small.support.max_commands = 1;
    small.support.max_backgrounds = 1;
    assert(cui_begin_frame_v2(growth, 96, 72, 0, &small));
    (void)finish(growth);
    assert(cui_queue_event(growth, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 23}));
    fail_after = 1;
    fail_allocation = true;
    assert(!cui_begin_frame_v2(growth, 96, 72, 0, &o));
    fail_allocation = false;
    assert(growth->v2->command_capacity == 256 && growth->v2->commands);
    assert(growth->v2->owner_capacity == 0 && !growth->v2->background_storage);
    assert(!growth->v2->map && !growth->v2->owners && growth->queued_count == 1 &&
           !growth->in_frame);
    assert(!growth->frame_v2 || !growth->in_frame); /* no new active mode published */
    cui_clear_error(growth);
    assert(cui_begin_frame_v2(growth, 96, 72, 0, &o));
    assert(growth->event_count == 1 && growth->events[0].x == 23);
    (void)finish(growth);
    cui_destroy(growth);
    /* Unrelated context keeps its capacities; no global owner lookup. */
    assert(ui->v2->commands == retained_commands && ui->v2->owners == retained_owners);
    assert(cui_begin_frame(ui, 96, 72, 0));
    box(ui, 1); /* unbalanced legacy diagnostics retain their old public operation */
    (void)cui_end_frame(ui);
    assert(strcmp(ui->first_diagnostic.operation, "cui_end_frame") == 0);
    cui_clear_error(ui);
    cui_set_diagnostic_callback(ui, guarded_diagnostic, ui);
    assert(oracle_begin(ui, 96, 72, 0));
    assert(!cui_element_set_background(ui, 999, &red));
    assert(diagnostic_rejections == 1);
    cui_clear_error(ui);
    (void)finish(ui);
    cui_destroy(ui);
    ui = cui_create(
        (cui_config){.max_elements = 128, .max_commands = 256, .measure_styled = guarded_measure});
    assert(ui);
    measure_context = ui;
    assert(oracle_begin(ui, 96, 72, 0));
    box(ui, 1);
    cui_text(ui, cui_string("measurement guard"), 12, (cui_color){255, 255, 255, 255});
    cui_end_box(ui);
    cui_draw_list_v2 failure;
    assert(!cui_end_frame_v2(ui, &failure) && failure.error == CUI_ERROR_LIFECYCLE);
    assert(measure_rejections && !failure.commands && !failure.count);
    measure_context = NULL;
    cui_destroy(ui);
}
static void linear_value_transport(void) {
    cui_context *ui = context();
    for (unsigned model = 0; model < 2; model++)
        for (unsigned space = 0; space < 2; space++) {
            assert(oracle_begin(ui, 96, 72, 0));
            box(ui, 1);
            cui_background b = {
                .kind = CUI_BACKGROUND_LINEAR,
                .model = (cui_background_model)model,
                .space = (cui_background_space)space,
                .angle_degrees = 270,
                .stops = {{{1, 0, 0, 0}, model ? .8f : .2f}, {{0, 0, 1, .75f}, model ? .2f : .8f}}};
            b.solid.r = NAN; /* inactive field */
            assert(cui_set_background(ui, &b));
            b.stops[0].color.r = 0;
            cui_background invalid = b;
            invalid.angle_degrees = INFINITY;
            assert(!cui_set_background(ui, &invalid));
            cui_clear_error(ui);
            cui_end_box(ui);
            cui_draw_list_v2 out = finish(ui);
            unsigned found = 0;
            for (size_t i = 0; i < out.count; i++)
                if (out.commands[i].kind == CUI_DRAW_V2_BACKGROUND) {
                    cui_background copy = out.commands[i].payload.background.brush;
                    assert(copy.kind == CUI_BACKGROUND_LINEAR &&
                           copy.model == (cui_background_model)model);
                    assert(copy.space == (cui_background_space)space && copy.angle_degrees == 270);
                    assert(copy.stops[0].color.r == 1 && copy.stops[1].color.a == .75f &&
                           copy.solid.r == 0);
                    found++;
                }
            assert(found == 1);
        }
    cui_destroy(ui);
}
static void numeric_node(cui_context *ui) {
    box(ui, 10);
    (void)cui_interact(ui, 10, CUI_ROLE_SLIDER, cui_string("value"), false, false, .25f);
    cui_set_semantic_details(ui, 10, (cui_semantic_details){.minimum = 0, .maximum = 1});
    assert(cui_set_accessibility_numeric_actions(ui, 10, 7));
    cui_end_box(ui);
}
static void numeric_finalizer(void) {
    cui_context *ui = context();
    assert(oracle_begin(ui, 96, 72, 0));
    numeric_node(ui);
    (void)finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    cui_accessibility_set_value_v1 request = {.version = 1, .value = .75};
    assert(cui_queue_accessibility_set_value_v1(ui, 10, sizeof(request), &request));
    request.value = 0;
    assert(oracle_begin(ui, 96, 72, 0));
    numeric_node(ui);
    cui_accessibility_numeric_input_v1 value;
    assert(cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof(value), &value));
    assert(value.value == .75);
    assert(cui_end_frame(ui).count == 0 && cui_frame_active(ui));
    cui_clear_error(ui);
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof(value), &value));
    (void)finish(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui)); /* diagnostic persists after clear */
    assert(oracle_begin(ui, 96, 72, 0));
    size_t count;
    (void)cui_frame_events(ui, &count);
    assert(count == 0); /* no request replay */
    numeric_node(ui);
    (void)finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    cui_destroy(ui);
}
int main(int argc, char **argv) {
    assert(frozen_v2_oracle(argc, argv) ==
           0); /* All24 independent V1 fixtures through V2 wrapping. */
    oracle_legacy_mode = true;
    assert(frozen_v2_oracle(argc, argv) ==
           0); /* Same immutable golden through unchanged V1 route. */
    oracle_legacy_mode = false;
    admission();
    ownership_and_order();
    failures_and_geometry();
    numeric_finalizer();
    extra_guards_and_storage();
    linear_value_transport();
    printf("V2 sizes command=%zu owner=%zu control=%zu alignment=%zu; max command payload=%zu; "
           "hard peak payload<=33554432\n",
           sizeof(cui_draw_command_v2), sizeof(cui_core_background_owner), sizeof(cui_core_v2),
           _Alignof(cui_core_background_owner), (size_t)16384 * sizeof(cui_draw_command_v2));
    puts("Core V2 admission/ownership/order/lifecycle/numeric tests: PASS");
    return 0;
}
