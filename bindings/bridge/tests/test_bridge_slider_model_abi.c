/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <stddef.h>
#define SIZE(type, size) _Static_assert(sizeof(type) == size, #type " size")
#define FIELD(type, field, offset) _Static_assert(offsetof(type, field) == offset, #type "." #field)
SIZE(cui_bridge_node, 8);
SIZE(cui_bridge_slider_value_v1, 24);
FIELD(cui_bridge_slider_value_v1, start, 8);
FIELD(cui_bridge_slider_value_v1, end, 16);
SIZE(cui_bridge_slider_domain_v1, 32);
FIELD(cui_bridge_slider_domain_v1, scale, 24);
SIZE(cui_bridge_slider_config_v1, 72);
FIELD(cui_bridge_slider_config_v1, source_generation, 16);
FIELD(cui_bridge_slider_config_v1, domain, 24);
FIELD(cui_bridge_slider_config_v1, rem, 56);
FIELD(cui_bridge_slider_config_v1, layout, 64);
SIZE(cui_bridge_slider_queue_options_v1, 16);
SIZE(cui_bridge_slider_receipt_v1, 56);
FIELD(cui_bridge_slider_receipt_v1, app_epoch, 8);
FIELD(cui_bridge_slider_receipt_v1, node, 16);
FIELD(cui_bridge_slider_receipt_v1, source_generation, 24);
FIELD(cui_bridge_slider_receipt_v1, configuration_revision, 32);
FIELD(cui_bridge_slider_receipt_v1, submission_frame, 40);
FIELD(cui_bridge_slider_receipt_v1, request_sequence, 48);
SIZE(cui_bridge_slider_input_v1, 72);
FIELD(cui_bridge_slider_input_v1, x, 16);
FIELD(cui_bridge_slider_input_v1, pointer_id, 40);
FIELD(cui_bridge_slider_input_v1, pointer_device, 48);
FIELD(cui_bridge_slider_input_v1, accessibility_target, 56);
FIELD(cui_bridge_slider_input_v1, accessibility_value, 64);
SIZE(cui_bridge_slider_request_v1, 296);
FIELD(cui_bridge_slider_request_v1, receipt, 32);
FIELD(cui_bridge_slider_request_v1, event_target, 88);
FIELD(cui_bridge_slider_request_v1, event_frame, 96);
FIELD(cui_bridge_slider_request_v1, event_index, 104);
FIELD(cui_bridge_slider_request_v1, current, 112);
FIELD(cui_bridge_slider_request_v1, requested, 136);
FIELD(cui_bridge_slider_request_v1, domain, 160);
FIELD(cui_bridge_slider_request_v1, gesture_domain, 192);
FIELD(cui_bridge_slider_request_v1, input, 224);
SIZE(cui_bridge_slider_presentation_v1, 32);
FIELD(cui_bridge_slider_presentation_v1, core_time, 16);
FIELD(cui_bridge_slider_presentation_v1, next_wake_seconds, 24);
int main(void) {
    return 0;
}
