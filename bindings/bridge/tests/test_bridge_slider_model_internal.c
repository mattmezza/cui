/* SPDX-License-Identifier: MIT */
/* Test-only same-TU access, no production test hooks or exported internals. */
#include <stdlib.h>
static int fail_next_bridge_allocation;
static void *test_bridge_calloc(size_t count, size_t size) {
    if (fail_next_bridge_allocation) {
        fail_next_bridge_allocation = 0;
        return NULL;
    }
    return calloc(count, size);
}
#define calloc test_bridge_calloc
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../../src/bridge/cui_bridge.c"
#undef calloc
#include <assert.h>

/* Test-only native boundary implementation: no platform library or window. */
static bool fail_render;
static unsigned render_calls;
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *app) {
    (void)app;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_native_render(cui_bridge_app *app, const cui_draw_list *list) {
    assert(app->stepping && list->count > 0);
    render_calls++;
    return fail_render ? CUI_BRIDGE_CORE_ERROR : CUI_BRIDGE_OK;
}
void cui_bridge_native_destroy(cui_bridge_app *app) {
    (void)app;
}
cui_bridge_status cui_bridge_native_create(cui_bridge_app *app, const char *title, const char *font,
                                           cui_config *config, bool hidden) {
    (void)app;
    (void)title;
    (void)font;
    (void)config;
    (void)hidden;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_run(cui_bridge_app *app, cui_bridge_run_callback callback,
                                        void *user) {
    (void)app;
    (void)callback;
    (void)user;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *app,
                                                       const char *const *paths, uint32_t count) {
    (void)app;
    (void)paths;
    (void)count;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *app, cui_str text) {
    (void)app;
    (void)text;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *app, char *text,
                                                  uint32_t capacity, uint32_t *length) {
    (void)app;
    (void)text;
    (void)capacity;
    (void)length;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_window_visible(cui_bridge_app *app, uint32_t visible) {
    (void)app;
    (void)visible;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_get_window_visible(cui_bridge_app *app, uint32_t *visible) {
    (void)app;
    (void)visible;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static void failure_and_identity(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 500,
                                  .height = 300};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK && !app->slider);
    cui_bridge_node root, node;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    cui_bridge_slider_queue_options_v1 q = {sizeof q, 1, 64, 0};
    uint64_t epoch_before = bridge_slider_next_epoch;
    fail_next_bridge_allocation = 1;
    assert(cui_bridge_slider_queue_enable_v1(app, sizeof q, &q) == CUI_BRIDGE_OUT_OF_MEMORY);
    assert(!app->slider && bridge_slider_next_epoch == epoch_before);
    assert(cui_bridge_slider_queue_enable_v1(app, sizeof q, &q) == CUI_BRIDGE_OK);
    cui_bridge_slider_config_v1 c = {.struct_size = sizeof c,
                                     .schema_version = 1,
                                     .flags = CUI_BRIDGE_SLIDER_REQUESTS |
                                              CUI_BRIDGE_SLIDER_REDUCED_MOTION,
                                     .domain = {0, 100, 1, 0, 0},
                                     .rem = 16,
                                     .length = 200};
    cui_bridge_slider_value_v1 v = {0, 0, 0, 40};
    node = (cui_bridge_node){42, 43};
    fail_next_bridge_allocation = 1;
    assert(cui_bridge_slider_model_create_v1(app, root, "typed", sizeof c, &c, sizeof v, &v,
                                             &node) == CUI_BRIDGE_OUT_OF_MEMORY);
    assert(node.slot == 42 && node.generation == 43);
    app->next_id = UINT64_C(0x100000009);
    assert(cui_bridge_slider_model_create_v1(app, root, "typed", sizeof c, &c, sizeof v, &v,
                                             &node) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    bridge_slider_owned *owned = app->slider->nodes[node.slot];
    /* A typed queue does not manufacture input evidence; actual Core targeted SET. */
    cui_id end =
        cui_base_slider_model_part_id_v1(app->nodes[node.slot].id, CUI_SLIDER_MODEL_END_THUMB);
    assert(end > UINT32_MAX);
    cui_accessibility_set_value_v1 payload = {1, 0, .75};
    assert(cui_queue_accessibility_set_value_v1(app->ui, end, sizeof payload, &payload));
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_slider_request_v1 request, update;
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof update, &update) == CUI_BRIDGE_OK);
    assert(update.phase == CUI_BRIDGE_SLIDER_UPDATE && update.requested.end == 75);
    assert(update.input.flags ==
           (CUI_BRIDGE_SLIDER_HAS_INPUT | CUI_BRIDGE_SLIDER_HAS_NUMERIC_VALUE));
    assert(update.input.accessibility_value == .75 && update.input.accessibility_target == end);
    assert(update.event_source == CUI_BRIDGE_SLIDER_SOURCE_ACCESSIBILITY);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof update.receipt, &update.receipt,
                                               sizeof update.requested,
                                               &update.requested) == CUI_BRIDGE_OK);
    /* Invalid current geometry cannot be revived by clearing the diagnostic. */
    assert(cui_begin_frame(app->ui, 500, 300, .016f));
    cui_begin_box(app->ui, 900, (cui_box){.width = 10, .height = 10});
    cui_end_box(app->ui);
    cui_begin_box(app->ui, 900, (cui_box){.width = 10, .height = 10});
    cui_end_box(app->ui);
    cui_clear_error(app->ui);
    (void)cui_end_frame(app->ui);
    assert(!cui_accessibility_numeric_snapshot_ready(app->ui));
    assert(cui_bridge_accept_slider_request_v1(app, sizeof update.receipt, &update.receipt,
                                               sizeof v, &v) == CUI_BRIDGE_INVALID_STATE);
    cui_clear_error(app->ui);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_slider_presentation_v1 observation;
    assert(cui_bridge_slider_model_get_presentation_v1(app, node, sizeof observation,
                                                       &observation) == CUI_BRIDGE_CORE_ERROR);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_slider_model_get_presentation_v1(app, node, sizeof observation,
                                                       &observation) == CUI_BRIDGE_OK);
    assert(cui_accessibility_numeric_snapshot_ready(app->ui));
    /* Readable-but-expired payload and removal/reuse pruning are separate policies. */
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(request.phase == CUI_BRIDGE_SLIDER_COMMIT && request.requested.end == 75);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                               sizeof v, &v) == CUI_BRIDGE_INVALID_STATE);
    uint64_t revision = owned->revision;
    owned->revision = UINT64_MAX;
    assert(cui_bridge_slider_model_set_value_v1(app, node, sizeof v, &v) == CUI_BRIDGE_CAPACITY);
    assert(owned->value.end == 75 && owned->revision == UINT64_MAX);
    owned->revision = revision;
    payload.value = .8;
    assert(cui_queue_accessibility_set_value_v1(app->ui, end, sizeof payload, &payload));
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, node) == CUI_BRIDGE_OK);
    cui_bridge_node replacement;
    assert(cui_bridge_slider_model_create_v1(app, root, "typed", sizeof c, &c, sizeof v, &v,
                                             &replacement) == CUI_BRIDGE_OK);
    assert(replacement.slot == node.slot && replacement.generation != node.generation);
    memset(&request, 0xab, sizeof request);
    unsigned char bytes[sizeof request];
    memcpy(bytes, &request, sizeof request);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_EMPTY);
    assert(!memcmp(bytes, &request, sizeof request));
    owned = app->slider->nodes[replacement.slot];
    cui_slider_model_request_v1 source = {0};
    source.phase = CUI_SLIDER_MODEL_UPDATE;
    source.thumb = CUI_SLIDER_MODEL_END;
    source.event.kind = CUI_COMPONENT_EVENT_REQUEST;
    source.event.source = CUI_COMPONENT_EVENT_SOURCE_KEYBOARD;
    source.input.flags = CUI_SLIDER_MODEL_HAS_INPUT;
    source.input.type = CUI_EVENT_KEY_DOWN;
    source.input.key = CUI_KEY_RIGHT;
    source.input.modifiers = CUI_MOD_SHIFT | CUI_MOD_CTRL | CUI_MOD_ALT | CUI_MOD_META;
    source.current = source.requested = bridge_slider_value(v);
    source.domain = source.gesture_domain = bridge_slider_domain(c.domain);
    assert(bridge_slider_map_request(owned, &source, &request));
    assert(request.input.modifiers ==
           (CUI_BRIDGE_MOD_SHIFT | CUI_BRIDGE_MOD_CTRL | CUI_BRIDGE_MOD_ALT | CUI_BRIDGE_MOD_META));
    source.request_sequence = 1;
    bridge_slider_request(owned, &source);
    memcpy(bytes, &request, sizeof request);
    source.requested.kind = UINT32_MAX;
    assert(!bridge_slider_map_request(owned, &source, &request));
    assert(!memcmp(bytes, &request, sizeof request));
    source.requested.kind = CUI_SLIDER_MODEL_SINGLE;
    source.gesture_domain.scale = UINT32_MAX;
    assert(!bridge_slider_map_request(owned, &source, &request));
    assert(!memcmp(bytes, &request, sizeof request));
    source.gesture_domain.scale = CUI_SLIDER_MODEL_LINEAR;
    source.input.flags |= CUI_SLIDER_MODEL_HAS_NUMERIC_VALUE;
    assert(!bridge_slider_map_request(owned, &source, &request));
    assert(!memcmp(bytes, &request, sizeof request));
    source.input.flags = CUI_SLIDER_MODEL_HAS_INPUT;
    source.input.modifiers |= 1u << 20;
    assert(!bridge_slider_map_request(owned, &source, &request));
    assert(!memcmp(bytes, &request, sizeof request));
    source.request_sequence = 2;
    bridge_slider_request(owned, &source);
    assert(owned->emitted_error && owned->first == 1 && owned->last == 1 &&
           app->slider->count == 1);
    assert(app->frame_error == CUI_BRIDGE_CORE_ERROR);
    source.input.modifiers = 0;
    source.request_sequence = 3;
    bridge_slider_request(owned, &source);
    assert(owned->first == 1 && owned->last == 1 && app->slider->count == 1);
    /* A failed map suppresses further publication; no gap can become acceptable. */
    owned->seen_frame = app->slider->frame;
    owned->model.last_painted_frame = owned->seen_frame;
    bridge_slider_finish(app, app->frame_error);
    assert(!owned->window);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                               sizeof v, &v) == CUI_BRIDGE_INVALID_STATE);
    /* Global epoch exhaustion issues MAX once, then leaves caller output unchanged. */
    bridge_slider_next_epoch = UINT64_MAX;
    bridge_slider_epoch_exhausted = false;
    uint64_t epoch = 0;
    assert(bridge_slider_epoch(&epoch) && epoch == UINT64_MAX);
    epoch = 123;
    assert(!bridge_slider_epoch(&epoch) && epoch == 123);
    cui_bridge_destroy(app);
}
static void partial_frame_failure(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 500,
                                  .height = 300};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, first, second;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    cui_bridge_slider_queue_options_v1 q = {sizeof q, 1, 4, 0};
    assert(cui_bridge_slider_queue_enable_v1(app, sizeof q, &q) == CUI_BRIDGE_OK);
    cui_bridge_slider_config_v1 c = {.struct_size = sizeof c,
                                     .schema_version = 1,
                                     .flags = CUI_BRIDGE_SLIDER_REQUESTS |
                                              CUI_BRIDGE_SLIDER_REDUCED_MOTION,
                                     .domain = {0, 100, 1, 0, 0},
                                     .rem = 16,
                                     .length = 200};
    cui_bridge_slider_value_v1 v = {0, 0, 0, 40};
    assert(cui_bridge_slider_model_create_v1(app, root, "first", sizeof c, &c, sizeof v, &v,
                                             &first) == CUI_BRIDGE_OK);
    assert(cui_bridge_slider_model_create_v1(app, root, "second", sizeof c, &c, sizeof v, &v,
                                             &second) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_id end =
        cui_base_slider_model_part_id_v1(app->nodes[first.slot].id, CUI_SLIDER_MODEL_END_THUMB);
    cui_accessibility_set_value_v1 payload = {1, 0, .75};
    assert(cui_queue_accessibility_set_value_v1(app->ui, end, sizeof payload, &payload));
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CAPACITY);
    assert(cui_last_error(app->ui) == CUI_OK && cui_accessibility_numeric_snapshot_ready(app->ui));
    assert(app->slider->count == 3 &&
           app->slider->nodes[first.slot]->model.last_painted_frame == app->slider->frame);
    cui_bridge_slider_request_v1 request;
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(request.requested.end == 75);
    for (unsigned clear = 0; clear < 2; ++clear) {
        cui_bridge_slider_presentation_v1 p;
        memset(&p, 0xa5, sizeof p);
        unsigned char before[sizeof p];
        memcpy(before, &p, sizeof p);
        assert(cui_bridge_slider_model_get_presentation_v1(app, first, sizeof p, &p) ==
               CUI_BRIDGE_CORE_ERROR);
        assert(!memcmp(before, &p, sizeof p));
        cui_bridge_rect bounds = {1, 2, 3, 4};
        assert(cui_bridge_slider_model_part_bounds_v1(app, first, CUI_BRIDGE_SLIDER_END_THUMB,
                                                      &bounds) == CUI_BRIDGE_CORE_ERROR);
        assert(bounds.x == 1 && bounds.height == 4);
        assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                                   sizeof v, &v) == CUI_BRIDGE_INVALID_STATE);
        cui_clear_error(app->ui);
    }
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_slider_presentation_v1 p;
    assert(cui_bridge_slider_model_get_presentation_v1(app, first, sizeof p, &p) == CUI_BRIDGE_OK);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                               sizeof v, &v) == CUI_BRIDGE_INVALID_STATE);
    /* One writable model now fits the same bounded queue. */
    c.flags &= ~CUI_BRIDGE_SLIDER_REQUESTS;
    assert(cui_bridge_slider_model_configure_v1(app, second, sizeof c, &c, sizeof v, &v) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    /* Actual public step path reaches injected native rendering AFTER Core end. */
    app->backend = CUI_BRIDGE_BACKEND_SDL;
    fail_render = true;
    assert(cui_queue_accessibility_set_value_v1(app->ui, end, sizeof payload, &payload));
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CORE_ERROR && render_calls == 1);
    assert(cui_accessibility_numeric_snapshot_ready(app->ui) && cui_last_error(app->ui) == CUI_OK);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                               sizeof request.requested,
                                               &request.requested) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_slider_model_get_presentation_v1(app, first, sizeof p, &p) ==
           CUI_BRIDGE_CORE_ERROR);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    fail_render = false;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK && render_calls == 2);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                               sizeof request.requested,
                                               &request.requested) == CUI_BRIDGE_INVALID_STATE);
    app->backend = CUI_BRIDGE_BACKEND_NONE;
    payload.value = .5;
    assert(cui_queue_accessibility_set_value_v1(app->ui, end, sizeof payload, &payload));
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(request.requested.end == 50);
    unsigned queued = app->slider->count;
    unsigned char request_bytes[sizeof request];
    memcpy(request_bytes, &request, sizeof request);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request - 1, &request) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(app->slider->count == queued && !memcmp(request_bytes, &request, sizeof request));
    cui_bridge_slider_presentation_v1 before_layout;
    assert(cui_bridge_slider_model_get_presentation_v1(app, first, sizeof before_layout,
                                                       &before_layout) == CUI_BRIDGE_OK);
    /* Test-only phase composition: bridge public authoring has no layout-only setter. */
    assert(cui_begin_frame(app->ui, 500, 300, .016f));
    bridge_slider_begin(app);
    cui_begin_box(app->ui, 9000, (cui_box){.width = 500, .height = 300, .layout_only = true});
    bridge_slider_render(app, first.slot);
    cui_end_box(app->ui);
    (void)cui_end_frame(app->ui);
    assert(cui_last_error(app->ui) == CUI_OK);
    bridge_slider_finish(app, CUI_BRIDGE_OK);
    assert(cui_bridge_slider_model_get_presentation_v1(app, first, sizeof p, &p) == CUI_BRIDGE_OK);
    assert(p.core_time == before_layout.core_time); /* No renewed observation. */
    assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                               sizeof request.requested,
                                               &request.requested) == CUI_BRIDGE_OK);
    /* A real ordinary omitted subtree expires authority without destroying the node. */
    app->nodes[root.slot].first_child = second.slot;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof request.receipt, &request.receipt,
                                               sizeof request.requested,
                                               &request.requested) == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_rect absent = {1, 2, 3, 4};
    assert(cui_bridge_slider_model_part_bounds_v1(app, first, CUI_BRIDGE_SLIDER_END_THUMB,
                                                  &absent) == CUI_BRIDGE_EMPTY);
    assert(absent.x == 1 && absent.height == 4);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_OK);
    assert(request.phase == CUI_BRIDGE_SLIDER_COMMIT && request.requested.end == 50);
    app->nodes[root.slot].first_child = first.slot;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_slider_model_part_bounds_v1(app, first, CUI_BRIDGE_SLIDER_END_THUMB,
                                                  &absent) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
}
int main(void) {
    partial_frame_failure();
    failure_and_identity();
    puts("typed bridge Core evidence, invalidity, identity/reuse and exhaustion PASS");
    return 0;
}
