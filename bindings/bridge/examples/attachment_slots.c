/* SPDX-License-Identifier: MIT */
#include <cui/cui_bridge.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    cui_bridge_app *app;
    cui_bridge_node inspect, status;
    unsigned inspections;
} file_view;

static bool add(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind, const char *key,
                const char *text, cui_bridge_node *out) {
    return cui_bridge_node_create(app, parent, kind, key, out) == CUI_BRIDGE_OK &&
           (!text || cui_bridge_set_text(app, *out, text) == CUI_BRIDGE_OK);
}

static bool build_file(file_view *view) {
    cui_bridge_app *app = view->app;
    cui_bridge_node root, card, media, metadata, actions, label;
    const cui_bridge_attachment_config_v1 presentation = {.struct_size = sizeof presentation,
                                                          .schema_version = 1};
    return add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "files", NULL, &root) &&
           cui_bridge_set_gap(app, root, 12) == CUI_BRIDGE_OK &&
           add(app, root, CUI_BRIDGE_LABEL, "title", "Project files", &label) &&
           add(app, root, CUI_BRIDGE_ATTACHMENT, "report", "Quarterly report.pdf", &card) &&
           cui_bridge_set_visual_size(app, card, 380, 0) == CUI_BRIDGE_OK &&
           cui_bridge_set_detail_text(app, card, "1.4 MB") == CUI_BRIDGE_OK &&
           cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
               CUI_BRIDGE_OK &&
           cui_bridge_attachment_configure_v1(app, card, &presentation) == CUI_BRIDGE_OK &&
           cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1,
                                                "report-media", 0, &media) == CUI_BRIDGE_OK &&
           add(app, media, CUI_BRIDGE_LABEL, "file-type", "PDF", &label) &&
           cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_METADATA_V1,
                                                "report-meta", 0, &metadata) == CUI_BRIDGE_OK &&
           add(app, metadata, CUI_BRIDGE_LABEL, "owner", "Finance team", &label) &&
           cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_ACTIONS_V1,
                                                "report-actions", 100, &actions) == CUI_BRIDGE_OK &&
           add(app, actions, CUI_BRIDGE_BUTTON, "inspect", "Inspect", &view->inspect) &&
           add(app, root, CUI_BRIDGE_LABEL, "status", "Select Inspect for file details.",
               &view->status);
}

static cui_bridge_status inspect_file(void *user) {
    file_view *view = user;
    cui_bridge_event event;
    cui_bridge_status status;
    while ((status = cui_bridge_poll_event(view->app, &event)) == CUI_BRIDGE_OK) {
        if (event.type == CUI_BRIDGE_EVENT_CLICK && event.node.slot == view->inspect.slot &&
            event.node.generation == view->inspect.generation) {
            status = cui_bridge_set_text(view->app, view->status,
                                         "Quarterly report.pdf · Finance team · 1.4 MB");
            if (status != CUI_BRIDGE_OK)
                return status;
            view->inspections++;
        }
    }
    return status == CUI_BRIDGE_EMPTY ? CUI_BRIDGE_OK : status;
}

static bool preview_headless(file_view *view) {
    cui_bridge_app *app = view->app;
    cui_bridge_rect button;
    if (cui_bridge_step(app, .016f) != CUI_BRIDGE_OK ||
        cui_bridge_node_bounds(app, view->inspect, &button) != CUI_BRIDGE_OK || button.width <= 0)
        return false;
    float x = button.x + button.width / 2, y = button.y + button.height / 2;
    return cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK &&
           cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK &&
           cui_bridge_step(app, .016f) == CUI_BRIDGE_OK && inspect_file(view) == CUI_BRIDGE_OK &&
           view->inspections == 1;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [font.ttf]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *font = argc == 2 ? argv[1] : NULL;
    const cui_bridge_options options = {
        .struct_size = sizeof options,
        .abi_version = CUI_BRIDGE_ABI_VERSION,
        .backend = font ? CUI_BRIDGE_BACKEND_SDL : CUI_BRIDGE_BACKEND_NONE,
        .font_path_utf8 = font,
        .title_utf8 = "Project files",
        .width = 500,
        .height = 240,
    };
    cui_bridge_status status;
    file_view view = {.app = cui_bridge_create(&options, &status)};
    bool ok = view.app && status == CUI_BRIDGE_OK && build_file(&view) &&
              (font ? cui_bridge_run(view.app, inspect_file, &view) == CUI_BRIDGE_OK
                    : preview_headless(&view));
    cui_bridge_destroy(view.app);
    if (!ok)
        fputs("Attachment slot example failed.\n", stderr);
    else if (!font)
        puts("Inspect updated the file summary once.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
