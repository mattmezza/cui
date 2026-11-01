/* SPDX-License-Identifier: MIT */
/* Linux owner-thread microgate; no simultaneous Core access or native window. */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
struct request {
    cui_bridge_app *app;
    cui_bridge_icon_handle_v1 handle;
    cui_bridge_node node;
};
static void *foreign_owner(void *arg) {
    struct request *r = arg;
    cui_bridge_icon_info_v1 out;
    memset(&out, 0xa5, sizeof out);
    out.struct_size = sizeof out;
    out.schema_version = 1;
    cui_bridge_icon_info_v1 before = out;
    assert(cui_bridge_icon_get_info_v1(r->app, r->handle, &out) == CUI_BRIDGE_INVALID_STATE &&
           !memcmp(&out, &before, sizeof out));
    assert(cui_bridge_icon_collect_v1(r->app, r->handle) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_text(r->app, r->node, "Foreign") == CUI_BRIDGE_INVALID_STATE);
    cui_bridge_icon_limits_v1 l = {.struct_size = sizeof l, .schema_version = 1};
    cui_bridge_icon_handle_v1 h = {77, 88};
    assert(cui_bridge_icon_create_v1(r->app, &l, &h) == CUI_BRIDGE_INVALID_STATE && h.slot == 77 &&
           h.generation == 88);
    return NULL;
}
int main(void) {
    cui_bridge_options o = {.struct_size = sizeof o, .abi_version = 1, .width = 320, .height = 240};
    cui_bridge_status status;
    cui_bridge_app *a = cui_bridge_create(&o, &status);
    assert(a && status == CUI_BRIDGE_OK);
    struct request r = {.app = a};
    assert(cui_bridge_node_create(a, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &r.node) ==
           CUI_BRIDGE_OK);
    cui_bridge_icon_limits_v1 l = {.struct_size = sizeof l, .schema_version = 1};
    assert(cui_bridge_icon_create_v1(a, &l, &r.handle) == CUI_BRIDGE_OK);
    pthread_t thread;
    assert(!pthread_create(&thread, NULL, foreign_owner, &r));
    assert(!pthread_join(thread, NULL));
    cui_bridge_icon_info_v1 out = {.struct_size = sizeof out, .schema_version = 1};
    assert(cui_bridge_icon_get_info_v1(a, r.handle, &out) == CUI_BRIDGE_OK);
    assert(cui_bridge_icon_destroy_v1(a, r.handle) == CUI_BRIDGE_OK);
    cui_bridge_destroy(a);
    puts("PASS actual serialized foreign-owner thread refusal");
    return 0;
}
