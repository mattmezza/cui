# Configured Badge in retained C apps

Use a configured Badge to put a count, dot, or SVG indicator on a child control.
It draws through the ordinary [C Badge component](BADGE.md) and places the
indicator without moving the content. A count of zero hides the indicator.
Children still produce their own events through the ordinary bridge queue.

The optional V1 factory is available in working source. Python, JavaScript,
TypeScript, Go and Rust can author the configured variant.
The new C kind uses a dedicated factory to admit children.
[Python guide](PYTHON_ICON_BADGE.md) · [Node and TypeScript guide](NODE_ICON_BADGE.md) ·
[Go guide](GO_ICON_BADGE.md) · [Rust guide](RUST_ICON_BADGE.md).

## Add an unread indicator

This helper creates an Inbox button with an unread count. Pass an app-owned Icon
handle instead of `{0}` to show custom artwork. The caller still owns that
resource and can share it with a standalone Icon or another Badge.

```c
#include "cui/cui_bridge.h"

cui_bridge_status add_inbox_badge(cui_bridge_app *app, cui_bridge_node parent,
                                  uint64_t count, cui_bridge_icon_handle_v1 resource,
                                  cui_bridge_node *badge_out, cui_bridge_node *button_out) {
    if (!(cui_bridge_badge_versions() & 1u))
        return CUI_BRIDGE_INVALID_STATE;

    cui_bridge_badge_config_v1 config = {
        .struct_size = sizeof config, .schema_version = 1
    };
    cui_bridge_status status = cui_bridge_badge_default_config_v1(&config);
    if (status != CUI_BRIDGE_OK)
        return status;
    config.count = count;
    cui_bridge_icon_options_v1 artwork = {
        .struct_size = sizeof artwork, .schema_version = 1,
        .flags = CUI_BRIDGE_ICON_WIDTH_V1 | CUI_BRIDGE_ICON_HEIGHT_V1,
        .width = 12, .height = 12
    };
    if (resource.slot) {
        config.variant = CUI_BRIDGE_BADGE_ICON_V1;
        config.artwork = resource;
    }
    const char *name = resource.slot ? "Inbox status" : 0;
    cui_bridge_node badge, button;
    status = cui_bridge_badge_create_v1(
        app, parent, "inbox-badge", &config, resource.slot ? &artwork : 0,
        name, name ? 12 : 0, 0, 0, &badge);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = cui_bridge_node_create(app, badge, CUI_BRIDGE_BUTTON, "inbox", &button);
    if (status == CUI_BRIDGE_OK)
        status = cui_bridge_set_text(app, button, "Inbox");
    if (status != CUI_BRIDGE_OK)
        cui_bridge_node_remove(app, badge);
    else {
        *badge_out = badge;
        *button_out = button;
    }
    return status;
}
```

The platform's ordinary button events identify `button_out`. The Badge
indicator exposes status semantics without taking keyboard focus. A numeric
indicator uses its displayed count as its default accessible name. A custom
name applies to the status marker. The artwork remains decorative.

## Configure and inspect

- `cui_bridge_badge_versions` bit 0 advertises the complete V1 group. Resolve
  the full group and match exact record sizes when loading dynamically.
  `cui_bridge_badge_record_size_v1` returns 64 for configuration and 40 for
  results. When supplying owned artwork, also check the Icon resource and
  presentation records. [Icon options and limits](BRIDGE_ICON_PRESENTATION.md).
- The default maximum is 99. Larger counts display the maximum followed by
  `+`. Maximum zero is explicit. Count and maximum must fit the host
  `size_t`. Dot uses the same shell and design sizes. Custom shell color uses
  `CUI_BRIDGE_BADGE_CUSTOM_COLOR_V1` and `0xRRGGBBAA`.
- Icon mode takes either a nonempty named catalog asset or a resource handle
  and its complete Icon options. Count and dot require no artwork. Named mode
  keeps the C component's named artwork sizing. Use an owned resource to
  inspect the exact artwork result.
- `cui_bridge_badge_configure_v1` replaces the copied options and two optional
  UTF-8 strings atomically. A failed update preserves the previous resource,
  children, and result. Remove or reconfigure the node before destroying a
  referenced resource. Use `cui_bridge_set_disabled` to disable its child
  controls. Use the dedicated configure API for size and presentation.
- Call `cui_bridge_badge_get_result_v1` after stepping. `submitted` records
  composition admission and `frame_status` records the step result. Read
  `icon_error` only when `CUI_BRIDGE_BADGE_ICON_OBSERVED_V1` is set. An artwork
  error preserves content and the shell. Reconfiguration, omission and steps
  stopped before submission return `CUI_BRIDGE_EMPTY`. A successful frame
  with a visible indicator also exposes its bounds. Result reads leave the
  event queue untouched.

Use these APIs on the owning thread between steps. Each node copies at most
4096 bytes for its status name and 4096 for its named Icon. The app uses one
lazy slot table bounded by its existing node limit. Count and dot allocate
no SVG resource slots. Resource cache collection remains explicit.
