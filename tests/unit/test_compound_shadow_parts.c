/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include "cui/cui_media.h"
#include "cui/cui_message.h"
#include "cui/cui_status.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static const cui_box_shadow layers[] = {
    {.layer = {.offset_x = 3, .color = {1, 2, 3, 255}}, .kind = CUI_BOX_SHADOW_DROP},
    {.layer = {.offset_y = 1, .color = {4, 5, 6, 255}}, .kind = CUI_BOX_SHADOW_INSET}};
static cui_context *context(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 4096, .max_commands = 16384});
    assert(ui);
    return ui;
}
static void begin(cui_context *ui, unsigned frame) {
    assert(cui_begin_frame(ui, 600, 500, .016f));
    cui_begin_box(ui, 900, (cui_box){.width = 600, .height = 500, .padding = (float)frame * 3});
}
static cui_draw_list end(cui_context *ui) {
    cui_end_box(ui);
    return cui_end_frame(ui);
}
static void decorate_at(cui_context *ui, cui_id id, unsigned line) {
    bool accepted = id && cui_element_set_box_shadows(ui, id, layers, 2, (cui_corner_radii){0});
    if (!accepted)
        fprintf(stderr, "expected paint owner at line %u\n", line);
    assert(accepted);
}
#define decorate(ui, id) decorate_at(ui, id, __LINE__)
static void absent_at(cui_context *ui, cui_id id, unsigned line) {
    bool rejected = id && !cui_element_set_box_shadows(ui, id, layers, 2, (cui_corner_radii){0});
    if (!rejected)
        fprintf(stderr, "expected absent owner at line %u\n", line);
    assert(rejected);
    cui_clear_error(ui);
}
#define absent(ui, id) absent_at(ui, id, __LINE__)
static size_t marked(cui_draw_list list) {
    size_t count = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_DROP_SHADOW &&
            list.commands[i].shadow.layer.color.r == 1 &&
            list.commands[i].shadow.layer.color.g == 2)
            count++;
    return count;
}
static void caster(cui_draw_list list, cui_rect bounds) {
    bool found = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_DROP_SHADOW &&
            list.commands[i].shadow.layer.color.r == 1 &&
            memcmp(&list.commands[i].shadow.caster, &bounds, sizeof(bounds)) == 0)
            found = true;
    assert(found);
}
static void plain_content(cui_context *ui, const cui_theme *t, cui_color color, void *user) {
    (void)t;
    (void)user;
    cui_text(ui, cui_string("reaction"), 12, color);
}
static cui_id callback_namespace;
static unsigned calls;
static void message_slot(cui_context *ui, cui_id id, void *user, cui_message_style style) {
    (void)user;
    callback_namespace = id;
    calls++;
    cui_text(ui, cui_string("custom"), style.font_size, style.color);
}
static void media_slot(cui_context *ui, cui_id id, void *user, float width, float height) {
    (void)user;
    callback_namespace = id;
    calls++;
    cui_begin_box(ui, id, (cui_box){.width = width, .height = height > 0 ? height : 12});
    cui_end_box(ui);
}
static void empty_media(cui_context *ui, cui_id id, void *user, float w, float h) {
    (void)ui;
    (void)user;
    (void)w;
    (void)h;
    callback_namespace = id;
    calls++;
}
static void notification_slot(cui_context *ui, cui_id id, void *user, float size, cui_color c,
                              float alpha) {
    (void)ui;
    (void)user;
    (void)size;
    (void)c;
    (void)alpha;
    callback_namespace = id;
    calls++;
}
static void media_action(void *user, cui_id id) {
    (void)user;
    (void)id;
    calls++;
}
static void validity_and_frozen_ids(void) {
    cui_context *ui = context();
    cui_error before = cui_last_error(ui);
    assert(!cui_bubble_part_id(0, CUI_BUBBLE_PART_SURFACE));
    assert(!cui_bubble_part_id(1, (cui_bubble_part)-1));
    assert(!cui_message_part_id(0, CUI_MESSAGE_PART_AVATAR));
    assert(!cui_message_part_id(1, (cui_message_part)9));
    assert(!cui_message_body_id(0, 0) && !cui_message_body_id(1, 256));
    assert(cui_message_body_id(1, 255));
    assert(!cui_message_scroller_part_id(0, CUI_MESSAGE_SCROLLER_PART_VIEWPORT));
    assert(!cui_message_scroller_part_id(1, (cui_message_scroller_part)9));
    assert(!cui_message_scroller_row_id(1, NULL, 0));
    assert(!cui_notification_part_id(0, (cui_notification_id){1, 2}, CUI_NOTIFICATION_PART_CARD));
    assert(!cui_notification_part_id(1, (cui_notification_id){0}, CUI_NOTIFICATION_PART_CARD));
    assert(!cui_notification_part_id(1, (cui_notification_id){1, 2}, (cui_notification_part)9));
    for (int part = 0; part <= CUI_ATTACHMENT_PART_MEDIA_RETRY; part++)
        assert(!cui_attachment_part_id(0, (cui_attachment_part)part));
    assert(!cui_attachment_part_id(1, (cui_attachment_part)99));
    assert(!cui_attachment_child_part_id(0, 0, CUI_ATTACHMENT_CHILD_PART_ROW));
    assert(!cui_attachment_child_part_id(1, 128, CUI_ATTACHMENT_CHILD_PART_ROW));
    assert(!cui_attachment_child_part_id(1, 0, (cui_attachment_child_part)99));
    assert(cui_attachment_child_part_id(1, 127, CUI_ATTACHMENT_CHILD_PART_ROW));
    assert(cui_last_error(ui) == before);
    /* Observed independently in compound_shadow_legacy.txt before implementation;
     * these are owner IDs, not a replicated derivation algorithm. */
    assert(cui_bubble_part_id(100, CUI_BUBBLE_PART_SURFACE) == UINT64_C(7058498678702963460));
    assert(cui_bubble_part_id(100, CUI_BUBBLE_PART_REACTIONS) == UINT64_C(11413184243639809006));
    assert(cui_message_part_id(101, CUI_MESSAGE_PART_AVATAR) == UINT64_C(317125338075985571));
    assert(cui_message_part_id(101, CUI_MESSAGE_PART_HEADER) == UINT64_C(10230203001953340975));
    assert(cui_message_body_id(101, 0) == UINT64_C(2329792219501244131));
    if (sizeof(size_t) == 8)
        assert(cui_notification_part_id(103, (cui_notification_id){3, 7},
                                        CUI_NOTIFICATION_PART_CARD) ==
               UINT64_C(16940737246258298884));
    assert(cui_attachment_part_id(104, CUI_ATTACHMENT_PART_MEDIA) ==
           UINT64_C(17578774239236958990));
    assert(cui_attachment_part_id(1, CUI_ATTACHMENT_PART_REMOVE) == cui_attachment_remove_id(1));
    assert(cui_attachment_part_id(1, CUI_ATTACHMENT_PART_MEDIA_RETRY) ==
           cui_attachment_retry_id(1));
    assert(cui_message_scroller_part_id(1, CUI_MESSAGE_SCROLLER_PART_JUMP) ==
           cui_message_scroller_jump_id(1));
    assert(cui_attachment_remove_id(0) && cui_attachment_retry_id(0) &&
           cui_message_scroller_jump_id(0));
    cui_destroy(ui);
}
static void bubble_and_message(void) {
    cui_context *ui = context();
    cui_theme t = cui_theme_light();
    cui_bubble_state s = {0};
    cui_rect bounds = {0};
    cui_id surface = cui_bubble_part_id(100, CUI_BUBBLE_PART_SURFACE);
    for (unsigned f = 0; f < 3; f++) {
        begin(ui, f);
        cui_bubble(ui, 100, &t,
                   (cui_bubble_options){.message = {"bubble", 6},
                                        .state = &s,
                                        .reactions = f < 2 ? plain_content : NULL});
        decorate(ui, surface);
        assert(cui_track_bounds(ui, surface, &bounds));
        if (f < 2)
            decorate(ui, cui_bubble_part_id(100, CUI_BUBBLE_PART_REACTIONS));
        else
            absent(ui, cui_bubble_part_id(100, CUI_BUBBLE_PART_REACTIONS));
        /* Clear does not remove the compound surface or its content. */
        assert(cui_element_set_shadow_preset(ui, surface, CUI_SHADOW_NONE, (cui_corner_radii){0}));
        decorate(ui, surface);
        cui_draw_list list = end(ui);
        assert(marked(list) == (f < 2 ? 2 : 1));
        caster(list, bounds);
        assert(bounds.w > 0 && bounds.h > 0 && bounds.w < cui_bounds(ui, 100).w);
    }
    begin(ui, 0);
    absent(ui, surface);
    end(ui);
    begin(ui, 0);
    cui_begin_bubble(ui, 100, &t, CUI_BUBBLE_FILLED, 120);
    cui_text(ui, cui_string("direct"), 12, t.text);
    cui_end_bubble(ui);
    decorate(ui, 100);
    absent(ui, surface);
    assert(marked(end(ui)) == 1);
    cui_message_body body[2] = {{{.text = {"typed", 5}}, true, CUI_BUBBLE_FILLED},
                                {{.render = message_slot}, false, CUI_BUBBLE_GHOST}};
    cui_message_options o = cui_message_options_default();
    o.width = 240;
    o.body = body;
    o.body_count = 2;
    o.avatar = (cui_message_slot){.text = {"A", 1}};
    o.header = (cui_message_slot){.text = {"header", 6}};
    o.footer = (cui_message_slot){.text = {"footer", 6}};
    begin(ui, 0);
    calls = 0;
    assert(cui_message(ui, 101, &t, o));
    assert(calls == 1);
    decorate(ui, cui_message_body_id(101, 0));
    decorate(ui, cui_message_body_id(101, 1));
    decorate(ui, cui_message_part_id(101, CUI_MESSAGE_PART_AVATAR));
    decorate(ui, cui_message_part_id(101, CUI_MESSAGE_PART_HEADER));
    decorate(ui, cui_message_part_id(101, CUI_MESSAGE_PART_FOOTER));
    absent(ui, callback_namespace);
    assert(marked(end(ui)) == 5);
    o.avatar = (cui_message_slot){0};
    o.header = (cui_message_slot){0};
    o.footer = (cui_message_slot){0};
    o.body_count = 0;
    begin(ui, 0);
    assert(cui_message(ui, 101, &t, o));
    absent(ui, cui_message_part_id(101, CUI_MESSAGE_PART_HEADER));
    absent(ui, cui_message_body_id(101, 0));
    end(ui);
    cui_message_body many[257] = {0};
    o.body = many;
    o.body_count = 256;
    begin(ui, 0);
    assert(cui_message(ui, 101, &t, o));
    decorate(ui, cui_message_body_id(101, 255));
    end(ui);
    o.body_count = 257;
    begin(ui, 0);
    assert(!cui_message(ui, 101, &t, o));
    absent(ui, cui_message_body_id(101, 255));
    end(ui);
    cui_destroy(ui);
}
static void render_row(cui_context *ui, cui_id id, size_t index, void *user, float width) {
    (void)index;
    (void)user;
    calls++;
    callback_namespace = id;
    cui_begin_bubble(ui, id, &(cui_theme){.surface = {20, 30, 40, 255}}, CUI_BUBBLE_OUTLINE, width);
    cui_text(ui, cui_string("row"), 12, (cui_color){1, 2, 3, 255});
    cui_end_bubble(ui);
}
static void scroller(void) {
    cui_context *ui = context();
    cui_theme t = cui_theme_light();
    cui_message_scroller_state *s =
        cui_message_scroller_create((cui_message_scroller_config){100, 80, 40});
    assert(s);
    cui_id kept = cui_message_scroller_row_id(102, s, 50);
    assert(kept);
    assert(cui_message_scroller_prepend(s, 2));
    assert(cui_message_scroller_row_id(102, s, 52) == kept);
    assert(cui_message_scroller_splice(s, 0, 1, 0));
    assert(cui_message_scroller_row_id(102, s, 51) == kept);
    assert(!cui_message_scroller_row_id(102, s, 100));
    cui_message_scroller_options o = cui_message_scroller_options_default();
    o.width = 240;
    o.height = 120;
    o.reduced_motion = true;
    o.jump_content = (cui_message_slot){.text = {"jump", 4}};
    for (unsigned f = 0; f < 2; f++) {
        begin(ui, f);
        calls = 0;
        cui_message_scroller_result r = cui_message_scroller(ui, 102, &t, s, render_row, NULL, o);
        assert(r.valid && calls);
        assert(r.view.end - r.view.begin < 81);
        decorate(ui, cui_message_scroller_part_id(102, CUI_MESSAGE_SCROLLER_PART_VIEWPORT));
        decorate(ui, cui_message_scroller_row_id(102, s, r.view.begin));
        absent(ui, cui_message_scroller_row_id(102, s, 0));
        if (r.jump_opacity > 0)
            decorate(ui, cui_message_scroller_part_id(102, CUI_MESSAGE_SCROLLER_PART_JUMP));
        end(ui);
    }
    cui_message_scroller_to_item(s, 0);
    o.jump_button = false;
    begin(ui, 0);
    assert(cui_message_scroller(ui, 102, &t, s, render_row, NULL, o).valid);
    absent(ui, cui_message_scroller_part_id(102, CUI_MESSAGE_SCROLLER_PART_JUMP));
    end(ui);
    assert(cui_message_scroller_reset(s, 81));
    assert(cui_message_scroller_row_id(102, s, 51) != kept);
    cui_message_scroller_destroy(s);
    cui_destroy(ui);
}
static bool submit_notes(cui_context *ui, cui_id id, const cui_theme *t, cui_notification_state *s,
                         double now) {
    cui_notification_advance out;
    return cui_notifications_checked(ui, id, t, s, (cui_rect){0, 0, 600, 500}, now, &out);
}
static bool host_send(void *user, const cui_notification *n) {
    (void)user;
    (void)n;
    return true;
}
static void notifications(void) {
    cui_context *ui = context();
    cui_theme t = cui_theme_light();
    cui_notification_entry entries[4];
    cui_notification_state s;
    cui_notification_settings settings = cui_notification_settings_default();
    settings.reduced_motion = true;
    settings.max_visible = 1;
    settings.shadow_policy = CUI_NOTIFICATION_SHADOW_SOURCE;
    assert(cui_notification_init(&s, entries, 4, settings,
                                 (cui_notification_host){.send = host_send}));
    cui_notification n = cui_notification_default();
    n.id = (cui_notification_id){3, 7};
    n.message = cui_string("note");
    n.autohide = false;
    n.action_label = cui_string("Act");
    assert(cui_notification_push(&s, n, 0));
    cui_id card = cui_notification_part_id(103, n.id, CUI_NOTIFICATION_PART_CARD);
    for (unsigned f = 0; f < 2; f++) {
        begin(ui, f);
        assert(submit_notes(ui, 103, &t, &s, f * .016));
        decorate(ui, card);
        decorate(ui, cui_notification_part_id(103, n.id, CUI_NOTIFICATION_PART_CONTENT));
        decorate(ui, cui_notification_part_id(103, n.id, CUI_NOTIFICATION_PART_ACTION));
        absent(ui, 103);
        absent(ui, cui_notification_part_id(104, n.id, CUI_NOTIFICATION_PART_CARD));
        end(ui);
    }
    cui_rect b = cui_bounds(ui, card);
    assert(b.w > 0 && b.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = b.x + 10, .y = b.y + 10}));
    begin(ui, 0);
    assert(submit_notes(ui, 103, &t, &s, .04));
    decorate(ui, cui_notification_part_id(103, n.id, CUI_NOTIFICATION_PART_CLOSE));
    end(ui);
    n.action = notification_slot;
    assert(cui_notification_push(&s, n, .05));
    begin(ui, 0);
    calls = 0;
    assert(submit_notes(ui, 103, &t, &s, .06));
    assert(calls == 1);
    assert(callback_namespace == cui_notification_part_id(103, n.id, CUI_NOTIFICATION_PART_ACTION));
    absent(ui, callback_namespace);
    end(ui);
    assert(cui_notification_dismiss(&s, n.id, .07));
    begin(ui, 0);
    (void)cui_notifications(ui, 103, &t, &s, (cui_rect){0, 0, 600, 500}, 2);
    end(ui);
    begin(ui, 0);
    (void)cui_notifications(ui, 103, &t, &s, (cui_rect){0, 0, 600, 500}, 3);
    absent(ui, card);
    end(ui);
    n.custom_delivery = true;
    n.delivery = CUI_NOTIFICATION_SYSTEM;
    n.action = NULL;
    assert(cui_notification_push(&s, n, 4));
    begin(ui, 0);
    (void)cui_notifications(ui, 103, &t, &s, (cui_rect){0, 0, 600, 500}, 4);
    absent(ui, card);
    end(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = -100, .y = -100}));
    n.delivery = CUI_NOTIFICATION_IN_APP;
    n.action_label = (cui_str){0};
    for (unsigned i = 0; i < 3; i++) {
        n.id = (cui_notification_id){5, i + 1};
        assert(cui_notification_push(&s, n, 5));
    }
    begin(ui, 0);
    (void)cui_notifications(ui, 103, &t, &s, (cui_rect){0, 0, 600, 500}, 5);
    /* At least one collapsed card has no paintable current-frame owner. */
    unsigned rejected = 0;
    for (unsigned i = 0; i < 3; i++) {
        cui_id id = cui_notification_part_id(103, (cui_notification_id){5, i + 1},
                                             CUI_NOTIFICATION_PART_CARD);
        if (!cui_element_set_box_shadows(ui, id, layers, 2, (cui_corner_radii){0})) {
            rejected++;
            cui_clear_error(ui);
        }
    }
    assert(rejected);
    end(ui);
    cui_destroy(ui);
    ui = context();
    settings.max_visible = 4;
    assert(cui_notification_init(&s, entries, 4, settings, (cui_notification_host){0}));
    n.id = (cui_notification_id){1, 0};
    assert(cui_notification_push(&s, n, 0));
    cui_notification_id collision = {0, 63};
    assert(cui_notification_part_id(103, n.id, CUI_NOTIFICATION_PART_CARD) ==
           cui_notification_part_id(103, collision, CUI_NOTIFICATION_PART_CARD));
    n.id = collision;
    assert(cui_notification_push(&s, n, 0));
    begin(ui, 0);
    (void)cui_notifications(ui, 103, &t, &s, (cui_rect){0, 0, 600, 500}, 0);
    assert(!cui_element_set_box_shadows(
        ui, cui_notification_part_id(103, n.id, CUI_NOTIFICATION_PART_CARD), layers, 2,
        (cui_corner_radii){0}));
    assert(marked(end(ui)) == 0);
    cui_destroy(ui);
}
static void attachments(void) {
    cui_context *ui = context();
    cui_theme t = cui_theme_light();
    cui_attachment_state s = {0};
    cui_attachment_child children[3] = {
        {.kind = CUI_ATTACHMENT_TITLE, .text = {"file", 4}},
        {.kind = CUI_ATTACHMENT_DESCRIPTION, .text = {"failed", 6}},
        {.kind = CUI_ATTACHMENT_DESCRIPTION, .text = {"second", 6}}};
    cui_attachment_options o = cui_attachment_options_default();
    o.model.status = CUI_ATTACHMENT_FAILED;
    o.model.identified = true;
    o.model.media = true;
    o.children = children;
    o.child_count = 3;
    o.actions = media_slot;
    o.actions_width = 40;
    o.media_overlay = media_slot;
    o.on_remove = media_action;
    o.on_retry = media_action;
    o.reduced_motion = true;
    for (unsigned f = 0; f < 3; f++) {
        begin(ui, f);
        calls = 0;
        o.model.vertical = f != 0;
        cui_attachment_result r = cui_attachment(ui, 104, &t, &s, o);
        assert(r.valid);
        decorate(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_CARD));
        decorate(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_MEDIA));
        decorate(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_METADATA));
        decorate(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_ACTIONS));
        decorate(ui, cui_attachment_child_part_id(104, 0, CUI_ATTACHMENT_CHILD_PART_ROW));
        decorate(ui, cui_attachment_child_part_id(104, 1, CUI_ATTACHMENT_CHILD_PART_RETRY));
        absent(ui, cui_attachment_child_part_id(104, 2, CUI_ATTACHMENT_CHILD_PART_RETRY));
        if (f)
            decorate(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_MEDIA_OVERLAY));
        else
            absent(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_MEDIA_OVERLAY));
        cui_draw_list list = end(ui);
        assert(marked(list) > 0);
        caster(list, s.card);
    }
    o.actions = empty_media;
    o.media_overlay = NULL;
    o.model.media = false;
    o.model.status = CUI_ATTACHMENT_COMPLETE;
    children[0] = (cui_attachment_child){.kind = CUI_ATTACHMENT_CUSTOM, .render = empty_media};
    begin(ui, 0);
    assert(cui_attachment(ui, 104, &t, &s, o).valid);
    decorate(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_ACTIONS));
    absent(ui, cui_attachment_part_id(104, CUI_ATTACHMENT_PART_MEDIA));
    absent(ui, cui_attachment_child_part_id(104, 0, CUI_ATTACHMENT_CHILD_PART_ROW));
    absent(ui, cui_attachment_child_part_id(104, 1, CUI_ATTACHMENT_CHILD_PART_RETRY));
    end(ui);
    cui_destroy(ui);
}
int main(void) {
    validity_and_frozen_ids();
    bubble_and_message();
    scroller();
    notifications();
    attachments();
    return 0;
}
