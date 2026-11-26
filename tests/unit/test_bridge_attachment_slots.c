/* SPDX-License-Identifier: MIT */
#include <assert.h>

#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"

static cui_bridge_app *create(uint32_t max_nodes) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 500,
                                  .height = 460,
                                  .max_nodes = max_nodes};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    return app;
}

static cui_bridge_node add(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                           const char *key, const char *text) {
    cui_bridge_node node;
    assert(cui_bridge_node_create(app, parent, kind, key, &node) == CUI_BRIDGE_OK);
    if (text)
        assert(cui_bridge_set_text(app, node, text) == CUI_BRIDGE_OK);
    return node;
}

static cui_bridge_node slot(cui_bridge_app *app, cui_bridge_node card, uint32_t role,
                            const char *key, float width) {
    cui_bridge_node node;
    assert(cui_bridge_attachment_slot_create_v1(app, card, role, key, width, &node) ==
           CUI_BRIDGE_OK);
    return node;
}

static cui_rect bounds(cui_bridge_app *app, cui_bridge_node node) {
    return cui_bounds(app->ui, app->nodes[node.slot].id);
}

static bool semantic(cui_bridge_app *app, cui_bridge_node node) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].id == app->nodes[node.slot].id)
            return true;
    return false;
}

static void step_at(cui_bridge_app *app, int line) {
    cui_bridge_status status = cui_bridge_step(app, .016f);
    if (status != CUI_BRIDGE_OK)
        fprintf(stderr, "step at line %d: bridge=%d diagnostic=%d\n", line, status,
                cui_last_error(app->ui));
    assert(status == CUI_BRIDGE_OK);
}
#define step(app) step_at(app, __LINE__)

static void tap(cui_bridge_app *app, float x, float y) {
    assert(cui_queue_event(app->ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
    assert(cui_queue_event(app->ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
}

static void tap_node(cui_bridge_app *app, cui_bridge_node target) {
    cui_rect b = bounds(app, target);
    assert(b.w > 0 && b.h > 0);
    tap(app, b.x + b.w / 2, b.y + b.h / 2);
}

static cui_bridge_status poll(cui_bridge_app *app, cui_bridge_notification_v1 *notice) {
    char text[1];
    uint32_t required;
    return cui_bridge_poll_value_notification_v1(app, sizeof *notice,
                                                 CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1, notice,
                                                 text, sizeof text, &required);
}

static void expect_child_click(cui_bridge_app *app, cui_bridge_node child) {
    cui_bridge_notification_v1 notice;
    assert(poll(app, &notice) == CUI_BRIDGE_OK);
    assert(notice.kind == CUI_BRIDGE_NOTIFY_ACTIVATE && notice.node_kind == CUI_BRIDGE_BUTTON);
    assert(notice.node.slot == child.slot && notice.node.generation == child.generation);
    assert(notice.payload_kind == CUI_BRIDGE_PAYLOAD_NONE);
}

static void expect_empty(cui_bridge_app *app) {
    cui_bridge_notification_v1 notice;
    assert(poll(app, &notice) == CUI_BRIDGE_EMPTY);
}

static void ownership_and_capacity(void) {
    cui_bridge_app *app = create(4);
    cui_bridge_node root = add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", NULL);
    cui_bridge_node card = add(app, root, CUI_BRIDGE_ATTACHMENT, "attachment", NULL);
    cui_bridge_node out = {199, 299};
    assert(cui_bridge_node_create(app, card, CUI_BRIDGE_COLUMN, "untyped", &out) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_attachment_slot_create_v1(app, root, CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1,
                                                "wrong-parent", 0,
                                                &out) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_attachment_slot_create_v1(app, card, 0, "wrong-role", 0, &out) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_OVERLAY_V1,
                                                "bad-overlay-width", 1,
                                                &out) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_ACTIONS_V1,
                                                "bad-action-width", NAN,
                                                &out) == CUI_BRIDGE_INVALID_ARGUMENT);
    char key[] = "preview";
    cui_bridge_node media = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1, key, 0);
    key[0] = 'x';
    assert(strcmp(app->nodes[media.slot].key, "preview") == 0);
    assert(cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1,
                                                "duplicate-role", 0,
                                                &out) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_create(app, media, CUI_BRIDGE_NOTIFICATION, "stack", &out) ==
           CUI_BRIDGE_INVALID_STATE);
    cui_bridge_node child = add(app, media, CUI_BRIDGE_BUTTON, "preview-btn", "View");
    assert(cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_METADATA_V1,
                                                "at-capacity", 0, &out) == CUI_BRIDGE_CAPACITY);
    assert(out.slot == 199 && out.generation == 299);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
           CUI_BRIDGE_OK);
    step(app);
    tap_node(app, child);
    step(app);
    assert(cui_bridge_node_remove(app, media) == CUI_BRIDGE_OK);
    expect_empty(app); /* An unread descendant activation dies with its generation. */
    assert(cui_bridge_set_text(app, child, "stale") == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_node reused = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1, "preview", 0);
    assert(reused.slot == media.slot && reused.generation != media.generation);
    assert(cui_bridge_node_remove(app, card) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, reused) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1,
                                                "stale", 0, &out) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}

static void present_and_activate(void) {
    cui_bridge_app *app = create(20);
    cui_bridge_node root = add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", NULL);
    cui_bridge_node card = add(app, root, CUI_BRIDGE_ATTACHMENT, "attachment", "Report");
    assert(cui_bridge_set_visual_size(app, card, 320, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, card, "PDF Document") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
           CUI_BRIDGE_OK);
    cui_bridge_attachment_config_v1 presentation = {.struct_size = sizeof presentation,
                                                    .schema_version = 1};
    assert(cui_bridge_attachment_configure_v1(app, card, &presentation) == CUI_BRIDGE_OK);
    assert(cui_bridge_attachment_set_actions_v1(app, card, CUI_BRIDGE_ATTACHMENT_OPEN_V1) ==
           CUI_BRIDGE_OK);
    cui_bridge_node media = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1, "media", 0);
    cui_bridge_node overlay = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_OVERLAY_V1, "overlay", 0);
    cui_bridge_node metadata = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_METADATA_V1, "meta", 0);
    cui_bridge_node actions = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_ACTIONS_V1, "actions", 80);
    cui_bridge_node fallback = add(app, media, CUI_BRIDGE_BUTTON, "media-button", "M");
    cui_bridge_node badge = add(app, overlay, CUI_BRIDGE_BUTTON, "overlay-button", "P");
    cui_bridge_node note = add(app, metadata, CUI_BRIDGE_BUTTON, "metadata-button", "Details");
    cui_bridge_node action = add(app, actions, CUI_BRIDGE_BUTTON, "action-button", "Copy");
    step(app);
    assert(semantic(app, fallback) && semantic(app, badge) && semantic(app, note) &&
           semantic(app, action));
    cui_rect m = bounds(app, media), v = bounds(app, overlay), c = bounds(app, card);
    cui_rect d = bounds(app, metadata), a = bounds(app, actions), b = bounds(app, badge);
    assert(m.w > 0 && m.h > 0 && m.x == v.x && m.y == v.y && m.w == v.w && m.h == v.h);
    assert(fabsf(b.x + b.w / 2 - (v.x + v.w / 2)) <= 1);
    assert(fabsf(b.y + b.h / 2 - (v.y + v.h / 2)) <= 1);
    assert(d.x >= m.x + m.w && d.y > m.y && a.x >= d.x + d.w && a.x + a.w <= c.x + c.w);
    tap_node(app, badge);
    tap_node(app, action);
    tap(app, c.x + 2, c.y + c.h / 2);
    step(app);
    expect_child_click(app, badge); /* Overlay owns the overlap with the fallback. */
    expect_child_click(app, action);
    cui_bridge_notification_v1 notice;
    assert(poll(app, &notice) == CUI_BRIDGE_OK);
    assert(notice.node.slot == card.slot &&
           notice.payload_kind == CUI_BRIDGE_PAYLOAD_ATTACHMENT_ACTION);
    assert(notice.payload.attachment_action.action == CUI_BRIDGE_ATTACHMENT_OPEN_V1);
    expect_empty(app);

    /* An accepted prior-tree action cannot run through a newly inert slot. */
    assert(cui_queue_accessibility_action(app->ui, app->nodes[action.slot].id,
                                          CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_bridge_set_disabled(app, actions, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(!semantic(app, action));
    expect_empty(app);
    assert(cui_bridge_set_disabled(app, actions, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(cui_queue_accessibility_action(app->ui, app->nodes[action.slot].id,
                                          CUI_ACCESSIBILITY_ACTIVATE));
    step(app);
    expect_child_click(app, action);
    expect_empty(app);
    cui_focus(app->ui, app->nodes[action.slot].id);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ENTER, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_ENTER, 0, 0) ==
           CUI_BRIDGE_OK);
    step(app);
    expect_child_click(app, action);
    expect_empty(app);

    tap_node(app, action);
    assert(cui_bridge_set_disabled(app, card, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(!semantic(app, action) && !semantic(app, note) && !semantic(app, badge));
    expect_empty(app);
    assert(cui_bridge_set_disabled(app, card, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(cui_queue_accessibility_action(app->ui, app->nodes[fallback.slot].id,
                                          CUI_ACCESSIBILITY_ACTIVATE));
    const uint8_t pixel[4] = {150, 210, 75, 255};
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, pixel, 1, 1, 4, 4) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(!semantic(app, fallback) && semantic(app, badge));
    expect_empty(app);
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, NULL, 0, 0, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_FAILED) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(!semantic(app, fallback) && semantic(app, badge));

    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
           CUI_BRIDGE_OK);
    presentation.flags = CUI_BRIDGE_ATTACHMENT_VERTICAL_V1;
    presentation.media_aspect = 2;
    assert(cui_bridge_attachment_configure_v1(app, card, &presentation) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, card, 152, 0) == CUI_BRIDGE_OK);
    step(app);
    m = bounds(app, media);
    v = bounds(app, overlay);
    c = bounds(app, card);
    a = bounds(app, actions);
    assert(m.x == v.x && m.y == v.y && m.w == v.w && m.h == v.h);
    assert(a.w == 80 && a.x + a.w < c.x + c.w && a.y > c.y && a.y + a.h < c.y + c.h);
    tap_node(app, action);
    tap(app, c.x + c.w - 24, c.y + c.h - 3); /* Below the action row, over the card. */
    step(app);
    expect_child_click(app, action);
    assert(poll(app, &notice) == CUI_BRIDGE_OK && notice.node.slot == card.slot);
    expect_empty(app);
    assert(cui_bridge_set_visual_size(app, card, 84, 0) == CUI_BRIDGE_OK);
    step(app);
    c = bounds(app, card);
    a = bounds(app, actions);
    assert(a.w == 60 && a.x >= c.x && a.x + a.w <= c.x + c.w);
    cui_bridge_destroy(app);
}

static void inherited_clip(void) {
    cui_bridge_app *app = create(12);
    cui_bridge_node root = add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", NULL);
    cui_bridge_node scroll = add(app, root, CUI_BRIDGE_SCROLLABLE, "scroll", NULL);
    cui_bridge_node card = add(app, scroll, CUI_BRIDGE_ATTACHMENT, "attachment", "Report");
    const cui_bridge_attachment_config_v1 presentation = {
        .struct_size = sizeof presentation,
        .schema_version = 1,
        .flags = CUI_BRIDGE_ATTACHMENT_VERTICAL_V1,
        .media_aspect = 2,
    };
    assert(cui_bridge_attachment_configure_v1(app, card, &presentation) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, card, 260, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, scroll, 84, 200) == CUI_BRIDGE_OK);
    cui_bridge_node overlay = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_OVERLAY_V1, "overlay", 0);
    cui_bridge_node actions = slot(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_ACTIONS_V1, "actions", 80);
    cui_bridge_node badge = add(app, overlay, CUI_BRIDGE_BUTTON, "preview-button", "Preview");
    cui_bridge_node action = add(app, actions, CUI_BRIDGE_BUTTON, "action-button", "Copy");
    step(app);
    cui_rect visible = bounds(app, scroll), clipped = bounds(app, badge);
    assert(clipped.x >= visible.x + visible.w);
    tap_node(app, badge);
    tap_node(app, action);
    step(app);
    expect_empty(app);
    assert(cui_bridge_set_visual_size(app, scroll, 320, 200) == CUI_BRIDGE_OK);
    step(app);
    tap_node(app, badge);
    tap_node(app, action);
    step(app);
    expect_child_click(app, badge);
    expect_child_click(app, action);
    expect_empty(app);
    cui_bridge_destroy(app);
}

int main(void) {
    ownership_and_capacity();
    present_and_activate();
    inherited_clip();
    return 0;
}
