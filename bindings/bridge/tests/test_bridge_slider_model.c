/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define OK(call) assert((call) == CUI_BRIDGE_OK)
static cui_bridge_slider_config_v1 config(void) {
    cui_bridge_slider_config_v1 c = {0};
    c.struct_size = sizeof c;
    c.schema_version = 1;
    c.flags = CUI_BRIDGE_SLIDER_REQUESTS | CUI_BRIDGE_SLIDER_REDUCED_MOTION;
    c.source_generation = UINT64_C(0x123456789abcdef0);
    c.domain.maximum = 100;
    c.domain.step = 1;
    c.rem = 16;
    c.length = 200;
    return c;
}
static cui_bridge_app *create(uint32_t capacity, cui_bridge_node *parent) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .backend = CUI_BRIDGE_BACKEND_NONE,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_slider_queue_options_v1 q = {sizeof q, 1, capacity, 0};
    OK(cui_bridge_slider_queue_enable_v1(app, sizeof q, &q));
    OK(cui_bridge_slider_queue_enable_v1(app, sizeof q, &q));
    cui_bridge_node root, row;
    OK(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root));
    OK(cui_bridge_node_create(app, root, CUI_BRIDGE_ROW, "row", &row));
    OK(cui_bridge_node_create(app, row, CUI_BRIDGE_COLUMN, "column", parent));
    return app;
}
static size_t poll(cui_bridge_app *app, cui_bridge_slider_request_v1 *r, size_t capacity) {
    size_t count = 0;
    cui_bridge_slider_request_v1 next;
    cui_bridge_status status;
    while ((status = cui_bridge_poll_slider_request_v1(app, sizeof next, &next)) == CUI_BRIDGE_OK) {
        assert(count < capacity);
        r[count++] = next;
    }
    assert(status == CUI_BRIDGE_EMPTY);
    return count;
}
static void accept(cui_bridge_app *app, const cui_bridge_slider_request_v1 *r) {
    OK(cui_bridge_accept_slider_request_v1(app, sizeof r->receipt, &r->receipt, sizeof r->requested,
                                           &r->requested));
}
static void interaction(uint32_t kind, uint32_t axis) {
    cui_bridge_node parent, node;
    cui_bridge_app *app = create(64, &parent);
    cui_bridge_slider_config_v1 c = config();
    c.orientation = axis;
    cui_bridge_slider_value_v1 v = {kind, 0, kind ? 20 : NAN, 60};
    OK(cui_bridge_slider_model_create_v1(app, parent, "typed", sizeof c, &c, sizeof v, &v, &node));
    OK(cui_bridge_set_text(app, node, "Controlled selection"));
    OK(cui_bridge_slider_model_get_value_v1(app, node, sizeof v, &v));
    assert(v.start == (kind ? 20 : 0) && v.end == 60);
    OK(cui_bridge_step(app, .016f));
    OK(cui_bridge_step(app, .016f));
    cui_bridge_rect thumb;
    OK(cui_bridge_slider_model_part_bounds_v1(app, node, CUI_BRIDGE_SLIDER_END_THUMB, &thumb));
    assert(thumb.width == 16 && thumb.height == 16);
    float x = thumb.x + thumb.width / 2, y = thumb.y + thumb.height / 2;
    OK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y));
    OK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y));
    OK(cui_bridge_step(app, .016f));
    cui_bridge_slider_request_v1 requests[32];
    size_t count = poll(app, requests, 32);
    assert(count >= 2 && requests[0].phase == CUI_BRIDGE_SLIDER_BEGIN &&
           requests[count - 1].phase == CUI_BRIDGE_SLIDER_COMMIT);
    for (size_t i = 0; i < count; i++) {
        assert(requests[i].receipt.node.slot == node.slot);
        assert(requests[i].receipt.source_generation == c.source_generation);
        assert(requests[i].current.kind == kind && requests[i].current.end == 60);
        assert(requests[i].event_source == CUI_BRIDGE_SLIDER_SOURCE_POINTER);
    }
    /* Explicit owner acceptance: polling alone never writes confirmed state. */
    OK(cui_bridge_slider_model_get_value_v1(app, node, sizeof v, &v));
    assert(v.end == 60);
    accept(app, &requests[count - 1]);
    double baseline = requests[count - 1].requested.end;
    assert(cui_bridge_accept_slider_request_v1(app, sizeof requests[0].receipt,
                                               &requests[0].receipt, sizeof v,
                                               &v) == CUI_BRIDGE_INVALID_STATE);
    OK(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_LEFT, 0, 0));
    OK(cui_bridge_queue_key(
        app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_LEFT,
        CUI_BRIDGE_MOD_SHIFT | CUI_BRIDGE_MOD_CTRL | CUI_BRIDGE_MOD_ALT | CUI_BRIDGE_MOD_META, 1));
    OK(cui_bridge_step(app, .016f));
    count = poll(app, requests, 32);
    assert(count == 6); /* BEGIN/UPDATE/COMMIT per original-order key. */
    assert(requests[1].requested.end == baseline - 1 && requests[4].requested.end == baseline - 2);
    assert(requests[1].current.end == baseline && requests[4].current.end == baseline);
    assert(requests[4].input.repeat == 1 && requests[4].input.key == CUI_BRIDGE_SLIDER_KEY_LEFT);
    assert(requests[4].input.flags & CUI_BRIDGE_SLIDER_HAS_INPUT);
    assert(requests[4].input.modifiers ==
           (CUI_BRIDGE_MOD_SHIFT | CUI_BRIDGE_MOD_CTRL | CUI_BRIDGE_MOD_ALT | CUI_BRIDGE_MOD_META));
    assert(requests[4].receipt.request_sequence > requests[1].receipt.request_sequence);
    cui_bridge_slider_receipt_v1 forged = requests[5].receipt;
    forged.app_epoch ^= UINT64_C(0x100000000);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof forged, &forged, sizeof v, &v) ==
           CUI_BRIDGE_INVALID_STATE);
    accept(app, &requests[1]);
    cui_bridge_slider_value_v1 alternative = requests[5].requested;
    alternative.end = baseline - 3;
    if (kind)
        alternative.start = 22;
    OK(cui_bridge_accept_slider_request_v1(app, sizeof requests[5].receipt, &requests[5].receipt,
                                           sizeof alternative, &alternative));
    OK(cui_bridge_slider_model_get_value_v1(app, node, sizeof v, &v));
    assert(v.end == baseline - 3 && v.start == (kind ? 22 : 0));
    cui_bridge_event legacy;
    assert(cui_bridge_poll_event(app, &legacy) == CUI_BRIDGE_EMPTY);
    /* An ordinary frame expires acceptance, but copied queued payload stays readable. */
    OK(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 0));
    OK(cui_bridge_step(app, .016f));
    OK(cui_bridge_step(app, .016f));
    count = poll(app, requests, 32);
    assert(count == 3 && requests[1].requested.end == baseline - 2);
    assert(cui_bridge_accept_slider_request_v1(app, sizeof requests[1].receipt,
                                               &requests[1].receipt, sizeof v,
                                               &v) == CUI_BRIDGE_INVALID_STATE);
    c.flags |= CUI_BRIDGE_SLIDER_READONLY;
    OK(cui_bridge_slider_model_configure_v1(app, node, sizeof c, &c, sizeof v, &v));
    OK(cui_bridge_step(app, .016f));
    OK(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 0));
    OK(cui_bridge_step(app, .016f));
    assert(poll(app, requests, 32) == 0);
    OK(cui_bridge_set_disabled(app, node, 1));
    OK(cui_bridge_step(app, .016f));
    cui_bridge_rect unchanged = {1, 2, 3, 4};
    assert(cui_bridge_slider_model_part_bounds_v1(app, node, CUI_BRIDGE_SLIDER_END_THUMB,
                                                  &unchanged) == CUI_BRIDGE_EMPTY);
    assert(unchanged.x == 1 && unchanged.height == 4);
    OK(cui_bridge_slider_model_part_bounds_v1(app, node, CUI_BRIDGE_SLIDER_TRACK, &unchanged));
    assert(unchanged.width > 0 && unchanged.height > 0); /* Disabled track is still painted. */
    OK(cui_bridge_slider_model_part_bounds_v1(app, node, CUI_BRIDGE_SLIDER_FILL, &unchanged));
    assert(unchanged.width > 0 && unchanged.height > 0);
    cui_bridge_destroy(app);
    assert(requests[1].requested.end == baseline - 2); /* Owned record, no borrowed app storage. */
}
static void validation(void) {
    cui_bridge_node parent, node = {42, 43};
    cui_bridge_app *app = create(1, &parent);
    cui_bridge_slider_config_v1 c = config();
    cui_bridge_slider_value_v1 v = {0, 0, NAN, 40};
    assert(cui_bridge_slider_model_create_v1(app, (cui_bridge_node){0}, "bad", sizeof c, &c,
                                             sizeof v, &v, &node) == CUI_BRIDGE_STALE_HANDLE);
    assert(node.slot == 42 && node.generation == 43);
    c.reserved = 1;
    assert(cui_bridge_slider_model_create_v1(app, parent, "bad", sizeof c, &c, sizeof v, &v,
                                             &node) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(node.slot == 42);
    c.reserved = 0;
    v.end = nextafter(40, INFINITY);
    OK(cui_bridge_slider_model_create_v1(app, parent, "typed", sizeof c, &c, sizeof v, &v, &node));
    cui_bridge_slider_value_v1 exact;
    OK(cui_bridge_slider_model_get_value_v1(app, node, sizeof exact, &exact));
    assert(exact.end == v.end && exact.end != 40 && exact.start == 0);
    v.end = 40;
    OK(cui_bridge_slider_model_set_value_v1(app, node, sizeof v, &v));
    OK(cui_bridge_step(app, .016f));
    cui_bridge_rect r;
    OK(cui_bridge_slider_model_part_bounds_v1(app, node, CUI_BRIDGE_SLIDER_END_THUMB, &r));
    OK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, r.x + 7, r.y + 7));
    OK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, r.x + 7, r.y + 7));
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CAPACITY);
    cui_bridge_slider_request_v1 request;
    memset(&request, 0xa5, sizeof request);
    unsigned char before[sizeof request];
    memcpy(before, &request, sizeof request);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) == CUI_BRIDGE_EMPTY);
    assert(memcmp(before, &request, sizeof request) == 0);
    c.layout = CUI_BRIDGE_SLIDER_PARENT;
    c.length = NAN; /* Inactive input canonicalized, never read as geometry. */
    OK(cui_bridge_slider_model_configure_v1(app, node, sizeof c, &c, sizeof v, &v));
    OK(cui_bridge_slider_model_get_config_v1(app, node, sizeof c, &c));
    assert(c.length == 0);
    v.end = INFINITY;
    assert(cui_bridge_slider_model_set_value_v1(app, node, sizeof v, &v) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_destroy(app);
}
int main(void) {
    assert(cui_bridge_slider_model_versions() == 1);
    assert(cui_bridge_slider_request_v1_size() == 296);
    validation();
    for (uint32_t kind = 0; kind < 2; kind++)
        for (uint32_t axis = 0; axis < 2; axis++)
            interaction(kind, axis);
    puts("typed bridge nested UI, copied requests, explicit acceptance, expiry and atomic errors "
         "PASS");
    return 0;
}
