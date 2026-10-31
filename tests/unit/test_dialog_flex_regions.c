/* SPDX-License-Identifier: MIT */
/* TEST ONLY real CUI tree versus executed, pinned independent Taffy rows. */
#include "cui/cui_scroll.h"
#include "cui/cui_software.h"
#include "dialog_flex_probe.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum {
    PANEL = 1000,
    OUTER,
    COLUMN,
    FRAME,
    BODY,
    BODY_CONTENT,
    FLEX_CONTENT,
    FOOTER,
    HEADER,
    TITLE,
    DESCRIPTION,
    THUMB,
    CARET
};
typedef struct {
    const char *name;
    float explicit_height, maximum, minimum, header, title, description, footer, content, body;
    bool has_content, has_body;
    float content_min, content_max, content_fixed;
    bool explicit_min, legacy;
    float padding_top, padding_bottom, panel_gap, inner_gap;
} test_case;
typedef struct {
    const test_case *c;
    cui_context *ui;
    cui_rect bounds[13]; /* Persistent through end_frame; never a callback local. */
    float outer_y, body_y, width;
    float expected[8];
    unsigned callbacks, prepaints, inner_parts;
    bool placement;
    float host_width, host_height, margin, preferred_top;
} fixture;
static float scene_width(const fixture *f) {
    return f->width > 0 ? f->width : 240;
}
static bool near(float a, float b) {
    return fabsf(a - b) < .055f;
}
static void equal(float a, float b, const char *what) {
    if (!near(a, b)) {
        fprintf(stderr, "%s %.6f != %.6f\n", what, a, b);
        assert(false);
    }
}
static const cui_semantic *semantic(cui_context *ui, cui_id id) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; ++i)
        if (s[i].id == id)
            return &s[i];
    return NULL;
}
static void fixed_slot(fixture *f, cui_id id, float height, uint8_t color) {
    if (height <= 0)
        return;
    ++f->callbacks;
    cui_begin_box(
        f->ui, id,
        (cui_box){.width = scene_width(f), .height = height, .background = {color, 0, 0, 255}});
    assert(cui_track_bounds(f->ui, id, &f->bounds[id - PANEL]));
    cui_end_box(f->ui);
}
/* Automatic Column gap would count the mandatory zero-height internal Frame
 * when Body is OMIT. Conditional fixed spacers preserve the exact authored
 * source child list without changing the Core record/topology. */
static void inner_gap(fixture *f) {
    if (f->inner_parts++ && f->c->inner_gap > 0) {
        cui_begin_box(
            f->ui, 5000 + f->inner_parts,
            (cui_box){.width = scene_width(f), .height = f->c->inner_gap, .fixed_height = true});
        cui_end_box(f->ui);
    }
}
static void inner_slot(fixture *f, cui_id id, float height, uint8_t color) {
    if (height > 0) {
        inner_gap(f);
        fixed_slot(f, id, height, color);
    }
}
static bool observe(const cui_prepaint_info *info, const void *payload,
                    cui_prepaint_writer *writer) {
    (void)writer;
    fixture *f = *(fixture *const *)payload;
    ++f->prepaints;
    equal(info->bounds.h, f->expected[7], "current prepaint viewport");
    if (f->placement)
        equal(info->bounds.y, f->expected[5], "current placement prepaint top");
    const cui_semantic *s = semantic(f->ui, BODY);
    assert(s);
    equal(s->details.scroll_max_y, fmaxf(0, f->c->body - f->expected[3]), "current prepaint range");
    return true;
}
static cui_scroll_region_sizing_v1 policy(bool present, bool intrinsic, float minimum,
                                          float maximum, float fixed) {
    if (!present)
        return (cui_scroll_region_sizing_v1){0};
    return (cui_scroll_region_sizing_v1){
        .mode = fixed > 0 ? CUI_SCROLL_REGION_FIXED : CUI_SCROLL_REGION_FLEX1,
        .minimum_policy = intrinsic ? CUI_SCROLL_REGION_MIN_INTRINSIC : CUI_SCROLL_REGION_MIN_ZERO,
        .requested_height = fixed,
        .min_height = minimum,
        .max_height = maximum > 0 ? maximum : FLT_MAX};
}
static cui_draw_list scene(fixture *f) {
    const test_case *c = f->c;
    assert(cui_begin_frame(f->ui, f->placement ? f->host_width : scene_width(f),
                           f->placement ? f->host_height : 600, .016f));
    if (f->placement) {
        cui_begin_portal(f->ui, 9000, (cui_box){.align_x = CUI_ALIGN_CENTER},
                         (cui_rect){f->margin, f->margin, f->host_width - 2 * f->margin,
                                    f->host_height - 2 * f->margin},
                         20);
        float cap = f->preferred_top - f->margin;
        if (cap > 0) {
            cui_begin_box(f->ui, 9001, (cui_box){.width = 1, .height = -1, .max_height = cap});
            cui_end_box(f->ui);
        }
    }
    f->inner_parts = 0;
    cui_begin_box(f->ui, PANEL,
                  (cui_box){.width = scene_width(f),
                            .gap = c->panel_gap,
                            .custom_padding = true,
                            .padding_edges = {.top = c->padding_top, .bottom = c->padding_bottom}});
    assert(cui_track_bounds(f->ui, PANEL, &f->bounds[0]));
    cui_begin_box(
        f->ui, OUTER,
        (cui_box){.width = scene_width(f), .height = -1, .clip = true, .offset_y = -f->outer_y});
    assert(cui_track_bounds(f->ui, OUTER, &f->bounds[1]));
    cui_interact(f->ui, OUTER, CUI_ROLE_GROUP, cui_string("Outer"), false, false, 0);
    cui_set_semantic_details(
        f->ui, OUTER, (cui_semantic_details){.scrollable = c->legacy, .scroll_y = f->outer_y});
    cui_begin_box(f->ui, COLUMN, (cui_box){.width = scene_width(f)});
    assert(cui_track_bounds(f->ui, COLUMN, &f->bounds[2]));
    inner_slot(f, HEADER, c->header, 30);
    inner_slot(f, TITLE, c->title, 40);
    inner_slot(f, DESCRIPTION, c->description, 50);
    if (c->has_content) {
        inner_gap(f);
        ++f->callbacks;
        cui_begin_box(f->ui, FLEX_CONTENT,
                      (cui_box){.width = scene_width(f), .background = {110, 0, 0, 255}});
        assert(cui_track_bounds(f->ui, FLEX_CONTENT, &f->bounds[6]));
        cui_begin_box(f->ui, 2000,
                      (cui_box){.width = scene_width(f),
                                .height = c->content,
                                .fixed_height = true,
                                .background = {120, 0, 0, 255}});
        cui_end_box(f->ui);
        cui_end_box(f->ui);
    }
    if (c->has_body)
        inner_gap(f);
    cui_begin_box(f->ui, FRAME, (cui_box){.width = scene_width(f), .height = -1});
    assert(cui_track_bounds(f->ui, FRAME, &f->bounds[3]));
    cui_begin_absolute_box(f->ui, BODY, (cui_box){.clip = true, .offset_y = -f->body_y},
                           (cui_rect){0, 0, -1, -1});
    assert(cui_track_bounds(f->ui, BODY, &f->bounds[4]));
    if (c->has_body) {
        cui_interact(f->ui, BODY, CUI_ROLE_GROUP, cui_string("Body"), false, false, 0);
        cui_set_semantic_details(
            f->ui, BODY,
            (cui_semantic_details){.parent = OUTER, .scrollable = true, .scroll_y = f->body_y});
        fixture *borrowed = f;
        assert(cui_set_prepaint(f->ui, (cui_prepaint_options){.callback = observe,
                                                              .payload = &borrowed,
                                                              .payload_bytes = sizeof borrowed,
                                                              .command_budget = 1}));
    }
    cui_begin_box(f->ui, BODY_CONTENT,
                  (cui_box){.width = scene_width(f),
                            .height = c->has_body ? c->body : 0,
                            .fixed_height = true,
                            .background = {130, 0, 0, 255}});
    assert(cui_track_bounds(f->ui, BODY_CONTENT, &f->bounds[5]));
    if (c->has_body)
        ++f->callbacks;
    if (c->has_body && c->body > 12) {
        cui_begin_absolute_box(f->ui, CARET, (cui_box){.background = {180, 0, 0, 255}},
                               (cui_rect){8, c->body - 10, 4, 8});
        assert(cui_track_bounds(f->ui, CARET, &f->bounds[12]));
        cui_end_box(f->ui);
    }
    cui_end_box(f->ui);
    cui_id bar = cui_scrollbar_id(BODY, true);
    if (c->has_body) {
        cui_interact(f->ui, bar, CUI_ROLE_SCROLLBAR, cui_string("Vertical"), true, false, 0);
        cui_set_semantic_details(f->ui, bar,
                                 (cui_semantic_details){.parent = BODY,
                                                        .controls = BODY,
                                                        .orientation = CUI_ORIENTATION_VERTICAL});
        cui_begin_absolute_box(f->ui, bar, (cui_box){.background = {150, 0, 0, 255}},
                               (cui_rect){0, 0, 6, 1});
        cui_begin_absolute_box(f->ui, THUMB, (cui_box){.background = {160, 0, 0, 255}},
                               (cui_rect){0, 0, 6, 1});
        assert(cui_track_bounds(f->ui, THUMB, &f->bounds[11]));
        cui_end_box(f->ui);
        cui_end_box(f->ui);
    }
    cui_end_box(f->ui); /* Body viewport */
    cui_end_box(f->ui); /* Body frame */
    if (c->legacy)
        fixed_slot(f, FOOTER, c->footer, 240);
    cui_end_box(f->ui); /* Column */
    cui_end_box(f->ui); /* Outer */
    if (!c->legacy)
        fixed_slot(f, FOOTER, c->footer, 240);
    cui_end_box(f->ui);
    cui_scroll_body_layout_v2 r = {
        .base = {.struct_size = sizeof r,
                 .version = 2,
                 .mode = c->explicit_height > 0 ? CUI_SCROLL_BODY_EXPLICIT : CUI_SCROLL_BODY_AUTO,
                 .panel = PANEL,
                 .outer_viewport = OUTER,
                 .column = COLUMN,
                 .body_frame = FRAME,
                 .viewport = BODY,
                 .content = BODY_CONTENT,
                 .requested_height = c->explicit_height,
                 .min_height = c->minimum,
                 .max_height = c->maximum},
        .content_region = c->has_content ? FLEX_CONTENT : 0,
        .footer = c->footer > 0 ? FOOTER : 0,
        .content_sizing = policy(c->has_content, !c->explicit_min, c->content_min, c->content_max,
                                 c->content_fixed),
        .body_sizing = policy(c->has_body, false, 0, 0, 0)};
    if (c->legacy) {
        r.base.struct_size = sizeof(r.base);
        r.base.version = 1;
        assert(cui_layout_scroll_body_v1(f->ui, &r.base, &f->outer_y, &f->body_y));
    } else {
        assert(cui_layout_scroll_body_v2(f->ui, &r, &f->outer_y, &f->body_y));
    }
    cui_scrollbar_track_layout_v1 track = {48, 1, 3, 0, BODY, bar, THUMB, 6, 8};
    if (c->has_body)
        assert(cui_layout_scrollbar_track_v1(f->ui, &track));
    if (f->placement)
        cui_end_box(f->ui);
    cui_draw_list draw = cui_end_frame(f->ui);
    if (cui_last_error(f->ui) != CUI_OK)
        fprintf(stderr, "%s: error%d\n", c->name, cui_last_error(f->ui));
    assert(cui_last_error(f->ui) == CUI_OK);
    return draw;
}
static void validate(fixture *f, cui_draw_list draw) {
    const test_case *c = f->c;
    equal(f->bounds[0].w, scene_width(f), "current panel width");
    equal(f->bounds[4].w, scene_width(f), "current viewport width");
    equal(f->bounds[0].h, f->expected[0], "panel");
    equal(f->bounds[1].h, f->expected[1], "inner");
    if (c->has_content) {
        equal(f->bounds[6].h, f->expected[2], "Content");
        equal(f->bounds[6].y, f->expected[4], "ContentY");
    }
    equal(f->bounds[3].h, f->expected[3], "Body frame");
    equal(f->bounds[4].h, f->expected[7], "Body viewport");
    if (c->has_body)
        equal(f->bounds[3].y, f->expected[5], "BodyY");
    if (c->footer > 0)
        equal(f->bounds[7].y, f->expected[6], "FooterY");
    if (c->has_body && c->body > 12) {
        equal(f->bounds[12].y, f->bounds[4].y - f->body_y + c->body - 10,
              "current caret positioning after final clamp");
        equal(f->bounds[12].h, 8, "current caret height");
    }
    equal(f->outer_y, 0, "source Outer no scroll");
    equal(f->body_y, fmaxf(0, c->body - f->expected[3]), "current Body clamp");
    const cui_semantic *s = semantic(f->ui, BODY);
    if (c->has_body) {
        assert(s);
        equal(s->bounds.h, f->expected[7], "published viewport");
        equal(s->details.scroll_max_y, fmaxf(0, c->body - f->expected[3]), "published range");
    } else {
        assert(!s && !semantic(f->ui, cui_scrollbar_id(BODY, true)));
    }
    const cui_semantic *outer_semantic = semantic(f->ui, OUTER);
    assert(outer_semantic && outer_semantic->details.scrollable == c->legacy);
    equal(outer_semantic->details.scroll_max_y, 0, "source hidden overflow range");
    unsigned expected = (c->header > 0) + (c->title > 0) + (c->description > 0) + (c->footer > 0) +
                        c->has_content + c->has_body;
    assert(f->callbacks == expected && f->prepaints == (unsigned)c->has_body);
    if (c->footer > 0) {
        cui_software *sw = cui_software_create((unsigned)scene_width(f), 600, NULL);
        assert(sw);
        assert(cui_software_render(sw, &draw, (cui_color){0}));
        unsigned w, h;
        const uint8_t *pixels = cui_software_pixels(sw, &w, &h);
        assert(w == (unsigned)scene_width(f) && h == 600);
        unsigned y = (unsigned)(f->bounds[7].y + 1);
        assert(y < 600 &&
               pixels[((size_t)y * w + 20) * 4] == 240); /* Footer is outside inner clip. */
        /* Footer overpaint alone would hide a missing inner clip. Content's
         * overflow must also disappear BELOW the Panel/Footer, where no later
         * child can conceal it. */
        float footer_bottom = f->bounds[7].y + f->bounds[7].h;
        unsigned below = (unsigned)(fmaxf(f->bounds[0].h, footer_bottom) + 10);
        assert(below < h && pixels[((size_t)below * w + 20) * 4] == 0);
        if (c->content >= 240 && !c->explicit_min && c->content_fixed == 0) {
            unsigned inside = (unsigned)(f->bounds[1].y + f->bounds[1].h - 2);
            if (f->bounds[1].h > 2)
                assert(pixels[((size_t)inside * w + 20) * 4] == 120);
        }
        if (footer_bottom > f->bounds[0].h + 12) {
            unsigned overflow = (unsigned)(f->bounds[0].h + 10);
            assert(pixels[((size_t)overflow * w + 20) * 4] == 240);
        }
        if (c->panel_gap > 2) {
            unsigned gap_y = (unsigned)(f->bounds[1].y + f->bounds[1].h + 1);
            assert(pixels[((size_t)gap_y * w + 20) * 4] == 0);
        }
        cui_software_destroy(sw);
    }
}
/* Give the straightforward nested V1 alternative the correct inner budget in
 * advance. It still allocates one Body, so ordinary Content remains40 rather
 * than the source's peer80. This is one real counterexample, not an impossibility
 * proof for every conceivable nested API/tree. */
static void nested_v1_counterexample(void) {
    cui_context *ui = cui_create((cui_config){.width = 240, .height = 600});
    assert(ui && cui_begin_frame(ui, 240, 600, .016f));
    float outer[2] = {0}, body[2] = {0};
    cui_rect content_bounds = {0}, body_bounds = {0}, footer_bounds = {0};
    for (unsigned i = 0; i < 2; ++i) {
        cui_id b = 3000 + i * 100;
        cui_begin_box(ui, b,
                      (cui_box){.width = 240, .height = i ? 160 : 260, .fixed_height = true});
        cui_begin_box(ui, b + 1, (cui_box){.width = 240, .clip = true});
        cui_begin_box(ui, b + 2, (cui_box){.width = 240});
        cui_begin_box(ui, b + 6,
                      (cui_box){.width = 240, .height = i ? 40 : 60, .fixed_height = true});
        if (i)
            assert(cui_track_bounds(ui, b + 6, &content_bounds));
        cui_end_box(ui);
        cui_begin_box(ui, b + 3, (cui_box){.width = 240, .height = -1});
        cui_begin_absolute_box(ui, b + 4, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
        if (i)
            assert(cui_track_bounds(ui, b + 4, &body_bounds));
        cui_begin_box(ui, b + 5,
                      (cui_box){.width = 240, .height = i ? 800 : 0, .fixed_height = i != 0});
    }
    for (int i = 1; i >= 0; --i) {
        cui_id b = 3000 + (unsigned)i * 100;
        cui_end_box(ui); /* content */
        cui_end_box(ui); /* viewport */
        cui_end_box(ui); /* frame */
        if (!i) {
            cui_begin_box(ui, b + 7, (cui_box){.width = 240, .height = 40, .fixed_height = true});
            assert(cui_track_bounds(ui, b + 7, &footer_bounds));
            cui_end_box(ui);
        }
        cui_end_box(ui); /* column */
        cui_end_box(ui); /* outer */
        cui_end_box(ui); /* panel */
        cui_scroll_body_layout_v1 r = {.struct_size = 80,
                                       .version = 1,
                                       .mode = CUI_SCROLL_BODY_EXPLICIT,
                                       .panel = b,
                                       .outer_viewport = b + 1,
                                       .column = b + 2,
                                       .body_frame = b + 3,
                                       .viewport = b + 4,
                                       .content = b + 5,
                                       .requested_height = i ? 160 : 260,
                                       .max_height = 260};
        assert(cui_layout_scroll_body_v1(ui, &r, &outer[i], &body[i]));
    }
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    equal(content_bounds.h, 40, "nested V1 Content remains FIT40");
    equal(body_bounds.h, 120, "nested V1 single flexible Body120");
    equal(footer_bounds.y, 220, "nested V1 Footer");
    assert(!near(content_bounds.h, 80) && !near(body_bounds.h, 80));
    cui_destroy(ui);
}
static bool capacity_tree(cui_context *ui, unsigned index, float *outer, float *body) {
    cui_id b = 10000 + index * 20;
    cui_begin_box(ui, b, (cui_box){.width = 240});
    cui_begin_box(ui, b + 1, (cui_box){.width = 240, .clip = true});
    cui_begin_box(ui, b + 2, (cui_box){.width = 240});
    cui_begin_box(ui, b + 6, (cui_box){.width = 240, .height = 40, .fixed_height = true});
    cui_end_box(ui);
    cui_begin_box(ui, b + 3, (cui_box){.width = 240, .height = -1});
    cui_begin_absolute_box(ui, b + 4, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
    cui_begin_box(ui, b + 5, (cui_box){.width = 240, .height = 800, .fixed_height = true});
    for (unsigned i = 0; i < 5; ++i)
        cui_end_box(ui);
    cui_begin_box(ui, b + 7, (cui_box){.width = 240, .height = 40, .fixed_height = true});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_scroll_body_layout_v2 r = {.base = {.struct_size = 144,
                                            .version = 2,
                                            .mode = CUI_SCROLL_BODY_EXPLICIT,
                                            .panel = b,
                                            .outer_viewport = b + 1,
                                            .column = b + 2,
                                            .body_frame = b + 3,
                                            .viewport = b + 4,
                                            .content = b + 5,
                                            .requested_height = 260,
                                            .max_height = 260},
                                   .content_region = b + 6,
                                   .footer = b + 7,
                                   .content_sizing = policy(true, true, 0, 0, 0),
                                   .body_sizing = policy(true, false, 0, 0, 0)};
    return cui_layout_scroll_body_v2(ui, &r, outer, body);
}
static void capacity_recovery(void) {
    cui_context *ui = cui_create((cui_config){.width = 240, .height = 600});
    assert(ui);
    float outer[33], body[33];
    for (unsigned fail = 0; fail < 2; ++fail) {
        assert(cui_begin_frame(ui, 240, 600, .016f));
        for (unsigned i = 0; i < 32 + fail; ++i) {
            outer[i] = body[i] = 10000;
            bool accepted = capacity_tree(ui, i, &outer[i], &body[i]);
            assert(accepted == (i < 32));
        }
        cui_draw_list draw = cui_end_frame(ui);
        if (fail) {
            assert(cui_last_error(ui) != CUI_OK && draw.count == 0);
            for (unsigned i = 0; i < 33; ++i)
                assert(outer[i] == 10000 && body[i] == 10000);
        } else {
            assert(cui_last_error(ui) == CUI_OK);
            for (unsigned i = 0; i < 32; ++i) {
                equal(outer[i], 0, "capacity Outer");
                equal(body[i], 690, "capacity Body");
            }
        }
    }
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 240, 600, .016f));
    assert(capacity_tree(ui, 0, &outer[0], &body[0]));
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    equal(body[0], 690, "next frame recovery");
    cui_destroy(ui);
}
static void placement_proof(void) {
    /* Source Positioner corner clamp, pinned positioner.rs:303. The expected
     * values are frozen constants, not computed using the proposed spacer. */
    static const struct {
        float host_w, host_h, body, max_h, expected_h, expected_y, width, preferred;
    } cases[] = {
        {400, 600, 40, 568, 40, 60, 240, 60},   {400, 600, 550, 568, 550, 34, 240, 60},
        {400, 600, 900, 568, 568, 16, 240, 60}, {180, 220, 900, 188, 188, 16, 148, 22},
        {180, 220, 40, 188, 40, 22, 148, 22},   {800, 800, 80, 768, 80, 96, 420, 96},
        {800, 800, 740, 768, 740, 44, 420, 96}, {400, 600, 40, 568, 40, 16, 240, 0},
        {400, 600, 900, 568, 568, 16, 240, 60}, {400, 600, 40, 568, 40, 60, 240, 60},
    };
    fixture f = {.placement = true, .margin = 16};
    f.ui = cui_create((cui_config){.width = 800, .height = 800});
    assert(f.ui);
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        test_case c = {.name = "source placement",
                       .has_body = true,
                       .body = cases[i].body,
                       .maximum = cases[i].max_h};
        f.c = &c;
        f.host_width = cases[i].host_w;
        f.host_height = cases[i].host_h;
        f.width = cases[i].width;
        f.preferred_top = cases[i].preferred;
        f.expected[3] = f.expected[7] = cases[i].expected_h;
        f.expected[5] = cases[i].expected_y;
        f.callbacks = f.prepaints = 0;
        cui_draw_list draw = scene(&f);
        assert(draw.count > 0);
        equal(f.bounds[0].h, cases[i].expected_h, "placement current height");
        equal(f.bounds[0].y, cases[i].expected_y, "placement current top");
        equal(f.bounds[0].x, (f.host_width - f.width) / 2, "placement centered width");
        equal(f.bounds[4].y, cases[i].expected_y, "placement current body");
        assert(f.callbacks == 1 && f.prepaints == 1);
        const cui_semantic *body = semantic(f.ui, BODY);
        assert(body);
        equal(body->bounds.y, cases[i].expected_y, "placement published body top");
        equal(body->bounds.h, cases[i].expected_h, "placement published body height");
        cui_software *sw =
            cui_software_create((unsigned)f.host_width, (unsigned)f.host_height, NULL);
        assert(sw && cui_software_render(sw, &draw, (cui_color){0}));
        unsigned w, h;
        const uint8_t *pixels = cui_software_pixels(sw, &w, &h);
        unsigned x = (unsigned)f.bounds[0].x + 20;
        unsigned top = (unsigned)cases[i].expected_y;
        unsigned bottom = (unsigned)(cases[i].expected_y + cases[i].expected_h);
        assert(w == (unsigned)f.host_width && h == (unsigned)f.host_height);
        assert(pixels[((size_t)(top + 2) * w + x) * 4] == 130);
        assert(top > 0 && pixels[((size_t)(top - 1) * w + x) * 4 + 3] == 0);
        assert(bottom < h && pixels[((size_t)bottom * w + x) * 4 + 3] == 0);
        cui_software_destroy(sw);
        equal(body->details.scroll_max_y, fmaxf(0, c.body - cases[i].expected_h),
              "placement current semantic range");
    }
    cui_destroy(f.ui);
    puts("SOURCE_PLACEMENT first-open/current grow-shrink/width/layer10 PASS");
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *oracle = fopen(argv[1], "r");
    assert(oracle);
    char line[1024];
    assert(fgets(line, sizeof line, oracle));
    static const char *names[] = {"both_fit_minimum",
                                  "content_freezes",
                                  "content_minimum_overflow",
                                  "content_minimum_zero",
                                  "content_max_freezes",
                                  "content_fixed",
                                  "body_only",
                                  "content_only",
                                  "neither",
                                  "both_empty",
                                  "auto_short",
                                  "auto_long",
                                  "auto_content_overflow",
                                  "auto_source_min96",
                                  "auto_no_spacer",
                                  "fixed_sibling_overflow",
                                  "resize_short",
                                  "resize_tall",
                                  "source_defaults",
                                  "source_body_omitted",
                                  "source_body_empty",
                                  "source_neither",
                                  "source_content_overflow",
                                  "source_auto_short",
                                  "source_footer_omitted",
                                  "source_footer_oversize_fixed",
                                  "source_footer_oversize_auto",
                                  "source_footer_auto_fits"};
    test_case cases[28];
    float oracle_geometry[28][8];
    for (unsigned i = 0; i < 28; ++i)
        cases[i] = (test_case){.name = names[i],
                               .explicit_height = 260,
                               .maximum = 260,
                               .header = 20,
                               .title = 20,
                               .description = 20,
                               .footer = 40,
                               .content = 40,
                               .body = 800,
                               .has_content = true,
                               .has_body = true};
    cases[1].content = 120;
    cases[2].content = cases[3].content = cases[5].content = 240;
    cases[3].explicit_min = true;
    cases[4].content = 20;
    cases[4].content_max = 30;
    cases[5].content_fixed = 50;
    cases[6].has_content = false;
    cases[7].has_body = cases[8].has_body = false;
    cases[8].has_content = false;
    cases[7].body = cases[8].body = cases[9].body = 0;
    cases[9].content = 0;
    for (unsigned i = 10; i <= 14; ++i)
        cases[i].explicit_height = 0;
    cases[10].content = 10;
    cases[10].body = 20;
    cases[12].content = 240;
    for (unsigned i = 13; i <= 14; ++i) {
        cases[i].header = cases[i].description = cases[i].footer = 0;
        cases[i].has_content = false;
        cases[i].body = 10;
    }
    cases[13].minimum = 96;
    cases[15].header = 300;
    cases[16].explicit_height = cases[16].maximum = 160;
    cases[17].explicit_height = cases[17].maximum = 400;
    for (unsigned i = 18; i < 28; ++i) {
        cases[i].minimum = 96;
        cases[i].header = 40;
        cases[i].description = 0;
        cases[i].padding_top = cases[i].padding_bottom = cases[i].panel_gap = 16;
        cases[i].inner_gap = 8;
    }
    cases[19].has_body = cases[21].has_body = false;
    cases[19].body = cases[20].body = cases[21].body = 0;
    cases[21].has_content = false;
    cases[22].content = 240;
    cases[23].explicit_height = 0;
    cases[23].content = 10;
    cases[23].body = 20;
    cases[24].footer = 0;
    for (unsigned i = 25; i < 28; ++i)
        cases[i].footer = 400;
    cases[26].explicit_height = cases[27].explicit_height = 0;
    cases[27].maximum = 600;
    cases[27].content = 10;
    cases[27].body = 20;
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        assert(fgets(line, sizeof line, oracle));
        char *name = strtok(line, ",");
        assert(name && strcmp(name, cases[i].name) == 0);
        fixture f = {.c = &cases[i], .outer_y = 10000, .body_y = 10000};
        for (unsigned j = 0; j < 8; ++j) {
            char *v = strtok(NULL, ",");
            assert(v);
            f.expected[j] = strtof(v, NULL);
        }
        memcpy(oracle_geometry[i], f.expected, sizeof f.expected);
        f.ui = cui_create((cui_config){.width = 240, .height = 600});
        assert(f.ui);
        printf("CASE %s\n", cases[i].name);
        fflush(stdout);
        validate(&f, scene(&f));
        cui_destroy(f.ui);
    }
    /* Same context, current dimensions on every resize, never prior tracked bounds. */
    fixture resize = {.outer_y = 10000, .body_y = 10000};
    resize.ui = cui_create((cui_config){.width = 240, .height = 600});
    assert(resize.ui);
    const unsigned sequence[] = {0, 16, 17, 2, 10, 0, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 18};
    const float widths[] = {240, 160, 400, 100, 320, 240, 240, 160, 320,
                            100, 400, 240, 240, 160, 320, 240, 240};
    for (unsigned i = 0; i < sizeof sequence / sizeof sequence[0]; ++i) {
        resize.width = widths[i];
        resize.c = &cases[sequence[i]];
        memcpy(resize.expected, oracle_geometry[sequence[i]], sizeof resize.expected);
        resize.outer_y = resize.body_y = 10000;
        resize.callbacks = resize.prepaints = 0;
        validate(&resize, scene(&resize));
    }
    cui_destroy(resize.ui);
    test_case legacy = cases[0];
    legacy.legacy = true;
    fixture v1 = {.c = &legacy,
                  .outer_y = 10000,
                  .body_y = 10000,
                  .expected = {260, 260, 40, 120, 60, 100, 220, 120}};
    v1.ui = cui_create((cui_config){.width = 240, .height = 600});
    assert(v1.ui);
    validate(&v1, scene(&v1));
    cui_destroy(v1.ui);
    assert(!fgets(line, sizeof line, oracle));
    fclose(oracle);
    placement_proof();
    nested_v1_counterexample();
    capacity_recovery();
    test_scroll_regions_contract();
    printf("SOURCE_FLEX rows28 plan%zu record%zu PASS\n",
           (dialog_flex_base_resource_bytes() + dialog_flex_sidecar_resource_bytes()),
           sizeof(cui_scroll_body_layout_v2));
    return 0;
}
