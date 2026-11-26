/* SPDX-License-Identifier: MIT */
#include <assert.h>

#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"

static const uint32_t all_actions = CUI_BRIDGE_ATTACHMENT_OPEN_V1 |
                                    CUI_BRIDGE_ATTACHMENT_REMOVE_V1 |
                                    CUI_BRIDGE_ATTACHMENT_RETRY_V1;

static cui_bridge_node add(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                           const char *key, const char *name) {
    cui_bridge_node node;
    assert(cui_bridge_node_create(app, parent, kind, key, &node) == CUI_BRIDGE_OK);
    if (name)
        assert(cui_bridge_set_text(app, node, name) == CUI_BRIDGE_OK);
    return node;
}

static void step_at(cui_bridge_app *app, int line) {
    cui_bridge_status status = cui_bridge_step(app, .016f);
    if (status != CUI_BRIDGE_OK)
        fprintf(stderr, "step at line %d: bridge=%d diagnostic=%d\n", line, status,
                cui_last_error(app->ui));
    assert(status == CUI_BRIDGE_OK);
}
#define step(app) step_at(app, __LINE__)

static cui_bridge_status poll(cui_bridge_app *app, uint32_t mask, cui_bridge_notification_v1 *out) {
    char text[1] = {'x'};
    uint32_t required = 0;
    cui_bridge_status status = cui_bridge_poll_value_notification_v1(app, sizeof *out, mask, out,
                                                                     text, sizeof text, &required);
    if (status == CUI_BRIDGE_OK)
        assert(text[0] == 0 && out->text_length == 0 && required == 1);
    return status;
}

static void empty(cui_bridge_app *app) {
    cui_bridge_notification_v1 out;
    assert(poll(app, CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1, &out) == CUI_BRIDGE_EMPTY);
}

static void pointer(cui_bridge_app *app, cui_id target, cui_pointer_type type,
                    cui_event_type phase) {
    cui_rect bounds = cui_bounds(app->ui, target);
    assert(bounds.w > 0 && bounds.h > 0);
    cui_event event = {.type = phase,
                       .x = bounds.x + bounds.w * .7f,
                       .y = bounds.y + bounds.h * .5f,
                       .pointer_type = type,
                       .pointer_id = type == CUI_POINTER_TOUCH ? 19 : 0};
    assert(cui_queue_event(app->ui, event));
}

static void tap(cui_bridge_app *app, cui_id target, cui_pointer_type type) {
    pointer(app, target, type, CUI_EVENT_POINTER_DOWN);
    pointer(app, target, type, CUI_EVENT_POINTER_UP);
}

static void key(cui_bridge_app *app, cui_bridge_key key) {
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, key, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_UP, key, 0, 0) == CUI_BRIDGE_OK);
}

static void activate(cui_bridge_app *app, cui_id target) {
    assert(cui_queue_accessibility_action(app->ui, target, CUI_ACCESSIBILITY_ACTIVATE));
}

static void expect_action(cui_bridge_app *app, cui_bridge_node node, uint32_t action, uint32_t part,
                          uint32_t source, uint32_t event_index) {
    cui_bridge_notification_v1 notice;
    assert(poll(app, CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1, &notice) == CUI_BRIDGE_OK);
    assert(notice.kind == CUI_BRIDGE_NOTIFY_ACTIVATE &&
           notice.flags == CUI_BRIDGE_NOTIFY_HAS_FRAME);
    assert(notice.payload_kind == CUI_BRIDGE_PAYLOAD_ATTACHMENT_ACTION);
    assert(notice.node_kind == CUI_BRIDGE_ATTACHMENT && notice.node.slot == node.slot &&
           notice.node.generation == node.generation && notice.frame_sequence != 0);
    assert(notice.legacy.type == CUI_BRIDGE_EVENT_CLICK && notice.legacy.value == action &&
           notice.legacy.frame_sequence == notice.frame_sequence);
    cui_bridge_attachment_action_v1 got = notice.payload.attachment_action;
    assert(got.action == action && got.part == part && got.source == source);
    if (source == CUI_BRIDGE_ACTIVATION_SOURCE_UNKNOWN_V1) {
        assert(got.flags == 0 && got.event_index == 0);
    } else {
        assert(got.flags ==
               (CUI_BRIDGE_ACTIVATION_HAS_SOURCE_V1 | CUI_BRIDGE_ACTIVATION_HAS_INDEX_V1));
        assert(got.event_index == event_index);
    }
}

int main(void) {
    const cui_bridge_options options = {.struct_size = sizeof options,
                                        .abi_version = CUI_BRIDGE_ABI_VERSION,
                                        .backend = CUI_BRIDGE_BACKEND_NONE,
                                        .width = 440,
                                        .height = 360,
                                        .max_events = 3};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", NULL);
    cui_bridge_node card = add(app, root, CUI_BRIDGE_ATTACHMENT, "report", "Delivery report.pdf");
    cui_bridge_node other = add(app, root, CUI_BRIDGE_BUTTON, "other", "Other action");
    cui_id card_id = app->nodes[card.slot].id, other_id = app->nodes[other.slot].id;
    cui_id remove_id = cui_attachment_remove_id(card_id);
    cui_id media_retry = cui_attachment_retry_id(card_id);
    cui_id description_retry =
        cui_attachment_child_part_id(card_id, 1, CUI_ATTACHMENT_CHILD_PART_RETRY);
    assert(cui_bridge_set_detail_text(app, card, "Could not upload") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, card, 216, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, card, .4) == CUI_BRIDGE_OK);
    const cui_bridge_attachment_config_v1 preview = {
        .struct_size = sizeof preview,
        .schema_version = 1,
        .flags = CUI_BRIDGE_ATTACHMENT_MEDIA_V1,
    };
    assert(cui_bridge_attachment_configure_v1(app, card, &preview) == CUI_BRIDGE_OK);
    step(app);
    tap(app, card_id, CUI_POINTER_MOUSE);
    step(app);
    empty(app);
    assert(cui_bridge_attachment_set_actions_v1(app, other, all_actions) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_attachment_set_actions_v1(app, card, all_actions) == CUI_BRIDGE_OK);
    assert(cui_bridge_attachment_set_actions_v1(app, card, 8) == CUI_BRIDGE_INVALID_ARGUMENT);
    step(app);
    assert(cui_bounds(app->ui, remove_id).w > 0);
    assert(!cui_queue_accessibility_action(app->ui, media_retry, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_last_error(app->ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(app->ui);

    tap(app, card_id, CUI_POINTER_MOUSE);
    step(app);
    cui_bridge_notification_v1 notice = {0};
    uint32_t required = 9;
    assert(cui_bridge_poll_value_notification_v1(app, sizeof notice,
                                                 CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1, &notice,
                                                 NULL, 0, &required) == CUI_BRIDGE_CAPACITY);
    assert(required == 1 && !notice.struct_size);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_OPEN_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_CARD_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_POINTER_V1, 1);
    empty(app);

    key(app, CUI_BRIDGE_KEY_ENTER);
    step(app);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_OPEN_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_CARD_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_KEYBOARD_V1, 0);
    key(app, CUI_BRIDGE_KEY_TAB);
    step(app);
    assert(cui_focused(app->ui) == remove_id);
    key(app, CUI_BRIDGE_KEY_ENTER);
    step(app);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_REMOVE_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_REMOVE_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_KEYBOARD_V1, 0);
    tap(app, remove_id, CUI_POINTER_TOUCH);
    step(app);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_REMOVE_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_REMOVE_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_POINTER_V1, 1);

    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_FAILED) ==
           CUI_BRIDGE_OK);
    step(app);
    activate(app, media_retry);
    step(app);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_RETRY_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_MEDIA_RETRY_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_ACCESSIBILITY_V1, 0);
    activate(app, description_retry);
    step(app);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_RETRY_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_DESCRIPTION_RETRY_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_ACCESSIBILITY_V1, 0);

    activate(app, description_retry);
    activate(app, media_retry);
    activate(app, remove_id);
    activate(app, card_id);
    step(app);
    uint64_t queued_frame = app->last_core_frame;
    tap(app, other_id, CUI_POINTER_MOUSE);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CAPACITY);
    char text[1];
    assert(cui_bridge_poll_notification_v1(app, sizeof notice, &notice, text, sizeof text,
                                           &required) == CUI_BRIDGE_OK);
    assert(notice.kind == CUI_BRIDGE_NOTIFY_ACTIVATE && notice.frame_sequence == queued_frame &&
           notice.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY &&
           notice.legacy.value == CUI_BRIDGE_ATTACHMENT_OPEN_V1);
    cui_bridge_event_ex old;
    assert(cui_bridge_poll_event_ex(app, &old, text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(old.type == CUI_BRIDGE_EVENT_CLICK && old.frame_sequence == queued_frame &&
           old.value == CUI_BRIDGE_ATTACHMENT_REMOVE_V1);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_RETRY_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_UNKNOWN_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_UNKNOWN_V1, 0);
    empty(app);
    assert(cui_bridge_get_value(app, card, &old.value) == CUI_BRIDGE_OK && old.value == .4);

    activate(app, card_id);
    step(app);
    assert(cui_bridge_set_disabled(app, card, 1) == CUI_BRIDGE_OK);
    activate(app, card_id); /* Admitted using last frame, then suppressed by current disablement. */
    step(app);
    expect_action(app, card, CUI_BRIDGE_ATTACHMENT_OPEN_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_CARD_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_ACCESSIBILITY_V1, 0);
    empty(app);
    assert(!cui_queue_accessibility_action(app->ui, card_id, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_last_error(app->ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(app->ui);
    assert(cui_bridge_set_disabled(app, card, 0) == CUI_BRIDGE_OK);
    step(app);
    activate(app, card_id);
    assert(cui_bridge_attachment_set_actions_v1(app, card, 0) == CUI_BRIDGE_OK);
    step(app);
    empty(app);
    assert(cui_bridge_attachment_set_actions_v1(app, card, all_actions) == CUI_BRIDGE_OK);
    step(app);
    activate(app, card_id);
    step(app);
    assert(cui_bridge_node_remove(app, card) == CUI_BRIDGE_OK);
    cui_bridge_node replacement = add(app, root, CUI_BRIDGE_ATTACHMENT, "replacement", "New file");
    assert(replacement.slot == card.slot && replacement.generation != card.generation);
    empty(app);
    assert(cui_bridge_attachment_set_actions_v1(app, card, 0) == CUI_BRIDGE_STALE_HANDLE);

    cui_bridge_node panel = add(app, root, CUI_BRIDGE_COLLAPSIBLE, "panel", "Files");
    cui_bridge_node nested = add(app, panel, CUI_BRIDGE_ATTACHMENT, "draft", "Draft.pdf");
    cui_id nested_id = app->nodes[nested.slot].id;
    assert(cui_bridge_attachment_set_actions_v1(app, nested, CUI_BRIDGE_ATTACHMENT_OPEN_V1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    step(app);
    activate(app, nested_id);
    assert(cui_bridge_set_open(app, panel, 0) == CUI_BRIDGE_OK);
    step(app);
    empty(app);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    step(app);
    activate(app, nested_id);
    step(app);
    expect_action(app, nested, CUI_BRIDGE_ATTACHMENT_OPEN_V1,
                  CUI_BRIDGE_ATTACHMENT_ACTION_PART_CARD_V1,
                  CUI_BRIDGE_ACTIVATION_SOURCE_ACCESSIBILITY_V1, 0);
    empty(app);
    cui_bridge_destroy(app);
    return 0;
}
