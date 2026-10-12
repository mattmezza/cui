/* SPDX-License-Identifier: MIT */
#include "cui/cui_media.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned char pixels[4 * 4 * 4];
typedef struct {
    cui_image_load load;
    size_t polls;
} loader;
static cui_image_load poll(void *user, cui_image_source source) {
    loader *l = user;
    assert(source.identity);
    l->polls++;
    return l->load;
}
static void images(void) {
    cui_image_layout r;
    cui_image_geometry g = {100, 100, 0, CUI_IMAGE_CONTAIN};
    assert(cui_image_layout_resolve(g, 200, 100, &r));
    assert(r.destination.y == 25 && r.destination.w == 100 && r.destination.h == 50);
    g.fit = CUI_IMAGE_COVER;
    assert(cui_image_layout_resolve(g, 200, 100, &r) && r.source.x == 50 && r.source.w == 100 &&
           r.destination.h == 100);
    g.fit = CUI_IMAGE_FILL;
    assert(cui_image_layout_resolve(g, 200, 100, &r) && r.source.w == 200 &&
           r.destination.h == 100);
    g.fit = CUI_IMAGE_NONE;
    assert(cui_image_layout_resolve(g, 200, 100, &r) && r.source.x == 50 && r.source.w == 100);
    g = (cui_image_geometry){100, 100, 0, CUI_IMAGE_SCALE_DOWN};
    assert(cui_image_layout_resolve(g, 20, 10, &r) && r.destination.x == 40 &&
           r.destination.y == 45);
    g = (cui_image_geometry){160, 0, 16.f / 9, CUI_IMAGE_COVER};
    assert(cui_image_layout_resolve(g, 0, 0, &r) && r.height == 90);
    g = (cui_image_geometry){0, 50, 0, CUI_IMAGE_CONTAIN};
    assert(cui_image_layout_resolve(g, 200, 100, &r) && r.width == 100);
    g.width = NAN;
    assert(!cui_image_layout_resolve(g, 20, 20, &r));
    cui_image_frame frames[2] = {{pixels, 4, 4, 16, 1, .1}, {pixels, 4, 4, 16, 2, .2}};
    assert(cui_image_frame_valid(frames[0]));
    cui_image_frame invalid = frames[0];
    invalid.stride = 15;
    assert(!cui_image_frame_valid(invalid));
    loader l = {.load = {CUI_IMAGE_PENDING, NULL, 0}};
    cui_image_state state = {0};
    cui_image_source source = {.kind = CUI_IMAGE_ASSET, .identity = 1};
    cui_image_clock clock = {true, true, false, 10};
    cui_image_result result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.valid && !result.loading_visible && result.next_wake_seconds == 10.2);
    clock.now_seconds = 10.1;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(!result.loading_visible && l.polls == 2);
    clock.now_seconds = 10.3;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.loading_visible);
    l.load = (cui_image_load){CUI_IMAGE_READY, frames, 2};
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.frame == frames && result.changed && !result.loading_visible);
    size_t polls = l.polls;
    clock.now_seconds = 10.45;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.frame == frames + 1 && result.changed && l.polls == polls);
    clock.active = false;
    clock.now_seconds = 20;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.frame == frames + 1 && !result.next_wake_seconds);
    clock.active = true;
    clock.reduced_motion = true;
    clock.now_seconds = 30;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.frame == frames + 1 && !result.next_wake_seconds);
    clock.reduced_motion = false;
    clock.now_seconds = 30.2;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.frame == frames);
    source.identity = 2;
    l.load = (cui_image_load){CUI_IMAGE_FAILED, NULL, 0};
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.status == CUI_IMAGE_FAILED && result.changed);
    polls = l.polls;
    l.load = (cui_image_load){CUI_IMAGE_READY, frames, 2};
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.status == CUI_IMAGE_FAILED && l.polls == polls);
    cui_image_state_reset(&state);
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.status == CUI_IMAGE_READY);
    source.kind = CUI_IMAGE_CUSTOM;
    polls = l.polls;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.status == CUI_IMAGE_READY && l.polls == polls + 1);
    source.kind = CUI_IMAGE_ASSET;
    source.identity = 3;
    clock.identified = false;
    l.load = (cui_image_load){CUI_IMAGE_PENDING, NULL, 0};
    result = cui_image_update(&state, source, poll, &l, clock);
    clock.now_seconds = 40;
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(!result.loading_visible && !result.next_wake_seconds);
    source.identity = 4;
    l.load = (cui_image_load){CUI_IMAGE_READY, &invalid, 1};
    result = cui_image_update(&state, source, poll, &l, clock);
    assert(result.status == CUI_IMAGE_FAILED);
}
static void attachments(void) {
    const float heights[] = {56, 40, 48, 64}, widths[] = {232, 176, 200, 272},
                media[] = {38, 28, 32, 44};
    cui_attachment_model m = cui_attachment_model_default();
    m.media = m.content = true;
    cui_attachment_plan p;
    for (int i = 0; i < 4; i++) {
        m.size = (cui_media_size)i;
        assert(cui_base_attachment(m, &p) && p.height == heights[i] && p.width == widths[i] &&
               p.media_width == media[i]);
    }
    m.size = CUI_MEDIA_MEDIUM;
    m.identified = m.remove = m.retry = true;
    m.progress_known = true;
    m.progress = 150;
    for (int i = 0; i < 5; i++) {
        m.status = (cui_attachment_status)i;
        assert(cui_base_attachment(m, &p));
        assert(p.overhang == 6 && !p.remove_visible);
        assert(p.dashed == (i == CUI_ATTACHMENT_PENDING));
        assert(p.progress_ring == (i == CUI_ATTACHMENT_UPLOADING));
        assert(p.progress_bar == (i == CUI_ATTACHMENT_UPLOADING));
        assert(p.append_percentage == (i == CUI_ATTACHMENT_UPLOADING));
        assert(p.spinner == (i == CUI_ATTACHMENT_PROCESSING));
        assert(p.retry_description == (i == CUI_ATTACHMENT_FAILED));
        assert(p.progress == 100);
    }
    m.image = m.touch = true;
    m.status = CUI_ATTACHMENT_FAILED;
    assert(cui_base_attachment(m, &p) && p.remove_visible && p.scrim == .55f && !p.rejection);
    m.identified = false;
    assert(cui_base_attachment(m, &p) && !p.retry_media && !p.retry_description && p.rejection &&
           !p.remove_visible);
    m.status = CUI_ATTACHMENT_PROCESSING;
    m.title_status_override = m.description_status_override = true;
    m.title_status = CUI_ATTACHMENT_COMPLETE;
    m.description_status = CUI_ATTACHMENT_FAILED;
    assert(cui_base_attachment(m, &p) && !p.title_loading && p.description_failed &&
           p.scrim == .35f);
    m.vertical = true;
    m.content = false;
    assert(cui_base_attachment(m, &p) && p.flush && p.width == 56 && p.height == 56 &&
           p.media_width == 54 && !p.progress_bar);
    m.content = true;
    m.media_aspect = 2;
    assert(cui_base_attachment(m, &p) && !p.flush && p.width == 120 && p.media_height == 52);
    m.progress = NAN;
    assert(!cui_base_attachment(m, &p));
    assert(!cui_attachment_metrics_resolve(CUI_MEDIA_CUSTOM, 16, NAN, &p.metrics));
    assert(cui_attachment_metrics_resolve(CUI_MEDIA_CUSTOM, 16, 8, &p.metrics) &&
           p.metrics.height == 28);
}
#ifndef CUI_MEDIA_BASE_ONLY
#include "cui/cui_components.h"
static void placeholder(cui_context *ui, cui_id id, void *user, float width, float height) {
    (*(unsigned *)user)++;
    cui_begin_box(ui, id, (cui_box){.width = width, .height = height});
    cui_text(ui, cui_string("Unavailable"), 12, (cui_color){255, 255, 255, 255});
    cui_end_box(ui);
}
static void image_drawing(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 200});
    assert(ui);
    cui_image_state state = {0};
    cui_image_frame frame = {pixels, 4, 2, 16, 7, .1};
    loader l = {.load = {CUI_IMAGE_READY, &frame, 1}};
    cui_image_source source = {.kind = CUI_IMAGE_ASSET, .identity = 1};
    cui_image_options o = cui_image_options_default();
    o.geometry = (cui_image_geometry){100, 100, 0, CUI_IMAGE_COVER};
    o.corners = (cui_corner_radii){1, 2, 3, 4};
    o.opacity = .5f;
    o.grayscale = true;
    o.description = cui_string("Colored photograph");
    assert(cui_begin_frame(ui, 200, 200, .016f));
    cui_image_result result = cui_image(ui, 10, &state, source, poll, &l, o);
    assert(result.valid && result.frame);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    size_t count = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_IMAGE) {
            cui_draw_command c = list.commands[i];
            count++;
            assert(c.bitmap.bitmap.pixels == pixels && c.bitmap.bitmap.revision == 7);
            assert(c.bitmap.source.x == 1 && c.bitmap.source.w == 2 && c.bitmap.source.h == 2);
            assert(c.bitmap.opacity == .5f && c.bitmap.grayscale);
            assert(c.custom_corners && c.corner_radii.top_left == 1 &&
                   c.corner_radii.bottom_right == 4);
            assert(c.bounds.w == 100 && c.bounds.h == 100);
        }
    assert(count == 1);
    size_t semantic_count;
    const cui_semantic *sem = cui_semantics(ui, &semantic_count);
    assert(semantic_count == 1 && sem[0].role == CUI_ROLE_LABEL && !sem[0].details.focusable);
    source.identity = 2;
    l.load = (cui_image_load){CUI_IMAGE_PENDING, NULL, 0};
    unsigned calls = 0;
    o.loading = o.fallback = placeholder;
    o.user = &calls;
    o.clock.now_seconds = 1;
    assert(cui_begin_frame(ui, 200, 200, .016f));
    result = cui_image(ui, 10, &state, source, poll, &l, o);
    cui_end_frame(ui);
    assert(result.valid && calls == 0);
    o.clock.now_seconds = 1.3;
    assert(cui_begin_frame(ui, 200, 200, .016f));
    result = cui_image(ui, 10, &state, source, poll, &l, o);
    cui_end_frame(ui);
    assert(result.loading_visible && calls == 1 && cui_bounds(ui, 10).h == 100);
    l.load = (cui_image_load){CUI_IMAGE_FAILED, NULL, 0};
    assert(cui_begin_frame(ui, 200, 200, .016f));
    result = cui_image(ui, 10, &state, source, poll, &l, o);
    cui_end_frame(ui);
    assert(result.status == CUI_IMAGE_FAILED && calls == 2);
    o.geometry.width = NAN;
    assert(cui_begin_frame(ui, 200, 200, .016f));
    result = cui_image(ui, 10, &state, source, poll, &l, o);
    cui_end_frame(ui);
    assert(!result.valid && calls == 2);
    cui_destroy(ui);
}
typedef struct {
    unsigned clicked, removed, retried, action;
    cui_id action_id;
} commands;
static void clicked(void *user, cui_id id) {
    assert(id == 20);
    ((commands *)user)->clicked++;
}
static void removed(void *user, cui_id id) {
    assert(id == 20);
    ((commands *)user)->removed++;
}
static void retried(void *user, cui_id id) {
    assert(id == 20);
    ((commands *)user)->retried++;
}
static void actions(cui_context *ui, cui_id id, void *user, float width, float height) {
    (void)height;
    commands *c = user;
    c->action_id = id;
    cui_interaction interaction =
        cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("Download"), false, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = width - 8, .height = 24});
    cui_text(ui, cui_string("↓"), 12, (cui_color){255, 255, 255, 255});
    cui_end_box(ui);
    if (interaction.activated)
        c->action++;
}
static cui_attachment_result attachment_scene(cui_context *ui, const cui_theme *t,
                                              cui_attachment_state *state,
                                              cui_attachment_options o) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_attachment_result r = cui_attachment(ui, 20, t, state, o);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(r.valid);
    return r;
}
static void click_at(cui_context *ui, cui_rect b) {
    assert(b.w > 0 && b.h > 0);
    cui_event e = {.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w / 2, .y = b.y + b.h / 2};
    assert(cui_queue_event(ui, e));
    e.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, e));
}
static void attachment_events(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; i++) {
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
        assert(ui);
        cui_attachment_state state = {0};
        commands c = {0};
        cui_attachment_options o = cui_attachment_options_default();
        cui_attachment_child children[] = {
            {.kind = CUI_ATTACHMENT_TITLE, .text = {"Report.pdf", 10}},
            {.kind = CUI_ATTACHMENT_DESCRIPTION, .text = {"PDF document", 12}}};
        o.children = children;
        o.child_count = 2;
        o.name = cui_string("Report attachment");
        o.model.identified = true;
        o.model.touch = true;
        o.on_click = clicked;
        o.on_remove = removed;
        o.on_retry = retried;
        o.action_user = &c;
        o.actions = actions;
        o.actions_user = &c;
        o.actions_width = 24;
        attachment_scene(ui, &themes[i], &state, o);
        attachment_scene(ui, &themes[i], &state, o);
        assert(fabsf(cui_bounds(ui, 20).w - 232 * themes[i].font_size / 14) < 1);
        cui_focus(ui, c.action_id);
        click_at(ui, (cui_rect){state.card.x + 10, state.card.y + 10, 10, 10});
        cui_attachment_result r = attachment_scene(ui, &themes[i], &state, o);
        assert(r.clicked && c.clicked == 1 && cui_focused(ui) == c.action_id);
        click_at(ui, cui_bounds(ui, c.action_id));
        r = attachment_scene(ui, &themes[i], &state, o);
        assert(!r.clicked && c.action == 1 && c.clicked == 1);
        cui_rect action_bounds = cui_bounds(ui, c.action_id);
        click_at(ui, (cui_rect){action_bounds.x + action_bounds.w + 1, action_bounds.y, 4,
                                action_bounds.h});
        r = attachment_scene(ui, &themes[i], &state, o);
        assert(!r.clicked && c.action == 1 && c.clicked == 1);
        cui_focus(ui, cui_attachment_remove_id(20));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        r = attachment_scene(ui, &themes[i], &state, o);
        assert(r.removed && c.removed == 1);
        cui_rect remove_bounds = cui_bounds(ui, cui_attachment_remove_id(20));
        o.model.touch = false;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                               .x = remove_bounds.x + 10,
                                               .y = remove_bounds.y + 10}));
        r = attachment_scene(ui, &themes[i], &state, o);
        assert(r.plan.remove_visible);
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 350, .y = 250}));
        r = attachment_scene(ui, &themes[i], &state, o);
        assert(!r.plan.remove_visible);
        o.model.touch = true;
        o.model.status = CUI_ATTACHMENT_FAILED;
        o.model.media = true;
        attachment_scene(ui, &themes[i], &state, o);
        attachment_scene(ui, &themes[i], &state, o);
        cui_focus(ui, cui_attachment_retry_id(20));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        r = attachment_scene(ui, &themes[i], &state, o);
        assert(r.retried && c.retried == 1 && !r.clicked);
        o.model.status = CUI_ATTACHMENT_COMPLETE;
        r = attachment_scene(ui, &themes[i], &state, o);
        assert(!r.plan.retry_media && !r.plan.retry_description);
        o.actions_width = NAN;
        assert(cui_begin_frame(ui, 400, 300, .016f));
        r = cui_attachment(ui, 20, &themes[i], &state, o);
        cui_end_frame(ui);
        assert(!r.valid);
        cui_destroy(ui);
    }
}
static void attachment_states(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++)
        for (unsigned vertical = 0; vertical < 2; vertical++) {
            cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
            assert(ui);
            cui_attachment_state state = {0};
            cui_image_state image = {0};
            cui_image_frame frame = {pixels, 4, 4, 16, 9, .1};
            loader l = {.load = {CUI_IMAGE_READY, &frame, 1}};
            cui_attachment_child children[] = {
                {.kind = CUI_ATTACHMENT_TITLE, .text = {"Photo.png", 9}},
                {.kind = CUI_ATTACHMENT_DESCRIPTION, .text = {"Uploading", 9}}};
            cui_attachment_options o = cui_attachment_options_default();
            o.children = children;
            o.child_count = 2;
            o.model.vertical = vertical != 0;
            o.model.identified = true;
            o.model.progress_known = true;
            o.model.progress = 62;
            o.image_state = &image;
            o.image_loader = poll;
            o.image_user = &l;
            o.image_source = (cui_image_source){.kind = CUI_IMAGE_ASSET, .identity = 1};
            o.reduced_motion = true;
            for (unsigned status = 0; status < 5; status++) {
                o.model.status = (cui_attachment_status)status;
                attachment_scene(ui, &themes[theme], &state, o);
                assert(cui_begin_frame(ui, 400, 300, .016f));
                cui_attachment_result r = cui_attachment(ui, 20, &themes[theme], &state, o);
                cui_draw_list list = cui_end_frame(ui);
                assert(r.valid && cui_last_error(ui) == CUI_OK);
                size_t images = 0;
                size_t icons = 0;
                bool percentage = false, scrim = false;
                for (size_t i = 0; i < list.count; i++) {
                    cui_draw_command c = list.commands[i];
                    if (c.type == CUI_DRAW_IMAGE) {
                        if (c.bitmap.bitmap.pixels == pixels)
                            images++;
                        else {
                            icons++;
                            unsigned coverage = 0;
                            for (uint32_t y = 0; y < c.bitmap.bitmap.height; ++y)
                                for (uint32_t x = 0; x < c.bitmap.bitmap.width; ++x)
                                    coverage +=
                                        c.bitmap.bitmap
                                            .pixels[y * c.bitmap.bitmap.stride + x * 4 + 3] != 0;
                            assert(coverage);
                            assert(c.bounds.w == 10 || c.bounds.w == 12 || r.plan.rejection);
                        }
                    }
                    if (c.type == CUI_DRAW_TEXT && c.text.length == 6 &&
                        !memcmp(c.text.data, "· 62%", 6))
                        percentage = true;
                    if (c.type == CUI_DRAW_RECT && c.color.r == 0 && c.color.g == 0 &&
                        c.color.b == 0 && c.color.a == (unsigned char)(r.plan.scrim * 255) &&
                        r.plan.scrim > 0)
                        scrim = true;
                }
                assert(images == 1);
                assert(icons == (size_t)r.plan.remove_visible + (size_t)r.plan.retry_media +
                                    (size_t)r.plan.rejection);
                assert(percentage == (status == CUI_ATTACHMENT_UPLOADING));
                assert(scrim ==
                       (status == CUI_ATTACHMENT_UPLOADING || status == CUI_ATTACHMENT_PROCESSING ||
                        status == CUI_ATTACHMENT_FAILED));
                assert(r.plan.progress_bar == (status == CUI_ATTACHMENT_UPLOADING && !vertical));
            }
            cui_destroy(ui);
        }
}
static void group_children(cui_context *ui, cui_id id, void *user, float width, float height) {
    (void)width;
    (void)height;
    size_t count = *(size_t *)user;
    for (size_t i = 0; i < count; i++) {
        cui_begin_box(ui, id + i,
                      (cui_box){.width = 100, .height = 50, .background = {40, 60, 80, 255}});
        cui_end_box(ui);
    }
}
static cui_attachment_group_result group_scene(cui_context *ui, cui_attachment_group_state *s,
                                               cui_attachment_group_options o) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_attachment_group_result r = cui_attachment_group(ui, 30, s, o);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(r.valid);
    return r;
}
static void group_events(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_attachment_group_state state = {0};
    size_t count = 5;
    cui_attachment_group_options o = cui_attachment_group_options_default();
    o.width = 300;
    o.children = group_children;
    o.user = &count;
    o.edge_fade = true;
    o.fade_color = (cui_color){10, 20, 30, 255};
    o.reduced_motion = true;
    group_scene(ui, &state, o);
    cui_attachment_group_result r = group_scene(ui, &state, o);
    assert(r.maximum == 248 && r.leading_opacity == 0 && r.trailing_opacity == 1);
    assert(state.content.w == 548 && state.viewport.h == 58);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 100, .y = 20, .dx = -2, .dy = 1}));
    r = group_scene(ui, &state, o);
    assert(state.scroll.x == 96 && r.leading_opacity == 1 && r.trailing_opacity == 1);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 100, .y = 20, .dx = -2, .dy = -2}));
    group_scene(ui, &state, o);
    assert(state.scroll.x == 96);
    cui_scroll_state external = {.x = 248};
    o.external_scroll = &external;
    r = group_scene(ui, &state, o);
    assert(r.trailing_opacity == 0 && r.leading_opacity == 1);
    assert(state.scroll.x == 96);
    count = 1;
    group_scene(ui, &state, o);
    r = group_scene(ui, &state, o);
    assert(r.maximum == 0 && external.x == 0 && r.leading_opacity == 0 && r.trailing_opacity == 0);
    count = 5;
    o.reduced_motion = false;
    o.now_seconds = 1;
    group_scene(ui, &state, o);
    r = group_scene(ui, &state, o);
    assert(r.trailing_opacity == 0 && r.next_wake_seconds > 1);
    o.now_seconds = 1.1;
    r = group_scene(ui, &state, o);
    assert(fabsf(r.trailing_opacity - .875f) < .001f);
    o.now_seconds = 1.3;
    r = group_scene(ui, &state, o);
    assert(r.trailing_opacity == 1 && !r.next_wake_seconds);
    r = group_scene(ui, &state, o);
    assert(!r.changed && !cui_needs_redraw(ui));
    cui_destroy(ui);
}
#endif
int main(void) {
    images();
    attachments();
#ifndef CUI_MEDIA_BASE_ONLY
    image_drawing();
    attachment_events();
    attachment_states();
    group_events();
#endif
    return 0;
}
