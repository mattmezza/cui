/* SPDX-License-Identifier: MIT */
/* Windowless real UI composition; choose --reject to leave the owner unchanged. */
#include "cui/cui_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            fprintf(stderr, "bridge example failed at line %d: %s\n", __LINE__, #expression);      \
            return EXIT_FAILURE;                                                                   \
        }                                                                                          \
    } while (0)
int main(int argc, char **argv) {
    int reject = argc > 1 && strcmp(argv[1], "--reject") == 0;
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 640,
                                  .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    CHECK(app && status == CUI_BRIDGE_OK);
    CHECK(cui_bridge_slider_model_versions() & 1);
    cui_bridge_slider_queue_options_v1 queue = {sizeof queue, 1, 64, 0};
    CHECK(cui_bridge_slider_queue_enable_v1(app, sizeof queue, &queue) == CUI_BRIDGE_OK);
    cui_bridge_node root, label, slider;
    CHECK(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_node_create(app, root, CUI_BRIDGE_LABEL, "label", &label) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_text(app, label, "Selected interval") == CUI_BRIDGE_OK);
    cui_bridge_slider_config_v1 config = {.struct_size = sizeof config,
                                          .schema_version = 1,
                                          .flags = CUI_BRIDGE_SLIDER_REQUESTS |
                                                   CUI_BRIDGE_SLIDER_REDUCED_MOTION,
                                          .source_generation = 1,
                                          .domain = {0, 100, 1, CUI_BRIDGE_SLIDER_LINEAR, 0},
                                          .rem = 16,
                                          .length = 240};
    cui_bridge_slider_value_v1 owner = {CUI_BRIDGE_SLIDER_RANGE, 0, 20, 60};
    CHECK(cui_bridge_slider_model_create_v1(app, root, "interval", sizeof config, &config,
                                            sizeof owner, &owner, &slider) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_text(app, slider, "Selected interval") == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect end;
    CHECK(cui_bridge_slider_model_part_bounds_v1(app, slider, CUI_BRIDGE_SLIDER_END_THUMB, &end) ==
          CUI_BRIDGE_OK);
    /* A real host supplies these events. This example deterministically focuses END. */
    CHECK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, end.x + end.width / 2,
                                   end.y + end.height / 2) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, end.x + end.width / 2,
                                   end.y + end.height / 2) == CUI_BRIDGE_OK);
    for (unsigned frame = 0; frame < 2; ++frame) {
        CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
        cui_bridge_slider_request_v1 request;
        while ((status = cui_bridge_poll_slider_request_v1(app, sizeof request, &request)) ==
               CUI_BRIDGE_OK) {
            printf("phase=%u current=[%.17g,%.17g] requested=[%.17g,%.17g] %s\n", request.phase,
                   request.current.start, request.current.end, request.requested.start,
                   request.requested.end, reject ? "reject" : "accept UPDATE");
            if (!reject && request.phase == CUI_BRIDGE_SLIDER_UPDATE) {
                CHECK(cui_bridge_accept_slider_request_v1(
                          app, sizeof request.receipt, &request.receipt, sizeof request.requested,
                          &request.requested) == CUI_BRIDGE_OK);
                owner = request.requested; /* External application state, explicit whole pair. */
            }
        }
        CHECK(status == CUI_BRIDGE_EMPTY);
        if (!frame)
            CHECK(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 0) ==
                  CUI_BRIDGE_OK);
    }
    cui_bridge_slider_value_v1 confirmed;
    CHECK(cui_bridge_slider_model_get_value_v1(app, slider, sizeof confirmed, &confirmed) ==
          CUI_BRIDGE_OK);
    CHECK(confirmed.start == owner.start && confirmed.end == owner.end);
    CHECK(reject ? owner.end == 60 : owner.end > 60);
    cui_bridge_destroy(app);
    return 0;
}
