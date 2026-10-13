# Portable theme registry and host seam

This optional module adds bounded named themes and source semantic configuration.
Existing `cui_theme` color/button fields, constructors, all 38 preset names and
preset colors remain unchanged. It does not automatically change component
motion, Core preferences, native fonts, or host theme selection.

The reference is GPUI Kit 0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, specifically
`crates/component/src/theme/{schema,registry,motion,mod,color}.rs` and Base
`theme_tokens.rs`. GPUI's serialized shadow layout comes from pinned
`gpui-pre=0.3.8`, `src/style.rs` and `src/color.rs`.

## Semantic configuration

`cui_theme_semantic_import` accepts the actual standalone source wrapper:

```json
{"tokens":{"colors":{"primary":"blue-600/80","foreground":"#1234"},
 "radius":{"md":6,"full":9999},"spacing":{"xxl":32},
 "typography":{"sans":"My embedded font","md":{"size":18,"line_height":26,"weight":500}},
 "shadow":{"sm":[{"color":{"h":0,"s":0,"l":0,"a":0.18},
 "offset":{"x":0,"y":1},"blur_radius":2,"spread_radius":0,"inset":false}]}}}
```

This is separate from the legacy ThemeSet schema. Groups are `colors`, `radius`,
`spacing`, `typography`, and `shadow`; motion is **not** in this JSON schema.
All source semantic groups are represented. There are 17 configurable color
roles; snapshot `selection` is an eighteenth role that source configuration does
not override. Radius has none/sm/md/lg/xl/full; spacing has xxs/xs/sm/md/lg/xl/xxl.
Typography includes sans/mono and xs/sm/md/lg/xl/mono_md, each with size,
line_height and numeric weight. Shadow arrays are sm/md/lg with the exact
serialized BoxShadow fields above; colors inside shadows use normalized HSLA,
not string color names. Optional field `null` preserves an inherited value;
non-optional groups/text-style objects require objects.

String colors support source hex #rgb/#rgba/#rrggbb/#rrggbbaa and 22 named palette
families at scales 50,100,...,900,950; a missing scale uses 500. Black/white and
`/opacity` percentages are supported. Hex is RGBA, not ARGB. Arbitrary CSS
rgb()/hsl() strings and semantic gradients are not accepted by the pinned source
parser. CUI rejects invalid scale/opacity/color values; the source sometimes
falls back or silently ignores invalid overrides. CUI also rejects unknown keys,
duplicate keys (including escaped duplicates), invalid Unicode/NUL, nonfinite
numbers and unsupported fields instead of silently losing data. Decimal parsing
is independent of the host numeric locale.

Snapshots own font strings and shadows. Defaults project generic colors/radii
from the supplied CUI theme, use the source spacing/text scales and source
sm/md/lg elevation metadata. Font defaults are logical `.SystemUIFont` and
`monospace`: this module performs no platform font discovery. Source shadow
metadata has no corresponding legacy CUI shadow-enabled field. Imported
families, weights, line heights, scale-only spacing/radius and elevation remain
available in the returned snapshot for application-owned UI.

Applying tokens updates only generic legacy fields and font_size/radius/radius_lg.
Surface maps to popover; muted background and foreground remain distinct.
Specialized button, sidebar, tab, chart and other component skins stay unchanged,
as in the source's generic projection policy. Selection is not rewritten by
this projection. Hosts must explicitly map the returned typography to their
font factory/style state; storing a family does not make it render automatically.

## Registry ownership and transactions

Allocate a `cui_theme_registry_entry` array, then initialize a registry with it.
No registry API allocates or reads files/network. Capacity is 1..256; seeded
initialization/reset requires room for exactly 38 frozen presets, including
Default Light and Default Dark. Unseeded reset produces an empty registry.
Every entry owns its UTF-8 name, semantic font strings and shadow data. Existing
`cui_theme` icon loader callbacks/userdata remain borrowed; their host lifetime
must cover any use of copied themes. The caller array must outlive the registry
and must not be mutated directly while registered. Reset invalidates all prior
borrowed query pointers; unused array slots are not a second registry.

`put` matches names case-sensitively. Existing names are skipped unless replacement
is explicit; skip does not change revision. Sorted queries put defaults first,
light before dark, then names using ASCII folding with deterministic UTF-8 byte
ordering/ties. This differs from source Unicode lowercasing. No Unicode-insensitive
sorting parity is claimed.

Legacy `cui_theme_registry_import` accepts source ThemeSet shape:
`{name,author,url,themes:[{name,mode,is_default,font.size,font.family,
mono_font.size,mono_font.family,radius,radius.lg,shadow,colors:{dotted keys},highlight}]}`.
The concrete representable color map is `src/theme/cui_theme_legacy_fields.inc`.
It includes 85 source keys; it does not implement every source role, highlight
syntax themes, gradient backgrounds or all derived fallback policies. The
existing preset adaptations remain the authoritative frozen built-ins.
Unsupported legacy fields reject the whole import by default. Explicit
`allow_unsupported` permits the mapped subset and returns `partial=true` plus
`unsupported_fields`; this is never a claim of full legacy import parity.
`is_default` affects ordering, while host selection names remain explicit.

Provide a disjoint caller scratch entry array large enough for the complete
resulting registry, not merely the new entries. Scratch, live entry storage and JSON source bytes must be mutually disjoint.
The optional report must not alias these regions or registry metadata. Scratch contents are temporary and may change on
failure; live registry entries/count/revision do not. All new entries are validated
before a single commit. Source skip-existing is the default batch policy; explicit
replacement is available. Conflicting names within a batch follow the same policy.
Imported source strings may be released immediately after the call. Mutation
increments revision once per successful batch; skipped-only batches leave it
unchanged. Revision overflow rejects. These are UI-thread APIs, not a concurrent
registry: transaction atomicity does not imply cross-thread synchronization.

Limits are explicit: 64 KiB JSON, 4096 parser nodes, depth 16, 127 UTF-8 bytes per
owned name/font, eight shadows per elevation. Parsing stack is bounded (under
2 MiB on usual 64-bit ABIs); avoid invoking it from a tiny host thread stack.
Use caller heap/long-lived storage for large entry arrays and scratch. Geometry
values are bounded to 1e6, text size/line height to 4096 and weight to 1..1000.
These stricter CUI bounds reject atomically instead of truncating. The new module
has no destruction function because its registry/snapshots own no allocations.

## Motion and appearance

`cui_theme_motion_default` represents every pinned MotionTokens field: durations
instant/fast/normal/slow = 0/.12/.18/.28 seconds; enter/exit/move cubic Bézier curves
(.16,1,.3,1),(.4,0,1,1),(.2,0,0,1); control/move springs .18/1/.001 and
.28/.85/.1 with travel enabled; short/medium distances .25/.5 rem.
`cui_theme_bezier_sample` solves x before sampling y. Custom finite Bézier y may
overshoot; x stays in [0,1]. This exposes the semantic policy's Bézier curves,
not every Base Easing enum (steps, piecewise/custom closures).

`cui_theme_motion_project` copies the full policy and converts distances using an
explicit rem pixel size. Reduced projection sets durations, distances, spring
response and travel to zero; it keeps easing/damping/tolerances. Durations/response
are bounded to 60 seconds, damping/epsilon and Bézier y to 1000, rem size to 4096,
rem distances to 4096 and projected distances to 1e6 pixels. Invalid input leaves
output unchanged. Existing component-specific curves (for example Plot's quartic
appear curve) are distinct; no implicit adoption changes those defaults.

`cui_theme_host_update` accepts caller preferences and selection names. It selects
registered light/dark themes in follow-system or forced mode, explicitly projects
motion, and maps reduced motion/auto-hide into the copied scrollbar style.
High contrast chooses CUI's existing high-contrast palette as a local policy
extension; this is not a claim of source/platform high-contrast detection.
Host revision must not rewind. The output owns its snapshots and reports visual
`changed`, typography `metrics_changed` and separate `motion_changed` flags;
repeated identical preferences do not replay changes. The host performs rendering
invalidation, font/style rebinding and Core preference calls as needed.

No OS observer, portal query, filesystem watcher, async callback or global
appearance mutation is installed. A host can read/watch its own appearance and
file events, call a transactional import, and then resolve again using registry
revision. Registry queries borrow until the next mutation; resolved host snapshots
remain owned and valid after registry reset/destruction of caller storage.

## Tests and integration

The dedicated portable unit test covers schema/Unicode/colors, ownership, strict
capacity failures, structured malformed input, rollback, source skip/replace and
case sensitivity, sorting/reset, all 38 preset/button compatibility, full motion
projection, host revisions and no replay. An optional primary-source fixture test reads the pinned Ayu/Aurora JSON,
verifies strict rejection and observable partial imports, then overwrites source
and scratch to check owned entries. Missing reference fixtures return 77.
The native test paints imported and
high-contrast themes with an actual Noto Sans font/software renderer; missing
font fixtures return 77 rather than passing an empty glyph raster.

`tests/web/test_theme_registry.py` builds a standalone pinned Zig 0.16.0 WASM
probe and runs Playwright 1.58.0/Chromium 1208. Real matchMedia preference events
feed the host seam and change actual canvas pixels/typography/motion snapshots.
This validates the explicit host contract, not an SDK-installed browser observer.
`--chromium` can select revision 1228.

Integrator registration: add `src/theme/cui_theme_registry.c` and
`cui_theme_motion.c`, package `cui_theme_registry.h`, embed the private JSON,
equality, palette and legacy map files in amalgamation, register
`tests/unit/test_theme_registry.c` (cui) and
`tests/integration/test_theme_registry_native.c` (cui+cui_text+cui_software,
skip 77). Default builds stay Rust-free. Root owns CMake/distribution/notice
updates. Stale distribution verification is expected before that regeneration.

Programmatic `put` treats `.theme` as authoritative for the seventeen shared generic
color roles, md/lg radius, and md font size. Conflicting `.semantic` values in
those fields are normalized. Selection, other scales/text styles, font families,
spacing, shadows, and motion remain intact. Use semantic JSON/the projector or
update both snapshots when editing shared semantic values.
