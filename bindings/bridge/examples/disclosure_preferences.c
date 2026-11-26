/* SPDX-License-Identifier: MIT */
#include <cui/cui_bridge.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    cui_bridge_app *app;
    cui_bridge_node account, notifications, summary;
    unsigned summary_updates;
} preferences;

static bool node(preferences *view, cui_bridge_node parent, cui_bridge_kind kind, const char *key,
                 const char *text, cui_bridge_node *out) {
    return cui_bridge_node_create(view->app, parent, kind, key, out) == CUI_BRIDGE_OK &&
           (!text || cui_bridge_set_text(view->app, *out, text) == CUI_BRIDGE_OK);
}

static cui_bridge_status update_summary(void *user) {
    preferences *view = user;
    cui_bridge_event_ex event;
    char payload[128];
    uint32_t required;
    cui_bridge_status status;
    bool changed = false;
    while ((status = cui_bridge_poll_event_ex(view->app, &event, payload, sizeof payload,
                                              &required)) == CUI_BRIDGE_OK)
        changed |=
            event.type == CUI_BRIDGE_EVENT_CHANGE &&
            (event.node.slot == view->account.slot || event.node.slot == view->notifications.slot);
    if (status != CUI_BRIDGE_EMPTY)
        return status;
    if (!changed)
        return CUI_BRIDGE_OK;
    uint32_t account, notifications;
    status = cui_bridge_get_checked(view->app, view->account, &account);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = cui_bridge_get_checked(view->app, view->notifications, &notifications);
    if (status != CUI_BRIDGE_OK)
        return status;
    const char *text = account         ? "Viewing account preferences"
                       : notifications ? "Viewing notification preferences"
                                       : "Preferences collapsed";
    status = cui_bridge_set_text(view->app, view->summary, text);
    if (status == CUI_BRIDGE_OK)
        view->summary_updates++;
    return status;
}

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            fprintf(stderr, "Disclosure example failed at line %d: %s\n", __LINE__, #expression);  \
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
        .title_utf8 = "Workspace preferences",
        .width = 480,
        .height = 340,
    };
    cui_bridge_status status;
    preferences view = {.app = cui_bridge_create(&options, &status)};
    CHECK(view.app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, title, group, account_text, advanced, advanced_text, notice_text;
    CHECK(node(&view, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "preferences", NULL, &root));
    CHECK(node(&view, root, CUI_BRIDGE_LABEL, "title", "Workspace preferences", &title));
    CHECK(node(&view, root, CUI_BRIDGE_LABEL, "summary", "Viewing account preferences",
               &view.summary));
    CHECK(node(&view, root, CUI_BRIDGE_ACCORDION, "sections", NULL, &group));
    CHECK(node(&view, group, CUI_BRIDGE_ITEM, "account", "Account", &view.account));
    CHECK(node(&view, view.account, CUI_BRIDGE_LABEL, "workspace", "Workspace: North Studio",
               &account_text));
    CHECK(
        node(&view, view.account, CUI_BRIDGE_COLLAPSIBLE, "details", "Account details", &advanced));
    CHECK(node(&view, advanced, CUI_BRIDGE_LABEL, "role", "Role: Workspace admin", &advanced_text));
    CHECK(
        node(&view, group, CUI_BRIDGE_ITEM, "notifications", "Notifications", &view.notifications));
    CHECK(node(&view, view.notifications, CUI_BRIDGE_LABEL, "digest", "Weekly email digest",
               &notice_text));

    const cui_bridge_disclosure_config_v1 card = {
        .struct_size = sizeof card,
        .schema_version = 1,
        .flags = CUI_BRIDGE_DISCLOSURE_JOINED_V1 | CUI_BRIDGE_DISCLOSURE_BORDERED_V1 |
                 CUI_BRIDGE_DISCLOSURE_SINGLE_V1 | CUI_BRIDGE_DISCLOSURE_ALLOW_COLLAPSE_V1,
        .width = -1,
        .design_size = CUI_BRIDGE_SIZE_LG,
        .collapsed_icon = "plus",
        .expanded_icon = "minus",
    };
    const cui_bridge_disclosure_config_v1 nested = {
        .struct_size = sizeof nested,
        .schema_version = 1,
        .flags = CUI_BRIDGE_DISCLOSURE_BORDERLESS_V1,
        .width = -1,
        .design_size = CUI_BRIDGE_SIZE_SM,
    };
    const cui_bridge_disclosure_trigger_style_v1 feedback = {
        .struct_size = sizeof feedback,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HOVER_V1 | CUI_BRIDGE_DISCLOSURE_PRESSED_V1 |
                  CUI_BRIDGE_DISCLOSURE_CURSOR_V1,
        .hover_rgba = 0x4879e02bu,
        .pressed_rgba = 0x4879e047u,
        .cursor = CUI_BRIDGE_DISCLOSURE_CURSOR_POINTER,
    };
    const cui_bridge_disclosure_heading_v1 heading = {
        .struct_size = sizeof heading,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HEADING_TYPOGRAPHY,
        .font_size = 17,
        .line_height = 24,
        .weight = 1, /* Medium. */
        .text_flags = CUI_BRIDGE_DISCLOSURE_TITLE_WRAP,
    };
    cui_bridge_disclosure_heading_v1 expanded = heading;
    expanded.font_size = 19;
    expanded.line_height = 26;
    expanded.weight = 3; /* Bold. */
    const cui_bridge_disclosure_divider_v1 divider = {
        .struct_size = sizeof divider,
        .schema_version = 1,
        .thickness = 2,
        .inset_left = 12,
        .inset_right = 12,
        .color_rgba = 0x4d80c0aau,
    };
    CHECK(cui_bridge_disclosure_configure_v1(view.app, group, &card) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_disclosure_configure_v1(view.app, advanced, &nested) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_disclosure_set_heading_v1(
              view.app, group, CUI_BRIDGE_DISCLOSURE_HEADING_NORMAL, &heading) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_disclosure_set_heading_v1(
              view.app, group, CUI_BRIDGE_DISCLOSURE_HEADING_EXPANDED, &expanded) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_disclosure_set_divider_v1(view.app, group, &divider) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_disclosure_set_trigger_style_v1(view.app, view.account, &feedback) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_disclosure_set_trigger_style_v1(view.app, view.notifications, &feedback) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_disclosure_set_trigger_style_v1(view.app, advanced, &feedback) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_checked(view.app, view.account, 1) == CUI_BRIDGE_OK);

    if (font) {
        CHECK(cui_bridge_run(view.app, update_summary, &view) == CUI_BRIDGE_OK);
    } else {
        CHECK(cui_bridge_step(view.app, .016f) == CUI_BRIDGE_OK);
        cui_bridge_rect row;
        CHECK(cui_bridge_node_bounds(view.app, advanced, &row) == CUI_BRIDGE_OK && row.width > 0);
        CHECK(cui_bridge_node_bounds(view.app, view.notifications, &row) == CUI_BRIDGE_OK);
        float x = row.x + row.width / 2, y = row.y + row.height / 2;
        CHECK(cui_bridge_queue_pointer(view.app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
        CHECK(cui_bridge_queue_pointer(view.app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
        CHECK(cui_bridge_step(view.app, .016f) == CUI_BRIDGE_OK);
        CHECK(update_summary(&view) == CUI_BRIDGE_OK);
        CHECK(view.summary_updates == 1);
        uint32_t expanded;
        CHECK(cui_bridge_get_checked(view.app, view.account, &expanded) == CUI_BRIDGE_OK &&
              !expanded);
        CHECK(cui_bridge_get_checked(view.app, view.notifications, &expanded) == CUI_BRIDGE_OK &&
              expanded);
        CHECK(cui_bridge_node_bounds(view.app, notice_text, &row) == CUI_BRIDGE_OK);
        puts("Preferences example opened Notifications and updated its summary.");
    }
    cui_bridge_destroy(view.app);
    return EXIT_SUCCESS;

failed:
    cui_bridge_destroy(view.app);
    return EXIT_FAILURE;
}
