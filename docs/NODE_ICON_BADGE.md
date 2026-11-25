# Retained Icons and Badges in JavaScript and TypeScript

Use `configured_icon()` to display an app-owned SVG in a retained layout and
`configured_badge()` to put a count, dot or icon on ordinary child widgets.
One `IconResource` can supply multiple Icons and Badges. Each use can set its
own dimensions, color, transform, box style and constraints. The Badge
indicator adds no layout space or focus stop.

## Build and run the inbox

Use Node 18 or newer, Node-API headers, Python 3, CMake 3.20+, Ninja and a C11
compiler. For the native example, use the SDL and font prerequisites in the
build guide. [Binding build prerequisites](BINDING_BUILDERS.md).

```sh
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=ON
cmake --build build-node --target cui_node_bindings
export CUI_NODE_ADDON="$PWD/build-node/cui.node"
node bindings/node/examples/icon-badge-inbox.cjs
node bindings/node/examples/icon-badge-inbox.cjs third_party/inter/Inter-Regular.otf
```

The first invocation exercises the inbox headlessly. The second opens the app.
Click **Mark all as read** to hide the count while the button stays in place.
For headless-only builds, set `-DCUI_BUILD_NATIVE=OFF`.

To compile and execute the TypeScript example with an installed `tsc`:

```sh
python bindings/node/test/run-icon-badge-types.py \
  --tsc "$(command -v tsc)" --node "$(command -v node)" \
  --output build-node/icon-badge-types
```

[JavaScript inbox](../bindings/node/examples/icon-badge-inbox.cjs) ·
[TypeScript inbox](../bindings/typescript/examples/icon_badge_inbox.ts).

```js
const {App, IconSource, button, column, configured_badge, configured_icon} =
  require('./bindings/node');

const app = new App();
try {
  const check = app.icon_resource(IconSource.named('check'));
  const icon = configured_icon('workspace', check, {width: 24, height: 24}, {name: 'Synced'});
  const ready = configured_badge('ready', {variant: 'icon', artwork: check,
    artworkOptions: {width: 12, height: 12}, name: 'Synced'});
  const unread = configured_badge('unread', {count: 8n, name: 'Unread messages'});
  const clear = button('clear').primary().label('Mark all as read').on_click(() => {
    unread.configure({...unread.get_config(), count: 0n});
  });
  app.mount(column().gap(12).children(icon,
    ready.children(button('outbox').label('Outbox')), unread.children(clear)));
  app.step();
} finally {
  app.dispose();
}
```

## Configure, inspect and release

- Probe `app.supports_icon_resources()`, `app.supports_icon_presentation()` and
  `app.supports_configured_badge()`. The alpha.4 C library supports the owned
  resource group. Configured nodes need the current bridge and addon. Legacy
  `icon()` and `badge()` work with their existing catalog and text APIs.
- `configure()` replaces the copied options. Failed updates preserve the
  previous configuration. `get_config()` returns a read-only snapshot. Width,
  height, opacity and color accept explicit zero. Use `null` for inherited or
  unspecified optional values and numeric `0xRRGGBBAA` for explicit colors.
  A configured Icon can use `reset()` when it was created with an embedded
  `fallback` name.
- Count and maximum accept safe integer numbers or `bigint` within the native
  `size_t` range, and snapshots always use `bigint`. A count of zero hides the
  marker. An explicit maximum of zero displays `0+` for a positive count.
  Use `namedIcon` or `artwork` for the icon variant.
- Use `result()` after `app.step()` to read the frame sequence and frame
  status. Artwork preparation errors are separate. A named Badge has
  `iconError: null`. An omitted node has no result for the latest frame.
  `indicator_bounds()` reports the committed Badge geometry when it exists.
- Child actions use ordinary callbacks dispatched after the step. Reading a
  snapshot leaves that event queue untouched. Use `badge.disabled()` to make
  the subtree inert and `badge.remove()` to detach it.
- A resource belongs to the same app as its consuming nodes. Reset, remove
  or reconfigure those nodes before calling `resource.close()`. App disposal
  releases the nodes and resources. Arbitrary render-time SVG loader callbacks
  and configured Spinner are outside these Node builders.

[Resource guide and limits](NODE_ICON_RESOURCES.md) ·
[C Icon presentation contract](BRIDGE_ICON_PRESENTATION.md) ·
[C Badge contract](BRIDGE_BADGE.md).
