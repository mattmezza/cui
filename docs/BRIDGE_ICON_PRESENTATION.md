# Retained Icon presentation in C

Use this optional bridge API to display SVG data, or an app-owned catalog or
path resource, in a retained `CUI_BRIDGE_ICON` node. Each node can set its own
size, tint, opacity, transform, box styling, constraints and accessible name.
Unconfigured nodes keep the named Icon behavior. This API is in working source.
The alpha.4 library provides the [resource utilities](BRIDGE_ICON_RESOURCES.md).

## Add an SVG Icon

Call this helper with an existing app and parent. Keep both returned handles for
updates and cleanup. It uses the same [Icon drawing rules](ICON.md) as a CUI frame.

```c
#include "cui/cui_bridge.h"

typedef struct {
    cui_bridge_node node;
    cui_bridge_icon_handle_v1 resource;
} workspace_icon;

cui_bridge_status add_workspace_icon(cui_bridge_app *app, cui_bridge_node parent,
                                     workspace_icon *out) {
    if (!(cui_bridge_icon_versions() & 1u) ||
        !(cui_bridge_icon_presentation_versions() & 1u))
        return CUI_BRIDGE_INVALID_STATE;

    workspace_icon icon = {0};
    const cui_bridge_icon_limits_v1 limits = {
        .struct_size = sizeof limits, .schema_version = 1
    };
    cui_bridge_status status =
        cui_bridge_icon_create_v1(app, &limits, &icon.resource);
    if (status != CUI_BRIDGE_OK)
        return status;

    static const char svg[] =
        "<svg width='16' height='16'><rect x='3' y='3' width='10' height='10' "
        "rx='2'/></svg>";
    status = cui_bridge_icon_set_source_v1(
        app, icon.resource, CUI_BRIDGE_ICON_DATA_V1, svg, sizeof svg - 1);
    if (status == CUI_BRIDGE_OK)
        status = cui_bridge_node_create(
            app, parent, CUI_BRIDGE_ICON, "workspace-icon", &icon.node);
    if (status == CUI_BRIDGE_OK) {
        const cui_bridge_icon_options_v1 options = {
            .struct_size = sizeof options, .schema_version = 1,
            .flags = CUI_BRIDGE_ICON_WIDTH_V1 | CUI_BRIDGE_ICON_HEIGHT_V1 |
                     CUI_BRIDGE_ICON_COLOR_V1,
            .width = 24, .height = 24, .color_rgba = UINT32_C(0x268369ff)
        };
        static const char name[] = "Workspace";
        status = cui_bridge_icon_configure_node_v1(
            app, icon.node, icon.resource, &options, name, sizeof name - 1);
        if (status != CUI_BRIDGE_OK)
            cui_bridge_node_remove(app, icon.node);
    }
    if (status != CUI_BRIDGE_OK)
        cui_bridge_icon_destroy_v1(app, icon.resource);
    else
        *out = icon;
    return status;
}
```

When the parent control already has a label, set
`CUI_BRIDGE_ICON_DECORATIVE_V1` and pass no name. An Icon with an explicit
accessible name contributes Core image semantics without taking keyboard focus.

## Configuration and lifetime

- Both capability groups use bit 0. Dynamically loaded consumers must resolve
  all presentation symbols and check the four exact record sizes before
  calling. `cui_bridge_icon_presentation_record_size_v1` returns 128 for style,
  32 for constraints, 256 for options, and 40 for results. The resource sizes
  use their own query and IDs. Records are native-endian.
- Initialize size and schema version 1 in each present record. Zero reserved
  fields and absent nested records. Set presence flags for width, height,
  opacity, tints, transforms, and individual constraints. Flags allow explicit
  zero or transparent black. Colors are `0xRRGGBBAA`. Options and enabled
  constraints use finite dimensions through 4096. The transform accepts up to
  64 in absolute value for each linear coefficient and 1,000,000 for each
  translation. The C painter supplies the final size, including theme defaults.
- Configuration copies the descriptors and a strict UTF-8 name of up to
  4096 bytes. One payload per configured node and a lazy table of
  `max_nodes + 1` pointers bound the additional retained storage. The app limit
  is 1024 nodes. A failed configure leaves the previous config and result
  intact. Read descriptors and names with `cui_bridge_icon_get_node_config_v1`
  and `cui_bridge_icon_copy_node_name_v1`. The copy size includes the final NUL.
- Several nodes can share one resource and render different immutable masks in
  the same frame. Destroying a referenced resource returns
  `CUI_BRIDGE_INVALID_STATE`. Reset or remove each consuming node before
  destroying the resource. Reset restores the retained legacy name and visual
  size. The corresponding legacy setters require reset first.
- Use the app and its resources on the owner thread between bridge steps.
  After synchronous frame consumption, call `cui_bridge_icon_collect_v1` as
  needed to free inactive masks before the cache limit is reached. The resource
  guide covers supported build modes, limits and pointer preconditions.

After `cui_bridge_step`, initialize a `cui_bridge_icon_result_v1` with its size
and version and call `cui_bridge_icon_get_node_result_v1`. A copied result
includes the node, frame sequence, exact `cui_icon_error`, and the separate
frame status. An artwork error can coexist with a successful step. The query
returns `CUI_BRIDGE_EMPTY` before the first invocation, after reconfiguration or
reset, and when the latest accepted step omitted the Icon or stopped before
composition. Reading a result leaves rendering and resources untouched.

The [retained Badge](BRIDGE_BADGE.md) uses the same owned resources and Icon
options. Python, JavaScript, TypeScript and Go expose those shared resources and
both components. Configurable Spinner and extended Rust authoring remain open.
[Python builders](PYTHON_ICON_BADGE.md) ·
[Node and TypeScript builders](NODE_ICON_BADGE.md) · [Go builders](GO_ICON_BADGE.md).
