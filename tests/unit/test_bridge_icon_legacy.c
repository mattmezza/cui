/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FILE *trace, *allocations;
static int tracing_allocations;
static void *legacy_calloc(size_t count, size_t size) {
    if (tracing_allocations)
        fprintf(allocations, "calloc %zu %zu\n", count, size);
    return calloc(count, size);
}
static void *legacy_malloc(size_t size) {
    if (tracing_allocations)
        fprintf(allocations, "malloc %zu\n", size);
    return malloc(size);
}
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#define calloc legacy_calloc
#define malloc legacy_malloc
#include BRIDGE_SOURCE_FILE
#undef malloc
#undef calloc
#include "../reference/bridge_icon_draw_normalizer.h"
static cui_software *software;
static unsigned frame;
static cui_bridge_status icon_capture(const cui_draw_list *list) {
    fprintf(trace, "FRAME %u %zu\n", ++frame, list->count);
    for (size_t i = 0; i < list->count; i++)
        command(trace, &list->commands[i]);
    /* Match the pre-existing V1 oracle: all text bytes/active fields above,
     * nontext pixels below. No font raster acceptance is claimed. */
    cui_draw_command filtered[256];
    size_t count = 0;
    for (size_t i = 0; i < list->count; i++)
        if (list->commands[i].type != CUI_DRAW_TEXT) {
            assert(count < 256);
            filtered[count++] = list->commands[i];
        }
    cui_draw_list paint = {filtered, count, list->width, list->height};
    assert(cui_software_render(software, &paint, (cui_color){1, 2, 3, 4}));
    unsigned w, h;
    const unsigned char *p = cui_software_pixels(software, &w, &h);
    for (size_t at = 0; at < (size_t)w * h;) {
        size_t end = at + 1;
        while (end < (size_t)w * h && !memcmp(p + at * 4, p + end * 4, 4))
            end++;
        fprintf(trace, "P %zu %u %u %u %u\n", end - at, p[at * 4], p[at * 4 + 1], p[at * 4 + 2],
                p[at * 4 + 3]);
        at = end;
    }
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_native_create(cui_bridge_app *a, const char *t, const char *f,
                                           cui_config *c, bool h) {
    (void)a;
    (void)t;
    (void)f;
    (void)c;
    (void)h;
    return CUI_BRIDGE_OK;
}
void cui_bridge_native_destroy(cui_bridge_app *a) {
    (void)a;
}
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *a) {
    (void)a;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_native_render(cui_bridge_app *a, const cui_draw_list *l) {
    (void)a;
    assert(l && l->count);
    return icon_capture(l);
}
cui_bridge_status cui_bridge_native_run(cui_bridge_app *a, cui_bridge_run_callback c, void *u) {
    (void)a;
    (void)c;
    (void)u;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *a, const char *const *p,
                                                       uint32_t n) {
    (void)a;
    (void)p;
    (void)n;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *a, cui_str s) {
    (void)a;
    (void)s;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *a, char *p, uint32_t c,
                                                  uint32_t *n) {
    (void)a;
    (void)p;
    (void)c;
    (void)n;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_window_visible(cui_bridge_app *a, uint32_t v) {
    (void)a;
    (void)v;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_get_window_visible(cui_bridge_app *a, uint32_t *v) {
    (void)a;
    (void)v;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_node make(cui_bridge_app *a, cui_bridge_node parent, cui_bridge_kind kind,
                            const char *key) {
    cui_bridge_node n;
    assert(cui_bridge_node_create(a, parent, kind, key, &n) == CUI_BRIDGE_OK);
    return n;
}
int main(int argc, char **argv) {
    assert(argc == 3);
    trace = fopen(argv[1], "wb");
    allocations = fopen(argv[2], "wb");
    assert(trace && allocations);
    software = cui_software_create(320, 240, NULL);
    assert(software);
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = 1,
                            .backend = CUI_BRIDGE_BACKEND_SDL,
                            .width = 320,
                            .height = 240,
                            .max_nodes = 32,
                            .max_events = 32};
    cui_bridge_status status;
    tracing_allocations = 1;
    cui_bridge_app *a = cui_bridge_create(&o, &status);
    tracing_allocations = 0;
    assert(a && status == CUI_BRIDGE_OK);
    cui_bridge_node root = make(a, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node badge = make(a, root, CUI_BRIDGE_BADGE, "badge"),
                    icon = make(a, root, CUI_BRIDGE_ICON, "icon");
    cui_bridge_node button = make(a, root, CUI_BRIDGE_BUTTON, "button");
    assert(cui_bridge_set_text(a, badge, "Ready") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(a, icon, "check") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(a, button, "Open") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect b;
    assert(cui_bridge_node_bounds(a, button, &b) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(a, CUI_BRIDGE_POINTER_DOWN, b.x + b.width / 2,
                                    b.y + b.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(a, CUI_BRIDGE_POINTER_UP, b.x + b.width / 2,
                                    b.y + b.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event e;
    unsigned clicks = 0;
    while (cui_bridge_poll_event(a, &e) == CUI_BRIDGE_OK) {
        fprintf(trace, "EVENT %u %u %u\n", e.type, e.node.slot, e.node.generation);
        if (e.type == CUI_BRIDGE_EVENT_CLICK)
            clicks++;
    }
    assert(clicks == 1);
    assert(cui_bridge_set_text(a, badge, "") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(a);
    cui_software_destroy(software);
    assert(!fclose(trace) && !fclose(allocations));
    return 0;
}
