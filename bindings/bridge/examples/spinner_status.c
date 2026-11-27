/* SPDX-License-Identifier: MIT */
#include <cui/cui_bridge.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    cui_bridge_app *app;
    cui_bridge_node row, label, button, spinner;
    cui_bridge_icon_handle_v1 icon;
} sync_panel;

static cui_bridge_status toggle_sync(sync_panel *panel) {
    cui_bridge_status status;
    if (panel->spinner.slot) {
        status = cui_bridge_node_remove(panel->app, panel->spinner);
        if (status != CUI_BRIDGE_OK)
            return status;
        panel->spinner = (cui_bridge_node){0};
    } else {
        const cui_bridge_spinner_config_v1 config = {
            .struct_size = sizeof config,
            .schema_version = CUI_BRIDGE_SPINNER_V1,
            .size = CUI_BRIDGE_SPINNER_SMALL,
            .rem_size = 20,
            .easing = CUI_BRIDGE_SPINNER_LINEAR,
        };
        status = cui_bridge_node_create(panel->app, panel->row, CUI_BRIDGE_SPINNER, "loading",
                                        &panel->spinner);
        if (status != CUI_BRIDGE_OK)
            return status;
        status = cui_bridge_spinner_configure_v1(panel->app, panel->spinner, &config);
        if (status == CUI_BRIDGE_OK)
            status =
                cui_bridge_spinner_set_icon_resource_v1(panel->app, panel->spinner, panel->icon);
        if (status != CUI_BRIDGE_OK) {
            (void)cui_bridge_node_remove(panel->app, panel->spinner);
            panel->spinner = (cui_bridge_node){0};
            return status;
        }
    }
    bool syncing = panel->spinner.slot != 0;
    status = cui_bridge_set_text(panel->app, panel->label, syncing ? "Syncing files…" : "Ready");
    if (status != CUI_BRIDGE_OK)
        return status;
    return cui_bridge_set_text(panel->app, panel->button, syncing ? "Stop sync" : "Start sync");
}

static cui_bridge_status after_frame(void *user) {
    sync_panel *panel = user;
    cui_bridge_event event;
    cui_bridge_status status;
    while ((status = cui_bridge_poll_event(panel->app, &event)) == CUI_BRIDGE_OK) {
        if (event.type == CUI_BRIDGE_EVENT_CLICK && event.node.slot == panel->button.slot) {
            status = toggle_sync(panel);
            if (status != CUI_BRIDGE_OK)
                return status;
        }
    }
    return status == CUI_BRIDGE_EMPTY ? CUI_BRIDGE_OK : status;
}

static bool smoke_click(sync_panel *panel) {
    cui_bridge_rect button;
    if (cui_bridge_node_bounds(panel->app, panel->button, &button) != CUI_BRIDGE_OK)
        return false;
    float x = button.x + button.width / 2, y = button.y + button.height / 2;
    return cui_bridge_queue_pointer(panel->app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK &&
           cui_bridge_queue_pointer(panel->app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK &&
           cui_bridge_step(panel->app, .016f) == CUI_BRIDGE_OK &&
           after_frame(panel) == CUI_BRIDGE_OK;
}

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            fprintf(stderr, "Spinner example failed at line %d: %s\n", __LINE__, #expression);     \
            goto failed;                                                                           \
        }                                                                                          \
    } while (0)

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
        .title_utf8 = "File sync",
        .width = 420,
        .height = 180,
    };
    cui_bridge_status status;
    sync_panel panel = {.app = cui_bridge_create(&options, &status)};
    CHECK(panel.app && status == CUI_BRIDGE_OK);
    const char svg[] = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
                       "<path d='M12 2 L20 14 L12 20 L6 13 L11 13 Z'/></svg>";
    const cui_bridge_icon_limits_v1 icon_limits = {.struct_size = sizeof icon_limits,
                                                   .schema_version = 1};
    CHECK(cui_bridge_icon_create_v1(panel.app, &icon_limits, &panel.icon) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_icon_set_source_v1(panel.app, panel.icon, CUI_BRIDGE_ICON_DATA_V1, svg,
                                        sizeof svg - 1) == CUI_BRIDGE_OK);
    cui_bridge_node root;
    CHECK(cui_bridge_node_create(panel.app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "file-sync",
                                 &root) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_node_create(panel.app, root, CUI_BRIDGE_ROW, "status", &panel.row) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_node_create(panel.app, panel.row, CUI_BRIDGE_LABEL, "status-label",
                                 &panel.label) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_text(panel.app, panel.label, "Ready") == CUI_BRIDGE_OK);
    CHECK(cui_bridge_node_create(panel.app, root, CUI_BRIDGE_BUTTON, "toggle", &panel.button) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_text(panel.app, panel.button, "Start sync") == CUI_BRIDGE_OK);
    if (font) {
        status = cui_bridge_run(panel.app, after_frame, &panel);
        CHECK(status == CUI_BRIDGE_OK || status == CUI_BRIDGE_CLOSED);
    } else {
        CHECK(cui_bridge_step(panel.app, .016f) == CUI_BRIDGE_OK);
        CHECK(smoke_click(&panel) && panel.spinner.slot);
        CHECK(cui_bridge_step(panel.app, .016f) == CUI_BRIDGE_OK);
        cui_bridge_spinner_presentation_v1 sample = {.struct_size = sizeof sample,
                                                     .schema_version = CUI_BRIDGE_SPINNER_V1};
        CHECK(cui_bridge_spinner_get_presentation_v1(panel.app, panel.spinner, &sample) ==
              CUI_BRIDGE_OK);
        CHECK(sample.next_wake_seconds > sample.core_time);
        CHECK(smoke_click(&panel) && !panel.spinner.slot);
        CHECK(cui_bridge_icon_destroy_v1(panel.app, panel.icon) == CUI_BRIDGE_OK);
    }
    cui_bridge_destroy(panel.app);
    return EXIT_SUCCESS;
failed:
    cui_bridge_destroy(panel.app);
    return EXIT_FAILURE;
}
