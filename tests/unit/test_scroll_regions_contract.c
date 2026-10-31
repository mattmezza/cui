/* SPDX-License-Identifier: MIT */
/* Public V2 resource/ownership/error oracles; no layout implementation copy. */
#include "dialog_flex_probe.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
static bool close_to(float a, float b) {
    return fabsf(a - b) < .05f;
}
static cui_scroll_region_sizing_v1 flex(bool intrinsic) {
    return (cui_scroll_region_sizing_v1){
        .mode = CUI_SCROLL_REGION_FLEX1,
        .minimum_policy = intrinsic ? CUI_SCROLL_REGION_MIN_INTRINSIC : CUI_SCROLL_REGION_MIN_ZERO,
        .max_height = FLT_MAX};
}
/* Returned records may immediately leave scope; retained bounds may not. */
static cui_scroll_body_layout_v2 tree(cui_context *ui, cui_id b, bool v2, unsigned flags,
                                      cui_rect *viewport) {
    cui_begin_box(
        ui, b, (cui_box){.width = 120, .inert = (flags & 1) != 0, .layout_only = (flags & 2) != 0});
    cui_begin_box(ui, b + 1, (cui_box){.width = 120, .clip = true});
    cui_begin_box(ui, b + 2, (cui_box){.width = 120});
    if (flags & 4)
        cui_begin_portal(ui, b + 6, (cui_box){0}, (cui_rect){0, 0, 120, 40}, 2);
    else
        cui_begin_box(
            ui, b + 6,
            (cui_box){.width = 120, .height = 40, .fixed_height = true, .clip = (flags & 8) != 0});
    cui_begin_box(ui, b + 8, (cui_box){.width = 120, .height = 40, .fixed_height = true});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, b + 3, (cui_box){.width = 120, .height = -1});
    cui_begin_absolute_box(ui, b + 4, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
    if (viewport)
        assert(cui_track_bounds(ui, b + 4, viewport));
    cui_begin_box(
        ui, b + 5,
        (cui_box){
            .width = 120, .height = 800, .fixed_height = true, .background = {91, 92, 93, 255}});
    if (flags & 16) {
        cui_begin_portal(ui, 90000, (cui_box){.background = {77, 78, 79, 255}},
                         (cui_rect){180, 10, 20, 20}, 3);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    if (!v2) {
        cui_begin_box(ui, b + 7, (cui_box){.width = 120, .height = 20});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    if (v2) {
        cui_begin_box(ui, b + 7, (cui_box){.width = 120, .height = 20});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return (cui_scroll_body_layout_v2){.base = {.struct_size = v2 ? 144 : 80,
                                                .version = v2 ? 2 : 1,
                                                .mode = CUI_SCROLL_BODY_EXPLICIT,
                                                .panel = b,
                                                .outer_viewport = b + 1,
                                                .column = b + 2,
                                                .body_frame = b + 3,
                                                .viewport = b + 4,
                                                .content = b + 5,
                                                .requested_height = 180,
                                                .max_height = 180},
                                       .content_region = b + 6,
                                       .footer = b + 7,
                                       .content_sizing = flex(true),
                                       .body_sizing = flex(false)};
}
static bool submit(cui_context *ui, const cui_scroll_body_layout_v2 *r, bool v2, float *outer,
                   float *body) {
    return v2 ? cui_layout_scroll_body_v2(ui, r, outer, body)
              : cui_layout_scroll_body_v1(ui, &r->base, outer, body);
}
static void resources(cui_context *ui, size_t n, size_t bytes) {
    size_t got_n, got_bytes;
    dialog_flex_resources(ui, &got_n, &got_bytes);
    assert(got_n == n && got_bytes == bytes);
}
static void budgets(void) {
    size_t base = dialog_flex_base_resource_bytes(), side = dialog_flex_sidecar_resource_bytes();
    assert(side == 2560 && base + side <= 16384);
    if (sizeof(void *) == 8)
        assert(base == 12856);
    cui_context *plain = cui_create((cui_config){.max_resources = 1, .resource_bytes = base});
    assert(plain);
    dialog_flex_allocations_begin(false);
    assert(cui_begin_frame(plain, 240, 600, .016f));
    cui_begin_box(plain, 99, (cui_box){.width = 10, .height = 10});
    cui_end_box(plain);
    (void)cui_end_frame(plain);
    assert(cui_last_error(plain) == CUI_OK);
    resources(plain, 0, 0);
    cui_destroy(plain);
    dialog_flex_allocation_counts idle = dialog_flex_allocations_read();
    assert(idle.base_alloc == 0 && idle.sidecar_alloc == 0 && idle.sidecar_attempt == 0);
    const size_t bytes[] = {base, base - 1, base + side, base + side - 1, base + side};
    const size_t slots[] = {1, 1, 2, 2, 1};
    for (unsigned i = 0; i < 5; ++i) {
        bool v2 = i >= 2, success = i == 0 || i == 2;
        cui_context *ui =
            cui_create((cui_config){.max_resources = slots[i], .resource_bytes = bytes[i]});
        assert(ui && cui_begin_frame(ui, 240, 600, .016f));
        resources(ui, 0, 0);
        dialog_flex_allocations_begin(false);
        float outer = 10000, body = 10000;
        cui_rect bounds = {0};
        cui_scroll_body_layout_v2 r = tree(ui, 1000, v2, 0, &bounds);
        assert(submit(ui, &r, v2, &outer, &body) == success);
        memset(&r, 0, sizeof r); /* Only the offset/bounds storage stays borrowed. */
        if (!success)
            cui_clear_error(ui); /* Cannot salvage this failed opt-in frame. */
        cui_draw_list draw = cui_end_frame(ui);
        if (success) {
            assert(cui_last_error(ui) == CUI_OK);
            assert(close_to(body, v2 ? 720 : 680) && outer == 0);
            assert(close_to(bounds.h, v2 ? 80 : 120));
            resources(ui, v2 ? 2 : 1, v2 ? base + side : base);
        } else {
            assert(draw.count == 0 && cui_last_error(ui) != CUI_OK && outer == 10000 &&
                   body == 10000);
            if (bytes[i] >= base) {
                cui_clear_error(ui);
                assert(cui_begin_frame(ui, 240, 600, .016f));
                r = tree(ui, 1000, false, 0, &bounds);
                assert(submit(ui, &r, false, &outer, &body));
                (void)cui_end_frame(ui);
                assert(cui_last_error(ui) == CUI_OK);
                assert(close_to(body, 680));
                resources(ui, 1, base);
            }
        }
        dialog_flex_allocation_counts before_free = dialog_flex_allocations_read();
        assert(before_free.base_alloc == (i == 1 ? 0 : 1));
        assert(before_free.sidecar_alloc == (i == 2 ? 1 : 0));
        assert(before_free.sidecar_attempt == (i == 2 ? 1 : 0));
        cui_destroy(ui);
        dialog_flex_allocation_counts after_free = dialog_flex_allocations_read();
        assert(after_free.base_free == before_free.base_alloc);
        assert(after_free.sidecar_free == before_free.sidecar_alloc);
    }
    /* Actual allocator failure, after the base succeeds; no fake stack/budget bypass. */
    cui_context *ui = cui_create((cui_config){0});
    assert(ui && cui_begin_frame(ui, 240, 600, .016f));
    dialog_flex_allocations_begin(true);
    float outer = 10000, body = 10000;
    cui_scroll_body_layout_v2 r = tree(ui, 1000, true, 0, NULL);
    assert(!cui_layout_scroll_body_v2(ui, &r, &outer, &body));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_draw_list draw = cui_end_frame(ui);
    assert(draw.count == 0 && outer == 10000 && body == 10000);
    dialog_flex_allocation_counts count = dialog_flex_allocations_read();
    assert(count.base_alloc == 1 && count.sidecar_attempt == 1 && count.sidecar_alloc == 0);
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 240, 600, .016f));
    r = tree(ui, 1000, false, 0, NULL);
    assert(submit(ui, &r, false, &outer, &body));
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    count = dialog_flex_allocations_read();
    assert(count.base_alloc == 1 && count.base_free == 1 && count.sidecar_free == 0);
}
static void mixed_and_retention(void) {
    size_t base = dialog_flex_base_resource_bytes(), side = dialog_flex_sidecar_resource_bytes();
    for (unsigned order = 0; order < 2; ++order) {
        cui_context *ui =
            cui_create((cui_config){.max_resources = 2, .resource_bytes = base + side});
        assert(ui && cui_begin_frame(ui, 240, 600, .016f));
        float outer[2] = {10000, 10000}, body[2] = {10000, 10000};
        cui_rect bounds[2] = {{0}};
        for (unsigned i = 0; i < 2; ++i) {
            bool v2 = i == order;
            cui_scroll_body_layout_v2 r = tree(ui, 1000 + i * 100, v2, 0, &bounds[i]);
            assert(submit(ui, &r, v2, &outer[i], &body[i]));
        }
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        resources(ui, 2, base + side);
        for (unsigned i = 0; i < 2; ++i)
            assert(close_to(body[i], i == order ? 720 : 680));
        cui_destroy(ui);
    }
    cui_context *ui = cui_create((cui_config){.max_resources = 2, .resource_bytes = base + side});
    assert(ui);
    dialog_flex_allocations_begin(false);
    const int route[] = {2, 1, 1, 2, 0, 2, 0, 0, 1};
    const unsigned counts[] = {2, 2, 1, 2, 2, 2, 2, 0, 1};
    for (unsigned i = 0; i < sizeof route / sizeof route[0]; ++i) {
        assert(cui_begin_frame(ui, 240, 600, .016f));
        float outer = 10000, body = 10000;
        if (route[i]) {
            cui_scroll_body_layout_v2 r = tree(ui, 1000, route[i] == 2, 0, NULL);
            assert(submit(ui, &r, route[i] == 2, &outer, &body));
        }
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        resources(ui, counts[i], counts[i] == 2 ? base + side : counts[i] ? base : 0);
    }
    dialog_flex_allocation_counts before = dialog_flex_allocations_read();
    assert(before.base_alloc == 3 && before.sidecar_alloc == 3 && before.base_free == 2 &&
           before.sidecar_free == 3);
    cui_destroy(ui);
    dialog_flex_allocation_counts after = dialog_flex_allocations_read();
    assert(after.base_free == 3 && after.sidecar_free == 3);
}
static void cache_failures(void) {
    for (unsigned mode = 0; mode < 6; ++mode) {
        cui_context *ui = cui_create((cui_config){0});
        assert(ui && cui_begin_frame(ui, 240, 600, .016f));
        float outer = 10000, body = 10000;
        cui_scroll_body_layout_v2 r = tree(ui, 1000, true, 0, NULL);
        assert(cui_layout_scroll_body_v2(ui, &r, &outer, &body));
        dialog_flex_corrupt_cache(ui, mode);
        cui_draw_list draw = cui_end_frame(ui);
        assert(draw.count == 0 && cui_last_error(ui) != CUI_OK && outer == 10000 && body == 10000);
        cui_clear_error(ui);
        assert(cui_begin_frame(ui, 240, 600, .016f));
        r = tree(ui, 1000, true, 0, NULL);
        assert(cui_layout_scroll_body_v2(ui, &r, &outer, &body));
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && close_to(body, 720));
        cui_destroy(ui);
    }
}
static void aliases_and_lifetime(void) {
    for (unsigned mode = 0; mode < 6; ++mode) {
        cui_context *ui = cui_create((cui_config){0});
        assert(ui && cui_begin_frame(ui, 240, 600, .016f));
        float outer = 10000, body = 10000;
        cui_rect retained = {1, 2, 3, 10000};
        cui_id b = mode == 3 ? 76888 : 1000;
        cui_scroll_body_layout_v2 r = tree(ui, b, true, 0, NULL);
        if (mode == 0)
            r.footer = r.content_region;
        if (mode == 1)
            r.content_sizing.reserved = 1;
        bool accepted = cui_layout_scroll_body_v2(ui, &r, &outer, mode == 2 ? &retained.h : &body);
        assert(accepted == (mode >= 2));
        if (mode == 2)
            assert(cui_track_bounds(ui, b, &retained));
        if (mode == 3) { /* Frozen actual folded alias of Content76894; no copied hash. */
            cui_begin_box(ui, 86448, (cui_box){.width = 1, .height = 1});
            cui_end_box(ui);
        }
        if (mode == 4)
            body = NAN;
        if (mode == 5) {
            cui_begin_box(ui, r.footer, (cui_box){.width = 1, .height = 1});
            cui_end_box(ui);
        }
        cui_draw_list draw = cui_end_frame(ui);
        assert(cui_last_error(ui) != CUI_OK && draw.count == 0 && outer == 10000);
        assert(mode == 4 ? isnan(body) : body == 10000);
        assert(retained.h == 10000);
        cui_destroy(ui);
    }
    /* Optional Content must not alias a standalone clamp in either order. */
    for (unsigned reverse = 0; reverse < 2; ++reverse) {
        cui_context *ui = cui_create((cui_config){0});
        assert(ui && cui_begin_frame(ui, 240, 600, .016f));
        float outer = 10000, body = 10000, extra = 10000;
        cui_scroll_body_layout_v2 r = tree(ui, 1000, true, 8, NULL);
        cui_scroll_clamp_v1 clamp = {32, 1, CUI_SCROLL_CLAMP_Y, 0, 1006, 1008};
        if (reverse) {
            assert(cui_layout_scroll_body_v2(ui, &r, &outer, &body));
            assert(!cui_layout_scroll_clamp_v1(ui, &clamp, &extra));
        } else {
            assert(cui_layout_scroll_clamp_v1(ui, &clamp, &extra));
            assert(!cui_layout_scroll_body_v2(ui, &r, &outer, &body));
        }
        cui_draw_list draw = cui_end_frame(ui);
        assert(draw.count == 0 && cui_last_error(ui) != CUI_OK && outer == 10000 && body == 10000 &&
               extra == 10000);
        cui_destroy(ui);
    }
}
static void inactive_and_portals(void) {
    for (unsigned flags = 1; flags <= 4; ++flags) {
        cui_context *ui = cui_create((cui_config){0});
        assert(ui && cui_begin_frame(ui, 240, 600, .016f));
        float outer = 10000, body = 10000;
        cui_rect bounds = {0};
        cui_scroll_body_layout_v2 r = tree(ui, 1000, true, flags == 3 ? 16 : flags, &bounds);
        bool accepted = cui_layout_scroll_body_v2(ui, &r, &outer, &body);
        assert(accepted == (flags != 4));
        cui_draw_list draw = cui_end_frame(ui);
        if (flags == 4)
            assert(cui_last_error(ui) != CUI_OK && draw.count == 0 && body == 10000);
        else {
            assert(cui_last_error(ui) == CUI_OK && close_to(bounds.h, 80));
            if (flags < 3)
                assert(outer == 10000 && body == 10000);
            if (flags == 2)
                assert(draw.count == 0);
            if (flags == 3) {
                bool portal = false;
                for (size_t i = 0; i < draw.count; ++i)
                    portal |=
                        draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == 77;
                assert(portal && close_to(body, 720));
            }
        }
        cui_destroy(ui);
    }
}
static void contexts_and_phase(void) {
    cui_context *a = cui_create((cui_config){0}), *b = cui_create((cui_config){0});
    assert(a && b);
    float outer_a = 10000, body_a = 10000, outer_b = 10000, body_b = 10000;
    assert(!cui_layout_scroll_body_v2(a, NULL, &outer_a, &body_a));
    assert(cui_last_error(a) == CUI_ERROR_LIFECYCLE);
    cui_clear_error(a);
    assert(cui_begin_frame(a, 240, 600, .016f));
    cui_scroll_body_layout_v2 r = tree(a, 1000, true, 0, NULL);
    r.body_sizing.reserved = 1;
    assert(!cui_layout_scroll_body_v2(a, &r, &outer_a, &body_a));
    cui_clear_error(a);
    assert(cui_begin_frame(b, 240, 600, .016f));
    r = tree(b, 1000, true, 0, NULL);
    assert(cui_layout_scroll_body_v2(b, &r, &outer_b, &body_b));
    (void)cui_end_frame(b);
    assert(cui_last_error(b) == CUI_OK && close_to(body_b, 720));
    cui_draw_list draw = cui_end_frame(a);
    assert(cui_last_error(a) != CUI_OK && draw.count == 0 && outer_a == 10000 && body_a == 10000);
    cui_destroy(a);
    cui_destroy(b);
}
void test_scroll_regions_contract(void) {
    budgets();
    mixed_and_retention();
    cache_failures();
    aliases_and_lifetime();
    inactive_and_portals();
    contexts_and_phase();
}
