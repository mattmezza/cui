/* SPDX-License-Identifier: MIT */
/* Independent pre-getter painter baseline. This source deliberately uses no new
 * getter and no copied salt/hash oracle. Capture against the preserved old
 * library, then compare exactly after integration. Portable, no host/window. */
#include "cui/cui_content.h"
#include "cui/cui_media.h"
#include "cui/cui_message.h"
#include "cui/cui_status.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t callbacks;
static void hash_bytes(uint64_t *h, const void *data, size_t count) {
    const unsigned char *p = data;
    for (size_t i = 0; i < count; i++) {
        *h ^= p[i];
        *h *= UINT64_C(1099511628211);
    }
}
#define HASH(h, value) hash_bytes(&(h), &(value), sizeof(value))
static void id_callback(cui_id id) {
    HASH(callbacks, id);
}
static void reactions(cui_context *ui, const cui_theme *t, cui_color fg, void *user) {
    (void)t;
    (void)user;
    id_callback(71);
    cui_text(ui, cui_string("+1"), 12, fg);
}
static void slot(cui_context *ui, cui_id id, void *user, cui_message_style style) {
    (void)user;
    id_callback(id);
    cui_text(ui, cui_string("slot"), style.font_size, style.color);
}
static void media(cui_context *ui, cui_id id, void *user, float width, float height) {
    (void)user;
    id_callback(id);
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .height = height > 0 ? height : 16,
                            .background = {70, 80, 90, 255}});
    cui_end_box(ui);
}
static void notice(cui_context *ui, cui_id id, void *user, float font, cui_color color,
                   float alpha) {
    (void)user;
    (void)alpha;
    id_callback(id);
    cui_text(ui, cui_string("notice"), font, color);
}
static void action(void *user, cui_id id) {
    (void)user;
    id_callback(id);
}
static void row(cui_context *ui, cui_id id, size_t index, void *user, float width) {
    (void)user;
    id_callback(id);
    HASH(callbacks, index);
    cui_begin_bubble(ui, id, &(cui_theme){.surface = {30, 40, 50, 255}}, CUI_BUBBLE_OUTLINE, width);
    cui_text(ui, cui_string("row"), 12, (cui_color){0, 0, 0, 255});
    cui_end_bubble(ui);
}
static uint64_t draw_hash(cui_draw_list list) {
    uint64_t h = UINT64_C(1469598103934665603);
    HASH(h, list.count);
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        HASH(h, c->type);
        HASH(h, c->bounds);
        HASH(h, c->color);
        HASH(h, c->radius);
        HASH(h, c->stroke);
        HASH(h, c->font_size);
        HASH(h, c->custom_corners);
        HASH(h, c->corner_radii);
        HASH(h, c->border_edges);
        HASH(h, c->custom_border_edges);
        HASH(h, c->text.length);
        if (c->text.length)
            hash_bytes(&h, c->text.data, c->text.length);
        HASH(h, c->bitmap.bitmap.width);
        HASH(h, c->bitmap.bitmap.height);
        HASH(h, c->bitmap.opacity);
        HASH(h, c->shadow);
    }
    return h;
}
int main(int argc, char **argv) {
    const uint16_t little = 1;
    if (sizeof(size_t) != 8 || *(const unsigned char *)&little != 1)
        return 77;
    assert(argc == 2 || (argc == 3 && !strcmp(argv[1], "--record")));
    bool record = argc == 3;
    FILE *file = fopen(argv[record ? 2 : 1], record ? "w" : "r");
    assert(file);
    char json[200000], actual[80000], expected[80000];
    for (unsigned kind = 0; kind < 5; kind++)
        for (unsigned zero = 0; zero < 2; zero++) {
            cui_id id = zero ? 0 : 100 + kind;
            cui_context *ui = cui_create((cui_config){.max_elements = 2048, .max_commands = 8192});
            assert(ui);
            cui_set_layout_debug(ui, true);
            cui_theme t = cui_theme_light();
            cui_bubble_state bubble = {0};
            cui_attachment_state attachment = {0};
            cui_notification_entry entries[4];
            cui_notification_state state;
            cui_notification_settings settings = cui_notification_settings_default();
            settings.reduced_motion = true;
            settings.shadow_policy = CUI_NOTIFICATION_SHADOW_SOURCE;
            assert(cui_notification_init(&state, entries, 4, settings, (cui_notification_host){0}));
            cui_notification n = cui_notification_default();
            n.id = (cui_notification_id){3, 7};
            n.message = cui_string("notification");
            n.content = notice;
            n.action = notice;
            n.autohide = false;
            assert(cui_notification_push(&state, n, 0));
            cui_message_scroller_state *scroller =
                cui_message_scroller_create((cui_message_scroller_config){8, 3, 24});
            assert(scroller);
            for (unsigned frame = 0; frame < 2; frame++) {
                callbacks = UINT64_C(1469598103934665603);
                assert(cui_begin_frame(ui, 500, 400, .016f));
                cui_begin_box(ui, 900, (cui_box){.width = 500, .height = 400});
                if (kind == 0)
                    cui_bubble(ui, id, &t,
                               (cui_bubble_options){.message = {"bubble", 6},
                                                    .state = &bubble,
                                                    .reactions = reactions});
                if (kind == 1) {
                    cui_message_body body[2] = {{{.text = {"typed", 5}}, true, CUI_BUBBLE_FILLED},
                                                {{.render = slot}, false, CUI_BUBBLE_GHOST}};
                    cui_message_options o = cui_message_options_default();
                    o.width = 240;
                    o.body = body;
                    o.body_count = 2;
                    o.avatar = (cui_message_slot){.text = {"A", 1}};
                    o.header = (cui_message_slot){.render = slot};
                    o.footer = (cui_message_slot){.render = slot};
                    (void)cui_message(ui, id, &t, o);
                }
                if (kind == 2) {
                    cui_message_scroller_options o = cui_message_scroller_options_default();
                    o.width = 240;
                    o.height = 120;
                    o.reduced_motion = true;
                    o.jump_content = (cui_message_slot){.text = {"Jump", 4}};
                    (void)cui_message_scroller(ui, id, &t, scroller, row, NULL, o);
                }
                if (kind == 3) {
                    cui_notification_advance out;
                    assert(cui_notifications_checked(ui, id, &t, &state, (cui_rect){0, 0, 500, 400},
                                                     frame * .016, &out));
                }
                if (kind == 4) {
                    cui_attachment_child children[2] = {
                        {.kind = CUI_ATTACHMENT_TITLE, .text = {"file", 4}},
                        {.kind = CUI_ATTACHMENT_DESCRIPTION, .text = {"failed", 6}}};
                    cui_attachment_options o = cui_attachment_options_default();
                    o.model.status = CUI_ATTACHMENT_FAILED;
                    o.model.media = true;
                    o.model.vertical = frame != 0;
                    o.children = children;
                    o.child_count = 2;
                    o.actions = media;
                    o.media_overlay = media;
                    o.actions_width = 40;
                    o.on_remove = action;
                    o.on_retry = action;
                    o.reduced_motion = true;
                    (void)cui_attachment(ui, id, &t, &attachment, o);
                }
                cui_end_box(ui);
                cui_draw_list list = cui_end_frame(ui);
                size_t required = 0;
                assert(cui_debug_layout_export(ui, CUI_DEBUG_LAYOUT_JSON, json, sizeof(json),
                                               &required));
                uint64_t layout = UINT64_C(1469598103934665603);
                hash_bytes(&layout, json, strlen(json));
                uint64_t semantics = UINT64_C(1469598103934665603);
                size_t count;
                const cui_semantic *nodes = cui_semantics(ui, &count);
                for (size_t i = 0; i < count; i++) {
                    HASH(semantics, nodes[i].id);
                    HASH(semantics, nodes[i].role);
                    HASH(semantics, nodes[i].bounds);
                    HASH(semantics, nodes[i].checked);
                    HASH(semantics, nodes[i].disabled);
                    if (nodes[i].name.length)
                        hash_bytes(&semantics, nodes[i].name.data, nodes[i].name.length);
                }
                size_t used = (size_t)snprintf(actual, sizeof(actual),
                                               "%u %u %u %u %016" PRIx64 " %016" PRIx64
                                               " %016" PRIx64 " %016" PRIx64 " ids",
                                               kind, zero, frame, (unsigned)cui_last_error(ui),
                                               layout, draw_hash(list), semantics, callbacks);
                const char *p = json;
                while ((p = strstr(p, "\"id\":\""))) {
                    p += 6;
                    uint64_t box = strtoull(p, NULL, 10);
                    used +=
                        (size_t)snprintf(actual + used, sizeof(actual) - used, " %" PRIu64, box);
                }
                assert(used + 2 < sizeof(actual));
                actual[used++] = '\n';
                actual[used] = 0;
                if (record)
                    assert(fputs(actual, file) >= 0);
                else {
                    assert(fgets(expected, sizeof(expected), file));
                    if (strcmp(actual, expected)) {
                        fprintf(stderr,
                                "legacy mismatch kind%u zero%u frame%u\nexpected %sactual %s", kind,
                                zero, frame, expected, actual);
                        abort();
                    }
                }
            }
            cui_message_scroller_destroy(scroller);
            cui_destroy(ui);
        }
    if (!record)
        assert(fgetc(file) == EOF);
    assert(fclose(file) == 0);
    return 0;
}
