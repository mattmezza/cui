/* SPDX-License-Identifier: MIT */
#include "cui/cui_status.h"
#ifndef CUI_STATUS_BASE_ONLY
#include "cui/cui_components.h"
#endif
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
#ifndef CUI_STATUS_BASE_ONLY
static bool equal(cui_color a, cui_color b) {
    return memcmp(&a, &b, sizeof(a)) == 0;
}
#endif
static void shimmer_models(void) {
    cui_shimmer_options o = cui_shimmer_options_default();
    cui_shimmer_plan p;
    o.now_seconds = .5;
    assert(cui_base_shimmer(o, &p) && p.phase == .25f && p.animated && p.next_wake_seconds > .5);
    o.reverse = true;
    assert(cui_base_shimmer(o, &p) && p.phase == .75f);
    o.once = true;
    o.now_seconds = 3;
    assert(cui_base_shimmer(o, &p) && p.phase == 0 && !p.animated && !p.next_wake_seconds);
    o.once = false;
    o.reduced_motion = true;
    assert(cui_base_shimmer(o, &p) && !p.animated && !p.next_wake_seconds);
    o.now_seconds = NAN;
    p.phase = 123;
    assert(!cui_base_shimmer(o, &p) && p.phase == 123);
    o.now_seconds = 0;
    o.duration = 0;
    o.reduced_motion = false;
    assert(cui_base_shimmer(o, &p) && p.animated);
    o.spread = -1;
    assert(!cui_base_shimmer(o, &p));
}
static void marker_models(void) {
    cui_marker_options o = cui_marker_options_default();
    cui_marker_plan p;
    cui_marker_child text = {.kind = CUI_MARKER_TEXT, .text = {"Thinking", 8}};
    o.loading = true;
    o.loading_style = CUI_MARKER_SHIMMER;
    o.now_seconds = .5;
    assert(cui_base_marker(o, &text, 1, &p) && p.animated && !p.spinner && p.phase == .25f &&
           p.next_wake_seconds > .5);
    o.reverse = true;
    assert(cui_base_marker(o, &text, 1, &p) && p.phase == .75f);
    o.reduced_motion = true;
    assert(cui_base_marker(o, &text, 1, &p) && !p.animated && p.rich_opacity == 1 &&
           p.next_wake_seconds == 0);
    o.reduced_motion = false;
    o.once = true;
    o.now_seconds = 3;
    assert(cui_base_marker(o, &text, 1, &p) && !p.animated && p.phase == 0);
    o.variant = CUI_MARKER_SEPARATOR;
    o.alignment = CUI_MARKER_ALIGN_END;
    assert(cui_base_marker(o, &text, 1, &p) && p.alignment == CUI_MARKER_ALIGN_END);
    assert(!cui_base_marker(o, &text, CUI_MARKER_MAX_CHILDREN + 1, &p));
}
#ifndef CUI_STATUS_BASE_ONLY
static void shimmer_render(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 300});
    assert(ui);
    cui_theme t = cui_theme_dark();
    cui_shimmer_options o = cui_shimmer_options_default();
    o.now_seconds = 1;
    o.font_size = 19;
    o.max_width = 40;
    o.custom_color = o.custom_highlight = true;
    o.color = (cui_color){23, 45, 67, 255};
    o.highlight = (cui_color){201, 211, 221, 128};
    cui_shimmer_state state = {0};
    for (int frame = 0; frame < 3; frame++) {
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_shimmer_text(ui, 81, &t, &state, cui_string("Long processing label"), o));
        cui_draw_list list = cui_end_frame(ui);
        size_t count;
        const cui_semantic *nodes = cui_semantics(ui, &count);
        assert(count == 1 && nodes[0].role == CUI_ROLE_LABEL && nodes[0].bounds.w == 40);
        assert(state.text_bounds.w > 40);
        if (frame == 2) {
            size_t texts = 0;
            int clips = 0;
            for (size_t i = 0; i < list.count; i++) {
                const cui_draw_command *c = &list.commands[i];
                if (c->type == CUI_DRAW_CLIP_PUSH)
                    clips++;
                if (c->type == CUI_DRAW_CLIP_POP)
                    clips--;
                assert(clips >= 0);
                if (c->type == CUI_DRAW_TEXT) {
                    assert(c->font_size == 19);
                    assert(equal(c->color, o.color) ||
                           (c->color.r == 201 && c->color.a >= 8 && c->color.a <= 10));
                    texts++;
                }
            }
            assert(texts > 1 && !clips);
        }
    }
    o.reduced_motion = true;
    for (int frame = 0; frame < 3; frame++) {
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_shimmer_text(ui, 81, &t, &state, cui_string("Long processing label"), o));
        cui_draw_list list = cui_end_frame(ui);
        size_t texts = 0;
        for (size_t i = 0; i < list.count; i++)
            texts += list.commands[i].type == CUI_DRAW_TEXT;
        assert(texts == 1);
    }
    assert(!cui_needs_redraw(ui) && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void marker_highlight_colors(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 300});
    assert(ui);
    for (int mode = 0; mode < 3; mode++) {
        cui_theme t = cui_theme_light();
        t.background = (cui_color){255, 255, 255, 255};
        t.text = (cui_color){0, 0, 0, 255};
        t.muted = mode == 1 ? t.background : t.text;
        cui_marker_state state = {0};
        cui_marker_options o = cui_marker_options_default();
        o.loading = true;
        o.loading_style = CUI_MARKER_SHIMMER;
        o.now_seconds = 1;
        o.custom_highlight = mode == 2;
        o.highlight = (cui_color){17, 99, 201, 255};
        cui_marker_child child = {.kind = CUI_MARKER_TEXT, .text = {"Wait", 4}};
        for (int frame = 0; frame < 2; frame++) {
            assert(cui_begin_frame(ui, 500, 300, .016f));
            assert(cui_marker(ui, 95, &t, &state, &child, 1, o));
            cui_draw_list draw = cui_end_frame(ui);
            if (frame == 1) {
                size_t overlays = 0;
                for (size_t i = 0; i < draw.count; i++)
                    if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].color.a < 255) {
                        cui_color c = draw.commands[i].color;
                        if (mode == 2)
                            assert(c.r == 17 && c.g == 99 && c.b == 201);
                        else if (mode == 1)
                            assert(c.r >= 21 && c.r <= 23 && c.g == c.r && c.b == c.r);
                        else
                            assert(c.r >= 189 && c.r <= 191 && c.g == c.r && c.b == c.r);
                        overlays++;
                    }
                assert(overlays == 12);
            }
        }
    }
    cui_destroy(ui);
}

static void marker_render(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 300});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_marker_child children[] = {{.kind = CUI_MARKER_TEXT, .text = {"Thinking", 8}},
                                   {.kind = CUI_MARKER_DIRECT, .text = {"!", 1}, .width = 10}};
    for (size_t theme = 0; theme < 4; theme++) {
        cui_marker_state state = {0};
        cui_marker_options o = cui_marker_options_default();
        o.loading = true;
        o.loading_style = CUI_MARKER_SHIMMER;
        o.now_seconds = 1;
        o.role = CUI_ROLE_STATUS;
        o.name = cui_string("Progress");
        for (int frame = 0; frame < 2; frame++) {
            assert(cui_begin_frame(ui, 500, 300, .016f));
            assert(cui_marker(ui, 91, &themes[theme], &state, children, 2, o));
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            if (frame == 1) {
                size_t clips = 0, text = 0;
                for (size_t i = 0; i < list.count; i++) {
                    clips += list.commands[i].type == CUI_DRAW_CLIP_PUSH;
                    text += list.commands[i].type == CUI_DRAW_TEXT;
                    if (list.commands[i].type == CUI_DRAW_CLIP_PUSH) {
                        assert(list.commands[i].bounds.w > 0 &&
                               list.commands[i].bounds.w <= state.text_bounds[0].w);
                    }
                }
                assert(clips == 12 && text == 14);
            }
        }
        o.reduced_motion = true;
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_marker(ui, 91, &themes[theme], &state, children, 2, o));
        cui_draw_list list = cui_end_frame(ui);
        for (size_t i = 0; i < list.count; i++)
            assert(list.commands[i].type != CUI_DRAW_CLIP_PUSH);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        assert(n == 1 && sem[0].role == CUI_ROLE_STATUS);
        o.variant = CUI_MARKER_BORDER;
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_marker(ui, 91, &themes[theme], &state, children, 2, o));
        list = cui_end_frame(ui);
        bool border = false;
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].type == CUI_DRAW_BORDER) {
                assert(list.commands[i].custom_border_edges &&
                       list.commands[i].border_edges.bottom == 1 &&
                       list.commands[i].border_edges.top == 0);
                border = true;
            }
        assert(border);
    }
    cui_marker_state state = {0};
    cui_marker_options o = cui_marker_options_default();
    cui_marker_child textchild = {.kind = CUI_MARKER_TEXT, .text = {"Center", 6}};
    for (int alignment = CUI_MARKER_ALIGN_START; alignment <= CUI_MARKER_ALIGN_END; alignment++) {
        o.variant = CUI_MARKER_SEPARATOR;
        o.alignment = (cui_marker_alignment)alignment;
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
        cui_draw_list list = cui_end_frame(ui);
        float x = -1, w = 0;
        size_t lines = 0;
        for (size_t i = 0; i < list.count; i++) {
            if (list.commands[i].type == CUI_DRAW_TEXT) {
                x = list.commands[i].bounds.x;
                w = list.commands[i].bounds.w;
            }
            if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].bounds.h == 1)
                lines++;
        }
        assert(lines == (alignment == CUI_MARKER_ALIGN_CENTER ? 2u : 1u));
        float expected = alignment == CUI_MARKER_ALIGN_START    ? 0
                         : alignment == CUI_MARKER_ALIGN_CENTER ? (o.width - w) / 2
                                                                : o.width - w;
        assert(fabsf(x - expected) < 1);
    }
    assert(cui_begin_frame(ui, 500, 300, .016f));
    assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
    cui_end_frame(ui);
    assert(!cui_needs_redraw(ui));
    o.loading = true;
    o.loading_style = CUI_MARKER_SHIMMER;
    o.now_seconds = .4;
    assert(cui_begin_frame(ui, 500, 300, .016f));
    assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
    cui_end_frame(ui);
    assert(cui_needs_redraw(ui));
    assert(cui_begin_frame(ui, 500, 300, .016f));
    assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
    cui_end_frame(ui);
    assert(!cui_needs_redraw(ui));
    cui_destroy(ui);
}
#endif
#ifndef CUI_STATUS_BASE_ONLY
static void notification_render(void) {
    cui_context *ui = cui_create((cui_config){.width = 900, .height = 700});
    assert(ui);
    cui_notification_entry entries[16];
    cui_notification_state s;
    cui_notification_settings settings = cui_notification_settings_default();
    settings.reduced_motion = true;
    assert(cui_notification_init(&s, entries, 16, settings, (cui_notification_host){0}));
    cui_notification n = cui_notification_default();
    n.title = cui_string("Saved");
    n.message = cui_string("Changes saved");
    n.autohide = false;
    n.height = 80;
    n.custom_placement = true;
    for (int a = 0; a < 8; a++) {
        n.id = (cui_notification_id){1, (uint64_t)a + 1};
        n.placement = (cui_notification_anchor)a;
        assert(cui_notification_push(&s, n, 0));
    }
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++) {
        assert(cui_begin_frame(ui, 900, 700, .016f));
        cui_notification_advance r =
            cui_notifications(ui, 500, &themes[theme], &s, (cui_rect){0, 0, 900, 700}, .4 + theme);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && r.next_wake_seconds == 0);
        size_t text = 0;
        for (size_t i = 0; i < list.count; i++)
            text += list.commands[i].type == CUI_DRAW_TEXT;
        assert(text == 16);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 16);
        size_t alerts = 0;
        for (size_t i = 0; i < count; i++)
            alerts += sem[i].role == CUI_ROLE_ALERT;
        assert(alerts == 8);
        assert(entries[0].bounds.x == 16 && entries[0].bounds.y == 48);
        assert(entries[1].bounds.x == 259);
        assert(entries[2].bounds.x == 502);
        assert(entries[3].bounds.y == 604);
        assert(entries[6].bounds.y == 310);
    }
    assert(cui_begin_frame(ui, 900, 700, .016f));
    cui_notification_advance idle =
        cui_notifications(ui, 500, &themes[3], &s, (cui_rect){0, 0, 900, 700}, 4);
    cui_end_frame(ui);
    assert(idle.next_wake_seconds == 0 && !cui_needs_redraw(ui));
    cui_notification_clear(&s, 5);
    assert(cui_begin_frame(ui, 900, 700, .016f));
    cui_notifications(ui, 500, &themes[0], &s, (cui_rect){0, 0, 900, 700}, 5.2);
    cui_end_frame(ui);
    assert(s.count == 0 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
#endif
typedef struct {
    int sent, retracted, activated, clicked, closed;
    bool reject;
} notification_counts;
static bool send_notification(void *user, const cui_notification *n) {
    notification_counts *c = user;
    c->sent++;
    assert(n->id.type || n->id.key);
    return !c->reject;
}
static void retract_notification(void *user, cui_notification_id id) {
    (void)id;
    ((notification_counts *)user)->retracted++;
}
static void activate_notification(void *user, cui_notification_id id) {
    (void)id;
    ((notification_counts *)user)->activated++;
}
static void click_notification(void *user, cui_notification_id id) {
    (void)id;
    ((notification_counts *)user)->clicked++;
}
static void close_notification(void *user, cui_notification_id id) {
    (void)id;
    ((notification_counts *)user)->closed++;
}
#ifndef CUI_STATUS_BASE_ONLY
static cui_notification_advance notification_frame(cui_context *ui, cui_notification_state *s,
                                                   double now) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_notification_advance r = cui_notifications(ui, 900, &t, s, (cui_rect){0, 0, 600, 400}, now);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return r;
}
static void pointer_notification(cui_context *ui, cui_rect b, cui_pointer_button button) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = b.x + b.w / 2,
                                           .y = b.y + b.h / 2,
                                           .pointer_button = button}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = b.x + b.w / 2,
                                           .y = b.y + b.h / 2,
                                           .pointer_button = button}));
}
static void notification_custom_content(cui_context *ui, cui_id id, void *user, float font,
                                        cui_color color, float opacity) {
    (void)font;
    (void)color;
    (void)opacity;
    cui_button(ui, id, user, cui_string("Custom action"), false);
}
static void notification_sized_content(cui_context *ui, cui_id id, void *user, float font,
                                       cui_color color, float opacity) {
    (void)font;
    (void)opacity;
    cui_begin_box(ui, id, (cui_box){.width = 30, .height = *(float *)user, .background = color});
    cui_end_box(ui);
}
static void notification_theme_tokens(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    assert(ui);
    cui_theme theme = cui_theme_light();
    theme.popover = (cui_color){11, 77, 133, 255};
    theme.radius_lg = 13;
    cui_notification_entry entries[1];
    cui_notification_state state;
    cui_notification_settings settings = cui_notification_settings_default();
    settings.reduced_motion = true;
    assert(cui_notification_init(&state, entries, 1, settings, (cui_notification_host){0}));
    cui_notification n = cui_notification_default();
    n.id.key = 1;
    n.message = cui_string("Independent popover styling");
    n.autohide = false;
    assert(cui_notification_push(&state, n, 0));
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_notifications(ui, 900, &theme, &state, (cui_rect){0, 0, 600, 400}, .4);
    cui_draw_list list = cui_end_frame(ui);
    bool found = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_RECT &&
            equal(list.commands[i].color, theme.popover)) {
            assert(list.commands[i].radius == 13);
            found = true;
        }
    assert(found && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static void notification_action_fades(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    assert(ui);
    cui_notification_entry entries[1];
    cui_notification_state state;
    assert(cui_notification_init(&state, entries, 1, cui_notification_settings_default(),
                                 (cui_notification_host){0}));
    cui_notification n = cui_notification_default();
    n.id.key = 1;
    n.message = cui_string("Entering");
    n.action_label = cui_string("Act");
    n.height = 80;
    n.autohide = false;
    assert(cui_notification_push(&state, n, 0));
    cui_theme theme = cui_theme_light();
    theme.buttons[CUI_THEME_BUTTON_PRIMARY].foreground = (cui_color){19, 99, 199, 255};
    for (int frame = 0; frame < 3; frame++) {
        assert(cui_begin_frame(ui, 600, 400, .016f));
        cui_notifications(ui, 900, &theme, &state, (cui_rect){0, 0, 600, 400}, frame * .1);
        cui_draw_list list = cui_end_frame(ui);
        if (frame == 2) {
            bool found = false;
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].text.length == 3 &&
                    memcmp(list.commands[i].text.data, "Act", 3) == 0) {
                    cui_color c = list.commands[i].color;
                    assert(c.r == 19 && c.g == 99 && c.b == 199 && c.a > 0 && c.a < 255);
                    found = true;
                }
            assert(found);
        }
    }
    cui_destroy(ui);
}

static void notification_autoheight(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    assert(ui);
    cui_notification_entry entries[1];
    cui_notification_state s;
    cui_notification_settings settings = cui_notification_settings_default();
    settings.reduced_motion = true;
    assert(cui_notification_init(&s, entries, 1, settings, (cui_notification_host){0}));
    float height = 10;
    cui_notification n = cui_notification_default();
    n.id = (cui_notification_id){4, 1};
    n.autohide = false;
    n.content = notification_sized_content;
    n.content_user = &height;
    assert(cui_notification_push(&s, n, 0));
    notification_frame(ui, &s, .4);
    notification_frame(ui, &s, .5);
    float before = entries[0].bounds.h;
    height = 70;
    notification_frame(ui, &s, .6);
    assert(entries[0].bounds.h > before && cui_needs_redraw(ui));
    float grown = entries[0].bounds.h;
    notification_frame(ui, &s, .7);
    notification_frame(ui, &s, .8);
    assert(!cui_needs_redraw(ui));
    height = 5;
    notification_frame(ui, &s, .9);
    assert(entries[0].bounds.h < grown && cui_needs_redraw(ui));
    notification_frame(ui, &s, 1);
    notification_frame(ui, &s, 1.1);
    assert(!cui_needs_redraw(ui));
    cui_destroy(ui);
}
static void notification_interactions(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    assert(ui);
    cui_notification_entry entries[4];
    cui_notification_state s;
    notification_counts counts = {0};
    cui_notification_settings settings = cui_notification_settings_default();
    settings.reduced_motion = true;
    assert(cui_notification_init(&s, entries, 4, settings, (cui_notification_host){0}));
    cui_notification n = cui_notification_default();
    n.id = (cui_notification_id){2, 1};
    n.message = cui_string("Click details");
    n.action_label = cui_string("Retry");
    n.height = 80;
    n.on_click = click_notification;
    n.on_close = close_notification;
    n.on_action = click_notification;
    n.user = &counts;
    assert(cui_notification_push(&s, n, 0));
    notification_frame(ui, &s, .4);
    cui_rect card = entries[0].bounds;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = card.x + 10, .y = card.y + 10}));
    notification_frame(ui, &s, 2);
    assert(entries[0].remaining == 5);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    cui_id action = 0, body = 0;
    for (size_t i = 0; i < count; i++) {
        if (sem[i].name.length == 5 && !memcmp(sem[i].name.data, "Retry", 5))
            action = sem[i].id;
        if (sem[i].name.length == 13 && !memcmp(sem[i].name.data, "Click details", 13))
            body = sem[i].id;
    }
    assert(action && body);
    cui_focus(ui, action);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 0, .y = 0}));
    notification_frame(ui, &s, 10);
    assert(entries[0].remaining == 5);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    notification_frame(ui, &s, 11);
    assert(counts.clicked == 1 && entries[0].phase == CUI_NOTIFICATION_PRESENT);
    cui_focus(ui, 0);
    notification_frame(ui, &s, 12);
    assert(entries[0].remaining == 4);
    pointer_notification(ui, (cui_rect){card.x + 20, card.y + 30, 20, 20}, CUI_POINTER_PRIMARY);
    notification_frame(ui, &s, 12.1);
    assert(entries[0].phase == CUI_NOTIFICATION_ENDING && counts.clicked == 2 &&
           counts.closed == 0);
    notification_frame(ui, &s, 12.3);
    assert(s.count == 0 && counts.closed == 1);
    n.id.key = 2;
    n.on_click = NULL;
    n.action_label = (cui_str){0};
    assert(cui_notification_push(&s, n, 13));
    notification_frame(ui, &s, 13.4);
    pointer_notification(ui, entries[0].bounds, CUI_POINTER_MIDDLE);
    notification_frame(ui, &s, 14);
    assert(entries[0].phase == CUI_NOTIFICATION_ENDING);
    notification_frame(ui, &s, 14.2);
    assert(s.count == 0 && counts.closed == 2);
    cui_theme theme = cui_theme_light();
    n.id.key = 3;
    n.autohide = true;
    n.content = notification_custom_content;
    n.content_user = &theme;
    assert(cui_notification_push(&s, n, 15));
    notification_frame(ui, &s, 15.4);
    sem = cui_semantics(ui, &count);
    cui_id custom = 0;
    for (size_t i = 0; i < count; i++)
        if (sem[i].name.length == 13 && !memcmp(sem[i].name.data, "Custom action", 13))
            custom = sem[i].id;
    assert(custom);
    cui_focus(ui, custom);
    notification_frame(ui, &s, 30);
    assert(entries[0].remaining == 5);
    cui_focus(ui, 0);
    cui_notification_clear(&s, 31);
    notification_frame(ui, &s, 31.2);
    n.id.key = 4;
    n.content = NULL;
    assert(cui_notification_push(&s, n, 32));
    notification_frame(ui, &s, 32.4);
    card = entries[0].bounds;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_MIDDLE,
                                           .x = card.x + 10,
                                           .y = card.y + 10}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_MIDDLE,
                                           .x = 0,
                                           .y = 0}));
    notification_frame(ui, &s, 32.5);
    assert(entries[0].phase == CUI_NOTIFICATION_PRESENT);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = card.x + 10, .y = card.y + 10}));
    notification_frame(ui, &s, 33);
    sem = cui_semantics(ui, &count);
    cui_id close = 0;
    for (size_t i = 0; i < count; i++)
        if (sem[i].name.length == 5 && !memcmp(sem[i].name.data, "Close", 5))
            close = sem[i].id;
    assert(close);
    int clicked = counts.clicked;
    pointer_notification(ui, cui_bounds(ui, close), CUI_POINTER_PRIMARY);
    notification_frame(ui, &s, 33.1);
    assert(entries[0].phase == CUI_NOTIFICATION_ENDING && counts.clicked == clicked);
    notification_frame(ui, &s, 33.3);
    assert(s.count == 0);
    cui_destroy(ui);
}
#endif
#ifndef CUI_STATUS_BASE_ONLY
static void notification_motion(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    assert(ui);
    cui_notification_entry entries[4];
    cui_notification_state s;
    cui_notification_settings settings = cui_notification_settings_default();
    assert(cui_notification_init(&s, entries, 4, settings, (cui_notification_host){0}));
    cui_notification n = cui_notification_default();
    n.autohide = false;
    n.height = 80;
    n.message = cui_string("Persistent");
    for (uint64_t i = 1; i <= 4; i++) {
        n.id = (cui_notification_id){3, i};
        assert(cui_notification_push(&s, n, 0));
    }
    notification_frame(ui, &s, .4);
    assert(entries[0].visibility == 0 && entries[1].motion_width == settings.width * .9f &&
           s.stack_height[2] == 122);
    cui_rect front = entries[3].bounds;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = front.x + 10, .y = front.y + 10}));
    cui_notification_advance r = notification_frame(ui, &s, .5);
    assert(r.next_wake_seconds > .5 && s.stack_height[2] > 122 && s.stack_height[2] < 362 &&
           entries[0].motion_y > 42 && entries[0].motion_y < 282 && entries[0].visibility > 0);
    double time = .5;
    for (int i = 0; i < 300 && r.next_wake_seconds; i++) {
        time += .016;
        r = notification_frame(ui, &s, time);
    }
    assert(r.next_wake_seconds == 0 && s.stack_height[2] == 362 && entries[0].visibility == 1);
    r = notification_frame(ui, &s, time + .016);
    assert(!cui_needs_redraw(ui) && r.next_wake_seconds == 0);
    cui_destroy(ui);
}
#endif
static void notification_models(void) {
    cui_notification_entry entries[4];
    cui_notification_state s;
    notification_counts counts = {0};
    cui_notification_settings settings = cui_notification_settings_default();
    settings.max_visible = 2;
    cui_notification_host host = {send_notification, retract_notification, activate_notification,
                                  &counts};
    assert(cui_notification_init(&s, entries, 4, settings, host));
    cui_notification n = cui_notification_default();
    n.id = (cui_notification_id){1, 1};
    n.user = &counts;
    n.on_close = close_notification;
    n.on_click = click_notification;
    assert(cui_notification_push(&s, n, 0));
    assert(s.count == 1 && entries[0].phase == CUI_NOTIFICATION_STARTING);
    cui_notification_advance r = cui_notification_update(&s, .4, false);
    assert(r.presented == 1 && entries[0].remaining == 5);
    r = cui_notification_update(&s, 10, true);
    assert(entries[0].remaining == 5 && r.next_wake_seconds == 0);
    r = cui_notification_update(&s, 14.9, false);
    assert(entries[0].phase == CUI_NOTIFICATION_PRESENT && entries[0].remaining > .09);
    r = cui_notification_update(&s, 15, false);
    assert(r.ending == 1 && counts.closed == 0 && s.count == 1);
    r = cui_notification_update(&s, 15.199, false);
    assert(s.count == 1 && counts.closed == 0);
    r = cui_notification_update(&s, 15.2, false);
    assert(r.removed == 1 && s.count == 0 && counts.closed == 1);
    for (uint64_t i = 1; i <= 4; i++) {
        n.id.key = i;
        assert(cui_notification_push(&s, n, 20));
    }
    assert(!cui_notification_visible(&s, 0) && !cui_notification_visible(&s, 1) &&
           cui_notification_visible(&s, 2) && cui_notification_visible(&s, 3));
    assert(cui_notification_dismiss(&s, (cui_notification_id){1, 1}, 20));
    assert(cui_notification_visible(&s, 0));
    n.id.key = 3;
    assert(cui_notification_push(&s, n, 21));
    assert(s.count == 4 && entries[3].notification.id.key == 3 && counts.closed == 1);
    n.id.key = 5;
    assert(!cui_notification_push(&s, n, 21) && s.count == 4);
    assert(cui_notification_clear(&s, 22) == 3);
    r = cui_notification_update(&s, 22.3, false);
    assert(s.count == 0 && r.removed == 4 && counts.closed == 5);
    n.id.key = 9;
    n.delivery = CUI_NOTIFICATION_BOTH;
    n.custom_delivery = true;
    assert(cui_notification_push(&s, n, 30) && counts.sent == 1 && s.system_count == 1);
    cui_notification_update(&s, 30.4, false);
    cui_notification_update(&s, 36, false);
    cui_notification_update(&s, 36.3, false);
    assert(s.count == 0 && s.system_count == 1 && counts.retracted == 0);
    assert(cui_notification_system_click(&s, n.id, 37) && counts.activated == 1 &&
           counts.clicked == 1 && counts.retracted == 1 && s.system_count == 0);
    assert(!cui_notification_system_click(&s, n.id, 37));
    n.delivery = CUI_NOTIFICATION_SYSTEM;
    assert(cui_notification_push(&s, n, 40) && s.count == 0 && s.system_count == 1);
    assert(cui_notification_clear(&s, 41) == 1 && s.system_count == 0);
    counts.reject = true;
    assert(!cui_notification_push(&s, n, 42) && s.count == 0 && s.system_count == 0);
    counts.reject = false;
    for (uint64_t i = 1; i <= 101; i++) {
        n.id.key = i;
        assert(cui_notification_push(&s, n, 43));
    }
    assert(s.system_count == 100 &&
           !cui_notification_system_click(&s, (cui_notification_id){1, 1}, 44));
    assert(cui_notification_dismiss_type(&s, 1, 45) == 100 && s.system_count == 0);
    float heights[] = {180, 80};
    cui_notification_geometry g;
    assert(cui_notification_stack_geometry(heights, 2, 0, false, &g) && g.collapsed_height == 194 &&
           g.expanded_height == 274 && g.collapsed_y == 14 && g.expanded_y == 94);
    assert(cui_notification_stack_geometry(heights, 2, 1, true, &g) && g.collapsed_y == 114 &&
           g.expanded_y == 194);
    heights[0] = FLT_MAX;
    heights[1] = FLT_MAX;
    assert(!cui_notification_stack_geometry(heights, 2, 0, false, &g));
    heights[0] = NAN;
    assert(!cui_notification_stack_geometry(heights, 2, 0, false, &g));
}
int main(void) {
    marker_models();
    shimmer_models();
    notification_models();
#ifndef CUI_STATUS_BASE_ONLY
    marker_render();
    shimmer_render();
    marker_highlight_colors();
    notification_render();
    notification_interactions();
    notification_autoheight();
    notification_theme_tokens();
    notification_action_fades();
    notification_motion();
#endif
#ifdef CUI_STATUS_BASE_ONLY
    cui_marker_options o = cui_marker_options_default();
    cui_marker_plan p;
    assert(cui_base_marker(o, NULL, 0, &p));
    assert(p.alignment == CUI_MARKER_ALIGN_START && !p.animated);
    o.variant = CUI_MARKER_SEPARATOR;
    assert(cui_base_marker(o, NULL, 0, &p) && p.alignment == CUI_MARKER_ALIGN_CENTER);
    o.loading = true;
    assert(cui_base_marker(o, NULL, 0, &p) && p.spinner && p.animated);
    cui_marker_child icon = {.kind = CUI_MARKER_ICON};
    assert(cui_base_marker(o, &icon, 1, &p) && !p.spinner && !p.animated);
    o.width = NAN;
    assert(!cui_base_marker(o, NULL, 0, &p));
    return 0;
#else
#ifdef CUI_STATUS_BASE_ONLY
    cui_theme themes[4] = {
        {.font_size = 14, .radius = 8, .surface = {1, 2, 3, 255}, .text = {4, 5, 6, 255}}};
    for (size_t i = 1; i < 4; i++)
        themes[i] = themes[0];
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    cui_tag_options o = cui_tag_options_default();
    assert(o.variant == CUI_TAG_SECONDARY && o.size == CUI_STATUS_MEDIUM);
    for (size_t i = 0; i < 4; i++) {
        cui_theme *t = &themes[i];
        cui_tag_style s;
        assert(cui_tag_resolve(t, o, false, &s));
        assert(equal(s.background, t->secondary));
        assert(s.padding_x == 10 && s.padding_y == 4 && s.radius == t->radius);
        for (int v = CUI_TAG_PRIMARY; v <= CUI_TAG_INFO; v++) {
            o.variant = (cui_tag_variant)v;
            o.outline = false;
            assert(cui_tag_resolve(t, o, false, &s));
            cui_color bg = s.background, border = s.border;
            o.outline = true;
            assert(cui_tag_resolve(t, o, false, &s));
            assert(s.background.a == 0 && equal(s.foreground, bg) && equal(s.border, border));
        }
        o = cui_tag_options_default();
        o.size = CUI_STATUS_SMALL;
        assert(cui_tag_resolve(t, o, false, &s));
        assert(s.padding_x == 6 && s.padding_y == 2 && s.radius == t->radius / 2);
        o.custom_radius = true;
        o.radius = 0;
        assert(cui_tag_resolve(t, o, false, &s) && s.radius == 0);
        o.pill = true;
        assert(cui_tag_resolve(t, o, false, &s) && s.radius > 1000);
        o = cui_tag_options_default();
#ifndef CUI_STATUS_BASE_ONLY
        cui_context *ui = cui_create((cui_config){.width = 300, .height = 100});
        assert(ui);
        assert(cui_begin_frame(ui, 300, 100, .016f));
        assert(cui_tag(ui, 21, t, cui_string("Ready"), o));
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        bool text = false, border = false;
        for (size_t k = 0; k < list.count; k++) {
            text |= list.commands[k].type == CUI_DRAW_TEXT;
            border |= list.commands[k].type == CUI_DRAW_BORDER;
        }
        assert(text && border);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 1 && sem[0].id == 21 && sem[0].role == CUI_ROLE_LABEL);
        cui_destroy(ui);
#endif
    }
    cui_theme custom_secondary = themes[0];
    custom_secondary.secondary = (cui_color){17, 33, 49, 255};
    custom_secondary.secondary_foreground = (cui_color){201, 211, 221, 255};
    cui_tag_style secondary_style;
    cui_tag_options secondary_options = cui_tag_options_default();
    assert(cui_tag_resolve(&custom_secondary, secondary_options, false, &secondary_style));
    assert(equal(secondary_style.background, custom_secondary.secondary));
    assert(equal(secondary_style.foreground, custom_secondary.secondary_foreground));
    secondary_options.outline = true;
    assert(cui_tag_resolve(&custom_secondary, secondary_options, false, &secondary_style));
    assert(secondary_style.background.a == 0 &&
           equal(secondary_style.foreground, custom_secondary.muted));
    cui_tag_style s = {.radius = 123};
    o = cui_tag_options_default();
    o.radius = NAN;
    assert(!cui_tag_resolve(&themes[0], o, false, &s) && s.radius == 123);
    o = cui_tag_options_default();
    o.variant = CUI_TAG_PALETTE;
    o.palette = (cui_tag_palette){.scale50 = {1, 2, 3, 255},
                                  .scale200 = {4, 5, 6, 255},
                                  .scale600 = {7, 8, 9, 255},
                                  .scale950 = {10, 11, 12, 255},
                                  .scale800 = {13, 14, 15, 255},
                                  .scale300 = {16, 17, 18, 255}};
    assert(cui_tag_resolve(&themes[0], o, false, &s) && s.background.r == 1 &&
           s.foreground.r == 7 && s.border.r == 4);
    o.palette.dark = true;
    assert(cui_tag_resolve(&themes[0], o, false, &s) && s.background.r == 10 &&
           s.background.a == 127 && s.foreground.r == 16 && s.border.r == 13);
    assert(cui_tag_resolve(&themes[0], o, true, &s) && s.background.a == 114);
    cui_tag_palette named;
    assert(cui_tag_named_palette(CUI_STATUS_COLOR_BLUE, false, &named));
    assert(named.scale50.r == 239 && named.scale50.g == 246 && named.scale50.b == 255);
    for (int name = CUI_STATUS_COLOR_WHITE; name <= CUI_STATUS_COLOR_ROSE; name++)
        assert(cui_tag_named_palette((cui_status_color)name, true, &named) && named.dark);
    named.dark = false;
    assert(!cui_tag_named_palette((cui_status_color)99, true, &named) && !named.dark);
#ifndef CUI_STATUS_BASE_ONLY
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 100});
    assert(ui);
    o = cui_tag_options_default();
    assert(cui_begin_frame(ui, 300, 100, .016f));
    assert(cui_begin_tag(ui, 44, &themes[0], o, &s));
    cui_begin_box(ui, 45, (cui_box){.width = 40, .height = 20});
    cui_end_box(ui);
    cui_end_tag(ui, 44, &s);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    bool geometry = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_RECT &&
            equal(list.commands[i].color, themes[0].secondary)) {
            assert(list.commands[i].bounds.w == 60 && list.commands[i].bounds.h == 28);
            geometry = true;
        }
    assert(geometry);
    cui_destroy(ui);
#endif
    return 0;
#endif
}
