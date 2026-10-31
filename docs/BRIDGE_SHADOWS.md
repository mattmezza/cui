# Typed root shadows in language builders

The additive bridge V1 implementation targets only Row, Column, Label, Button,
Input and Badge paint roots. This is two authoring containers and four catalog
families. Compound indicators, tracks, repeated items, popup panels, triggers,
backdrops and every other node kind remain unsupported. Bundled C headers do not
expand that typed scope. Focused Linux portable software and all five facade checks pass for this bounded
subset. Integration into a released candidate remains pending.

A style owns at most eight mixed DROP/INSET layers and four explicit corner
radii. Ordinary layers use Gaussian sigma; the separate CSS factory accepts
blur radius and delegates radius/2 conversion to Core. Presets also delegate to
Core, with no language-specific preset math. Numeric packed colors are
`0xRRGGBBAA`. The typed facades reject nonfinite values, float32 overflow, sigma
outside[0,256], offsets/spread outside[-4096,4096], CSS radius outside[0,512],
radii outside[0,16384] and more than eight layers.

Authoring copies arrays/slices and values before returning, including before
mount. No caller-owned layer buffer is retained by the bridge. Successful set
replaces the entire list; rejected replacements preserve it. `box_shadows([])`
(or an empty typed style) and preset NONE explicitly suppress both DROP/INSET
on every following frame. Reset removes the override and restores component
defaults. Node/subtree removal and app destruction follow existing stale-handle
and disposed-object rules; recycled slots start without an override.

The bridge stores 224 bytes per node (73,128 bytes for the complete private node on the
validated Linux x86-64 build) and applies the retained value after the
owner finishes submitting each real painted frame. Existing component defaults
run first. Inert ancestry still paints; measurement/layout-only work does not
apply decoration. Style changes neither layout, hit regions, focus, semantics
nor input consumption. A missing expected root is a frame error. Radii are
explicit; presets do not guess theme/state-dependent component radii. The Core
paint, clipping, resource and renderer limits remain in [ELEMENT_SHADOWS.md](ELEMENT_SHADOWS.md).

Python values are immutable and sequences are copied:

```python
from cui_bridge import (App, BoxShadow, ShadowKind, ShadowPreset, CornerRadii,
                        button, column)
radii = CornerRadii(4, 4, 4, 4)
layer = BoxShadow.css(0, 2, 8, -1, rgba=0x00000040)
node = button("save").label("Save").box_shadows(
    [layer, BoxShadow(sigma=1, kind=ShadowKind.INSET)], radii)
with App(width=360, height=240) as app:
    app.mount(column().children(node))
    app.step()
    node.shadow_preset(ShadowPreset.MD, radii)
    node.box_shadows([])       # explicit suppression
    node.reset_box_shadows()   # restore defaults
    app.step()
```

Python CSS/preset values are immutable requests, normalized through the mounting
or updating App's own bridge. They never implicitly load a different default
library. Go factory methods require an active App; its loader enforces one
library identity. JS uses its one addon/runtime and Rust its linked runtime.
Python and Go preserve old default library loads; requesting the new feature
against an older library fails explicitly. Match generated headers, facade and
runtime artifacts. ABI version1 alone does not prove new symbols exist.

JavaScript and TypeScript share the same validated runtime and immutable values:

```javascript
const {App, BoxShadow, ShadowKind, ShadowPreset, CornerRadii,
       button, column} = require('@cui/native');
const radii = new CornerRadii(4, 4, 4, 4);
const node = button('save').label('Save').box_shadows([
  BoxShadow.css(0, 2, 8, -1, 0x00000040),
  new BoxShadow(0, 0, 1, 0, ShadowKind.INSET)
], radii);
const app = new App({width:360, height:240});
try {
  app.mount(column().children(node)); app.step();
  node.shadow_preset(ShadowPreset.MD, radii);
  node.box_shadows([]); node.reset_box_shadows(); app.step();
} finally { app.dispose(); }
```

The TypeScript constructor results expose styling only for the six supported
kinds; shared Label/Input interfaces do not grant it to derived compound controls.

Go factory methods use the creating App's loaded bridge; all mutations stay on
its existing owner thread. `BoxShadows` copies the supplied style's layer slice:

```go
radii := cui.CornerRadii{TopLeft:4, TopRight:4, BottomLeft:4, BottomRight:4}
layer, err := app.ShadowCSS(0, 2, 8, -1, 0x00000040, cui.ShadowDrop)
if err != nil { return err }
node := cui.Button("save").Text("Save").BoxShadows(cui.ShadowStyle{
    Layers: []cui.BoxShadow{layer, {Kind:cui.ShadowInset, Sigma:1, RGBA:0x00000040}},
    Radii: radii,
})
if err := app.Mount(cui.Column("root").Children(node)); err != nil { return err }
preset, err := app.ShadowPreset(cui.ShadowMD, radii)
if err != nil { return err }
node.BoxShadows(preset).BoxShadows(cui.ShadowStyle{}).ResetBoxShadows()
if err := node.Err(); err != nil { return err }
return app.Step(.016)
```

Rust uses typed enums and owned safe values, with a separate fixed-layout FFI
record. `App::box_shadows` and fluent Builder methods use the same copying bridge:

```rust
let radii = CornerRadii { top_left:4., top_right:4., bottom_left:4., bottom_right:4. };
let layer = BoxShadow::css(0., 2., 8., -1., 0x00000040, ShadowKind::Drop)?;
let style = ShadowStyle::new(&[layer], radii)?;
app.box_shadows(button, &style)?;
app.shadow_preset(button, ShadowPreset::MD, radii)?;
app.box_shadows(button, &ShadowStyle::new(&[], radii)?)?;
app.reset_box_shadows(button)?;
app.step(0.016)?;
```

This follows existing setter/callback mutation rules; no new callback phase or
notification category is introduced. See the signed V1 checkpoint in
[BRIDGE_ELEMENT_SHADOW_AUTHORING.md](design/BRIDGE_ELEMENT_SHADOW_AUTHORING.md)
for exact sizes, offsets, capability query, error precedence and deferred mapping.

For agent discovery, read the exact bridge header and typed definition modules:
`include/cui/cui_bridge.h`, `bindings/python/cui_bridge/shadows.py`,
`bindings/node/shadows.cjs`, `bindings/typescript/index.d.ts`,
`bindings/rust/src/shadows.rs`. The consumer skill
exports these narrowly selected definitions; inspect the matching checked-out
`bindings/go/cui/native.go` for Go value/method declarations; C declarations remain authoritative
in the bridge header. Pass `--source /path/to/actual-app-cui-source` on helper
reads and use facade sources matching that app's pinned revision. A minimal C SDK
snapshot alone is not a foreign authoring implementation. Never substitute the
installed skill's newer module when the pinned app lacks that API.


## Focused validation

Native tests run through the private-Xvfb wrapper; no desktop session is used.
The six-root oracle compares box pixels with TEXT commands excluded from the
fontless software renderer. Original text command count, copied UTF-8 content,
bounds, font size/style/color, overall root geometry and semantic count are
checked independently. It does not claim glyph rasterization or hardware GPU
proof. Other checks cover replacement/clear/reset, invalid replacement atomicity,
eight mixed layers, slot recycling, nested supported roots, measurement/inert/
layout-only/absent passes, missing-target frame errors and unchanged activation.

The Python, JS, Go and Rust tests exercise actual runtime ownership, geometry,
callbacks and validation. Strict TypeScript checks reject unsupported constructor
results. Python tests cover deferred owning-App factory resolution and optional
missing symbols; Go and Rust additionally run against an actual older shared
library. No browser shadow renderer or cross-platform parity is claimed.

Reproduce in one existing portable Release build directory, with local CPU limits:

```sh
nice -n 19 taskset -c 0 cmake --build "$BUILD" --parallel 1 --target cui_bridge test_bridge_shadows test_bridge_core test_bridge_components
nice -n 19 taskset -c 0 python scripts/test_local.py --build-dir "$BUILD" -R '^(bridge_shadows|bridge_core|bridge_components|bridge_python|bridge_python_shadows)$' -j1
nice -n 19 taskset -c 0 python scripts/test_shadow_runtimes.py --build-dir "$BUILD" --old-library "$OLD_BRIDGE"
```

The focused runtime helper compiles only the N-API adapter against that already
built library, then registers temporary CTest entries inside the same directory.
It uses the private display wrapper, Go's `cui_dynamic` tag, offline single-job
Cargo and no dependency downloads. The regular Python/Node/Go/Rust CI suites also
include these regression files; TypeScript uses its pinned compiler.


The original root setter above keeps its six-kind allowlist. The separate
[semantic target API](BRIDGE_PART_SHADOWS.md) adds bounded Checkbox/Radio
indicators, Switch track/thumb, Slider track and explicit outer ROOT roles in a
reviewed ordinary-tree topology. It does not infer every compound/portal part.
