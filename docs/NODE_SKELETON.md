# Responsive Skeleton in JavaScript and TypeScript

Use `responsive_skeleton` in a Node-authored CUI tree while content loads.
A Skeleton is decorative. Keep an application-owned loading status and remove
the placeholders once the content is ready.

```js
const {App, column, label, responsive_skeleton} = require('@cui/native');

const avatar = responsive_skeleton('avatar', {
  widthMode: 'fixed', width: 40, height: 40, radius: 20,
});
const name = responsive_skeleton('name', {width: .65, radius: 8});
const role = responsive_skeleton('role', {
  width: .4, height: 12, radius: 6, secondary: true,
});
const description = responsive_skeleton('description', {height: 36, radius: 8});

const app = new App({width: 480, height: 320});
try {
  app.mount(column().gap(8).children(
    label('Loading profile…'), avatar, name, role, description,
  ));
  app.step();
  const sample = description.presentation();
  if (sample && sample.nextWakeSeconds > 0) {
    const delay = sample.nextWakeSeconds - sample.coreTime;
    console.log(`Next update in ${delay} seconds`);
  }
} finally {
  app.dispose();
}
```

A default Skeleton fills its parent's inner width and is 16 logical pixels
tall. Fractional widths resolve against a parent with a defined width. Fixed
width and grow can have a cap through `maxWidth`. Use
`{widthMode: 'grow', width: 0, maxWidth: 128}` for capped flexible space.
Fractional widths require `maxWidth: 0`. Fixed width, height, caps and radii use
logical pixels.

The active preset supplies the fill. `secondary: true` halves its alpha.
`backgroundRgba` replaces the fill with a numeric `0xRRGGBBAA` value, including
`0` for transparent black. The default pulse lasts two seconds.
`reducedMotion: true` keeps the fill static.

`get_config()` returns an immutable copy, usable in
`shape.configure({...shape.get_config(), reducedMotion: true})`. Cosmetic
changes preserve the pulse. Changing the period or reduced-motion policy
restarts it. Remove the mounted node with `shape.remove()` when loading ends.
The [complete JavaScript example](../bindings/node/examples/skeleton-profile.cjs)
wires a button to remove all four placeholders and display profile content.

`presentation()` returns a frozen sample, with a `bigint` frame number. It returns
`undefined` before a successful frame, after reconfiguration, and when the node
was omitted. The deadline shares the Core clock advanced by stepping. Zero
means the placeholder asks for no wake. The native runner retains its cadence.

## Run the examples

From a checkout with the responsive addon and matching bridge:

```sh
cmake -S bindings/node -B build-node -G Ninja
cmake --build build-node --target cui_node_bindings
CUI_NODE_ADDON="$PWD/build-node/cui.node" node bindings/node/examples/skeleton-profile.cjs
```

The default example runs headlessly and exercises the button. For a visible
window, build with `-DCUI_BUILD_NATIVE=ON -DCUI_REQUIRE_NATIVE=ON` and add a
TTF or OTF path after the example filename. See [native prerequisites](BINDING_BUILDERS.md).

The [TypeScript example](../bindings/typescript/examples/skeleton_profile.ts)
uses the same public factories and types. With an existing TypeScript compiler,
typecheck and run its emitted JavaScript:

```sh
CUI_NODE_ADDON="$PWD/build-node/cui.node" python bindings/node/test/run-skeleton-types.py \
  --tsc "$(command -v tsc)" --node "$(command -v node)" --output build-node/skeleton-types
```

Use `app.supports_skeleton_presentation()` to probe the optional API. The wrapper
checks all six bridge exports, the V1 capability bit, and both record sizes
before mounting. The legacy `skeleton(key, text)` and `.visual_size(width, height)`
calls work with the published alpha.4 library. This responsive builder targets
a future release. Direct C offers custom easing and the full Core box descriptor.
