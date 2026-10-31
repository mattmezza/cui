/* SPDX-License-Identifier: MIT */
/* Portable real-Core contract tests. Allocation injection affects only this TU. */
#include <stdlib.h>
static int fail_next_allocation;
static void *test_calloc(size_t n, size_t size) {
    if (fail_next_allocation) {
        fail_next_allocation = 0;
        return NULL;
    }
    return calloc(n, size);
}
#define calloc test_calloc
#ifdef CUI_SCROLL_LAYOUT_DISTRIBUTION
#define CUI_IMPLEMENTATION
#include "cui.h"
#else
#include "../../src/core/cui.c"
#endif
#undef calloc
#include <assert.h>

static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    (void)size;
    return (cui_size){(float)text.length * 8, 10};
}
static cui_context *context(size_t bytes, size_t slots) {
    cui_context *ui = cui_create((cui_config){.width = 400,
                                              .height = 400,
                                              .measure = measure,
                                              .resource_bytes = bytes,
                                              .max_resources = slots});
    assert(ui);
    return ui;
}
static cui_scroll_body_layout_v1 tree_content(cui_context *ui, cui_id id, bool inert,
                                              bool layout_only, cui_box content, bool portal) {
    cui_begin_box(
        ui, id, (cui_box){.width = 100, .height = 100, .inert = inert, .layout_only = layout_only});
    cui_begin_box(ui, id + 1, (cui_box){.width = 100, .height = 100, .clip = true});
    cui_begin_box(ui, id + 2, (cui_box){.width = 100, .height = -1});
    cui_begin_box(ui, id + 3, (cui_box){.width = 100, .height = -1});
    cui_begin_absolute_box(ui, id + 4, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
    cui_begin_box(ui, id + 5, content);
    if (portal) {
        cui_begin_portal(ui, id + 6, (cui_box){.width = 5, .height = 5}, (cui_rect){0, 0, 5, 5}, 2);
        cui_end_box(ui);
    }
    for (unsigned i = 0; i < 6; ++i)
        cui_end_box(ui);
    return (cui_scroll_body_layout_v1){.struct_size = 80,
                                       .version = 1,
                                       .mode = CUI_SCROLL_BODY_EXPLICIT,
                                       .panel = id,
                                       .outer_viewport = id + 1,
                                       .column = id + 2,
                                       .body_frame = id + 3,
                                       .viewport = id + 4,
                                       .content = id + 5,
                                       .requested_height = 100,
                                       .max_height = 100};
}
static cui_scroll_body_layout_v1 tree(cui_context *ui, cui_id id, bool inert, bool layout_only) {
    return tree_content(ui, id, inert, layout_only,
                        (cui_box){.width = 100, .height = 200, .background = {20, 40, 60, 255}},
                        false);
}
static cui_scroll_clamp_v1 standalone(cui_context *ui, cui_id id, float height) {
    cui_begin_box(ui, id, (cui_box){.width = 100, .height = height, .clip = true});
    cui_begin_box(ui, id + 1,
                  (cui_box){.width = 100, .height = 200, .background = {60, 40, 20, 255}});
    cui_end_box(ui);
    cui_end_box(ui);
    return (cui_scroll_clamp_v1){32, 1, CUI_SCROLL_CLAMP_Y, 0, id, id + 1};
}
static void failed_frame(cui_context *ui, cui_error error) {
    assert(cui_end_frame(ui).count == 0);
    assert(cui_last_error(ui) == error);
    assert(ui->scroll_layout == &cui_core_sl_failed);
    assert(cui_core_sl_state_get(ui) == NULL);
    assert(cui_core_sl_failed.flags == CUI_CORE_SL_FAILED && !cui_core_sl_failed.clamps);
    cui_clear_error(ui);
}
static void malformed(void) {
    cui_context *ui = context(0, 0);
    float outer = 0, body = 180;
    cui_rect retained = {0};
    for (unsigned mode = 0; mode < 16; ++mode) {
        assert(cui_begin_frame(ui, 400, 400, .016f));
        cui_scroll_body_layout_v1 r = tree(ui, 100, false, false);
        float *a = &outer, *b = &body;
        switch (mode) {
        case 0:
            r.struct_size = 79;
            break;
        case 1:
            r.version = 2;
            break;
        case 2:
            r.reserved = 1;
            break;
        case 3:
            r.reserved2 = 1;
            break;
        case 4:
            r.mode = 2;
            break;
        case 5:
            r.max_height = NAN;
            break;
        case 6:
            r.min_height = 101;
            break;
        case 7:
            r.content = 0;
            break;
        case 8:
            r.content = r.panel;
            break;
        case 9:
            r.outer_viewport = r.column;
            r.column = 101;
            break;
        case 10:
            a = NULL;
            break;
        case 11:
            b = a;
            break;
        case 12:
            b = &retained.y;
            assert(cui_track_bounds(ui, r.panel, &retained));
            break;
        case 13:
            r.panel = 999;
            break;
        case 14:
            r.requested_height = 101;
            break;
        default:
            r.mode = CUI_SCROLL_BODY_AUTO;
            r.requested_height = 1;
            break;
        }
        assert(!cui_layout_scroll_body_v1(ui, &r, a, b));
        assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
        assert(ui->scroll_layout == &cui_core_sl_failed);
        failed_frame(ui, CUI_ERROR_INVALID_ACTION);
        assert(body == 180);
    }
    /* Closed-scope validation, no bounds inference or open-box fallback. */
    assert(cui_begin_frame(ui, 400, 400, .016f));
    cui_scroll_body_layout_v1 r = tree(ui, 100, false, false);
    cui_begin_box(ui, 700, (cui_box){.width = 10, .height = 10});
    r.content = 700;
    assert(!cui_layout_scroll_body_v1(ui, &r, &outer, &body));
    cui_end_box(ui);
    failed_frame(ui, CUI_ERROR_INVALID_ACTION);
    cui_destroy(ui);
}
static unsigned prepaints;
static cui_context *wrong_phase_ui;
static bool paint(const cui_prepaint_info *info, const void *payload, cui_prepaint_writer *writer) {
    (void)info;
    (void)payload;
    (void)writer;
    ++prepaints;
    if (wrong_phase_ui) {
        assert(!cui_layout_scroll_body_v1(wrong_phase_ui, NULL, NULL, NULL));
        assert(!wrong_phase_ui->scroll_layout);
    }
    return true;
}
static void atomic_aliases_and_contexts(void) {
    cui_context *a = context(0, 0), *b = context(0, 0);
    float ax = 0, ay = 180, bx = 0, by = 180;
    assert(cui_begin_frame(a, 400, 400, .016f));
    cui_scroll_body_layout_v1 ar = tree(a, 100, false, false);
    assert(cui_layout_scroll_body_v1(a, &ar, &ax, &ay));
    const cui_core_scroll_layout_header *accepted = a->scroll_layout;
    cui_scroll_body_layout_v1 other = tree(a, 200, false, false);
    assert(!cui_layout_scroll_body_v1(a, &other, &ay, &by)); /* opposite-role crosslink */
    assert(accepted->bodies == 1 && accepted->clamps == 2);
    /* Immutable failure marker cannot poison a simultaneous different context. */
    assert(cui_begin_frame(b, 400, 400, .016f));
    cui_scroll_body_layout_v1 br = tree(b, 100, false, false);
    assert(cui_layout_scroll_body_v1(b, &br, &bx, &by));
    cui_end_frame(b);
    assert(cui_last_error(b) == CUI_OK && by == 100);
    failed_frame(a, CUI_ERROR_INVALID_ACTION);
    assert(ay == 180 && cui_core_sl_failed.flags == CUI_CORE_SL_FAILED &&
           !cui_core_sl_failed.clamps);
    /* Late retained-bounds alias invalidates all plans and preserves both models. */
    assert(cui_begin_frame(a, 400, 400, .016f));
    cui_rect storage = {0, 180, 0, 0};
    ar = tree(a, 100, false, false);
    assert(cui_layout_scroll_body_v1(a, &ar, &ax, &ay));
    other = tree(a, 200, false, false);
    assert(cui_layout_scroll_body_v1(a, &other, &bx, &storage.y));
    assert(cui_track_bounds(a, other.panel, &storage));
    failed_frame(a, CUI_ERROR_LAYOUT);
    assert(ay == 180 && storage.y == 180);
    /* Clearing a registration diagnostic never revives this frame. */
    assert(cui_begin_frame(a, 400, 400, .016f));
    ar = tree(a, 100, false, false);
    ar.reserved = 1;
    assert(!cui_layout_scroll_body_v1(a, &ar, &ax, &ay));
    cui_clear_error(a);
    failed_frame(a, CUI_ERROR_LAYOUT);
    assert(ay == 180);
    /* Finite values are revalidated after registration even without a text provider. */
    assert(cui_begin_frame(a, 400, 400, .016f));
    ar = tree(a, 100, false, false);
    ax = 37;
    assert(cui_layout_scroll_body_v1(a, &ar, &ax, &ay));
    ay = NAN;
    failed_frame(a, CUI_ERROR_LAYOUT);
    assert(ax == 37 && isnan(ay));
    ay = 180;
    /* Next frame starts normally, including reused resource slots. */
    assert(cui_begin_frame(a, 400, 400, .016f));
    ar = tree(a, 100, false, false);
    assert(cui_layout_scroll_body_v1(a, &ar, &ax, &ay));
    cui_end_frame(a);
    assert(cui_last_error(a) == CUI_OK && ay == 100);
    cui_destroy(a);
    cui_destroy(b);
}
static void clamps_and_limits(void) {
    cui_context *ui = context(0, 0);
    float ys[65];
    for (unsigned trial = 0; trial < 3; ++trial) {
        assert(cui_begin_frame(ui, 400, 400, .016f));
        unsigned count = trial == 1 ? 65 : 64;
        for (unsigned i = 0; i < count; ++i) {
            ys[i] = 180;
            cui_scroll_clamp_v1 r = standalone(ui, 100 + i * 10, 100);
            assert(cui_layout_scroll_clamp_v1(ui, &r, &ys[i]) == (i < 64));
        }
        if (trial == 1) {
            failed_frame(ui, CUI_ERROR_CAPACITY);
            for (unsigned i = 0; i < count; ++i)
                assert(ys[i] == 180);
        } else {
            cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            for (unsigned i = 0; i < count; ++i)
                assert(ys[i] == 100);
        }
    }
    /* Independent same-frame clamp resize and no implicit X support. */
    assert(cui_begin_frame(ui, 400, 400, .016f));
    float y = 180;
    cui_scroll_clamp_v1 r = standalone(ui, 100, 150);
    assert(cui_layout_scroll_clamp_v1(ui, &r, &y));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && y == 50);
    assert(cui_begin_frame(ui, 400, 400, .016f));
    r = standalone(ui, 100, 150);
    r.axes = 3;
    assert(!cui_layout_scroll_clamp_v1(ui, &r, &y));
    failed_frame(ui, CUI_ERROR_INVALID_ACTION);
    cui_destroy(ui);
}
static void minimum_and_admission(void) {
    cui_context *ui = context(0, 0);
    float outer = 0, body = 180;
    assert(cui_begin_frame(ui, 400, 400, .016f));
    cui_scroll_body_layout_v1 r = tree(ui, 100, false, false);
    r.mode = CUI_SCROLL_BODY_AUTO;
    r.requested_height = 0;
    r.min_height = 300;
    r.max_height = 400;
    assert(cui_layout_scroll_body_v1(ui, &r, &outer, &body));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_bounds(ui, 100).h == 300 && cui_bounds(ui, 104).h == 300 && body == 0);
    for (unsigned mode = 0; mode < 2; ++mode) {
        assert(cui_begin_frame(ui, 400, 400, .016f));
        body = 180;
        r = tree_content(
            ui, 100, false, false,
            (cui_box){.width = 100, .height = mode ? 200 : .5f, .height_percent = !mode}, mode);
        assert(cui_layout_scroll_body_v1(ui, &r, &outer, &body));
        if (!mode) {
            failed_frame(ui, CUI_ERROR_LAYOUT);
            assert(body == 180);
        } else {
            /* Core portals intentionally detach their parent: a separate root,
             * not intrinsic content or an inherited-clip relationship. */
            cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK && body == 100);
            assert(cui_bounds(ui, 106).w == 5 && cui_bounds(ui, 106).h == 5);
        }
    }
    /* A copied descriptor is independent of later caller record mutation. */
    assert(cui_begin_frame(ui, 400, 400, .016f));
    r = tree(ui, 100, false, false);
    assert(cui_layout_scroll_body_v1(ui, &r, &outer, &body));
    memset(&r, 0, sizeof r);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && body == 100);
    cui_destroy(ui);
}
static void allocations_and_legacy(void) {
    for (unsigned mode = 0; mode < 3; ++mode) {
        cui_context *ui = context(mode == 0 ? 1 : 0, mode == 1 ? 1 : 0);
        float outer = 0, body = 180;
        assert(cui_begin_frame(ui, 400, 400, .016f));
        if (mode == 1)
            assert(cui_resource_get(ui, 7, 8, 1, NULL, NULL));
        cui_scroll_body_layout_v1 r = tree(ui, 100, false, false);
        if (mode == 2)
            fail_next_allocation = 1;
        assert(!cui_layout_scroll_body_v1(ui, &r, &outer, &body));
        failed_frame(ui, CUI_ERROR_CAPACITY);
        assert(body == 180);
        cui_destroy(ui);
    }
    cui_context *ui = context(0, 0);
    /* Wrong phase is ordinary lifecycle failure; it cannot opt a frame in. */
    assert(!cui_layout_scroll_body_v1(ui, NULL, NULL, NULL));
    assert(cui_last_error(ui) == CUI_ERROR_LIFECYCLE && !ui->scroll_layout);
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 400, 400, .016f));
    cui_begin_box(ui, 100, (cui_box){.width = 40, .height = 40, .background = {1, 2, 3, 255}});
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint, .command_budget = 1}));
    cui_end_box(ui);
    /* Preexisting ordinary error does not suppress legacy draw/prepaint. */
    cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
    assert(cui_end_frame(ui).count > 0 && prepaints == 1 && !ui->scroll_layout);
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 400, 400, .016f));
    assert(!ui->scroll_layout && !ui->resource_bytes);
    cui_begin_box(ui, 100, (cui_box){.width = 40, .height = 40, .background = {1, 2, 3, 255}});
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint, .command_budget = 1}));
    cui_end_box(ui);
    wrong_phase_ui = ui;
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_LIFECYCLE && !ui->scroll_layout && prepaints == 2);
    wrong_phase_ui = NULL;
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 400, 400, .016f));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && !ui->scroll_layout);
    cui_destroy(ui);
}
int main(void) {
    malformed();
    atomic_aliases_and_contexts();
    clamps_and_limits();
    minimum_and_admission();
    allocations_and_legacy();
    printf("SCROLL_LAYOUT_CONTRACT PASS; context=%zu pointer=%zu payload=%zu body=%zu clamp=%zu\n",
           sizeof(cui_context), sizeof(void *), sizeof(cui_core_sl_state),
           sizeof(cui_scroll_body_layout_v1), sizeof(cui_scroll_clamp_v1));
    return 0;
}
