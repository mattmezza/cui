/* SPDX-License-Identifier: MIT */
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include <assert.h>

static cui_bridge_node make_node(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                                 const char *key) {
    cui_bridge_node node;
    assert(cui_bridge_node_create(app, parent, kind, key, &node) == CUI_BRIDGE_OK);
    if (kind != CUI_BRIDGE_COLUMN)
        assert(cui_bridge_set_text(app, node, key) == CUI_BRIDGE_OK);
    return node;
}

static cui_bridge_node make_slot(cui_bridge_app *app, cui_bridge_node dialog, uint32_t role,
                                 const char *key) {
    cui_bridge_node slot;
    assert(cui_bridge_dialog_slot_create_v1(app, dialog, role, key, &slot) == CUI_BRIDGE_OK);
    return slot;
}

static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}

static bool is_open(cui_bridge_app *app, cui_bridge_node node) {
    uint32_t open;
    assert(cui_bridge_get_open(app, node, &open) == CUI_BRIDGE_OK);
    return open != 0;
}

static void set_open(cui_bridge_app *app, cui_bridge_node node, bool open) {
    assert(cui_bridge_set_open(app, node, open) == CUI_BRIDGE_OK);
    step(app);
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect rect;
    assert(cui_bridge_node_bounds(app, node, &rect) == CUI_BRIDGE_OK);
    return rect;
}

static void click_at(cui_bridge_app *app, float x, float y) {
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    step(app);
}

static void click(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect box = bounds(app, node);
    click_at(app, box.x + box.width / 2, box.y + box.height / 2);
}

static void key(cui_bridge_app *app, cui_bridge_key key) {
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, key, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_UP, key, 0, 0) == CUI_BRIDGE_OK);
    step(app);
}

static cui_bridge_event_ex next_event(cui_bridge_app *app, cui_bridge_node node,
                                      cui_bridge_event_type type) {
    cui_bridge_event_ex event;
    char payload[8];
    uint32_t required;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(event.type == type && event.node.slot == node.slot &&
           event.node.generation == node.generation);
    return event;
}

static cui_bridge_event_ex expect_close(cui_bridge_app *app, cui_bridge_node node,
                                        uint32_t reason) {
    cui_bridge_notification_v1 event;
    char payload[8];
    uint32_t required;
    assert(cui_bridge_poll_notification_v1(app, sizeof event, &event, payload, sizeof payload,
                                           &required) == CUI_BRIDGE_OK);
    assert(event.kind == CUI_BRIDGE_NOTIFY_LIFECYCLE &&
           event.payload_kind == CUI_BRIDGE_PAYLOAD_SURFACE);
    assert(event.node.slot == node.slot && event.node.generation == node.generation);
    assert(event.payload.surface_result == reason && event.legacy.value == reason);
    assert(!is_open(app, node));
    return event.legacy;
}

static void expect_empty(cui_bridge_app *app) {
    cui_bridge_event_ex event;
    uint32_t required;
    char payload[8];
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_EMPTY);
}

static void test_dialog(cui_bridge_app *app, cui_bridge_node root, cui_bridge_node trigger) {
    cui_bridge_node dialog = make_node(app, root, CUI_BRIDGE_DIALOG, "Workspace settings");
    assert(cui_bridge_set_detail_text(app, dialog, "Settings description") == CUI_BRIDGE_OK);
    cui_bridge_node direct = make_node(app, dialog, CUI_BRIDGE_LABEL, "Existing body child");
    cui_bridge_dialog_config_v1 config = {
        .flags = CUI_BRIDGE_DIALOG_CENTERED_V1, .width = 350, .height = 270, .margin = 12};
    cui_bridge_node unchanged = root;
    assert(cui_bridge_dialog_slot_create_v1(app, dialog, CUI_BRIDGE_DIALOG_HEADER_V1, "header",
                                            &unchanged) == CUI_BRIDGE_INVALID_STATE);
    set_open(app, dialog, true);
    assert(cui_bridge_dialog_configure_v1(app, dialog, sizeof config, &config) ==
           CUI_BRIDGE_INVALID_STATE);
    set_open(app, dialog, false);
    expect_close(app, dialog, CUI_BRIDGE_SURFACE_CLOSED);
    assert(cui_bridge_dialog_configure_v1(app, trigger, sizeof config, &config) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_dialog_configure_v1(app, dialog, sizeof config, &config) == CUI_BRIDGE_OK);
    cui_bridge_dialog_config_v1 invalid = config;
    invalid.height = NAN;
    assert(cui_bridge_dialog_configure_v1(app, dialog, sizeof invalid, &invalid) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = config;
    invalid.flags |= CUI_BRIDGE_ALERT_DIALOG_SHOW_CLOSE_V1;
    assert(cui_bridge_dialog_configure_v1(app, dialog, sizeof invalid, &invalid) ==
           CUI_BRIDGE_INVALID_ARGUMENT);

    /* Author in reverse layout order. The named Body and legacy child both appear. */
    cui_bridge_node footer = make_slot(app, dialog, CUI_BRIDGE_DIALOG_FOOTER_V1, "footer");
    cui_bridge_node save = make_node(app, footer, CUI_BRIDGE_BUTTON, "Save settings");
    cui_bridge_node body = make_slot(app, dialog, CUI_BRIDGE_DIALOG_BODY_V1, "body");
    for (int i = 0; i < 24; ++i) {
        char line[32];
        (void)snprintf(line, sizeof line, "Setting row %d", i);
        make_node(app, body, CUI_BRIDGE_LABEL, line);
    }
    cui_bridge_node content = make_slot(app, dialog, CUI_BRIDGE_DIALOG_CONTENT_V1, "content");
    make_node(app, content, CUI_BRIDGE_LABEL, "General settings");
    cui_bridge_node description =
        make_slot(app, dialog, CUI_BRIDGE_DIALOG_DESCRIPTION_V1, "description");
    make_node(app, description, CUI_BRIDGE_LABEL, "Edit permissions for your team");
    cui_bridge_node title = make_slot(app, dialog, CUI_BRIDGE_DIALOG_TITLE_V1, "title");
    make_node(app, title, CUI_BRIDGE_LABEL, "Custom settings title");
    cui_bridge_node header = make_slot(app, dialog, CUI_BRIDGE_DIALOG_HEADER_V1, "header");
    make_node(app, header, CUI_BRIDGE_LABEL, "Account");
    assert(cui_bridge_dialog_slot_create_v1(app, dialog, CUI_BRIDGE_DIALOG_HEADER_V1, "duplicate",
                                            &unchanged) == CUI_BRIDGE_INVALID_STATE);
    assert(unchanged.slot == root.slot && unchanged.generation == root.generation);
    assert(cui_bridge_dialog_set_action_v1(app, trigger, CUI_BRIDGE_DIALOG_ACTION_CONFIRM_V1) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_dialog_set_action_v1(app, save, CUI_BRIDGE_DIALOG_ACTION_CONFIRM_V1) ==
           CUI_BRIDGE_OK);

    step(app);
    click(app, trigger);
    next_event(app, trigger, CUI_BRIDGE_EVENT_CLICK);
    config.width = 1; /* The bridge owns the original copy. */
    set_open(app, dialog, true);
    cui_bridge_rect panel = bounds(app, dialog), fixed = bounds(app, save);
    assert(panel.width == 350 && panel.height == 270 && panel.y == 105);
    assert(bounds(app, header).y < bounds(app, title).y);
    assert(bounds(app, title).y < bounds(app, description).y);
    assert(bounds(app, description).y < bounds(app, content).y);
    assert(bounds(app, direct).y < bounds(app, body).y && bounds(app, body).y < fixed.y);
    assert(fixed.y + fixed.height < panel.y + panel.height);
    cui_bridge_node_record *owner = &app->nodes[dialog.slot];
    cui_rect viewport = owner->dialog->layout.completed[5];
    assert(viewport.h > 0 && viewport.y + viewport.h <= fixed.y);
    assert(cui_bridge_queue_scroll(app, viewport.x + 4, viewport.y + 4, 0, -3) == CUI_BRIDGE_OK);
    step(app);
    assert(owner->surface_state.body_scroll > 0 && bounds(app, save).y == fixed.y);
    assert(cui_bridge_dialog_configure_v1(app, dialog, 0, NULL) == CUI_BRIDGE_INVALID_STATE);

    assert(cui_bridge_set_disabled(app, footer, 1) == CUI_BRIDGE_OK);
    step(app);
    click(app, save);
    assert(is_open(app, dialog));
    expect_empty(app);
    assert(cui_bridge_set_disabled(app, footer, 0) == CUI_BRIDGE_OK);
    step(app);
    click(app, save);
    cui_bridge_event_ex activation = next_event(app, save, CUI_BRIDGE_EVENT_CLICK);
    cui_bridge_event_ex close = expect_close(app, dialog, CUI_BRIDGE_SURFACE_ACCEPTED);
    assert(activation.frame_sequence == close.frame_sequence);
    assert(cui_focused(app->ui) == app->nodes[trigger.slot].id);
    expect_empty(app);

    config.width = 310;
    config.flags |=
        CUI_BRIDGE_DIALOG_NO_ESCAPE_DISMISS_V1 | CUI_BRIDGE_DIALOG_NO_OUTSIDE_DISMISS_V1;
    set_open(app, dialog, true);
    assert(owner->surface_state.body_scroll == 0);
    assert(cui_bridge_dialog_configure_v1(app, dialog, sizeof config, &config) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, dialog).width == 310);
    key(app, CUI_BRIDGE_KEY_ESCAPE);
    click_at(app, 2, 2);
    assert(is_open(app, dialog));
    expect_empty(app);

    /* Reset to an ordinary Button and let the app decide after its click. */
    assert(cui_bridge_dialog_set_action_v1(app, save, CUI_BRIDGE_DIALOG_ACTION_NONE_V1) ==
           CUI_BRIDGE_OK);
    click(app, save);
    next_event(app, save, CUI_BRIDGE_EVENT_CLICK);
    assert(is_open(app, dialog));
    expect_empty(app);
    assert(cui_focused(app->ui) == app->nodes[save.slot].id);
    assert(owner->surface_state.overlay.trigger == app->nodes[trigger.slot].id);
    assert(cui_focus_within(app->ui, owner->surface_state.overlay.id));
    assert(cui_bridge_node_remove(app, dialog) == CUI_BRIDGE_OK);
    assert(app->event_count == 0);
    assert(cui_focused(app->ui) == app->nodes[trigger.slot].id);
    step(app);
    assert(cui_focused(app->ui) == app->nodes[trigger.slot].id);
    expect_empty(app);
}

static void test_alert(cui_bridge_app *app, cui_bridge_node root) {
    cui_bridge_node alert = make_node(app, root, CUI_BRIDGE_ALERT_DIALOG, "Remove account?");
    cui_bridge_dialog_config_v1 config = {.flags = CUI_BRIDGE_DIALOG_CENTERED_V1};
    assert(cui_bridge_dialog_configure_v1(app, alert, sizeof config, &config) == CUI_BRIDGE_OK);
    set_open(app, alert, true);
    assert(bounds(app, alert).height < 150);
    click_at(app, 2, 2);
    assert(is_open(app, alert));
    expect_empty(app);
    cui_id id = app->nodes[alert.slot].surface_state.overlay.id;
    cui_rect confirm = cui_bounds(app->ui, cui_surface_part_id(id, 2));
    assert(confirm.w > 0 && confirm.h > 0);
    click_at(app, confirm.x + confirm.w / 2, confirm.y + confirm.h / 2);
    expect_close(app, alert, CUI_BRIDGE_SURFACE_ACCEPTED);
    expect_empty(app);

    cui_bridge_node footer = make_slot(app, alert, CUI_BRIDGE_DIALOG_FOOTER_V1, "footer");
    cui_bridge_node cancel = make_node(app, footer, CUI_BRIDGE_BUTTON, "Keep account");
    assert(cui_bridge_dialog_set_action_v1(app, cancel, CUI_BRIDGE_DIALOG_ACTION_CANCEL_V1) ==
           CUI_BRIDGE_OK);
    set_open(app, alert, true);
    step(app);
    cui_bridge_rect box = bounds(app, alert), button = bounds(app, cancel);
    assert(fabsf(box.x + box.width / 2 - button.x - button.width / 2) < 1);
    assert(cui_bounds(app->ui, cui_surface_part_id(id, 2)).w == 0);
    key(app, CUI_BRIDGE_KEY_TAB);
    assert(cui_focused(app->ui) == app->nodes[cancel.slot].id);
    key(app, CUI_BRIDGE_KEY_ENTER);
    next_event(app, cancel, CUI_BRIDGE_EVENT_CLICK);
    expect_close(app, alert, CUI_BRIDGE_SURFACE_CANCELLED);
    expect_empty(app);

    config.flags |=
        CUI_BRIDGE_ALERT_DIALOG_OUTSIDE_DISMISS_V1 | CUI_BRIDGE_ALERT_DIALOG_SHOW_CLOSE_V1;
    assert(cui_bridge_dialog_configure_v1(app, alert, sizeof config, &config) == CUI_BRIDGE_OK);
    set_open(app, alert, true);
    click_at(app, 2, 2);
    expect_close(app, alert, CUI_BRIDGE_SURFACE_CANCELLED);
    set_open(app, alert, true);
    cui_rect close = cui_bounds(app->ui, cui_surface_part_id(id, 1));
    assert(close.w > 0 && close.h > 0);
    click_at(app, close.x + close.w / 2, close.y + close.h / 2);
    expect_close(app, alert, CUI_BRIDGE_SURFACE_CLOSED);
    assert(cui_bridge_dialog_configure_v1(app, alert, 0, NULL) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, footer) == CUI_BRIDGE_OK);
    assert(cui_bridge_dialog_configure_v1(app, alert, 0, NULL) == CUI_BRIDGE_OK);
    set_open(app, alert, true);
    key(app, CUI_BRIDGE_KEY_ESCAPE);
    expect_close(app, alert, CUI_BRIDGE_SURFACE_CANCELLED);
    expect_empty(app);
}

static void test_nested_escape(cui_bridge_app *app, cui_bridge_node root, cui_bridge_node trigger) {
    cui_bridge_node parent = make_node(app, root, CUI_BRIDGE_DIALOG, "Workspace");
    cui_bridge_node launch = make_node(app, parent, CUI_BRIDGE_BUTTON, "Edit workspace name");
    cui_bridge_node child = make_node(app, parent, CUI_BRIDGE_DIALOG, "Workspace name");
    cui_bridge_dialog_config_v1 config = {.width = 340, .height = 220};
    assert(cui_bridge_dialog_configure_v1(app, parent, sizeof config, &config) == CUI_BRIDGE_OK);
    config.width = 260;
    assert(cui_bridge_dialog_configure_v1(app, child, sizeof config, &config) == CUI_BRIDGE_OK);
    step(app);
    click(app, trigger);
    next_event(app, trigger, CUI_BRIDGE_EVENT_CLICK);
    set_open(app, parent, true);
    click(app, launch);
    next_event(app, launch, CUI_BRIDGE_EVENT_CLICK);
    set_open(app, child, true);
    assert(bounds(app, child).width == 260);
    key(app, CUI_BRIDGE_KEY_ESCAPE);
    expect_close(app, child, CUI_BRIDGE_SURFACE_CANCELLED);
    assert(is_open(app, parent) && cui_focused(app->ui) == app->nodes[launch.slot].id);
    key(app, CUI_BRIDGE_KEY_ESCAPE);
    expect_close(app, parent, CUI_BRIDGE_SURFACE_CANCELLED);
    assert(cui_focused(app->ui) == app->nodes[trigger.slot].id);
    expect_empty(app);
}

int main(void) {
    cui_bridge_options option = {.struct_size = sizeof option,
                                 .abi_version = CUI_BRIDGE_ABI_VERSION,
                                 .width = 720,
                                 .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&option, &status);
    assert(status == CUI_BRIDGE_OK && app);
    cui_bridge_node root = make_node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node trigger = make_node(app, root, CUI_BRIDGE_BUTTON, "Open settings");
    test_dialog(app, root, trigger);
    test_alert(app, root);
    test_nested_escape(app, root, trigger);
    cui_bridge_destroy(app);
}
