# Owned SVG resources from JavaScript and TypeScript

An `IconResource` lets a Node app own an SVG, prepare it at different sizes
and copy its pixels. Use a catalog name, a resource-local asset key or raw
SVG. The catalog, source limits and raster cache come from the loaded C bridge.

## Try it

Use Node 18 or newer, Node-API headers, Python 3, CMake 3.20+, Ninja and a C11
compiler. These commands build the headless addon and run the example from
the repository root. Use the platform-specific addon path if your generator
puts it in a configuration directory. [Native build guide](BINDING_BUILDERS.md).

```sh
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build-node --target cui_node_bindings
CUI_NODE_ADDON="$PWD/build-node/cui.node" node bindings/node/examples/icon-resource.cjs
```

```js
const {App, IconError, IconSource} = require('./bindings/node');
const app = new App();
try {
  if (!app.supports_icon_resources()) throw new Error('Update the CUI bridge');
  const artwork = app.icon_resource(IconSource.named('check'), {maxVersions: 4});
  try {
    if (artwork.prepare(24, 24) === IconError.OK) {
      const {rgba, stride, paint} = artwork.bitmap();
      console.log(rgba.length, stride, paint);
    }
  } finally {
    artwork.close();
  }
} finally {
  app.dispose();
}
```

The example installs a local asset from SVG bytes and reads its pixels.
[Runnable example](../bindings/node/examples/icon-resource.cjs).

## Source and lifetime

- `IconSource.named(name)` uses the C catalog. `IconSource.path(key)` accepts
  a key supplied through `put_asset(key, svg)`. Path keys resolve in the
  resource-local copied map and the embedded catalog. Render-time custom
  loader callbacks are direct C APIs.
- `IconSource.svg(svg)` and `put_asset()` accept a string or `Uint8Array` of
  strict UTF-8 without NUL. `set_source()`, map edits and `reload()`
  invalidate preparation.
- Call `prepare(width, height, transform?)` to rasterize. Check its returned
  `IconError` before reading `bitmap()`. After a failed prepare, the last
  successfully rasterized pixels can still be available. Pixel buffers are
  independent copies in packed straight RGBA8.
- `info()` returns counters and revisions as `bigint`. Inspect and update cache
  limits with `limits()` and `set_limits()`. `collect()` releases inactive
  masks after synchronous rendering.
- Use the resource on its app's Node thread. Call `resource.close()` to
  release it during the app lifetime. Disposing the app releases any
  outstanding resources. JavaScript garbage collection of the resource
  wrapper leaves the native resource owned by the app.
- Use `icon_catalog_count()`, `icon_catalog_entry()`, `query_icon()` and
  `copy_icon_asset()` to inspect the current catalog and copy embedded SVG.

Current Node and TypeScript source supports these resource operations against
alpha.4's C bridge. Use the current bridge and addon to share these resources
between the configured retained Icon and Badge builders.
[Configured Icon and Badge inbox](NODE_ICON_BADGE.md).
[Bridge contract and limits](BRIDGE_ICON_RESOURCES.md).
