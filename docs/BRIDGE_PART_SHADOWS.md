# Typed semantic part shadows

The additive target V1 API styles a bounded set of existing component surfaces.
It does not expand the old six-root `box_shadows` allowlist, name every component
part, or provide a style cascade. Focused portable implementation checks pass; integration remains pending. See the
[review checkpoint](design/BRIDGE_PART_SHADOW_AUTHORING.md) for decisions.

| Direct bridge node | ROOT | MAIN alias | Additional roles |
| --- | --- | --- | --- |
| Row, Column, Label, Button, Badge, Input | Existing root | ROOT | None |
| Checkbox, individual Radio | Whole label row | INDICATOR | INDICATOR |
| Switch | Whole label row | TRACK | TRACK, THUMB |
| Slider | Framed label/control | TRACK | TRACK only; no thumb exists |
| RadioGroup | Group container | Unsupported | None |
| ITEM immediately owned by RadioGroup | Item label row | INDICATOR | INDICATOR |

The first target API accepts ordinary Row/Column ancestor paths. A RadioGroup
ITEM permits its immediate group parent and Row/Column ancestors above that.
Settings-owned fields, other ITEM owners, panel callbacks, portals and unlisted
kinds remain unsupported. A semantic handle with the same component name is not
proof that its owner uses the direct painter. Query the mounted node's capability.
This topology check applies only to the new target operations; old six-root
setters retain their existing contract.

ROOT never silently selects an indicator. MAIN aliases exactly one physical slot;
setting/resetting either spelling changes that same slot. ROOT and each other
physical role are independent. Each list copies at most eight mixed DROP/INSET
layers and explicit TL/TR/BL/BR radii. Numeric colors are0xRRGGBBAA. Reuse
[typed values and canonical CSS/preset factories](BRIDGE_SHADOWS.md); sigma and
CSS blur radius retain their distinct meanings. No radii are inferred from theme
or control state.

Empty lists and preset NONE explicitly suppress both shadow kinds; reset removes
an override and restores defaults. Explicit empty indicator styles still opt into
the named C painter. Reset returns standalone Checkbox/Radio to the legacy
anonymous indicator route even when a ROOT override remains.

## RadioGroup identity boundary

When any ITEM indicator has an authored override, including empty, the group uses
one `cui_radio_group_with_parts` call and names **every** current peer indicator.
Only authored targets receive overrides. Unstyled peer paint identities and
explicit-ID registration/capacity costs change in that opted-in group, while their
normal layer defaults remain intact. ROOT-only styles do not activate naming.
The last indicator reset or removal returns the group to its legacy anonymous
route. Reorder associates styles/derived IDs with stable retained item identity,
not array position. No partial-naming C API or per-item interactive Radio reducer
is introduced.

## C bridge record

With a live bridge node handle in the documented topology, query capability
before requesting an indicator override. Values are copied; keep ordinary status
handling and check the later frame result independently:

```c
cui_bridge_shadow_target_v1 target = {sizeof(target), 1,
    CUI_BRIDGE_SHADOW_INDICATOR, 0};
cui_bridge_shadow_target_info_v1 info;
cui_bridge_status status = cui_bridge_shadow_target_query_v1(
    app, node, sizeof(target), &target, sizeof(info), &info);
if (status == CUI_BRIDGE_OK && (info.flags & CUI_BRIDGE_SHADOW_SUPPORTED)) {
    cui_bridge_box_shadows_v1 style;
    status = cui_bridge_shadow_preset_v1(4 /* MD */, 3, 3, 3, 3,
        sizeof(style), &style);
    if (status == CUI_BRIDGE_OK)
        status = cui_bridge_set_target_box_shadows_v1(app, node,
            sizeof(target), &target, sizeof(style), &style);
}
```

A successful query with flags0 is declared unsupported, not a temporarily
missing indicator. Reset uses `cui_bridge_reset_target_box_shadows_v1` with the
same descriptor. Style count0 is explicit suppression rather than reset.

## Python

```python
from cui_bridge import (App, BoxShadow, ShadowPart, ShadowPreset,
                        checkbox, column, radio_group, item)
check = checkbox('check', 'Check').part_shadows(
    ShadowPart.MAIN, [BoxShadow.css(0, 1, 8, rgba=0x00000060)])
choice = item('a', 'Choice').part_shadows(ShadowPart.INDICATOR, [])
with App() as app:
    app.mount(column().children(check, radio_group('g').children(choice)))
    info = check.shadow_target(ShadowPart.MAIN)  # capability, before first paint
    assert info.supported and info.canonical_part == ShadowPart.INDICATOR
    app.step()
    check.part_shadow_preset(ShadowPart.ROOT, ShadowPreset.SM)
    check.reset_part_shadows(ShadowPart.MAIN)
    app.step()
```

Python immutable CSS/preset requests normalize through the owning App's selected
library during mount/update, preserving the existing provider/revision contract.
An unmounted generic ITEM can store a request, but a wrong owner fails mounting
with the existing rollback rules. Query requires a mounted node; no implicit App
or globally loaded library is created.

## JavaScript and TypeScript

```javascript
const {App, ShadowPart: P, BoxShadow, checkbox, slider, column,
       radio_group, radio_item} = require('@cui/native');
const check = checkbox('check', 'Check').part_shadows(P.INDICATOR, [new BoxShadow(0,1,2)]);
const choice = radio_item('a', 'Choice').part_shadows(P.MAIN, []);
const app = new App();
try {
  app.mount(column().children(check, slider('value'), radio_group('g').children(choice)));
  if (check.shadow_target(P.INDICATOR).supported) app.step();
  check.part_shadows(P.INDICATOR, []).reset_part_shadows(P.MAIN); app.step();
} finally { app.dispose(); }
```

TypeScript constructor results restrict roles, so Slider THUMB and RadioGroup
MAIN fail type checking. `radio_item` is an ITEM authoring convenience requiring
RadioGroup ownership at mount; generic `item` makes no typed indicator promise.
Runtime ownership validation still applies. JS retains its existing failed-mount
App disposal behavior.

## Go

```go
layer, err := app.ShadowCSS(0, 1, 8, 0, 0x00000060, cui.ShadowDrop)
if err != nil { return err }
check := cui.Checkbox("check").Text("Check").PartShadows(cui.ShadowMain,
    cui.ShadowStyle{Layers: []cui.BoxShadow{layer}})
if err := app.Mount(cui.Column("root").Children(check)); err != nil { return err }
info, err := check.ShadowTarget(cui.ShadowIndicator)
if err != nil { return err }
if !info.Supported() { return fmt.Errorf("unsupported target") }
check.PartShadowPreset(cui.ShadowRoot, cui.ShadowSM, cui.CornerRadii{}).
    ResetPartShadows(cui.ShadowMain)
if err := check.Err(); err != nil { return err }
return app.Step(.016)
```

Go copies the supplied slice. CSS/preset factories use the owning App;
`PartShadowPreset` requires a mounted node. For an unmounted value use
`app.ShadowPreset`, then `PartShadows`. New symbols remain optional for default
older-library loads; feature use fails explicitly. Go methods retain fluent
sticky-error and existing owner-thread rules.

## Rust

```rust
let style = ShadowStyle::new(&[BoxShadow::css(0.,1.,8.,0.,0x00000060,
    ShadowKind::Drop)?], CornerRadii::default())?;
app.part_box_shadows(check, ShadowPart::Main, &style)?;
assert!(app.shadow_target(check, ShadowPart::Indicator)?.supported());
app.part_shadow_preset(check, ShadowPart::Root, ShadowPreset::SM,
    CornerRadii::default())?;
app.reset_part_box_shadows(check, ShadowPart::Indicator)?;
app.step(0.016)?;
```

Rust typed enums/owned styles and fluent Builder methods use the same wire call;
app identity, stale handle and Result errors retain their current rules.

## Capability, errors and lifetime

The exact C authority is `include/cui/cui_bridge.h`: target/info records are16
bytes with four uint32 fields; layers/styles remain24/224 bytes. Query reports
SUPPORTED and NAMED_INDICATOR flags and a canonical role. Unsupported known roles
return supported=false; malformed descriptors or stale handles are errors.
Query neither allocates nor reports current paint presence. The initial topology
has no internally conditional mapped part, so its CONDITIONAL_SUBMISSION flag is0.

Unvisited owners and measurement/layout-only work retain values without applying
styles or reporting missing paint. Inert ancestry still paints. A mandatory part
missing during actual owner submission, identity collision or renderer/layout
capacity failure remains a frame failure. It is not silently treated as normal
omission. Such late failure may follow Base input reduction/owner callbacks and
does not roll back accepted interaction state. No old bounds or semantic lookup
is used to decide admission.

Each setter validates and acquires storage before committing its one target;
invalid or out-of-memory replacement preserves old C/local state. Separate calls
are separate transactions. Node/subtree removal frees storage; slot reuse begins
with no part overrides. A nullable extension adds8 idle bytes/node on the measured
Linux x86-64 ABI: total73,136 versus73,128 previously. Active payloads are232
bytes for one indicator/track or456 for Switch's two parts, excluding allocator
metadata. ROOT reuses the existing224-byte inline field; MAIN uses no extra slot.

The matching typed modules are `bindings/python/cui_bridge/shadows.py`,
`bindings/node/shadows.cjs`, `bindings/go/cui/part_shadows.go` and
`bindings/rust/src/part_shadows.rs` plus the shared style modules. Match source,
facade and runtime revisions; installed-skill resources do not upgrade an older
app. Existing Core clipping, opacity and renderer budgets remain in
[ELEMENT_SHADOWS.md](ELEMENT_SHADOWS.md). Browser/GPU, other compound parts and
remaining78-family coverage are separate pending work.


## Validation boundaries

The focused C fixture checks actual software part-only paint/reset, unchanged
root geometry/semantic counts and queued activation, all64 group peer identities,
ROOT-only legacy anonymity, last reset/removal, unsupported topology with old
six-root behavior preserved, no-allocation query, failed initial allocation,
invalid replacement, reuse/destruction, inert/measurement/layout-only/omission and
unexpected mandatory-part failure/recovery. It compares fontless box paint only;
text/glyph rendering, browser and GPU proof are not added here.

Python, JS, Go and Rust runtime regressions cover owned values, aliases,
capability/geometry, callbacks and lifetime/rollback. Python optional-symbol
proxies and actual older Go/Rust libraries exercise feature failure without
breaking default loads. Strict TypeScript checks cover role restrictions.
Reproduce with the same focused build and private display wrapper:

```sh
nice -n 19 taskset -c 0 cmake --build "$BUILD" --parallel 1 --target test_bridge_part_shadows cui_bridge
nice -n 19 taskset -c 0 python scripts/test_local.py --build-dir "$BUILD" -R '^bridge_(part_shadows|python_part_shadows)$' -j1
nice -n 19 taskset -c 0 python scripts/test_shadow_runtimes.py --build-dir "$BUILD" --parts --old-library "$OLD_BRIDGE"
```

The helper reuses the existing shared bridge and build directory, compiles only
the N-API adapter and required Go/Rust facade tests, uses Go `cui_dynamic`, and
runs offline/single-job via private-Xvfb CTest entries. Static source-package
closure and other platforms remain CI checks rather than extra local CUI builds.
