/* SPDX-License-Identifier: MIT */
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include <assert.h>

static cui_bridge_node make_node(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                                 const char *name) {
    cui_bridge_node node;
    assert(cui_bridge_node_create(app, parent, kind, name, &node) == CUI_BRIDGE_OK);
    if (kind != CUI_BRIDGE_COLUMN)
        assert(cui_bridge_set_text(app, node, name) == CUI_BRIDGE_OK);
    return node;
}

static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}

static void key(cui_bridge_app *app, uint32_t key) {
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, key, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_UP, key, 0, 0) == CUI_BRIDGE_OK);
    step(app);
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect rect;
    assert(cui_bridge_node_bounds(app, node, &rect) == CUI_BRIDGE_OK);
    return rect;
}

static void click(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect r = bounds(app, node);
    float x = r.x + r.width / 2, y = r.y + r.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    step(app);
}

static bool is_open(cui_bridge_app *app, cui_bridge_node popover) {
    uint32_t open;
    assert(cui_bridge_get_open(app, popover, &open) == CUI_BRIDGE_OK);
    return open != 0;
}

static void expect_link(cui_bridge_app *app, cui_bridge_node anchor, cui_id controls) {
    size_t count;
    const cui_semantic *semantics = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; ++i) {
        const cui_semantic *node = &semantics[i];
        if (node->id == app->nodes[anchor.slot].id) {
            assert(node->details.controls == controls && node->details.expanded == (controls != 0));
            return;
        }
    }
    assert(false);
}

static bool take_event(cui_bridge_app *app, cui_bridge_node node, cui_bridge_event_type kind,
                       uint32_t value) {
    cui_bridge_event_ex event;
    uint32_t required;
    char payload[8];
    cui_bridge_status status =
        cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required);
    if (status == CUI_BRIDGE_EMPTY)
        return false;
    assert(status == CUI_BRIDGE_OK && event.type == kind && event.node.slot == node.slot);
    assert(event.value == value);
    return true;
}

static void test_options(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 720,
                                  .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = make_node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    assert(cui_bridge_set_gap(app, root, 10) == CUI_BRIDGE_OK);
    /* Deliberately submit the panel before its two possible anchors. */
    cui_bridge_node panel = make_node(app, root, CUI_BRIDGE_POPOVER, "Account actions");
    cui_bridge_node disabled = make_node(app, panel, CUI_BRIDGE_BUTTON, "Unavailable");
    cui_bridge_node profile = make_node(app, panel, CUI_BRIDGE_BUTTON, "Profile");
    cui_bridge_node account = make_node(app, root, CUI_BRIDGE_BUTTON, "Account");
    cui_bridge_node outside = make_node(app, root, CUI_BRIDGE_BUTTON, "Other");
    assert(cui_bridge_set_disabled(app, disabled, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_INVALID_STATE);

    cui_bridge_popover_config_v1 config = {
        .anchor = CUI_BRIDGE_POPOVER_LEFT_CENTER_V1,
        .flags = CUI_BRIDGE_POPOVER_ARROW_V1 | CUI_BRIDGE_POPOVER_FOCUS_CONTENT_V1 |
                 CUI_BRIDGE_POPOVER_KEEP_POINTER_FOCUS_V1 | CUI_BRIDGE_POPOVER_CUSTOM_OFFSET_V1,
        .width = 260,
        .height = 176,
        .margin = 8,
        .offset = 7};
    assert(cui_bridge_popover_configure_v1(app, account, sizeof config, &config) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof config, NULL) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof config - 1, &config) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof config, &config) == CUI_BRIDGE_OK);

    cui_bridge_popover_config_v1 bad = config;
    bad.flags |= (1u << 31);
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof bad, &bad) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    bad = config;
    bad.width = NAN;
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof bad, &bad) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    bad = config;
    bad.flags &= ~CUI_BRIDGE_POPOVER_CUSTOM_OFFSET_V1;
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof bad, &bad) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    bad = config;
    bad.anchor = 8;
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof bad, &bad) ==
           CUI_BRIDGE_INVALID_ARGUMENT);

    assert(cui_bridge_set_anchor(app, panel, account) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    step(app);
    cui_bridge_rect unavailable;
    assert(is_open(app, panel) && cui_needs_redraw(app->ui));
    assert(cui_bridge_node_bounds(app, panel, &unavailable) == CUI_BRIDGE_INVALID_STATE);
    expect_link(app, account, 0);
    step(app);
    cui_id panel_id = app->nodes[panel.slot].surface_state.overlay.id;
    cui_id account_id = app->nodes[account.slot].id;
    cui_bridge_rect trigger_rect = bounds(app, account), rect = bounds(app, panel);
    assert(rect.width == config.width && rect.height == config.height);
    assert(fabsf(rect.x - trigger_rect.x - trigger_rect.width - 13) < .1f);
    expect_link(app, account, panel_id);
    assert(cui_focused(app->ui) == app->nodes[profile.slot].id);
    key(app, CUI_BRIDGE_KEY_ENTER);
    assert(take_event(app, profile, CUI_BRIDGE_EVENT_CLICK, 0));

    /* Live wrapping/position changes are measured once, then the panel settles. */
    assert(cui_bridge_set_text(app, account, "Account for this workspace") == CUI_BRIDGE_OK);
    step(app);
    assert(cui_needs_redraw(app->ui) && bounds(app, panel).x == rect.x);
    step(app);
    trigger_rect = bounds(app, account);
    rect = bounds(app, panel);
    assert(fabsf(rect.x - trigger_rect.x - trigger_rect.width - 13) < .1f);
    step(app);
    assert(!cui_needs_redraw(app->ui));

    key(app, CUI_BRIDGE_KEY_ESCAPE);
    assert(!is_open(app, panel) && cui_focused(app->ui) == account_id);
    expect_link(app, account, 0);
    assert(take_event(app, panel, CUI_BRIDGE_EVENT_CLOSE, CUI_SURFACE_CANCELLED));
    assert(cui_bridge_node_bounds(app, panel, &unavailable) == CUI_BRIDGE_INVALID_STATE);

    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    step(app);
    click(app, outside);
    assert(!is_open(app, panel) && cui_focused(app->ui) == app->nodes[outside.slot].id);
    assert(take_event(app, panel, CUI_BRIDGE_EVENT_CLOSE, CUI_SURFACE_CANCELLED));
    assert(take_event(app, outside, CUI_BRIDGE_EVENT_CLICK, 0));

    config.flags |=
        CUI_BRIDGE_POPOVER_NO_ESCAPE_DISMISS_V1 | CUI_BRIDGE_POPOVER_NO_OUTSIDE_DISMISS_V1;
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof config, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    step(app);
    key(app, CUI_BRIDGE_KEY_ESCAPE);
    click(app, outside);
    assert(is_open(app, panel) && take_event(app, outside, CUI_BRIDGE_EVENT_CLICK, 0));
    assert(cui_bridge_set_anchor(app, panel, outside) == CUI_BRIDGE_OK);
    step(app);
    expect_link(app, account, 0);
    step(app);
    expect_link(app, outside, panel_id);
    assert(cui_bridge_node_remove(app, outside) == CUI_BRIDGE_OK);
    step(app);
    assert(!is_open(app, panel));
    assert(take_event(app, panel, CUI_BRIDGE_EVENT_CLOSE, CUI_SURFACE_CLOSED));

    assert(cui_bridge_popover_configure_v1(app, panel, 0, NULL) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_anchor(app, panel, account) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, panel, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(is_open(app, panel));
    expect_link(app, account, 0);
    assert(cui_bridge_node_remove(app, panel) == CUI_BRIDGE_OK);
    assert(cui_bridge_popover_configure_v1(app, panel, sizeof config, &config) ==
           CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}

static void test_remove_open_popover(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 640,
                                  .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = make_node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node trigger = make_node(app, root, CUI_BRIDGE_BUTTON, "Account");
    cui_bridge_node popup = make_node(app, root, CUI_BRIDGE_POPOVER, "Account actions");
    cui_bridge_node action = make_node(app, popup, CUI_BRIDGE_BUTTON, "Profile");
    cui_bridge_popover_config_v1 config = {.flags = CUI_BRIDGE_POPOVER_FOCUS_CONTENT_V1};
    assert(cui_bridge_popover_configure_v1(app, popup, sizeof config, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_anchor(app, popup, trigger) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, popup, 1) == CUI_BRIDGE_OK);
    step(app);
    step(app);
    assert(cui_focused(app->ui) == app->nodes[action.slot].id);
    key(app, CUI_BRIDGE_KEY_ENTER);
    assert(take_event(app, action, CUI_BRIDGE_EVENT_CLICK, 0));
    assert(cui_bridge_node_remove(app, popup) == CUI_BRIDGE_OK);
    assert(cui_needs_redraw(app->ui));
    step(app);
    assert(cui_focused(app->ui) == app->nodes[trigger.slot].id);
    expect_link(app, trigger, 0);
    assert(!take_event(app, popup, CUI_BRIDGE_EVENT_CLOSE, 0));
    cui_bridge_destroy(app);
}

static void test_hidden_anchor(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 640,
                                  .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = make_node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node collapse = make_node(app, root, CUI_BRIDGE_COLLAPSIBLE, "Account");
    cui_bridge_node trigger = make_node(app, collapse, CUI_BRIDGE_BUTTON, "Settings");
    cui_bridge_node popup = make_node(app, root, CUI_BRIDGE_POPOVER, "Settings actions");
    cui_bridge_popover_config_v1 config = {.width = 180, .height = 135};
    assert(cui_bridge_set_anchor(app, popup, trigger) == CUI_BRIDGE_OK);
    assert(cui_bridge_popover_configure_v1(app, popup, sizeof config, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, popup, 1) == CUI_BRIDGE_OK);
    cui_bridge_rect unused;
    step(app);
    step(app);
    assert(is_open(app, popup));
    assert(cui_bridge_node_bounds(app, popup, &unused) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_open(app, collapse, 1) == CUI_BRIDGE_OK);
    step(app);
    step(app);
    assert(bounds(app, popup).width == 180);
    assert(cui_bridge_set_open(app, collapse, 0) == CUI_BRIDGE_OK);
    step(app);
    step(app);
    assert(is_open(app, popup));
    assert(cui_bridge_node_bounds(app, popup, &unused) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_open(app, popup, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(!is_open(app, popup));
    assert(take_event(app, popup, CUI_BRIDGE_EVENT_CLOSE, CUI_SURFACE_CLOSED));
    cui_bridge_destroy(app);
}

static void queue_button_at(cui_bridge_app *app, cui_bridge_rect rect,
                            cui_bridge_pointer_button button) {
    float x = rect.x + rect.width / 2, y = rect.y + rect.height / 2;
    assert(cui_bridge_queue_pointer_button_v1(app, CUI_BRIDGE_POINTER_DOWN, button, x, y) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer_button_v1(app, CUI_BRIDGE_POINTER_UP, button, x, y) ==
           CUI_BRIDGE_OK);
}

static void test_secondary_trigger(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 640,
                                  .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = make_node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node popup = make_node(app, root, CUI_BRIDGE_POPOVER, "Account actions");
    cui_bridge_node profile = make_node(app, popup, CUI_BRIDGE_BUTTON, "Profile");
    cui_bridge_node collapse = make_node(app, root, CUI_BRIDGE_COLLAPSIBLE, "Workspace");
    cui_bridge_node trigger = make_node(app, collapse, CUI_BRIDGE_BUTTON, "Account");
    cui_bridge_node another = make_node(app, root, CUI_BRIDGE_POPOVER, "Other account actions");
    cui_bridge_node blocker = make_node(app, root, CUI_BRIDGE_DIALOG, "Modal");
    cui_bridge_popover_config_v1 config = {
        .width = 180,
        .height = 135,
        .flags = CUI_BRIDGE_POPOVER_FOCUS_CONTENT_V1,
    };
    assert(cui_bridge_set_anchor(app, popup, trigger) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_anchor(app, another, trigger) == CUI_BRIDGE_OK);
    assert(cui_bridge_popover_configure_v1(app, popup, sizeof config, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, collapse, 1) == CUI_BRIDGE_OK);
    step(app);
    cui_bridge_rect target = bounds(app, trigger);
    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    step(app);
    assert(!is_open(app, popup));
    assert(!take_event(app, trigger, CUI_BRIDGE_EVENT_CLICK, 0));

    config.flags |= CUI_BRIDGE_POPOVER_SECONDARY_TRIGGER_V1;
    assert(cui_bridge_popover_configure_v1(app, popup, sizeof config, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_popover_configure_v1(app, another, sizeof config, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer_button_v1(app, CUI_BRIDGE_POINTER_DOWN,
                                              (cui_bridge_pointer_button)3, 8,
                                              8) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_queue_pointer_button_v1(app, CUI_BRIDGE_POINTER_MOVE,
                                              CUI_BRIDGE_POINTER_SECONDARY, 8,
                                              8) == CUI_BRIDGE_INVALID_ARGUMENT);
    queue_button_at(app, target, CUI_BRIDGE_POINTER_MIDDLE);
    step(app);
    assert(!is_open(app, popup));
    click(app, trigger);
    assert(!is_open(app, popup) && take_event(app, trigger, CUI_BRIDGE_EVENT_CLICK, 0));
    key(app, CUI_BRIDGE_KEY_ENTER);
    assert(!is_open(app, popup) && take_event(app, trigger, CUI_BRIDGE_EVENT_CLICK, 0));

    /* Disabling or omitting the current trigger rejects a press already
     * routed to its last completed snapshot. */
    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    assert(cui_bridge_set_disabled(app, trigger, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(!is_open(app, popup) && !is_open(app, another));
    assert(cui_bridge_set_disabled(app, trigger, 0) == CUI_BRIDGE_OK);
    step(app);
    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    assert(cui_bridge_set_open(app, collapse, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(!is_open(app, popup) && !is_open(app, another));
    assert(cui_bridge_set_open(app, collapse, 1) == CUI_BRIDGE_OK);
    step(app);
    target = bounds(app, trigger);

    /* A modal opened since the last snapshot blocks the outside trigger
     * even though it submits later than the trigger and both Popovers. */
    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    assert(cui_bridge_set_open(app, blocker, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(!is_open(app, popup) && !is_open(app, another));
    assert(cui_bridge_set_open(app, blocker, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(take_event(app, blocker, CUI_BRIDGE_EVENT_CLOSE, CUI_SURFACE_CLOSED));

    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    step(app);
    assert(is_open(app, popup) && !is_open(app, another));
    assert(cui_needs_redraw(app->ui));
    cui_bridge_rect unavailable;
    assert(cui_bridge_node_bounds(app, popup, &unavailable) == CUI_BRIDGE_INVALID_STATE);
    assert(!take_event(app, trigger, CUI_BRIDGE_EVENT_CLICK, 0));
    step(app);
    assert(bounds(app, popup).width == config.width);
    expect_link(app, trigger, app->nodes[popup.slot].surface_state.overlay.id);
    assert(cui_focused(app->ui) == app->nodes[profile.slot].id);

    target = bounds(app, trigger);
    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    queue_button_at(app, target, CUI_BRIDGE_POINTER_SECONDARY);
    step(app);
    assert(is_open(app, popup) && !take_event(app, popup, CUI_BRIDGE_EVENT_CLOSE, 0));
    step(app);
    assert(!is_open(app, popup) && !is_open(app, another));
    assert(take_event(app, popup, CUI_BRIDGE_EVENT_CLOSE, CUI_SURFACE_CLOSED));
    assert(cui_focused(app->ui) == app->nodes[trigger.slot].id);
    expect_link(app, trigger, 0);
    step(app);
    assert(!take_event(app, popup, CUI_BRIDGE_EVENT_CLOSE, 0));
    cui_bridge_destroy(app);
}

int main(void) {
    test_options();
    test_secondary_trigger();
    test_hidden_anchor();
    test_remove_open_popover();
    return 0;
}
