/* SPDX-License-Identifier: MIT */
#include <assert.h>

#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"

static cui_bridge_app *create(uint32_t max_nodes) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 220,
                                  .height = 400,
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

static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .4f) == CUI_BRIDGE_OK);
}

static cui_id id(cui_bridge_app *app, cui_bridge_node node) {
    return app->nodes[node.slot].id;
}

static cui_id part(cui_bridge_app *app, cui_bridge_node note, cui_notification_part kind) {
    return cui_notification_part_id(UINT64_C(0x4e4f544946594255), (cui_notification_id){1, id(app, note)},
                                    kind);
}

static void tap(cui_bridge_app *app, cui_id target, cui_pointer_type type) {
    cui_rect b = cui_bounds(app->ui, target);
    assert(b.w > 0 && b.h > 0);
    cui_event e = {.type = CUI_EVENT_POINTER_DOWN,
                   .x = b.x + b.w / 2,
                   .y = b.y + b.h / 2,
                   .pointer_type = type,
                   .pointer_id = 1};
    assert(cui_queue_event(app->ui, e));
    e.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(app->ui, e));
}

static bool open(cui_bridge_app *app, cui_bridge_node note) {
    uint32_t value;
    assert(cui_bridge_get_open(app, note, &value) == CUI_BRIDGE_OK);
    return value != 0;
}

static void expect_event_at(cui_bridge_app *app, cui_bridge_node node,
                            cui_bridge_event_type type, int line) {
    cui_bridge_event_ex event;
    char payload[1];
    uint32_t required;
    cui_bridge_status status = cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required);
    if (status != CUI_BRIDGE_OK)
        fprintf(stderr, "line %d: expected bridge event %d for slot %u, status %d\n", line, type,
                node.slot, status);
    assert(status == CUI_BRIDGE_OK);
    assert(event.type == type && event.node.slot == node.slot &&
           event.node.generation == node.generation);
}

#define expect_event(app, node, type) expect_event_at(app, node, type, __LINE__)

static void expect_empty(cui_bridge_app *app) {
    cui_bridge_event_ex event;
    char payload[1];
    uint32_t required;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) == CUI_BRIDGE_EMPTY);
}

static void ownership(void) {
    cui_bridge_app *app = create(3);
    cui_bridge_node root = add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", NULL);
    cui_bridge_node note = add(app, root, CUI_BRIDGE_NOTIFICATION, "note", "Updates");
    cui_bridge_node out = {99, 100};
    assert(cui_bridge_notification_action_create_v1(app, root, "wrong-parent", &out) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_notification_action_create_v1(app, note, "", &out) ==
           CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_node_create(app, note, CUI_BRIDGE_BUTTON, "ordinary", &out) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(out.slot == 99 && out.generation == 100);
    char key[] = "review";
    assert(cui_bridge_notification_action_create_v1(app, note, key, &out) == CUI_BRIDGE_OK);
    key[0] = 'x';
    assert(strcmp(app->nodes[out.slot].key, "review") == 0);
    cui_bridge_node other = {99, 100};
    assert(cui_bridge_notification_action_create_v1(app, note, "duplicate", &other) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, out) == CUI_BRIDGE_OK);
    assert(cui_bridge_notification_action_create_v1(app, note, "review", &other) == CUI_BRIDGE_OK);
    assert(other.slot == out.slot && other.generation != out.generation);
    assert(cui_bridge_node_remove(app, note) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, other) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_notification_action_create_v1(app, note, "stale", &out) ==
           CUI_BRIDGE_STALE_HANDLE);
    note = add(app, root, CUI_BRIDGE_NOTIFICATION, "replacement", NULL);
    (void)add(app, root, CUI_BRIDGE_BUTTON, "fills-capacity", NULL);
    out = (cui_bridge_node){99, 100};
    assert(cui_bridge_notification_action_create_v1(app, note, "no-room", &out) ==
           CUI_BRIDGE_CAPACITY);
    assert(out.slot == 99 && out.generation == 100);
    cui_bridge_destroy(app);
}

static void render_and_dispatch(bool responsive) {
    cui_bridge_app *app = create(12);
    assert(cui_bridge_set_responsive_notifications_v1(app, responsive) == CUI_BRIDGE_OK);
    cui_bridge_node root = add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", NULL);
    cui_bridge_node note = add(app, root, CUI_BRIDGE_NOTIFICATION, "note", "Review workspace");
    assert(cui_bridge_set_detail_text(app, note, "Three tasks are ready for your review.") ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, note, 1) == CUI_BRIDGE_OK);
    step(app);
    step(app);
    cui_bridge_node action;
    assert(cui_bridge_notification_action_create_v1(app, note, "review", &action) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, action, "Open review") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_variant(app, action, CUI_BRIDGE_BUTTON_OUTLINE) == CUI_BRIDGE_OK);
    step(app);
    step(app);

    cui_id action_id = id(app, action), close_id = part(app, note, CUI_NOTIFICATION_PART_CLOSE);
    cui_rect b = cui_bounds(app->ui, action_id);
    cui_rect close = cui_bounds(app->ui, close_id);
    cui_rect content = cui_bounds(app->ui, part(app, note, CUI_NOTIFICATION_PART_CONTENT));
    assert(b.w > 0 && b.h > 0 && b.x >= content.x && b.x + b.w <= close.x &&
           b.y >= content.y + content.h && close.h > 0);
    tap(app, action_id, CUI_POINTER_MOUSE);
    tap(app, action_id, CUI_POINTER_TOUCH);
    step(app);
    expect_event(app, action, CUI_BRIDGE_EVENT_CLICK);
    expect_event(app, action, CUI_BRIDGE_EVENT_CLICK);
    expect_empty(app);
    assert(open(app, note));

    cui_focus(app->ui, action_id);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_ENTER, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_ENTER, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_queue_accessibility_action(app->ui, action_id, CUI_ACCESSIBILITY_ACTIVATE));
    step(app);
    expect_event(app, action, CUI_BRIDGE_EVENT_CLICK);
    expect_event(app, action, CUI_BRIDGE_EVENT_CLICK);
    expect_empty(app);

    assert(cui_queue_accessibility_action(app->ui, action_id, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_bridge_set_disabled(app, action, 1) == CUI_BRIDGE_OK);
    step(app);
    expect_empty(app);
    assert(cui_bridge_set_disabled(app, action, 0) == CUI_BRIDGE_OK);
    step(app);

    tap(app, action_id, CUI_POINTER_MOUSE);
    step(app);
    assert(cui_bridge_node_remove(app, action) == CUI_BRIDGE_OK);
    expect_empty(app); /* The queued click cannot reach a replacement generation. */
    assert(cui_bridge_notification_action_create_v1(app, note, "review", &action) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, action, "Review tasks") == CUI_BRIDGE_OK);
    step(app);
    assert(cui_bounds(app->ui, action_id).w == 0 && id(app, action) != action_id);
    tap(app, close_id, CUI_POINTER_MOUSE);
    step(app);
    assert(!open(app, note));
    tap(app, id(app, action), CUI_POINTER_MOUSE); /* During the exit the action is inert. */
    assert(cui_bridge_step(app, .05f) == CUI_BRIDGE_OK);
    expect_empty(app);
    step(app);
    expect_event(app, note, CUI_BRIDGE_EVENT_CLOSE);
    expect_empty(app);

    assert(cui_bridge_set_open(app, note, 1) == CUI_BRIDGE_OK);
    step(app);
    step(app);
    tap(app, part(app, note, CUI_NOTIFICATION_PART_CONTENT), CUI_POINTER_MOUSE);
    step(app);
    assert(!open(app, note));
    expect_event(app, note, CUI_BRIDGE_EVENT_CLICK);
    step(app);
    expect_event(app, note, CUI_BRIDGE_EVENT_CLOSE);
    expect_empty(app);
    cui_bridge_destroy(app);
}

int main(void) {
    ownership();
    render_and_dispatch(false);
    render_and_dispatch(true);
    return 0;
}
