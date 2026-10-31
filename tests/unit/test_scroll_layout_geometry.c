/* SPDX-License-Identifier: MIT */
/* Test-only design probe. No Dialog composition API is implemented here. */
#include "cui/cui.h"
#include "cui/cui_scroll.h"
#include "cui/cui_text.h"
#include "scroll_layout_probe.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PANEL = 100,
    OUTER = 113,
    COLUMN = 114,
    HEADER = 109,
    CONTENT = 111,
    BODY_FRAME = 115,
    BODY = 106,
    BODY_CONTENT = 107,
    FOOTER = 112,
    TEXTBOX = 120,
    CARET = 121
};
static const char title[] = "A A A A A A A A A A A A A A A A A A A A";
static const cui_color body_color = {32, 160, 64, 255};
static const cui_color caret_color = {240, 220, 32, 255};
typedef struct fixture {
    cui_context *ui;
    cui_rect panel, outer, column, header, content, body, body_content, footer, textbox, caret;
    cui_scroll_state body_scroll, outer_scroll;
    unsigned calls[4];
    float width, height, body_height, fixed_header;
    bool auto_panel, solve, expect_error, fail_plan;
    unsigned prepaint_calls;
    cui_rect prepaint_panel;
    cui_id base;
    struct fixture *nested;
} fixture;

static cui_id fid(const fixture *f, cui_id id) {
    return f->base + id;
}
static void build_tree(fixture *, float, float, float, float);
static bool close_to(float a, float b) {
    return fabsf(a - b) < .06f;
}
static void eq(float actual, float expected) {
    if (!close_to(actual, expected)) {
        fprintf(stderr, "geometry: actual %.3f, expected %.3f\n", actual, expected);
        assert(close_to(actual, expected));
    }
}
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    (void)size;
    float width = 0;
    for (size_t i = 0; i < text.length; ++i)
        width += text.data[i] == ' ' ? 4 : 8;
    return (cui_size){width, 10};
}
static void init(fixture *f, cui_font *font) {
    memset(f, 0, sizeof *f);
    cui_config config = {.width = 272, .height = 332, .measure = measure};
    if (font) {
        config.measure = cui_font_measure;
        config.measure_styled = cui_font_measure_styled;
        config.measure_user = font;
        config.font_metrics = cui_font_get_metrics;
        config.text_geometry = cui_font_text_geometry;
        config.text_geometry_line = cui_font_text_geometry_line;
        config.text_capabilities = cui_font_text_capabilities();
    }
    f->ui = cui_create(config);
    assert(f->ui);
}
static void track(fixture *f, cui_id id, cui_rect *out) {
    /* All out pointers address fixture storage, not a returned scene stack. */
    assert(cui_track_bounds(f->ui, id, out));
}
static cui_scroll_options scroll_options(float width, float height, float content_height) {
    return (cui_scroll_options){.width = width,
                                .height = height,
                                .content_width = width,
                                .content_height = content_height,
                                .vertical = true,
                                .hide_scrollbars = true};
}
static void emit_header(fixture *f) {
    cui_context *ui = f->ui;
    const float inner_width = f->width - 32;
    ++f->calls[0];
    cui_begin_box(ui, fid(f, HEADER), (cui_box){.width = inner_width, .height = f->fixed_header});
    track(f, fid(f, HEADER), &f->header);
    if (f->fixed_header <= 0)
        cui_text_with_line_height(ui, cui_string(title), 10, (cui_color){0, 0, 0, 255}, 10);
    cui_end_box(ui);
}
static void emit_content(fixture *f) {
    cui_context *ui = f->ui;
    const float inner_width = f->width - 32;
    ++f->calls[1];
    cui_begin_box(ui, fid(f, CONTENT), (cui_box){.width = inner_width, .height = 20});
    track(f, fid(f, CONTENT), &f->content);
    cui_end_box(ui);
}
static void emit_body(fixture *f) {
    cui_context *ui = f->ui;
    const float inner_width = f->width - 32;
    ++f->calls[2];
    cui_begin_box(ui, fid(f, BODY_CONTENT),
                  (cui_box){.width = inner_width,
                            .height = f->fail_plan ? .5f : f->body_height,
                            .height_percent = f->fail_plan,
                            .background = body_color});
    track(f, fid(f, BODY_CONTENT), &f->body_content);
    if (f->nested) {
        fixture *child = f->nested;
        child->ui = ui;
        build_tree(child, fminf(128, inner_width), 200, child->body_height, 0);
    } else if (f->body_height >= 24) {
        /* A real semantic/caret subtree near the body's end exposes coordinate drift. */
        cui_begin_absolute_box(ui, fid(f, TEXTBOX), (cui_box){.background = {180, 30, 40, 255}},
                               (cui_rect){4, fmaxf(0, f->body_height - 24), 80, 20});
        cui_interact(ui, fid(f, TEXTBOX), CUI_ROLE_TEXTBOX, cui_string("Caret owner"), false, false,
                     0);
        assert(cui_set_text_input_config(ui, fid(f, TEXTBOX), (cui_text_input_config){0}));
        cui_focus(ui, fid(f, TEXTBOX));
        track(f, fid(f, TEXTBOX), &f->textbox);
        cui_begin_absolute_box(ui, fid(f, CARET), (cui_box){.background = caret_color},
                               (cui_rect){4, 4, 1, 12});
        cui_end_box(ui);
        assert(cui_set_text_input_geometry_rect(ui, fid(f, TEXTBOX), fid(f, CARET), 12));
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
static void emit_footer(fixture *f) {
    cui_context *ui = f->ui;
    const float inner_width = f->width - 32;
    ++f->calls[3];
    cui_begin_box(ui, fid(f, FOOTER),
                  (cui_box){.width = inner_width, .height = 40, .background = {40, 60, 200, 255}});
    track(f, fid(f, FOOTER), &f->footer);
    cui_interact(ui, fid(f, FOOTER), CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    cui_end_box(ui);
}
static bool prepaint(const cui_prepaint_info *info, const void *payload,
                     cui_prepaint_writer *writer) {
    (void)writer;
    fixture *f = *(fixture *const *)payload;
    ++f->prepaint_calls;
    f->prepaint_panel = info->bounds;
    return true;
}
static void build_tree(fixture *f, float width, float height, float body_height,
                       float fixed_header) {
    cui_context *ui = f->ui;
    f->width = width;
    f->height = height;
    f->body_height = body_height;
    f->fixed_header = fixed_header;
    const float inner_width = width - 32, inner_height = height - 32;
    const cui_scroll_options outer_options = scroll_options(inner_width, inner_height, f->column.h);
    const cui_scroll_options body_options =
        scroll_options(inner_width, f->body.h > 0 ? f->body.h : 48, f->body_content.h);
    cui_begin_box(ui, fid(f, PANEL),
                  (cui_box){.width = width,
                            .height = f->auto_panel ? 0 : height,
                            .max_height = f->auto_panel ? height : 0,
                            .padding = 16});
    track(f, fid(f, PANEL), &f->panel);
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = prepaint,
                                                       .payload = &f,
                                                       .payload_bytes = sizeof f,
                                                       .command_budget = 1}));
    assert(cui_begin_focus_scope(ui, fid(f, PANEL), true));
    cui_interact(ui, fid(f, PANEL), CUI_ROLE_DIALOG, cui_string("Prototype"), false, false, 0);
    cui_begin_box(ui, fid(f, OUTER),
                  (cui_box){.width = inner_width,
                            .height = f->auto_panel ? -1 : inner_height,
                            .max_height = f->auto_panel ? inner_height : 0,
                            .clip = true,
                            .offset_y = -f->outer_scroll.y});
    track(f, fid(f, OUTER), &f->outer);
    cui_base_scrollable(ui, fid(f, OUTER), &f->outer_scroll, outer_options);
    cui_begin_box(ui, fid(f, COLUMN), (cui_box){.width = inner_width, .height = -1, .gap = 8});
    track(f, fid(f, COLUMN), &f->column);

    emit_header(f);
    emit_content(f);

    cui_begin_box(ui, fid(f, BODY_FRAME), (cui_box){.width = inner_width, .height = -1});
    if (!f->solve) {
        cui_begin_box(
            ui, 0, (cui_box){.width = 0, .fixed_width = true, .height = fminf(48, inner_height)});
        cui_end_box(ui);
    }
    cui_begin_absolute_box(ui, fid(f, BODY), (cui_box){.clip = true, .offset_y = -f->body_scroll.y},
                           (cui_rect){0, 0, -1, -1});
    track(f, fid(f, BODY), &f->body);
    cui_base_scrollable(ui, fid(f, BODY), &f->body_scroll, body_options);
    emit_body(f);
    cui_base_scrollable_finish(ui, fid(f, BODY), &f->body_scroll, body_options);
    assert(cui_set_box_scroll_offset(ui, fid(f, BODY), 0, -f->body_scroll.y));
    cui_end_box(ui); /* attached viewport */
    cui_end_box(ui); /* flow body frame */

    emit_footer(f);
    cui_end_box(ui); /* column */
    cui_base_scrollable_finish(ui, fid(f, OUTER), &f->outer_scroll, outer_options);
    assert(cui_set_box_scroll_offset(ui, fid(f, OUTER), 0, -f->outer_scroll.y));
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    cui_end_box(ui);
    if (f->solve) {
        prototype_link link = {.panel = fid(f, PANEL),
                               .outer = fid(f, OUTER),
                               .column = fid(f, COLUMN),
                               .frame = fid(f, BODY_FRAME),
                               .viewport = fid(f, BODY),
                               .content = fid(f, BODY_CONTENT),
                               .requested_height = height,
                               .max_height = height,
                               .outer_y = &f->outer_scroll.y,
                               .body_y = &f->body_scroll.y,
                               .automatic = f->auto_panel};
        assert(prototype_register(ui, link));
    }
}
static cui_draw_list scene(fixture *f, float width, float height, float body_height,
                           float fixed_header) {
    cui_context *ui = f->ui;
    assert(cui_begin_frame(ui, width, height, .016f));
    build_tree(f, width, height, body_height, fixed_header);
    cui_draw_list draw = cui_end_frame(ui);
    f->caret = cui_bounds(ui, fid(f, CARET)); /* IME owns the sole retained caret registration. */
    if (f->expect_error) {
        assert(cui_last_error(ui) == CUI_ERROR_LAYOUT);
        assert(draw.count == 0);
    } else {
        assert(cui_last_error(ui) == CUI_OK);
        eq(f->prepaint_panel.h, f->panel.h);
        if (f->solve) {
            prototype_stats stats = prototype_get_stats(ui);
            assert(stats.first == 1 && stats.second == 1 && stats.commits == (f->nested ? 2u : 1u));
            assert(stats.input_unchanged && stats.resources_unchanged && stats.text_unchanged);
            assert(stats.wrapped_before == stats.wrapped_after);
        }
    }
    return draw;
}
static void callbacks(fixture *f, unsigned frames) {
    for (unsigned i = 0; i < 4; ++i)
        assert(f->calls[i] == frames);
    assert(f->prepaint_calls == frames);
}
static void geometry(fixture *f, float header_height) {
    eq(f->outer.x, 16);
    eq(f->outer.y, 16);
    eq(f->outer.w, f->width - 32);
    eq(f->outer.h, f->height - 32);
    eq(f->header.h, header_height);
    eq(f->content.h, 20);
    eq(f->body.h, fmaxf(48, f->height - 32 - header_height - 20 - 40 - 24));
    eq(f->body.w, f->width - 32);
    eq(f->footer.h, 40);
    eq(f->footer.y, f->height - 56);
    eq(f->column.h, f->height - 32);
}
static void title_commands(cui_draw_list draw, unsigned expected_lines) {
    unsigned lines = 0, letters = 0;
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            ++lines;
            for (size_t k = 0; k < draw.commands[i].text.length; ++k)
                letters += draw.commands[i].text.data[k] == 'A';
        }
    assert(lines == expected_lines && letters == 20);
}
static unsigned expected_lines(cui_font *font, float width) {
    float a = font ? cui_font_measure(font, cui_string("A"), 10).width : 8;
    float space = font ? cui_font_measure(font, cui_string(" "), 10).width : 4;
    unsigned per_line = (unsigned)floorf((width + space) / (a + space));
    assert(per_line > 0);
    return (20 + per_line - 1) / per_line;
}
static void geometry_suite(cui_font *font, bool solve) {
    fixture f;
    init(&f, font);
    f.solve = solve;
    const float widths[] = {272, 128, 272, 128, 272};
    for (unsigned i = 0; i < 5; ++i) {
        unsigned lines = expected_lines(font, widths[i] - 32);
        cui_draw_list draw = scene(&f, widths[i], 332, 1000, 0);
        geometry(&f, (float)lines * 10);
        title_commands(draw, lines);
        callbacks(&f, i + 1);
    }
    unsigned lines = expected_lines(font, 240);
    scene(&f, 272, 432, 1000, 0);
    geometry(&f, (float)lines * 10);
    callbacks(&f, 6);
    scene(&f, 272, 432, 2000, 0);
    geometry(&f, (float)lines * 10);
    eq(f.body_content.h, 2000);
    callbacks(&f, 7);
    scene(&f, 272, 332, 1000, 320);
    eq(f.column.h, solve ? 1404 : 452);
    eq(f.body.h, solve ? 1000 : 48);
    eq(f.footer.y, solve ? 1380 : 428);
    callbacks(&f, 8);
    f.outer_scroll.y = solve ? 1104 : 152;
    scene(&f, 272, 332, 1000, 320);
    eq(f.footer.y, 276);
    callbacks(&f, 9);
    cui_destroy(f.ui);
}
static bool auto_size_probe(cui_font *font, bool require_intrinsic) {
    fixture f;
    init(&f, font);
    f.auto_panel = true;
    f.solve = true;
    const float widths[] = {272, 272, 128, 128};
    const float bodies[] = {20, 1000, 20, 1000};
    bool intrinsic = true;
    for (unsigned i = 0; i < 4; ++i) {
        unsigned lines = expected_lines(font, widths[i] - 32);
        scene(&f, widths[i], 332, bodies[i], 0);
        float fixed_demand = (float)lines * 10 + 20 + 40 + 24;
        float required_height = fminf(332, 32 + fixed_demand + bodies[i]);
        eq(f.panel.h, required_height);
        intrinsic &= close_to(f.panel.h, required_height);
        callbacks(&f, i + 1);
        printf("AUTO_SIZE SOLVED: font=%s width=%.0f body=%.0f actual_panel=%.0f "
               "required_panel=%.0f\n",
               font ? "current" : "synthetic", widths[i], bodies[i], f.panel.h, required_height);
    }
    cui_destroy(f.ui);
    if (require_intrinsic)
        return intrinsic;
    assert(intrinsic);
    return true;
}
static void nested_empty_and_failure(void) {
    fixture f, child = {0};
    init(&f, NULL);
    f.solve = f.auto_panel = true;
    scene(&f, 272, 332, 0, 0);
    eq(f.panel.h, 126);
    eq(f.body.h, 0);
    callbacks(&f, 1);
    child.base = 1000;
    child.solve = child.auto_panel = true;
    child.body_height = 20;
    f.nested = &child;
    scene(&f, 272, 332, 0, 0);
    eq(child.panel.h, 166);
    eq(f.panel.h, 292);
    callbacks(&f, 2);
    callbacks(&child, 1);
    scene(&f, 128, 332, 0, 0);
    eq(child.panel.h, 176);
    eq(f.panel.h, 322);
    callbacks(&f, 3);
    callbacks(&child, 2);
    child.body_height = 1000;
    scene(&f, 272, 332, 0, 0);
    eq(child.panel.h, 200);
    eq(f.panel.h, 326);
    callbacks(&f, 4);
    callbacks(&child, 3);
    f.nested = NULL;
    f.auto_panel = false;
    scene(&f, 272, 332, 1000, 0);
    f.body_scroll.y = 794;
    f.fail_plan = f.expect_error = true;
    unsigned prepaints = f.prepaint_calls;
    scene(&f, 272, 432, 1000, 0);
    eq(f.body_scroll.y, 794);
    assert(prototype_get_stats(f.ui).commits == 0);
    assert(f.prepaint_calls == prepaints);
    cui_clear_error(f.ui);
    f.fail_plan = f.expect_error = false;
    scene(&f, 272, 432, 1000, 0);
    eq(f.body_scroll.y, 694);
    printf("NESTED_EMPTY_FAILURE PASS; resource bytes=%zu; failure commits=0\n",
           prototype_get_stats(f.ui).bytes);
    cui_destroy(f.ui);
}
static prototype_link minimal_tree_flags(cui_context *ui, cui_id base, float *outer, float *body,
                                         bool inert, bool layout_only) {
    cui_begin_box(ui, base + PANEL,
                  (cui_box){.width = 40, .height = 60, .inert = inert, .layout_only = layout_only});
    cui_begin_box(ui, base + OUTER, (cui_box){.width = 40, .height = 60, .clip = true});
    cui_begin_box(ui, base + COLUMN, (cui_box){.width = 40, .height = 60});
    cui_begin_box(ui, base + BODY_FRAME, (cui_box){.width = 40, .height = 60});
    cui_begin_absolute_box(ui, base + BODY, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
    cui_begin_box(ui, base + BODY_CONTENT, (cui_box){.width = 40, .height = 100});
    for (unsigned i = 0; i < 6; ++i)
        cui_end_box(ui);
    return (prototype_link){.panel = base + PANEL,
                            .outer = base + OUTER,
                            .column = base + COLUMN,
                            .frame = base + BODY_FRAME,
                            .viewport = base + BODY,
                            .content = base + BODY_CONTENT,
                            .requested_height = 60,
                            .max_height = 60,
                            .outer_y = outer,
                            .body_y = body};
}
static prototype_link minimal_tree(cui_context *ui, cui_id base, float *outer, float *body) {
    return minimal_tree_flags(ui, base, outer, body, false, false);
}
static unsigned destroyed_resource;
static void destroyed(void *payload) {
    (void)payload;
    ++destroyed_resource;
}
static void inactive_slots_and_collision(void) {
    cui_context *ui = cui_create(
        (cui_config){.width = 400, .height = 400, .measure = measure, .max_resources = 1});
    assert(ui);
    float outer = 0, body = 80;
    assert(cui_begin_frame(ui, 400, 400, .016f));
    assert(cui_resource_get(ui, 7, 8, 1, NULL, destroyed));
    prototype_link link = minimal_tree(ui, 1000, &outer, &body);
    assert(!prototype_register(ui, link));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    cui_end_frame(ui);
    cui_clear_error(ui); /* Failed opt-in cannot be salvaged within its frame. */
    assert(cui_begin_frame(ui, 400, 400, .016f));
    cui_end_frame(ui); /* complete omission permits normal resource collection */
    assert(cui_begin_frame(ui, 400, 400, .016f));
    assert(destroyed_resource == 1);
    link = minimal_tree_flags(ui, 1000, &outer, &body, true, false);
    assert(prototype_register(ui, link));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    eq(body, 80); /* borrowed model is not committed for inactive ancestry */
    eq(cui_bounds(ui, 1000 + BODY_CONTENT).y - cui_bounds(ui, 1000 + BODY).y, -40);
    assert(prototype_get_stats(ui).commits == 0);
    assert(cui_begin_frame(ui, 400, 400, .016f));
    link = minimal_tree_flags(ui, 1000, &outer, &body, false, true);
    assert(prototype_register(ui, link));
    assert(cui_end_frame(ui).count == 0);
    assert(cui_last_error(ui) == CUI_OK);
    eq(body, 80);
    assert(prototype_get_stats(ui).commits == 0);
    /* Actual pinned folded collision, after successful registration. */
    assert(cui_begin_frame(ui, 400, 400, .016f));
    link = minimal_tree(ui, 1000, &outer, &body);
    assert(prototype_register(ui, link));
    cui_begin_box(ui, 76894, (cui_box){.width = 1, .height = 1});
    cui_end_box(ui);
    cui_begin_box(ui, 86448, (cui_box){.width = 1, .height = 1});
    cui_end_box(ui);
    assert(cui_last_error(ui) == CUI_ERROR_DUPLICATE_ID);
    assert(!prototype_register(ui, link));
    assert(cui_end_frame(ui).count == 0);
    eq(body, 80);
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 400, 400, .016f));
    link = minimal_tree(ui, 1000, &outer, &body);
    assert(prototype_register(ui, link));
    cui_end_frame(ui);
    eq(body, 40);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    puts("INACTIVE_SLOTS_COLLISION_RECOVERY PASS");
}
static void registration_failures_and_capacity(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 400, .measure = measure});
    assert(ui);
    float offsets[PROTOTYPE_LINK_MAX + 1][2] = {{0}};
    for (unsigned trial = 0; trial < 3; ++trial) {
        assert(cui_begin_frame(ui, 400, 400, .016f));
        for (unsigned i = 0; i < PROTOTYPE_LINK_MAX + (trial == 1); ++i) {
            offsets[i][1] = 80;
            prototype_link link = minimal_tree(ui, (i + 1) * 1000, &offsets[i][0], &offsets[i][1]);
            assert(prototype_register(ui, link) == (i < PROTOTYPE_LINK_MAX));
        }
        cui_draw_list draw = cui_end_frame(ui);
        if (trial == 1) {
            assert(cui_last_error(ui) == CUI_ERROR_CAPACITY && draw.count == 0);
            for (unsigned i = 0; i <= PROTOTYPE_LINK_MAX; ++i)
                eq(offsets[i][1], 80);
            cui_clear_error(ui);
        } else {
            assert(cui_last_error(ui) == CUI_OK);
            assert(prototype_get_stats(ui).commits == PROTOTYPE_LINK_MAX);
            for (unsigned i = 0; i < PROTOTYPE_LINK_MAX; ++i)
                eq(offsets[i][1], 40);
        }
    }
    cui_destroy(ui);
    puts("CAPACITY32_REJECTION33_AND_RECOVERY PASS");
}
static bool color_is(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static bool scroll_resize_probe(bool require_current_clamp) {
    fixture f;
    init(&f, NULL);
    f.solve = true;
    scene(&f, 128, 332, 1000, 0);
    eq(f.body.h, 186);
    f.body_scroll.y = 814;
    scene(&f, 128, 332, 1000, 0);
    eq(f.body_scroll.y, 814);
    cui_draw_list draw = scene(&f, 272, 432, 1000, 0);
    eq(f.body.h, 306);
    const float expected_offset = 694;
    const bool clamped = close_to(f.body_scroll.y, expected_offset);
    eq(f.body_content.y, f.body.y - f.body_scroll.y);
    eq(f.textbox.y, f.body.y + 976 - f.body_scroll.y);
    eq(f.caret.y, f.textbox.y + 4);
    cui_rect ime = {0};
    assert(cui_text_input_rect(f.ui, &ime));
    eq(ime.y, f.caret.y);
    size_t count;
    const cui_semantic *semantics = cui_semantics(f.ui, &count);
    bool semantic = false, painted_caret = false, painted_body = false;
    for (size_t i = 0; i < count; ++i)
        if (semantics[i].id == TEXTBOX) {
            eq(semantics[i].bounds.y, f.textbox.y);
            semantic = true;
        }
    for (size_t i = 0; i < draw.count; ++i) {
        if (draw.commands[i].type == CUI_DRAW_RECT &&
            color_is(draw.commands[i].color, caret_color)) {
            eq(draw.commands[i].bounds.y, f.caret.y);
            painted_caret = true;
        }
        if (draw.commands[i].type == CUI_DRAW_RECT &&
            color_is(draw.commands[i].color, body_color)) {
            eq(draw.commands[i].bounds.y, f.body_content.y);
            painted_body = true;
        }
    }
    assert(semantic && painted_caret && painted_body);
    /* Input targeting consumes the same completed frame regions as the draw/caret. */
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                             .pointer_button = CUI_POINTER_PRIMARY,
                                             .x = f.textbox.x + 10,
                                             .y = f.textbox.y + 10}));
    assert(cui_begin_frame(f.ui, 272, 432, .016f));
    assert(cui_event_target(f.ui, 0) == TEXTBOX);
    build_tree(&f, 272, 432, 1000, 0);
    cui_end_frame(f.ui);
    assert(cui_last_error(f.ui) == CUI_OK);
    assert(prototype_get_stats(f.ui).input_unchanged);
    printf("SCROLL_RESIZE %s: actual_offset=%.0f required_offset=%.0f; draw/hit/semantic/caret "
           "agree\n",
           clamped ? "CLAMPED" : "KNOWN_GAP", f.body_scroll.y, expected_offset);
    cui_destroy(f.ui);
    if (require_current_clamp)
        return clamped;
    assert(clamped);
    return true;
}
static bool poison_armed, poison_only_second;
static float *poison_destination;
static unsigned poison_calls;
static cui_text_layout_status poisoning_geometry(void *font, cui_text_line_source source,
                                                 float size, cui_text_style style,
                                                 cui_text_geometry *out) {
    if (poison_armed && (!poison_only_second || prototype_second_pass())) {
        *poison_destination = NAN;
        ++poison_calls;
    }
    return cui_font_text_geometry_line(font, source, size, style, out);
}
static void late_measurement_offsets(cui_font *font) {
    if (!(cui_font_text_capabilities() & CUI_TEXT_CAP_LETTER_SPACING)) {
        puts("Late tracking-provider test unavailable without compiled bidi geometry provider");
        return;
    }
    for (unsigned trial = 0; trial < 2; ++trial) {
        cui_context *ui =
            cui_create((cui_config){.width = 400,
                                    .height = 400,
                                    .measure = cui_font_measure,
                                    .measure_styled = cui_font_measure_styled,
                                    .measure_user = font,
                                    .font_metrics = cui_font_get_metrics,
                                    .text_geometry = cui_font_text_geometry,
                                    .text_geometry_line = poisoning_geometry,
                                    .text_capabilities = cui_font_text_capabilities()});
        assert(ui && cui_begin_frame(ui, 400, 400, .016f));
        float outer = 37, body = 180;
        cui_begin_box(ui, 100, (cui_box){.width = 100, .height = 100});
        cui_begin_box(ui, 113, (cui_box){.width = 100, .height = 100, .clip = true});
        cui_begin_box(ui, 114, (cui_box){.width = 100, .height = -1});
        cui_begin_box(ui, 115, (cui_box){.width = 100, .height = -1});
        cui_begin_absolute_box(ui, 106, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
        cui_begin_box(ui, 107, (cui_box){.width = 100, .height = 200});
        assert(cui_text_paragraph_colored(
            ui, cui_string(title), 10, (cui_color){0, 0, 0, 255}, 10,
            (cui_text_style){.weight = CUI_TEXT_WEIGHT_NORMAL, .letter_spacing = 1}, true, NULL,
            0));
        for (unsigned i = 0; i < 6; ++i)
            cui_end_box(ui);
        prototype_link link = {.panel = 100,
                               .outer = 113,
                               .column = 114,
                               .frame = 115,
                               .viewport = 106,
                               .content = 107,
                               .requested_height = 100,
                               .max_height = 100,
                               .outer_y = &outer,
                               .body_y = &body};
        assert(prototype_register(ui, link));
        poison_destination = &body;
        poison_calls = 0;
        poison_only_second = trial != 0;
        poison_armed = true;
        assert(cui_end_frame(ui).count == 0);
        poison_armed = false;
        assert(poison_calls > 0 && cui_last_error(ui) == CUI_ERROR_LAYOUT);
        assert(isnan(body) &&
               outer == 37); /* Provider mutation is not rolled back; no partial commit. */
        prototype_stats stats = prototype_get_stats(ui);
        assert(stats.commits == 0 && stats.second == trial);
        cui_clear_error(ui);
        body = 180;
        assert(cui_begin_frame(ui, 400, 400, .016f));
        link = minimal_tree(ui, 0, &outer, &body);
        assert(prototype_register(ui, link));
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && outer == 0 && body == 40);
        cui_destroy(ui);
    }
    puts("LATE_FONT_CALLBACK_NONFINITE_BEFORE_AND_AFTER_STAGING PASS");
}
int main(int argc, char **argv) {
    if (argc > 2 && !strcmp(argv[2], "--probe-scroll-resize")) {
        return scroll_resize_probe(true) ? 0 : 3;
    }
    if (argc > 2 && !strcmp(argv[2], "--require-current-clamp")) {
        return scroll_resize_probe(true) ? 0 : 3;
    }
    if (argc > 2 && !strcmp(argv[2], "--probe-auto-size")) {
        cui_font *font = cui_font_open(argv[1]);
        assert(font);
        bool demand = true;
        bool ok = auto_size_probe(NULL, demand) && auto_size_probe(font, demand);
        cui_font_close(font);
        return ok ? 0 : 3;
    }
    assert(argc == 2);
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    inactive_slots_and_collision();
    late_measurement_offsets(font);
    registration_failures_and_capacity();
    nested_empty_and_failure();
    geometry_suite(NULL, false);
    geometry_suite(font, false);
    geometry_suite(NULL, true);
    geometry_suite(font, true);
    cui_font_close(font);
    puts("FIRST_FRAME_GEOMETRY synthetic + current font PASS; callbacks exactly once");
    return 0;
}
